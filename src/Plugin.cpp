#include "Plugin.h"

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
int UsageItem::GetItemWidth() const { return 120; }

void UsageItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    // TODO: implement in next session
}

// --- ClaudeUsagePlugin ---

ClaudeUsagePlugin ClaudeUsagePlugin::m_instance;

ClaudeUsagePlugin::ClaudeUsagePlugin()
{
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
    // TODO: implement in next session
}

const wchar_t* ClaudeUsagePlugin::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME:        return L"Claude Usage Monitor";
    case TMI_DESCRIPTION: return L"Displays Claude AI 5-hour and 7-day usage";
    case TMI_AUTHOR:      return L"cubicj";
    case TMI_COPYRIGHT:   return L"Copyright (C) 2026 cubicj";
    case TMI_VERSION:     return L"0.1.0";
    case TMI_URL:         return L"https://github.com/cubicj/claude-usage-taskbar";
    default:              return L"";
    }
}

const wchar_t* ClaudeUsagePlugin::GetTooltipInfo()
{
    return L"Claude Usage: --";
}

// --- DLL Export ---

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &ClaudeUsagePlugin::Instance();
}
