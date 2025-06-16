# How to Build ShipNetSim

This document provides comprehensive instructions for building ShipNetSim from source code. ShipNetSim is a Qt6-based maritime simulation software with modular components including GUI, server, and core simulation engine.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Dependencies](#dependencies)
- [Build Configuration Options](#build-configuration-options)
- [Platform-Specific Instructions](#platform-specific-instructions)
- [Building the Project](#building-the-project)
- [Installation](#installation)
- [Troubleshooting](#troubleshooting)

## Prerequisites

### Required Software

- **CMake**: Version 3.24 or higher
- **C++ Compiler**: 
 - Windows: MSVC 2019/2022 (Visual Studio Build Tools)
 - Linux: GCC or Clang with C++23 support
 - macOS: Xcode with C++23 support
- **Qt6**: Version 6.4.2 or higher
- **Git**: For cloning the repository

### System Requirements

- **RAM**: Minimum 8GB, recommended 16GB+
- **Storage**: At least 10GB free space for build artifacts
- **Graphics**: OpenGL-compatible graphics card (for GUI components)

## Dependencies

ShipNetSim requires several third-party libraries. The build system expects these to be installed in standard locations or specified via CMake variables.

### Core Dependencies (Required for All Builds)

1. **Qt6 Components**:
  - Qt6::Core
  - Qt6::Concurrent
  - Qt6::Xml
  - Qt6::Sql
  - Qt6::Network

2. **GDAL** (Geospatial Data Abstraction Library)
  - Windows: Download from [OSGeo4W](https://trac.osgeo.org/osgeo4w/) or [GISInternals](https://www.gisinternals.com/)
  - Linux: `sudo apt-get install libgdal-dev` (Ubuntu/Debian) or `yum install gdal-devel` (CentOS/RHEL)
  - macOS: `brew install gdal`

3. **GeographicLib**
  - Windows: Download from [GeographicLib website](https://geographiclib.sourceforge.io/)
  - Linux: `sudo apt-get install libgeographiclib-dev`
  - macOS: `brew install geographiclib`

### GUI-Specific Dependencies (Required if BUILD_GUI=ON)

4. **OpenSceneGraph (OSG)**
  - Windows: Download precompiled binaries or build from source
  - Linux: `sudo apt-get install libopenscenegraph-dev`
  - macOS: `brew install open-scene-graph`

5. **osgEarth**
  - Download from [osgEarth website](http://osgearth.org/)
  - Must be compatible with your OpenSceneGraph version

6. **osgQt (osgQOpenGL)**
  - Often bundled with osgEarth or available separately

7. **KDReports**
  - Download from [KDAB website](https://www.kdab.com/development-resources/qt-tools/kd-reports/)

8. **Additional Qt6 Components**:
  - Qt6::Widgets
  - Qt6::OpenGLWidgets
  - Qt6::PrintSupport

### Server-Specific Dependencies (Required if BUILD_SERVER=ON)

9. **Container Library**
  - Custom library (path must be specified via CONTAINER_SEARCH_PATHS)

10. **RabbitMQ-C**
   - Windows: Download from [RabbitMQ-C releases](https://github.com/alanxz/rabbitmq-c/releases)
   - Linux: `sudo apt-get install librabbitmq-dev`
   - macOS: `brew install rabbitmq-c`

### Optional Dependencies

11. **Fontconfig** (Linux/macOS only)
   - Linux: Usually pre-installed, or `sudo apt-get install libfontconfig1-dev`
   - macOS: `brew install fontconfig`

## Build Configuration Options

ShipNetSim provides several CMake options to customize the build:

### Primary Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_GUI` | `ON` | Build the GUI components (ShipNetSimGUI) |
| `BUILD_SERVER` | `ON` | Build the server components (ShipNetSimServer) |
| `BUILD_INSTALLER` | `ON` | Build the installer package |

### Path Configuration Options

| Variable | Default (Windows) | Description |
|----------|-------------------|-------------|
| `QT_BIN_DIR` | `C:/Qt/6.4.2/msvc2019_64/bin` | Path to Qt binary directory |
| `osgEarth_DIR` | `C:/Program Files/OSGEarth/cmake` | Path to osgEarth CMake directory |
| `OpenSceneGraph_DIR` | `C:/Program Files/OpenSceneGraph` | Path to OpenSceneGraph directory |
| `OSGQT_INCLUDE_DIR` | `C:/Program Files/osgQt/include` | Path to osgQt include directory |
| `OSGQOPENGL_LIB` | `C:/Program Files/osgQt/lib/osg145-osgQOpenGL.lib` | Path to osgQt library |
| `KDREPORTS_DIR` | `C:/KDAB/KDReports-2.3.95/lib/cmake/KDReports-qt6` | Path to KDReports CMake directory |
| `CONTAINER_SEARCH_PATHS` | Platform-specific | Path to Container library |
| `RABBITMQ_CMAKE_DIR` | Platform-specific | Path to RabbitMQ-C CMake directory |
| `GDAL_ROOT_HINTS` | Empty | Hint path for GDAL installation |
| `GeographicLib_ROOT_HINTS` | Empty | Hint path for GeographicLib installation |

### Compiler Options

The build system automatically configures compiler-specific options:

- **MSVC**: `/W4`, `/MP`, optimization flags (`/Od /Zi` for Debug, `/O2` for Release)
- **GCC/Clang**: `-Wall`, optimization flags (`-O0 -g` for Debug, `-O3` for Release)

## Platform-Specific Instructions

### Windows

1. **Install Visual Studio Build Tools**:
  ```cmd
  # Download Visual Studio Installer and install:
  # - MSVC v143 compiler toolset
  # - Windows 10/11 SDK
  # - CMake tools for Visual Studio
  ```

2. **Install Qt6**:
  ```cmd
  # Download Qt Online Installer
  # Install Qt 6.4.2 or later with MSVC 2019/2022 kit
  ```

3. **Set Environment Variables** (Optional):
  ```cmd
  set Qt6_DIR=C:\Qt\6.4.2\msvc2019_64\lib\cmake\Qt6
  set PATH=%PATH%;C:\Qt\6.4.2\msvc2019_64\bin
  ```

### Linux (Ubuntu/Debian)

1. **Install Build Tools**:
  ```bash
  sudo apt-get update
  sudo apt-get install build-essential cmake git
  ```

2. **Install Qt6**:
  ```bash
  sudo apt-get install qt6-base-dev qt6-tools-dev qt6-tools-dev-tools
  sudo apt-get install libqt6opengl6-dev libqt6openglwidgets6
  ```

3. **Install Dependencies**:
  ```bash
  sudo apt-get install libgdal-dev libgeographiclib-dev
  sudo apt-get install libopenscenegraph-dev libfontconfig1-dev
  sudo apt-get install librabbitmq-dev
  ```

### macOS

1. **Install Xcode Command Line Tools**:
  ```bash
  xcode-select --install
  ```

2. **Install Homebrew** (if not already installed):
  ```bash
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  ```

3. **Install Dependencies**:
  ```bash
  brew install cmake qt6 gdal geographiclib
  brew install open-scene-graph fontconfig rabbitmq-c
  ```

## Building the Project

### 1. Clone the Repository

```bash
git clone https://github.com/VTTI-CSM/ShipNetSim.git
cd ShipNetSim
```

### 2. Create Build Directory

```bash
mkdir build
cd build
```

### 3. Configure with CMake

#### Basic Configuration (All Components)

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

#### Custom Configuration Examples

**GUI Only Build**:
```bash
cmake .. \
   -DCMAKE_BUILD_TYPE=Release \
   -DBUILD_GUI=ON \
   -DBUILD_SERVER=OFF \
   -DBUILD_INSTALLER=OFF
```

**Server Only Build**:
```bash
cmake .. \
   -DCMAKE_BUILD_TYPE=Release \
   -DBUILD_GUI=OFF \
   -DBUILD_SERVER=ON \
   -DBUILD_INSTALLER=OFF
```

**Custom Paths (Windows Example)**:
```bash
cmake .. \
   -DCMAKE_BUILD_TYPE=Release \
   -DQT_BIN_DIR="C:/Qt/6.5.0/msvc2022_64/bin" \
   -DosgEarth_DIR="C:/OSGEarth/cmake" \
   -DOpenSceneGraph_DIR="C:/OpenSceneGraph" \
   -DGDAL_ROOT_HINTS="C:/OSGeo4W64"
```

### 4. Build the Project

```bash
# Build all targets
cmake --build . --config Release

# Or build specific targets
cmake --build . --target ShipNetSimCore --config Release
cmake --build . --target ShipNetSimGUI --config Release
cmake --build . --target ShipNetSimServer --config Release
```

#### Parallel Building

```bash
# Use multiple cores (replace 8 with your core count)
cmake --build . --config Release --parallel 8
```

### 5. Build Installer (Optional)

```bash
cmake --build . --target ShipNetSimInstaller --config Release
```

## Installation

### Install to System

```bash
# Configure with install prefix
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local

# Build and install
cmake --build . --config Release
cmake --install . --config Release
```

### Package Creation

```bash
# Create installer package (Windows)
cmake --build . --target package --config Release

# This creates a .exe installer in the build directory
```

## Troubleshooting

### Common Issues

1. **Qt6 Not Found**:
  ```bash
  # Set Qt6_DIR explicitly
  cmake .. -DQt6_DIR="C:/Qt/6.4.2/msvc2019_64/lib/cmake/Qt6"
  ```

2. **GDAL Not Found**:
  ```bash
  # Specify GDAL path
  cmake .. -DGDAL_ROOT_HINTS="C:/OSGeo4W64"
  ```

3. **OpenSceneGraph Issues**:
  ```bash
  # Ensure all OSG components are found
  cmake .. -DOpenSceneGraph_DIR="C:/OpenSceneGraph"
  ```

4. **Missing DLLs (Windows)**:
  - Ensure all dependency DLLs are in PATH or copied to output directory
  - The installer automatically handles DLL deployment

5. **Permission Issues**:
  - Run CMake and build commands with appropriate permissions
  - On Windows, consider running as Administrator for system-wide installs

### Build Output Structure

After successful build, the output structure will be:
```
build/
├── bin/                    # Executables and DLLs
│   ├── ShipNetSim.exe     # Core executable
│   ├── ShipNetSimGUI.exe  # GUI application (if BUILD_GUI=ON)
│   ├── ShipNetSimServer.exe # Server application (if BUILD_SERVER=ON)
│   └── ShipNetSimCore.dll # Core library
├── lib/                   # Static libraries
├── data/                  # Application data files
└── plugins/               # Qt plugins (Windows)
```

### Environment Variables

For development, you may want to set:

```bash
# Windows
set PATH=%PATH%;C:\Qt\6.4.2\msvc2019_64\bin
set QT_PLUGIN_PATH=C:\Qt\6.4.2\msvc2019_64\plugins

# Linux/macOS
export PATH=$PATH:/path/to/qt6/bin
export QT_PLUGIN_PATH=/path/to/qt6/plugins
```

### Getting Help

If you encounter build issues:

1. Check the [GitHub Issues](https://github.com/VTTI-CSM/ShipNetSim/issues) page
2. Verify all dependencies are correctly installed
3. Ensure CMake version compatibility (3.24+)
4. Contact the maintainers: [AhmedAredah@vt.edu](mailto:AhmedAredah@vt.edu)

### Contributing

Before submitting pull requests:

1. Ensure your changes build successfully on your target platform
2. Follow the project's coding standards
3. Update documentation as needed

For detailed contribution guidelines, see [CONTRIBUTING.md](CONTRIBUTING.md).