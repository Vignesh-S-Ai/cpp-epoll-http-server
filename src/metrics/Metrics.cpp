#include "Metrics.hpp"
#include <sstream>

std::mutex Metrics::mtx;
std::unordered_map<std::string, long> Metrics::total_latency;
std::unordered_map<std::string, int> Metrics::success_count;
std::unordered_map<std::string, int> Metrics::failure_count;

void Metrics::recordSuccess(const std::string& url, long latency) {
    std::lock_guard<std::mutex> lock(mtx);
    total_latency[url] += latency;
    success_count[url]++;
}

void Metrics::recordFailure(const std::string& url) {
    std::lock_guard<std::mutex> lock(mtx);
    failure_count[url]++;
}

std::string Metrics::exportMetrics() {
    std::lock_guard<std::mutex> lock(mtx);
    std::ostringstream oss;

    for (auto& pair : success_count) {
        const std::string& url = pair.first;
        int success = pair.second;
        int failure = failure_count[url];
        long avg_latency = success > 0 ? total_latency[url] / success : 0;

        oss << "site_success{url=\"" << url << "\"} " << success << "\n";
        oss << "site_failure{url=\"" << url << "\"} " << failure << "\n";
        oss << "site_avg_latency_ms{url=\"" << url << "\"} "
            << avg_latency << "\n";
    }

    return oss.str();
}