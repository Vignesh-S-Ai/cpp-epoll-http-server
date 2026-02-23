#pragma once
#include <string>

class Metrics {
public:
    static void recordSuccess(
        const std::string& url,
        long latency,
        size_t bytes);

    static void recordFailure(
        const std::string& url);

    static std::string exportMetrics();
};