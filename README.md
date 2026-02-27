# 🚀 ProductionHTTPServer

High-performance C++ HTTP server built using Linux EPOLL with adaptive scheduling, circuit breaker pattern, real-time metrics monitoring, and Docker support.

Designed to simulate production-grade backend system architecture.

---

## 🧠 Architecture Overview


## 🧠 System Flow


```
                +-------------------+
                |      Client       |
                +-------------------+
                          |
                          v
                +-------------------+
                |  EPOLL Event Loop |
                +-------------------+
                          |
                          v
                +-------------------+
                |    Thread Pool    |
                +-------------------+
                          |
                          v
                +-------------------+
                | Adaptive Scheduler|
                +-------------------+
                          |
                          v
                +-------------------+
                |  Circuit Breaker  |
                +-------------------+
                          |
                          v
                +-------------------+
                |   HTTP Handler    |
                +-------------------+
                          |
                          v
                +-------------------+
                | Metrics + Logging |
                +-------------------+
```
