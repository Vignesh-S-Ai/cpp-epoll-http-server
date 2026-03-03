#pragma once

#include <unordered_map>
#include <string>
#include <mutex>
#include "PerIPStats.hpp"

class IPTracker {
public:
    void recordRequest(const std::string& ip);
    uint64_t getTotalRequests(const std::string& ip);
    uint64_t getCurrentRate(const std::string& ip);

private:
    std::unordered_map<std::string, PerIPStats> ip_stats_;
    std::mutex mutex_;
};