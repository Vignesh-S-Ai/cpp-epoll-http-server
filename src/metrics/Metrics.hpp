#pragma once
#include <string>
#include "../http/HttpClient.hpp"

class Metrics {
public:

    static void recordSuccess(
        const std::string& url,
        long latency,
        size_t bytes,
        int status_code);

    static void recordFailure(
        const std::string& url);

    static void recordFailureType(
        const std::string& url,
        HttpClient::CheckResult result);

    static std::string exportMetrics();

    static int getHealth(
        const std::string& url);
};