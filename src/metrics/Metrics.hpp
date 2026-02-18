#pragma once
#include <atomic>

class Metrics {
public:
    static void incrementTotalRequests();
    static void incrementActiveConnections();
    static void decrementActiveConnections();

    static long getTotalRequests();
    static long getActiveConnections();

private:
    static std::atomic<long> total_requests;
    static std::atomic<long> active_connections;
};
