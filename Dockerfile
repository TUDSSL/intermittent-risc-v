# Base Image
FROM ubuntu:22.04

# Set build arguments
ARG DEBIAN_FRONTEND=noninteractive
ARG USER_ID=1000
ARG GROUP_ID=1000

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    python3 \
    libboost-filesystem-dev \
    libboost-system-dev \
    libboost-program-options-dev \
    git \
    ninja-build \
    clang-12 \
    gcc-arm-none-eabi \
    wget \
    golang \
    python3-distutils \
    python3-tabulate \
    python3-pip \
    cm-super \
    dvipng \
    vim \
    neovim \
    cscope \
    exuberant-ctags \
    curl \
    sudo \
    texlive \
    texlive-latex-extra \
    texlive-extra-utils \
    && apt-get clean

# Install Python packages
RUN pip install matplotlib==3.5.2 pandas==1.4.4 nbconvert==6.5.4 ipykernel

# Create a new user
RUN addgroup --gid $GROUP_ID user && \
    adduser --uid $USER_ID --gid $GROUP_ID --disabled-password --gecos '' user && \
    adduser user sudo && \
    echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# Copy project files into the container as root
WORKDIR /home/user
COPY . /home/user

# Grant ownership to the non-root user
RUN chown -R user:user /home/user

# Configure Git to allow the /home/user directory
USER user
RUN git config --global --add safe.directory /home/user

# Set up environment variables from setup.sh
ENV LLVM_VERSION="16.0.2" \
    PATH="/home/user/bin:/home/user/host-bin:/home/user/icemu/icemu/bin:$PATH" \
    LLVM_BIN_ROOT="/home/user/llvm/llvm-${LLVM_VERSION}-bin" \
    LLVM_RC_ROOT="/home/user/llvm/llvm-${LLVM_VERSION}/install" \
    NOELLE="/home/user/noelle/noelle/install" \
    ICLANG_ROOT="/home/user" \
    ICEMU_PLUGIN_PATH="/home/user/icemu/plugins/build/plugins"

# Set up libgcc path from setup.sh
RUN libgcc_loc=$(arm-none-eabi-gcc -print-libgcc-file-name) && \
    libgcc_loc=$(dirname "$libgcc_loc") && \
    echo "export CMAKE_LIBGCC_ARM_BASE_DIR=${libgcc_loc}" >> /home/user/.bashrc

# Add sourceme.sh configurations
ENV MAKEFLAGS="-j$(nproc)" \
    CMAKE_GENERATOR="Ninja"

# Set the default command to open bash
CMD ["bash"]
