@echo off
REM Run ShipNetSim GUI on Windows with X11 server

echo 🚢 Starting ShipNetSim GUI on Windows...

REM Check if Docker is available
docker --version >nul 2>&1
if errorlevel 1 (
    echo ❌ Docker is not installed or not in PATH
    echo    Install Docker Desktop for Windows: https://docs.docker.com/docker-for-windows/install/
    pause
    exit /b 1
)

REM Check if Docker is running
docker info >nul 2>&1
if errorlevel 1 (
    echo ❌ Docker is not running
    echo    Please start Docker Desktop
    pause
    exit /b 1
)

REM Check if image exists
docker image inspect shipnetsim:latest >nul 2>&1
if errorlevel 1 (
    echo ❌ Docker image 'shipnetsim:latest' not found
    echo    Build it first with: docker build -t shipnetsim:latest .
    pause
    exit /b 1
)

echo 🪟 Detected Windows - checking for X11 server...

REM Try to detect X11 server
netstat -an | findstr ":6000" >nul 2>&1
if errorlevel 1 (
    echo ❌ No X11 server detected on port 6000
    echo.
    echo 📋 To run GUI applications, you need an X11 server:
    echo.
    echo 🎯 Option 1: VcXsrv ^(Recommended^)
    echo    1. Download and install VcXsrv: https://sourceforge.net/projects/vcxsrv/
    echo    2. Start XLaunch with these settings:
    echo       - Display: Multiple windows
    echo       - Client startup: Start no client
    echo       - Extra settings: ✅ Disable access control
    echo.
    echo 🎯 Option 2: X410 ^(Microsoft Store^)
    echo    1. Install X410 from Microsoft Store
    echo    2. Start X410
    echo.
    echo 🎯 Option 3: Use WSL2 ^(Recommended for development^)
    echo    1. Install WSL2: wsl --install
    echo    2. Run this script from within WSL2
    echo.
    pause
    exit /b 1
)

echo ✅ X11 server detected

REM Create directories if they don't exist
if not exist "%cd%\data" mkdir "%cd%\data"
if not exist "%cd%\output" mkdir "%cd%\output"

echo 🐳 Starting Docker container...

docker run --rm -it ^
    -e DISPLAY=host.docker.internal:0 ^
    -e QT_X11_NO_MITSHM=1 ^
    -e LIBGL_ALWAYS_INDIRECT=1 ^
    -v "%cd%\data:/app/data" ^
    -v "%cd%\output:/app/output" ^
    --add-host=host.docker.internal:host-gateway ^
    --name shipnetsim-gui ^
    shipnetsim:latest ^
    ./ShipNetSimGUI

echo ✅ ShipNetSim GUI container finished
pause