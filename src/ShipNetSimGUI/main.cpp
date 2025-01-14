// #include "shipnetsim.h"

#include "gui/windowMangement/shipnetsim.h"
#include "../ShipNetSimCore/utils/utils.h"
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

#include <QApplication>
#include <QSurfaceFormat>
#include <QWindow>
#include <qsplashscreen.h>

#include "utils/logger.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif


#include "./gui/components/globalmapmanager.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);

    // Create application first
    QApplication app(argc, argv);

    // Attach the logger first thing:
    ShipNetSimCore::Logger::attach("ShipNetSimGUI");

    // Create and show splash immediately
    QString splashPath =
        ShipNetSimCore::Utils::getDataFile("ShipNetSimSplash.png");
    QPixmap originalPix(splashPath);
    QPixmap scaledPix = originalPix.scaled(originalPix.width() * 0.5,
                                           originalPix.height() * 0.5,
                                           Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
    QSplashScreen splash(scaledPix);
    splash.setWindowFlags(Qt::WindowStaysOnTopHint | splash.windowFlags());
    splash.show();
    app.processEvents();

    // Initialize osgEarth after splash is shown
    splash.showMessage("Initializing OSG...",
                       Qt::AlignBottom | Qt::AlignCenter, Qt::white);
    app.processEvents();

    osgEarth::initialize();

// #ifdef _DEBUG
    osg::setNotifyLevel(osg::FATAL);
    osgEarth::setNotifyLevel(osg::FATAL);
// #endif

    // Set up OpenGL format
    splash.showMessage("Setting up OpenGL...",
                       Qt::AlignBottom | Qt::AlignCenter, Qt::white);
    app.processEvents();

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

    // Preload model data
    splash.showMessage("Loading model data...",
                       Qt::AlignBottom | Qt::AlignCenter, Qt::white);
    app.processEvents();
    GlobalMapManager::getInstance()->preloadModelData();

    // Create main window
    splash.showMessage("Initializing main window...",
                       Qt::AlignBottom | Qt::AlignCenter, Qt::white);
    app.processEvents();
    ShipNetSim w;

    // Show main window and finish splash
    w.show();
    splash.finish(&w);

    // Detach logger and start event loop.
    ShipNetSimCore::Logger::detach();

    return app.exec();
}
