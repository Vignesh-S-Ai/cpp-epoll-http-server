#include "Metrics.hpp"
#include <unordered_map>
#include <mutex>
#include <sstream>

static std::mutex mtx;
static std::unordered_map<std::string, long> total_latency;
static std::unordered_map<std::string, int> success_count;
static std::unordered_map<std::string, int> failure_count;

void Metrics::recordSuccess(
    const std::string& url,
    long latency)
{
    std::lock_guard<std::mutex> lock(mtx);
    total_latency[url] += latency;
    success_count[url]++;
}

void Metrics::recordFailure(
    const std::string& url)
{
    std::lock_guard<std::mutex> lock(mtx);
    failure_count[url]++;
}

std::string Metrics::exportMetrics()
{
    std::lock_guard<std::mutex> lock(mtx);

    std::stringstream ss;

    for (auto& pair : success_count) {
        const std::string& url = pair.first;
        int success = pair.second;
        int failure = failure_count[url];

        long avg_latency =
            success > 0 ?
            total_latency[url] / success : 0;

        ss << "site_success{url=\""
           << url << "\"} "
           << success << "\n";

        ss << "site_failure{url=\""
           << url << "\"} "
           << failure << "\n";

        ss << "site_avg_latency_ms{url=\""
           << url << "\"} "
           << avg_latency << "\n";
    }

    return ss.str();
}