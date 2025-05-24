#!/bin/bash
# Run ShipNetSim GUI with X11 forwarding

xhost +local:docker

docker run --rm -it \
    -e DISPLAY=$DISPLAY \
    -e QT_X11_NO_MITSHM=1 \
    -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
    -v $(pwd)/data:/app/data \
    -v $(pwd)/output:/app/output \
    --device /dev/dri:/dev/dri \
    --network host \
    --name shipnetsim-gui \
    shipnetsim:latest \
    ./ShipNetSimGUI

xhost -local:docker
