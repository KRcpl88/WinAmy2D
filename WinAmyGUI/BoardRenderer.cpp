/*
    WinAmyGUI — BoardRenderer
    GDI-based rendering of the standard 8x8 chess board.

    The engine still stores positions on a multi-level lattice; the flat 2D view
    renders only the standard board level (level h, index 7), which is the
    ordinary 8x8 board with files a–h and ranks 1–8.
*/

#include "BoardRenderer.h"

// ---------------------------------------------------------------------------
// Layout helpers
// ---------------------------------------------------------------------------

/* static */ POINT BoardRenderer::BoardOrigin() {
    POINT Origin;
    Origin.x = BOARD_MARGIN + COORD_GUTTER;
    Origin.y = BOARD_MARGIN;
    return Origin;
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
        COORD_GUTTER - 2, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );
}

BoardRenderer::~BoardRenderer() {
    if (m_hPieceFont) {
        DeleteObject(m_hPieceFont);
    }
    if (m_hLabelFont) {
        DeleteObject(m_hLabelFont);
    }
}

// ---------------------------------------------------------------------------
// DrawBoard
// ---------------------------------------------------------------------------

void BoardRenderer::DrawBoard(HDC hdc, const CPosition* pos,
                              const CSCoord* selectedSquare,
                              const std::vector<CSCoord>& legalDests,
                              const std::vector<CSCoord>& HintSquares) const {
    POINT Origin = BoardOrigin();

    for (int nRank = BOARD_WIDTH - 1; nRank >= 0; --nRank) {
        for (int nFile = 0; nFile < BOARD_WIDTH; ++nFile) {
            int nPx = Origin.x + nFile * SQUARE_SIZE;
            int nPy = Origin.y + (BOARD_WIDTH - 1 - nRank) * SQUARE_SIZE;

            CSCoord SquareCoord((uint16_t)STANDARD_LEVEL, (uint16_t)nFile, (uint16_t)nRank);
            uint16_t nOffset = SquareCoord.BitOffset();

            COLORREF clrBg = ((nFile + nRank) % 2 == 0) ? CLR_DARK : CLR_LIGHT;

            if (selectedSquare && selectedSquare->IsValid()
                    && selectedSquare->BitOffset() == nOffset) {
                clrBg = CLR_SELECTED;
            } else {
                for (const auto& HintSquare : HintSquares) {
                    if (HintSquare.IsValid() && HintSquare.BitOffset() == nOffset) {
                        clrBg = CLR_HINT;
                        break;
                    }
                }
                if (clrBg != CLR_HINT) {
                    for (const auto& Dest : legalDests) {
                        if (Dest.IsValid() && Dest.BitOffset() == nOffset) {
                            clrBg = CLR_LEGAL_MOVE;
                            break;
                        }
                    }
                }
            }

            RECT SquareRect{ nPx, nPy, nPx + SQUARE_SIZE, nPy + SQUARE_SIZE };
            HBRUSH hbr = CreateSolidBrush(clrBg);
            FillRect(hdc, &SquareRect, hbr);
            DeleteObject(hbr);

            if (pos) {
                int8_t nPiece = pos->GetPiece(nOffset);
                if (nPiece != 0) {
                    wchar_t rgGlyph[2] = { PieceGlyph(nPiece), L'\0' };
                    HFONT hOldFont = (HFONT)SelectObject(hdc, m_hPieceFont);
                    SetBkMode(hdc, TRANSPARENT);
                    SetTextColor(hdc, (nPiece > 0) ? RGB(240, 240, 240) : RGB(20, 20, 20));
                    DrawTextW(hdc, rgGlyph, 1, &SquareRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    SelectObject(hdc, hOldFont);
                }
            }

            HPEN hPen = CreatePen(PS_SOLID, 1, CLR_BORDER);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, nPx, nPy, nPx + SQUARE_SIZE, nPy + SQUARE_SIZE);
            SelectObject(hdc, hOldPen);
            SelectObject(hdc, hOldBrush);
            DeleteObject(hPen);
        }
    }

    DrawRankFileLabels(hdc);
}

// ---------------------------------------------------------------------------
// DrawRankFileLabels
// ---------------------------------------------------------------------------

void BoardRenderer::DrawRankFileLabels(HDC hdc) const {
    POINT Origin = BoardOrigin();
    const int nBoardBottom = Origin.y + BOARD_WIDTH * SQUARE_SIZE;

    HFONT hOldFont = (HFONT)SelectObject(hdc, m_hLabelFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, CLR_COORD_LABEL);
    UINT uOldAlign = GetTextAlign(hdc);

    // Rank labels (1–8) down the left side. The text baseline is aligned with
    // the bottom of each rank square (TA_BASELINE) so it is unambiguous which
    // rank the digit belongs to.
    SetTextAlign(hdc, TA_RIGHT | TA_BASELINE);
    for (int nRank = 0; nRank < BOARD_WIDTH; ++nRank) {
        int nSquareTop = Origin.y + (BOARD_WIDTH - 1 - nRank) * SQUARE_SIZE;
        int nBaselineY = nSquareTop + SQUARE_SIZE;
        wchar_t chRank = (wchar_t)(L'1' + nRank);
        TextOutW(hdc, Origin.x - COORD_LABEL_GAP, nBaselineY, &chRank, 1);
    }

    // File labels (a–h) along the bottom, centred under each file column.
    SetTextAlign(hdc, TA_CENTER | TA_TOP);
    for (int nFile = 0; nFile < BOARD_WIDTH; ++nFile) {
        int nCenterX = Origin.x + nFile * SQUARE_SIZE + SQUARE_SIZE / 2;
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
    POINT Origin = BoardOrigin();
    const int nBoardSize = BOARD_WIDTH * SQUARE_SIZE;

    if (pt.x >= Origin.x && pt.x < Origin.x + nBoardSize
     && pt.y >= Origin.y && pt.y < Origin.y + nBoardSize) {
        int nFile = (pt.x - Origin.x) / SQUARE_SIZE;
        int nRankFromTop = (pt.y - Origin.y) / SQUARE_SIZE;
        int nRank = (BOARD_WIDTH - 1) - nRankFromTop;
        if (nFile >= 0 && nFile < BOARD_WIDTH && nRank >= 0 && nRank < BOARD_WIDTH) {
            return CSCoord((uint16_t)STANDARD_LEVEL, (uint16_t)nFile, (uint16_t)nRank);
        }
    }
    return InvalidSquareCoord();
}

// ---------------------------------------------------------------------------
// GetBoardAreaSize
// ---------------------------------------------------------------------------

/* static */ SIZE BoardRenderer::GetBoardAreaSize() {
    // Left margin + rank-label gutter + board + right margin.
    int nWidth = BOARD_MARGIN + COORD_GUTTER + BOARD_WIDTH * SQUARE_SIZE + BOARD_MARGIN;
    // Top margin + board + file-label gutter + bottom margin.
    int nHeight = BOARD_MARGIN + BOARD_WIDTH * SQUARE_SIZE + COORD_GUTTER + BOARD_MARGIN;

    SIZE Size{ nWidth, nHeight };
    return Size;
}

// ---------------------------------------------------------------------------
// PieceGlyph
// ---------------------------------------------------------------------------

/* static */ wchar_t BoardRenderer::PieceGlyph(int8_t piece) {
    bool fWhite = (piece > 0);
    int nType = (piece > 0) ? piece : -piece;

    static const wchar_t WHITE_GLYPHS[] = { 0,
        L'\u2659', L'\u2658', L'\u2657', L'\u2656', L'\u2655', L'\u2654', L'\u2659'
    };
    static const wchar_t BLACK_GLYPHS[] = { 0,
        L'\u265F', L'\u265E', L'\u265D', L'\u265C', L'\u265B', L'\u265A', L'\u265F'
    };

    if (nType < 1 || nType > 7) {
        return L' ';
    }
    return fWhite ? WHITE_GLYPHS[nType] : BLACK_GLYPHS[nType];
}
