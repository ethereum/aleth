FROM debian:stretch

LABEL maintainer="C++ Ethereum team"
LABEL repo="https://github.com/ethereum/cpp-ethereum"
LABEL version="3"
LABEL description="Build environment for C++ Ethereum projects"

RUN export DEBIAN_FRONTEND=noninteractive \
  && apt-get -qq update && apt-get install -yq --no-install-recommends \
    curl \
    dirmngr \
    gnupg2 \
    sudo \
  && rm -rf /var/lib/apt/lists/*

RUN export DEBIAN_FRONTEND=noninteractive \
  && echo 'deb http://deb.debian.org/debian jessie main' >> /etc/apt/sources.list \
  # && echo 'deb http://ppa.launchpad.net/ubuntu-toolchain-r/test/ubuntu xenial main' >> /etc/apt/sources.list \
  && echo 'deb http://apt.llvm.org/stretch/ llvm-toolchain-stretch-5.0 main' >> /etc/apt/sources.list \
  && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys \
    6084F3CF814B57C1CF12EFD515CF4D18AF4F7421 \
    60C317803A41BA51845E371A1E9377A2BA9EF27F \
  && apt-get -qq update && apt-get install -yq --no-install-recommends \
    # Build tools
    git \
    ssh-client \
    cmake \
    ninja-build \
    python-pip \
    python-requests \
    # Compilers
    g++ \
    g++-4.8 \
    # g++-7 \
    clang-5.0 \
    llvm-5.0 \
    # Dependencies
    libleveldb-dev \
  && rm -rf /var/lib/apt/lists/* \
  && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-5.0 1 \
  && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-5.0 1 \
  && update-alternatives --install /usr/bin/gcov gcov /usr/bin/llvm-cov-5.0 1 \
  && pip install codecov

RUN adduser --disabled-password --gecos '' builder && adduser builder sudo && printf 'builder\tALL=NOPASSWD: ALL\n' >> /etc/sudoers

USER builder
WORKDIR /home/builder
