FROM alpine

RUN echo "@testing http://dl-4.alpinelinux.org/alpine/edge/testing" >> /etc/apk/repositories
RUN apk update
RUN apk add --no-cache git cmake
RUN git clone --recursive https://github.com/ethereum/cpp-ethereum --branch build-on-linux --single-branch --depth 6
# RUN cpp-ethereum/scripts/install_deps.sh
RUN apk add --no-cache g++ make
RUN apk add --no-cache curl-dev boost-dev
RUN apk add --no-cache leveldb-dev --repository http://dl-3.alpinelinux.org/alpine/edge/testing/
RUN cd /tmp && cmake /cpp-ethereum
