#pragma once

#include <string>
#include <cstddef>

class HttpClient {
public:

    enum class CheckResult {
        SUCCESS,
        DNS_ERROR,
        CONNECTION_ERROR,
        TIMEOUT,
        RECEIVE_ERROR,
        UNKNOWN_ERROR
    };

    struct Response {
        CheckResult result;
        long latency_ms;
        size_t bytes_received;

        Response(CheckResult r = CheckResult::UNKNOWN_ERROR,
                 long l = 0,
                 size_t b = 0)
            : result(r), latency_ms(l), bytes_received(b) {}
    };

    static Response checkWebsite(
        const std::string& host,
        int timeout_seconds = 5
    );
};