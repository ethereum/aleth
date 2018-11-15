# Multistage Dockerfile for the testeth tool.

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
RUN cmake /source -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DTOOLS=OFF -DHUNTER_JOBS_NUMBER=$(nproc)
RUN make -j $(nproc) && make install

# Install stage
FROM alpine:latest
RUN apk add --no-cache libstdc++
COPY --from=builder /build/test/testeth /usr/bin/testeth

ENTRYPOINT ["/usr/bin/testeth"]
