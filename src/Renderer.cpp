#include "Renderer.h"
#include <cstdio>
#include <algorithm>

static COLORREF LerpColor(COLORREF a, COLORREF b, double t)
{
    t = (std::max)(0.0, (std::min)(1.0, t));
    return RGB(
        GetRValue(a) + static_cast<int>((GetRValue(b) - GetRValue(a)) * t),
        GetGValue(a) + static_cast<int>((GetGValue(b) - GetGValue(a)) * t),
        GetBValue(a) + static_cast<int>((GetBValue(b) - GetBValue(a)) * t));
}

static COLORREF BarColorForPct(double pct)
{
    if (pct <= 60.0) return kBarBlue;
    if (pct <= 80.0) return LerpColor(kBarBlue, kBarOrange, (pct - 60.0) / 20.0);
    return LerpColor(kBarOrange, kBarRed, (pct - 80.0) / 20.0);
}

void RenderUsageItem(
    HDC hdc, int x, int y, int w, int h,
    bool dark_mode,
    const wchar_t* label,
    double pct,
    bool has_data,
    bool refreshing)
{
    RECT itemRect = {x, y, x + w, y + h};
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &itemRect, nullptr, 0, nullptr);

    SetBkMode(hdc, TRANSPARENT);

    COLORREF labelColor = dark_mode ? kLabelDark : kLabelLight;
    COLORREF pctColor   = dark_mode ? kPctDark   : kPctLight;
    COLORREF trackColor = dark_mode ? kTrackDark  : kTrackLight;

    bool hasLabel = label && label[0] != L'\0';

    SIZE labelSize = {};
    if (hasLabel)
        GetTextExtentPoint32W(hdc, label, static_cast<int>(wcslen(label)), &labelSize);

    wchar_t pctText[16];
    if (refreshing)
        swprintf_s(pctText, L"...");
    else if (has_data)
        swprintf_s(pctText, L"%.0f%%", pct);
    else
        swprintf_s(pctText, L"--");

    SIZE pctSize;
    GetTextExtentPoint32W(hdc, pctText, static_cast<int>(wcslen(pctText)), &pctSize);

    SIZE maxPctSize;
    GetTextExtentPoint32W(hdc, L"100%", 4, &maxPctSize);

    int labelGap = 3;
    int barPctGap = 6;
    int barX = hasLabel ? x + labelSize.cx + labelGap : x;
    int pctAreaX = x + w - maxPctSize.cx;
    int pctX = pctAreaX + (maxPctSize.cx - pctSize.cx);
    int barW = pctAreaX - barX - barPctGap;

    if (barW < 10) barW = 10;

    int barH = h * 30 / 100;
    if (barH < 3) barH = 3;
    int barY = y + (h - barH) / 2;

    if (hasLabel) {
        SetTextColor(hdc, labelColor);
        RECT labelRect = {x, y, x + labelSize.cx, y + h};
        DrawTextW(hdc, label, -1, &labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
    }

    RECT trackRect = {barX, barY, barX + barW, barY + barH};
    HBRUSH trackBrush = CreateSolidBrush(trackColor);
    FillRect(hdc, &trackRect, trackBrush);
    DeleteObject(trackBrush);

    if (has_data && pct > 0.0) {
        int fillW = static_cast<int>(barW * pct / 100.0);
        if (fillW < 1 && pct > 0.0) fillW = 1;
        RECT fillRect = {barX, barY, barX + fillW, barY + barH};
        HBRUSH fillBrush = CreateSolidBrush(BarColorForPct(pct));
        FillRect(hdc, &fillRect, fillBrush);
        DeleteObject(fillBrush);
    }

    SetTextColor(hdc, pctColor);
    RECT pctRect = {pctX, y, pctX + pctSize.cx, y + h};
    DrawTextW(hdc, pctText, -1, &pctRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
}
