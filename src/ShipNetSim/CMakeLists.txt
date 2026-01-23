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
target_link_libraries(${SHIPNETSIM_NAME} PRIVATE
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
    target_link_libraries(${SHIPNETSIM_NAME} PRIVATE Container::Container)
    target_include_directories(${SHIPNETSIM_NAME} PRIVATE ${CONTAINER_INCLUDE_DIRS})
    target_compile_definitions(${SHIPNETSIM_NAME} PRIVATE BUILD_SERVER_ENABLED)
endif()



# Ensure the ShipNetSimCore DLL is copied to the output directory
# Note: DEPENDS is not supported with add_custom_command(TARGET) - use add_dependencies instead
add_custom_command(TARGET ShipNetSim POST_BUILD
    # Command to copy core library
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:ShipNetSimCore> $<TARGET_FILE_DIR:ShipNetSim>

    # Command to copy the data directory
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/data $<TARGET_FILE_DIR:ShipNetSim>/../data
)

# Conditionally include Container library only if BUILD_SERVER is ON
# Using generator expressions for proper multi-config generator support
# Container library uses different suffixes for each build type:
#   Debug: d suffix, Release: no suffix, RelWithDebInfo: rd suffix, MinSizeRel: s suffix
if(BUILD_SERVER)
    if(WIN32) # Windows-specific logic
        add_custom_command(TARGET ${SHIPNETSIM_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CONTAINER_BIN_DIR}/$<IF:$<CONFIG:Debug>,Containerd.dll,$<IF:$<CONFIG:RelWithDebInfo>,Containerrd.dll,$<IF:$<CONFIG:MinSizeRel>,Containers.dll,Container.dll>>>"
                "$<TARGET_FILE_DIR:${SHIPNETSIM_NAME}>/$<IF:$<CONFIG:Debug>,Containerd.dll,$<IF:$<CONFIG:RelWithDebInfo>,Containerrd.dll,$<IF:$<CONFIG:MinSizeRel>,Containers.dll,Container.dll>>>"
        )
    elseif(UNIX AND NOT APPLE) # Linux-specific logic
        add_custom_command(TARGET ${SHIPNETSIM_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CONTAINER_LIB_DIR}/$<IF:$<CONFIG:Debug>,libContainerd.so,$<IF:$<CONFIG:RelWithDebInfo>,libContainerrd.so,$<IF:$<CONFIG:MinSizeRel>,libContainers.so,libContainer.so>>>"
                "$<TARGET_FILE_DIR:${SHIPNETSIM_NAME}>/$<IF:$<CONFIG:Debug>,libContainerd.so,$<IF:$<CONFIG:RelWithDebInfo>,libContainerrd.so,$<IF:$<CONFIG:MinSizeRel>,libContainers.so,libContainer.so>>>"
        )
    elseif(APPLE) # macOS-specific logic
        add_custom_command(TARGET ${SHIPNETSIM_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CONTAINER_LIB_DIR}/$<IF:$<CONFIG:Debug>,libContainerd.dylib,$<IF:$<CONFIG:RelWithDebInfo>,libContainerrd.dylib,$<IF:$<CONFIG:MinSizeRel>,libContainers.dylib,libContainer.dylib>>>"
                "$<TARGET_FILE_DIR:${SHIPNETSIM_NAME}>/$<IF:$<CONFIG:Debug>,libContainerd.dylib,$<IF:$<CONFIG:RelWithDebInfo>,libContainerrd.dylib,$<IF:$<CONFIG:MinSizeRel>,libContainers.dylib,libContainer.dylib>>>"
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
