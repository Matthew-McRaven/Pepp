# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.154.2/containers/cpp/.devcontainer/base.Dockerfile

# [Choice] Debian / Ubuntu version: debian-10, debian-9, ubuntu-20.04, ubuntu-18.04
ARG VARIANT="ubuntu-20.04"


FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-${VARIANT}

# Must occur after the "FROM" directive
ARG BOOST_VERSION="1.76.0"
SHELL ["/bin/bash", "-c"]

# Required to move an ARG to an environment variable.
RUN echo $BOOST_VERSION
# Install additional packages.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
     && apt-get -y install --no-install-recommends g++-10 make zip cmake git libsqlite3-dev zlib1g-dev \
     libreadline-gplv2-dev libncursesw5-dev libssl-dev tk-dev libgdbm-dev libc6-dev libbz2-dev libffi-dev \
     python3.9 python3.9-dev python3.9-distutils vim ripgrep doxygen clang-tidy ssh-client lcov libsecret-1-0 \
     libx11-dev libxkbfile-dev lsb-release wget software-properties-common graphviz
     
# Install pip for python 3.9
RUN curl https://bootstrap.pypa.io/get-pip.py | python3.9
RUN python3.9 -m pip install pytest

# Install LLVM 14
RUN wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh &&  ./llvm.sh 14 && rm llvm.sh

WORKDIR /

# Download boost code.
RUN git clone --jobs 10 --recursive --branch boost-${BOOST_VERSION}  https://github.com/boostorg/boost && git clone https://github.com/emscripten-core/emsdk.git /emsdk

# Install EMSDK
RUN cd /emsdk && git pull && ./emsdk install latest && ./emsdk activate latest
RUN source /emsdk/emsdk_env.sh && npm i -g corepack && yarn set version stable

# Copy boost configuration, which is necessary for building python librs correctly.
COPY user-config.jam /boost/user-config.jam
RUN ls -la /boost
# Build boost, with python, in a static confgiuration so that we don't need to deploy it.
# Boost will install to /usr/local/lib
RUN cd boost && ./bootstrap.sh --with-python-version=3.9 \
    && ./b2 cxxflags="-fPIC -std=c++17" --user-config=/boost/user-config.jam address-model=64 link=static runtime-link=static -j 20 threading=multi --build-dir=./build-native --stagedir=./stage-native \
    && ./b2 cxxflags="-fPIC -std=c++17" --user-config=/boost/user-config.jam address-model=64 link=static runtime-link=static -j 20 threading=multi install --build-dir=./build-native --stagedir=./stage-native

RUN source /emsdk/emsdk_env.sh \
    && cd boost \
    && ./b2 --clean-all -n \
    && emconfigure ./b2 cxxflags="-fPIC -std=c++17" link=static runtime-link=static -j 20 --build-dir=./build-js --stagedir=./stage-js \
    && emconfigure ./b2 cxxflags="-fPIC -std=c++17" link=static runtime-link=static -j 20 install --build-dir=./build-js --stagedir=./stage-js
