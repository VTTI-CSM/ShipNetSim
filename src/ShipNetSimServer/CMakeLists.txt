# Define the project name (ShipNetSim) and
# the programming language used (CXX for C++)
set(SHIPNETSIM_SERVER_NAME "ShipNetSimServer" CACHE STRING "ShipNetSimServer name" FORCE)
project(${SHIPNETSIM_SERVER_NAME} VERSION ${ShipNetSim_VERSION} LANGUAGES CXX)

# Define and find the required libraries for the project
# Find Qt version 6 and include the Core, Concurent, Xml, Network components
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Concurrent Xml Sql Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent Xml Sql Network)


# Include directories for ShipNetSimServer
# These include directories should point to where the headers are installed or located
include_directories(${CMAKE_SOURCE_DIR}/src/ShipNetSimCore ${CMAKE_BINARY_DIR}/include)

# Generate the VersionConfig.h file from a template
configure_file(${CMAKE_SOURCE_DIR}/src/ShipNetSimServer/VersionConfig.h.in
               ${CMAKE_BINARY_DIR}/include/VersionConfig.h @ONLY)

# Define the executable target for the project,
# listing the required source and header files
add_executable(${SHIPNETSIM_SERVER_NAME}
    simulationserver.h simulationserver.cpp
    # simulationworker.h simulationworker.cpp
    main.cpp
)

# Ensure that ShipNetSimCore is built first by specifying it as a dependency
add_dependencies(${SHIPNETSIM_SERVER_NAME} ShipNetSimCore)

# Link required libraries to the executable target
target_link_libraries(${SHIPNETSIM_SERVER_NAME} PRIVATE
    ShipNetSimCore
    GeographicLib::GeographicLib
    GDAL::GDAL
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Sql
    Container::Container
    rabbitmq::rabbitmq
)

# Include Container library
target_include_directories(${SHIPNETSIM_CORE_DLL_NAME} PRIVATE ${CONTAINER_INCLUDE_DIRS})
target_compile_definitions(${SHIPNETSIM_CORE_DLL_NAME} PRIVATE BUILD_SERVER_ENABLED)


# Set compiler options for different build types using generator expressions
target_compile_options(${SHIPNETSIM_SERVER_NAME} PRIVATE
    # MSVC-specific flags
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4
        $<$<CONFIG:Debug>:/Od /Zi> # Debug flags
        $<$<CONFIG:Release>:/Ox>   # Release flags
    >
    # GCC and Clang-specific flags
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        -Wall
        $<$<CONFIG:Debug>:-O0 -g>  # Debug flags
        $<$<CONFIG:Release>:-O3>   # Release flags
    >
)

# Ensure the ShipNetSimCore DLL is copied to the output directory
add_custom_command(TARGET ${SHIPNETSIM_SERVER_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:ShipNetSimCore> $<TARGET_FILE_DIR:${SHIPNETSIM_SERVER_NAME}>
    DEPENDS $<TARGET_FILE:ShipNetSimCore>
)

# Ensure the ShipNetSimCore DLL is copied to the output directory
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Copy container.dll only in Release mode
    add_custom_command(TARGET ${SHIPNETSIM_SERVER_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CONTAINER_BIN_DIR}/container.dll"
            "$<TARGET_FILE_DIR:${SHIPNETSIM_SERVER_NAME}>/container.dll"
    )

    add_custom_command(TARGET ${SHIPNETSIM_SERVER_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RABBITMQ_BIN_DIR}/rabbitmq.4.dll"
            "$<TARGET_FILE_DIR:${SHIPNETSIM_SERVER_NAME}>/rabbitmq.4.dll"
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Copy containerd.dll only in Debug mode
    add_custom_command(TARGET ${SHIPNETSIM_SERVER_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CONTAINER_BIN_DIR}/containerd.dll"
            "$<TARGET_FILE_DIR:${SHIPNETSIM_SERVER_NAME}>/containerd.dll"
    )

    add_custom_command(TARGET ${SHIPNETSIM_SERVER_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RABBITMQ_BIN_DIR}/rabbitmq.4.dll"
            "$<TARGET_FILE_DIR:${SHIPNETSIM_SERVER_NAME}>/rabbitmq.4.dll"
    )
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
