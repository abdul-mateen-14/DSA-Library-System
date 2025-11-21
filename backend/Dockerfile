FROM debian:bookworm

RUN apt-get update && \
    apt-get install -y g++ cmake make sqlite3 libsqlite3-dev wget git

WORKDIR /app

COPY . .

RUN cmake -B build
RUN cmake --build build --config Release

EXPOSE 8080

CMD ["./build/library_server"]
