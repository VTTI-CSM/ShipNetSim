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


# Platform-specific compiler flags
if(MSVC)
    # MSVC-specific flags
    add_compile_options(/W4 /MP)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/Od /Zi)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2)
    endif()
else()
    # GCC/Clang-specific flags
    add_compile_options(-Wall)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-O0 -g)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3)
    endif()
endif()

# Add definitions based on build type
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif()

# -------------------------------------------------------
# --------------------- OPTIONS -------------------------
# -------------------------------------------------------

# Option to build GUI components
option(BUILD_GUI "Build the GUI components" OFF)
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
    # Manually specify paths for osgEarth, OpenSceneGraph, and osgQt
    set(osgEarth_DIR "C:/Program Files/OSGEarth/cmake" CACHE PATH "Default path to osgEarth's cmake directory")
    set(OpenSceneGraph_DIR "C:/Program Files/OpenSceneGraph" CACHE PATH "Default path to OpenSceneGraph's directory")
    set(OSGQT_INCLUDE_DIR "C:/Program Files/osgQt/include" CACHE PATH "Path to osgQt include directory")
    set(OSGQOPENGL_LIB "C:/Program Files/osgQt/lib/osg145-osgQOpenGL.lib" CACHE FILEPATH "Path to osgQt library")

    # Use these paths to find osgEarth and OpenSceneGraph
    find_package(OpenSceneGraph REQUIRED COMPONENTS osg osgDB osgManipulator osgGA osgUtil osgViewer osgSim osgText osgShadow CONFIG PATHS ${OpenSceneGraph_DIR})

    find_package(osgEarth CONFIG REQUIRED PATHS ${osgEarth_DIR})
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib" ".so" ".dylib")

    # If osgEarth is not found, manually specify the include and library paths
    if(NOT osgEarth_FOUND)
        set(osgEarth_INCLUDE_DIR "C:/Program Files/OSGEarth/include" CACHE PATH "Path to osgEarth include directory")
        set(osgEarth_LIBRARY "C:/Program Files/OSGEarth/lib/osgEarth.lib" CACHE PATH "Path to osgEarth library")
        
        if(NOT EXISTS ${osgEarth_INCLUDE_DIR})
            message(FATAL_ERROR "osgEarth include directory not found. Please specify the correct path using osgEarth_INCLUDE_DIR.")
        endif()

        if(NOT EXISTS ${osgEarth_LIBRARY})
            message(FATAL_ERROR "osgEarth library not found. Please specify the correct path using osgEarth_LIBRARY.")
        endif()

        # Set the variables manually for CMake to use
        set(osgEarth_INCLUDE_DIRS ${osgEarth_INCLUDE_DIR})
        set(osgEarth_LIBRARIES ${osgEarth_LIBRARY})
    endif()

    # Check if both osgEarth and OpenSceneGraph libraries were found
    if(NOT osgEarth_FOUND AND (NOT EXISTS ${osgEarth_INCLUDE_DIR} OR NOT EXISTS ${osgEarth_LIBRARY}))
        message(FATAL_ERROR "osgEarth not found and manual paths do not exist. Please specify the correct paths to the osgEarth installation.")
    endif()

    if(NOT OpenSceneGraph_FOUND)
        message(FATAL_ERROR "OpenSceneGraph not found. Please specify the correct path to the OpenSceneGraph installation.")
    endif()

    # Manually specify osgQt include and library paths if needed
    if(NOT EXISTS ${OSGQT_INCLUDE_DIR})
        message(FATAL_ERROR "osgQt include directory not found. Please specify the correct path using OSGQT_INCLUDE_DIR.")
    endif()

    if(NOT EXISTS ${OSGQOPENGL_LIB})
        message(FATAL_ERROR "osgQt library not found. Please specify the correct path using OSGQOPENGL_LIB.")
    endif()

    # Include osgQt directories
    include_directories(${OSGQT_INCLUDE_DIR})

    # Link osgQt library
    set(OSGQT_LIBRARIES ${OSGQOPENGL_LIB})

    message(STATUS "OpenSceneGraph_INCLUDE_DIRS: ${OpenSceneGraph_INCLUDE_DIRS}")
    message(STATUS "OpenSceneGraph_LIBRARIES: ${OpenSceneGraph_LIBRARIES}")

endif()

if(BUILD_SERVER)
    set(CONTAINER_CMAKE_DIR "C:/Program Files/Container/cmake" CACHE PATH "Default path to container's library")
    set(RABBITMQ_CMAKE_DIR "C:/Program Files/rabbitmq-c/lib/cmake/rabbitmq-c")

    # Find the installed Container library
    find_package(Container REQUIRED PATHS ${CONTAINER_CMAKE_DIR} NO_DEFAULT_PATH)

    if (NOT Container_FOUND)
        message(FATAL_ERROR "Container not found. Please specify the correct path to the Container Library cmake installation.")
    endif()

    # Set and cache the path to the bin directory using CONTAINER_CMAKE_DIR
    set(CONTAINER_BIN_DIR "${CONTAINER_CMAKE_DIR}/../bin" CACHE PATH "Path to the Container library's bin directory")

    find_package(RabbitMQ-C REQUIRED CONFIG PATHS ${RABBITMQ_CMAKE_DIR})

    if (NOT RabbitMQ-C_FOUND)
        message(FATAL_ERROR "RabbitMQ-C not found. Please specify the correct path to the RabbitMQ-C cmake installation.")
    endif()

    # Set and cache the path to the RabbitMQ bin directory using RABBITMQ_CMAKE_DIR
    set(RABBITMQ_BIN_DIR "${RABBITMQ_CMAKE_DIR}/../../../bin" CACHE PATH "Path to the RabbitMQ-C library's bin directory")

endif()

# Library directories - Prompt users to set paths if libraries aren't found
set(GDAL_ROOT_HINTS "" CACHE STRING "Path to GDAL root directory")
set(GeographicLib_ROOT_HINTS "" CACHE STRING "Path to GeographicLib root directory")


# Allow users to override default paths
if (NOT "${GDAL_DIR}" STREQUAL "")
    list(APPEND GDAL_ROOT_HINTS "${GDAL_DIR}")
endif()
if (NOT "${GeographicLib_DIR}" STREQUAL "")
    list(APPEND GeographicLib_ROOT_HINTS "${GeographicLib_DIR}")
endif()


# Find packages with specified or default paths
find_package(GDAL REQUIRED HINTS ${GDAL_ROOT_HINTS})
find_package(GeographicLib REQUIRED HINTS ${GeographicLib_ROOT_HINTS})



# Error messages if libraries not found
if(NOT GDAL_FOUND)
    message(FATAL_ERROR "GDAL not found. Please specify the path using GDAL_DIR.")
endif()
if(NOT GeographicLib_FOUND)
    message(FATAL_ERROR "GeographicLib not found. Please specify the path using GeographicLib_DIR.")
endif()



# -------------------------------------------------------
# ---------------- RULES AND SUB PROJECTS ---------------
# -------------------------------------------------------

# include src directory
add_subdirectory(src)

# Installation rules
install(TARGETS ShipNetSim RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
