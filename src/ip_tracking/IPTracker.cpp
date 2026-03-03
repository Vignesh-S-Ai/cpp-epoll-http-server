#include "IPTracker.hpp"

#include <chrono>

using Clock = std::chrono::steady_clock;
using Seconds = std::chrono::seconds;

void IPTracker::recordRequest(const std::string& ip) {
    auto now = Clock::now();

    std::lock_guard<std::mutex> lock(mutex_);

    auto& stats = ip_stats_[ip];

    // First time initialization
    if (stats.window_start.time_since_epoch().count() == 0) {
        stats.window_start = now;
    }

    auto elapsed = std::chrono::duration_cast<Seconds>(
        now - stats.window_start
    );

    // If window expired (1 second window)
    if (elapsed.count() >= 1) {
        stats.requests_this_window = 0;
        stats.window_start = now;
    }

    stats.total_requests++;
    stats.requests_this_window++;
}

uint64_t IPTracker::getTotalRequests(const std::string& ip) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = ip_stats_.find(ip);
    if (it == ip_stats_.end()) {
        return 0;
    }

    return it->second.total_requests;
}

uint64_t IPTracker::getCurrentRate(const std::string& ip) {
    auto now = Clock::now();

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = ip_stats_.find(ip);
    if (it == ip_stats_.end()) {
        return 0;
    }

    auto& stats = it->second;

    auto elapsed = std::chrono::duration_cast<Seconds>(
        now - stats.window_start
    );

    // If window expired, rate is 0
    if (elapsed.count() >= 1) {
        return 0;
    }

    return stats.requests_this_window;
}