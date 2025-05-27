# Use pre-built Qt6 image as base for both building and runtime
FROM stateoftheartio/qt6:6.8-gcc-aqt AS release

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

# Install all dependencies (build + runtime)
RUN apt-get update && apt-get install -y \
    # Build tools
    build-essential \
    cmake \
    git \
    pkg-config \
    wget \
    curl \
    xvfb \
    x11-utils \
    # X11 and OpenGL for GUI support
    libx11-dev \
    libx11-6 \
    libxext-dev \
    libxext6 \
    libxrandr-dev \
    libxcursor-dev \
    libxi-dev \
    libxinerama-dev \
    libxxf86vm-dev \
    libxss-dev \
    libgl1-mesa-dev \
    libgl1-mesa-glx \
    libglu1-mesa-dev \
    libglu1-mesa \
    libxkbcommon-x11-0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-xinerama0 \
    libxcb-xfixes0 \
    libxcb-cursor0 \
    libxcb-cursor-dev \
    libxcb-glx0 \
    libxcb-render0 \
    libxcb-shape0 \
    libxcb-shm0 \
    libxcb-sync1 \
    libxcb1 \
    libxrender1 \
    x11-apps \
    mesa-utils \
    # Qt6 runtime dependencies
    libdbus-1-3 \
    libxcb-xkb1 \
    libxkbcommon0 \
    libegl1-mesa \
    libnss3 \
    libnspr4 \
    libatk-bridge2.0-0 \
    libdrm2 \
    libgtk-3-0 \
    # System libraries
    libssl-dev \
    libfontconfig1-dev \
    libfontconfig1 \
    libfreetype6-dev \
    libfreetype6 \
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
    libproj15 \
    libgeos-dev \
    libgeos-c1v5 \
    libsqlite3-dev \
    libsqlite3-0 \
    libexpat1-dev \
    libexpat1 \
    libxml2-dev \
    libxml2 \
    libxerces-c-dev \
    libxerces-c3.2 \
    libnetcdf-dev \
    libnetcdf15 \
    libhdf5-dev \
    libhdf5-103 \
    libkml-dev \
    libspatialite-dev \
    libspatialite7 \
    liblzma-dev \
    liblzma5 \
    libzstd-dev \
    libzstd1 \
    libblosc-dev \
    libblosc1 \
    libcfitsio-dev \
    libcfitsio8 \
    # RabbitMQ-C
    librabbitmq-dev \
    # Dependencies for osgEarth
    libcurl4-openssl-dev \
    libcurl4 \
    libprotobuf-dev \
    libprotobuf17 \
    protobuf-compiler \
    libpoco-dev \
    # OpenSceneGraph
    libopenscenegraph-dev \
    zlib1g-dev \
    zlib1g \
    # Software rendering support
    libosmesa6 \
    # Locale support
    locales \
    && rm -rf /var/lib/apt/lists/*

# Configure locale to fix std::locale errors
RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US:en
ENV LC_ALL=en_US.UTF-8

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
RUN if [ -d /usr/local/lib64 ]; then mv /usr/local/lib64/* /usr/local/lib/ && rmdir /usr/local/lib64; fi && \
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

# Install Git LFS
RUN curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | bash && \
    apt-get install -y git-lfs && \
    git lfs install

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

# Create a clean app directory with only executables, libraries and data
RUN mkdir -p /app_clean && \
    # Copy executables to app root
    cp /app/build/src/ShipNetSim/ShipNetSim /app_clean/ && \
    cp /app/build/src/ShipNetSimGUI/ShipNetSimGUI /app_clean/ && \
    cp /app/build/src/ShipNetSimServer/ShipNetSimServer /app_clean/ && \
    # Copy all shared libraries to /usr/local/lib (they'll be found via LD_LIBRARY_PATH)
    find /app/build -name "*.so*" -exec cp {} /usr/local/lib/ \; && \
    # Move data directory
    if [ -d /app/src/data ]; then mv /app/src/data /app_clean/data; fi && \
    # Replace old app directory with clean one
    rm -rf /app && \
    mv /app_clean /app

# Clean up build dependencies and temporary files to reduce image size
RUN apt-get autoremove -y \
        build-essential \
        cmake \
        git \
        pkg-config \
        wget \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* \
    && rm -rf /build /tmp/*

# Set Qt6 environment variables
ENV Qt6_DIR=/opt/Qt/6.8.0/gcc_64/lib/cmake/Qt6
ENV PATH=/opt/Qt/6.8.0/gcc_64/bin:$PATH
ENV LD_LIBRARY_PATH=/opt/Qt/6.8.0/gcc_64/lib:/usr/local/lib:/usr/lib/x86_64-linux-gnu
ENV QT_PLUGIN_PATH=/opt/Qt/6.8.0/gcc_64/plugins
ENV QT_QPA_PLATFORM_PLUGIN_PATH=/opt/Qt/6.8.0/gcc_64/plugins/platforms
ENV QT_QPA_PLATFORM=xcb

# Update library cache
RUN ldconfig

# Create non-root user
RUN useradd -m -s /bin/bash shipnetsim

# Change ownership of the app directory to the shipnetsim user
RUN chown -R shipnetsim:shipnetsim /app

# Create startup script for GUI applications
RUN echo '#!/bin/bash' > /usr/local/bin/start-gui.sh && \
    echo '# Start Xvfb virtual display' >> /usr/local/bin/start-gui.sh && \
    echo 'Xvfb :99 -screen 0 1024x768x24 > /dev/null 2>&1 &' >> /usr/local/bin/start-gui.sh && \
    echo 'sleep 2' >> /usr/local/bin/start-gui.sh && \
    echo '' >> /usr/local/bin/start-gui.sh && \
    echo '# Set environment for GUI applications' >> /usr/local/bin/start-gui.sh && \
    echo 'export DISPLAY=:99' >> /usr/local/bin/start-gui.sh && \
    echo 'export LIBGL_ALWAYS_SOFTWARE=1' >> /usr/local/bin/start-gui.sh && \
    echo 'export OSMESA_LIBRARY=/usr/lib/x86_64-linux-gnu/libOSMesa.so.8' >> /usr/local/bin/start-gui.sh && \
    echo 'export GALLIUM_DRIVER=llvmpipe' >> /usr/local/bin/start-gui.sh && \
    echo 'export QT_QPA_PLATFORM=xcb' >> /usr/local/bin/start-gui.sh && \
    echo 'export QT_X11_NO_MITSHM=1' >> /usr/local/bin/start-gui.sh && \
    echo 'export LC_ALL=C.UTF-8' >> /usr/local/bin/start-gui.sh && \
    echo 'export LANG=C.UTF-8' >> /usr/local/bin/start-gui.sh && \
    echo '' >> /usr/local/bin/start-gui.sh && \
    echo '# Execute the GUI application' >> /usr/local/bin/start-gui.sh && \
    echo 'exec "$@"' >> /usr/local/bin/start-gui.sh

RUN chmod +x /usr/local/bin/start-gui.sh

# Set Qt6 environment variables for shipnetsim user
USER shipnetsim
ENV Qt6_DIR=/opt/Qt/6.8.0/gcc_64/lib/cmake/Qt6
ENV PATH=/opt/Qt/6.8.0/gcc_64/bin:$PATH
ENV LD_LIBRARY_PATH=/opt/Qt/6.8.0/gcc_64/lib:/usr/local/lib:/usr/lib/x86_64-linux-gnu
ENV QT_PLUGIN_PATH=/opt/Qt/6.8.0/gcc_64/plugins
ENV QT_QPA_PLATFORM_PLUGIN_PATH=/opt/Qt/6.8.0/gcc_64/plugins/platforms
ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

# Set default environment variables for headless rendering
ENV LIBGL_ALWAYS_SOFTWARE=1
ENV OSMESA_LIBRARY=/usr/lib/x86_64-linux-gnu/libOSMesa.so.8
ENV GALLIUM_DRIVER=llvmpipe
ENV QT_QPA_PLATFORM=xcb
ENV QT_X11_NO_MITSHM=1

WORKDIR /app
CMD ["./ShipNetSimServer"]