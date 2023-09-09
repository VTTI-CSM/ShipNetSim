# Define the project name (ShipNetSim) and
# the programming language used (CXX for C++)
project(ShipNetSim VERSION ${MY_PROJECT_VERSION} LANGUAGES CXX)

# Boost library settings: turn off static linking,
# enable multi-threading, turn off static runtime
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

# Add the units library target
add_library(${units_LIB_TARGET_NAME})

# Define and find the required libraries for the project
# Find Qt version 6 and include the Core component
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

# Find the Boost library
find_package(Boost REQUIRED)

# Check if the Boost library was found, and if
# not, print a fatal error message
if(NOT Boost_FOUND)
  message(FATAL_ERROR "Boost not found
      in the specified path: ${BOOST_ROOT}")
endif()


# Include the directories for the Boost header files
# and link the Boost library directories
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

# Define the executable target for the project,
# listing the required source and header files
add_executable(ShipNetSim
  main.cpp
  ship/ship.h ship/ship.cpp
  ship/hydrology.h
  ship/ishipresistancepropulsionstrategy.h
  ship/holtropmethod.h ship/holtropmethod.cpp
  utils/logger.h utils/logger.cpp
  network/geometry.h
  network/point.h network/point.cpp
  network/line.h network/line.cpp
  network/polygon.h network/polygon.cpp
  network/visibilitygraph.h network/visibilitygraph.cpp



)

# Link the Boost and Qt libraries to the executable target
target_link_libraries(ShipNetSim
    ${Boost_LIBRARIES}
    Qt${QT_VERSION_MAJOR}::Core)

# Include the GNU installation directories,
# for compatibility with GNU install conventions
include(GNUInstallDirs)

# Install the compiled target (the executable)
# to the specified directories
install(TARGETS ShipNetSim
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


