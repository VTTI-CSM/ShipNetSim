#include "osgearthqtwidget.h"
#include <QRegularExpression>

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
#include <osgEarth/EarthManipulator>
#include <osgEarth/CachePolicy>
#include <osgEarth/MapNode>
#include <osgEarth/GLUtils>
#include <osgEarth/ObjectIndex>

// For convenience, use osgEarth::Util namespace for EarthManipulator
using osgEarth::Util::EarthManipulator;
#include <QToolTip>
#include <qcombobox.h>
#include <qstackedwidget.h>
#include <qtablewidget.h>
#include <qtabwidget.h>


#include "globalmapmanager.h"
#include "gui/components/customtablewidget.h"

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

    QObject::connect(this, &osgQOpenGLWidget::initialized, [this, &parent]
                     {

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

        // Apply GL3 settings required by osgEarth 3.x.
        // This enables vertex attribute aliasing and matrix uniforms
        // which are needed for modern GLSL shaders.
        osg::GraphicsContext* gc = getOsgViewer()->getCamera()->getGraphicsContext();
        if (gc)
        {
            osg::State* state = gc->getState();
            if (state)
            {
                state->resetVertexAttributeAlias(false);
                state->setUseModelViewAndProjectionUniforms(true);
                state->setUseVertexAttributeAliasing(true);
            }
        }

        auto m = new EarthManipulator();
        auto de = m->getSettings();
        de->bindScroll(osgEarth::Util::EarthManipulator::ACTION_ZOOM_IN,
                       osgGA::GUIEventAdapter::SCROLL_UP);
        de->bindScroll(osgEarth::Util::EarthManipulator::ACTION_ZOOM_OUT,
                       osgGA::GUIEventAdapter::SCROLL_DOWN);
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
            manager->preloadModelData();
            setMapNode(preloadedMapRoot);
        }

        if (!preloadedMapRoot) {
            qFatal("Preloaded map root is null.");
            return;
        }

        osgEarth::ObjectIDPicker* picker = new osgEarth::ObjectIDPicker();
        if (!picker) {
            qFatal("Failed to create ObjectIDPicker.");
            return;
        }

        qDebug() << "Setting picker view and graph.";
        picker->setView(getOsgViewer());
        picker->setGraph(manager->getMapNode().get());
        manager->getMapNode()->addChild(picker);
        qDebug() << "Picker added to scene graph.";

        picker->onClick([&](const osgEarth::ObjectID& id)
        {
            if (id == (osgEarth::ObjectID)0)
            {
                return;
            }

            auto place = osgEarth::Registry::objectIndex()->get<osgEarth::AnnotationNode>(id);
            if (!place)
            {
                return;
            }

            GlobalMapManager::getInstance()->toggleHighlightNode(id);

            // Retrieve custom data
            auto customData =
                dynamic_cast<GlobalMapManager::CustomData<
                std::shared_ptr<ShipNetSimCore::SeaPort>>*>(
                place->getUserData());

            if (!customData)
            {
                return;
            }

            // retreive the port obj
            std::shared_ptr<ShipNetSimCore::SeaPort> seaPortPtr =
                customData->getData();

            // check if the parent is the path tab
            // not the simulation tab
            QWidget* parent = parentWidget();
            if (!parent || parent->objectName() != "tab_path") {
                return;
            }

            // Go up three levels to reach tabWidget_newTrainOD
            QStackedWidget* stacked =
                qobject_cast<QStackedWidget*>(parent->parentWidget());
            if (!stacked) {
                return;
            }
            QWidget* grandParent = stacked->parentWidget();
            QTabWidget* tabWidget =
                qobject_cast<QTabWidget*>(grandParent);
            if (!tabWidget ||
                tabWidget->objectName() != "tabWidget_newTrainOD") {
                return;
            }

            QString portCoords =
                seaPortPtr->getPortCoordinate().
                toString("%x,%y").remove(" ");

            // combobox that holds current ships
            QComboBox* selector =
                parent->findChild<QComboBox*>("combo_visualizeShip");
            if (!selector) {
                return;
            }

            // Get the selected ship id
            QString currentShip =
                selector->currentText().trimmed();

            // Get main tab that hosts the table
            QWidget* firstTab = tabWidget->widget(0);
            if (!firstTab) {
                return;
            }

            // Get the table by its name
            CustomTableWidget* table =
                firstTab->findChild<CustomTableWidget*>("table_newShips");
            if (!table) {
                return;
            }

            // get rows that hold the ship ID
            auto currentShipRows =
                table->findRowsWithData(currentShip, 0);
            if (currentShipRows.isEmpty()) {
                return;
            }

            // get table item that holds the path
            QTableWidgetItem* item = table->item(currentShipRows[0], 1);
            if (!item) {
                // Create new item with empty text
                item = new QTableWidgetItem("");
                table->setItem(currentShipRows[0], 1, item);
            }

            QString newText = item->text().remove(" ");
            if (newText.contains(portCoords)) {
                // Remove the coordinates
                if (newText == portCoords) {
                    // If it's the only coordinate, clear the text
                    newText.clear();
                } else {
                    // Remove the coordinate and any leading/trailing semicolon
                    newText.remove(portCoords);
                    newText.remove(QRegularExpression("^;|;$")); // Remove semicolon at start or end
                    newText.replace(";;", ";"); // Fix any double semicolons
                }
            } else {
                // Add the coordinates
                newText = newText.isEmpty() ? portCoords :
                              newText + ";" + portCoords;
            }
            item->setText(newText);


            // std::cout << "Custom data retrieved successfully."
            //           << std::endl;
            // std::cout << "From Country: \""
            //           << seaPortPtr->getCountryName().toStdString()
            //           << "\"" << std::endl;

            // GlobalMapManager::getInstance()->createShipNode(
            //     "1",
            //     seaPortPtr->getPortCoordinate());


        });


        picker->onHover([&](const osgEarth::ObjectID& id) {
            if (id != (osgEarth::ObjectID)0)
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

        qDebug() << "Picker onClick function connected.";
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
            unsigned int optimizations =
                osgUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS |
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
