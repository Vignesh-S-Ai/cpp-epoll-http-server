#pragma once
#include <atomic>
#include <chrono>

class MetricsEngine {
public:
    void incrementRequest();
    void incrementActiveConnections();
    void decrementActiveConnections();
    void recordLatency(double ms);

    double getCurrentRPS();
    double getP95Latency();

private:
    std::atomic<uint64_t> total_requests{0};
    std::atomic<uint64_t> active_connections{0};
};  