#pragma once

#include <string>
#include <cstdint>

struct Credentials {
    std::string accessToken;
    std::string refreshToken;
    int64_t expiresAt = 0;
};

struct UsageResult {
    double fiveHourPct = 0.0;
    double sevenDayPct = 0.0;
    std::string fiveHourResetsAt;
    std::string sevenDayResetsAt;
};

struct ApiResponse {
    bool success = false;
    std::string error;
    Credentials credentials;
    UsageResult usage;
};

ApiResponse ReadCredentials();
bool IsTokenExpired(const Credentials& creds);
ApiResponse RefreshToken(const Credentials& creds);
ApiResponse FetchUsage(const Credentials& creds);
ApiResponse FetchUsageWithAutoRefresh();
