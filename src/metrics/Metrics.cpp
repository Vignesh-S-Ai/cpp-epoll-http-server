#include "Metrics.hpp"

std::atomic<long> Metrics::total_requests(0);
std::atomic<long> Metrics::active_connections(0);

void Metrics::incrementTotalRequests() {
    total_requests++;
}

void Metrics::incrementActiveConnections() {
    active_connections++;
}

void Metrics::decrementActiveConnections() {
    active_connections--;
}

long Metrics::getTotalRequests() {
    return total_requests.load();
}

long Metrics::getActiveConnections() {
    return active_connections.load();
}
