# Multistage Dockerfile for the Aleth Ethereum node.

# Build stage
FROM alpine:latest as builder
RUN apk add --no-cache \
        git \
        cmake \
        g++ \
        make \
        linux-headers \
        leveldb-dev --repository http://dl-cdn.alpinelinux.org/alpine/edge/testing/
RUN git clone --recursive https://github.com/ethereum/cpp-ethereum --branch docker --single-branch --depth 1
RUN mkdir /build
RUN cd /build && \
        cmake /cpp-ethereum -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DTOOLS=OFF -DTESTS=OFF
RUN cd /build && make -j $(nproc) && make install

# Install stage
FROM alpine:latest
RUN apk add --no-cache \
        libstdc++ \
        leveldb --repository http://dl-cdn.alpinelinux.org/alpine/edge/testing/
COPY --from=builder /usr/bin/aleth /usr/bin/aleth
ENTRYPOINT ["/usr/bin/aleth"]
