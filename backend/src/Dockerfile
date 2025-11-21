FROM ubuntu:20.04
RUN apt-get update && apt-get install -y cmake g++ libboost-all-dev libmysqlcppconn-dev
WORKDIR /app
COPY . .
RUN mkdir build && cd build && cmake .. && make
EXPOSE 8080
CMD ["./build/library_server"]
