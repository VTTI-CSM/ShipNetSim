project(${SHIPNETSIM_NAME}"GUI" VERSION ${ShipNetSim_VERSION} LANGUAGES CXX)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets concurrent PrintSupport )
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGLWidgets concurrent PrintSupport )

set(BUILD_STATIC_LIBS ON)
find_package(OpenSceneGraph REQUIRED COMPONENTS osg osgDB osgGA osgUtil osgViewer HINTS ${OpenSceneGraph_ROOT_HINTS})
find_package(osgEarth CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(JKQTPlotter6 REQUIRED HINTS "C:/Program Files/JKQTPlotter")
include_directories("C:/Program Files/osgQt/include")
link_directories("C:/Program Files/osgQt/lib")


# Find all header files in the shipnetsim directory and its subdirectories
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../ShipNetSim/*.h)

# Get unique directories from the list of header files
set(INCLUDE_DIRS "")
foreach(_headerFile ${HEADER_FILES})
    get_filename_component(_dir ${_headerFile} PATH)
    list(APPEND INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)

# Include all directories found
include_directories(${INCLUDE_DIRS})

if(NOT OpenSceneGraph_FOUND)
    message(FATAL_ERROR "GeographicLib not found in
        the specific path: ${GEOGRAPHICLIB_ROOT}")
endif()
if (NOT osgEarth_FOUND)
    message(FATAL_ERROR "osgEarth not found in
        the specific path: ${GDAL_ROOT}")
endif()



set(PROJECT_SOURCES
        ../ShipNetSim/network/point.h
        ../ShipNetSim/network/point.cpp
        ../ShipNetSim/network/gpoint.h
        ../ShipNetSim/network/gpoint.cpp
        ../ShipNetSim/network/seaport.h
        ../ShipNetSim/network/seaport.cpp
        ../ShipNetSim/network/line.h
        ../ShipNetSim/network/line.cpp
        ../ShipNetSim/network/gline.h
        ../ShipNetSim/network/gline.cpp
        ../ShipNetSim/network/polygon.h
        ../ShipNetSim/network/polygon.cpp
        ../ShipNetSim/network/algebraicvector.h
        ../ShipNetSim/network/algebraicvector.cpp
        ../ShipNetSim/network/galgebraicvector.h
        ../ShipNetSim/network/galgebraicvector.cpp
        ../ShipNetSim/network/networkdefaults.h
        ../ShipNetSim/network/optimizedvisibilitygraph.h
        ../ShipNetSim/network/optimizedvisibilitygraph.cpp
        ../ShipNetSim/network/quadtree.h
        ../ShipNetSim/network/quadtree.cpp
        ../ShipNetSim/network/optimizednetwork.h
        ../ShipNetSim/network/optimizednetwork.cpp
        gui/osgearthqtwidget.h gui/osgearthqtwidget.cpp
        defaults.h defaults.cpp
        gui/aboutwindow.cpp
        gui/aboutwindow.h
        gui/aboutwindow.ui
        gui/checkboxdelegate.h
        gui/clickablelabel.cpp
        gui/ClickableLabel.h
        gui/comboboxdelegate.h
        gui/customprogressbar.h
        gui/customtablewidget.cpp
        gui/customtablewidget.h
        gui/disappearinglabel.cpp
        gui/disappearinglabel.h
        gui/intnumericdelegate.h
        gui/nonemptydelegate.h
        gui/numericdelegate.h
        gui/osgwidget.cpp
        gui/osgwidget.h
        gui/settingswindow.cpp
        gui/settingswindow.h
        gui/settingswindow.ui
        # gui/simulationworker.cpp
        # gui/simulationworker.h
        gui/textboxdelegate.h
        gui/togglebutton.cpp
        gui/togglebutton.h
        gui/globalmapmanager.h gui/globalmapmanager.cpp

        main.cpp
        shipnetsim.cpp
        shipnetsim.h
        shipnetsim.ui
)


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ShipNetSimGUI
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        gui/portclickhandler.h


        # guiclasses/osgwidget.h guiclasses/osgwidget.cpp
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ShipNetSimGUI APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(ShipNetSimGUI SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(ShipNetSimGUI
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# Add include directory
target_include_directories(ShipNetSimGUI PRIVATE
    ${osgEarth_INCLUDE_DIRS}
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
)


target_link_libraries(ShipNetSimGUI PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::PrintSupport
    OpenGL::GL
    GeographicLib::GeographicLib
    GDAL::GDAL
    JKQTPlotter${QT_VERSION_MAJOR}::JKQTPlotter${QT_VERSION_MAJOR}
    "C:/Program Files/OpenSceneGraph/lib/osgd.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgDBd.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgManipulatord.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgGAd.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgUtild.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgViewerd.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgSimd.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgTextd.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgUtild.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgDBd.lib"
    "C:/Program Files/OpenSceneGraph/lib/osgShadowd.lib"
    "C:/Program Files/OSGEarth/lib/osgEarthd.lib"
    "C:/Program Files/osgQt/lib/osg145-osgQOpenGLd.lib"
    "C:/Program Files/OpenSceneGraph/lib/ot21-OpenThreadsd.lib"
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ShipNetSimGUI)
endif()
# set_target_properties(ShipNetSimGUI PROPERTIES
#     ${BUNDLE_ID_OPTION}
#     MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
#     MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
#     MACOSX_BUNDLE TRUE
#     WIN32_EXECUTABLE TRUE
# )

include(GNUInstallDirs)
install(TARGETS ShipNetSimGUI
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ShipNetSimGUI)
endif()
