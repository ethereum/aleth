# For running this container as non-root user, see
# https://github.com/ethereum/cpp-ethereum/blob/develop/scripts/docker-eth
# or call `docker run` like this:
#
##    mkdir -p ~/.ethereum ~/.web3
##    docker run --rm -it --net=host \
##    -v ~/.ethereum:/.ethereum -v ~/.web3:/.web3 \
##    -e HOME=/ --user $(id -u):$(id -g) ethereum/client-cpp

FROM alpine:3.4
MAINTAINER chriseth

# Since we need to build some libs, set the flags globally
ENV CFLAGS   '-fPIC'
ENV CPPFLAGS '-fPIC'
ENV LDFLAGS  '-fPIC'

# Build cpp-ethereum on the fly and delete all build tools afterwards
  # Install any dependencies of cpp-ethereum
RUN \
  apk add --update sed git make cmake gcc g++ musl-dev curl-dev boost-dev libmicrohttpd-dev                                      && \
  apk add --update crypto++-dev leveldb-dev --update-cache --repository http://dl-cdn.alpinelinux.org/alpine/edge/testing/ --allow-untrusted && \
  # Install libraries missing from Alpine
  wget http://prdownloads.sourceforge.net/argtable/argtable2-13.tar.gz && tar -zxvf argtable2-13.tar.gz                   && \
  cd argtable2-13 && mkdir build && cd build && cmake .. && make install && cp /argtable2-13/src/argtable2.h /usr/include && \
  cd / && rm -rf argtable2-13                                                                                             && \
  \
  git clone --depth 1 git://github.com/miniupnp/miniupnp.git                                                              && \
  cd miniupnp/miniupnpc && make && make install                                                                           && \
  cd / && rm -rf miniupnp                                                                                                 && \
  \
  # Install cpp-ethereum itself, with a bit of a hack to fix some includes
  git clone --depth 1 --branch develop https://github.com/ethereum/cpp-ethereum                                           && \
  sed -i -e s/\<sys\\/poll.h\>/\<poll.h\>/g /usr/include/boost/asio/detail/socket_types.hpp                               && \
  cd cpp-ethereum && mkdir build && cd build && cmake -DETHASHCL=0 -DTESTS=0 ..                                           && \
  make install                                                                                                            && \
  cd / && rm -rf cpp-ethereum                                                                                             && \
  \
  # Clean up all the dependency crap
  apk add --update libcurl boost-random boost-regex boost-filesystem boost-thread libmicrohttpd crypto++ leveldb          && \
  apk del sed git make cmake gcc g++ musl-dev curl-dev boost-dev libmicrohttpd-dev crypto++-dev leveldb-dev               && \
  rm -rf /var/cache/apk/*

EXPOSE 8545 30303

ENTRYPOINT [ "/usr/local/bin/eth" ]
CMD [ "-j" ]

