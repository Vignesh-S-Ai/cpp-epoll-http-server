#include "MonitorManager.hpp"
#include "../metrics/Metrics.hpp"
#include "../http/HttpClient.hpp"
#include "../external/json.hpp"

#include <fstream>
#include <iostream>

using json = nlohmann::json;

void MonitorManager::loadConfig(const std::string& file)
{
    std::ifstream f(file);

    if (!f.is_open()) {
        std::cerr << "Failed to open config file\n";
        return;
    }

    json config;
    f >> config;

    for (auto& site : config["sites"]) {
        std::string url = site["url"];
        int interval = site["interval"];

        addSite(url, interval);
    }
}

void MonitorManager::addSite(
    const std::string& host,
    int interval)
{
    std::lock_guard<std::mutex> lock(sites_mutex);

    sites.push_back({
        host,
        interval,
        std::chrono::steady_clock::now()
    });
}

void MonitorManager::start()
{
    running = true;
    scheduler_thread =
        std::thread(&MonitorManager::schedulerLoop, this);
}

void MonitorManager::stop()
{
    running = false;

    if (scheduler_thread.joinable())
        scheduler_thread.join();
}

void MonitorManager::schedulerLoop()
{
    while (running) {

        {
            std::lock_guard<std::mutex> lock(sites_mutex);

            for (auto& site : sites) {

                auto now = std::chrono::steady_clock::now();

                // ---------------- OPEN STATE ----------------
                if (site.breaker == Site::BreakerState::OPEN) {

                    auto elapsed =
                        std::chrono::duration_cast<
                            std::chrono::seconds>(
                            now - site.breaker_open_time).count();

                    if (elapsed >= site.cooldown_seconds) {

                        std::cout << "[BREAKER] "
                                  << site.host
                                  << " entering HALF_OPEN state\n";

                        site.breaker =
                            Site::BreakerState::HALF_OPEN;
                    }
                    else {
                        continue;
                    }
                }

                // Interval check
                if (now - site.last_checked <
                    std::chrono::seconds(site.interval_sec))
                    continue;

                auto response =
                    HttpClient::checkWebsite(site.host);

                bool success =
                    response.result ==
                    HttpClient::CheckResult::SUCCESS;

                if (success) {

                    Metrics::recordSuccess(
                        site.host,
                        response.latency_ms,
                        response.bytes_received);

                    site.local_consecutive_failures = 0;

                    if (site.breaker ==
                        Site::BreakerState::HALF_OPEN)
                    {
                        std::cout << "[RECOVERY] "
                                  << site.host
                                  << " restored to CLOSED\n";

                        site.breaker =
                            Site::BreakerState::CLOSED;

                        site.open_attempts = 0;
                        site.cooldown_seconds = 10;
                    }
                }
                else {

                    Metrics::recordFailure(site.host);

                    site.local_consecutive_failures++;

                    // Only open breaker after 3 consecutive failures
                    if (site.local_consecutive_failures >= 3) {

                        site.breaker =
                            Site::BreakerState::OPEN;

                        site.breaker_open_time = now;

                        site.open_attempts++;

                        site.cooldown_seconds =
                            std::min(300,
                                     10 * (1 << site.open_attempts));

                        std::cout << "[BREAKER] "
                                  << site.host
                                  << " entering OPEN state "
                                  << "(cooldown "
                                  << site.cooldown_seconds
                                  << "s)\n";

                        site.local_consecutive_failures = 0;
                    }
                }

                site.last_checked = now;
            }
        }

        std::this_thread::sleep_for(
            std::chrono::seconds(1));
    }
}