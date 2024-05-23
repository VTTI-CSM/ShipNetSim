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



void ShipNetSim::on_tabWidget_results_currentChanged(int index)
{
    if (index == 1)
    {
        // ui->plot_trains->setLoadedModelToWidget(earthResults.second);
    }
}

