#include "Metrics.hpp"
#include <unordered_map>
#include <mutex>
#include <sstream>
#include <deque>
#include <vector>
#include <algorithm>
#include <limits>

struct SiteStats {
    long total_latency = 0;
    int success_count = 0;
    int failure_count = 0;

    long min_latency = std::numeric_limits<long>::max();
    long max_latency = 0;

    size_t total_bytes = 0;

    std::deque<long> latencies; // rolling window
};

static std::mutex mtx;
static std::unordered_map<std::string, SiteStats> stats_map;

static long computePercentile(
    const std::deque<long>& data,
    double percentile)
{
    if (data.empty()) return 0;

    std::vector<long> sorted(data.begin(), data.end());
    std::sort(sorted.begin(), sorted.end());

    size_t index =
        static_cast<size_t>(percentile * sorted.size());

    if (index >= sorted.size())
        index = sorted.size() - 1;

    return sorted[index];
}

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

    if (latency < s.min_latency)
        s.min_latency = latency;

    if (latency > s.max_latency)
        s.max_latency = latency;

    s.latencies.push_back(latency);

    if (s.latencies.size() > 1000)
        s.latencies.pop_front();
}

void Metrics::recordFailure(
    const std::string& url)
{
    std::lock_guard<std::mutex> lock(mtx);
    stats_map[url].failure_count++;
}

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
    }

    return ss.str();
}