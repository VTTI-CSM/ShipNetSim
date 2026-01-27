#ifndef OSGWIDGET_H
#define OSGWIDGET_H

#include <QObject>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QTimerEvent>

#include <osgViewer/Viewer>
#include <osg/Group>
#include <osg/Node>
#include <osgGA/GUIEventAdapter>
#include <osgGA/TrackballManipulator>
#include <osgDB/WriteFile>
#include <osgViewer/ViewerEventHandlers>

#include <osgQOpenGL/osgQOpenGLWidget.h>

// #include <OSGWidget.h>

#include <osgEarth/MapNode>
#include <osgEarth/EarthManipulator>




class OSGWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit OSGWidget(QWidget* parent = nullptr);
    virtual ~OSGWidget();

    void setModel(osg::Node* model);
    void appendModel(osg::Node* model);
    void resetCameraToHomePosition();
    void saveCameraState();
    void restoreCameraState();
    void setFrameRate(unsigned int fps);
    void takeScreenshot(const QString& filePath);
    bool removeModelByIndex(unsigned int index);
    void setupEarthManipulator();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void timerEvent(QTimerEvent* event) override;
    void showEvent(QShowEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    osgViewer::Viewer viewer;
    osg::ref_ptr<osg::Group> rootNode;
    osg::ref_ptr<osg::Camera> camera;
    osg::Matrixd savedCameraMatrix;
    int timerId = 0;
    unsigned int targetFrameRate = 60;

    void setKeyboardModifiers(QInputEvent* event);
    void adjustFrameRate();
    unsigned int convertMouseButton(Qt::MouseButton button);
};

#endif // OSGWIDGET_H
