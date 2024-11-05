# Indicate that this directory contains additional CMakeLists.txt that should be processed
cmake_minimum_required(VERSION 3.24)

# Set the variable 'units_LIB_TARGET_NAME' with the value "unitsLib".
# This can be used as the name of a library target in the project
set(units_LIB_TARGET_NAME "unitsLib")

# Core simulation engine DLL - build this first
add_subdirectory(ShipNetSimCore)

# Executable program
add_subdirectory(ShipNetSim)

# GUI components
if (BUILD_GUI)
    add_subdirectory(ShipNetSimGUI)
endif()

if (BUILD_SERVER)
    add_subdirectory(ShipNetSimServer)
endif()

# Third-party dependencies or shared libraries
add_subdirectory(third_party)

# Add the sudirectory 'test' to the build.
# add_subdirectory(test)

# Add the subdirectory 'ShipNetSimInstaller' to the build.
if(BUILD_INSTALLER)
    add_subdirectory(ShipNetSimInstaller)
endif()
