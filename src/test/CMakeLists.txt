cmake_minimum_required(VERSION 3.19)

project(HydrologyTest)

find_package(Qt6 COMPONENTS Test REQUIRED)

add_executable(${PROJECT_NAME}
    hydrologytest.cpp
    ../ShipNetSim/ship/hydrology.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Test)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
