#pragma once

#include <windows.h>

#include "dbase.h"
#include "bitboard.h"
#include "scoord.h"
#include "ucoord.h"
#include "move.h"

#include <vector>

class BoardRenderer {
public:
    // Retained for source compatibility with the window shell, which still
    // exposes a view-plane selector left over from the 4D board. The flat 2D
    // view now always renders the single standard 8x8 board, so the selected
    // plane has no effect on what is drawn.
    enum class ViewPlane { PlaneXY, PlaneXZ, PlaneYZ };

    // Square size in pixels.
    static constexpr int SQUARE_SIZE = 36;

    // Margin around the board.
    static constexpr int BOARD_MARGIN = 12;

    // Index of the standard 8x8 board within the engine's level stack (level h),
    // and its width in squares.
    static constexpr int STANDARD_LEVEL = 7;
    static constexpr int BOARD_WIDTH = CBitBoard::LEVEL_WIDTH[STANDARD_LEVEL];

    // Gap (in pixels) between a board edge and the rank/file coordinate labels.
    static constexpr int COORD_LABEL_GAP = 4;

    // Width of the gutter reserved on the left and bottom of the board for the
    // rank/file coordinate labels.
    static constexpr int COORD_GUTTER = 16;

    // Square colours (warm brown).
    static constexpr COLORREF CLR_LIGHT = RGB(200, 170, 150);
    static constexpr COLORREF CLR_DARK  = RGB(120, 100,  90);

    // Highlight and label colours.
    static constexpr COLORREF CLR_SELECTED    = RGB( 20, 180,  20);
    static constexpr COLORREF CLR_LEGAL_MOVE  = RGB(120, 200, 120);
    // Cyan highlight for an engine move suggestion (both the piece to move and
    // the recommended destination).
    static constexpr COLORREF CLR_HINT        = RGB(  0, 220, 220);
    static constexpr COLORREF CLR_BORDER      = RGB( 80,  80,  80);
    // Dark grey used for the rank/file coordinate labels.
    static constexpr COLORREF CLR_COORD_LABEL = RGB( 60,  60,  60);

    BoardRenderer();
    ~BoardRenderer();

    // Draw the standard 8x8 board onto the given HDC. HintSquares marks engine
    // move suggestions or other recommendation highlights in cyan.
    void DrawBoard(HDC hdc, const CPosition* pos,
                   const CSCoord* selectedSquare,
                   const std::vector<CSCoord>& legalDests,
                   const std::vector<CSCoord>& HintSquares) const;

    // Return the board square under the given client-area pixel, or an invalid
    // coord if no square is there.
    CSCoord HitTest(POINT pt) const;

    // Retained no-op view-plane accessors (see ViewPlane above). The flat 2D
    // view always renders the standard 8x8 board regardless of the value.
    void SetViewPlane(ViewPlane eViewPlane) { m_eViewPlane = eViewPlane; }
    ViewPlane GetViewPlane() const { return m_eViewPlane; }

    // Return the total width and height required for the board area.
    static SIZE GetBoardAreaSize();

private:
    HFONT m_hPieceFont{nullptr};
    HFONT m_hLabelFont{nullptr};

    // Pixel origin (top-left) of the 8x8 grid, inside the coordinate gutter.
    static POINT BoardOrigin();

    // Draw the rank (left) and file (bottom) coordinate labels around the board.
    void DrawRankFileLabels(HDC hdc) const;

    // Return the Unicode chess piece glyph for the given piece value.
    static wchar_t PieceGlyph(int8_t piece);

    // Retained selected view plane. Has no effect on the flat 8x8 view.
    ViewPlane m_eViewPlane{ViewPlane::PlaneXY};
};
