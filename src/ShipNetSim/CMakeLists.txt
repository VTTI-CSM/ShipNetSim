# Define the project name (ShipNetSim) and
# the programming language used (CXX for C++)
project(${SHIPNETSIM_NAME} VERSION ${ShipNetSim_VERSION} LANGUAGES CXX)
# Add the units library target
add_library(${units_LIB_TARGET_NAME})

# Define and find the required libraries for the project
# Find Qt version 6 and include the Core, Concurent, Xml, Network components
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Concurrent  Xml Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent  Xml Network)

# Find the libraries
find_package(Boost 1.83 REQUIRED HINTS ${BOOST_DIR} )
find_package(GeographicLib REQUIRED HINTS ${GeographicLib_DIR} )
find_package(GDAL REQUIRED HINTS ${GDAL_DIR} )

# Include the directories for the libTinyTIFF header files
include_directories(${LIBTINYTIFF_INCLUDE_DIR})

# Check if the Boost library was found, and if
# not, print a fatal error message
if(NOT Boost_FOUND)
  message(FATAL_ERROR "Boost not found
      in the specified path: ${BOOST_ROOT}")
endif()

if(NOT GEOGRAPHICLIB_FOUND)
    message(FATAL_ERROR "GeographicLib not found in
        the specific path: ${GEOGRAPHICLIB_ROOT}")
endif()

if(NOT GDAL_FOUND)
    message(FATAL_ERROR "GDAL not found in
        the specific path: ${GDAL_ROOT}")
endif()


# Include the directories for the Boost header files
# and link the Boost library directories
#include_directories(${Boost_INCLUDE_DIRS})
#link_directories(${Boost_LIBRARY_DIRS})

include_directories(${GDAL_INCLUDE_DIR})
include_directories(${GeographicLib_INCLUDE_DIR})
link_directories(${GDAL_LIBRARY})

# Define the executable target for the project,
# listing the required source and header files
add_executable(${SHIPNETSIM_NAME}
  main.cpp
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
  utils/utils.h
  utils/data.h utils/data.cpp
  ship/ienergysource.h
  ship/battery.h ship/battery.cpp
  ship/tank.h ship/tank.cpp
  simulator.h simulator.cpp
  network/algebraicvector.h network/algebraicvector.cpp
  ship/shipfuel.h ship/shipfuel.cpp
  ship/ienergyConsumer.h ship/ienergyconsumer.cpp
  ship/ishipengine.h
  ship/shipengine.h ship/shipengine.cpp
  ship/ishipgearbox.h ship/ishipgearbox.cpp
  ship/shipgearbox.h ship/shipgearbox.cpp
  ship/readships.h
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



)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Add compiler flags for Debug mode
if(MSVC)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${SHIPNETSIM_NAME} PRIVATE /W4 /Od /Zi) #/fsanitize=address
    # set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /fsanitize=address")
  endif()
else() # GCC or Clang
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${SHIPNETSIM_NAME} PRIVATE -Wall -O0 -g ) #-fsanitize=address
    # set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address")
  endif()
endif()

# Add compiler flags for Release mode
if(MSVC)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${SHIPNETSIM_NAME} PRIVATE /W4 /O3)
  endif()
else() # GCC or Clang
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${SHIPNETSIM_NAME} PRIVATE -Wall -O3)
  endif()
endif()


# set configuration file
configure_file(VersionConfig.h.in VersionConfig.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(${SHIPNETSIM_NAME} PRIVATE ${CMAKE_BINARY_DIR})

target_include_directories(${SHIPNETSIM_NAME} PRIVATE ${Boost_INCLUDE_DIR})

target_include_directories(${SHIPNETSIM_NAME} PRIVATE ${GeographicLib_INCLUDE_DIR})

target_include_directories(${SHIPNETSIM_NAME} PRIVATE ${GDAL_INCLUDE_DIR})

target_include_directories(${SHIPNETSIM_NAME} PRIVATE ${LIBTINYTIFF_INCLUDE_DIR})


# Link the Boost and Qt libraries to the executable target
target_link_libraries(${SHIPNETSIM_NAME}
    ${Boost_LIBRARIES}
    ${GeographicLib_LIBRARIES}
    ${GDAL_LIBRARY}
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Xml
    ${LIBTINYTIFF_LIBRARY}
)

# Include the GNU installation directories,
# for compatibility with GNU install conventions
include(GNUInstallDirs)

# Install the compiled target (the executable)
# to the specified directories
install(TARGETS ${SHIPNETSIM_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
