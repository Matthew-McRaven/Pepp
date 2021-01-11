FROM ubuntu:latest
ENV DEBIAN_FRONTEND=noninteractive 
RUN apt update && apt install -y texlive-latex-base texlive-fonts-recommended texlive-fonts-extra texlive-latex-extra latexmk texlive-xetex

# Install non-free fonts, like times.
RUN apt update && apt install -y wget && wget http://tug.org/fonts/getnonfreefonts/install-getnonfreefonts && texlua install-getnonfreefonts