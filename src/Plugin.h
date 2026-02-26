#pragma once
#include "PluginInterface.h"

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

private:
    const wchar_t* m_name;
    const wchar_t* m_id;
    const wchar_t* m_label;
};

class ClaudeUsagePlugin : public ITMPlugin
{
public:
    static ClaudeUsagePlugin& Instance();

    IPluginItem* GetItem(int index) override;
    void DataRequired() override;
    const wchar_t* GetInfo(PluginInfoIndex index) override;
    const wchar_t* GetTooltipInfo() override;

private:
    ClaudeUsagePlugin();

    static ClaudeUsagePlugin m_instance;
    UsageItem m_five_hour{ L"5h Usage", L"claude_5h", L"5h:" };
    UsageItem m_seven_day{ L"7d Usage", L"claude_7d", L"7d:" };
};
