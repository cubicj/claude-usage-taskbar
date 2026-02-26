#include <cstdio>
#include <cassert>
#include <string>
#include <ctime>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../src/ApiClient.h"
#include "../src/WorkerThread.h"
#include <thread>
#include <chrono>

void test_placeholder();
void test_read_credentials();
void test_is_token_expired();
void test_refresh_token();
void test_fetch_usage();
void test_worker_thread();
void test_worker_request_refresh();

int main()
{
    printf("=== Claude Usage Plugin Tests ===\n\n");

    test_placeholder();
    test_read_credentials();
    test_is_token_expired();
    test_refresh_token();
    test_fetch_usage();
    test_worker_thread();
    test_worker_request_refresh();

    printf("\n=== All tests passed ===\n");
    return 0;
}

void test_placeholder()
{
    printf("[PASS] test_placeholder\n");
}

void test_read_credentials()
{
    auto result = ReadCredentials();
    if (result.success) {
        assert(!result.credentials.accessToken.empty());
        assert(!result.credentials.refreshToken.empty());
        assert(result.credentials.expiresAt > 0);
        printf("[PASS] test_read_credentials (live)\n");
    } else {
        printf("[SKIP] test_read_credentials: %s\n", result.error.c_str());
    }
}

void test_is_token_expired()
{
    Credentials creds;
    creds.expiresAt = static_cast<int64_t>(time(nullptr)) * 1000 - 3600000;
    assert(IsTokenExpired(creds) == true);

    creds.expiresAt = static_cast<int64_t>(time(nullptr)) * 1000 + 3600000;
    assert(IsTokenExpired(creds) == false);

    creds.expiresAt = static_cast<int64_t>(time(nullptr)) * 1000 + 240000;
    assert(IsTokenExpired(creds) == true);

    printf("[PASS] test_is_token_expired\n");
}

void test_refresh_token()
{
    auto cred_result = ReadCredentials();
    if (!cred_result.success) {
        printf("[SKIP] test_refresh_token: no credentials\n");
        return;
    }

    Credentials expired = cred_result.credentials;
    expired.expiresAt = 0;
    assert(IsTokenExpired(expired));

    auto refresh_result = RefreshToken(expired);
    if (refresh_result.success) {
        assert(!refresh_result.credentials.accessToken.empty());
        assert(!refresh_result.credentials.refreshToken.empty());
        printf("[PASS] test_refresh_token (live)\n");
    } else {
        printf("[SKIP] test_refresh_token: %s\n", refresh_result.error.c_str());
    }
}

void test_fetch_usage()
{
    auto result = FetchUsageWithAutoRefresh();
    if (result.success) {
        assert(result.usage.fiveHourPct >= 0.0 && result.usage.fiveHourPct <= 100.0);
        assert(result.usage.sevenDayPct >= 0.0 && result.usage.sevenDayPct <= 100.0);
        assert(!result.usage.fiveHourResetsAt.empty());
        assert(!result.usage.sevenDayResetsAt.empty());
        printf("[PASS] test_fetch_usage (live) - 5h: %.1f%%, 7d: %.1f%%\n",
            result.usage.fiveHourPct, result.usage.sevenDayPct);
    } else {
        printf("[FAIL] test_fetch_usage: %s\n", result.error.c_str());
        assert(false);
    }
}

void test_worker_thread()
{
    WorkerThread worker;
    worker.Start();

    for (int i = 0; i < 150; ++i) {
        auto snap = worker.GetSnapshot();
        if (snap.last_success_tick > 0) {
            assert(snap.five_hour_pct >= 0.0 && snap.five_hour_pct <= 100.0);
            assert(snap.seven_day_pct >= 0.0 && snap.seven_day_pct <= 100.0);
            printf("[PASS] test_worker_thread - 5h: %.1f%%, 7d: %.1f%%\n",
                snap.five_hour_pct, snap.seven_day_pct);
            worker.Stop();
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    worker.Stop();
    printf("[FAIL] test_worker_thread: no data after 15s\n");
    assert(false);
}

void test_worker_request_refresh()
{
    WorkerThread worker;
    worker.Start();

    for (int i = 0; i < 150; ++i) {
        if (worker.GetSnapshot().last_success_tick > 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    auto before = worker.GetSnapshot().last_success_tick;
    worker.RequestRefresh();

    for (int i = 0; i < 150; ++i) {
        if (worker.GetSnapshot().last_success_tick > before) {
            printf("[PASS] test_worker_request_refresh\n");
            worker.Stop();
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    worker.Stop();
    printf("[FAIL] test_worker_request_refresh: no update\n");
    assert(false);
}
