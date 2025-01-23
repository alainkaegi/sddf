FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# ubuntu:24.04 now adds a non-root "ubuntu" user, this causes some problems so remove it
# [stackoverflow](https://askubuntu.com/questions/1513927/ubuntu-24-04-docker-images-now-includes-user-ubuntu-with-uid-gid-1000)
RUN userdel -r ubuntu

# Add a non-root user
ARG UNAME=user
ARG UID=1000
ARG GID=1000
RUN groupadd -g $GID -o $UNAME
RUN useradd -m -u $UID -g $GID -o -s /bin/bash $UNAME
ARG WORKING_DIR=/home/$UNAME
WORKDIR $WORKING_DIR

# Install basic utilites
RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get -y --fix-missing install \
    git \
    wget \
    device-tree-compiler

# Python for sel4 bitfield tools
RUN apt-get -y --fix-missing install \
    python3.12 \
    python3-pip \
    python3.12-venv

# Create a working directory for compiler tools
RUN mkdir tools
ARG WORKING_DIR=$WORKING_DIR/tools
WORKDIR $WORKING_DIR

# Install AArch64 GCC toolchain
ARG TRIPLE=aarch64-none-elf
ARG VERSION=12.2.rel1

RUN wget -O aarch64-toolchain.tar.xz "https://developer.arm.com/-/media/Files/downloads/gnu/$VERSION/binrel/arm-gnu-toolchain-$VERSION-x86_64-$TRIPLE.tar.xz"
RUN tar -Jxf aarch64-toolchain.tar.xz
RUN rm aarch64-toolchain.tar.xz

# Add to bash path and docker env
ARG AARCH_PATH=$WORKING_DIR/arm-gnu-toolchain-$VERSION-x86_64-$TRIPLE/bin
ENV PATH="$AARCH_PATH:$PATH"
RUN echo "export PATH=$PATH:\$PATH\n" >> /home/$UNAME/.bashrc

# Download microkit version currently used by sDDF
ARG WORKING_DIR=/home/$UNAME
WORKDIR $WORKING_DIR
RUN wget -O microkit.tar.gz "https://trustworthy.systems/Downloads/microkit/microkit-sdk-1.4.1-dev.54+a8b7894-linux-x86-64.tar.gz"
RUN tar -xf microkit.tar.gz
RUN rm microkit.tar.gz

# Create python virtual env and install local Python deps
ENV VIRTUAL_ENV=/opt/venv
RUN python3 -m venv $VIRTUAL_ENV
ENV PATH="$VIRTUAL_ENV/bin:$PATH"
RUN echo "export PATH=$PATH:\$PATH\n" >> /home/$UNAME/.bashrc
COPY requirements.txt $WORKING_DIR/requirements.txt
RUN python3 -m pip install -r requirements.txt

# Back to host directory
WORKDIR /host/

# Create link for expected location of the SDK
RUN ln -s /home/$UNAME/microkit-sdk-1.4.1-dev.54+a8b7894-linux-x86-64 /microkit

# Switch to USER
USER $UNAME
ENTRYPOINT [ "bash" ]
