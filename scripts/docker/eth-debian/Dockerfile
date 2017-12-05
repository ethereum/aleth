FROM debian:jessie

RUN apt-get -q update && \
    apt-get -qy install \
        git \
        curl \
        build-essential \
        libboost-all-dev \
        libgmp-dev \
        libleveldb-dev && \
    git clone --recursive https://github.com/ethereum/cpp-ethereum --branch build-on-linux --single-branch --depth 1 && \
    cpp-ethereum/scripts/install_cmake.sh --prefix /tmp && \
    cd /tmp && PATH=$PATH:/tmp/bin cmake /cpp-ethereum && \
    PATH=$PATH:/tmp/bin cmake --build /tmp -- -j8 && \
    cd /tmp && make install && \
    apt-get purge git build-essential -qy && \
    apt-get autoremove -qy && \
    apt-get clean && \
    rm /cpp-ethereum -rf && \
    rm /tmp/* -rf

ENTRYPOINT ["/usr/local/bin/eth"]
