FROM debian:bookworm

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    cmake \
    make \
    sqlite3 \
    libsqlite3-dev \
    wget \
    git \
    libssl-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Install standalone ASIO
WORKDIR /tmp
RUN wget https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-28-1.tar.gz && \
    tar -xzf asio-1-28-1.tar.gz && \
    cp -r asio-asio-1-28-1/include/asio /usr/include/ && \
    rm -rf asio-1-28-1.tar.gz asio-asio-1-28-1

# Set working directory
WORKDIR /app
COPY . .

# Build the project
RUN cmake -S . -B build
RUN cmake --build build

# Expose port
EXPOSE 8080

# Run the application
CMD ["./build/library_server"]
