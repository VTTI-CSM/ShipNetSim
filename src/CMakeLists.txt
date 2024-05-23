# Indicate that this directory contains additional CMakeLists.txt that should be processed
cmake_minimum_required(VERSION 3.24)

# Set the variable 'units_LIB_TARGET_NAME' with the value "unitsLib".
# This can be used as the name of a library target in the project
set(units_LIB_TARGET_NAME "unitsLib")

# Core simulation engine
add_subdirectory(ShipNetSim)

if (BUILD_GUI)
    # Graphical User Interface for the simulation
    add_subdirectory(ShipNetSimGUI)
endif()

# Third-party dependencies or shared libraries
add_subdirectory(third_party)

# Add the sudirectory 'test' to the build.
# add_subdirectory(test)

# Add the subdirectory 'ShipNetSimInstaller' to the build.
#add_subdirectory(ShipNetSimInstaller)

