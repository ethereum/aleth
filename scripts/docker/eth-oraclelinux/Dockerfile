FROM oraclelinux:7.3

RUN yum -q -y install git
RUN git clone --recursive https://github.com/ethereum/cpp-ethereum --branch build-on-linux --single-branch --depth 2
RUN yum -y install \
                    make \
                    gcc-c++ \
                    curl-devel \
                    gmp-devel

RUN yum -y install epel-release
RUN yum -y install leveldb-devel

RUN /cpp-ethereum/scripts/install_cmake.sh --prefix /usr

RUN cmake --version

RUN cd /tmp && cmake /cpp-ethereum
RUN cd /tmp && make -j8 && make install && ldconfig

ENTRYPOINT ["/usr/local/bin/eth"]
