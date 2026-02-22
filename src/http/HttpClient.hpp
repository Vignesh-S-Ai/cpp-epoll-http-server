#pragma once
#include <string>

class HttpClient {
public:
    static bool checkWebsite(const std::string& host, long& latency_ms);
};