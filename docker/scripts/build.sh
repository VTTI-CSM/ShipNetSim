#!/bin/bash
# Build ShipNetSim from GitHub repository

echo "Building ShipNetSim from GitHub..."
docker build -t shipnetsim:latest https://github.com/VTTI-CSM/ShipNetSim.git

echo "Build complete!"
