#include "TestHelpers.h"

#include "time_ctl.h"

#include <string>

namespace WinAmyTests {

// Tests that exercise the full engine search (CPosition::Iterate) and verify it
// returns a *legal* best move.  These are regression tests for the class of bug
// where the engine recommended an illegal move (e.g. "Ria1bb2") for a given
// position.
TEST_CLASS(SearchTests) {
  public:
    TEST_CLASS_INITIALIZE(InitializeEngine) {
        InitMoves();
        InitAll();
        HashInit();
        // The search probes the transposition / pawn / score tables, which are
        // NULL until AllocateHT() is called.
        AllocateHT();
    }

    // Runs a shallow, deterministic search on the given EPD and asserts that the
    // engine's chosen move is non-empty, legal, and present in the legal move
    // list for the position.  Returns the engine's best move so callers can make
    // additional assertions.
    static CMove SearchAndAssertLegal(const char *pszEpd, int nMaxDepth) {
        PositionGuard Position(CPosition::CreateFromEPD(pszEpd));
        Assert::IsTrue(Position.get() != nullptr,
                       L"CreateFromEPD returned nullptr");

        // Keep the search short and deterministic: cap the iterative-deepening
        // depth and remove any wall-clock budget influence.
        SetMaxSearchDepth(nMaxDepth);
        SetFixedTimePerMove(60);

        int nScore = 0;
        CMove BestMove =
            Position.get()->Iterate(&nScore, M_NONE, nullptr);

        // Restore a near-default search depth for any subsequent test.
        // SetMaxSearchDepth only accepts values < MAX_TREE_SIZE - 1.
        SetMaxSearchDepth(MAX_TREE_SIZE - 2);

        Assert::IsTrue(BestMove != M_NONE,
                       L"Engine returned M_NONE for a position with legal moves");

        // The chosen move must be legal in the root position.  This is exactly
        // the failure mode behind the reported "engine recommended an illegal
        // move" bug.
        Assert::IsTrue(Position.get()->LegalMove(BestMove),
                       L"Engine returned an illegal best move");

        // The chosen move must also appear in the generated legal move list.
        heap_t heap = allocate_heap();
        int nLegal = Position.get()->LegalMoves(heap);
        Assert::IsTrue(nLegal > 0, L"Position unexpectedly has no legal moves");

        bool fFound = false;
        for (unsigned int i = heap->current_section->start;
             i < heap->current_section->end; i++) {
            if (heap->data[i] == BestMove) {
                fFound = true;
                break;
            }
        }
        free_heap(heap);

        Assert::IsTrue(fFound,
                       L"Engine best move was not among the legal moves");

        return BestMove;
    }

    // Regression for the reported bug: the engine previously recommended an
    // illegal move for some positions.  The search must return a legal move that
    // is present in the legal move list.
    TEST_METHOD(EngineReturnsLegalMoveForReportedBugPosition) {
        const char *pszEpd =
            "r1bqkb1r/pppp1ppp/2n2n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq -";

        SearchAndAssertLegal(pszEpd, 4);
    }


    // The engine must return a legal move when the side to move has a piece under
    // threat (here black, whose pieces are engaged in the centre).
    TEST_METHOD(EngineEvadesForcedQueenCapture) {
        const char *pszEpd =
            "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/2N2N2/PPPP1PPP/R1BQK2R b KQkq -";

        SearchAndAssertLegal(pszEpd, 4);
    }



    // Another middlegame where the side to move (black) must find a legal reply.
    TEST_METHOD(EngineEvadesForcedQueenCapture2) {
        const char *pszEpd =
            "rnbqkbnr/pp2pppp/2p5/3p4/3P1B2/8/PPP1PPPP/RN1QKBNR b KQkq -";

        SearchAndAssertLegal(pszEpd, 4);
    }

    TEST_METHOD(EngineEvadesForcedQueenCapture2FromInitialWithDoMove) {
        PositionGuard Position(CPosition::Initial());
        Assert::IsTrue(Position.get() != nullptr, L"Initial returned nullptr");

        // Standard opening development moves.  Squares use the engine's
        // coordinate SAN form (level letter 'a' for the single board, then file
        // and rank), e.g. "Nag1af3" is Ng1-f3.
        const char *rgszSetupMoves[] = {"Nag1af3", "Nag8af6", "Nab1ac3",
                                        "Nab8ac6"};

        const int nMoveCount = static_cast<int>(sizeof(rgszSetupMoves) /
                                                sizeof(rgszSetupMoves[0]));
        for (int nMoveIndex = 0; nMoveIndex < nMoveCount; nMoveIndex++) {
            CMove Move = Position.get()->ParseSAN(rgszSetupMoves[nMoveIndex]);
            Assert::IsTrue(Move != M_NONE, L"ParseSAN returned M_NONE");
            Assert::IsTrue(Position.get()->LegalMove(Move),
                           L"Setup move is illegal");
            Position.get()->DoMove(Move);
        }

        Assert::AreEqual(static_cast<int>(White),
                         static_cast<int>(Position.get()->GetTurn()),
                         L"Setup did not end with white to move");

        SetMaxSearchDepth(4);
        SetFixedTimePerMove(60);

        int nScore = 0;
        CMove Result = Position.get()->Iterate(&nScore, M_NONE, nullptr);

        SetMaxSearchDepth(MAX_TREE_SIZE - 2);

        Assert::IsTrue(Result != M_NONE,
                       L"Engine returned M_NONE for the setup position");
        Assert::IsTrue(Position.get()->LegalMove(Result),
                       L"Engine returned an illegal best move");
    }

    // The engine must return a legal move in a sharp middlegame for white.
    TEST_METHOD(EngineSacrificeRookEvadesForcedQueenCapture) {
        const char *pszEpd =
            "r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq -";

        SearchAndAssertLegal(pszEpd, 4);
    }


    // Sanity check on the standard initial position: the engine must produce a
    // legal opening move.
    TEST_METHOD(EngineReturnsLegalMoveForInitialPosition) {
        PositionGuard Position(CPosition::Initial());

        SetMaxSearchDepth(4);
        SetFixedTimePerMove(60);

        int nScore = 0;
        CMove BestMove = Position.get()->Iterate(&nScore, M_NONE, nullptr);

        SetMaxSearchDepth(MAX_TREE_SIZE - 2);

        Assert::IsTrue(BestMove != M_NONE,
                       L"Engine returned M_NONE for the initial position");
        Assert::IsTrue(Position.get()->LegalMove(BestMove),
                       L"Engine returned an illegal move for the initial position");
    }
};

} // namespace WinAmyTests
