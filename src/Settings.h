#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

struct PluginSettings {
    std::wstring credentialsPath;
    int itemWidth = 160;
    int pollInterval = 60;
};

class Settings {
public:
    static Settings& Instance();

    void SetDllModule(HMODULE hModule);
    void Load();
    void Save();

    const PluginSettings& Get() const { return m_settings; }
    PluginSettings& GetMutable() { return m_settings; }

    std::wstring GetEffectiveCredentialsPath() const;
    std::wstring GetIniPath() const;
    static std::wstring GetDefaultCredentialsPath();

private:
    Settings() = default;
    HMODULE m_hModule = nullptr;
    PluginSettings m_settings;
};
