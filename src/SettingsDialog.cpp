#include "SettingsDialog.h"
#include "Settings.h"
#include <commdlg.h>

static const int kClientW = 700;
static const int kClientH = 320;
static const int kMargin = 24;
static const int kLabelH = 22;
static const int kEditH = 28;
static const int kBtnW = 100;
static const int kBtnH = 34;
static const int kSmallBtnW = 90;
static const int kRowGap = 18;

enum CtrlId {
    ID_CRED_PATH = 101,
    ID_BROWSE,
    ID_DEFAULT,
    ID_ITEM_WIDTH,
    ID_POLL_INTERVAL,
    ID_OK,
    ID_CANCEL,
};

static HWND hCredPath, hItemWidth, hPollInterval;

static void OnBrowse(HWND hDlg)
{
    wchar_t file[MAX_PATH] = {};
    GetWindowTextW(hCredPath, file, MAX_PATH);

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hDlg;
    ofn.lpstrFilter = L"JSON Files\0*.json\0All Files\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn))
        SetWindowTextW(hCredPath, file);
}

static bool OnOK()
{
    auto& settings = Settings::Instance().GetMutable();

    wchar_t buf[MAX_PATH] = {};
    GetWindowTextW(hCredPath, buf, MAX_PATH);
    std::wstring path(buf);

    if (path == Settings::GetDefaultCredentialsPath())
        settings.credentialsPath.clear();
    else
        settings.credentialsPath = path;

    wchar_t numBuf[16] = {};
    GetWindowTextW(hItemWidth, numBuf, 16);
    int w = _wtoi(numBuf);
    if (w < 80 || w > 400) {
        MessageBoxW(nullptr, L"Item Width must be between 80 and 400.", L"Invalid Value", MB_OK | MB_ICONWARNING);
        return false;
    }
    settings.itemWidth = w;

    GetWindowTextW(hPollInterval, numBuf, 16);
    int p = _wtoi(numBuf);
    if (p < 10 || p > 3600) {
        MessageBoxW(nullptr, L"Poll Interval must be between 10 and 3600 seconds.", L"Invalid Value", MB_OK | MB_ICONWARNING);
        return false;
    }
    settings.pollInterval = p;

    Settings::Instance().Save();
    return true;
}

static LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_BROWSE:   OnBrowse(hWnd); return 0;
        case ID_DEFAULT:  SetWindowTextW(hCredPath, Settings::GetDefaultCredentialsPath().c_str()); return 0;
        case ID_OK:       if (OnOK()) DestroyWindow(hWnd); return 0;
        case ID_CANCEL:   DestroyWindow(hWnd); return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool ShowSettingsDialog(HWND hParent)
{
    static bool registered = false;
    const wchar_t* className = L"ClaudeUsageSettingsDlg";

    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = DlgProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = className;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassW(&wc);
        registered = true;
    }

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    DWORD exStyle = WS_EX_DLGMODALFRAME;
    RECT rc = {0, 0, kClientW, kClientH};
    AdjustWindowRectEx(&rc, style, FALSE, exStyle);
    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    HWND hDlg = CreateWindowExW(exStyle,
        className, L"Claude Usage Settings", style,
        (screenW - winW) / 2, (screenH - winH) / 2, winW, winH,
        hParent, nullptr, GetModuleHandle(nullptr), nullptr);

    auto& s = Settings::Instance().Get();
    auto displayPath = s.credentialsPath.empty()
        ? Settings::GetDefaultCredentialsPath() : s.credentialsPath;

    int y = kMargin;
    int contentW = kClientW - kMargin * 2;

    CreateWindowW(L"STATIC", L"Credentials Path:", WS_CHILD | WS_VISIBLE,
        kMargin, y, contentW, kLabelH, hDlg, nullptr, nullptr, nullptr);
    y += kLabelH + 4;

    int btnRowW = kSmallBtnW * 2 + 8;
    int editW = contentW - btnRowW - 8;
    hCredPath = CreateWindowW(L"EDIT", displayPath.c_str(),
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        kMargin, y, editW, kEditH, hDlg, (HMENU)ID_CRED_PATH, nullptr, nullptr);

    int btnX = kMargin + editW + 8;
    CreateWindowW(L"BUTTON", L"Browse",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        btnX, y, kSmallBtnW, kEditH, hDlg, (HMENU)ID_BROWSE, nullptr, nullptr);

    CreateWindowW(L"BUTTON", L"Default",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        btnX + kSmallBtnW + 8, y, kSmallBtnW, kEditH, hDlg, (HMENU)ID_DEFAULT, nullptr, nullptr);
    y += kEditH + kRowGap;

    int labelW = 210;
    int numEditX = kMargin + labelW + 8;
    int numEditW = 80;

    CreateWindowW(L"STATIC", L"Item Width:", WS_CHILD | WS_VISIBLE,
        kMargin, y + 3, labelW, kLabelH, hDlg, nullptr, nullptr, nullptr);
    wchar_t widthStr[16];
    swprintf_s(widthStr, L"%d", s.itemWidth);
    hItemWidth = CreateWindowW(L"EDIT", widthStr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        numEditX, y, numEditW, kEditH, hDlg, (HMENU)ID_ITEM_WIDTH, nullptr, nullptr);
    y += kEditH + kRowGap;

    CreateWindowW(L"STATIC", L"Poll Interval (sec):", WS_CHILD | WS_VISIBLE,
        kMargin, y + 3, labelW, kLabelH, hDlg, nullptr, nullptr, nullptr);
    wchar_t pollStr[16];
    swprintf_s(pollStr, L"%d", s.pollInterval);
    hPollInterval = CreateWindowW(L"EDIT", pollStr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
        numEditX, y, numEditW, kEditH, hDlg, (HMENU)ID_POLL_INTERVAL, nullptr, nullptr);
    y += kEditH + kRowGap + 12;

    int btnAreaX = (kClientW - kBtnW * 2 - 16) / 2;
    CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        btnAreaX, y, kBtnW, kBtnH, hDlg, (HMENU)ID_OK, nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        btnAreaX + kBtnW + 16, y, kBtnW, kBtnH, hDlg, (HMENU)ID_CANCEL, nullptr, nullptr);

    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    EnumChildWindows(hDlg, [](HWND hChild, LPARAM lParam) -> BOOL {
        SendMessage(hChild, WM_SETFONT, (WPARAM)lParam, TRUE);
        return TRUE;
    }, (LPARAM)hFont);

    ShowWindow(hDlg, SW_SHOW);
    UpdateWindow(hDlg);

    EnableWindow(hParent, FALSE);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);

    return true;
}
