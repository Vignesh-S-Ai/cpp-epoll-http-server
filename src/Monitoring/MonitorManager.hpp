#pragma once
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

struct MonitoredSite {
    std::string host;
    int interval_sec;
    std::chrono::steady_clock::time_point last_checked;
};

class MonitorManager {
public:
    void start();
    void stop();
    void addSite(const std::string& host, int interval);

private:
    void schedulerLoop();

    std::vector<MonitoredSite> sites;
    std::mutex sites_mutex;
    std::thread scheduler_thread;
    std::atomic<bool> running{false};
};