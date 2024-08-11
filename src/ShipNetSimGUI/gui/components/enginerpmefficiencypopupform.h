#ifndef ENGINERPMEFFICIENCYPOPUPFORM_H
#define ENGINERPMEFFICIENCYPOPUPFORM_H

#include "ShipNetSimGUI/gui/components/disappearinglabel.h"
#include "ShipNetSimGUI/gui/components/numericdelegate.h"
#include "qheaderview.h"
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QWidget>
#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QMenu>
#include <QContextMenuEvent>
#include "../third_party/qcustomplot/qcustomplot.h"


class EngineRPMEfficiencyPopupForm : public QDialog {
    Q_OBJECT
public:
    EngineRPMEfficiencyPopupForm(QWidget *parent = nullptr) : QDialog(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        QLabel *label = new QLabel("Enter Engine Data:", this);
        warnningLabel = new DisappearingLabel(this);

        plot = new QCustomPlot();
        plot->addGraph();
        // Set titles for the axes
        plot->xAxis->setLabel("RPM");
        plot->yAxis->setLabel("Fuel Consumption Rate (g/kWh)");

        // Add Fuel Calorific Value input
        QHBoxLayout *calorificValueLayout = new QHBoxLayout();
        QLabel *calorificValueLabel = new QLabel("Reference Fuel Calorific Value (kWh/kg):", this);
        fuelCalorificValue = new QDoubleSpinBox(this);
        fuelCalorificValue->setDecimals(4);
        fuelCalorificValue->setMaximum(100.0);
        fuelCalorificValue->setMinimum(0.0);
        fuelCalorificValue->setValue(11.8611); // default value, adjust as needed

        calorificValueLayout->addWidget(calorificValueLabel);
        calorificValueLayout->addWidget(fuelCalorificValue);

        tableWidget = new QTableWidget(1, 3, this);
        QStringList headers = QStringList() << "Engine RPM" << "Fuel Consumption Rate (g/kWh)" << "Efficiency";
        tableWidget->setHorizontalHeaderLabels(headers);
        tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        // tableWidget->verticalHeader()->setVisible(false);

        // Set separate delegates for each column
        tableWidget->setItemDelegateForColumn(0, new NumericDelegate(this, 10000.0, 0.0, 0, 100.0, 0.0)); // RPM column
        tableWidget->setItemDelegateForColumn(1, new NumericDelegate(this, 1000.0, 0.0, 2, 0.1, 0.0)); // Fuel Consumption Rate column
        tableWidget->setItemDelegateForColumn(2, new NumericDelegate(this, 1.0, 0.0, 2, 0.01, 0.0)); // Efficiency column

        // Install event filter for context menu
        tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(tableWidget, &QWidget::customContextMenuRequested, this, &EngineRPMEfficiencyPopupForm::onCustomContextMenuRequested);

        // Connect to handle row addition and cell changes
        connect(tableWidget, &QTableWidget::cellChanged, this,
                &EngineRPMEfficiencyPopupForm::onCellChanged);

        QDialogButtonBox *buttonBox =
            new QDialogButtonBox(QDialogButtonBox::Save |
                                 QDialogButtonBox::Cancel, this);

        QHBoxLayout *lowerSection = new QHBoxLayout();
        lowerSection->addWidget(warnningLabel);
        lowerSection->addSpacerItem(new QSpacerItem(20, 20,
                                                    QSizePolicy::Expanding,
                                                    QSizePolicy::Minimum));
        lowerSection->addWidget(buttonBox);

        layout->addWidget(label);
        layout->addLayout(calorificValueLayout); // Add the calorific value input above the table
        layout->addWidget(tableWidget);
        layout->addWidget(plot);
        layout->addLayout(lowerSection);

        connect(buttonBox, &QDialogButtonBox::accepted, this, &EngineRPMEfficiencyPopupForm::validateAndAccept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        plot->setFixedHeight(250); // Set the height to 250 pixels
        resize(470, 630);

    }

    QTableWidget *tableWidget;
    QDoubleSpinBox *fuelCalorificValue;
    DisappearingLabel *warnningLabel;
    QCustomPlot *plot;
    bool disableSlotLogic = false;
    int currentRow = -1; // To store the row index for the context menu

private slots:
    void onCellChanged(int row, int column) {
        if (disableSlotLogic) {
            return;
        }
        if (row == tableWidget->rowCount() - 1) {
            tableWidget->insertRow(tableWidget->rowCount());
        }

        // Calculate Efficiency or Fuel Consumption Rate
        // based on the changed column
        if (column == 1 || column == 2) {
            disableSlotLogic = true;

            double calorificValue = fuelCalorificValue->value();
            bool ok1 = false, ok2 = false;
            double fuelConsumptionRate =
                (tableWidget->item(row, 1)) ?
                    tableWidget->item(row, 1)->text().toDouble(&ok1) : 0.0;
            double efficiency =
                tableWidget->item(row, 2) ?
                    tableWidget->item(row, 2)->text().toDouble(&ok2) : 0.0;

            if (ok1 && column == 1) {
                // Update Efficiency when Fuel Consumption Rate changes
                double fuelConsumptionRate_kg =
                    1000.0 / fuelConsumptionRate; // Convert g/kWh to kg/kWh
                efficiency = fuelConsumptionRate_kg/ calorificValue;

                QTableWidgetItem *item = tableWidget->item(row, 2);
                if (!item) {
                    item = new QTableWidgetItem(
                        QString::number(efficiency, 'f', 2));
                    tableWidget->setItem(row, 2, item);
                }
                else {
                    item->setText(QString::number(efficiency, 'f', 2));
                }

            } else if (ok2 && column == 2) {
                // Update Fuel Consumption Rate when Efficiency changes
                fuelConsumptionRate =
                    (1.0 / (calorificValue * efficiency)) *
                                      1000.0; // Convert kg/kWh to g/kWh

                QTableWidgetItem *item = tableWidget->item(row, 1);
                if (!item) {
                    item = new QTableWidgetItem(
                        QString::number(fuelConsumptionRate, 'f', 2));
                    tableWidget->setItem(row, 1, item);
                }
                else {
                    item->setText(
                        QString::number(fuelConsumptionRate, 'f', 2));
                }

            }


            disableSlotLogic = false;
        }

        // update plot
        updatePlot();
    }

    void validateAndAccept() {
        if (validateInputs()) {
            accept();
        } else {
            warnningLabel->setTextWithTimeout(
                "Please fill in all cells before saving.", 3000, Qt::red);
        }
    }

    void onCustomContextMenuRequested(const QPoint &pos) {
        QTableWidgetItem *item = tableWidget->itemAt(pos);
        if (item) {
            currentRow = item->row();
            showContextMenu(pos);
        }
    }

    void showContextMenu(const QPoint &pos) {
        QMenu contextMenu(tr("Context menu"), this);
        QAction actionDelete(tr("Delete Row"), this);
        connect(&actionDelete, &QAction::triggered, this, &EngineRPMEfficiencyPopupForm::deleteRow);
        contextMenu.addAction(&actionDelete);
        contextMenu.exec(tableWidget->mapToGlobal(pos));
    }

    void deleteRow() {
        if (currentRow >= 0 && currentRow < tableWidget->rowCount() &&
            tableWidget->rowCount() > 1) {
            tableWidget->removeRow(currentRow);
            currentRow = -1; // Reset after deletion
            updatePlot();
        }
    }

private:
    bool validateInputs() {
        for (int row = 0; row < tableWidget->rowCount() - 1; ++row)
        {
            for (int col = 0; col < tableWidget->columnCount(); ++col)
            {
                if (!tableWidget->item(row, col) ||
                    tableWidget->item(row, col)->text().isEmpty())
                {
                    return false;
                }
            }
        }
        return true;
    }

    void updatePlot() {
        if (tableWidget->columnCount() > 1) {
            QCPGraph *graph = plot->graph(0);
            graph->setScatterStyle(QCPScatterStyle::ssCircle); // Change to circle scatter style
            graph->setLineStyle(QCPGraph::lsLine); // Ensure lines are shown
            graph->data()->clear();

            QVector<double> xData, yData;
            for (int r = 0; r < tableWidget->rowCount(); r++) {
                bool okX = false, okY = false;
                double x = (tableWidget->item(r, 0)) ?
                               tableWidget->item(r, 0)->text().toDouble(&okX) :
                               std::nan("");
                double y = (tableWidget->item(r, 1)) ?
                               tableWidget->item(r, 1)->text().toDouble(&okY) :
                               std::nan("");
                if (okX && okY) {
                    xData.append(x);
                    yData.append(y);
                }
            }

            graph->setData(xData, yData);
            plot->rescaleAxes();
            plot->replot();
        }
    }
};

#endif // ENGINERPMEFFICIENCYPOPUPFORM_H
