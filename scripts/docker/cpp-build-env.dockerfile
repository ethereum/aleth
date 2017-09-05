FROM debian:stretch

LABEL maintainerName="C++ Ethereum developers team"
LABEL repo="https://github.com/ethereum/cpp-ethereum"
LABEL version="0.0.1"
LABEL description="A Docker image for building cpp-ethereum."

RUN export DEBIAN_FRONTEND=noninteractive && apt-get -qq update && apt-get install -yq \
    # Build tools
    git \
    curl \
    cmake \
    g++ \
    python-requests \
    # Dependencies
    libleveldb-dev \
    libmicrohttpd-dev \
  && rm -rf /var/lib/apt/lists/*

RUN adduser --disabled-password --gecos '' builder && adduser builder sudo && printf 'builder\tALL=NOPASSWD: ALL\n' >> /etc/sudoers

USER builder
WORKDIR /home/builder
