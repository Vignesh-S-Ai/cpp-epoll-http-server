#include "HttpClient.hpp"
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <chrono>

bool HttpClient::checkWebsite(const std::string& host, long& latency_ms) {
    auto start = std::chrono::high_resolution_clock::now();

    struct hostent* server = gethostbyname(host.c_str());
    if (!server) return false;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;

    struct sockaddr_in serv_addr {};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(80);
    memcpy(&serv_addr.sin_addr.s_addr,
           server->h_addr,
           server->h_length);

    if (connect(sock, (struct sockaddr*)&serv_addr,
                sizeof(serv_addr)) < 0) {
        close(sock);
        return false;
    }

    close(sock);

    auto end = std::chrono::high_resolution_clock::now();
    latency_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start).count();

    return true;
}