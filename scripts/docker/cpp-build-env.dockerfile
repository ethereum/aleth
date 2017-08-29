# get latest stable debian release image -- see https://hub.docker.com/_/debian/
FROM debian:stretch

# maintainer label
LABEL maintainerName="C++ Ethereum developers team"
LABEL repo="https://github.com/ethereum/cpp-ethereum"
LABEL version="0.0.1"
LABEL description="A Docker image for building cpp-ethereum."
RUN export DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get upgrade -yq && \
    apt-get install -yq software-properties-common && \
    apt-get install -yq build-essential && \
    apt-get install -yq libleveldb-dev && \
    apt-get install -yq libmicrohttpd-dev && \
    apt-get install wget -y

RUN apt-get install -yq cmake && \
    apt-add-repository "deb http://apt.llvm.org/stretch/ llvm-toolchain-stretch-4.0 main" && \
    # add gpg key of that repo to confirm authentic downloads
    wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key|apt-key add - && \
    # refresh package search corpus
    apt-get update && \
    # download all clang-4.0 packages
    apt-get install -yq clang-4.0 clang-4.0-dbgsym clang-4.0-doc clang-4.0-examples libclang-4.0-dev python-clang-4.0 && \
    # set keyword clang to reference clang-4.0 as default
    update-alternatives --install /usr/bin/clang clang /usr/bin/clang-4.0 100
