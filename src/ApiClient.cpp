#include "ApiClient.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <shlobj.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <ctime>
#include <vector>

using json = nlohmann::json;

static const int64_t kTokenExpiryBufferMs = 5 * 60 * 1000;

static std::wstring GetCredentialsPath()
{
    wchar_t* profile = nullptr;
    SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &profile);
    std::wstring path(profile);
    CoTaskMemFree(profile);
    path += L"\\.claude\\.credentials.json";
    return path;
}

static std::string ReadFileUtf8(const std::wstring& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return {};
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

ApiResponse ReadCredentials()
{
    ApiResponse resp;
    auto path = GetCredentialsPath();
    auto content = ReadFileUtf8(path);

    if (content.empty()) {
        resp.error = "credentials.json not found or empty";
        return resp;
    }

    try {
        auto j = json::parse(content);
        auto& oauth = j.at("claudeAiOauth");
        resp.credentials.accessToken = oauth.at("accessToken").get<std::string>();
        resp.credentials.refreshToken = oauth.at("refreshToken").get<std::string>();
        resp.credentials.expiresAt = oauth.at("expiresAt").get<int64_t>();
        resp.success = true;
    } catch (const json::exception& e) {
        resp.error = std::string("JSON parse error: ") + e.what();
    }

    return resp;
}

bool IsTokenExpired(const Credentials& creds)
{
    auto nowMs = static_cast<int64_t>(time(nullptr)) * 1000;
    return creds.expiresAt <= (nowMs + kTokenExpiryBufferMs);
}

static const wchar_t* kRefreshHost = L"platform.claude.com";
static const wchar_t* kRefreshPath = L"/v1/oauth/token";
static const char* kClientId = "9d1c250a-e61b-44d9-88ed-5944d1962f5e";
static const DWORD kTimeoutMs = 10000;

struct HttpResponse {
    bool success = false;
    int statusCode = 0;
    std::string body;
    std::string error;
};

static HttpResponse HttpRequest(
    const wchar_t* host,
    const wchar_t* path,
    const wchar_t* method,
    const wchar_t* headers,
    const std::string& body)
{
    HttpResponse resp;

    HINTERNET hSession = WinHttpOpen(L"claude-usage-taskbar/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);
    if (!hSession) { resp.error = "WinHttpOpen failed"; return resp; }

    WinHttpSetTimeouts(hSession, kTimeoutMs, kTimeoutMs, kTimeoutMs, kTimeoutMs);

    HINTERNET hConnect = WinHttpConnect(hSession, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        resp.error = "WinHttpConnect failed";
        WinHttpCloseHandle(hSession);
        return resp;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, method, path,
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        resp.error = "WinHttpOpenRequest failed";
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return resp;
    }

    if (headers) {
        WinHttpAddRequestHeaders(hRequest, headers, static_cast<DWORD>(-1), WINHTTP_ADDREQ_FLAG_ADD);
    }

    BOOL sent = WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(body.c_str()),
        static_cast<DWORD>(body.size()),
        static_cast<DWORD>(body.size()), 0);

    if (!sent || !WinHttpReceiveResponse(hRequest, nullptr)) {
        resp.error = "HTTP request failed (error " + std::to_string(GetLastError()) + ")";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return resp;
    }

    DWORD statusCode = 0;
    DWORD size = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        nullptr, &statusCode, &size, nullptr);
    resp.statusCode = static_cast<int>(statusCode);

    std::string responseBody;
    DWORD bytesAvailable = 0;
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
        std::vector<char> buf(bytesAvailable);
        DWORD bytesRead = 0;
        WinHttpReadData(hRequest, buf.data(), bytesAvailable, &bytesRead);
        responseBody.append(buf.data(), bytesRead);
    }
    resp.body = responseBody;
    resp.success = (statusCode >= 200 && statusCode < 300);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return resp;
}

static bool WriteCredentialsFile(const Credentials& creds)
{
    auto path = GetCredentialsPath();
    auto content = ReadFileUtf8(path);
    if (content.empty()) return false;

    try {
        auto j = json::parse(content);
        j["claudeAiOauth"]["accessToken"] = creds.accessToken;
        j["claudeAiOauth"]["refreshToken"] = creds.refreshToken;
        j["claudeAiOauth"]["expiresAt"] = creds.expiresAt;

        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return false;
        file << j.dump();
        return true;
    } catch (...) {
        return false;
    }
}

ApiResponse RefreshToken(const Credentials& creds)
{
    ApiResponse resp;

    json body;
    body["grant_type"] = "refresh_token";
    body["refresh_token"] = creds.refreshToken;
    body["client_id"] = kClientId;
    body["scope"] = "user:profile user:inference user:sessions:claude_code user:mcp_servers";

    auto http = HttpRequest(kRefreshHost, kRefreshPath, L"POST",
        L"Content-Type: application/json", body.dump());

    if (!http.success) {
        resp.error = "Token refresh failed: " + (http.error.empty()
            ? ("HTTP " + std::to_string(http.statusCode))
            : http.error);
        return resp;
    }

    try {
        auto j = json::parse(http.body);
        resp.credentials.accessToken = j.at("access_token").get<std::string>();
        resp.credentials.refreshToken = j.value("refresh_token", creds.refreshToken);
        auto expiresIn = j.at("expires_in").get<int64_t>();
        resp.credentials.expiresAt = static_cast<int64_t>(time(nullptr)) * 1000 + expiresIn * 1000;
        resp.success = true;

        WriteCredentialsFile(resp.credentials);
    } catch (const json::exception& e) {
        resp.error = std::string("Refresh response parse error: ") + e.what();
    }

    return resp;
}

static const wchar_t* kUsageHost = L"api.anthropic.com";
static const wchar_t* kUsagePath = L"/api/oauth/usage";

ApiResponse FetchUsage(const Credentials& creds)
{
    ApiResponse resp;

    std::wstring headers = L"Authorization: Bearer ";
    std::wstring wToken(creds.accessToken.begin(), creds.accessToken.end());
    headers += wToken;
    headers += L"\r\nanthropic-beta: oauth-2025-04-20";

    auto http = HttpRequest(kUsageHost, kUsagePath, L"GET", headers.c_str(), {});

    if (!http.success) {
        resp.error = "Usage fetch failed: " + (http.error.empty()
            ? ("HTTP " + std::to_string(http.statusCode))
            : http.error);
        return resp;
    }

    try {
        auto j = json::parse(http.body);
        resp.usage.fiveHourPct = j.at("five_hour").at("utilization").get<double>();
        resp.usage.sevenDayPct = j.at("seven_day").at("utilization").get<double>();
        resp.usage.fiveHourResetsAt = j.at("five_hour").at("resets_at").get<std::string>();
        resp.usage.sevenDayResetsAt = j.at("seven_day").at("resets_at").get<std::string>();
        resp.success = true;
    } catch (const json::exception& e) {
        resp.error = std::string("Usage response parse error: ") + e.what();
    }

    return resp;
}

ApiResponse FetchUsageWithAutoRefresh()
{
    auto credResult = ReadCredentials();
    if (!credResult.success) return credResult;

    auto creds = credResult.credentials;

    if (IsTokenExpired(creds)) {
        auto refreshResult = RefreshToken(creds);
        if (!refreshResult.success) return refreshResult;
        creds = refreshResult.credentials;
    }

    auto usageResult = FetchUsage(creds);

    if (!usageResult.success && usageResult.error.find("HTTP 401") != std::string::npos) {
        auto refreshResult = RefreshToken(creds);
        if (!refreshResult.success) return refreshResult;
        usageResult = FetchUsage(refreshResult.credentials);
    }

    return usageResult;
}
