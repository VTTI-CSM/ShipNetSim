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

# # Add the project name as a compiler definition
# add_definitions(-DPROJECT_NAME="${PROJECT_NAME}")


# Add definitions based on build type
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif()


# -------------------------------------------------------
# --------------------- Libraries -----------------------
# --------------- Define Default Paths ------------------
# -------------------------------------------------------

# Library directories - Users can provide path if not set
# Using multiple default paths as hints for find_package
set(GDAL_ROOT_HINTS
    "C:/Program Files/gdal/"
    "C:/Program Files (x86)/gdal/"
    CACHE STRING "Hints for finding GDAL"
)
set(GeographicLib_ROOT_HINTS
    "C:/Program Files/GeographicLib"
    "C:/Program Files (x86)/GeographicLib"
    CACHE STRING "Hints for finding GeographicLib"
)
set(OpenSceneGraph_ROOT_HINTS
    "C:/Program Files/OpenSceneGraph/"
    "C:/Program Files/OSG/"
    "C:/Program Files (x86)/OpenSceneGraph/"
    CACHE STRING "Hints for finding OpenSceneGraph"
)

set(osgEarth_LIBRARY "C:/Program Files/OSGEarth/lib/osgEarth.lib" CACHE PATH "Path to debug library of osgEarth")
set(osgEarth_LIBRARY_DEBUG "C:/Program Files/OSGEarth/lib/osgEarthd.lib" CACHE PATH "Path to debug library of osgEarth")
set(osgEarth_INCLUDE_DIR "C:/Program Files/OSGEarth/include" CACHE PATH "Path to osgEarth include directory")

# Allow the user to override the default paths
if (NOT "${GDAL_DIR}" STREQUAL "")
    list(APPEND GDAL_ROOT_HINTS "${GDAL_DIR}")
endif()
if (NOT "${GeographicLib_DIR}" STREQUAL "")
    list(APPEND GeographicLib_ROOT_HINTS "${GeographicLib_DIR}")
endif()
# If OSG_DIR is provided, append it to the hints list
if (NOT "${OSG_DIR}" STREQUAL "")
    list(APPEND OpenSceneGraph_ROOT_HINTS "${OSG_DIR}")
endif()

# Find packages with specified or default paths
# If the user does not provide a specific path, use default paths as hints
find_package(GDAL REQUIRED HINTS ${GDAL_ROOT_HINTS})
find_package(GeographicLib REQUIRED HINTS ${GeographicLib_ROOT_HINTS})


# Check if the libraries were found, and if
# not, print a fatal error message
if(NOT GDAL_FOUND)
    message(FATAL_ERROR "GDAL not found in
        the specific path: ${GDAL_ROOT}")
endif()
if(NOT GEOGRAPHICLIB_FOUND)
    message(FATAL_ERROR "GeographicLib not found in
        the specific path: ${GEOGRAPHICLIB_ROOT}")
endif()


# -------------------------------------------------------
# --------------------- OPTIONS -------------------------
# -------------------------------------------------------

# Add an option for the user to select building GUI or not. Default is ON.
option(BUILD_GUI "Build the GUI components" ON)
# Cache the option so it's stored between runs
set(BUILD_GUI ${BUILD_GUI} CACHE BOOL "Build the GUI components" FORCE)


# -------------------------------------------------------
# ---------------- RULES AND SUB PROJECTS ---------------
# -------------------------------------------------------

# include src directory
add_subdirectory(src)

# Installation rules
install(TARGETS ShipNetSim RUNTIME DESTINATION bin)

# CPack for packaging
# include(InstallRequiredSystemLibraries)
# set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
# set(CPACK_PACKAGE_VERSION_MAJOR "${ShipNetSim_VERSION_MAJOR}")
# set(CPACK_PACKAGE_VERSION_MINOR "${ShipNetSim_VERSION_MINOR}")
# set(CPACK_PACKAGE_VERSION_PATCH "${ShipNetSim_VERSION_PATCH}")
# include(CPack)
