#include "osgwidget.h"
#include <QMouseEvent>
#include <osgGA/GUIEventAdapter>
#include <QTimer>

OSGWidget::OSGWidget(QWidget* parent)
    : QOpenGLWidget(parent), rootNode(new osg::Group())
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    adjustFrameRate();
}

OSGWidget::~OSGWidget()
{
    if(timerId != 0) {
        killTimer(timerId);
    }
}

void OSGWidget::setupEarthManipulator()
{
    if (!viewer.getCameraManipulator())
    {
        osg::ref_ptr<osgEarth::Util::EarthManipulator> manipulator = new osgEarth::Util::EarthManipulator;
        viewer.setCameraManipulator(manipulator);
    }
}

void OSGWidget::initializeGL()
{
    qDebug() << "Initializing GL";

    // Set the display settings for multisampling
    osg::DisplaySettings::instance()->setNumMultiSamples(4);

    // Configure the traits for the graphics context
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->x = 0;
    traits->y = 0;
    traits->width = this->width();
    traits->height = this->height();
    traits->windowDecoration = false;
    traits->doubleBuffer = true;
    traits->sharedContext = 0;

    // Create the graphics context based on the traits
    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    osg::ref_ptr<osg::Camera> cam = nullptr;
    if (gc.valid())
    {
        cam = viewer.getCamera();
        cam->setGraphicsContext(gc.get());
        cam->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));
        GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;
        cam->setDrawBuffer(buffer);
        cam->setReadBuffer(buffer);
    }
    else
    {
        qWarning() << "Failed to create a valid graphics context for the viewer.";
        return;
    }

    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.setSceneData(rootNode.get()); // Assuming rootNode is already initialized

    // Set an initial manipulator
    viewer.setCameraManipulator(new osgGA::TrackballManipulator());

    this->camera = cam;
}

void OSGWidget::showEvent(QShowEvent* event)
{
    QOpenGLWidget::showEvent(event); // Call base implementation

    // Setup EarthManipulator here
    setupEarthManipulator();

    // Optionally, check if the event has been handled
    if (!event->isAccepted())
        event->accept();
}

void OSGWidget::paintGL()
{
    viewer.frame();
}

void OSGWidget::resizeGL(int width, int height)
{
    auto* eventQueue = viewer.getEventQueue();
    if (eventQueue) {
        eventQueue->windowResize(this->x(), this->y(), width, height);
    }
    viewer.getCamera()->setViewport(new osg::Viewport(0, 0, width, height));
}

void OSGWidget::timerEvent(QTimerEvent* /*event*/)
{
    update();
}

void OSGWidget::setModel(osg::Node* model)
{
    if (!model) return;

    // Clear existing models and add the new one
    rootNode->removeChildren(0, rootNode->getNumChildren());
    rootNode->addChild(model);
}

void OSGWidget::appendModel(osg::Node* model)
{
    if (!model) return;

    rootNode->addChild(model);

}

void OSGWidget::resetCameraToHomePosition()
{
    viewer.getCameraManipulator()->home(0.0);
}

void OSGWidget::saveCameraState()
{
    savedCameraMatrix = camera->getViewMatrix();
}

void OSGWidget::restoreCameraState()
{
    viewer.getCameraManipulator()->setByMatrix(savedCameraMatrix);
}

void OSGWidget::setFrameRate(unsigned int fps)
{
    targetFrameRate = fps;
    adjustFrameRate();
}

void OSGWidget::takeScreenshot(const QString& filePath)
{
    osg::ref_ptr<osgViewer::ScreenCaptureHandler> capture =
        new osgViewer::ScreenCaptureHandler(
            new osgViewer::ScreenCaptureHandler::
            WriteToFile(filePath.toStdString(), "jpg"));
    viewer.addEventHandler(capture);
    capture->captureNextFrame(viewer);
}

void OSGWidget::adjustFrameRate()
{
    if(timerId != 0) {
        killTimer(timerId);
    }
    if(targetFrameRate > 0) {
        timerId = startTimer(1000 / targetFrameRate);
    }
}

bool OSGWidget::removeModelByIndex(unsigned int index)
{
    if (index >= rootNode->getNumChildren()) {
        std::cerr << "Index out of range for model removal." << std::endl;
        return false;
    }

    // Remove the child node at the specified index
    bool removed = rootNode->removeChildren(index, 1);
    if (!removed) {
        std::cerr << "Failed to remove model by index." << std::endl;
    }

    return removed;
}

unsigned int OSGWidget::convertMouseButton(Qt::MouseButton button)
{
    switch (button)
    {
    case Qt::LeftButton:
        return osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON;
    case Qt::MiddleButton:
        return osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON;
    case Qt::RightButton:
        return osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON;
    default:
        return 0;
    }
}

void OSGWidget::mousePressEvent(QMouseEvent* event)
{
    setKeyboardModifiers(event);
    viewer.getEventQueue()->mouseButtonPress(
        event->x(), event->y(),
        convertMouseButton(event->button()));
}

void OSGWidget::mouseReleaseEvent(QMouseEvent* event)
{
    setKeyboardModifiers(event);
    viewer.getEventQueue()->mouseButtonRelease(
        event->x(), event->y(),
        convertMouseButton(event->button()));
}

void OSGWidget::mouseMoveEvent(QMouseEvent* event)
{
    setKeyboardModifiers(event);
    viewer.getEventQueue()->mouseMotion(event->x(), event->y());
}

void OSGWidget::setKeyboardModifiers(QInputEvent* event)
{
    unsigned int modKeyMask = 0;
    if (event->modifiers() & Qt::ShiftModifier)
        modKeyMask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
    if (event->modifiers() & Qt::ControlModifier)
        modKeyMask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
    if (event->modifiers() & Qt::AltModifier)
        modKeyMask |= osgGA::GUIEventAdapter::MODKEY_ALT;
    viewer.getEventQueue()->getCurrentEventState()->setModKeyMask(modKeyMask);
}
