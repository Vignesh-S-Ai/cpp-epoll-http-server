#include <iostream>
#include <atomic>
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sstream>
#include <chrono>
#include <errno.h>

#include "metrics/Metrics.hpp"
#include "Monitoring/MonitorManager.hpp"
#include "core/Reactor.hpp"
#include "server/ConnectionManager.hpp"

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 4096;
constexpr int IDLE_TIMEOUT_SEC = 10;
constexpr int MAX_CONNECTIONS = 1000;
constexpr int MAX_REQUEST_SIZE = 8192;

std::atomic<bool> running(true);
std::atomic<long> total_requests(0);

int server_fd = -1;
Reactor* global_reactor = nullptr;

void set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void signal_handler(int) {
    running = false;
    if (global_reactor)
        global_reactor->stop();
    if (server_fd != -1)
        close(server_fd);
}

int main() {

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    MonitorManager monitor;
    monitor.loadConfig("config.json");
    monitor.start();

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, SOMAXCONN);

    set_non_blocking(server_fd);

    Reactor reactor;
    global_reactor = &reactor;

    reactor.addFd(server_fd, EPOLLIN | EPOLLET);

    std::cout << "Monitoring EPOLLET Server running on port "
              << PORT << "\n";

    ConnectionManager connManager;

    reactor.run([&](int fd, uint32_t events) {

        if (!running) return;

        // HEARTBEAT (Idle Sweep)
        if (fd == -1) {
            connManager.sweepIdle(IDLE_TIMEOUT_SEC);
            return;
        }

        // ACCEPT
        if (fd == server_fd) {

            while (true) {

                sockaddr_in client{};
                socklen_t len = sizeof(client);

                int client_fd = accept(server_fd,
                                       (struct sockaddr*)&client,
                                       &len);

                if (client_fd == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        break;
                    break;
                }

                if (connManager.activeCount() >= MAX_CONNECTIONS) {

                    const char* body = "Service Unavailable";

                    std::string resp =
                        "HTTP/1.1 503 Service Unavailable\r\n"
                        "Content-Length: " +
                        std::to_string(strlen(body)) +
                        "\r\nConnection: close\r\n\r\n" +
                        std::string(body);

                    send(client_fd, resp.c_str(), resp.size(), 0);
                    close(client_fd);
                    continue;
                }

                set_non_blocking(client_fd);
                reactor.addFd(client_fd, EPOLLIN | EPOLLET);
                connManager.add(client_fd);
            }
        }

        // READ
        else if (events & EPOLLIN) {

            if (!connManager.exists(fd)) return;

            char buffer[BUFFER_SIZE];
            auto &conn = connManager.get(fd);

            while (true) {

                ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);

                if (bytes <= 0) {
                    if (bytes == -1 && errno == EAGAIN)
                        break;

                    reactor.removeFd(fd);
                    connManager.remove(fd);
                    return;
                }

                conn.read_buffer.append(buffer, bytes);
                connManager.updateActivity(fd);

                if (conn.read_buffer.size() > MAX_REQUEST_SIZE) {
                    reactor.removeFd(fd);
                    connManager.remove(fd);
                    return;
                }
            }

            size_t header_end = conn.read_buffer.find("\r\n\r\n");

            if (header_end != std::string::npos) {

                total_requests++;

                std::string request =
                    conn.read_buffer.substr(0, header_end + 4);

                std::istringstream stream(request);
                std::string method, path, version;
                stream >> method >> path >> version;

                conn.keep_alive =
                    request.find("Connection: close") == std::string::npos;

                std::string body;
                std::string response;

                if (path == "/stats") {

                    body =
                        "{\n"
                        "  \"total_requests\": " +
                        std::to_string(total_requests.load()) +
                        ",\n"
                        "  \"active_connections\": " +
                        std::to_string(connManager.activeCount()) +
                        "\n}";

                    response =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n";

                } else if (path == "/metrics") {

                    body = Metrics::exportMetrics();
                    response =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n";

                } else {

                    body = "Monitoring EPOLLET Server";
                    response =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n";
                }

                response +=
                    "Content-Length: " +
                    std::to_string(body.size()) +
                    "\r\n";

                response += conn.keep_alive
                            ? "Connection: keep-alive\r\n\r\n"
                            : "Connection: close\r\n\r\n";

                response += body;

                conn.write_buffer = response;
                conn.read_buffer.clear();

                reactor.modifyFd(fd, EPOLLOUT | EPOLLET);
            }
        }

        // WRITE
        else if (events & EPOLLOUT) {

            if (!connManager.exists(fd)) return;

            auto &conn = connManager.get(fd);

            while (!conn.write_buffer.empty()) {

                ssize_t sent =
                    send(fd,
                         conn.write_buffer.c_str(),
                         conn.write_buffer.size(),
                         0);

                if (sent <= 0) {
                    if (sent == -1 && errno == EAGAIN)
                        break;

                    reactor.removeFd(fd);
                    connManager.remove(fd);
                    return;
                }

                conn.write_buffer.erase(0, sent);
            }

            if (conn.write_buffer.empty()) {

                if (conn.keep_alive)
                    reactor.modifyFd(fd, EPOLLIN | EPOLLET);
                else {
                    reactor.removeFd(fd);
                    connManager.remove(fd);
                }
            }
        }

    });

    monitor.stop();
    std::cout << "Server stopped.\n";
    return 0;
}