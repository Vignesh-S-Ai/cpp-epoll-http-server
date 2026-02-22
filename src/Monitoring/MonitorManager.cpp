#include "MonitorManager.hpp"
#include "../http/HttpClient.hpp"
#include "../metrics/Metrics.hpp"
#include <chrono>

void MonitorManager::start() {
    running = true;
    scheduler_thread = std::thread(&MonitorManager::schedulerLoop, this);
}

void MonitorManager::stop() {
    running = false;
    if (scheduler_thread.joinable())
        scheduler_thread.join();
}

void MonitorManager::addSite(const std::string& host, int interval) {
    std::lock_guard<std::mutex> lock(sites_mutex);
    sites.push_back({host, interval,
        std::chrono::steady_clock::now()});
}

void MonitorManager::schedulerLoop() {
    while (running) {
        {
            std::lock_guard<std::mutex> lock(sites_mutex);

            for (auto& site : sites) {
                auto now = std::chrono::steady_clock::now();

                if (now - site.last_checked >=
                    std::chrono::seconds(site.interval_sec)) {

                    long latency;
                    bool success =
                        HttpClient::checkWebsite(site.host, latency);

                    if (success)
                        Metrics::recordSuccess(site.host, latency);
                    else
                        Metrics::recordFailure(site.host);

                    site.last_checked = now;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}