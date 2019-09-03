FROM fedora:25

RUN dnf -qy install git cmake
RUN git clone --recursive https://github.com/ethereum/aleth --branch build-on-linux --single-branch --depth 2

RUN cd /tmp && cmake /aleth
RUN cd /tmp && make -j8 && make install && ldconfig

ENTRYPOINT ["/usr/local/bin/eth"]
