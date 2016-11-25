# For running this container as non-root user, see
# https://github.com/ethereum/cpp-ethereum/blob/develop/scripts/docker-eth
# or call `docker run` like this:
#
##    mkdir -p ~/.ethereum ~/.web3
##    docker run --rm -it \
##      -p 127.0.0.1:8545:8545 \
##      -p 0.0.0.0:30303:30303 \
##      -v ~/.ethereum:/.ethereum -v ~/.web3:/.web3 \
##      -e HOME=/ --user $(id -u):$(id -g) ethereum/client-cpp

FROM alpine
MAINTAINER chriseth

# Since we need to build some libs, set the flags globally
ENV CFLAGS   '-fPIC'
ENV CPPFLAGS '-fPIC'
ENV LDFLAGS  '-fPIC'

# Build cpp-ethereum on the fly and delete all build tools afterwards
  # Install any dependencies of cpp-ethereum
RUN \
  apk add --update sed git perl make cmake gcc g++ musl-dev openssl-dev boost-dev libmicrohttpd-dev

RUN  apk add --update openssl

RUN  git clone --depth 1 -b curl-7_51_0 git://github.com/curl/curl                                                         && \
  sed -i -E -e 's/include <sys\/poll.h>/include <poll.h>/' curl/include/curl/curlbuild.h.in                                && \  
  sed -i -E -e 's/include <sys\/poll.h>/include <poll.h>/' curl/include/curl/curlbuild.h.cmake                             && \  
  cd curl && mkdir build && cd build && cmake -DCURL_STATICLIB=yes .. && make install                                          && \
  cd / && rm -rf curl                                                                                             

RUN cd / && mkdir cryptopp && cd cryptopp && wget https://cryptopp.com/cryptopp562.zip && unzip cryptopp562.zip
RUN cd /cryptopp && make static && make install
RUN cd / && rm -rf crypotpp

  # Install libraries missing from Alpine
RUN  wget http://prdownloads.sourceforge.net/argtable/argtable2-13.tar.gz && tar -zxvf argtable2-13.tar.gz                   && \
  cd argtable2-13 && mkdir build && cd build && cmake .. && make install && cp /argtable2-13/src/argtable2.h /usr/include && \
  cd / && rm -rf argtable2-13                                                                                             

RUN  git clone --depth 1 git://github.com/miniupnp/miniupnp.git                                                              && \
  cd miniupnp/miniupnpc && make && make install                                                                           && \
  cd / && rm -rf miniupnp                                                                                                 

RUN  git clone --depth 1 -b v1.19 git://github.com/google/leveldb.git                                                              && \
  cd leveldb && make
RUN cd /leveldb && cp -r include/leveldb /usr/local/include/ && cp out-static/*.a /usr/local/lib/ 
RUN  cd / && rm -rf leveldb                                                                                                 

RUN  sed -i -E -e 's/include <sys\/poll.h>/include <poll.h>/' /usr/include/boost/asio/detail/socket_types.hpp  

# #   # Install cpp-ethereum itself, with a bit of a hack to fix some includes
# RUN  git clone --depth 1 --recursive --branch staticbuild https://github.com/ethereum/cpp-ethereum                               && \
#   cd cpp-ethereum && mkdir build && cd build && cmake -DETHASHCL=0 -DTESTS=0 -DSTATIC_BUILD=On ..                        && \
#   make install                                                                                                            && \
#   cd / && rm -rf cpp-ethereum                                                                                             && \
#   \
#   # Clean up all the dependency crap
#   apk del sed git perl make cmake gcc g++ musl-dev openssl-dev boost-dev libmicrohttpd-dev openssl               && \
#   rm -rf /var/cache/apk/*

# EXPO  SE 8545 30303

#ENTRYPOINT [ "/usr/local/bin/eth" ]
#CMD [ "-j" ]

