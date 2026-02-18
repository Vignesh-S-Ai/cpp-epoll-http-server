#include <iostream>
#include <atomic>
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <unordered_map>
#include <sstream>
#include <chrono>

constexpr int PORT = 8080;
constexpr int MAX_EVENTS = 1024;
constexpr int BUFFER_SIZE = 4096;
constexpr int IDLE_TIMEOUT_SEC = 10;
constexpr int MAX_CONNECTIONS = 1000;
constexpr int MAX_REQUEST_SIZE = 8192;

std::atomic<bool> running(true);
std::atomic<long> total_requests(0);
std::atomic<long> active_connections(0);

int server_fd = -1;

struct Connection {
    int fd;
    std::string read_buffer;
    std::string write_buffer;
    bool keep_alive = true;
    std::chrono::steady_clock::time_point last_active;
};

std::unordered_map<int, Connection> connections;

void set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void signal_handler(int) {
    running = false;
    if (server_fd != -1) close(server_fd);
}

int main() {

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

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

    int epoll_fd = epoll_create1(0);

    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = server_fd;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    epoll_event events[MAX_EVENTS];

    std::cout << "Hardened EPOLLET HTTP Server running on port "
              << PORT << "\n";

    while (running) {

        int event_count = epoll_wait(epoll_fd,
                                     events,
                                     MAX_EVENTS,
                                     1000);

        for (int i = 0; i < event_count; ++i) {

            int fd = events[i].data.fd;

            // Accept new clients
            if (fd == server_fd) {

                while (true) {

                    sockaddr_in client{};
                    socklen_t len = sizeof(client);

                    int client_fd = accept(server_fd,
                                           (struct sockaddr*)&client,
                                           &len);

                    if (client_fd == -1) {
                        if (errno == EAGAIN ||
                            errno == EWOULDBLOCK)
                            break;
                        break;
                    }

                    if (active_connections >= MAX_CONNECTIONS) {

                        const char* body =
                            "Service Unavailable";

                        std::string resp =
                            "HTTP/1.1 503 Service Unavailable\r\n"
                            "Content-Length: " +
                            std::to_string(strlen(body)) +
                            "\r\nConnection: close\r\n\r\n" +
                            std::string(body);

                        send(client_fd,
                             resp.c_str(),
                             resp.size(), 0);

                        close(client_fd);
                        continue;
                    }

                    set_non_blocking(client_fd);

                    epoll_event ev{};
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = client_fd;

                    epoll_ctl(epoll_fd,
                              EPOLL_CTL_ADD,
                              client_fd,
                              &ev);

                    connections[client_fd] =
                        Connection{
                            client_fd,
                            "",
                            "",
                            true,
                            std::chrono::steady_clock::now()
                        };

                    active_connections++;
                }
            }

            // Client readable
            else if (events[i].events & EPOLLIN) {

                char buffer[BUFFER_SIZE];

                while (true) {

                    ssize_t bytes =
                        recv(fd, buffer,
                             sizeof(buffer), 0);

                    if (bytes == -1) {
                        if (errno == EAGAIN)
                            break;

                        close(fd);
                        epoll_ctl(epoll_fd,
                                  EPOLL_CTL_DEL,
                                  fd,
                                  nullptr);
                        connections.erase(fd);
                        active_connections--;
                        break;
                    }

                    if (bytes == 0) {
                        close(fd);
                        epoll_ctl(epoll_fd,
                                  EPOLL_CTL_DEL,
                                  fd,
                                  nullptr);
                        connections.erase(fd);
                        active_connections--;
                        break;
                    }

                    auto &conn = connections[fd];

                    conn.read_buffer.append(buffer, bytes);
                    conn.last_active =
                        std::chrono::steady_clock::now();

                    if (conn.read_buffer.size() >
                        MAX_REQUEST_SIZE) {

                        close(fd);
                        epoll_ctl(epoll_fd,
                                  EPOLL_CTL_DEL,
                                  fd,
                                  nullptr);
                        connections.erase(fd);
                        active_connections--;
                        break;
                    }
                }

                auto &conn = connections[fd];

                size_t header_end =
                    conn.read_buffer.find("\r\n\r\n");

                if (header_end !=
                    std::string::npos) {

                    total_requests++;

                    std::string request =
                        conn.read_buffer.substr(
                            0, header_end + 4);

                    std::istringstream stream(request);
                    std::string method, path, version;
                    stream >> method >> path >> version;

                    conn.keep_alive = true;

                    if (request.find(
                        "Connection: close")
                        != std::string::npos)
                        conn.keep_alive = false;

                    std::string body;
                    std::string response;

                    if (path == "/stats") {

                        body =
                            "{\n"
                            "  \"total_requests\": " +
                            std::to_string(
                                total_requests.load()) +
                            ",\n"
                            "  \"active_connections\": " +
                            std::to_string(
                                active_connections.load()) +
                            "\n}";

                        response =
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: application/json\r\n"
                            "Content-Length: " +
                            std::to_string(
                                body.size()) +
                            "\r\n";

                    } else {

                        body =
                            "Hardened EPOLLET Server";

                        response =
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: " +
                            std::to_string(
                                body.size()) +
                            "\r\n";
                    }

                    if (conn.keep_alive)
                        response +=
                            "Connection: keep-alive\r\n\r\n";
                    else
                        response +=
                            "Connection: close\r\n\r\n";

                    response += body;

                    conn.write_buffer = response;
                    conn.read_buffer.clear();

                    epoll_event ev{};
                    ev.events = EPOLLOUT | EPOLLET;
                    ev.data.fd = fd;

                    epoll_ctl(epoll_fd,
                              EPOLL_CTL_MOD,
                              fd,
                              &ev);
                }
            }

            // Client writable
            else if (events[i].events & EPOLLOUT) {

                auto &conn = connections[fd];

                while (!conn.write_buffer.empty()) {

                    ssize_t sent =
                        send(fd,
                             conn.write_buffer.c_str(),
                             conn.write_buffer.size(),
                             0);

                    if (sent == -1) {
                        if (errno == EAGAIN)
                            break;

                        close(fd);
                        epoll_ctl(epoll_fd,
                                  EPOLL_CTL_DEL,
                                  fd,
                                  nullptr);
                        connections.erase(fd);
                        active_connections--;
                        break;
                    }

                    conn.write_buffer.erase(0, sent);
                }

                if (conn.write_buffer.empty()) {

                    if (conn.keep_alive) {

                        epoll_event ev{};
                        ev.events =
                            EPOLLIN | EPOLLET;
                        ev.data.fd = fd;

                        epoll_ctl(epoll_fd,
                                  EPOLL_CTL_MOD,
                                  fd,
                                  &ev);

                    } else {

                        close(fd);
                        epoll_ctl(epoll_fd,
                                  EPOLL_CTL_DEL,
                                  fd,
                                  nullptr);
                        connections.erase(fd);
                        active_connections--;
                    }
                }
            }
        }

        // Idle timeout sweep
        auto now =
            std::chrono::steady_clock::now();

        for (auto it = connections.begin();
             it != connections.end();) {

            auto duration =
                std::chrono::duration_cast<
                    std::chrono::seconds>(
                    now - it->second.last_active)
                    .count();

            if (duration > IDLE_TIMEOUT_SEC) {

                close(it->first);
                epoll_ctl(epoll_fd,
                          EPOLL_CTL_DEL,
                          it->first,
                          nullptr);

                it = connections.erase(it);
                active_connections--;

            } else {
                ++it;
            }
        }
    }

    close(epoll_fd);
    std::cout << "Server stopped.\n";
    return 0;
}
