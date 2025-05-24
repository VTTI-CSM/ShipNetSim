# Docker Configuration for ShipNetSim

This directory contains Docker configurations for building and running ShipNetSim.

## Quick Start

1. Build the Docker image:
```bash
./scripts/build.sh
```

2. Run different versions:
```bash
# GUI version (requires X11) ./scripts/run-gui.sh # CLI version ./scripts/run-cli.sh # Server with RabbitMQ ./scripts/run-server.sh # Development environment ./scripts/dev.sh
```

# Building from specific branch

```bash
docker build \
    --build-arg GITHUB_BRANCH=develop \
    -t shipnetsim:develop \
    https://github.com/VTTI-CSM/ShipNetSim.git
```

# Docker Compose Commands

```bash
# Build all services
docker-compose build

# Run specific service
docker-compose up shipnetsim-server

# Run with detached mode
docker-compose up -d rabbitmq shipnetsim-server

# Stop all services
docker-compose down
```

## X11 Forwarding for GUI
### Linux
```bash
xhost +local:docker 
./scripts/run-gui.sh 
xhost -local:docker
```

### macOS (requires XQuartz)
1. Install XQuartz: brew install --cask xquartz
2. Enable "Allow connections from network clients" in XQuartz preferences
3. Run with IP address:

```bash
IP=$(ifconfig en0 | grep inet | awk '$1=="inet" {print $2}')
xhost + $IP
docker run -e DISPLAY=$IP:0 ... shipnetsim:latest ./ShipNetSimGUI
```

### Windows (requires VcXsrv)
1. Install and run VcXsrv with "Disable access control"
2. Get your IP: ipconfig
3. Run with: docker run -e DISPLAY=YOUR_IP:0.0 ... shipnetsim:latest ./ShipNetSimGUI
# Troubleshooting
	•	Cannot connect to X server: Check DISPLAY variable and xhost permissions
	•	OpenGL issues: Try export LIBGL_ALWAYS_SOFTWARE=1
	•	Qt platform issues: Install additional plugins or use -platform offscreen

