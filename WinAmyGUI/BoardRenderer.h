#pragma once

#include <windows.h>

#include "dbase.h"
#include "bitboard.h"
#include "scoord.h"
#include "move.h"

#include <vector>

class BoardRenderer {
public:
    // Square size in pixels.
    static constexpr int SQUARE_SIZE = 36;

    // Margin around the board.
    static constexpr int BOARD_MARGIN = 12;

    // Gap (in pixels) between a board edge and the rank/file coordinate labels.
    static constexpr int COORD_LABEL_GAP = 4;

    // Board square colours (warm brown).
    static constexpr COLORREF CLR_LIGHT      = RGB(200, 170, 150);
    static constexpr COLORREF CLR_DARK       = RGB(120, 100,  90);

    // Highlight colours.
    static constexpr COLORREF CLR_SELECTED   = RGB( 20, 180,  20);
    static constexpr COLORREF CLR_LEGAL_MOVE = RGB(120, 200, 120);
    static constexpr COLORREF CLR_HINT       = RGB(  0, 220, 220);
    static constexpr COLORREF CLR_BORDER     = RGB( 80,  80,  80);
    // Dark grey for the rank/file coordinate labels.
    static constexpr COLORREF CLR_COORD_LABEL = RGB( 60,  60,  60);

    BoardRenderer();
    ~BoardRenderer();

    // Draw the 8x8 board onto the given HDC. HintSquares marks engine move
    // suggestions or other recommendation highlights in cyan.
    void DrawBoard(HDC hdc, const CPosition* pos,
                   const CSCoord* selectedSquare,
                   const std::vector<CSCoord>& legalDests,
                   const std::vector<CSCoord>& HintSquares) const;

    // Return the board square under the given client-area pixel, or an
    // invalid coord if no square is there.
    CSCoord HitTest(POINT pt) const;

    // Return the total width and height required for the board area.
    static SIZE GetBoardAreaSize();

private:
    HFONT m_hPieceFont{nullptr};
    HFONT m_hLabelFont{nullptr};

    // Returns the pixel origin (top-left of the grid area) for the board.
    static POINT BoardOrigin();

    // Draw the rank (left) and file (bottom) coordinate labels.
    void DrawRankFileLabels(HDC hdc) const;

    // Return the Unicode chess piece glyph for the given piece value.
    static wchar_t PieceGlyph(int8_t piece);
};
