#include "Settings.h"
#include <shlobj.h>

Settings& Settings::Instance()
{
    static Settings s;
    return s;
}

void Settings::SetDllModule(HMODULE hModule)
{
    m_hModule = hModule;
}

std::wstring Settings::GetIniPath() const
{
    wchar_t dllPath[MAX_PATH] = {};
    GetModuleFileNameW(m_hModule, dllPath, MAX_PATH);
    std::wstring path(dllPath);
    auto dot = path.rfind(L'.');
    if (dot != std::wstring::npos)
        path = path.substr(0, dot);
    path += L".ini";
    return path;
}

void Settings::Load()
{
    auto ini = GetIniPath();
    const wchar_t* section = L"Settings";

    wchar_t buf[MAX_PATH] = {};
    GetPrivateProfileStringW(section, L"CredentialsPath", L"", buf, MAX_PATH, ini.c_str());
    m_settings.credentialsPath = buf;

    m_settings.itemWidth = GetPrivateProfileIntW(section, L"ItemWidth", 160, ini.c_str());
    if (m_settings.itemWidth < 80) m_settings.itemWidth = 80;
    if (m_settings.itemWidth > 400) m_settings.itemWidth = 400;

    m_settings.pollInterval = GetPrivateProfileIntW(section, L"PollInterval", 60, ini.c_str());
    if (m_settings.pollInterval < 10) m_settings.pollInterval = 10;
    if (m_settings.pollInterval > 3600) m_settings.pollInterval = 3600;
}

void Settings::Save()
{
    auto ini = GetIniPath();
    const wchar_t* section = L"Settings";

    WritePrivateProfileStringW(section, L"CredentialsPath",
        m_settings.credentialsPath.c_str(), ini.c_str());

    WritePrivateProfileStringW(section, L"ItemWidth",
        std::to_wstring(m_settings.itemWidth).c_str(), ini.c_str());

    WritePrivateProfileStringW(section, L"PollInterval",
        std::to_wstring(m_settings.pollInterval).c_str(), ini.c_str());
}

std::wstring Settings::GetDefaultCredentialsPath()
{
    wchar_t* profile = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &profile)) || !profile)
        return L"";
    std::wstring path(profile);
    CoTaskMemFree(profile);
    path += L"\\.claude\\.credentials.json";
    return path;
}

std::wstring Settings::GetEffectiveCredentialsPath() const
{
    if (!m_settings.credentialsPath.empty())
        return m_settings.credentialsPath;
    return GetDefaultCredentialsPath();
}
