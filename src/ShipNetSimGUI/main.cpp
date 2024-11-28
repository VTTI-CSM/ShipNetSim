// #include "shipnetsim.h"

#include "gui/windowMangement/shipnetsim.h"
#include "gui/windowMangement/ui_shipnetsim.h"
#include <osgQOpenGL/osgQOpenGLWidget.h>

#include <osgDB/ReadFile>
#include <osgUtil/Optimizer>
#include <osg/CoordinateSystemNode>

#include <osg/Switch>
#include <osg/Types>
#include <osgText/Text>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <osgGA/TrackballManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgGA/StateSetManipulator>
#include <osgGA/AnimationPathManipulator>
#include <osgGA/TerrainManipulator>
#include <osgGA/SphericalManipulator>
#include <osgGA/Device>

#include <osgEarth/Notify>
#include <osgEarth/MapNode>
#include <osgEarth/Utils>
#include <osgEarth/Registry>
#include <osgEarth/Metrics>
// #include <osgEarth/ExampleResources>

#include <QApplication>
#include <QSurfaceFormat>
#include <QWindow>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <iostream>

#include "./gui/components/globalmapmanager.h"


int main(int argc, char *argv[])
{
    osgEarth::initialize();

#ifdef _DEBUG
    osg::setNotifyLevel(osg::FATAL); //DEBUG_INFO
    osgEarth::setNotifyLevel(osg::FATAL); //DEBUG_INFO
#endif

    QApplication app(argc, argv);

    QSurfaceFormat format = QSurfaceFormat::defaultFormat();

    #ifdef OSG_GL3_AVAILABLE
        format.setVersion(3, 2);
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setRenderableType(QSurfaceFormat::OpenGL);
        format.setOption(QSurfaceFormat::DebugContext);
    #else
        format.setVersion(2, 0);
        format.setProfile(QSurfaceFormat::CompatibilityProfile);
        format.setRenderableType(QSurfaceFormat::OpenGL);
        format.setOption(QSurfaceFormat::DebugContext);
    #endif

    format.setDepthBufferSize(24);
    format.setSamples(8);
    format.setStencilBufferSize(8);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    QSurfaceFormat::setDefaultFormat(format);

    GlobalMapManager::getInstance()->preloadEarthModel();

    ShipNetSim w;

    // auto earthResults = w.ui->plot_trains->loadMap();

    w.show();

    // w.ui->plot_trains->setMapNode(earthResults.second);

    return app.exec();
}
