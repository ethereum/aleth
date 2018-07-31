# Multistage Dockerfile for the Aleth Ethereum node.

# Build stage
FROM alpine:latest as builder
RUN apk add --no-cache \
        cmake \
        g++ \
        make \
        linux-headers \
        leveldb-dev --repository http://dl-cdn.alpinelinux.org/alpine/edge/testing/
ADD . /source
WORKDIR /build
RUN cmake /source -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DTOOLS=OFF -DTESTS=OFF -DHUNTER_JOBS_NUMBER=$(nproc)
RUN make -j $(nproc) && make install

# Install stage
FROM alpine:latest
RUN apk add --no-cache \
        python3 \
        libstdc++ \
        leveldb --repository http://dl-cdn.alpinelinux.org/alpine/edge/testing/
COPY --from=builder /usr/bin/aleth /source/scripts/aleth.py /source/scripts/jsonrpcproxy.py /usr/bin/
EXPOSE 8545
ENTRYPOINT ["/usr/bin/aleth.py"]
