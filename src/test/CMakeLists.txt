cmake_minimum_required(VERSION 3.19)

project(test)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Widgets Test REQUIRED)

add_executable(${PROJECT_NAME}
    ../ShipNetSim/ship/ishipresistancepropulsionstrategy.h
    ../ShipNetSim/ship/hydrology.h
    ../ShipNetSim/ship/ship.h
    ../ShipNetSim/ship/holtropmethod.cpp
    ../ShipNetSim/ship/ship.cpp
    hydrologytest.h
    hydrologytest.cpp
    shiptest.h
    shiptest.cpp
    holtropresistancemethodtest.h
    holtropresistancemethodtest.cpp
    testHandler.cpp

)


target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Test)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
