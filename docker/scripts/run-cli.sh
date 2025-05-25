#!/bin/bash
# Run ShipNetSim CLI

docker run --rm -it \
    -v $(pwd)/data:/app/data \
    -v $(pwd)/output:/app/output \
    --name shipnetsim-cli \
    shipnetsim:latest \
    ./ShipNetSim "$@"
