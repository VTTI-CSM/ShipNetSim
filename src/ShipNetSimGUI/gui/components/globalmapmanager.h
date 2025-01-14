#ifndef GLOBALMAPMANAGER_H
#define GLOBALMAPMANAGER_H

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
#include <osgEarth/Registry>
#include <osgEarth/AnnotationNode>
#include <osgEarth/AnnotationData>
#include <osgEarth/ModelNode>
#include <QString>
#include "../../utils/defaults.h"
#include "osgEarth/ObjectIndex"
#include "ship/ship.h"
#include "utils/utils.h"
#include "network/seaport.h"
#include "network/gpoint.h"

class GlobalMapManager
{

private:
    // std::unordered_map<std::string, osgEarth::GeoTransform*> _shipTransforms;
    // osg::ref_ptr<osg::Node> _shipModelPrototype;

    std::unordered_map<osgEarth::ObjectID, osgEarth::PlaceNode*> portNodes;
    std::unordered_map<QString, osg::Node*> _shipTransforms;
    std::unordered_map<QString, osg::ref_ptr<osg::MatrixTransform>> _shipPaths;
    osgEarth::Style _shipStyle;
    osg::ref_ptr<osg::Image> _normalImage;
    osg::ref_ptr<osg::Image> _highlightImage;
    osg::ref_ptr<osg::Image> _tempImage;
    // osgEarth::Style _normalStyle;
    // osgEarth::Style _highlightStyle;
    std::set<osgEarth::ObjectID> _highlightedNodes;

    void updateShipPath(const QString& shipId,
                        const QVector<std::shared_ptr<
                            ShipNetSimCore::GLine>>& paths)
    {
        // Remove old path if exists
        auto it = _shipPaths.find(shipId);
        if (it != _shipPaths.end() && _mapNode.valid()) {
            _mapNode->removeChild(it->second);
        }

        if (paths.isEmpty()) return;

        // Create geometry
        osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;

        // Add points
        for (const auto& line : paths) {
            vertices->push_back(osg::Vec3(
                line->startPoint()->getLongitude().value(),
                line->startPoint()->getLatitude().value(),
                0.0f));

            if (line == paths.last()) {
                vertices->push_back(osg::Vec3(
                    line->endPoint()->getLongitude().value(),
                    line->endPoint()->getLatitude().value(),
                    0.0f));
            }
        }

        // Set color (blue)
        colors->push_back(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));

        geometry->setVertexArray(vertices.get());
        geometry->setColorArray(colors.get());
        geometry->setColorBinding(osg::Geometry::BIND_OVERALL);

        // Set line width using OpenGL directly
        geometry->getOrCreateStateSet()->setMode(GL_LINE_SMOOTH,
                                                 osg::StateAttribute::ON);
        glLineWidth(2.0f);

        geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0,
                                                      vertices->size()));

        // Create transform to hold the geometry
        osg::ref_ptr<osg::MatrixTransform> pathNode = new osg::MatrixTransform;
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(geometry.get());
        pathNode->addChild(geode);

        _mapNode->addChild(pathNode);
        _shipPaths[shipId] = pathNode;
    }

public:
    static GlobalMapManager* getInstance() {
        static GlobalMapManager instance;
        return &instance;
    }

    template <typename T>
    class CustomData : public osg::Referenced
    {
    public:
        CustomData(const T& data) : _data(data) {}

        const T& getData() const { return _data; }

    private:
        T _data;
    };

    void toggleHighlightNode(const osgEarth::ObjectID& id) {
        auto node =
            osgEarth::Registry::objectIndex()->get<osgEarth::PlaceNode>(id);
        if (!node) return;

        auto it = _highlightedNodes.find(id);

        if (it != _highlightedNodes.end()) {
            if (!_normalImage->valid()) {
                std::cout << "Normal image is invalid!" << std::endl;
                return;
            }
            node->setIconImage(_normalImage);
            _highlightedNodes.erase(it);
        }
        else
        {
            if (!_highlightImage->valid()) {
                std::cout << "Highlight image is invalid!" << std::endl;
                return;
            }
            node->setIconImage(_highlightImage);
            _highlightedNodes.insert(id);
        }
    }

    bool toggleHighlightNode(const ShipNetSimCore::GPoint& coordinate) {
        // Find the node at this coordinate
        // Small threshold for floating point comparison
        double epsilon = 0.01;

        for (const auto& [id, node] : portNodes) {
            osgEarth::GeoPoint nodePos = node->getPosition();

            if (std::abs(nodePos.x() -
                         coordinate.getLongitude().value()) < epsilon &&
                std::abs(nodePos.y() -
                         coordinate.getLatitude().value()) < epsilon)
            {
                toggleHighlightNode(id);
                return true;
            }
        }
        return false;
    }

    void clearAllHighlights() {
        // Create a temp copy since we'll be modifying the set while iterating
        std::set<osgEarth::ObjectID> highlightedCopy = _highlightedNodes;

        // Reset each highlighted node to normal icon
        for (const auto& id : highlightedCopy) {
            toggleHighlightNode(id);
        }

        // Clear the set (should already be empty from toggles)
        _highlightedNodes.clear();
    }

    void preloadModelData() {
        if (!_mapNode) { // Prevent reloading if already loaded
            // get the earth file path
            auto dataDir = ShipNetSimCore::Utils::getDataDirectory();
            QString earthModelPath =
                ShipNetSimCore::Utils::getFirstExistingPathFromList(
                    Defaults::getEarthTifPaths(dataDir));

            if (earthModelPath.isEmpty()) {
                qDebug() << "Error: Could not find earth model file";
                return;
            }

            // load icons
            {
                auto dataDir = ShipNetSimCore::Utils::getDataDirectory();
                std::string normalIcon =
                    ShipNetSimCore::Utils::getFirstExistingPathFromList(
                        Defaults::getIconPaths(dataDir)).toStdString();
                std::string highlightedIcon =
                    ShipNetSimCore::Utils::getFirstExistingPathFromList(
                        Defaults::getHighlightedIconPaths(dataDir)).
                    toStdString();
                std::string tempIcon =
                    ShipNetSimCore::Utils::getFirstExistingPathFromList(
                        Defaults::getTemporaryIconPaths(dataDir)).
                    toStdString();

                _normalImage = osgDB::readImageFile(normalIcon);
                _highlightImage = osgDB::readImageFile(highlightedIcon);
                _tempImage = osgDB::readRefImageFile(tempIcon);
            }

            loadEarthModel(earthModelPath);
        }

        // Load ship model if not already loaded
        {
            auto dataDir = ShipNetSimCore::Utils::getDataDirectory();
            std::string shipIcon =
                ShipNetSimCore::Utils::getFirstExistingPathFromList(
                    Defaults::getShipIconPaths(dataDir)).toStdString();

            _shipStyle.getOrCreate<osgEarth::IconSymbol>()->url()->
                setLiteral(shipIcon);
            _shipStyle.getOrCreate<osgEarth::TextSymbol>()->alignment() =
                osgEarth::TextSymbol::ALIGN_CENTER_CENTER;
            _shipStyle.getOrCreate<osgEarth::TextSymbol>()->fill()->color() =
                osgEarth::Color::White;
        }
        // if (!_shipModelPrototype.valid()) {
        //     auto dataDir = ShipNetSimCore::Utils::getDataDirectory();
        //     QString shipModelPath =
        //         ShipNetSimCore::Utils::getFirstExistingPathFromList(
        //             Defaults::getShipModelPaths(dataDir));

        //     if (shipModelPath.isEmpty()) {
        //         qDebug() << "Error: Could not find ship model file";
        //         return;
        //     }

        //     loadShipModel(shipModelPath);
        // }

    }


    osg::ref_ptr<osgEarth::MapNode> getMapNode() {
        return _mapNode;
    }

    osg::ref_ptr<osg::Group> getRootGroup() {
        return _root;
    }

    void loadEarthModel(const QString &filename)
    {
        // Check if the file exists using QFile
        if (!QFile::exists(filename)) {
            qFatal() << "Error: File does not exist - "
                      << filename.toStdString();
            return;
        }

        // read in the Earth file:
        osg::ref_ptr<osg::Node> node =
            osgDB::readRefNodeFile(filename.toStdString());
        if (!node.valid())
        {
            qDebug() << "Failed to load Earth model from: " << filename;
            return;
        }

        osg::Node* n = node.get();
        if (!n)
        {
            qDebug() << "Node not valid!";
            return;
        }

        _mapNode = osgEarth::MapNode::get(n);
        if (!_mapNode.valid())
        {
            qDebug() << "Loaded scene graph does not contain a MapNode";
            return;
        }

        // a root node to hold everything:
        _root->addChild(_mapNode);

        // open the map node:
        if (!_mapNode->open())
        {
            qDebug() << "Failed to open MapNode";
            return;
        }

        // load the ports
        addSeaPort();
    }

    // void loadShipModel(const QString &filename)
    // {
    //     // Check if the file exists using QFile
    //     if (!QFile::exists(filename)) {
    //         ErrorHandler::showError(
    //             QString("Error: File does not exist - %1").arg(filename));
    //         return;
    //     }

    //     if (!_mapNode.valid()) {
    //         ErrorHandler::showError(
    //             QString("Error: MapNode is not valid!"));
    //         return;
    //     }

    //     // Load the ship model
    //     _shipModelPrototype = osgDB::readNodeFile(filename.toStdString());
    //     if (!_shipModelPrototype.valid())
    //     {
    //         ErrorHandler::showWarning("Failed to load glTF model.");
    //         return;
    //     }

    //     // Create base style for the model
    //     osgEarth::Style style;
    //     style.getOrCreate<osgEarth::ModelSymbol>()->
    //         setModel(_shipModelPrototype.get());
    // }

    // osgEarth::GeoTransform* createShipNode(const std::string& shipId,
    //                                        double longitude,
    //                                        double latitude,
    //                                        double heading = 0.0)
    // {
    //     if (!_mapNode.valid() || !_shipModelPrototype.valid()) {
    //         qDebug() << "Error: MapNode or ship model not valid!";
    //         return nullptr;
    //     }

    //     // Create a new transform for the ship
    //     osgEarth::GeoTransform* transform = new osgEarth::GeoTransform();
    //     transform->setPosition(osgEarth::GeoPoint(
    //         _mapNode->getMapSRS(),
    //         longitude,
    //         latitude,
    //         0.0,  // altitude
    //         osgEarth::ALTMODE_RELATIVE));

    //     // Create a rotation transform for heading
    //     osg::MatrixTransform* rotationTransform = new osg::MatrixTransform();
    //     rotationTransform->setMatrix(osg::Matrix::rotate(
    //         osg::DegreesToRadians(heading),
    //         osg::Vec3d(0.0, 0.0, 1.0)));

    //     // Clone the ship model for this instance
    //     osg::ref_ptr<osg::Node> shipInstance =
    //         static_cast<osg::Node*>(_shipModelPrototype->
    //                                 clone(osg::CopyOp::DEEP_COPY_ALL));

    //     // Add the ship model to the rotation transform
    //     rotationTransform->addChild(shipInstance);

    //     // Add the rotation transform to the geo transform
    //     transform->addChild(rotationTransform);

    //     // Add the transform to the map
    //     _mapNode->addChild(transform);

    //     // Store the transform for later updates
    //     _shipTransforms[shipId] = transform;

    //     return transform;
    // }

    // void updateShipPosition(const std::string& shipId,
    //                         double longitude,
    //                         double latitude,
    //                         double heading)
    // {
    //     auto it = _shipTransforms.find(shipId);
    //     if (it == _shipTransforms.end()) {
    //         qDebug() << "Error: Ship"
    //                  << QString::fromStdString(shipId)
    //                  << "not found!";
    //         return;
    //     }

    //     osgEarth::GeoTransform* transform = it->second;
    //     if (!transform) return;

    //     // Update position
    //     transform->setPosition(osgEarth::GeoPoint(
    //         _mapNode->getMapSRS(),
    //         longitude,
    //         latitude,
    //         0.0,  // altitude
    //         osgEarth::ALTMODE_RELATIVE));

    //     // Update heading
    //     if (transform->getNumChildren() > 0) {
    //         osg::MatrixTransform* rotationTransform =
    //             dynamic_cast<osg::MatrixTransform*>(transform->getChild(0));
    //         if (rotationTransform) {
    //             rotationTransform->setMatrix(osg::Matrix::rotate(
    //                 osg::DegreesToRadians(heading),
    //                 osg::Vec3d(0.0, 0.0, 1.0)));
    //         }
    //     }
    // }

    osgEarth::PlaceNode* createShipNode(const QString& shipId,
                                        ShipNetSimCore::GPoint coordinate)
    {
        if (!_mapNode.valid()) {
            qDebug() << "Error: MapNode not valid!";
            return nullptr;
        }

        osgEarth::PlaceNode* shipNode = new osgEarth::PlaceNode(
            osgEarth::GeoPoint(
                _mapNode->getMapSRS(),
                coordinate.getLongitude().value(),
                coordinate.getLatitude().value(),
                0.0,
                osgEarth::ALTMODE_RELATIVE),
            shipId.toStdString(),  // Display ship ID as text
            _shipStyle);

        _mapNode->addChild(shipNode);
        _shipTransforms[shipId] = shipNode;

        return shipNode;
    }

    void updateShipPosition(const QString& shipId,
                            ShipNetSimCore::GPoint coordinate,
                            QVector<std::shared_ptr<
                                ShipNetSimCore::GLine>> paths = {})
    {
        auto it = _shipTransforms.find(shipId);
        if (it == _shipTransforms.end()) {
            qDebug() << "Error: Ship" << shipId << "not found!";
            return;
        }

        osgEarth::PlaceNode* shipNode =
            dynamic_cast<osgEarth::PlaceNode*>(it->second);
        if (!shipNode) return;

        shipNode->setPosition(osgEarth::GeoPoint(
            _mapNode->getMapSRS(),
            coordinate.getLongitude().value(),
            coordinate.getLatitude().value(),
            0.0,
            osgEarth::ALTMODE_RELATIVE));

        // Only update path if paths vector is not empty
        if (!paths.isEmpty()) {
            updateShipPath(shipId, paths);
        }
    }

    void updateShipPosition(const std::shared_ptr<ShipNetSimCore::Ship> ship) {
        updateShipPosition(ship->getUserID(),
                           ship->getCurrentPosition(),
                           *ship->getShipPathLines());
    }

    // A cleanup function to remove ships
    void removeShip(const QString& shipId)
    {
        auto it = _shipTransforms.find(shipId);
        if (it != _shipTransforms.end()) {
            if (it->second && _mapNode.valid()) {
                _mapNode->removeChild(it->second);
            }
            _shipTransforms.erase(it);
        }

        auto pathIt = _shipPaths.find(shipId);
        if (pathIt != _shipPaths.end()) {
            if (pathIt->second && _mapNode.valid()) {
                _mapNode->removeChild(pathIt->second);
            }
            _shipPaths.erase(pathIt);
        }
    }

    void clearAllShips() {
        // Remove all ship nodes from the map
        for (const auto& shipPair : _shipTransforms) {
            if (shipPair.second && _mapNode.valid()) {
                _mapNode->removeChild(shipPair.second);
            }
        }

        // Remove all path nodes
        for (const auto& pathPair : _shipPaths) {
            if (pathPair.second && _mapNode.valid()) {
                _mapNode->removeChild(pathPair.second);
            }
        }

        // Clear the map
        _shipTransforms.clear();
        _shipPaths.clear();
    }

    void addTemporaryPort(const ShipNetSimCore::GPoint& location,
                          const QString& portName)
    {
        if (!_mapNode.valid())
        {
            qDebug() << "MapNode is not valid!";
            return;
        }

        const osgEarth::SpatialReference* geoSRS =
            _mapNode->getMapSRS()->getGeographicSRS();

        // Style our labels
        osgEarth::Style labelStyle;
        labelStyle.getOrCreate<osgEarth::TextSymbol>()->alignment() =
            osgEarth::TextSymbol::ALIGN_CENTER_CENTER;
        labelStyle.getOrCreate<osgEarth::TextSymbol>()->fill()->color() =
            osgEarth::Color::Black;
        labelStyle.getOrCreate<osgEarth::TextSymbol>()->halo() =
            osgEarth::Color("#5f5f5f");

        // Style for place node marker - using same icon as regular ports
        osgEarth::Style pm;
        pm.getOrCreate<osgEarth::IconSymbol>()->setImage(_normalImage);
        pm.getOrCreate<osgEarth::IconSymbol>()->declutter() = true;

        // Create temporary port annotation
        osgEarth::PlaceNode* label = new osgEarth::PlaceNode(
            osgEarth::GeoPoint(
                geoSRS,
                location.getLongitude().value(),
                location.getLatitude().value(),
                0,
                osgEarth::AltitudeMode::ALTMODE_ABSOLUTE),
            portName.toStdString(),
            pm);

        label->setStyle(labelStyle);

        // Tag the node for identification
        osgEarth::Registry::objectIndex()->tagNode(label, label);
        osgEarth::ObjectID id;
        if (osgEarth::Registry::objectIndex()->getObjectID(label, id)) {
            portNodes[id] = label;
        }

        // Add to map
        _mapNode->addChild(label);
    }

    void removeTemporaryPort(const QString& portName = QString())
    {
        if (!_mapNode.valid())
        {
            qDebug() << "MapNode is not valid!";
            return;
        }

        // If name is empty, remove all temporary ports
        if (portName.isEmpty())
        {
            // Safely remove ports from the map and portNodes
            std::vector<osgEarth::ObjectID> idsToRemove;
            for (const auto& pair : portNodes)
            {
                auto placeNode = pair.second;
                if (placeNode && _mapNode->containsNode(placeNode))
                {
                    _mapNode->removeChild(placeNode);
                    idsToRemove.push_back(pair.first);
                }
            }

            // Remove from portNodes map
            for (const auto& id : idsToRemove)
            {
                portNodes.erase(id);
            }
        }
        else
        {
            // Remove specific port by name
            std::vector<osgEarth::ObjectID> idsToRemove;
            for (const auto& pair : portNodes)
            {
                auto placeNode =
                    dynamic_cast<osgEarth::PlaceNode*>(pair.second);
                if (placeNode &&
                    QString::fromStdString(placeNode->getText()) == portName)
                {
                    if (_mapNode->containsNode(placeNode))
                    {
                        _mapNode->removeChild(placeNode);
                        idsToRemove.push_back(pair.first);
                    }
                }
            }

            // Remove from portNodes map
            for (const auto& id : idsToRemove)
            {
                portNodes.erase(id);
            }
        }
    }

    void
    addSeaPort()
    {

        if (!_mapNode.valid())
        {
            qDebug() << "MapNode is not valid!";
            return;
        }

        // load sea ports
        QVector<std::shared_ptr<ShipNetSimCore::SeaPort>> ports =
            ShipNetSimCore::OptimizedNetwork::loadFirstAvailableSeaPorts();

        // hold sea ports as a map of countries
        std::unordered_map<QString,
                           QVector<std::shared_ptr<ShipNetSimCore::SeaPort>>>
            portsByCountry;

        // Style our labels:
        osgEarth::Style labelStyle;
        labelStyle.getOrCreate<osgEarth::TextSymbol>()->alignment() =
            osgEarth::TextSymbol::ALIGN_CENTER_CENTER;
        labelStyle.getOrCreate<osgEarth::TextSymbol>()->fill()->color() =
            osgEarth::Color::Black;
        labelStyle.getOrCreate<osgEarth::TextSymbol>()->halo() =
            osgEarth::Color("#5f5f5f");

        // Style for place node marker
        osgEarth::Style pm;
        pm.getOrCreate<osgEarth::IconSymbol>()->setImage(_normalImage);
        pm.getOrCreate<osgEarth::IconSymbol>()->declutter() = true;

        // group ports by country
        for (const auto& port : ports) {
            portsByCountry[port->getCountryName()].push_back(port);
        }

        // hold all ports
        osg::Group* parentGroup = new osg::Group();

        const osgEarth::SpatialReference* geoSRS =
            _mapNode->getMapSRS()->getGeographicSRS();
        // PortClickHandler* clickHandler =
        //     PortClickHandler::getInstance();
        for (const auto& countryPorts : portsByCountry) {
            osg::Group* countryGroup = new osg::Group();
            countryGroup->setName(
                "Country_" +
                countryPorts.second[0]->getCountryName().toStdString());
            for (const auto& port : countryPorts.second) {
                QString labelText = port->getPortName() +
                                    " (" + port->getPortCode() + ")";
                osgEarth::AnnotationNode* iconNode    = 0L;
                osgEarth::PlaceNode* label =
                    new osgEarth::PlaceNode(
                        osgEarth::GeoPoint(
                            geoSRS,
                            port->getPortCoordinate().getLongitude().value(),
                            port->getPortCoordinate().getLatitude().value(), 0,
                            osgEarth::AltitudeMode::ALTMODE_ABSOLUTE),
                        labelText.toStdString(),
                        pm);

                // store port data on the label
                auto customData =
                    new GlobalMapManager::CustomData<
                    std::shared_ptr<ShipNetSimCore::SeaPort>>(port);
                label->setUserData(customData);

                label->setStyle(labelStyle);
                // label->setName("Port_" + port->getPortName().toStdString());
                iconNode = label;

                osgEarth::Registry::objectIndex()->tagNode( iconNode, iconNode );
                osgEarth::ObjectID id;
                bool found = osgEarth::Registry::objectIndex()->
                             getObjectID(label, id);
                if (found) {
                    portNodes[id] = label;
                }

                // clickHandler->addPortNode(label, port);

                countryGroup->addChild(iconNode);
            }
            parentGroup->addChild(countryGroup);
        }
        _mapNode->addChild(parentGroup);

        // clickHandler->setTraversingRoot(parentGroup);
        // printSceneGraph(root);

    }


    std::vector<osgEarth::AnnotationNode*>
    filterAnnotationsByTitle(const std::string& title)
    {
        std::vector<osgEarth::AnnotationNode*> results;
        if (_mapNode) {
            osg::NodeVisitor nv(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN);
            nv.setNodeMaskOverride(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN);
            _mapNode->accept(nv);

            for (auto& node : nv.getNodePath())
            {
                osgEarth::AnnotationNode* annotation =
                    dynamic_cast<osgEarth::AnnotationNode*>(node);
                if (annotation && annotation->getName().find(title) !=
                                      std::string::npos)
                {
                    results.push_back(annotation);
                }
            }
        }
        return results;
    }

    void navigateToAnnotation(osgEarth::AnnotationNode* annotation,
                              osgViewer::Viewer* viewer)
    {
        if (annotation && viewer)
        {
            auto location = annotation->getBound().center();
            // Cast the camera manipulator to EarthManipulator
            osgEarth::Util::EarthManipulator* earthManipulator =
                dynamic_cast<osgEarth::Util::EarthManipulator*>(
                viewer->getCameraManipulator());
            if (earthManipulator)
            {
                osgEarth::Viewpoint vp("Target", location.x(), location.y(),
                                       location.z(), 0.0, -90.0, 1000.0);
                earthManipulator->setViewpoint(vp);
            }
        }
    }


    void printSceneGraph(const osg::Node* node, int level = 0) {
        if (!node) return; // Safety check

        // Create an indentation string based on the current level
        std::string indent(level * 2, ' ');

        // Print the current node's name and class type
        std::cout << indent << node->className() << ": " <<
            node->getName() << std::endl;

        // If the node is a Group, recursively print its children
        const osg::Group* group = node->asGroup();
        if (group) {
            for (unsigned int i = 0; i < group->getNumChildren(); ++i) {
                printSceneGraph(group->getChild(i), level + 1); // Recurse into each child
            }
        }
    }

private:
    osg::ref_ptr<osgEarth::MapNode> _mapNode;
    osg::ref_ptr<osg::Group> _root = new osg::Group();
    GlobalMapManager() {}
};

#endif // GLOBALMAPMANAGER_H
