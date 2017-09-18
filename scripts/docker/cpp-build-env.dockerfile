FROM debian:stretch

LABEL maintainer="C++ Ethereum team"
LABEL repo="https://github.com/ethereum/cpp-ethereum"
LABEL version="2"
LABEL description="Build environment for C++ Ethereum projects"

RUN export DEBIAN_FRONTEND=noninteractive \
  && apt-get -qq update && apt-get install -yq --no-install-recommends \
    sudo \
    # Build tools
    git \
    ssh-client \
    curl \
    cmake \
    ninja-build \
    g++ \
    gnupg2 \
    python-pip \
    python-requests \
    # Dependencies
    libleveldb-dev \
    libmicrohttpd-dev \
  && rm -rf /var/lib/apt/lists/* \
  && pip install codecov

RUN export DEBIAN_FRONTEND=noninteractive \
  && echo 'deb http://apt.llvm.org/stretch/ llvm-toolchain-stretch-5.0 main' >> /etc/apt/sources.list \
  && curl -s https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -q - \
  && apt-get -qq update && apt-get install -yq --no-install-recommends \
    clang-5.0 \
    llvm-5.0 \
  && rm -rf /var/lib/apt/lists/* \
  && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-5.0 1 \
  && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-5.0 1 \
  && update-alternatives --install /usr/bin/gcov gcov /usr/bin/llvm-cov-5.0 1

RUN adduser --disabled-password --gecos '' builder && adduser builder sudo && printf 'builder\tALL=NOPASSWD: ALL\n' >> /etc/sudoers

USER builder
WORKDIR /home/builder
