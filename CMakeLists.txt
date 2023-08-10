# min required cmake version
cmake_minimum_required(VERSION 3.14)

# project name
project(ShipNetSim LANGUAGES CXX)

# Set BOOST_ROOT based on the platform
if(WIN32)
    set(BOOST_ROOT "C:/dev/boost" CACHE STRING "Boost root directory") # Windows path
elseif(APPLE)
    set(BOOST_ROOT "/path/to/boost/on/mac" CACHE STRING "Boost root directory") # macOS path
else()
    set(BOOST_ROOT "/path/to/boost/on/linux" CACHE STRING "Boost root directory") # Linux path
endif()

# include src directory
add_subdirectory(src)
