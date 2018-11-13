# Multistage Dockerfile for the Aleth Ethereum node.

# Build stage
FROM alpine:latest as builder
RUN apk add --no-cache \
        git \
        cmake \
        g++ \
        make \
        linux-headers
ADD . /source
WORKDIR /build
RUN cmake /source -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DTOOLS=OFF -DTESTS=OFF -DHUNTER_JOBS_NUMBER=$(nproc)
RUN make -j $(nproc) && make install

# Install stage
FROM alpine:latest
RUN apk add --no-cache \
        python3 \
        libstdc++
COPY --from=builder /usr/bin/aleth /source/scripts/aleth.py /source/scripts/jsonrpcproxy.py /usr/bin/
COPY --from=builder /usr/share/aleth/ /usr/share/aleth/
EXPOSE 8545
ENTRYPOINT ["/usr/bin/aleth.py"]
