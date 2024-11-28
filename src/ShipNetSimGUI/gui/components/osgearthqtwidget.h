#ifndef OSGEARTHQTWIDGET_H
#define OSGEARTHQTWIDGET_H

#include "osgEarth/AnnotationNode"
#include "qcoreapplication.h"
#include <osgQOpenGL/osgQOpenGLWidget.h>
#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgGA/Device>
#include <osgViewer/ViewerEventHandlers>
#include <osg/ArgumentParser>

// OSGEarth includes
#include <osgEarth/Map>
#include <osgEarth/MapNode>
#include <osgEarthDrivers/engine_rex/RexTerrainEngineNode>
#include <osgEarth/EarthManipulator>
#include <osgEarth/Viewpoint>
#include <osgEarth/AutoClipPlaneHandler>
#include <osgEarth/Feature>
#include <osgEarth/ObjectIDPicker>
#include <osgEarth/Feature>
#include <osgEarth/FeatureIndex>

class OSGEarthQtWidget : public osgQOpenGLWidget
{
    Q_OBJECT

public:
    explicit OSGEarthQtWidget(QWidget *parent = 0);
    ~OSGEarthQtWidget() override;
    void setMapNode(osg::ref_ptr<osg::Group> root);
    void addDefaultPorts();
private:
    osg::ref_ptr<AnnotationNode> _pickedAnno;
    void configureView( osgViewer::View* view ) const;
    void printSceneGraph(const osg::Node* node, int level = 0);
signals:

public slots:
};

#endif // OSGEARTHQTWIDGET_H
