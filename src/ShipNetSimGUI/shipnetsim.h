#ifndef SHIPNETSIM_H
#define SHIPNETSIM_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class ShipNetSim; }
QT_END_NAMESPACE

class ShipNetSim : public QMainWindow
{
    Q_OBJECT

public:
    ShipNetSim(QWidget *parent = nullptr);
    ~ShipNetSim();

private:
    Ui::ShipNetSim *ui;
};
#endif // SHIPNETSIM_H
