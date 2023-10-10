# Define the project name (ShipNetSim) and
# the programming language used (CXX for C++)
project(${SHIPNETSIM_NAME} VERSION ${ShipNetSim_VERSION} LANGUAGES CXX)
# Add the units library target
add_library(${units_LIB_TARGET_NAME})

# Define and find the required libraries for the project
# Find Qt version 6 and include the Core component
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

# Find the Boost library
find_package(Boost 1.83 REQUIRED )

# Check if the Boost library was found, and if
# not, print a fatal error message
if(NOT Boost_FOUND)
  message(FATAL_ERROR "Boost not found
      in the specified path: ${BOOST_ROOT}")
endif()


# Include the directories for the Boost header files
# and link the Boost library directories
#include_directories(${Boost_INCLUDE_DIRS})
#link_directories(${Boost_LIBRARY_DIRS})


# Define the executable target for the project,
# listing the required source and header files
add_executable(${SHIPNETSIM_NAME}
  main.cpp
  ship/ship.h ship/ship.cpp
  ship/hydrology.h
  ship/ishipresistancepropulsionstrategy.h
  ship/holtropmethod.h ship/holtropmethod.cpp
  utils/logger.h utils/logger.cpp
  network/basegeometry.h
  network/point.h network/point.cpp
  network/line.h network/line.cpp
  network/polygon.h network/polygon.cpp
  network/visibilitygraph.h network/visibilitygraph.cpp
  ship/ishippropeller.h ship/ishippropeller.cpp
  ship/shippropeller.h ship/shippropeller.cpp
  utils/utils.h
  utils/data.h utils/data.cpp
  ship/ienergysource.h
  ship/battery.h ship/battery.cpp
  ship/tank.h ship/tank.cpp
  network/network.h network/network.cpp
  simulator.h simulator.cpp
  network/algebraicvector.h network/algebraicvector.cpp
  ship/shipfuel.h ship/shipfuel.cpp
  ship/ienergyConsumer.h ship/ienergyconsumer.cpp
  ship/ishipengine.h
  ship/shipengine.h ship/shipengine.cpp
  ship/ishipgearbox.h ship/ishipgearbox.cpp
  ship/shipgearbox.h ship/shipgearbox.cpp
  ship/ishipengine.cpp



)

# set configuration file
configure_file(VersionConfig.h.in VersionConfig.h)
target_include_directories(${SHIPNETSIM_NAME} PRIVATE ${CMAKE_BINARY_DIR})

target_include_directories(${SHIPNETSIM_NAME} PRIVATE ${Boost_INCLUDE_DIR})

# Link the Boost and Qt libraries to the executable target
target_link_libraries(${SHIPNETSIM_NAME}
    ${Boost_LIBRARIES}
    Qt${QT_VERSION_MAJOR}::Core)

# Include the GNU installation directories,
# for compatibility with GNU install conventions
include(GNUInstallDirs)

# Install the compiled target (the executable)
# to the specified directories
install(TARGETS ${SHIPNETSIM_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
