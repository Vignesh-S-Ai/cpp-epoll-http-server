#include "Metrics.hpp"
#include <unordered_map>
#include <mutex>
#include <sstream>
#include <deque>
#include <vector>
#include <algorithm>
#include <limits>
#include <iostream>

// ---------------- INTERNAL DATA STRUCTURE ----------------

struct SiteStats {
    long total_latency = 0;
    int success_count = 0;
    int failure_count = 0;

    int consecutive_failures = 0;
    int last_health = -1;

    long min_latency = std::numeric_limits<long>::max();
    long max_latency = 0;

    size_t total_bytes = 0;

    std::deque<long> latencies; // rolling window
};

static std::mutex mtx;
static std::unordered_map<std::string, SiteStats> stats_map;

// ---------------- PERCENTILE CALCULATION ----------------

static long computePercentile(
    const std::deque<long>& data,
    double percentile)
{
    if (data.empty())
        return 0;

    std::vector<long> sorted(data.begin(), data.end());
    std::sort(sorted.begin(), sorted.end());

    size_t index =
        static_cast<size_t>(percentile * sorted.size());

    if (index >= sorted.size())
        index = sorted.size() - 1;

    return sorted[index];
}

// ---------------- HEALTH STRING HELPER ----------------

static std::string healthToString(int h)
{
    switch (h) {
        case 0: return "DOWN";
        case 1: return "HEALTHY";
        case 2: return "SLOW";
        case 3: return "DEGRADED";
        default: return "UNKNOWN";
    }
}

// ---------------- HEALTH ENGINE ----------------

static int computeHealth(const SiteStats& s)
{
    long p90 = computePercentile(s.latencies, 0.90);

    double failure_rate = 0.0;
    int total_checks = s.success_count + s.failure_count;

    if (total_checks > 0) {
        failure_rate =
            static_cast<double>(s.failure_count) /
            total_checks;
    }

    // DOWN logic unchanged
    if (s.consecutive_failures >= 3)
        return 0;  // DOWN

    // If previously SLOW, require stronger recovery
    if (s.last_health == 2) {
        if (p90 < 900)
            return 1;  // Back to HEALTHY
        else
            return 2;  // Stay SLOW
    }

    // Enter SLOW only if clearly slow
    if (p90 > 1200)
        return 2;  // SLOW

    // DEGRADED logic
    if (failure_rate > 0.3)
        return 3;  // DEGRADED

    return 1;  // HEALTHY
}

// ---------------- ALERT ON STATE CHANGE ----------------

static void evaluateAndAlert(
    const std::string& url,
    SiteStats& s)
{
    int new_health = computeHealth(s);

    if (s.last_health != -1 &&
        s.last_health != new_health)
    {
        std::cout << "[ALERT] "
                  << url << " changed from "
                  << healthToString(s.last_health)
                  << " -> "
                  << healthToString(new_health)
                  << "\n";
    }

    s.last_health = new_health;
}

// ---------------- RECORD SUCCESS ----------------

void Metrics::recordSuccess(
    const std::string& url,
    long latency,
    size_t bytes)
{
    std::lock_guard<std::mutex> lock(mtx);

    auto& s = stats_map[url];

    s.total_latency += latency;
    s.success_count++;
    s.total_bytes += bytes;

    s.consecutive_failures = 0;

    if (latency < s.min_latency)
        s.min_latency = latency;

    if (latency > s.max_latency)
        s.max_latency = latency;

    s.latencies.push_back(latency);

    // Keep rolling window size limited
    if (s.latencies.size() > 1000)
        s.latencies.pop_front();

    evaluateAndAlert(url, s);
}

// ---------------- RECORD FAILURE ----------------

void Metrics::recordFailure(
    const std::string& url)
{
    std::lock_guard<std::mutex> lock(mtx);

    auto& s = stats_map[url];

    s.failure_count++;
    s.consecutive_failures++;

    evaluateAndAlert(url, s);
}

// ---------------- EXPORT METRICS ----------------

std::string Metrics::exportMetrics()
{
    std::lock_guard<std::mutex> lock(mtx);

    std::stringstream ss;

    for (auto& pair : stats_map) {

        const std::string& url = pair.first;
        SiteStats& s = pair.second;

        long avg_latency =
            s.success_count > 0 ?
            s.total_latency / s.success_count : 0;

        long p50 = computePercentile(s.latencies, 0.50);
        long p90 = computePercentile(s.latencies, 0.90);
        long p99 = computePercentile(s.latencies, 0.99);

        int health = computeHealth(s);

        ss << "site_success{url=\""
           << url << "\"} "
           << s.success_count << "\n";

        ss << "site_failure{url=\""
           << url << "\"} "
           << s.failure_count << "\n";

        ss << "site_avg_latency_ms{url=\""
           << url << "\"} "
           << avg_latency << "\n";

        ss << "site_latency_ms{url=\""
           << url << "\",quantile=\"0.50\"} "
           << p50 << "\n";

        ss << "site_latency_ms{url=\""
           << url << "\",quantile=\"0.90\"} "
           << p90 << "\n";

        ss << "site_latency_ms{url=\""
           << url << "\",quantile=\"0.99\"} "
           << p99 << "\n";

        ss << "site_min_latency_ms{url=\""
           << url << "\"} "
           << (s.min_latency ==
               std::numeric_limits<long>::max()
               ? 0 : s.min_latency) << "\n";

        ss << "site_max_latency_ms{url=\""
           << url << "\"} "
           << s.max_latency << "\n";

        ss << "site_bytes_received_total{url=\""
           << url << "\"} "
           << s.total_bytes << "\n";

        ss << "site_health{url=\""
           << url << "\"} "
           << health << "\n";
    }

    return ss.str();
}

// ---------------- GET HEALTH (FOR ADAPTIVE SCHEDULER) ----------------

int Metrics::getHealth(const std::string& url)
{
    std::lock_guard<std::mutex> lock(mtx);

    auto it = stats_map.find(url);
    if (it == stats_map.end())
        return 1; // Default HEALTHY

    return computeHealth(it->second);
}