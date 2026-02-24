#include "HttpClient.hpp"
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>

HttpClient::Response
HttpClient::checkWebsite(const std::string& host,
                         int timeout_seconds)
{
    auto start =
        std::chrono::high_resolution_clock::now();

    // ---------------- DNS ----------------
    struct hostent* server =
        gethostbyname(host.c_str());

    if (!server) {
        return Response(CheckResult::DNS_ERROR);
    }

    // ---------------- SOCKET ----------------
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return Response(CheckResult::CONNECTION_ERROR);
    }

    // ---------------- TIMEOUT CONFIG ----------------
    struct timeval timeout;
    timeout.tv_sec = timeout_seconds;
    timeout.tv_usec = 0;

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
               &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,
               &timeout, sizeof(timeout));

    struct sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(80);

    memcpy(&serv_addr.sin_addr.s_addr,
           server->h_addr,
           server->h_length);

    // ---------------- CONNECT ----------------
    if (connect(sock,
                (struct sockaddr*)&serv_addr,
                sizeof(serv_addr)) < 0)
    {
        close(sock);

        if (errno == ETIMEDOUT)
            return Response(CheckResult::TIMEOUT);

        return Response(CheckResult::CONNECTION_ERROR);
    }

    // ---------------- SEND REQUEST ----------------
    std::string request =
        "GET / HTTP/1.1\r\n"
        "Host: " + host + "\r\n"
        "Connection: close\r\n\r\n";

    if (send(sock,
             request.c_str(),
             request.size(),
             0) <= 0)
    {
        close(sock);
        return Response(CheckResult::CONNECTION_ERROR);
    }

    // ---------------- RECEIVE ----------------
    char buffer[4096];
    size_t bytes_received = 0;

    while (true) {

        ssize_t bytes =
            recv(sock,
                 buffer,
                 sizeof(buffer),
                 0);

        if (bytes > 0) {
            bytes_received += bytes;
        }
        else if (bytes == 0) {
            break;  // Normal close
        }
        else {

            if (errno == EAGAIN ||
                errno == EWOULDBLOCK)
            {
                close(sock);
                return Response(CheckResult::TIMEOUT);
            }

            close(sock);
            return Response(CheckResult::RECEIVE_ERROR);
        }
    }

    auto end =
        std::chrono::high_resolution_clock::now();

    long latency_ms =
        std::chrono::duration_cast<
            std::chrono::milliseconds>(
            end - start).count();

    close(sock);

    return Response(CheckResult::SUCCESS,
                    latency_ms,
                    bytes_received);
}