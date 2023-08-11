# min required cmake version
cmake_minimum_required(VERSION 3.24)

# Define the project name (ShipNetSim) and
# the programming language used (CXX for C++)
set(MY_PROJECT_VERSION "1.0.0" CACHE STRING "Project version")


project(ShipNetSim VERSION ${MY_PROJECT_VERSION} LANGUAGES CXX)

# Enable Qt's Automatic User Interface Compiler (UIC)
set(CMAKE_AUTOUIC ON)
# Enable Qt's Meta-Object Compiler (MOC) which allows
# the use of Qt features such as signals and slots
set(CMAKE_AUTOMOC ON)
# Enable Qt's Resource Compiler (RCC) for compiling
# resource files into binary format
set(CMAKE_AUTORCC ON)

# Set the C++20 standard to be used for compiling
set(CMAKE_CXX_STANDARD 20)

# Ensure that the selected C++ standard is a
# requirement for the compiler
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set BOOST_ROOT based on the platform
if(WIN32)
    set(BOOST_ROOT "C:/dev/boost"
        CACHE STRING "Boost root directory") # Windows path
elseif(APPLE)
    set(BOOST_ROOT "/path/to/boost/on/mac"
        CACHE STRING "Boost root directory") # macOS path
else()
    set(BOOST_ROOT "/path/to/boost/on/linux"
        CACHE STRING "Boost root directory") # Linux path
endif()

# include src directory
add_subdirectory(src)
