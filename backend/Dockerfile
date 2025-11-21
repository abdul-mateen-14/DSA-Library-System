FROM debian:bookworm

RUN apt-get update && \
    apt-get install -y g++ cmake make sqlite3 libsqlite3-dev wget git

# Install standalone ASIO
RUN wget https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-28-1.tar.gz && \
    tar -xvzf asio-1-28-1.tar.gz && \
    mv asio-asio-1-28-1/include/asio /usr/include/asio

WORKDIR /app
COPY . .

RUN cmake -B build
RUN cmake --build build --config Release

EXPOSE 8080

CMD ["./build/library_server"]
