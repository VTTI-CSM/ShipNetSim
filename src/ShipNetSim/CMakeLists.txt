cmake_minimum_required(VERSION 3.14)

project(ShipNetSim LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${ShipNeTSim_LIB_TARGET_NAME})

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

add_executable(ShipNetSim
  main.cpp
  ship.h ship.cpp
  hydrology.h
)
target_link_libraries(ShipNetSim Qt${QT_VERSION_MAJOR}::Core)

include(GNUInstallDirs)
install(TARGETS ShipNetSim
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


