/*
    WinAmyGUI — BoardRenderer
    GDI-based rendering of a single 8x8 chess board.
*/

#include "BoardRenderer.h"
#include <cstring>

// ---------------------------------------------------------------------------
// Board geometry helpers
// ---------------------------------------------------------------------------

static constexpr int BOARD_WIDTH = CBitBoard::LEVEL_WIDTH[0]; // 8

/* static */ POINT BoardRenderer::BoardOrigin() {
    POINT pt;
    pt.x = BOARD_MARGIN;
    pt.y = BOARD_MARGIN;
    return pt;
}

/* static */ SIZE BoardRenderer::GetBoardAreaSize() {
    int nBoardPx = BOARD_WIDTH * SQUARE_SIZE;
    // Board + margin on each side + extra space for coordinate labels.
    SIZE sz;
    sz.cx = nBoardPx + 2 * BOARD_MARGIN;
    sz.cy = nBoardPx + 2 * BOARD_MARGIN;
    return sz;
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

BoardRenderer::BoardRenderer() {
    m_hPieceFont = CreateFontW(
        SQUARE_SIZE - 4, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI Symbol"
    );
    m_hLabelFont = CreateFontW(
        14, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );
}

BoardRenderer::~BoardRenderer() {
    if (m_hPieceFont) { DeleteObject(m_hPieceFont); }
    if (m_hLabelFont) { DeleteObject(m_hLabelFont); }
}

// ---------------------------------------------------------------------------
// Public drawing entry point
// ---------------------------------------------------------------------------

void BoardRenderer::DrawBoard(HDC hdc, const CPosition* pos,
                              const CSCoord* selectedSquare,
                              const std::vector<CSCoord>& legalDests,
                              const std::vector<CSCoord>& HintSquares) const {
    POINT origin = BoardOrigin();
    int nBoardY = origin.y;

    for (int nRank = BOARD_WIDTH - 1; nRank >= 0; --nRank) {
        for (int nFile = 0; nFile < BOARD_WIDTH; ++nFile) {
            int px = origin.x + nFile * SQUARE_SIZE;
            int py = nBoardY + (BOARD_WIDTH - 1 - nRank) * SQUARE_SIZE;

            CSCoord Coord(0, (uint16_t)nFile, (uint16_t)nRank);
            uint16_t nOffset = Coord.BitOffset();

            COLORREF bg = ((nFile + nRank) % 2 == 0) ? CLR_DARK : CLR_LIGHT;

            if (selectedSquare && selectedSquare->IsValid()
                    && selectedSquare->BitOffset() == nOffset) {
                bg = CLR_SELECTED;
            } else {
                for (const auto& HintSquare : HintSquares) {
                    if (HintSquare.IsValid() && HintSquare.BitOffset() == nOffset) {
                        bg = CLR_HINT;
                        break;
                    }
                }
                if (bg != CLR_HINT) {
                    for (const auto& dest : legalDests) {
                        if (dest.IsValid() && dest.BitOffset() == nOffset) {
                            bg = CLR_LEGAL_MOVE;
                            break;
                        }
                    }
                }
            }

            RECT r{ px, py, px + SQUARE_SIZE, py + SQUARE_SIZE };
            HBRUSH hbr = CreateSolidBrush(bg);
            FillRect(hdc, &r, hbr);
            DeleteObject(hbr);

            if (pos) {
                int8_t piece = pos->GetPiece(nOffset);
                if (piece != 0) {
                    wchar_t glyph[2] = { PieceGlyph(piece), L'\0' };
                    HFONT hOldFont = (HFONT)SelectObject(hdc, m_hPieceFont);
                    SetBkMode(hdc, TRANSPARENT);
                    SetTextColor(hdc, (piece > 0) ? RGB(240,240,240) : RGB(20,20,20));
                    DrawTextW(hdc, glyph, 1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    SelectObject(hdc, hOldFont);
                }
            }

            HPEN hpen = CreatePen(PS_SOLID, 1, CLR_BORDER);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hpen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, px, py, px + SQUARE_SIZE, py + SQUARE_SIZE);
            SelectObject(hdc, hOldPen);
            SelectObject(hdc, hOldBrush);
            DeleteObject(hpen);
        }
    }

    DrawRankFileLabels(hdc);
}

// ---------------------------------------------------------------------------
// DrawRankFileLabels
// ---------------------------------------------------------------------------

void BoardRenderer::DrawRankFileLabels(HDC hdc) const {
    POINT origin = BoardOrigin();
    const int nBoardBottom = origin.y + BOARD_WIDTH * SQUARE_SIZE;

    HFONT hOldFont = (HFONT)SelectObject(hdc, m_hLabelFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, CLR_COORD_LABEL);
    UINT uOldAlign = GetTextAlign(hdc);

    // Rank labels down the left side.
    SetTextAlign(hdc, TA_RIGHT | TA_BASELINE);
    for (int nRank = 0; nRank < BOARD_WIDTH; ++nRank) {
        int nSquareTop = origin.y + (BOARD_WIDTH - 1 - nRank) * SQUARE_SIZE;
        int nBaselineY = nSquareTop + SQUARE_SIZE;
        wchar_t chRank = (wchar_t)(L'1' + nRank);
        TextOutW(hdc, origin.x - COORD_LABEL_GAP, nBaselineY, &chRank, 1);
    }

    // File labels along the bottom.
    SetTextAlign(hdc, TA_CENTER | TA_TOP);
    for (int nFile = 0; nFile < BOARD_WIDTH; ++nFile) {
        int nCenterX = origin.x + nFile * SQUARE_SIZE + SQUARE_SIZE / 2;
        wchar_t chFile = (wchar_t)(L'a' + nFile);
        TextOutW(hdc, nCenterX, nBoardBottom + COORD_LABEL_GAP, &chFile, 1);
    }

    SetTextAlign(hdc, uOldAlign);
    SelectObject(hdc, hOldFont);
}

// ---------------------------------------------------------------------------
// HitTest
// ---------------------------------------------------------------------------

CSCoord BoardRenderer::HitTest(POINT pt) const {
    POINT origin = BoardOrigin();
    int nBoardPx = BOARD_WIDTH * SQUARE_SIZE;

    if (pt.x >= origin.x && pt.x < origin.x + nBoardPx
     && pt.y >= origin.y && pt.y < origin.y + nBoardPx) {
        int nFile       = (pt.x - origin.x) / SQUARE_SIZE;
        int nRankFromTop = (pt.y - origin.y) / SQUARE_SIZE;
        int nRank       = (BOARD_WIDTH - 1) - nRankFromTop;
        if (nFile >= 0 && nFile < BOARD_WIDTH && nRank >= 0 && nRank < BOARD_WIDTH) {
            return CSCoord(0, (uint16_t)nFile, (uint16_t)nRank);
        }
    }
    return InvalidSquareCoord();
}

// ---------------------------------------------------------------------------
// PieceGlyph
// ---------------------------------------------------------------------------

/* static */ wchar_t BoardRenderer::PieceGlyph(int8_t piece) {
    bool isWhite = (piece > 0);
    int type = (piece > 0) ? piece : -piece;

    static const wchar_t WHITE_GLYPHS[] = { 0,
        L'\u2659', L'\u2658', L'\u2657', L'\u2656', L'\u2655', L'\u2654', L'\u2659'
    };
    static const wchar_t BLACK_GLYPHS[] = { 0,
        L'\u265F', L'\u265E', L'\u265D', L'\u265C', L'\u265B', L'\u265A', L'\u265F'
    };

    if (type < 1 || type > 7) { return L' '; }
    return isWhite ? WHITE_GLYPHS[type] : BLACK_GLYPHS[type];
}
