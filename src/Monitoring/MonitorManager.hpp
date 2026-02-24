#pragma once

#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

class MonitorManager {
public:
    void start();
    void stop();

    void loadConfig(const std::string& file);
    void addSite(const std::string& host, int interval);

private:
    struct Site {
        std::string host;
        int interval_sec;
        std::chrono::steady_clock::time_point last_checked;

        // ---------------- Circuit Breaker ----------------
        enum class BreakerState {
            CLOSED,
            OPEN,
            HALF_OPEN
        };

        BreakerState breaker = BreakerState::CLOSED;

        // Track failures locally for breaker alignment
        int local_consecutive_failures = 0;

        // Exponential backoff control
        int open_attempts = 0;
        int cooldown_seconds = 10;

        std::chrono::steady_clock::time_point breaker_open_time;
    };

    void schedulerLoop();

    std::vector<Site> sites;
    std::mutex sites_mutex;

    std::thread scheduler_thread;
    std::atomic<bool> running{false};
};