# min required cmake version
cmake_minimum_required(VERSION 3.24)

# Define the project name (ShipNetSim) and
# the programming language used (CXX for C++)
set(ShipNetSim_VERSION "0.0.1" CACHE STRING "Project version" FORCE)
set(SHIPNETSIM_NAME "ShipNetSim" CACHE STRING "Project name" FORCE)
set(SHIPNETSIM_VENDOR "(C) 2022-2023 Virginia Tech Transportation Institute - Center for Sustainable Mobility." CACHE STRING "Project vendor" FORCE)
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

# Add the project name as a compiler definition
add_definitions(-DPROJECT_NAME="${PROJECT_NAME}")

# Enable Qt's Automatic User Interface Compiler (UIC)
set(CMAKE_AUTOUIC ON)
# Enable Qt's Meta-Object Compiler (MOC) which allows
# the use of Qt features such as signals and slots
set(CMAKE_AUTOMOC ON)
# Enable Qt's Resource Compiler (RCC) for compiling
# resource files into binary format
set(CMAKE_AUTORCC ON)

# Set the C++20 standard to be used for compiling
set(CMAKE_CXX_STANDARD 23)

# Ensure that the selected C++ standard is a
# requirement for the compiler
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif()

# Set BOOST_ROOT based on the platform
if(WIN32)
    set(BOOST_ROOT "C:/Boost"
        CACHE STRING "Boost root directory") # Windows path
elseif(APPLE)
    set(BOOST_ROOT "/path/to/boost/on/mac"
        CACHE STRING "Boost root directory") # macOS path
else()
    set(BOOST_ROOT "/path/to/boost/on/linux"
        CACHE STRING "Boost root directory") # Linux path
endif()

# Set GeographicLib based on the platform
if(WIN32)
    set(GeographicLib_ROOT " C:/Program Files/GeographicLib"
        CACHE STRING "GeographicLib root directory") # Windows Path
elseif(APPLE)
    set(GeographicLib_ROOT "/path/to/gdal/on//max"
        CACHE STRING "GeographicLib root directory") # macOS Path
else()
    set(GeographicLib_ROOT "/path/to/gdal/on/linux"
        CACHE STRING "GeographicLib root directory") # Linux Path
endif()

# Set GDAL based on the platform
if(WIN32)
    set(GDAL_ROOT "C:/Program Files/gdal"
        CACHE STRING "GDAL root directory") # Windows Path
elseif(APPLE)
    set(GDAL_ROOT "/path/to/gdal/on//max"
        CACHE STRING "GDAL root directory") # macOS Path
else()
    set(GDAL_ROOT "/path/to/gdal/on/linux"
        CACHE STRING "GDAL root directory") # Linux Path
endif()

if(WIN32)
    set(LIBTINYTIFF_INCLUDE_DIR "C:/dev/TinyTIFF/include/lib/include"
        CACHE STRING "TinyTiff include directory")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(LIBTINYTIFF_LIBRARY
            "C:/dev/TinyTIFF/include/lib/lib/TinyTIFFShared_Debug.lib"
            CACHE STRING "TinyTiff lib directory")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(LIBTINYTIFF_LIBRARY
            "C:/dev/TinyTIFF/include/lib/lib/TinyTIFFShared_Release.lib"
            CACHE STRING "TinyTiff lib directory")
    else()
        message(FATAL_ERROR "Unsupported build type: ${CMAKE_BUILD_TYPE}")
    endif()

elseif(APPLE)
    set(TINYTIFF_ROOT ""
        CACHE STRING "TinyTiff include directory")

else()
    set(TINYTIFF_ROOT ""
        CACHE STRING "TinyTiff include directory") # Linux path
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Boost library settings: turn off static linking,
# enable multi-threading, turn off static runtime
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)


# include src directory
add_subdirectory(src)
