FROM ubuntu:xenial
MAINTAINER chriseth
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get -y update
RUN apt-get upgrade -q -y
RUN apt-get -q -y install git lsb-release sudo software-properties-common
RUN git clone https://github.com/ethereum/cpp-ethereum.git
RUN cpp-ethereum/scripts/install_deps.sh
RUN mkdir cpp-ethereum/build
RUN cd cpp-ethereum/build && cmake .. && make install
RUN ldconfig # should actually be part of make install...

EXPOSE 8545 30303

ENTRYPOINT [ "/usr/local/bin/eth" ]
CMD [ "-j" ]

