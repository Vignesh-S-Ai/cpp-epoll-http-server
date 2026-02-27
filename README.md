# 🚀 ProductionHTTPServer

High-performance C++ HTTP server built using Linux EPOLL with adaptive scheduling, circuit breaker pattern, real-time metrics monitoring, and Docker support.

Designed to simulate production-grade backend system architecture.

---

## 🧠 Architecture Overview



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

- This server follows an event-driven, non-blocking I/O architecture optimized for concurrency and fault tolerance.
---

## 🔥 Features

- ⚡ EPOLL-based non-blocking I/O
- 🧵 Custom ThreadPool implementation
- 🧠 Adaptive load-aware scheduler
- 🛡 Circuit Breaker (CLOSED / OPEN / HALF_OPEN)
- 📊 Real-time metrics tracking
- 📝 Structured logging
- 🔄 Graceful shutdown handling
- 🐳 Dockerized deployment
- ⚙️ Configurable via JSON

---

## 📂 Project Structure
- src/
- http/ → HTTP parsing and response handling
- threadpool/ → Worker pool implementation
- metrics/ → Atomic counters and monitoring
- Monitoring/ → Health and status endpoints
- utils/ → Logging and helper utilities
- external/ → Config handling and integrations
- main.cpp → Server bootstrap and EPOLL loop


---

## ⚙️ Build & Run

### 🔨 Compile Manually

```bash
g++ -std=c++17 -pthread src/main.cpp -o server
./server
```

## 🐳 Using Docker
```bash
docker build -t production-http .
docker run -p 8080:8080 production-http
```
##📊 Metrics Endpoint Example
```json
{
  "active_connections": 42,
  "total_requests": 10321,
  "failed_requests": 12,
  "circuit_state": "CLOSED"
}
```
-Metrics are tracked using atomic counters for thread safety.


## Circuit Breaker Logic
- The circuit breaker prevents cascading failures.
- If failure count exceeds threshold → State = OPEN
- After cooldown period → State = HALF_OPEN
- If request succeeds → State = CLOSED
- If request fails again → State = OPEN
- This ensures system stability under downstream failures.

## Concurrency Model
- EPOLL handles event multiplexing

- ThreadPool processes requests

- Adaptive Scheduler distributes workload

- Atomic counters ensure safe metric updates

## Performance Testing
Example benchmark using Apache Benchmark:
```bash
ab -n 10000 -c 100 http://localhost:8080/
```
You can measure:
- Requests per second
- Average latency
- Error rate under load

## Future Improvements
- Token Bucket Rate Limiter
- Async Logging Queue
- Memory Pool Allocator
- HTTP Keep-Alive Support
- Load-based Auto Scaling Strategy
- CMake modular build system

## Tech Stack
C++17

- Linux EPOLL

- Multithreading (pthread)

- Atomic Operations

- Docker

## Author

**S Vignesh**  
Backend Systems Engineer | C++ | Concurrency | Distributed Systems  
