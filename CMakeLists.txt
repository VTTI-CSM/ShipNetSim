# min required cmake version
cmake_minimum_required(VERSION 3.24)

# Define the project name (ShipNetSim) and
# the programming language used (CXX for C++)
set(MY_PROJECT_VERSION "1.0.0" CACHE STRING "Project version")


project(ShipNetSim VERSION ${MY_PROJECT_VERSION} LANGUAGES CXX)

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



# Boost library settings: turn off static linking,
# enable multi-threading, turn off static runtime
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

# include src directory
add_subdirectory(src)
