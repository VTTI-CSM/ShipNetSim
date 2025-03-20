#include "osgearthqtwidget.h"

#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osgUtil/Optimizer>
#include <osgGA/TrackballManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/TerrainManipulator>
#include <osgGA/OrbitManipulator>
#include <osgGA/FirstPersonManipulator>
#include <osgGA/SphericalManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgViewer/ViewerEventHandlers>

#include <QKeyEvent>
#include <network/optimizednetwork.h>

#include <osgEarth/Utils>
#include <osgEarth/Registry>
#include <osgEarth/Metrics>
#include <osgEarth/ExampleResources>
#include <osgEarth/PlaceNode>
#include <osgEarth/Style>
#include <osgEarth/Feature>
#include <osgEarth/ObjectIDPicker>
#include <osgEarth/Feature>
#include <osgEarth/FeatureIndex>
#include <osgEarth/GLUtils>
#include <osgEarth/ObjectIndex>


#include <OpenGL/gl3.h>

#include <QToolTip>


#include "globalmapmanager.h"

namespace
{
struct MultiRealizeOperation : public osg::Operation
{
    void operator()(osg::Object* obj) override
    {
        for (auto& op : _ops)
            op->operator()(obj);
    }
    std::vector<osg::ref_ptr<osg::Operation>> _ops;
};
}

OSGEarthQtWidget::OSGEarthQtWidget(QWidget *parent)
    : osgQOpenGLWidget(parent) {

    QObject::connect(this, &osgQOpenGLWidget::initialized, [this]
                     {

        std::cout << "Widget initialized." << std::endl;

        // This is normally called by Viewer::run but we are
        // running our frame loop manually so we need to call it here.
        getOsgViewer()->setReleaseContextAtEndOfFrameHint(false);

        // Tell the database pager to not modify the unref settings
        getOsgViewer()->getDatabasePager()->
            setUnrefImageDataAfterApplyPolicy( true, false );

        // thread-safe initialization of the OSG wrapper manager.
        // Calling this here prevents the "unsupported wrapper"
        // messages from OSG
        osgDB::Registry::instance()->getObjectWrapperManager()->
            findWrapper("osg::Image");

        auto m = new osgEarth::EarthManipulator();
        auto de = m->getSettings();
        de->bindScroll(osgEarth::Util::EarthManipulator::ACTION_ZOOM_IN, osgGA::GUIEventAdapter::SCROLL_UP);
        de->bindScroll(osgEarth::Util::EarthManipulator::ACTION_ZOOM_OUT, osgGA::GUIEventAdapter::SCROLL_DOWN);
        // install our default manipulator (do this before calling load)
        getOsgViewer()->setCameraManipulator( m );

        // disable the small-feature culling
        getOsgViewer()->getCamera()->setSmallFeatureCullingPixelSize(-1.0f);

        // no caching
        osgEarth::Registry::instance()->setOverrideCachePolicy(osgEarth::CachePolicy::NO_CACHE);

        // collect the views
        osgViewer::Viewer::Views views;
        if (getOsgViewer())
        {
            getOsgViewer()->getViews(views);
        }

        // configures each view with some stock goodies
        for (auto view : views)
        {
            configureView(view);
        }


        if (getOsgViewer())
        {
            // MultiRealizeOperation* op = new MultiRealizeOperation();

            // if (getOsgViewer()->getRealizeOperation())
            // {
            //     op->_ops.push_back(getOsgViewer()->getRealizeOperation());
            // }

            // getOsgViewer()->setRealizeOperation(op);
        }

        // load the earth data
        auto manager = GlobalMapManager::getInstance();

        osg::ref_ptr<osg::Group> preloadedMapRoot =
            manager->getRootGroup();

        // if the map is already loaded, do not load again.
        if (preloadedMapRoot) {
            setMapNode(preloadedMapRoot);
        }
        else {
            // Load the Earth model normally if not preloaded
            manager->preloadEarthModel();
            setMapNode(preloadedMapRoot);
        }

        if (!preloadedMapRoot) {
            std::cerr << "Preloaded map root is null." << std::endl;
            return;
        }

        osgEarth::ObjectIDPicker* picker = new osgEarth::ObjectIDPicker();
        if (!picker) {
            std::cerr << "Failed to create ObjectIDPicker." << std::endl;
            return;
        }

        std::cout << "Setting picker view and graph." << std::endl;
        picker->setView(getOsgViewer());
        picker->setGraph(manager->getMapNode().get());
        manager->getMapNode()->addChild(picker);
        std::cout << "Picker added to scene graph." << std::endl;

        picker->onClick([&](const osgEarth::ObjectID& id)
        {
            osgEarth::ObjectID nullID = (osgEarth::ObjectID)0;

            if (id != nullID)
            {
                auto place = osgEarth::Registry::objectIndex()->get<osgEarth::AnnotationNode>(id);
                if (place)
                {
                    std::cout << "Clicked on \"" << place->getText() << "\""
                              << std::endl;

                    // Retrieve custom data
                    auto customData =
                        dynamic_cast<GlobalMapManager::CustomData<
                        std::shared_ptr<ShipNetSimCore::SeaPort>>*>(
                        place->getUserData());

                    if (customData)
                    {
                        std::shared_ptr<ShipNetSimCore::SeaPort> seaPortPtr =
                            customData->getData();
                        std::cout << "Custom data retrieved successfully."
                                  << std::endl;
                        std::cout << "From Country: \""
                                  << seaPortPtr->getCountryName().toStdString()
                                  << "\"" << std::endl;

                    }
                }
            }
        });


        picker->onHover([&](const osgEarth::ObjectID& id) {
            osgEarth::ObjectID nullID = (osgEarth::ObjectID)0;
            if (id != nullID)
            {
                auto place = osgEarth::Registry::objectIndex()->get<osgEarth::AnnotationNode>(id);
                if (place)
                {
                    // Retrieve custom data
                    auto customData =
                        dynamic_cast<GlobalMapManager::CustomData<
                            std::shared_ptr<ShipNetSimCore::SeaPort>>*>(
                            place->getUserData());

                    if (customData)
                    {
                        std::shared_ptr<ShipNetSimCore::SeaPort> seaPortPtr =
                            customData->getData();

                        auto p = QCursor::pos();
                        QString text =
                            QString(
                                "<html>"
                                "<head/>"
                                "<body>"
                                "<div style='width: 300px; "
                                           "font-family: Arial, sans-serif; "
                                           "font-size: 12px;'>"
                                "<p><strong>Port:</strong> %1 (%2)<br/>"
                                "<strong>Country:</strong> %3</p>"
                                "<p><strong>Has Rail Terminal:</strong> %4<br/>"
                                "<strong>Has Road Terminal:</strong> %5<br/>"
                                "<strong>Status of Entry:</strong> %6</p>"
                                "</div>"
                                "</body>"
                                "</html>"
                                )
                                .arg(seaPortPtr->getPortName())
                                .arg(seaPortPtr->getPortCode())
                                .arg(seaPortPtr->getCountryName())
                                .arg(seaPortPtr->getHasRailTerminal() ?
                                                    "Yes" : "No")
                                .arg(seaPortPtr->getHasRoadTerminal() ?
                                                    "Yes" : "No")
                                .arg(seaPortPtr->getStatusOfEntry());

                        QToolTip::showText(p, text, this);

                    }
                }


            }
            else {
                QToolTip::hideText();
            }
        });

        std::cout << "Picker onClick function connected." << std::endl;
    });


}

OSGEarthQtWidget::~OSGEarthQtWidget() {
    if (parent()) {
        setParent(nullptr); // Detach from parent to prevent double deletion
    }
}


void OSGEarthQtWidget::setMapNode(osg::ref_ptr<osg::Group> root)
{
    if (root.valid())
    {
        if (osgEarth::MapNode::get(root))
        {
            getOsgViewer()->setSceneData(root);

            // Create an optimizer instance
            osgUtil::Optimizer optimizer;

            // Specify optimizations using flags
            unsigned int optimizations = osgUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS |
                                         osgUtil::Optimizer::SPATIALIZE_GROUPS;

            // Apply optimizations to your scene graph
            optimizer.optimize(root.get());

            // printSceneGraph(root.get());
        }
    }
}

void OSGEarthQtWidget::addDefaultPorts() {
    auto manager = GlobalMapManager::getInstance();
    manager->addSeaPort();
}

void OSGEarthQtWidget::printSceneGraph(const osg::Node* node, int level) {
    if (!node) return; // Safety check

    // Create an indentation string based on the current level
    std::string indent(level * 2, ' ');

    // Print the current node's name and class type
    std::cout << indent << node->className() << ": " << node->getName() << std::endl;

    // If the node is a Group, recursively print its children
    const osg::Group* group = node->asGroup();
    if (group) {
        for (unsigned int i = 0; i < group->getNumChildren(); ++i) {
            printSceneGraph(group->getChild(i), level + 1); // Recurse into each child
        }
    }
}


void
OSGEarthQtWidget::configureView( osgViewer::View* view ) const
{
    // default uniform values:
    osgEarth::GLUtils::setGlobalDefaults(view->getCamera()->getOrCreateStateSet());

    // disable small feature culling (otherwise Text annotations won't render)
    view->getCamera()->setSmallFeatureCullingPixelSize(-1.0f);

    // thread-safe initialization of the OSG wrapper manager. Calling this here
    // prevents the "unsupported wrapper" messages from OSG
    osgDB::Registry::instance()->getObjectWrapperManager()->
        findWrapper("osg::Image");

    // add some stock OSG handlers:
    view->addEventHandler(new osgViewer::StatsHandler());
    view->addEventHandler(new osgViewer::WindowSizeHandler());
    view->addEventHandler(new osgViewer::ThreadingHandler());
    view->addEventHandler(new osgViewer::LODScaleHandler());
    view->addEventHandler(new osgGA::StateSetManipulator(
        view->getCamera()->getOrCreateStateSet()));
    view->addEventHandler(new osgViewer::RecordCameraPathHandler());
    view->addEventHandler(new osgViewer::ScreenCaptureHandler());


    // PortClickHandler* clickHandler = PortClickHandler::getInstance();
    // view->addEventHandler(clickHandler);

}
