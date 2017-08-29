# build:          docker build -t test_build_20 .
# run:            docker run -it --rm test_build_20:latest
# versioning:     docker tag cpp-build-env ethereum/cpp-build-env:v0.8.29.2017
# upload:         docker push tcsiwula/cpp-build-env

# get latest stable debian release image -- see https://hub.docker.com/_/debian/
from debian:latest

# maintainer email
MAINTAINER Tim Siwula <tcsiwula@gmail.com>

# by defualt, docker runs all processes as root within the container
USER root

RUN apt-get update -yq && \
    apt-get install -yq software-properties-common && \
    apt-get install -yq --no-install-recommends apt-utils && \
    apt-get install -yq build-essential && \
    apt-get install -yq libleveldb-dev && \
    apt-get install -yq libmicrohttpd-dev && \
# version cmake 3
    apt-get install -yq cmake && \
# version clang 3 -- unable to locate
    apt-get install -yq clang && \
# install gcc-6
    apt-get install -yq gcc-6
# install gcc-7 -- unable to locate
    #apt-get install -yq gcc-7-base

ENTRYPOINT ["/bin/bash"]
