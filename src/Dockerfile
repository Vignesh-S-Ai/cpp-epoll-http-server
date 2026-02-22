FROM ubuntu:22.04

RUN apt update && apt install -y g++ make

WORKDIR /app
COPY . .

RUN g++ -std=c++17 -pthread \
    main.cpp \
    monitoring/MonitorManager.cpp \
    metrics/Metrics.cpp \
    http/HttpClient.cpp \
    -o monitor_server

EXPOSE 8080
CMD ["./monitor_server"]