FROM alpine

RUN apk add --no-cache \
        libstdc++ \
        boost-filesystem \
        boost-random \
        boost-regex \
        boost-thread \
        gmp \
        libcurl \
        libmicrohttpd \
        leveldb --repository http://dl-cdn.alpinelinux.org/alpine/edge/testing/ \
    && apk add --no-cache --virtual .build-deps \
        git \
        cmake \
        g++ \
        make \
        curl-dev boost-dev libmicrohttpd-dev \
        leveldb-dev --repository http://dl-cdn.alpinelinux.org/alpine/edge/testing/ \
    && sed -i -E -e 's/include <sys\/poll.h>/include <poll.h>/' /usr/include/boost/asio/detail/socket_types.hpp \
    && git clone --recursive https://github.com/ethereum/cpp-ethereum --branch develop --single-branch --depth 1 \
    && mkdir /build && cd /build \
    && cmake /cpp-ethereum -DCMAKE_BUILD_TYPE=RelWithDebInfo -DTOOLS=Off -DTESTS=Off \
    && make -j8 \
    && make install \
    && cd / && rm /build -rf \
    && rm /cpp-ethereum -rf \
    && apk del .build-deps \
    && rm /var/cache/apk/* -f

ENTRYPOINT ["/usr/local/bin/eth"]
