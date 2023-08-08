# Make the directory containing units.h available to other targets that need it
target_include_directories(${ShipNeTSim_LIB_TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

