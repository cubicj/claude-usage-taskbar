#include "Plugin.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "Renderer.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// --- UsageItem ---

UsageItem::UsageItem(const wchar_t* name, const wchar_t* id, const wchar_t* label)
    : m_name(name), m_id(id), m_label(label)
{
}

const wchar_t* UsageItem::GetItemName() const { return m_name; }
const wchar_t* UsageItem::GetItemId() const { return m_id; }
const wchar_t* UsageItem::GetItemLableText() const { return m_label; }
const wchar_t* UsageItem::GetItemValueText() const { return L"--"; }
const wchar_t* UsageItem::GetItemValueSampleText() const { return L"100%"; }

bool UsageItem::IsCustomDraw() const { return true; }
int UsageItem::GetItemWidth() const { return Settings::Instance().Get().itemWidth; }

void UsageItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    RenderUsageItem(static_cast<HDC>(hDC), x, y, w, h,
        dark_mode, m_label, m_pct, m_hasData, m_refreshing);
}

int UsageItem::OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag)
{
    if (type == MT_LCLICKED && m_owner) {
        m_owner->RequestRefresh();
        return 1;
    }
    return 0;
}

void UsageItem::UpdateData(double pct, bool has_data, bool refreshing)
{
    m_pct = pct;
    m_hasData = has_data;
    m_refreshing = refreshing;
}

// --- ClaudeUsagePlugin ---

ClaudeUsagePlugin ClaudeUsagePlugin::m_instance;

ClaudeUsagePlugin::ClaudeUsagePlugin()
{
    m_five_hour.SetOwner(this);
    m_seven_day.SetOwner(this);
}

ClaudeUsagePlugin& ClaudeUsagePlugin::Instance()
{
    return m_instance;
}

IPluginItem* ClaudeUsagePlugin::GetItem(int index)
{
    switch (index)
    {
    case 0: return &m_five_hour;
    case 1: return &m_seven_day;
    default: return nullptr;
    }
}

void ClaudeUsagePlugin::DataRequired()
{
    if (!m_workerStarted) {
        m_worker.Start();
        m_workerStarted = true;
    }

    auto snap = m_worker.GetSnapshot();
    bool has_data = snap.last_success_tick > 0;

    if (m_refreshing && snap.last_success_tick > m_refreshTick)
        m_refreshing = false;

    m_five_hour.UpdateData(snap.five_hour_pct, has_data, m_refreshing);
    m_seven_day.UpdateData(snap.seven_day_pct, has_data, m_refreshing);

    if (has_data) {
        m_notifiedNoCredentials = false;
        m_notifiedAuthFailed = false;
    }

    if (snap.has_error && m_pApp) {
        auto& msg = snap.error_msg;
        if (msg.find(L"credentials") != std::wstring::npos && !m_notifiedNoCredentials) {
            m_pApp->ShowNotifyMessage(L"Claude Usage: Credentials not found. Install Claude Code and run 'claude login'.");
            m_notifiedNoCredentials = true;
        }
        if (msg.find(L"401") != std::wstring::npos && !m_notifiedAuthFailed) {
            m_pApp->ShowNotifyMessage(L"Claude Usage: Authentication failed. Run 'claude login' to re-authenticate.");
            m_notifiedAuthFailed = true;
        }
    }

    m_tooltip.clear();
    if (has_data) {
        wchar_t buf[256];
        swprintf_s(buf, L"Session (5hr): %.0f%% \u2014 %s\nWeekly (7day): %.0f%% \u2014 %s",
            snap.five_hour_pct, snap.five_hour_resets.c_str(),
            snap.seven_day_pct, snap.seven_day_resets.c_str());
        m_tooltip = buf;
    } else {
        m_tooltip = L"Claude Usage: waiting for data...";
    }

    if (snap.has_error) {
        auto elapsed = (GetTickCount64() - snap.last_success_tick) / 1000;
        wchar_t errBuf[128];
        if (snap.last_success_tick > 0) {
            if (elapsed < 60)
                swprintf_s(errBuf, L"\n\u26A0 Last updated %llds ago", elapsed);
            else
                swprintf_s(errBuf, L"\n\u26A0 Last updated %lldm ago", elapsed / 60);
        } else {
            swprintf_s(errBuf, L"\n\u26A0 %s", snap.error_msg.c_str());
        }
        m_tooltip += errBuf;
    }
}

const wchar_t* ClaudeUsagePlugin::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME:        return L"Claude Usage Monitor";
    case TMI_DESCRIPTION: return L"Displays Claude AI 5-hour and 7-day usage";
    case TMI_AUTHOR:      return L"cubicj";
    case TMI_COPYRIGHT:   return L"Copyright (C) 2026 cubicj";
    case TMI_VERSION:     return L"1.0.0";
    case TMI_URL:         return L"https://github.com/cubicj/claude-usage-taskbar";
    default:              return L"";
    }
}

const wchar_t* ClaudeUsagePlugin::GetTooltipInfo()
{
    return m_tooltip.c_str();
}

void ClaudeUsagePlugin::OnInitialize(ITrafficMonitor* pApp)
{
    m_pApp = pApp;
}

void ClaudeUsagePlugin::RequestRefresh()
{
    m_refreshing = true;
    m_refreshTick = m_worker.GetSnapshot().last_success_tick;
    m_worker.RequestRefresh();
}

void ClaudeUsagePlugin::Shutdown()
{
    if (m_workerStarted) {
        m_worker.Stop();
        m_workerStarted = false;
    }
}

ITMPlugin::OptionReturn ClaudeUsagePlugin::ShowOptionsDialog(void* hParent)
{
    auto oldSettings = Settings::Instance().Get();
    ShowSettingsDialog(static_cast<HWND>(hParent));
    auto& newSettings = Settings::Instance().Get();

    bool changed = (oldSettings.credentialsPath != newSettings.credentialsPath
        || oldSettings.itemWidth != newSettings.itemWidth
        || oldSettings.pollInterval != newSettings.pollInterval);

    return changed ? OR_OPTION_CHANGED : OR_OPTION_UNCHANGED;
}

// --- DLL Export ---

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &ClaudeUsagePlugin::Instance();
}
