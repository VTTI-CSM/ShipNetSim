# min required cmake version
cmake_minimum_required(VERSION 3.24)

# Add cmake directory to module path for custom modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(InstallRequiredSystemLibraries)

# Enable automatic runtime dependency detection
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

# Define the project name (ShipNetSim) and
# the programming language used (CXX for C++)
set(ShipNetSim_VERSION "0.0.3" CACHE STRING "Project version" FORCE)
set(SHIPNETSIM_NAME "ShipNetSim" CACHE STRING "Project name" FORCE)
set(SHIPNETSIM_VENDOR "Virginia Tech Transportation Institute - Center for Sustainable Mobility." CACHE STRING "Project vendor" FORCE)
# Get the current date and time
string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M:%S")
# Set the BUILD_DATE variable
set(BUILD_DATE ${BUILD_DATE} CACHE STRING "Project build time" FORCE)

# Extract major, minor, and patch version from ShipNetSim_VERSION
string(REPLACE "." ";" VERSION_LIST ${ShipNetSim_VERSION})
list(GET VERSION_LIST 0 ShipNetSim_VERSION_MAJOR)
list(GET VERSION_LIST 1 ShipNetSim_VERSION_MINOR)
list(GET VERSION_LIST 2 ShipNetSim_VERSION_PATCH)

project(${SHIPNETSIM_NAME} VERSION ${ShipNetSim_VERSION} LANGUAGES CXX)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Compiler settings
# Set the C++20 standard to be used for compiling
set(CMAKE_CXX_STANDARD 23)
# Ensure that the selected C++ standard is a
# requirement for the compiler
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Enable Qt's Automatic User Interface Compiler (UIC)
set(CMAKE_AUTOUIC ON)
# Enable Qt's Meta-Object Compiler (MOC) which allows
# the use of Qt features such as signals and slots
set(CMAKE_AUTOMOC ON)
# Enable Qt's Resource Compiler (RCC) for compiling
# resource files into binary format
set(CMAKE_AUTORCC ON)


# Platform-specific compiler flags
# Using generator expressions for proper multi-config generator support (Visual Studio, Xcode, Ninja Multi-Config)
# See: https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html
if(MSVC)
    # MSVC-specific flags
    add_compile_options(/W4 /MP)
    # Configuration-specific flags using generator expressions (each flag must be separate)
    add_compile_options(
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Debug>:/Zi>
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/Zi>
        $<$<CONFIG:MinSizeRel>:/O1>
    )
else()
    # GCC/Clang-specific flags
    add_compile_options(-Wall -Wextra -Wpedantic)
    # Configuration-specific flags using generator expressions (each flag must be separate)
    add_compile_options(
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:RelWithDebInfo>:-O2>
        $<$<CONFIG:RelWithDebInfo>:-g>
        $<$<CONFIG:MinSizeRel>:-Os>
    )
endif()

# Add definitions based on build type using generator expressions
# Disable Qt debug output for Release and MinSizeRel builds
add_compile_definitions(
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
    $<$<CONFIG:MinSizeRel>:QT_NO_DEBUG_OUTPUT>
)

# -------------------------------------------------------
# --------------------- Library Search Paths ------------
# -------------------------------------------------------
# Include and setup library search paths based on build type and platform.
# This module configures CMAKE_PREFIX_PATH and individual library paths to support:
#   - Build-type-specific installations (/usr/local/Debug/, /usr/local/Release/, etc.)
#   - Default system installations (/usr/local/, /usr/, Homebrew on macOS)
#   - Custom Windows installation paths
include(LibrarySearchPaths)
setup_library_search_paths()

# -------------------------------------------------------
# --------------------- OPTIONS -------------------------
# -------------------------------------------------------

# Option to build GUI components
option(BUILD_GUI "Build the GUI components" ON)
# Cache the option so it's stored between runs
set(BUILD_GUI ${BUILD_GUI} CACHE BOOL "Build the GUI components" FORCE)

# Option to build Server components
option(BUILD_SERVER "Build the SERVER components" ON)
# Cache the option so it's stored between runs
set(BUILD_SERVER ${BUILD_SERVER} CACHE BOOL "Build the SERVER components" FORCE)

option(BUILD_INSTALLER "Build the installer" ON)
set(BUILD_INSTALLER ${BUILD_INSTALLER} CACHE BOOL "Build the INSTALLER components" FORCE)

# -------------------------------------------------------
# --------------------- Libraries -----------------------
# --------------- Define Default Paths ------------------
# -------------------------------------------------------

# Allow the user to specify the Qt binary directory
set(QT_BIN_DIR "C:/Qt/6.4.2/msvc2019_64/bin" CACHE PATH "Path to the Qt binary directory")

if(BUILD_GUI)
    # -------------------------------------------------------------------------
    # OpenSceneGraph - MUST be found first before osgEarth (osgEarth depends on it)
    # Paths configured by LibrarySearchPaths module
    # Use MODULE mode (FindOpenSceneGraph) with hint variables for build-type-specific paths
    # -------------------------------------------------------------------------
    if(OpenSceneGraph_DIR)
        message(STATUS "Using OpenSceneGraph_DIR: ${OpenSceneGraph_DIR}")
        # Get the installation prefix from the cmake directory (go up 3 levels: lib/cmake/OpenSceneGraph -> prefix)
        get_filename_component(OSG_PREFIX "${OpenSceneGraph_DIR}/../../.." ABSOLUTE)
        # Set hint variables for CMake's FindOpenSceneGraph module
        set(OSG_DIR "${OSG_PREFIX}" CACHE PATH "OpenSceneGraph directory")
        set(OSGDIR "${OSG_PREFIX}" CACHE PATH "OpenSceneGraph directory")
        set(OSG_ROOT "${OSG_PREFIX}" CACHE PATH "OpenSceneGraph root")
        set(OpenSceneGraph_ROOT "${OSG_PREFIX}" CACHE PATH "OpenSceneGraph root")
        set(ENV{OSG_DIR} "${OSG_PREFIX}")
        set(ENV{OSGDIR} "${OSG_PREFIX}")
        set(ENV{OSG_ROOT} "${OSG_PREFIX}")
        list(PREPEND CMAKE_PREFIX_PATH "${OSG_PREFIX}")
    endif()
    find_package(OpenSceneGraph REQUIRED COMPONENTS osg osgDB osgManipulator osgGA osgUtil osgViewer osgSim osgShadow osgText)

    # -------------------------------------------------------------------------
    # osgEarth - Paths configured by LibrarySearchPaths module
    # Note: OpenSceneGraph must be found first as osgEarth depends on it
    # -------------------------------------------------------------------------
    if(osgEarth_DIR)
        message(STATUS "Using osgEarth_DIR: ${osgEarth_DIR}")
        find_package(osgEarth CONFIG REQUIRED PATHS ${osgEarth_DIR} NO_DEFAULT_PATH)
    else()
        find_package(osgEarth CONFIG REQUIRED)
    endif()

    # -------------------------------------------------------------------------
    # osgQt - Paths configured by LibrarySearchPaths module
    # Automatically handles build-type-specific library suffixes (d, rd, s)
    # -------------------------------------------------------------------------
    if(osgQt_DIR)
        message(STATUS "Using osgQt_DIR: ${osgQt_DIR}")
        find_package(osgQt CONFIG REQUIRED PATHS ${osgQt_DIR} NO_DEFAULT_PATH)
    else()
        find_package(osgQt CONFIG REQUIRED)
    endif()

    # -------------------------------------------------------------------------
    # KDReports paths (KDREPORTS_DIR is set by LibrarySearchPaths module)
    # Only set default if not already configured
    # -------------------------------------------------------------------------
    if(NOT KDREPORTS_DIR)
        if(WIN32)
            set(KDREPORTS_DIR "C:/KDAB/KDReports-2.3.95/lib/cmake/KDReports-qt6" CACHE PATH "Path to KDReports CMake directory")
        elseif(APPLE)
            set(KDREPORTS_DIR "/usr/local/KDAB/KDReports-2.3.95/lib/cmake/KDReports-qt6" CACHE PATH "Path to KDReports CMake directory")
        elseif(UNIX)
            set(KDREPORTS_DIR "/usr/local/KDAB/KDReports-2.3.95/lib/cmake/KDReports-qt6" CACHE PATH "Path to KDReports CMake directory")
        endif()
    endif()

    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib" ".so" ".dylib")

    # Use the KDReports CMake package
    find_package(KDReports-qt6 REQUIRED CONFIG PATHS ${KDREPORTS_DIR})

    # Check if osgEarth was found
    if(NOT osgEarth_FOUND)
        message(FATAL_ERROR "osgEarth not found. Please specify the correct path using -DosgEarth_DIR=<path>. "
            "For build-type-specific installation, ensure osgEarth is installed at /usr/local/${CMAKE_BUILD_TYPE}/")
    endif()

    # Check if OpenSceneGraph was found
    if(NOT OpenSceneGraph_FOUND)
        message(FATAL_ERROR "OpenSceneGraph not found. Please specify the correct path using -DOpenSceneGraph_DIR=<path>. "
            "For build-type-specific installation, ensure OpenSceneGraph is installed at /usr/local/${CMAKE_BUILD_TYPE}/")
    endif()

    # Verify osgQt was found
    if(NOT osgQt_FOUND)
        message(FATAL_ERROR "osgQt not found. Please specify the correct path using -DosgQt_DIR=<path>. "
            "For build-type-specific installation, ensure osgQt is installed at /usr/local/${CMAKE_BUILD_TYPE}/")
    endif()

    if(NOT KDReports-qt6_FOUND)
        message(FATAL_ERROR "KDReports-qt6 not found. Please specify the correct path using KDREPORTS_DIR.")
    endif()

    # osgQt provides the osgQt::osgQOpenGL target - no need for manual include/library paths
    message(STATUS "osgQt found - using osgQt::osgQOpenGL target")

    message(STATUS "OpenSceneGraph_INCLUDE_DIRS: ${OpenSceneGraph_INCLUDE_DIRS}")
    message(STATUS "OpenSceneGraph_LIBRARIES: ${OpenSceneGraph_LIBRARIES}")

endif()

if(BUILD_SERVER)
    # -------------------------------------------------------------------------
    # Container library paths (CONTAINER_CMAKE_DIR is set by LibrarySearchPaths)
    # -------------------------------------------------------------------------
    if(NOT CONTAINER_CMAKE_DIR)
        if(WIN32)
            set(CONTAINER_CMAKE_DIR "C:/Program Files/ContainerLib/lib/cmake/Container" CACHE PATH "Path to Container library's CMake files")
        elseif(APPLE)
            set(CONTAINER_CMAKE_DIR "/usr/local/lib/cmake/Container" CACHE PATH "Path to Container library's CMake files")
        elseif(UNIX)
            set(CONTAINER_CMAKE_DIR "/usr/local/lib/cmake/Container" CACHE PATH "Path to Container library's CMake files")
        else()
            message(FATAL_ERROR "Unsupported platform. Please set CONTAINER_CMAKE_DIR manually.")
        endif()
    endif()

    # -------------------------------------------------------------------------
    # RabbitMQ-C paths (RABBITMQ_CMAKE_DIR is set by LibrarySearchPaths)
    # -------------------------------------------------------------------------
    if(NOT RABBITMQ_CMAKE_DIR)
        if(WIN32)
            set(RABBITMQ_CMAKE_DIR "C:/Program Files/rabbitmq-c/lib/cmake/rabbitmq-c" CACHE PATH "Path to RabbitMQ-C CMake directory")
        elseif(APPLE)
            set(RABBITMQ_CMAKE_DIR "/usr/local/lib/rabbitmq-c/cmake" CACHE PATH "Path to RabbitMQ-C CMake directory")
        elseif(UNIX)
            set(RABBITMQ_CMAKE_DIR "/usr/local/lib/cmake/rabbitmq-c" CACHE PATH "Path to RabbitMQ-C CMake directory")
        else()
            message(FATAL_ERROR "Unsupported platform. Please set RABBITMQ_CMAKE_DIR manually.")
        endif()
    endif()

    # Find the installed Container library
    find_package(Container REQUIRED PATHS ${CONTAINER_CMAKE_DIR} NO_DEFAULT_PATH)

    if(NOT Container_FOUND)
        message(FATAL_ERROR "Container library not found at ${CONTAINER_CMAKE_DIR}. "
            "Please specify the correct path using -DCONTAINER_CMAKE_DIR=<path>")
    endif()

    # Find RabbitMQ-C
    find_package(RabbitMQ-C REQUIRED CONFIG PATHS ${RABBITMQ_CMAKE_DIR})

    if(NOT RabbitMQ-C_FOUND)
        message(FATAL_ERROR "RabbitMQ-C not found at ${RABBITMQ_CMAKE_DIR}. "
            "Please specify the correct path using -DRABBITMQ_CMAKE_DIR=<path>")
    endif()

    # -------------------------------------------------------------------------
    # Set Container library bin/lib directories for DLL/SO copying
    # -------------------------------------------------------------------------
    get_container_library_paths(CONTAINER_BIN_DIR CONTAINER_LIB_DIR)
    set(CONTAINER_BIN_DIR "${CONTAINER_BIN_DIR}" CACHE PATH "Path to Container library binaries")
    set(CONTAINER_LIB_DIR "${CONTAINER_LIB_DIR}" CACHE PATH "Path to Container library files")
    message(STATUS "Container BIN_DIR: ${CONTAINER_BIN_DIR}")
    message(STATUS "Container LIB_DIR: ${CONTAINER_LIB_DIR}")

    # -------------------------------------------------------------------------
    # Set RabbitMQ bin directory for DLL/SO copying
    # -------------------------------------------------------------------------
    get_rabbitmq_library_path(RABBITMQ_BIN_DIR)
    set(RABBITMQ_BIN_DIR "${RABBITMQ_BIN_DIR}" CACHE PATH "Path to RabbitMQ-C library binaries")
    message(STATUS "RabbitMQ BIN_DIR: ${RABBITMQ_BIN_DIR}")
endif()

# -------------------------------------------------------------------------
# GDAL and GeographicLib - Core dependencies
# Paths are configured by LibrarySearchPaths module based on build type
# -------------------------------------------------------------------------

# GDAL - Allow user override, otherwise use paths from LibrarySearchPaths
if(GDAL_DIR)
    message(STATUS "Using user-specified GDAL_DIR: ${GDAL_DIR}")
    find_package(GDAL REQUIRED CONFIG PATHS ${GDAL_DIR} NO_DEFAULT_PATH)
else()
    # Search in build-type-specific location first, then fall back to system
    find_package(GDAL REQUIRED CONFIG)
endif()

# GeographicLib - Allow user override, otherwise use paths from LibrarySearchPaths
if(GeographicLib_DIR)
    message(STATUS "Using user-specified GeographicLib_DIR: ${GeographicLib_DIR}")
    find_package(GeographicLib REQUIRED CONFIG PATHS ${GeographicLib_DIR} NO_DEFAULT_PATH)
else()
    # Search in build-type-specific location first, then fall back to system
    find_package(GeographicLib REQUIRED CONFIG)
endif()



# Print found library locations for verification
if(GDAL_FOUND)
    message(STATUS "GDAL found: ${GDAL_VERSION}")
    if(GDAL_INCLUDE_DIRS)
        message(STATUS "  Include: ${GDAL_INCLUDE_DIRS}")
    endif()
else()
    message(FATAL_ERROR "GDAL not found. Please specify the path using -DGDAL_DIR=<path>. "
        "For build-type-specific installation, ensure GDAL is installed at /usr/local/${CMAKE_BUILD_TYPE}/")
endif()

if(GeographicLib_FOUND)
    message(STATUS "GeographicLib found: ${GeographicLib_VERSION}")
    if(GeographicLib_INCLUDE_DIRS)
        message(STATUS "  Include: ${GeographicLib_INCLUDE_DIRS}")
    endif()
else()
    message(FATAL_ERROR "GeographicLib not found. Please specify the path using -DGeographicLib_DIR=<path>. "
        "For build-type-specific installation, ensure GeographicLib is installed at /usr/local/${CMAKE_BUILD_TYPE}/")
endif()



# -------------------------------------------------------
# ---------------- RULES AND SUB PROJECTS ---------------
# -------------------------------------------------------

# include src directory
add_subdirectory(src)

# Enable testing
enable_testing()

# Add test directory when BUILD_TESTING is enabled
# Note: For multi-config generators (Visual Studio, Xcode), the build type is selected at build time,
# so we add the test targets unconditionally when BUILD_TESTING is ON
option(BUILD_TESTING "Build the testing tree." OFF)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Installation rules
install(TARGETS ShipNetSim RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
