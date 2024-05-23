#ifndef SHIPNETSIM_H
#define SHIPNETSIM_H

#include <QMainWindow>
#include <osgEarth/ExampleResources>


QT_BEGIN_NAMESPACE
namespace Ui { class ShipNetSim; }
QT_END_NAMESPACE

class ShipNetSim : public QMainWindow
{
    Q_OBJECT

public:
    ShipNetSim(QWidget *parent = nullptr);
    ~ShipNetSim();

public:
    Ui::ShipNetSim *ui;

    // std::pair<osg::ref_ptr<osgEarth::MapNode>,
    //           osg::ref_ptr<osg::Group>> earthResults;
private slots:
    void on_tabWidget_results_currentChanged(int index);
};
#endif // SHIPNETSIM_H
