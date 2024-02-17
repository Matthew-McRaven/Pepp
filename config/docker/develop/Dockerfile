# Will be the latest LTS by the time we release the book, which I assume to be ~ 2025.
FROM ubuntu:22.04 as base-gcc
# Prevent apt-get from pausing for input in an environment in which it can never
ENV DEBIAN_FRONTEND=noninteractive
# Building boost + final build image needs all of the same tools.
# Combination of explicit Qt dependencies, implicit dependencies (as reported by aqtinstall),
# and my development dependencies.
RUN apt update \
    &&  apt install -y --no-install-recommends curl bash wget python3-pip zstd jq python3-dev \
        cloc curl build-essential libgl1-mesa-dev libpulse-dev libxcb-glx0 libxcb-icccm4 \
        libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 libxcb-render0 libxcb-shape0 \
        libxcb-shm0 libxcb-sync1 libxcb-util1 libxcb-xfixes0 libxcb-xinerama0 libxcb1 fontconfig ccache \
        libxkbcommon-dev libxcb-xkb-dev zip unzip tar git git-lfs lsb-release software-properties-common gnupg \
    && rm -rf /var/lib/apt/lists/*
# Install latest cmake that does not support modules, which is 3.27. Module detection breaks CI when the compiler doesn't support modules.
# Remove cmake docs, since that's 40MB we really don't need.
RUN wget -q -O cmake.sh "https://github.com/Kitware/CMake/releases/download/v3.27.7/cmake-3.27.7-linux-$(arch).sh" \
    && chmod +x cmake.sh && mkdir /cmake && ./cmake.sh --skip-license --exclude-subdir --prefix=/cmake && rm cmake.sh \
    && cp -r /cmake/* /usr && rm -rf /cmake && rm -rf /usr/doc/cmake
# Install ninja, which supports modules (for future development) and generally should be faster than make.
RUN wget -q -O ninja.zip "https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-linux.zip" \
    && unzip ninja.zip -d /bin && rm ninja.zip
# Need to pick this locale otherwise Qt complains and overrides the locale anyway.
ENV LANG="C.UTF-8"
# Must occur after the "FROM" directive, allows choice of Qt version.
ARG QT_VERSION="6.6.1"
# Don't cache pip internal files, reduce image size.
ENV PIP_NO_CACHE_DIR=1
SHELL ["/bin/bash", "-c"]
LABEL org.opencontainers.image.authors="Matthew McRaven <matthew.mcraven@gmail.com>"
LABEL org.opencontainers.image.source="https://github.com/Matthew-McRaven/Pepp"


FROM base-gcc as base-wasm
# Must install documentation-building tools in WASM image.
# Otherwise it will be very difficult to build GH pages deployments.
RUN apt update \
    && apt install -y --no-install-recommends graphviz imagemagick doxygen \
    && rm -rf /var/lib/apt/lists/*
# Insert workaround for Qt+breathe: https://github.com/breathe-doc/breathe/issues/574
# Must install exactly 4.35.0, otherwise insert may fail.
RUN pip install Sphinx==7.1.2 breathe==4.35.0 Pillow \
  && sed -i '253 i \        "property": (CMacroObject, "macro"),' "/usr/local/lib/python3.10/dist-packages/breathe/renderer/sphinxrenderer.py"
RUN git clone https://github.com/emscripten-core/emsdk.git /emsdk
# Must combine, otherwise a layer is the same in multi-arch builds, breaking everything
RUN cd /emsdk && git pull && ./emsdk install 3.1.37  && ./emsdk activate 3.1.37 && echo uname -a /emsdk.arch


FROM base-wasm as output-wasm
RUN pip install aqtinstall \
    && aqt install-qt linux desktop ${QT_VERSION} wasm_singlethread --outputdir /qt  --autodesktop \
    && pip uninstall aqtinstall -y && rm aqtinstall.log
# Define standard Qt environment variables so that executing cmake directly will work.
ENV LD_LIBRARY_PATH=/qt/${QT_VERSION}/wasm_singlethread/lib:${LD_LIBRARY_PATH}
ENV Qt6_DIR=/qt/${QT_VERSION}/wasm_singlethread/lib/cmake/Qt6
ENV QT_PLUGIN_PATH=/qt/${QT_VERSION}/wasm_singlethread/plugins
ENV QML2_IMPORT_PATH=/qt/${QT_VERSION}/wasm_singlethread/qml
ENV PATH=/qt/${QT_VERSION}/wasm_singlethread/bin/:$PATH
LABEL description="A wasm build container using Qt $QT_VERSION."


FROM base-gcc as output-gcc
# Install Qt with aqtinstall (see https://github.com/miurahr/aqtinstall), selecting only the modules we need.
RUN pip install aqtinstall && aqt install-qt linux desktop ${QT_VERSION} --outputdir /qt && pip uninstall aqtinstall -y && rm aqtinstall.log
# Define standard Qt environment variables so that executing cmake directly will work.
ENV LD_LIBRARY_PATH=/qt/${QT_VERSION}/gcc_64/lib:${LD_LIBRARY_PATH}
ENV Qt6_DIR=/qt/${QT_VERSION}/gcc_64
ENV QT_PLUGIN_PATH=/qt/${QT_VERSION}/gcc_64/plugins
ENV QML2_IMPORT_PATH=/qt/${QT_VERSION}/gcc_64/qml
ENV PATH=/qt/${QT_VERSION}/gcc_64/bin/:$PATH
# Install mold, so that we can reduce link times
ARG MOLD_VERSION="2.3.1"
RUN --mount=type=tmpfs,target=/mold,size=2G \
    cd mold \
    && wget -q -O mold.tar.gz "https://github.com/rui314/mold/releases/download/v$MOLD_VERSION/mold-$MOLD_VERSION-$(arch)-linux.tar.gz" \
    && tar xf mold.tar.gz && ls && cd mold-$MOLD_VERSION-$(arch)-linux \
    && find -type d -exec mkdir -vp ""/{} \; -or -exec mv -nv {} ""/{} \;
    # No need to clean up, all temp data should be in tmpfs
LABEL description="A GCC build container using Qt $QT_VERSION, and Mold $MOLD_VERSION."


# LLVM-install script inspired by: https://github.com/teeks99/clang-ubuntu-docker
#MIT License
#
#Copyright (c) 2018 Tom Kent
#
#Permission is hereby granted, free of charge, to any person obtaining a copy
#of this software and associated documentation files (the "Software"), to deal
#in the Software without restriction, including without limitation the rights
#to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#copies of the Software, and to permit persons to whom the Software is
#furnished to do so, subject to the following conditions:
#
#The above copyright notice and this permission notice shall be included in all
#copies or substantial portions of the Software.
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#SOFTWARE.
FROM output-gcc as output-clang
ARG LLVM_VERSION="17"
ADD llvm.list /etc/apt/sources.list.d/
ADD llvm-snapshot.gpg.key.gpg /etc/apt/trusted.gpg.d/
RUN apt update && apt install  --no-install-recommends -y \
    ca-certificates clang-$LLVM_VER clang-tools-$LLVM_VERSION clang-format-$LLVM_VERSION \
    libfuzzer-${LLVM_VERSION}-dev lld-$LLVM_VERSION \
    && ln -s /usr/bin/clang++-$LLVM_VERSION /usr/bin/clang++ \
    && ln -s /usr/bin/clang-$LLVM_VERSION /usr/bin/clang \
    && rm -rf /var/lib/apt/lists/*
LABEL description="A clang $LLVM_VERSION build container using Qt $QT_VERSION."
