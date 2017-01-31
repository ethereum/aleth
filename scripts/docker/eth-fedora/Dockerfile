FROM fedora:25

RUN dnf -qy install git cmake
RUN git clone --recursive https://github.com/ethereum/cpp-ethereum --branch build-on-linux --single-branch --depth 2
RUN /cpp-ethereum/scripts/install_deps.sh

RUN cd /tmp && cmake /cpp-ethereum
RUN cd /tmp && make -j8 && make install && ldconfig

ENTRYPOINT ["/usr/local/bin/eth"]
