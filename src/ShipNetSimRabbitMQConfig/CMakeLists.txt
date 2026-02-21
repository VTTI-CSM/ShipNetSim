find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Xml Network)
find_package(Qt6Keychain QUIET)

set(RABBITMQ_CONFIG_SOURCES
    main.cpp
    RabbitMQConfigDialog.h
    RabbitMQConfigDialog.cpp
)

add_executable(ShipNetSimRabbitMQConfig ${RABBITMQ_CONFIG_SOURCES})

target_link_libraries(ShipNetSimRabbitMQConfig
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Xml
    Qt6::Network
)

# Conditionally link QtKeychain if available
if(Qt6Keychain_FOUND)
    target_link_libraries(ShipNetSimRabbitMQConfig PRIVATE Qt6Keychain::Qt6Keychain)
    target_compile_definitions(ShipNetSimRabbitMQConfig PRIVATE HAVE_QTKEYCHAIN)
endif()

target_include_directories(ShipNetSimRabbitMQConfig
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

# Installation
install(TARGETS ShipNetSimRabbitMQConfig
    RUNTIME DESTINATION bin
    COMPONENT ShipNetSimServer_COMP
)
