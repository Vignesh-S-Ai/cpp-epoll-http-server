#pragma once
#include <string>
#include <unordered_map>
#include <mutex>

class Metrics {
public:
    static void recordSuccess(const std::string& url, long latency);
    static void recordFailure(const std::string& url);
    static std::string exportMetrics();

private:
    static std::mutex mtx;
    static std::unordered_map<std::string, long> total_latency;
    static std::unordered_map<std::string, int> success_count;
    static std::unordered_map<std::string, int> failure_count;
};