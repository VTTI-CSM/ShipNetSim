#ifndef ENGINEPOWERPOPUPFORM_H
#define ENGINEPOWERPOPUPFORM_H

#include "./numericdelegate.h"
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
#include <QStyledItemDelegate>

class EnginePowerPopupForm : public QDialog {
    Q_OBJECT
public:
    EnginePowerPopupForm(QWidget *parent = nullptr) : QDialog(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        QLabel *label = new QLabel("Enter Engine Data:", this);

        tableWidget = new QTableWidget(1, 1, this);
        tableWidget->setHorizontalHeaderLabels(QStringList() << "Engine Power (kW)");
        tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tableWidget->verticalHeader()->setVisible(false);

        // Set separate delegates for each column
        tableWidget->setItemDelegateForColumn(
            0, new NumericDelegate(this, 1000000.0, 0.0, 2, 100.0, 0.0)); // Power column

        // Connect to handle row addition
        connect(tableWidget, &QTableWidget::cellChanged,
                this, &EnginePowerPopupForm::onCellChanged);

        QDialogButtonBox *buttonBox =
            new QDialogButtonBox(QDialogButtonBox::Save |
                                 QDialogButtonBox::Cancel, this);

        QHBoxLayout *lowerSection = new QHBoxLayout();
        lowerSection->addSpacerItem(new QSpacerItem(20, 20,
                                                    QSizePolicy::Expanding,
                                                    QSizePolicy::Minimum));
        lowerSection->addWidget(buttonBox);

        layout->addWidget(label);
        layout->addWidget(tableWidget);
        layout->addLayout(lowerSection);

        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    QTableWidget *tableWidget;

private slots:
    void onCellChanged(int row, int column) {
        if (row == tableWidget->rowCount() - 1) {
            tableWidget->insertRow(tableWidget->rowCount());
        }
    }
};

#endif // ENGINEPOWERPOPUPFORM_H
