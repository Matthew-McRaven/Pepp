FROM ubuntu:latest
ENV DEBIAN_FRONTEND=noninteractive 
RUN apt update && apt install -y --no-install-recommends  \
        texlive-latex-base texlive-fonts-recommended texlive-fonts-extra texlive-latex-extra latexmk texlive-xetex wget

# Install non-free fonts, like times.
RUN wget https://tug.org/fonts/getnonfreefonts/install-getnonfreefonts && texlua install-getnonfreefonts
