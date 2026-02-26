#include "WorkerThread.h"
#include "ApiClient.h"

#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

static const std::chrono::seconds kPollInterval{60};

static std::wstring FormatResetsIn(const std::string& isoTimestamp)
{
    if (isoTimestamp.empty()) return L"";

    std::tm tm = {};
    std::istringstream ss(isoTimestamp);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail()) return L"Unknown";

    time_t resetTime = _mkgmtime(&tm);
    time_t now = time(nullptr);
    auto diff = static_cast<int64_t>(difftime(resetTime, now));

    if (diff <= 0) return L"Now";

    int days = static_cast<int>(diff / 86400);
    int hours = static_cast<int>((diff % 86400) / 3600);
    int minutes = static_cast<int>((diff % 3600) / 60);

    wchar_t buf[64];
    if (days > 0)
        swprintf_s(buf, L"Resets in %dd %dh", days, hours);
    else if (hours > 0)
        swprintf_s(buf, L"Resets in %dh %dm", hours, minutes);
    else
        swprintf_s(buf, L"Resets in %dm", minutes);

    return buf;
}

static std::wstring Utf8ToWide(const std::string& str)
{
    if (str.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring result(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), len);
    return result;
}

void WorkerThread::Start()
{
    m_shutdown = false;
    m_thread = std::thread(&WorkerThread::Run, this);
}

void WorkerThread::Stop()
{
    m_shutdown = true;
    m_cv.notify_one();
    if (m_thread.joinable()) {
        if (m_thread.get_id() != std::this_thread::get_id())
            m_thread.join();
        else
            m_thread.detach();
    }
}

void WorkerThread::RequestRefresh()
{
    m_refreshRequested = true;
    m_cv.notify_one();
}

UsageData WorkerThread::GetSnapshot()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_data;
}

void WorkerThread::Run()
{
    while (!m_shutdown) {
        auto result = FetchUsageWithAutoRefresh();

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (result.success) {
                m_data.five_hour_pct = result.usage.fiveHourPct;
                m_data.seven_day_pct = result.usage.sevenDayPct;
                m_data.five_hour_resets = FormatResetsIn(result.usage.fiveHourResetsAt);
                m_data.seven_day_resets = FormatResetsIn(result.usage.sevenDayResetsAt);
                m_data.has_error = false;
                m_data.error_msg.clear();
                m_data.last_success_tick = GetTickCount64();
            } else {
                m_data.has_error = true;
                m_data.error_msg = Utf8ToWide(result.error);
            }
        }

        m_refreshRequested = false;
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait_for(lock, kPollInterval, [this] {
            return m_shutdown.load() || m_refreshRequested.load();
        });
    }
}
