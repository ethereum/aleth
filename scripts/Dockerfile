FROM ubuntu:xenial
MAINTAINER chriseth
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update &&\
    apt-get -q -y install git-core sudo lsb-release software-properties-common &&\
    git clone https://github.com/ethereum/cpp-ethereum.git &&\
    cpp-ethereum/scripts/install_deps.sh &&\
    mkdir cpp-ethereum/build &&\
    cd cpp-ethereum/build &&\
    cmake .. && \
    make install && \
    ldconfig &&\
    rm -rf /cpp-ethereum

EXPOSE 8545 30303

ENTRYPOINT [ "/usr/local/bin/eth" ]
CMD [ "-j" ]

