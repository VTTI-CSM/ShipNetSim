#ifndef PORTCLICKHANDLER_H
#define PORTCLICKHANDLER_H
#include <QObject>
#include <unordered_map>
#include <osgEarth/PlaceNode>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osgGA/GUIEventHandler>
#include "../network/seaport.h"
#include "qdebug.h"
#include <osgViewer/ViewerEventHandlers>

class PortClickHandler : public QObject, public osgGA::GUIEventHandler
{
    Q_OBJECT
public:
    // Method to retrieve the static instance
    static PortClickHandler* getInstance()
    {
        static PortClickHandler instance;
        return &instance;
    }

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE &&
            ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
        {
            qDebug() << ea.getX() << " " << ea.getY();
            osgViewer::View* view = dynamic_cast<osgViewer::View*>(&aa);
            if (view)
            {
                osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector =
                    new osgUtil::LineSegmentIntersector(
                    osgUtil::Intersector::WINDOW, ea.getX(), ea.getY());

                osgUtil::IntersectionVisitor iv(intersector.get());
                iv.setTraversalMask(osg::Node::NodeMask(~0));
                root->accept(iv);
                if (intersector->containsIntersections())
                {
                    std::cout << "Count: " << intersector->getIntersections().size();

                    for (const auto& intersection :
                         intersector->getIntersections())
                    {
                        for (auto& node : intersection.nodePath) {
                            if (node) {
                                std::cout << "Node in Path: " << node->className() << ": " << node->getName() << std::endl;
                            }
                        }

                        osgEarth::PlaceNode* placeNode =
                            dynamic_cast<osgEarth::PlaceNode*>(
                            intersection.nodePath.back());

                        if (placeNode)
                        {
                            std::shared_ptr<ShipNetSimCore::SeaPort> port =
                                getPortFromLabel(placeNode);
                            if (port)
                            {
                                qDebug() << " ------------------ " << port->getPortCode();
                                emit portSelected(port);
                                return true; // Event handled
                            }
                        }
                    }
                }
            }
        }
        return false; // Event not handled
    }

    void addPortNode(osgEarth::PlaceNode* placeNode,
                     std::shared_ptr<ShipNetSimCore::SeaPort> port)
    {
        portNodeMap[placeNode] = port;
    }

    void setTraversingRoot(osg::ref_ptr<osg::Group> newRoot) {
        root = newRoot;
    }

private:
    osg::ref_ptr<osg::Group> root;

    PortClickHandler() {}

    // Map to associate PlaceNodes with port IDs
    std::unordered_map<osgEarth::PlaceNode*,
                       std::shared_ptr<ShipNetSimCore::SeaPort>> portNodeMap;

    std::shared_ptr<ShipNetSimCore::SeaPort>
    getPortFromLabel(osgEarth::PlaceNode* placeNode)
    {
        // Retrieve port ID from the map
        auto it = portNodeMap.find(placeNode);
        if (it != portNodeMap.end())
        {
            return it->second;

        }
        return nullptr; // Port not found
    }



signals:
    void portSelected(std::shared_ptr<ShipNetSimCore::SeaPort> port);


};

#endif // PORTCLICKHANDLER_H
