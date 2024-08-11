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
        ../ShipNetSim/network/point.h
        ../ShipNetSim/network/gpoint.h
        ../ShipNetSim/network/seaport.h
        ../ShipNetSim/network/line.h
        ../ShipNetSim/network/gline.h
        ../ShipNetSim/network/polygon.h
        ../ShipNetSim/network/algebraicvector.h
        ../ShipNetSim/network/galgebraicvector.h
        ../ShipNetSim/network/networkdefaults.h
        ../ShipNetSim/network/optimizedvisibilitygraph.h
        ../ShipNetSim/network/quadtree.h
        ../ShipNetSim/network/optimizednetwork.h
        ../ShipNetSim/utils/data.h

        ../ShipNetSim/ship/battery.h
        ../ShipNetSim/ship/defaults.h
        ../ShipNetSim/ship/holtropmethod.h
        ../ShipNetSim/ship/hydrology.h
        ../ShipNetSim/ship/ienergyconsumer.h
        ../ShipNetSim/ship/ienergysource.h
        ../ShipNetSim/ship/ishipcalmresistancestrategy.h
        ../ShipNetSim/ship/ishipdynamicresistancestrategy.h
        ../ShipNetSim/ship/ishipengine.h
        ../ShipNetSim/ship/ishipgearbox.h
        ../ShipNetSim/ship/ishippropeller.h
        ../ShipNetSim/ship/langmaomethod.h
        ../ShipNetSim/ship/shipslist.h
        ../ShipNetSim/ship/ship.h
        ../ShipNetSim/ship/shipengine.h
        ../ShipNetSim/ship/shipfuel.h
        ../ShipNetSim/ship/shipgearbox.h
        ../ShipNetSim/ship/shippropeller.h
        ../ShipNetSim/ship/tank.h

        ../ShipNetSim/simulator.h

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
        ../ShipNetSim/network/point.cpp
        ../ShipNetSim/network/gpoint.cpp
        ../ShipNetSim/network/seaport.cpp
        ../ShipNetSim/network/line.cpp
        ../ShipNetSim/network/gline.cpp
        ../ShipNetSim/network/polygon.cpp
        ../ShipNetSim/network/algebraicvector.cpp
        ../ShipNetSim/network/galgebraicvector.cpp
        ../ShipNetSim/network/optimizedvisibilitygraph.cpp
        ../ShipNetSim/network/quadtree.cpp
        ../ShipNetSim/network/optimizednetwork.cpp
        ../ShipNetSim/utils/data.cpp

        ../ShipNetSim/ship/battery.cpp
        ../ShipNetSim/ship/holtropmethod.cpp
        ../ShipNetSim/ship/ienergyconsumer.cpp
        ../ShipNetSim/ship/ishipengine.cpp
        ../ShipNetSim/ship/ishipgearbox.cpp
        ../ShipNetSim/ship/ishippropeller.cpp
        ../ShipNetSim/ship/langmaomethod.cpp
        ../ShipNetSim/ship/ship.cpp
        ../ShipNetSim/ship/shipengine.cpp
        ../ShipNetSim/ship/shipfuel.cpp
        ../ShipNetSim/ship/shipgearbox.cpp
        ../ShipNetSim/ship/shippropeller.cpp
        ../ShipNetSim/ship/tank.cpp

        ../ShipNetSim/simulator.cpp

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
        utils/errorhandler.cpp

        ../third_party/qcustomplot/qcustomplot.cpp

        main.cpp
)

# set configuration file
configure_file(VersionConfig.h.in VersionConfig.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ShipNetSimGUI
        MANUAL_FINALIZATION

        ${PROJECT_H_FILES}
        ${PROJECT_UI_FILES}
        ${PROJECT_SOURCES}
        gui/media/src.qrc







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
