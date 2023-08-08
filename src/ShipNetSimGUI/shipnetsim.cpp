#include "shipnetsim.h"
#include "./ui_shipnetsim.h"

ShipNetSim::ShipNetSim(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ShipNetSim)
{
    ui->setupUi(this);
}

ShipNetSim::~ShipNetSim()
{
    delete ui;
}

