FROM debian:bookworm

# Install all dependencies Crow needs
RUN apt-get update && \
    apt-get install -y \
        g++ \
        cmake \
        make \
        sqlite3 \
        libsqlite3-dev \
        libboost-all-dev \
        libasio-dev \
        libssl-dev \
        git \
        wget

# Create app directory
WORKDIR /app

# Copy all files into container
COPY . .

# Configure and build the project using CMake
RUN cmake -B build
RUN cmake --build build --config Release

# Railway automatically assigns a port. (Expose for clarity)
EXPOSE 8080

# Start the server
CMD ["./build/library_server"]
