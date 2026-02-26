#pragma once

#include <string>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct UsageData {
    double five_hour_pct = 0.0;
    double seven_day_pct = 0.0;
    std::wstring five_hour_resets;
    std::wstring seven_day_resets;
    bool has_error = false;
    std::wstring error_msg;
    ULONGLONG last_success_tick = 0;
};

class WorkerThread {
public:
    void Start();
    void Stop();
    void RequestRefresh();
    UsageData GetSnapshot();

private:
    void Run();

    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_shutdown{false};
    std::atomic<bool> m_refreshRequested{false};
    UsageData m_data;
};
