#!/bin/bash
# Run ShipNetSim Server using host rabbitmq with configurable host and port

# Default values
DEFAULT_HOST="localhost"
DEFAULT_PORT="5672"

# Initialize variables with defaults
RABBITMQ_HOST="$DEFAULT_HOST"
RABBITMQ_PORT="$DEFAULT_PORT"

# Function to display usage
show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Run ShipNetSim Server with RabbitMQ connection options.

OPTIONS:
    -n, --hostname HOSTNAME    RabbitMQ server hostname (default: $DEFAULT_HOST)
    -p, --port PORT           RabbitMQ server port (default: $DEFAULT_PORT)
    -h, --help               Show this help message

NOTES:
    - 'localhost' refers to your host machine's localhost
    - The container will automatically resolve host connectivity
    - Use IP addresses for remote RabbitMQ servers

EXAMPLES:
    $0                                    # Connect to localhost:5672
    $0 -n localhost -p 5673              # Connect to localhost:5673
    $0 -n 192.168.1.100 -p 5672         # Connect to remote server

EOF
}

# Function to detect platform and resolve hostname
resolve_hostname_for_docker() {
    local hostname="$1"
    local resolved_hostname="$hostname"
    
    # Detect platform
    local platform=$(uname -s)
    
    case "$hostname" in
        "localhost"|"127.0.0.1")
            case "$platform" in
                "Linux")
                    # On Linux, we'll use host network mode, so localhost works directly
                    resolved_hostname="localhost"
                    ;;
                "Darwin"|"MINGW"*|"CYGWIN"*|"MSYS"*)
                    # On macOS/Windows, use host.docker.internal
                    resolved_hostname="host.docker.internal"
                    ;;
                *)
                    # Default fallback
                    resolved_hostname="host.docker.internal"
                    ;;
            esac
            ;;
        *)
            # For other hostnames (IPs, domain names), use as-is
            resolved_hostname="$hostname"
            ;;
    esac
    
    echo "$resolved_hostname"
}

# Function to determine network mode
get_network_mode() {
    local platform=$(uname -s)
    local hostname="$1"
    
    case "$hostname" in
        "localhost"|"127.0.0.1")
            case "$platform" in
                "Linux")
                    echo "host"
                    ;;
                *)
                    echo "bridge"
                    ;;
            esac
            ;;
        *)
            echo "bridge"
            ;;
    esac
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -n|--hostname)
            if [[ -n "$2" && "$2" != -* ]]; then
                RABBITMQ_HOST="$2"
                shift 2
            else
                echo "Error: Hostname argument required for $1"
                show_usage
                exit 1
            fi
            ;;
        -p|--port)
            if [[ -n "$2" && "$2" != -* ]]; then
                if [[ "$2" =~ ^[0-9]+$ ]] && [ "$2" -ge 1 ] && [ "$2" -le 65535 ]; then
                    RABBITMQ_PORT="$2"
                    shift 2
                else
                    echo "Error: Port must be a number between 1 and 65535"
                    exit 1
                fi
            else
                echo "Error: Port argument required for $1"
                show_usage
                exit 1
            fi
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            echo "Error: Unknown option $1"
            show_usage
            exit 1
            ;;
    esac
done

# Resolve hostname for Docker
RABBITMQ_HOST_RESOLVED=$(resolve_hostname_for_docker "$RABBITMQ_HOST")
NETWORK_MODE=$(get_network_mode "$RABBITMQ_HOST")

# Display configuration
echo "========================================="
echo "ShipNetSim Server Configuration"
echo "========================================="
echo "Target RabbitMQ: $RABBITMQ_HOST:$RABBITMQ_PORT"
if [[ "$RABBITMQ_HOST" != "$RABBITMQ_HOST_RESOLVED" ]]; then
    echo "Docker Hostname: $RABBITMQ_HOST_RESOLVED (resolved for container)"
fi
echo "Network Mode: $NETWORK_MODE"
echo "Platform: $(uname -s)"
echo "========================================="

# Export variables for docker-compose
export RABBITMQ_HOST
export RABBITMQ_HOST_RESOLVED
export RABBITMQ_PORT
export NETWORK_MODE

# Check Docker availability
if ! docker info > /dev/null 2>&1; then
    echo "Error: Docker is not running or not accessible"
    exit 1
fi

# Test connectivity to RabbitMQ before starting
echo "Testing RabbitMQ connectivity..."
if command -v nc >/dev/null 2>&1; then
    if nc -z "$RABBITMQ_HOST" "$RABBITMQ_PORT" 2>/dev/null; then
        echo "✅ RabbitMQ is accessible at $RABBITMQ_HOST:$RABBITMQ_PORT"
    else
        echo "⚠️  Warning: Cannot connect to $RABBITMQ_HOST:$RABBITMQ_PORT"
        echo "   Make sure RabbitMQ is running on the host"
        read -p "Continue anyway? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
fi

# Check if the image exists
if ! docker image inspect shipnetsim:latest > /dev/null 2>&1; then
    echo "Warning: shipnetsim:latest image not found"
    echo "Building the image first..."
    docker-compose build shipnetsim-builder
fi

echo ""
echo "Starting ShipNetSim Server..."
echo "Press Ctrl+C to stop the server"
echo ""

# Choose the appropriate docker-compose service based on network mode
if [[ "$NETWORK_MODE" == "host" ]]; then
    docker-compose up shipnetsim-server-host-network
else
    docker-compose up shipnetsim-server-host-bridge
fi