# Define the project name (ShipNetSimCore) and the programming language used (CXX for C++)
set(SHIPNETSIM_CORE_DLL_NAME "ShipNetSimCore" CACHE STRING "Project CORE name" FORCE)
project(${SHIPNETSIM_CORE_DLL_NAME} VERSION ${ShipNetSim_VERSION} LANGUAGES CXX)

# Add the units library target
add_library(${units_LIB_TARGET_NAME})

# Define and find the required libraries for the project
# Find Qt version 6 and include the Core, Concurrent, Xml, Network components
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Concurrent Xml Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent Xml Network)

# Define the shared library target for the project,
# listing the required source and header files
add_library(${SHIPNETSIM_CORE_DLL_NAME} SHARED
  ship/ship.h ship/ship.cpp
  ship/hydrology.h
  ship/ishipcalmresistancestrategy.h
  ship/ishipdynamicresistancestrategy.h
  ship/holtropmethod.h ship/holtropmethod.cpp
  utils/logger.h utils/logger.cpp
  network/basegeometry.h
  network/point.h network/point.cpp
  network/line.h network/line.cpp
  network/polygon.h network/polygon.cpp
  ship/ishippropeller.h ship/ishippropeller.cpp
  ship/shippropeller.h ship/shippropeller.cpp
  utils/utils.h utils/utils.cpp
  utils/data.h utils/data.cpp
  ship/ienergysource.h
  ship/battery.h ship/battery.cpp
  ship/tank.h ship/tank.cpp
  simulator.h simulator.cpp
  network/algebraicvector.h network/algebraicvector.cpp
  ship/shipfuel.h ship/shipfuel.cpp
  ship/ienergyConsumer.h ship/ienergyconsumer.cpp
  ship/ishipengine.h ship/ishipengine.cpp
  ship/shipengine.h ship/shipengine.cpp
  ship/ishipgearbox.h ship/ishipgearbox.cpp
  ship/shipgearbox.h ship/shipgearbox.cpp
  ship/shipsList.h ship/shipsList.cpp
  ship/defaults.h
  network/quadtree.h network/quadtree.cpp
  network/optimizedvisibilitygraph.h network/optimizedvisibilitygraph.cpp
  network/optimizednetwork.h network/optimizednetwork.cpp
  ship/langmaomethod.h ship/langmaomethod.cpp
  network/gpoint.h network/gpoint.cpp
  network/gline.h network/gline.cpp
  network/galgebraicvector.h network/galgebraicvector.cpp
  network/seaport.h network/seaport.cpp
  utils/updatechecker.h utils/updatechecker.cpp
  network/networkdefaults.h
)

# Define SHIPNETSIM_LIBRARY during compilation to activate symbol export
target_compile_definitions(${SHIPNETSIM_CORE_DLL_NAME} PRIVATE SHIPNETSIM_LIBRARY)

# Include directories and link libraries are inherited from the parent scope
target_include_directories(${SHIPNETSIM_CORE_DLL_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/ShipNetSim
    ${CMAKE_BINARY_DIR}
)

# Link the Boost and Qt libraries to the shared library target
target_link_libraries(${SHIPNETSIM_CORE_DLL_NAME}
    GeographicLib::GeographicLib
    GDAL::GDAL
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Xml
)

# Set compiler options for different build types using generator expressions
target_compile_options(${SHIPNETSIM_CORE_DLL_NAME} PRIVATE
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

# Set configuration file
configure_file(VersionConfig.h.in VersionConfig.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Include the GNU installation directories,
# for compatibility with GNU install conventions
include(GNUInstallDirs)

# Install the compiled target (the shared library)
# to the specified directories
install(TARGETS ${SHIPNETSIM_CORE_DLL_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
)

# Install header files
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/ShipNetSimCore/
        DESTINATION include/ShipNetSimCore
        FILES_MATCHING PATTERN "*.h")
