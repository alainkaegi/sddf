#!/bin/bash
HOST_DIR=$(pwd)
IMAGE=dependable/sddf

docker run \
    -it \
    --hostname in-container \
    --rm \
    --user $(id -u) \
    --ulimit nofile=65536:65536 \
    -v $HOST_DIR:/host:z \
    -v ~/.bash_history:/home/$(whoami)/.bash_history \
    $IMAGE
