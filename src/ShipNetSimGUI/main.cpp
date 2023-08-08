#include "shipnetsim.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ShipNetSim w;
    w.show();
    return a.exec();
}
