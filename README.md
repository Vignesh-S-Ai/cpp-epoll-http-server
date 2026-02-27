# 🚀 ProductionHTTPServer

High-performance C++ HTTP server built using Linux EPOLL with adaptive scheduling, circuit breaker pattern, real-time metrics monitoring, and Docker support.

Designed to simulate production-grade backend system architecture.

---

## 🧠 Architecture Overview


## 🧠 System Flow

```mermaid
flowchart TD
    A[Client] --> B[Non-blocking EPOLL Loop]
    B --> C[Worker Thread Pool]
    C --> D[Load-Aware Scheduler]
    D --> E[Circuit Breaker State Machine]
    E --> F[HTTP Processing Layer]
    F --> G[Metrics Collector]
    F --> H[Structured Logger]
