FROM ubuntu:20.04

ARG USER_ID=1000
ARG GROUP_ID=1000

ARG DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && \
    apt-get install -y build-essential cmake python3 libboost-filesystem-dev  \
    libboost-system-dev libboost-program-options-dev vim git ninja-build clang \
    gcc-arm-none-eabi wget golang \
    python3-distutils python3-tabulate python3-pip cm-super dvipng

RUN pip install matplotlib==3.5.1
RUN pip install pandas==1.4.1


# Create a new user
RUN addgroup --gid $GROUP_ID user
RUN useradd -u $USER_ID -g $GROUP_ID -ms /bin/bash user

# Change the working directory
WORKDIR /home/user

# Change the user
USER user

# Install gllvm (https://github.com/SRI-CSL/gllvm)
RUN go get github.com/SRI-CSL/gllvm/cmd/...

## Configure the environment
RUN echo "export PATH=$HOME/go/bin:\$PATH" >> "$HOME/.env"
RUN echo "source $HOME/.env" >> "$HOME/.bashrc"
