#pragma once
#include <functional>
#include <sys/epoll.h>
#include <atomic>

class Reactor {
public:
    Reactor();
    ~Reactor();

    void addFd(int fd, uint32_t events);
    void modifyFd(int fd, uint32_t events);
    void removeFd(int fd);

    void run(std::function<void(int, uint32_t)> handler);
    void stop();

private:
    int epoll_fd;
    std::atomic<bool> running;
    static constexpr int MAX_EVENTS = 1024;
};