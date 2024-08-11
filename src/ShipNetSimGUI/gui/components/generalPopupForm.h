#ifndef GENERALPOPUPFORM_H
#define GENERALPOPUPFORM_H

#include "ShipNetSimGUI/gui/components/comboboxdelegate.h"
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

class GeneralPopupForm : public QDialog {
    Q_OBJECT
public:

    GeneralPopupForm(const QString labelName,
                     const QStringList ColNames,
                     const QStringList rowsNames = QStringList(),
                     QVector<QStringList> dataList = QVector<QStringList>(),
                     QWidget *parent = nullptr) : QDialog(parent) {
        assert(dataList.size() == ColNames.size());

        warnningLabel = new DisappearingLabel(this);

        QVBoxLayout *layout = new QVBoxLayout(this);
        QLabel *label = new QLabel(labelName, this);

        int rowCount = rowsNames.size() > 0 ? rowsNames.size() : 1;
        tableWidget = new QTableWidget(rowCount, ColNames.size(), this);
        tableWidget->setHorizontalHeaderLabels(ColNames);
        tableWidget->horizontalHeader()->
            setSectionResizeMode(QHeaderView::Interactive);
        QDialogButtonBox *buttonBox =
            new QDialogButtonBox(   QDialogButtonBox::Save |
                                     QDialogButtonBox::Cancel, this);

        if (rowsNames.isEmpty()) {
            tableWidget->verticalHeader()->setVisible(true);
            // Connect to handle row addition and cell changes
            connect(tableWidget, &QTableWidget::cellChanged, this,
                    &GeneralPopupForm::onCellChanged);
            // Install event filter for context menu
            tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(tableWidget, &QWidget::customContextMenuRequested,
                    this, &GeneralPopupForm::onCustomContextMenuRequested);
            connect(buttonBox, &QDialogButtonBox::accepted,
                    this, &GeneralPopupForm::validateAndAccept);

        }
        else {
            tableWidget->setVerticalHeaderLabels(rowsNames);
            tableWidget->verticalHeader()->setVisible(true);
            connect(buttonBox, &QDialogButtonBox::accepted,
                    this, &QDialog::accept);
        }

        for (int c = 0; c < dataList.size(); c++) {
            QStringList data = dataList[c];
            if (!data.isEmpty() && data[0] == "comboBox") {
                data.removeFirst(); // Remove "comboBox" identifier

                tableWidget->setItemDelegateForColumn(
                    c, new ComboBoxDelegate(data, this));

            }
            else if (!data.isEmpty() && data[0] == "numericSpin") {
                // Set separate delegates for each column
                tableWidget->setItemDelegateForColumn(
                    c, new NumericDelegate(this,
                                        data[1].toDouble(),
                                        data[2].toDouble(),
                                        data[3].toInt(),
                                        data[4].toDouble(),
                                        data[5].toDouble() ));
            }
        }



        QHBoxLayout *lowerSection = new QHBoxLayout();
        lowerSection->addWidget(warnningLabel);
        lowerSection->addSpacerItem(new QSpacerItem(20, 20,
                                                    QSizePolicy::Expanding,
                                                    QSizePolicy::Minimum));
        lowerSection->addWidget(buttonBox);

        layout->addWidget(label);
        layout->addWidget(tableWidget);
        layout->addLayout(lowerSection);


        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);


    }

    QTableWidget *tableWidget;
    QDoubleSpinBox *fuelCalorificValue;
    DisappearingLabel *warnningLabel;

    int currentRow = -1; // To store the row index for the context menu

private slots:
    void onCellChanged(int row, int column) {
        if (row == tableWidget->rowCount() - 1) {
            tableWidget->insertRow(tableWidget->rowCount());
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
        connect(&actionDelete, &QAction::triggered,
                this, &GeneralPopupForm::deleteRow);
        contextMenu.addAction(&actionDelete);
        contextMenu.exec(tableWidget->mapToGlobal(pos));
    }

    void deleteRow() {
        if (currentRow >= 0 && currentRow < tableWidget->rowCount() &&
            tableWidget->rowCount() > 1) {
            tableWidget->removeRow(currentRow);
            currentRow = -1; // Reset after deletion
        }
    }

    void validateAndAccept() {
        if (validateInputs()) {
            accept();
        } else {
            warnningLabel->setTextWithTimeout(
                "Please fill in all cells before saving.", 3000, Qt::red);
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
};

#endif // GENERALPOPUPFORM_H
