project(ShipNetSimAdjBuilder VERSION ${ShipNetSim_VERSION} LANGUAGES CXX)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent)

include_directories(${CMAKE_SOURCE_DIR}/src/ShipNetSimCore)

add_executable(ShipNetSimAdjBuilder main.cpp)
add_dependencies(ShipNetSimAdjBuilder ShipNetSimCore)

target_link_libraries(ShipNetSimAdjBuilder PRIVATE
    ShipNetSimCore
    GeographicLib::GeographicLib
    GDAL::GDAL
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
)

# Copy core library to output directory
add_custom_command(TARGET ShipNetSimAdjBuilder POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:ShipNetSimCore> $<TARGET_FILE_DIR:ShipNetSimAdjBuilder>
)

include(GNUInstallDirs)
install(TARGETS ShipNetSimAdjBuilder
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
