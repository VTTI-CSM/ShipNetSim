set(SHIPNETSIM_GUI_NAME "ShipNetSimGUI" CACHE STRING "Project GUI name" FORCE)
project(${SHIPNETSIM_NAME} VERSION ${ShipNetSim_VERSION} LANGUAGES CXX)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets concurrent PrintSupport )
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGLWidgets concurrent PrintSupport )

set(BUILD_STATIC_LIBS ON)
find_package(OpenSceneGraph REQUIRED COMPONENTS osg osgDB osgGA osgUtil osgViewer HINTS ${OpenSceneGraph_ROOT_HINTS})
find_package(osgEarth CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(JKQTPlotter6 REQUIRED HINTS "C:/Program Files/JKQTPlotter")
include_directories("C:/Program Files/osgQt/include")
link_directories("C:/Program Files/osgQt/lib")


# Include directories for ShipNetSimCore
# These include directories should point to where the headers are installed or located
include_directories(
    ${CMAKE_SOURCE_DIR}/src/ShipNetSimCore
    ${CMAKE_BINARY_DIR}/include
)

# Find all header files in the shipnetsim directory and its subdirectories
#file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../ShipNetSim/*.h)

# Get unique directories from the list of header files
set(INCLUDE_DIRS "")
foreach(_headerFile ${HEADER_FILES})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)

# Include all directories found
include_directories(${INCLUDE_DIRS}
${CMAKE_SOURCE_DIR}/src
)

if(NOT OpenSceneGraph_FOUND)
    message(FATAL_ERROR "GeographicLib not found in
        the specific path: ${GEOGRAPHICLIB_ROOT}")
endif()
if (NOT osgEarth_FOUND)
    message(FATAL_ERROR "osgEarth not found in
        the specific path: ${GDAL_ROOT}")
endif()

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

    ../third_party/qcustomplot/qcustomplot.h

)

set(PROJECT_UI_FILES
        gui/windowMangement/aboutwindow.ui
        gui/windowMangement/settingswindow.ui
        gui/windowMangement/shipnetsim.ui
)

set(PROJECT_SOURCES
    utils/defaults.cpp
    utils/configurationmanager.cpp

    gui/windowMangement/aboutwindow.cpp
    gui/windowMangement/settingswindow.cpp
    gui/windowMangement/shipnetsimui.cpp

    gui/components/osgearthqtwidget.cpp
    gui/components/clickablelabel.cpp
    gui/components/customtablewidget.cpp
    gui/components/disappearinglabel.cpp
    gui/components/simulationworker.cpp
    gui/components/togglebutton.cpp
    gui/components/globalmapmanager.cpp
    gui/windowMangement/shipnetsim.cpp

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
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ${SHIPNETSIM_GUI_NAME} APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(${SHIPNETSIM_GUI_NAME} SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(${SHIPNETSIM_GUI_NAME}
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# Ensure that ShipNetSimCore is built first by specifying it as a dependency
add_dependencies(${SHIPNETSIM_GUI_NAME} ShipNetSimCore)

# Add include directory
target_include_directories(${SHIPNETSIM_GUI_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/ShipNetSimCore
    ${CMAKE_SOURCE_DIR}/src/ShipNetSim
    ${osgEarth_INCLUDE_DIRS}
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
)


target_link_libraries(${SHIPNETSIM_GUI_NAME} PRIVATE
    ShipNetSimCore
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::PrintSupport
    OpenGL::GL
    GeographicLib::GeographicLib
    GDAL::GDAL
    JKQTPlotter${QT_VERSION_MAJOR}::JKQTPlotter${QT_VERSION_MAJOR}
    "C:/Program Files/OpenSceneGraph/lib/osg.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgDB.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgManipulator.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgGA.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgUtil.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgViewer.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgSim.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgText.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgUtil.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgDB.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgShadow.lib"
    "C:/Program Files/OSGEarth/lib/osgEarth.lib"
    "C:/Program Files/osgQt/lib/osg145-osgQOpenGL.lib"
    "C:/Program Files/OpenSceneGraph/lib/ot21-OpenThreads.lib"
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ShipNetSimGUI)
endif()
# set_target_properties(${SHIPNETSIM_GUI_NAME} PROPERTIES
#     ${BUNDLE_ID_OPTION}
#     MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
#     MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
#     MACOSX_BUNDLE TRUE
#     WIN32_EXECUTABLE TRUE
# )

include(GNUInstallDirs)
install(TARGETS ${SHIPNETSIM_GUI_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


# Ensure the ShipNetSimCore DLL is copied to the output directory
add_custom_command(TARGET ${SHIPNETSIM_GUI_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:ShipNetSimCore> $<TARGET_FILE_DIR:ShipNetSimGUI>
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${SHIPNETSIM_GUI_NAME})
endif()
