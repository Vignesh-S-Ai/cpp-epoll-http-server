#include <iostream>
#include <atomic>
#include <csignal>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <thread>

#include "threadpool/ThreadPool.hpp"
#include "utils/Logger.hpp"

std::atomic<bool> running(true);
int server_fd = -1;

void signal_handler(int signum) {
    running = false;
    if (server_fd != -1) {
        close(server_fd);
    }
}

int main() {

    // Initialize Logger
    Logger::init("server.log");
    Logger::log(LogLevel::INFO, "Server starting...");

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        Logger::log(LogLevel::ERROR, "Socket creation failed");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        Logger::log(LogLevel::ERROR, "Bind failed");
        return 1;
    }

    if (listen(server_fd, 128) < 0) {
        Logger::log(LogLevel::ERROR, "Listen failed");
        return 1;
    }

    Logger::log(LogLevel::INFO, "Server listening on port 8080");

    ThreadPool pool(std::thread::hardware_concurrency());

    while (running) {

        sockaddr_in client{};
        socklen_t client_len = sizeof(client);

        int client_fd = accept(server_fd, (struct sockaddr*)&client, &client_len);

        if (client_fd < 0) {
            if (!running) break;
            Logger::log(LogLevel::WARN, "Failed to accept connection");
            continue;
        }

        Logger::log(LogLevel::INFO, "Accepted new connection");

        pool.enqueue([client_fd]() {

            Logger::log(LogLevel::INFO, "Processing request");

            std::string body = "Production Server";

            std::string response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Length: " + std::to_string(body.size()) + "\r\n"
                "Content-Type: text/plain\r\n"
                "\r\n" +
                body;

            send(client_fd, response.c_str(), response.size(), 0);
            close(client_fd);

            Logger::log(LogLevel::INFO, "Request completed");
        });
    }

    Logger::log(LogLevel::INFO, "Server shutting down...");

    pool.shutdown();

    Logger::log(LogLevel::INFO, "Server stopped cleanly");

    return 0;
}
