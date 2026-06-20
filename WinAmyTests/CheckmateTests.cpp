#include "TestHelpers.h"

namespace WinAmyTests {

TEST_CLASS(CheckmateTests) {
  public:
    TEST_CLASS_INITIALIZE(InitializeEngine) {
        InitMoves();
        InitAll();
        HashInit();
    }

    // Regression test: after 1.e4 e6 2.d4 Bb4+, white is in check but NOT in
    // checkmate. White can block with Nb1-c3 (and other moves), so LegalMoves
    // must return > 0.  Squares use the engine's coordinate SAN form (level
    // letter 'a' for the single board, then file and rank).
    TEST_METHOD(AfterBishopCheckWhiteHasLegalBlockingMoves) {
        PositionGuard position(CPosition::Initial());

        const char *moves[] = {"Pae2ae4", "Pae7ae6", "Pad2ad4", "Baf8ab4"};
        for (const char *san : moves) {
            CMove move = position.get()->ParseSAN(san);
            Assert::IsTrue(move != M_NONE,
                           L"ParseSAN returned M_NONE for move");
            position.get()->DoMove(move);
        }

        // After 2...Bb4+, it is White's turn. The position should be check.
        Assert::IsTrue(position.get()->InCheck(White),
                       L"Expected white king to be in check after Bb4+");

        // White must have at least one legal move (e.g. Nb1-c3 blocking).
        int legalCount = position.get()->LegalMoves(NULL);
        Assert::IsTrue(legalCount > 0,
                       L"Expected white to have legal moves, but got 0 (false checkmate)");
    }

    // Verify specifically that Nb1-c3 is a legal blocking move.
    TEST_METHOD(Nb1c3IsLegalBlockingMoveAfterBishopCheck) {
        PositionGuard position(CPosition::Initial());

        const char *moves[] = {"Pae2ae4", "Pae7ae6", "Pad2ad4", "Baf8ab4"};
        for (const char *san : moves) {
            CMove move = position.get()->ParseSAN(san);
            Assert::IsTrue(move != M_NONE,
                           L"ParseSAN returned M_NONE for move");
            position.get()->DoMove(move);
        }

        CMove block = position.get()->ParseSAN("Nac3");
        Assert::IsTrue(block != M_NONE, L"ParseSAN could not find Nb1-c3");
        Assert::IsTrue(position.get()->LegalMove(block),
                       L"Nb1-c3 should be a legal blocking move");
    }
};

} // namespace WinAmyTests
