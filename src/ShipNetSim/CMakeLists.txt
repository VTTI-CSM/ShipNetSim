# Define the project name (ShipNetSim) and
# the programming language used (CXX for C++)
project(${SHIPNETSIM_NAME} VERSION ${ShipNetSim_VERSION} LANGUAGES CXX)

# Define and find the required libraries for the project
# Find Qt version 6 and include the Core, Concurent, Xml, Network components
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Concurrent Xml Sql Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent Xml Sql Network)


# Include directories for ShipNetSimCore
# These include directories should point to where the headers are installed or located
include_directories(${CMAKE_SOURCE_DIR}/src/ShipNetSimCore ${CMAKE_BINARY_DIR}/include)

# Generate the VersionConfig.h file from a template
configure_file(${CMAKE_SOURCE_DIR}/src/ShipNetSimCore/VersionConfig.h.in
               ${CMAKE_BINARY_DIR}/include/VersionConfig.h @ONLY)

# Define the executable target for the project,
# listing the required source and header files
add_executable(${SHIPNETSIM_NAME} main.cpp)

# Ensure that ShipNetSimCore is built first by specifying it as a dependency
add_dependencies(${SHIPNETSIM_NAME} ShipNetSimCore)

# Link required libraries to the executable target
target_link_libraries(${SHIPNETSIM_NAME}
    ShipNetSimCore
    GeographicLib::GeographicLib
    GDAL::GDAL
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Sql
)
# Conditionally include Container library only if BUILD_SERVER is ON
if(BUILD_SERVER)
    target_link_libraries(${SHIPNETSIM_CORE_DLL_NAME} PRIVATE Container::Container)
    target_include_directories(${SHIPNETSIM_CORE_DLL_NAME} PRIVATE ${CONTAINER_INCLUDE_DIRS})
    target_compile_definitions(${SHIPNETSIM_CORE_DLL_NAME} PRIVATE BUILD_SERVER_ENABLED)
endif()



# Ensure the ShipNetSimCore DLL is copied to the output directory
add_custom_command(TARGET ShipNetSim POST_BUILD
    # Command to copy core library
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:ShipNetSimCore> $<TARGET_FILE_DIR:ShipNetSim>

    # Command to copy the data directory
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/data $<TARGET_FILE_DIR:ShipNetSim>/../data

    DEPENDS $<TARGET_FILE:ShipNetSimCore>
)

# Conditionally include Container library only if BUILD_SERVER is ON
if(BUILD_SERVER)
    # Ensure the ShipNetSimCore DLL is copied to the output directory
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        # Copy container.dll only in Release mode
        add_custom_command(TARGET ${SHIPNETSIM_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CONTAINER_BIN_DIR}/container.dll"
                "$<TARGET_FILE_DIR:${SHIPNETSIM_NAME}>/container.dll"
        )
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        # Copy containerd.dll only in Debug mode
        add_custom_command(TARGET ${SHIPNETSIM_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CONTAINER_BIN_DIR}/containerd.dll"
                "$<TARGET_FILE_DIR:${SHIPNETSIM_NAME}>/containerd.dll"
        )
    endif()
endif()


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
