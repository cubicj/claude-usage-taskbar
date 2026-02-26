#pragma once
#include "PluginInterface.h"
#include "WorkerThread.h"
#include <string>

class ClaudeUsagePlugin;

class UsageItem : public IPluginItem
{
public:
    UsageItem(const wchar_t* name, const wchar_t* id, const wchar_t* label);

    const wchar_t* GetItemName() const override;
    const wchar_t* GetItemId() const override;
    const wchar_t* GetItemLableText() const override;
    const wchar_t* GetItemValueText() const override;
    const wchar_t* GetItemValueSampleText() const override;
    bool IsCustomDraw() const override;
    int GetItemWidth() const override;
    void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;
    int OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag) override;

    void SetOwner(ClaudeUsagePlugin* owner) { m_owner = owner; }
    void UpdateData(double pct, bool has_data, bool refreshing);

private:
    const wchar_t* m_name;
    const wchar_t* m_id;
    const wchar_t* m_label;
    ClaudeUsagePlugin* m_owner = nullptr;
    double m_pct = 0.0;
    bool m_hasData = false;
    bool m_refreshing = false;
};

class ClaudeUsagePlugin : public ITMPlugin
{
public:
    static ClaudeUsagePlugin& Instance();

    IPluginItem* GetItem(int index) override;
    void DataRequired() override;
    const wchar_t* GetInfo(PluginInfoIndex index) override;
    const wchar_t* GetTooltipInfo() override;
    OptionReturn ShowOptionsDialog(void* hParent) override;
    void OnInitialize(ITrafficMonitor* pApp) override;

    void RequestRefresh();
    void Shutdown();

private:
    ClaudeUsagePlugin();

    static ClaudeUsagePlugin m_instance;
    UsageItem m_five_hour{L"5h Usage", L"claude_5h", L""};
    UsageItem m_seven_day{L"7d Usage", L"claude_7d", L""};
    WorkerThread m_worker;
    bool m_workerStarted = false;
    std::wstring m_tooltip;
    ITrafficMonitor* m_pApp = nullptr;
    bool m_notifiedNoCredentials = false;
    bool m_notifiedAuthFailed = false;
    bool m_refreshing = false;
    ULONGLONG m_refreshTick = 0;
};
