# For running this container as non-root user, see
# https://github.com/ethereum/cpp-ethereum/blob/develop/scripts/docker-eth
# or call `docker run` like this:
#
##    mkdir -p ~/.ethereum ~/.web3
##    docker run --rm -it \
##      -p 0.0.0.0:30303:30303 \
##      -v ~/.ethereum:/.ethereum -v ~/.web3:/.web3 \
##      -e HOME=/ --user $(id -u):$(id -g) ethereum/client-cpp

FROM alpine

RUN apk add --no-cache \
        libstdc++ \
        gmp \
        leveldb --repository http://dl-cdn.alpinelinux.org/alpine/edge/community/ \
    && apk add --no-cache --virtual .build-deps \
        git \
        cmake \
        g++ \
        make \
        linux-headers \
        leveldb-dev --repository http://dl-cdn.alpinelinux.org/alpine/edge/community/ \
    && sed -i -E -e 's|#warning|//#warning|' /usr/include/sys/poll.h \
    && git clone --recursive https://github.com/ethereum/aleth --branch master --single-branch --depth 1 \
    && mkdir /build && cd /build \
    && cmake /aleth -DCMAKE_BUILD_TYPE=RelWithDebInfo -DTOOLS=Off -DTESTS=Off \
    && make aleth \
    && make install \
    && cd / && rm /build -rf \
    && rm /aleth -rf \
    && apk del .build-deps \
    && rm /var/cache/apk/* -f

EXPOSE 30303

ENTRYPOINT [ "/usr/local/bin/aleth" ]
