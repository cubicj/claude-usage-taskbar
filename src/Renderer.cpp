#include "Renderer.h"
#include <cstdio>

void RenderUsageItem(
    HDC hdc, int x, int y, int w, int h,
    bool dark_mode,
    const wchar_t* label,
    double pct,
    bool has_data)
{
    SetBkMode(hdc, TRANSPARENT);

    COLORREF labelColor = dark_mode ? kLabelDark : kLabelLight;
    COLORREF pctColor   = dark_mode ? kPctDark   : kPctLight;
    COLORREF trackColor = dark_mode ? kTrackDark  : kTrackLight;

    SIZE labelSize;
    GetTextExtentPoint32W(hdc, label, static_cast<int>(wcslen(label)), &labelSize);

    wchar_t pctText[16];
    if (has_data)
        swprintf_s(pctText, L"%.0f%%", pct);
    else
        swprintf_s(pctText, L"--");

    SIZE pctSize;
    GetTextExtentPoint32W(hdc, pctText, static_cast<int>(wcslen(pctText)), &pctSize);

    int padding = 3;
    int labelX = x;
    int barX = x + labelSize.cx + padding;
    int pctX = x + w - pctSize.cx;
    int barW = pctX - barX - padding;

    if (barW < 10) barW = 10;

    int barH = h * 30 / 100;
    if (barH < 3) barH = 3;
    int barY = y + (h - barH) / 2;

    SetTextColor(hdc, labelColor);
    RECT labelRect = {labelX, y, labelX + labelSize.cx, y + h};
    DrawTextW(hdc, label, -1, &labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);

    RECT trackRect = {barX, barY, barX + barW, barY + barH};
    HBRUSH trackBrush = CreateSolidBrush(trackColor);
    FillRect(hdc, &trackRect, trackBrush);
    DeleteObject(trackBrush);

    if (has_data && pct > 0.0) {
        int fillW = static_cast<int>(barW * pct / 100.0);
        if (fillW < 1 && pct > 0.0) fillW = 1;
        RECT fillRect = {barX, barY, barX + fillW, barY + barH};
        HBRUSH fillBrush = CreateSolidBrush(kBarFill);
        FillRect(hdc, &fillRect, fillBrush);
        DeleteObject(fillBrush);
    }

    SetTextColor(hdc, pctColor);
    RECT pctRect = {pctX, y, pctX + pctSize.cx, y + h};
    DrawTextW(hdc, pctText, -1, &pctRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
}
