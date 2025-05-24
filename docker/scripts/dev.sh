#!/bin/bash
# Start development environment

xhost +local:docker

docker run --rm -it \
    -e DISPLAY=$DISPLAY \
    -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
    -v $(pwd):/workspace \
    --network host \
    --name shipnetsim-dev \
    shipnetsim:dev \
    /bin/bash

xhost -local:docker
