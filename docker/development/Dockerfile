FROM ubuntu:20.04

ARG USER_ID=1000
ARG GROUP_ID=1000

ARG DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && \
    apt-get install -y build-essential cmake python3 libboost-filesystem-dev  \
    libboost-system-dev libboost-program-options-dev git ninja-build clang-12 \
    gcc-arm-none-eabi wget golang \
    python3-distutils python3-tabulate python3-pip cm-super dvipng \
    vim neovim cscope ctags curl sudo

# Install components required for plotting
RUN apt-get install -y texlive texlive-latex-extra texlive-extra-utils
RUN pip install matplotlib==3.5.2 pandas==1.4.4 nbconvert==6.5.4 ipykernel

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

## Configure the environment
RUN echo "export PATH=$HOME/go/bin:\$PATH" >> "$HOME/.env"
RUN echo "source $HOME/.env" >> "$HOME/.bashrc"
