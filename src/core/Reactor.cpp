#include "Reactor.hpp"
#include <unistd.h>
#include <stdexcept>

Reactor::Reactor() : running(true) {
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        throw std::runtime_error("Failed to create epoll instance");
    }
}

Reactor::~Reactor() {
    if (epoll_fd != -1)
        close(epoll_fd);
}

void Reactor::addFd(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

void Reactor::modifyFd(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

void Reactor::removeFd(int fd) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
}

void Reactor::run(std::function<void(int, uint32_t)> handler) {

    epoll_event events[MAX_EVENTS];

    while (running) {

        int event_count =
            epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);

        if (event_count == 0) {
            // heartbeat tick
            handler(-1, 0);
            continue;
        }

        for (int i = 0; i < event_count; ++i) {
            handler(events[i].data.fd, events[i].events);
        }
    }
}


void Reactor::stop() {
    running = false;
}