# Use pre-built Qt6 image as base
FROM stateoftheartio/qt6:6.8-gcc-aqt AS builder

# Switch to root for package installation
USER root

# Install newer GCC first
RUN apt-get update && \
    apt-get install -y software-properties-common && \
    add-apt-repository ppa:ubuntu-toolchain-r/test && \
    apt-get update && \
    apt-get install -y gcc-11 g++-11 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 60 --slave /usr/bin/g++ g++ /usr/bin/g++-11 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 40 --slave /usr/bin/g++ g++ /usr/bin/g++-9 && \
    gcc --version && g++ --version

# Install dependencies (removing libgeographic-dev, adding GeographicLib build dependencies)
RUN apt-get update && apt-get install -y \
    # Build tools
    build-essential \
    cmake \
    git \
    pkg-config \
    wget \
    # X11 and OpenGL for GUI support
    libx11-dev \
    libxext-dev \
    libxrandr-dev \
    libxcursor-dev \
    libxi-dev \
    libxinerama-dev \
    libxxf86vm-dev \
    libxss-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libxkbcommon-x11-0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-xinerama0 \
    libxcb-xfixes0 \
    x11-apps \
    # System libraries
    libssl-dev \
    libfontconfig1-dev \
    libfreetype6-dev \
    libpng-dev \
    libjpeg-dev \
    libtiff-dev \
    libwebp-dev \
    # CUPS for Qt6PrintSupport
    libcups2-dev \
    # PostgreSQL for GDAL
    libpq-dev \
    # GDAL build dependencies
    libproj-dev \
    libgeos-dev \
    libsqlite3-dev \
    libexpat1-dev \
    libxml2-dev \
    libxerces-c-dev \
    libnetcdf-dev \
    libhdf5-dev \
    libkml-dev \
    libspatialite-dev \
    liblzma-dev \
    libzstd-dev \
    libblosc-dev \
    libcfitsio-dev \
    # RabbitMQ-C
    librabbitmq-dev \
    # Dependencies for osgEarth
    libcurl4-openssl-dev \
    libprotobuf-dev \
    protobuf-compiler \
    libpoco-dev \
    # OpenSceneGraph
    libopenscenegraph-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Build and install GeographicLib 2.3 from source (GitHub)
RUN git clone https://github.com/geographiclib/geographiclib.git /tmp/geographiclib && \
    cd /tmp/geographiclib && \
    git checkout v2.3 && \
    mkdir build && cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DBUILD_SHARED_LIBS=ON \
        -DGEOGRAPHICLIB_TOOLS=ON \
        -DGEOGRAPHICLIB_DOCUMENTATION=OFF \
        -DGEOGRAPHICLIB_EXAMPLES=OFF \
        -DGEOGRAPHICLIB_TESTS=OFF && \
    make -j$(nproc) && \
    make install && \
    rm -rf /tmp/geographiclib

# Update library cache after GeographicLib installation
RUN ldconfig

# Build and install GDAL 3.8.0 from source (GitHub)
RUN git clone https://github.com/OSGeo/gdal.git /tmp/gdal && \
    cd /tmp/gdal && \
    git checkout v3.8.0 && \
    mkdir build && cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DBUILD_SHARED_LIBS=ON \
        -DGDAL_BUILD_OPTIONAL_DRIVERS=ON \
        -DGDAL_ENABLE_DRIVER_GIF=ON \
        -DGDAL_ENABLE_DRIVER_JPEG=ON \
        -DGDAL_ENABLE_DRIVER_PNG=ON \
        -DGDAL_ENABLE_DRIVER_GTiff=ON \
        -DGDAL_USE_GEOS=ON \
        -DGDAL_USE_PROJ=ON \
        -DGDAL_USE_CURL=ON \
        -DGDAL_USE_EXPAT=ON \
        -DGDAL_USE_SQLITE3=ON \
        -DGDAL_USE_POSTGRESQL=ON \
        -DGDAL_USE_NETCDF=ON \
        -DGDAL_USE_HDF5=ON \
        -DGDAL_USE_SPATIALITE=ON && \
    make -j$(nproc) && \
    make install && \
    rm -rf /tmp/gdal

# Update library cache after GDAL installation
RUN ldconfig

# Build and install osgEarth from source (Release and Debug)
RUN git clone --branch osgearth-3.5 https://github.com/gwaldron/osgearth.git /tmp/osgearth && \
    cd /tmp/osgearth && \
    # Build Release version
    mkdir build-release && cd build-release && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DBUILD_OSGEARTH_EXAMPLES=OFF && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    # Build Debug version
    mkdir build-debug && cd build-debug && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DBUILD_OSGEARTH_EXAMPLES=OFF && \
    make -j$(nproc) && \
    make install && \
    cd / && \
    rm -rf /tmp/osgearth

# Move everything from lib64 to lib directory
RUN mv /usr/local/lib64/* /usr/local/lib/ && \
    rmdir /usr/local/lib64 && \
    ldconfig

# Build and install osgQt
RUN git clone --branch topic/Qt6 https://github.com/AhmedAredah/osgQt.git /tmp/osgQt  && \
    cd /tmp/osgQt && \
    mkdir build && cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DQt6_DIR=/opt/Qt/6.8.0/gcc_64/lib/cmake/Qt6 \
        -DBUILD_OSG_EXAMPLES=OFF && \
    make -j$(nproc) && \
    make install && \
    rm -rf /tmp/osgQt

# Build and install KDReports
RUN git clone https://github.com/KDAB/KDReports.git /tmp/kdreports && \
    cd /tmp/kdreports && \
    mkdir build && cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DKDReports_QT6=ON \
        -DKDReports_STATIC=OFF \
        -DKDReports_TESTS=OFF \
        -DKDReports_EXAMPLES=OFF \
        -DKDReports_DOCS=OFF \
        -DKDReports_PYTHON_BINDINGS=OFF \
        -DQt6_DIR=/opt/Qt/6.8.0/gcc_64/lib/cmake/Qt6 && \
    make -j$(nproc) && \
    make install && \
    rm -rf /tmp/kdreports

# Build and install Container library
RUN git clone https://github.com/AhmedAredah/container.git /tmp/container && \
cd /tmp/container && \
mkdir build && cd build && \
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DQt6_DIR=/opt/Qt/6.8.0/gcc_64/lib/cmake/Qt6 \
    -DBUILD_PYTHON_BINDINGS=OFF && \
make -j$(nproc) && \
make install && \
rm -rf /tmp/container

# Build and install rabbitmq-c from source with CMake support
RUN git clone https://github.com/alanxz/rabbitmq-c.git /tmp/rabbitmq-c && \
    cd /tmp/rabbitmq-c && \
    mkdir build && cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_TESTS=OFF \
        -DBUILD_TOOLS=OFF && \
    make -j$(nproc) && \
    make install && \
    rm -rf /tmp/rabbitmq-c

# Set Qt6 environment variables
ENV Qt6_DIR=/opt/Qt/6.8.0/gcc_64/lib/cmake/Qt6
ENV PATH=/opt/Qt/6.8.0/gcc_64/bin:$PATH
ENV LD_LIBRARY_PATH=/opt/Qt/6.8.0/gcc_64/lib:/usr/local/lib:/usr/lib/x86_64-linux-gnu

# Clone the ShipNetSim repository
ARG GITHUB_BRANCH=main
RUN git clone --branch ${GITHUB_BRANCH} https://github.com/VTTI-CSM/ShipNetSim.git /app

# Build the project
WORKDIR /app

# Fix C++ standard, GDAL CONFIG mode calls, GeographicLib target, OSG targets, bit_cast, and add GeographicLib CMake module path
RUN sed -i 's/set(CMAKE_CXX_STANDARD 23)/set(CMAKE_CXX_STANDARD 23)/' /app/CMakeLists.txt && \
    find /app -name "CMakeLists.txt" -exec sed -i 's/find_package(GDAL.*)/find_package(GDAL REQUIRED MODULE)/g' {} \; && \
    find /app -name "CMakeLists.txt" -exec sed -i 's/osg3::[^ ]*/\${OPENSCENEGRAPH_LIBRARIES}/g' {} \; && \
    sed -i '/^project(/a list(APPEND CMAKE_MODULE_PATH /usr/share/cmake/geographiclib)' /app/CMakeLists.txt

RUN mkdir -p build && cd build && \
    CMAKE_PREFIX_PATH=/usr/local \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_FIND_DEBUG_MODE=OFF \
        -DBUILD_GUI=ON \
        -DBUILD_SERVER=ON \
        -DBUILD_INSTALLER=OFF \
        -DQt6_DIR=/opt/Qt/6.8.0/gcc_64/lib/cmake/Qt6 \
        -DRabbitMQ-C_DIR=/usr/lib/x86_64-linux-gnu/cmake/rabbitmq-c \
        -DOSGQOPENGL_LIB=/usr/local/lib/libosgQOpenGL.a && \
    cat CMakeCache.txt && \
    make -j$(nproc)

# Create a user for running applications
RUN useradd -m -s /bin/bash shipnetsim

# Runtime stage
FROM ubuntu:20.04 AS release_stage

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    # OpenGL and X11
    libgl1-mesa-glx \
    libglu1-mesa \
    libx11-6 \
    libxext6 \
    libxrender1 \
    libxcb1 \
    libxkbcommon-x11-0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-xinerama0 \
    libxcb-xfixes0 \
    # Qt6 runtime dependencies
    libdbus-1-3 \
    libxcb-xkb1 \
    libxkbcommon0 \
    libxcb-render0 \
    libxcb-shape0 \
    libxcb-shm0 \
    libxcb-sync1 \
    libxcb-cursor0 \
    libxcb-glx0 \
    libegl1-mesa \
    libnss3 \
    libnspr4 \
    libatk-bridge2.0-0 \
    libdrm2 \
    libgtk-3-0 \
    # OpenSceneGraph runtime (Ubuntu 20.04 versions)
    libopenscenegraph-dev \
    # GDAL and GeographicLib dependencies (Ubuntu 20.04 versions)
    libproj15 \
    libgeos-c1v5 \
    libsqlite3-0 \
    libexpat1 \
    libxml2 \
    libxerces-c3.2 \
    libnetcdf15 \
    libhdf5-103 \
    libspatialite7 \
    liblzma5 \
    libzstd1 \
    libblosc1 \
    libcfitsio8 \
    libcurl4 \
    libprotobuf17 \
    libpoco-dev \
    zlib1g \
    # Other runtime dependencies
    libfontconfig1 \
    libfreetype6 \
    && rm -rf /var/lib/apt/lists/*

# Copy built binaries and libraries from builder
COPY --from=builder /app/build/src/ShipNetSim/ShipNetSim /app/
COPY --from=builder /app/build/src/ShipNetSimGUI/ShipNetSimGUI /app/
COPY --from=builder /app/build/src/ShipNetSimServer/ShipNetSimServer /app/
COPY --from=builder /app/build/src/ShipNetSimCore/libShipNetSimCore.so* /usr/local/lib/

# Copy all custom-built libraries with broader patterns to catch all versions
COPY --from=builder /usr/local/lib/libosgEarth*.so* /usr/local/lib/
COPY --from=builder /usr/local/lib/libKDReports*.so* /usr/local/lib/
COPY --from=builder /usr/local/lib/libgdal*.so* /usr/local/lib/
COPY --from=builder /usr/local/lib/libgeographic*.so* /usr/local/lib/
COPY --from=builder /usr/local/lib/*GeographicLib*.so* /usr/local/lib/
COPY --from=builder /usr/local/lib/*geographic*.so* /usr/local/lib/
COPY --from=builder /usr/local/lib/libosgQt*.so* /usr/local/lib/
COPY --from=builder /usr/local/lib/librabbitmq*.so* /usr/local/lib/

# Copy Container library with all possible variations
COPY --from=builder /usr/local/lib/libcontainer*.so* /usr/local/lib/
COPY --from=builder /usr/local/lib/libContainer*.so* /usr/local/lib/

# Copy Qt6 libraries and dependencies from builder
COPY --from=builder /opt/Qt/6.8.0/gcc_64/lib/libQt6*.so* /usr/local/lib/

# Copy ICU libraries from builder (to match Qt6's ICU version)
COPY --from=builder /opt/Qt/6.8.0/gcc_64/lib/libicu*.so* /usr/local/lib/

# Copy newer libstdc++ and libgcc from builder to fix GLIBCXX version issues
COPY --from=builder /usr/lib/x86_64-linux-gnu/libstdc++.so.6* /usr/local/lib/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libgcc_s.so.1* /usr/local/lib/

# Create Qt6 directory structure and copy plugins
RUN mkdir -p /usr/local/lib/qt6
COPY --from=builder /opt/Qt/6.8.0/gcc_64/plugins /usr/local/lib/qt6/plugins

# Copy data directory
COPY --from=builder /app/src/data /app/data

# Set comprehensive library paths and Qt environment variables
ENV LD_LIBRARY_PATH=/usr/local/lib:/usr/lib/x86_64-linux-gnu
ENV QT_PLUGIN_PATH=/usr/local/lib/qt6/plugins
ENV QT_QPA_PLATFORM_PLUGIN_PATH=/usr/local/lib/qt6/plugins/platforms
ENV QT_QPA_PLATFORM=xcb

# Update library cache
RUN ldconfig

# Create non-root user
RUN useradd -m -s /bin/bash shipnetsim
USER shipnetsim

WORKDIR /app
CMD ["./ShipNetSimServer"]