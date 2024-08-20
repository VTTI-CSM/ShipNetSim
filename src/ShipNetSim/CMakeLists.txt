# Define the project name (ShipNetSim) and
# the programming language used (CXX for C++)
project(${SHIPNETSIM_NAME} VERSION ${ShipNetSim_VERSION} LANGUAGES CXX)

# Define and find the required libraries for the project
# Find Qt version 6 and include the Core, Concurent, Xml, Network components
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Concurrent  Xml Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent  Xml Network)


# Include directories for ShipNetSimCore
# These include directories should point to where the headers are installed or located
include_directories(${CMAKE_SOURCE_DIR}/src/ShipNetSimCore)
include_directories(${CMAKE_BINARY_DIR}/include)

# Define the executable target for the project,
# listing the required source and header files
add_executable(${SHIPNETSIM_NAME}
  main.cpp
)

# Ensure that ShipNetSimCore is built first by specifying it as a dependency
add_dependencies(${SHIPNETSIM_NAME} ShipNetSimCore)

# Include directories and link libraries are inherited from the parent scope
target_include_directories(${SHIPNETSIM_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/ShipNetSimCore
    ${CMAKE_SOURCE_DIR}/src/ShipNetSim
    ${CMAKE_BINARY_DIR}
)

# Link the Boost and Qt libraries to the executable target
target_link_libraries(${SHIPNETSIM_NAME}
    ShipNetSimCore
    GeographicLib::GeographicLib
    GDAL::GDAL
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Xml
)

# Set compiler options for different build types using generator expressions
target_compile_options(${SHIPNETSIM_NAME} PRIVATE
    # MSVC-specific flags
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4
        $<$<CONFIG:Debug>:/Od /Zi> # Debug flags
        $<$<CONFIG:Release>:/O3>   # Release flags
    >
    # GCC and Clang-specific flags
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        -Wall
        $<$<CONFIG:Debug>:-O0 -g>  # Debug flags
        $<$<CONFIG:Release>:-O3>   # Release flags
    >
)

# Ensure the ShipNetSimCore DLL is copied to the output directory
add_custom_command(TARGET ShipNetSim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:ShipNetSimCore> $<TARGET_FILE_DIR:ShipNetSim>
)

# set configuration file
# configure_file(VersionConfig.h.in VersionConfig.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Include the GNU installation directories,
# for compatibility with GNU install conventions
include(GNUInstallDirs)

# Install the compiled target (the executable)
# to the specified directories
install(TARGETS ${SHIPNETSIM_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
)
