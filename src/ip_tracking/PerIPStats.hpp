#pragma once
#include <chrono>

struct PerIPStats {
    uint64_t total_requests = 0;
    uint64_t requests_this_window = 0;
    std::chrono::steady_clock::time_point window_start{};
};