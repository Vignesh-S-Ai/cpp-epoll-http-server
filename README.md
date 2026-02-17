# Production HTTP Server (C++)

## Features
- Multithreaded ThreadPool
- Graceful shutdown (SIGINT/SIGTERM)
- Structured logging
- Basic HTTP response handling
- Concurrent request processing

## Build

g++ src/main.cpp \
    src/threadpool/ThreadPool.cpp \
    src/utils/Logger.cpp \
    -o server -pthread

## Run

./server
