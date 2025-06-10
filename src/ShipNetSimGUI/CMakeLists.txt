set(SHIPNETSIM_GUI_NAME "ShipNetSimGUI" CACHE STRING "Project GUI name" FORCE)
project(${SHIPNETSIM_NAME} VERSION ${ShipNetSim_VERSION} LANGUAGES CXX)
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets Concurrent Sql PrintSupport)
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets Concurrent Sql PrintSupport)

if(NOT WIN32)
    find_package(Fontconfig REQUIRED)
endif()

# Find OpenGL 
find_package(OpenGL REQUIRED)


# Define source and header files
set(PROJECT_H_FILES
    utils/defaults.h
    utils/errorhandler.h
    utils/configurationmanager.h
    utils/unittype.h

    gui/windowMangement/aboutwindow.h
    gui/windowMangement/settingswindow.h
    gui/windowMangement/shipnetsimui.h
    gui/windowMangement/shipnetsim.h

    gui/components/osgearthqtwidget.h
    gui/components/checkboxdelegate.h
    gui/components/ClickableLabel.h
    gui/components/comboboxdelegate.h
    gui/components/customprogressbar.h
    gui/components/customtablewidget.h
    gui/components/disappearinglabel.h
    gui/components/intnumericdelegate.h
    gui/components/nonemptydelegate.h
    gui/components/numericdelegate.h
    gui/components/osgwidget.cpp
    gui/components/osgwidget.h
    gui/components/simulationworker.h
    gui/components/textboxdelegate.h
    gui/components/togglebutton.h
    gui/components/globalmapmanager.h
    gui/components/customprogressbar.h
    gui/components/portclickhandler.h
    gui/components/textboxbuttondelegate.h
    gui/components/enginepowerpopupform.h
    gui/components/enginerpmefficiencypopupform.h
    gui/components/generalPopupForm.h
    gui/components/reportwidget.h
    gui/components/customplot.h
    gui/components/spinnerwidget.h
    gui/windowMangement/processingwindow.h

    utils/guiutils.h

    ../third_party/qcustomplot/qcustomplot.h

)

set(PROJECT_UI_FILES
    gui/windowMangement/aboutwindow.ui
    gui/windowMangement/settingswindow.ui
    gui/windowMangement/shipnetsim.ui
    gui/windowMangement/processingwindow.ui
)

set(PROJECT_SOURCES
    utils/defaults.cpp
    utils/errorhandler.cpp
    utils/configurationmanager.cpp

    gui/windowMangement/aboutwindow.cpp
    gui/windowMangement/settingswindow.cpp
    gui/windowMangement/shipnetsimui.cpp
    gui/windowMangement/shipnetsim.cpp

    gui/components/osgearthqtwidget.cpp
    gui/components/clickablelabel.cpp
    gui/components/customtablewidget.cpp
    gui/components/disappearinglabel.cpp
    gui/components/simulationworker.cpp
    gui/components/togglebutton.cpp
    gui/components/globalmapmanager.cpp
    gui/components/reportwidget.cpp
    gui/components/customplot.cpp
    gui/components/spinnerwidget.cpp
    gui/windowMangement/processingwindow.cpp

    ../third_party/qcustomplot/qcustomplot.cpp

    main.cpp
)

# set configuration file
configure_file(VersionConfig.h.in VersionConfig.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${SHIPNETSIM_GUI_NAME}
        MANUAL_FINALIZATION

        ${PROJECT_H_FILES}
        ${PROJECT_UI_FILES}
        ${PROJECT_SOURCES}
        gui/media/src.qrc

        # guiclasses/osgwidget.h guiclasses/osgwidget.cpp
    )
else()
    if(ANDROID)
        add_library(${SHIPNETSIM_GUI_NAME} SHARED
            ${PROJECT_SOURCES}
        )
else()
    add_executable(${SHIPNETSIM_GUI_NAME}
        ${PROJECT_SOURCES}
    )
endif()
endif()

# Ensure that ShipNetSimCore is built first by specifying it as a dependency
add_dependencies(${SHIPNETSIM_GUI_NAME} ShipNetSimCore)

# Include necessary directories
target_include_directories(${SHIPNETSIM_GUI_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/ShipNetSimCore
    ${CMAKE_SOURCE_DIR}/src/ShipNetSimGUI     # Include the source files
    ${osgEarth_INCLUDE_DIRS}                  # Include osgEarth headers
    ${OpenSceneGraph_INCLUDE_DIRS}            # Include OpenSceneGraph headers
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Link necessary libraries
target_link_libraries(${SHIPNETSIM_GUI_NAME} PRIVATE
    ShipNetSimCore
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::Sql
    OpenGL::GL
    # ${OPENGL_gl_LIBRARY}
    GeographicLib::GeographicLib
    GDAL::GDAL
    osgEarth
    ${OSGQT_LIBRARIES}
    osg3::osg
    osg3::osgDB
    osg3::osgGA
    osg3::osgManipulator
    osg3::osgUtil
    osg3::osgViewer
    osg3::osgSim
    # osg3::osgText
    osg3::osgShadow
    osg3::OpenThreads
    KDReports::kdreports
)

# Add Fontconfig only for non-Windows platforms
if(NOT WIN32)
    target_link_libraries(${SHIPNETSIM_GUI_NAME} PRIVATE Fontconfig::Fontconfig)
endif()

# Apply additional linker flags, if necessary
if(osgEarth_LDFLAGS)
    target_link_options(${PROJECT_NAME} PRIVATE ${osgEarth_LDFLAGS})
endif()

# Apply additional compiler definitions, if necessary
if(osgEarth_DEFINITIONS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${osgEarth_DEFINITIONS})
endif()

# Conditionally include Container library only if BUILD_SERVER is ON
if(BUILD_SERVER)
    target_link_libraries(${SHIPNETSIM_GUI_NAME} PRIVATE Container::Container)
    target_include_directories(${SHIPNETSIM_GUI_NAME} PRIVATE ${CONTAINER_INCLUDE_DIRS})
    target_compile_definitions(${SHIPNETSIM_GUI_NAME} PRIVATE BUILD_SERVER_ENABLED)
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ShipNetSimGUI)
endif()

include(GNUInstallDirs)
install(TARGETS ${SHIPNETSIM_GUI_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

target_compile_options(${SHIPNETSIM_NAME} PRIVATE
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

# Conditionally include Container library only if BUILD_SERVER is ON
if(BUILD_SERVER)
    if(WIN32) # Windows-specific logic
        # Ensure the ShipNetSimCore DLL is copied to the output directory
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            # Copy container.dll only in Release mode
            add_custom_command(TARGET ${SHIPNETSIM_GUI_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CONTAINER_BIN_DIR}/container.dll"
                "$<TARGET_FILE_DIR:${SHIPNETSIM_GUI_NAME}>/container.dll"
            )
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        # Copy containerd.dll only in Debug mode
        add_custom_command(TARGET ${SHIPNETSIM_GUI_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CONTAINER_BIN_DIR}/containerd.dll"
            "$<TARGET_FILE_DIR:${SHIPNETSIM_GUI_NAME}>/containerd.dll"
        )
endif()
endif()

endif()

# Ensure the ShipNetSimCore DLL is copied to the output directory
add_custom_command(TARGET ${SHIPNETSIM_GUI_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:ShipNetSimCore> $<TARGET_FILE_DIR:ShipNetSimGUI>
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${SHIPNETSIM_GUI_NAME})
endif()
