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

#include <QString>
#include "../../utils/defaults.h"
#include "osgEarth/ObjectIndex"
#include "portclickhandler.h"
#include "utils/utils.h"
#include "../network/seaport.h"

class GlobalMapManager
{
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

    void preloadEarthModel() {
        if (!_mapNode) { // Prevent reloading if already loaded
            // get the earth file path
            QString filePath =
                ShipNetSimCore::Utils::getFirstExistingPathFromList(
                    Defaults::earthTifPaths);

            loadEarthModel(filePath);
        }
    }

    osg::ref_ptr<osgEarth::MapNode> getMapNode() {
        return _mapNode;
    }

    osg::ref_ptr<osg::Group> getRootGroup() {
        return _root;
    }

    void loadEarthModel(const QString &filename)
    {
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

        // osgEarth::Style pointStyle;
        // pointStyle.getOrCreate<osgEarth::IconSymbol>()->url()->setLiteral("path/to/icon.png");
        // pointStyle.getOrCreate<osgEarth::IconSymbol>()->declutter() = true;


        // Path for icon
        std::string icon =
            ShipNetSimCore::Utils::
            getFirstExistingPathFromList(Defaults::iconPath).toStdString();

        // Style for place node marker
        osgEarth::Style pm;
        pm.getOrCreate<osgEarth::IconSymbol>()->url()->setLiteral( icon );
        pm.getOrCreate<osgEarth::IconSymbol>()->declutter() = true;
        // pm.getOrCreate<osgEarth::TextSymbol>()->halo() =
        //     osgEarth::Color("#5f5f5f");

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

                // clickHandler->addPortNode(label, port);

                countryGroup->addChild(iconNode);
            }
            parentGroup->addChild(countryGroup);
        }
        _mapNode->addChild(parentGroup);

        // clickHandler->setTraversingRoot(parentGroup);
        // printSceneGraph(root);

    }

    void printSceneGraph(const osg::Node* node, int level = 0) {
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

private:
    osg::ref_ptr<osgEarth::MapNode> _mapNode;
    osg::ref_ptr<osg::Group> _root = new osg::Group();
    GlobalMapManager() {}
};

#endif // GLOBALMAPMANAGER_H
