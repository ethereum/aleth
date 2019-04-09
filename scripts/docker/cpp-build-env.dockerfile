FROM debian:testing

LABEL maintainer="C++ Ethereum team"
LABEL repo="https://github.com/ethereum/aleth"
LABEL version="11"
LABEL description="Build environment for C++ Ethereum projects"

RUN export DEBIAN_FRONTEND=noninteractive \
  && apt-get -qq update && apt-get install -yq --no-install-recommends \
    curl \
    dirmngr \
    gnupg2 \
    sudo \
  && rm -rf /var/lib/apt/lists/*

RUN export DEBIAN_FRONTEND=noninteractive \
  && echo 'deb http://apt.llvm.org/unstable/ llvm-toolchain-6.0 main' >> /etc/apt/sources.list \
  && echo 'deb http://apt.llvm.org/unstable/ llvm-toolchain-8 main' >> /etc/apt/sources.list \
  && apt-key adv --keyserver keyserver.ubuntu.com --no-tty --recv-keys \
    6084F3CF814B57C1CF12EFD515CF4D18AF4F7421 \
    60C317803A41BA51845E371A1E9377A2BA9EF27F \
  && apt-get -qq update \
  && apt-get install -yq --no-install-recommends \
    # Build tools
    git \
    ssh-client \
    cmake \
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
    g++-7 \
    g++-8 \
    clang-3.8 \
    clang-8 \
    clang-format-8 \
    clang-tidy-8 \
    llvm-6.0-dev \
    llvm-8-dev \
    libncurses-dev \
    libz-dev \
    # Dependencies
    libleveldb-dev \
  && rm -rf /var/lib/apt/lists/* \
  && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-8 1 \
  && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-8 1 \
  && update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-8 1 \
  && update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-8 1 \
  && update-alternatives --install /usr/bin/llvm-symbolizer llvm-symbolizer /usr/bin/llvm-symbolizer-8 1 \
  && update-alternatives --install /usr/bin/gcov gcov /usr/bin/llvm-cov-8 1 \
  && pip3 install setuptools \
  && pip3 install codecov codespell

RUN adduser --disabled-password --gecos '' builder && adduser builder sudo && printf 'builder\tALL=NOPASSWD: ALL\n' >> /etc/sudoers

USER builder
WORKDIR /home/builder