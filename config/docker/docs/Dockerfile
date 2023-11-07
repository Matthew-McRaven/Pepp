FROM sphinxdoc/sphinx:7.1.2
ENV DEBIAN_FRONTEND=noninteractive

RUN apt update &&  apt install -y --no-install-recommends doxygen wget && rm -rf /var/lib/apt/lists/*

# Install latest cmake that does not support modules, which is 3.27. Module detection breaks CI when the compiler doesn't support modules.
RUN wget -O cmake.sh "https://github.com/Kitware/CMake/releases/download/v3.27.7/cmake-3.27.7-linux-$(arch).sh" \
    && chmod +x cmake.sh && mkdir /cmake && ./cmake.sh --skip-license --exclude-subdir --prefix=/cmake && rm cmake.sh \
    && cp -r /cmake/* /usr && rm -rf /cmake
# Define standard Qt environment variables so

RUN pip install breathe

# Insert workaround for Qt+breathe: https://gitmemories.com/michaeljones/breathe/issues/574
RUN sed -i '253 i \        "property": (CMacroObject, "macro"),' "/usr/local/lib/python3.11/site-packages/breathe/renderer/sphinxrenderer.py"
#RUN cat -n /usr/local/lib/python3.11/site-packages/breathe/renderer/sphinxrenderer.py
