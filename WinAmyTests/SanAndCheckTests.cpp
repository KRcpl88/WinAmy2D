#include "TestHelpers.h"
#include "heap.h"
#include "position.h"

#include <string>
#include <vector>

namespace WinAmyTests {

// Regression tests for 4D level-awareness in SAN notation, check detection,
// and pawn double-push legality.  Each played game is driven by a fixed,
// deterministic pseudo-random sequence so the tests are reproducible.
TEST_CLASS(SanAndCheckTests) {
  public:
    TEST_CLASS_INITIALIZE(InitializeEngine) {
        InitMoves();
        InitAll();
        HashInit();
    }

    static std::vector<CMove> CollectLegalMoves(CPosition *pPosition) {
        heap_t hHeap = allocate_heap();
        pPosition->LegalMoves(hHeap);
        std::vector<CMove> rgMoves;
        for (unsigned int nIndex = hHeap->current_section->start;
             nIndex < hHeap->current_section->end; nIndex++) {
            rgMoves.push_back(hHeap->data[nIndex]);
        }
        free_heap(hHeap);
        return rgMoves;
    }

    // SAN must round-trip through ParseSAN for every legal move.
    TEST_METHOD(SanRoundTripsForEveryLegalMove) {
        uint32_t dwRng = 0x1234567u;
        auto Next = [&dwRng]() {
            dwRng = dwRng * 1664525u + 1013904223u;
            return dwRng;
        };
        for (int nGame = 0; nGame < 25; nGame++) {
            CPosition *pPosition = CPosition::Initial();
            for (int nPly = 0; nPly < 35; nPly++) {
                std::vector<CMove> rgMoves = CollectLegalMoves(pPosition);
                if (rgMoves.empty()) {
                    break;
                }
                for (CMove move : rgMoves) {
                    char szSan[32];
                    std::string strSan = pPosition->SAN(move, szSan);
                    CMove parsed = pPosition->ParseSAN(strSan.c_str());
                    Assert::IsTrue(
                        parsed != M_NONE,
                        L"ParseSAN returned M_NONE for a generated SAN");
                    Assert::IsTrue(
                        parsed.GetFromToIndex() == move.GetFromToIndex(),
                        L"SAN did not round-trip to the same move");
                }
                pPosition->DoMove(rgMoves[Next() % rgMoves.size()]);
            }
            CPosition::Free(pPosition);
        }
    }

    // IsCheckingMove must agree with actually making the move and testing for
    // check, for every legal move.  The old heuristic ignored cross-level
    // attacks (and many discovered checks), so it disagreed frequently.
    TEST_METHOD(IsCheckingMoveMatchesActualCheck) {
        uint32_t dwRng = 0x89abcdefu;
        auto Next = [&dwRng]() {
            dwRng = dwRng * 1664525u + 1013904223u;
            return dwRng;
        };
        int nChecks = 0;
        for (int nGame = 0; nGame < 25; nGame++) {
            CPosition *pPosition = CPosition::Initial();
            for (int nPly = 0; nPly < 35; nPly++) {
                std::vector<CMove> rgMoves = CollectLegalMoves(pPosition);
                if (rgMoves.empty()) {
                    break;
                }
                for (CMove move : rgMoves) {
                    bool fReports = pPosition->IsCheckingMove(move);
                    pPosition->DoMove(move);
                    bool fActual = pPosition->InCheck(pPosition->GetTurn());
                    pPosition->UndoMove(move);
                    if (fActual) {
                        nChecks++;
                    }
                    Assert::AreEqual(
                        fActual, fReports,
                        L"IsCheckingMove disagreed with the actual check status");
                }
                pPosition->DoMove(rgMoves[Next() % rgMoves.size()]);
            }
            CPosition::Free(pPosition);
        }
        Assert::IsTrue(nChecks > 0, L"test did not exercise any checking moves");
    }

    // A double-push flagged move whose pawn is not on the main board's home
    // rank must be rejected by LegalMove.  Double pushes only exist on the
    // main level's home rank, so legality must check the source level/rank and
    // not merely that the two squares ahead are empty.
    TEST_METHOD(LegalMoveRejectsDoublePushOffHomeRank) {
        char szEpd[] = "4k3/8/8/8/8/4P3/8/4K3 w - -";
        PositionGuard position(CreatePositionFromLegacyMainEPD(szEpd));
        CMove move = MakeMainBoardMove(he3, he5, M_PAWND);
        Assert::IsFalse(position.get()->LegalMove(move));
    }

    // The matching legal double push from the home rank is still accepted.
    TEST_METHOD(LegalMoveAcceptsDoublePushFromHomeRank) {
        char szEpd[] = "4k3/8/8/8/8/8/4P3/4K3 w - -";
        PositionGuard position(CreatePositionFromLegacyMainEPD(szEpd));
        CMove move = MakeMainBoardMove(he2, he4, M_PAWND);
        Assert::IsTrue(position.get()->LegalMove(move));
    }

    // Every legal move in this position must round-trip through SAN, and a SAN
    // string for a square no rook can reach must not resolve to a legal move.
    TEST_METHOD(StrategyEpdSanRoundTripsAndRejectsRca3) {
        const char *szEpd =
            "r1bq1rk1/p1pp1ppp/1pnbp3/8/8/2NPN3/PPPQPPPP/2BR1BKR w - -";
        PositionGuard position(CPosition::CreateFromEPD(szEpd));
        CPosition *pPosition = position.get();

        std::vector<CMove> rgMoves = CollectLegalMoves(pPosition);
        Assert::IsTrue(rgMoves.size() > 0, L"position produced no legal moves");
        for (CMove move : rgMoves) {
            char szSan[32];
            std::string strSan = pPosition->SAN(move, szSan);
            CMove parsed = pPosition->ParseSAN(strSan.c_str());
            Assert::IsTrue(parsed != M_NONE,
                           L"ParseSAN returned M_NONE for a generated SAN");
            Assert::IsTrue(parsed.GetFromToIndex() == move.GetFromToIndex(),
                           L"SAN did not round-trip to the same move");
        }

        // No rook can reach aa3 (level a, file a, rank 3): the d1 rook is
        // blocked along its file and rank, and the h1 rook is boxed in by the
        // king, so the rook-qualified SAN "Raa3" must not resolve to a move.
        Assert::IsTrue(pPosition->ParseSAN("Raa3") == M_NONE,
                       L"Raa3 must not parse to a legal move");
    }
};

} // namespace WinAmyTests
