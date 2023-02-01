FROM ubuntu:20.04

ARG USER_ID=1000
ARG GROUP_ID=1000

ARG DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && \
    apt-get install -y build-essential cmake python3 libboost-filesystem-dev  \
    libboost-system-dev libboost-program-options-dev git ninja-build clang \
    gcc-arm-none-eabi wget golang \
    python3-distutils python3-tabulate python3-pip cm-super dvipng \
    vim neovim cscope ctags curl sudo

# Chipyard dependencies
# https://chipyard.readthedocs.io/en/stable/Chipyard-Basics/Initial-Repo-Setup.html
# install sbt: https://www.scala-sbt.org/release/docs/Installing-sbt-on-Linux.html#Ubuntu+and+other+Debian-based+distributions
RUN echo "deb https://repo.scala-sbt.org/scalasbt/debian /" | tee -a /etc/apt/sources.list.d/sbt.list && \
    curl -sL "https://keyserver.ubuntu.com/pks/lookup?op=get&search=0x2EE0EA64E40A89B84B2DF73499E82A75642AC823" | apt-key add
RUN apt-get update && \
    apt-get install -y \
    bison flex software-properties-common \
    libgmp-dev libmpfr-dev libmpc-dev zlib1g-dev default-jdk default-jre \
    sbt \
    texinfo gengetopt \
    libexpat1-dev libusb-dev libncurses5-dev cmake \
    patch diffstat texi2html texinfo subversion chrpath \
    libgtk-3-dev gettext \
    python3-pip python3-dev rsync libguestfs-tools expat \
    device-tree-compiler \
    python autoconf


RUN pip install matplotlib==3.5.1
RUN pip install pandas==1.4.1

# Create a new user
RUN addgroup --gid $GROUP_ID user
#RUN useradd -u $USER_ID -g $GROUP_ID -ms /bin/bash user

# Make the user able to do sudo without a password
RUN adduser --uid $USER_ID --gid $GROUP_ID --disabled-password --gecos '' user

#  Add new user docker to sudo group
RUN adduser user sudo

# Ensure sudo group users are not
# asked for a password when using
# sudo command by ammending sudoers file
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# Remove the sudo messasge
RUN touch /home/user/.sudo_as_admin_successful

# Change the working directory
WORKDIR /home/user

# Change the user
USER user

# Install gllvm (https://github.com/SRI-CSL/gllvm)
RUN git -c advice.detachedHead=false clone https://github.com/SRI-CSL/gllvm -b v1.3.0 && \
    cd gllvm && go mod init github.com/SRI-CSL/gllvm && make install

## Configure the environment
RUN echo "export PATH=$HOME/go/bin:\$PATH" >> "$HOME/.env"
RUN echo "source $HOME/.env" >> "$HOME/.bashrc"
