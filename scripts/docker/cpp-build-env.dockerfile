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
  && echo 'deb http://ftp.debian.org/debian stretch-backports main' >> /etc/apt/sources.list \
  && echo 'deb http://apt.llvm.org/stretch/ llvm-toolchain-stretch-5.0 main' >> /etc/apt/sources.list \
  && echo 'deb http://apt.llvm.org/stretch/ llvm-toolchain-stretch-6.0 main' >> /etc/apt/sources.list \
  && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys \
    6084F3CF814B57C1CF12EFD515CF4D18AF4F7421 \
    60C317803A41BA51845E371A1E9377A2BA9EF27F \
  && apt-get -qq update \
  && apt-get install -yq --no-install-recommends \
    # Build tools
    git \
    ssh-client \
    make \
    ninja-build \
    python \
    python-pip \
    python3-pip \
    python3-requests \
    python3-git \
    ccache \
    doxygen \
    # Compilers
    g++ \
    g++-4.8 \
    # g++-7 \
    clang-3.8 \
    clang-6.0 \
    clang-format-6.0 \
    clang-tidy-6.0 \
    llvm-6.0-dev \
    llvm-5.0-dev \
    libncurses-dev \
    libz-dev \
    # Dependencies
    libleveldb-dev \
  && apt-get -t stretch-backports install -yq --no-install-recommends \
    cmake \
  && rm -rf /var/lib/apt/lists/* \
  && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-6.0 1 \
  && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-6.0 1 \
  && update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-6.0 1 \
  && update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-6.0 1 \
  && update-alternatives --install /usr/bin/llvm-symbolizer llvm-symbolizer /usr/bin/llvm-symbolizer-6.0 1 \
  && update-alternatives --install /usr/bin/gcov gcov /usr/bin/llvm-cov-6.0 1 \
  && pip3 install setuptools \
  && pip3 install codecov codespell

RUN adduser --disabled-password --gecos '' builder && adduser builder sudo && printf 'builder\tALL=NOPASSWD: ALL\n' >> /etc/sudoers

USER builder
WORKDIR /home/builder