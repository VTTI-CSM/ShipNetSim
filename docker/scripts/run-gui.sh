#!/bin/bash
# Run ShipNetSim GUI with cross-platform support (macOS, Linux, and Windows)

# Function to detect operating system
detect_os() {
    case "$(uname -s)" in
        Darwin*)    echo "macos" ;;
        Linux*)     echo "linux" ;;
        CYGWIN*|MINGW*|MSYS*) echo "windows" ;;
        *)          echo "unknown" ;;
    esac
}

# Function to detect if running in WSL
is_wsl() {
    if [ -n "${WSL_DISTRO_NAME}" ] || [ -n "${WSLENV}" ]; then
        return 0
    elif [ -f /proc/version ] && grep -qi microsoft /proc/version; then
        return 0
    else
        return 1
    fi
}

# Function to get Windows host IP from WSL
get_windows_host_ip() {
    # Try multiple methods to get Windows host IP
    local windows_ip
    
    # Method 1: Use WSL2 host IP
    windows_ip=$(ip route show default | awk '/default/ {print $3}')
    
    # Method 2: Parse /etc/resolv.conf
    if [ -z "$windows_ip" ]; then
        windows_ip=$(grep nameserver /etc/resolv.conf | awk '{print $2}' | head -1)
    fi
    
    # Method 3: Use PowerShell to get host IP
    if [ -z "$windows_ip" ] && command -v powershell.exe >/dev/null 2>&1; then
        windows_ip=$(powershell.exe -Command "(Get-NetIPAddress | Where-Object {`$_.AddressFamily -eq 'IPv4' -and `$_.PrefixOrigin -eq 'Dhcp'}).IPAddress" 2>/dev/null | tr -d '\r' | head -1)
    fi
    
    # Fallback
    if [ -z "$windows_ip" ]; then
        windows_ip="host.docker.internal"
    fi
    
    echo "$windows_ip"
}

# Function to setup X11 for macOS
setup_macos_x11() {
    echo "🍎 Detected macOS - Setting up XQuartz..."
    
    # Check if XQuartz is installed
    if [ ! -f "/opt/X11/bin/xhost" ]; then
        echo "❌ XQuartz is not installed. Install it with:"
        echo "   brew install --cask xquartz"
        echo "   Then restart and run this script again."
        exit 1
    fi
    
    # Check if XQuartz is running
    if ! pgrep -x "XQuartz" > /dev/null; then
        echo "❌ XQuartz is not running. Starting XQuartz..."
        open -a XQuartz
        echo "⏳ Waiting for XQuartz to start..."
        sleep 3
        
        # Wait for XQuartz to be ready
        timeout=30
        while [ $timeout -gt 0 ] && ! pgrep -x "XQuartz" > /dev/null; do
            sleep 1
            timeout=$((timeout - 1))
        done
        
        if ! pgrep -x "XQuartz" > /dev/null; then
            echo "❌ Failed to start XQuartz. Please start it manually:"
            echo "   1. Open XQuartz application"
            echo "   2. Go to Preferences → Security"
            echo "   3. Check 'Allow connections from network clients'"
            exit 1
        fi
    fi
    
    # Get IP address for Docker to connect to
    IP=$(ifconfig en0 2>/dev/null | grep 'inet ' | awk '{print $2}')
    if [ -z "$IP" ]; then
        IP=$(ifconfig en1 2>/dev/null | grep 'inet ' | awk '{print $2}')
    fi
    if [ -z "$IP" ]; then
        IP="host.docker.internal"
    fi
    
    echo "🔗 Using IP address: $IP"
    
    # Allow X11 forwarding
    /opt/X11/bin/xhost + $IP > /dev/null 2>&1
    
    # Set environment variables for macOS
    DISPLAY_VAR="$IP:0"
    DOCKER_ARGS="--add-host=host.docker.internal:host-gateway"
    XHOST_CMD="/opt/X11/bin/xhost"
    LIBGL_SETTING="1"
}

# Function to setup X11 for Linux
setup_linux_x11() {
    echo "🐧 Detected Linux - Setting up X11..."
    
    # Check if X11 is running
    if [ -z "$DISPLAY" ]; then
        echo "❌ No X11 display detected. Make sure you're running in a graphical environment."
        exit 1
    fi
    
    # Check if xhost command exists
    if ! command -v xhost >/dev/null 2>&1; then
        echo "❌ xhost command not found. Install it with:"
        echo "   Ubuntu/Debian: sudo apt-get install x11-xserver-utils"
        echo "   CentOS/RHEL: sudo yum install xorg-x11-server-utils"
        echo "   Arch: sudo pacman -S xorg-xhost"
        exit 1
    fi
    
    # Allow local Docker connections
    xhost +local:docker > /dev/null 2>&1
    
    # Set environment variables for Linux
    DISPLAY_VAR="$DISPLAY"
    
    # Check if DRI devices exist for hardware acceleration
    if [ -d "/dev/dri" ]; then
        DOCKER_ARGS="--device /dev/dri:/dev/dri"
        echo "🎮 Hardware acceleration enabled"
    else
        DOCKER_ARGS=""
        echo "⚠️  No DRI devices found - software rendering only"
    fi
    
    XHOST_CMD="xhost"
    LIBGL_SETTING="0"
}

# Function to setup X11 for Windows/WSL
setup_windows_x11() {
    if is_wsl; then
        echo "🪟 Detected WSL - Setting up X11 forwarding to Windows..."
        setup_wsl_x11
    else
        echo "🪟 Detected Windows - Setting up X11..."
        setup_native_windows_x11
    fi
}

# Function to setup X11 for WSL
setup_wsl_x11() {
    # Get Windows host IP
    WINDOWS_IP=$(get_windows_host_ip)
    echo "🔗 Windows host IP: $WINDOWS_IP"
    
    # Check if X11 server is running on Windows
    if ! timeout 2 bash -c "</dev/tcp/$WINDOWS_IP/6000" 2>/dev/null; then
        echo "❌ No X11 server detected on Windows host at $WINDOWS_IP:6000"
        echo ""
        echo "📋 To run GUI applications from WSL, you need an X11 server on Windows:"
        echo ""
        echo "🎯 Option 1: VcXsrv (Recommended)"
        echo "   1. Download and install VcXsrv: https://sourceforge.net/projects/vcxsrv/"
        echo "   2. Start XLaunch with these settings:"
        echo "      - Display: Multiple windows"
        echo "      - Client startup: Start no client"
        echo "      - Extra settings: ✅ Disable access control"
        echo "      - Extra settings: ✅ Native opengl (optional)"
        echo ""
        echo "🎯 Option 2: X410 (Microsoft Store)"
        echo "   1. Install X410 from Microsoft Store"
        echo "   2. Start X410"
        echo ""
        echo "🎯 Option 3: WSLg (Windows 11 with WSL2)"
        echo "   1. Update to Windows 11"
        echo "   2. Use: wsl --update"
        echo "   3. WSLg provides built-in GUI support"
        echo ""
        exit 1
    fi
    
    echo "✅ X11 server detected on Windows"
    
    # Set environment variables for WSL
    DISPLAY_VAR="$WINDOWS_IP:0"
    DOCKER_ARGS="--add-host=host.docker.internal:$WINDOWS_IP"
    XHOST_CMD=""
    LIBGL_SETTING="1"
}

# Function to setup X11 for native Windows (Git Bash/MinGW/Cygwin)
setup_native_windows_x11() {
    echo "⚠️  Running on native Windows (not WSL)"
    echo ""
    echo "📋 For GUI applications on Windows, you have these options:"
    echo ""
    echo "🎯 Option 1: Use WSL2 (Recommended)"
    echo "   1. Install WSL2: wsl --install"
    echo "   2. Run this script from within WSL2"
    echo ""
    echo "🎯 Option 2: Use Docker Desktop with WSL2 backend"
    echo "   1. Install Docker Desktop for Windows"
    echo "   2. Enable WSL2 integration"
    echo "   3. Run from WSL2 terminal"
    echo ""
    echo "🎯 Option 3: Use X11 server + Docker Toolbox (Legacy)"
    echo "   1. Install VcXsrv or X410"
    echo "   2. Configure Docker to use X11 forwarding"
    echo ""
    exit 1
}

# Function to check if running with WSLg (Windows 11 WSL2)
check_wslg() {
    if is_wsl && [ -n "$WAYLAND_DISPLAY" ]; then
        echo "🎮 Detected WSLg (Windows 11 WSL2 with built-in GUI support)"
        DISPLAY_VAR="$DISPLAY"
        DOCKER_ARGS="--volume=/tmp/.X11-unix:/tmp/.X11-unix:rw --volume=/mnt/wslg:/mnt/wslg:rw"
        XHOST_CMD=""
        LIBGL_SETTING="0"
        return 0
    fi
    return 1
}

# Function to cleanup X11 permissions
cleanup_x11() {
    echo "🧹 Cleaning up X11 permissions..."
    case $OS in
        "macos")
            if [ ! -z "$IP" ] && [ "$IP" != "host.docker.internal" ]; then
                /opt/X11/bin/xhost - $IP > /dev/null 2>&1
            fi
            ;;
        "linux")
            if [ ! -z "$XHOST_CMD" ]; then
                xhost -local:docker > /dev/null 2>&1
            fi
            ;;
        "windows")
            # No cleanup needed for Windows/WSL
            ;;
    esac
}

# Function to check Docker
check_docker() {
    if ! command -v docker >/dev/null 2>&1; then
        echo "❌ Docker is not installed or not in PATH"
        case $OS in
            "macos")
                echo "   Install Docker Desktop for Mac: https://docs.docker.com/docker-for-mac/install/"
                ;;
            "linux")
                echo "   Install Docker: https://docs.docker.com/engine/install/"
                ;;
            "windows")
                if is_wsl; then
                    echo "   Install Docker Desktop for Windows with WSL2 backend"
                    echo "   Or install Docker in WSL2: https://docs.docker.com/engine/install/ubuntu/"
                else
                    echo "   Install Docker Desktop for Windows: https://docs.docker.com/docker-for-windows/install/"
                fi
                ;;
        esac
        exit 1
    fi
    
    if ! docker info >/dev/null 2>&1; then
        echo "❌ Docker is not running or you don't have permission to access it"
        if is_wsl; then
            echo "   Make sure Docker Desktop is running and WSL2 integration is enabled"
        fi
        exit 1
    fi
}

# Function to check if image exists
check_image() {
    if ! docker image inspect shipnetsim:latest >/dev/null 2>&1; then
        echo "❌ Docker image 'shipnetsim:latest' not found"
        echo "   Build it first with: docker build -t shipnetsim:latest ."
        exit 1
    fi
}

# Main execution
main() {
    echo "🚢 Starting ShipNetSim GUI..."
    
    # Detect OS and setup accordingly
    OS=$(detect_os)
    
    case $OS in
        "macos")
            setup_macos_x11
            ;;
        "linux")
            setup_linux_x11
            ;;
        "windows")
            # Check for WSLg first (Windows 11 WSL2)
            if check_wslg; then
                echo "✅ Using WSLg for GUI support"
            else
                setup_windows_x11
            fi
            ;;
        *)
            echo "❌ Unsupported operating system: $(uname -s)"
            echo "   This script supports macOS, Linux, and Windows (WSL) only"
            exit 1
            ;;
    esac
    
    # Check prerequisites
    check_docker
    check_image
    
    # Create data and output directories if they don't exist
    mkdir -p "$(pwd)/data" "$(pwd)/output"
    
    echo "🐳 Starting Docker container..."
    
    # Set up trap to cleanup on script exit
    trap cleanup_x11 EXIT
    
    # Build docker run command
    docker_cmd="docker run --rm -it"
    docker_cmd="$docker_cmd -e DISPLAY=\"$DISPLAY_VAR\""
    docker_cmd="$docker_cmd -e QT_X11_NO_MITSHM=1"
    docker_cmd="$docker_cmd -e LIBGL_ALWAYS_INDIRECT=\"$LIBGL_SETTING\""
    
    # Add volumes
    if [ "$OS" != "windows" ] || check_wslg; then
        docker_cmd="$docker_cmd -v /tmp/.X11-unix:/tmp/.X11-unix:rw"
    fi
    docker_cmd="$docker_cmd -v \"$(pwd)/data:/app/data\""
    docker_cmd="$docker_cmd -v \"$(pwd)/output:/app/output\""
    
    # Add platform-specific arguments
    if [ ! -z "$DOCKER_ARGS" ]; then
        docker_cmd="$docker_cmd $DOCKER_ARGS"
    fi
    
    docker_cmd="$docker_cmd --network host"
    docker_cmd="$docker_cmd --name shipnetsim-gui"
    docker_cmd="$docker_cmd shipnetsim:latest"
    docker_cmd="$docker_cmd ./ShipNetSimGUI"
    
    # Execute the command
    eval $docker_cmd
    
    echo "✅ ShipNetSim GUI container finished"
}

# Handle script interruption
handle_interrupt() {
    echo ""
    echo "🛑 Script interrupted by user"
    cleanup_x11
    exit 130
}

trap handle_interrupt INT TERM

# Run main function
main "$@"