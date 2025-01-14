#ifndef TEXTBOXBUTTONDELEGATE_H
#define TEXTBOXBUTTONDELEGATE_H

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
#include "enginepowerpopupform.h"
#include "enginerpmefficiencypopupform.h"
#include "generalPopupForm.h"


class TextBoxButtonDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    enum FormType{
        RPMEfficiency,
        Power,
        General,
    };

    struct FormDetails {
        bool boolVal;
        QString labelName;
        QStringList ColNames;
        QStringList rowsNames;
        QVector<QStringList> data;

        FormDetails() = default;

        FormDetails(QString formLabelName, QStringList formColNames,
                    QStringList formRowsNames, QVector<QStringList> formComboData)
            : boolVal(false),
            labelName(std::move(formLabelName)),
            ColNames(std::move(formColNames)),
            rowsNames(std::move(formRowsNames)), data(std::move(formComboData)) {}

        FormDetails(bool formBool) :
            boolVal(formBool), labelName(""), ColNames({}),
            rowsNames({}), data({{}}) {}
    };

    TextBoxButtonDelegate(const FormType &formType,
                          QWidget *parent = nullptr,
                          const FormDetails &formDetails = FormDetails())
        : QStyledItemDelegate(parent), mFormType(formType),
        mFormDetails(std::move(formDetails)) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override
    {
        QWidget *editor = new QWidget(parent);
        QHBoxLayout *layout = new QHBoxLayout(editor);
        layout->setContentsMargins(0, 0, 0, 0);

        QTextEdit *textEdit = new QTextEdit(editor);
        QPushButton *button = new QPushButton("...", editor);

        // Make the button smaller
        button->setFixedSize(20, 20);
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        layout->addWidget(textEdit);
        layout->addWidget(button);

        connect(button, &QPushButton::clicked, this, [=, this]() mutable {
            if (mFormType == FormType::RPMEfficiency) {
                EngineRPMEfficiencyPopupForm form(mFormDetails.boolVal, parent);

                // Load data from textEdit into the form
                QStringList entries =
                    textEdit->toPlainText().split(";", Qt::SkipEmptyParts);
                int row = 0;
                for (const QString &entry : entries) {
                    QStringList values = entry.split(", ");
                    if (values.size() == 3) {
                        if (row >= form.tableWidget->rowCount()) {
                            form.tableWidget->insertRow(
                                form.tableWidget->rowCount());
                        }
                        form.tableWidget->setItem(
                            row, 0, new QTableWidgetItem(values[0])); // Engine Power
                        form.tableWidget->setItem(
                            row, 1, new QTableWidgetItem(values[1])); // Engine RPM
                        form.tableWidget->setItem(
                            row, 3, new QTableWidgetItem(values[2])); // Efficiency
                        row++;
                    }
                }

                if (form.exec() == QDialog::Accepted) {
                    // Collect data from the table
                    QString data;
                    for (int row = 0; row < form.tableWidget->rowCount(); ++row)
                    {
                        if (form.tableWidget->item(row, 0) &&
                            form.tableWidget->item(row, 1) &&
                            form.tableWidget->item(row, 3))
                        {
                            data += form.tableWidget->item(row, 0)->text() +
                                    ", " +
                                    form.tableWidget->item(row, 1)->text() +
                                    ", " +
                                    form.tableWidget->item(row, 3)->text() +
                                    "; ";
                        }
                    }
                    textEdit->setText(data);
                    emit const_cast<TextBoxButtonDelegate*>(this)->
                        commitData(editor);
                }
            } else if (mFormType == FormType::Power) {
                EnginePowerPopupForm form(parent);

                // Load data from textEdit into the form
                QStringList entries =
                    textEdit->toPlainText().split("; ", Qt::SkipEmptyParts);
                int row = 0;
                for (const QString &entry : entries) {
                    QString value = entry.trimmed();
                    if (!value.isEmpty()) {
                        if (row >= form.tableWidget->rowCount()) {
                            form.tableWidget->
                                insertRow(form.tableWidget->rowCount());
                        }
                        form.tableWidget->setItem(
                            row, 0, new QTableWidgetItem(value)); // Power value
                        row++;
                    }
                }

                if (form.exec() == QDialog::Accepted) {
                    // Collect data from the table
                    QString data;
                    for (int row = 0; row < form.tableWidget->rowCount(); ++row)
                    {
                        if (form.tableWidget->item(row, 0)) {
                            if (! form.tableWidget->item(row, 0)->
                                 text().trimmed().isEmpty())
                            {
                                data += form.tableWidget->item(row, 0)->text() +
                                        "; ";
                            }
                        }
                    }
                    textEdit->setText(data);
                    emit const_cast<TextBoxButtonDelegate*>(this)->
                        commitData(editor);
                }
            } else if (mFormType == FormType::General) {
                GeneralPopupForm form(mFormDetails.labelName,
                                      mFormDetails.ColNames,
                                      mFormDetails.rowsNames,
                                      mFormDetails.data,
                                      parent);

                QVector<int> comboIndecies = {};
                for (int i = 0; i < mFormDetails.data.size(); i++) {
                    if (mFormDetails.data[i][0] == "comboBox") {
                        comboIndecies.push_back(i);
                    }
                }

                // Load data from textEdit into the form
                QStringList entries =
                    textEdit->toPlainText().split("; ", Qt::SkipEmptyParts);
                int row = 0;
                for (const QString &entry : entries) {
                    QStringList values = entry.split(", ");
                    if (values.size() == form.tableWidget->columnCount()) {
                        if (row >= form.tableWidget->rowCount()) {
                            form.tableWidget->insertRow(
                                form.tableWidget->rowCount());
                        }
                        int colCount = std::min(form.tableWidget->columnCount(),
                                                int(values.size()));
                        for (int col = 0; col < colCount; ++col) {
                            if (comboIndecies.contains(col)) {
                                auto d = mFormDetails.data[col].
                                         at(values[col].toInt() + 1);
                                form.tableWidget->setItem(row, col,
                                new QTableWidgetItem(d));
                            }
                            else {
                                form.tableWidget->setItem(
                                    row, col,
                                    new QTableWidgetItem(values[col]));
                            }
                        }
                        row++;
                    }
                }

                if (form.exec() == QDialog::Accepted) {
                    // Collect data from the table
                    QString data;
                    for (int row = 0; row < form.tableWidget->rowCount(); ++row)
                    {
                        bool isEmptyRow = true;
                        QString rowData;
                        for (int col = 0;
                             col < form.tableWidget->columnCount(); ++col)
                        {
                            QTableWidgetItem *item =
                                form.tableWidget->item(row, col);
                            if (item && !item->text().isEmpty())
                            {
                                isEmptyRow = false;
                                if (comboIndecies.contains(col)) {
                                    auto index =
                                        mFormDetails.data[col].
                                        indexOf(item->text(), 0,
                                                Qt::CaseInsensitive) - 1;
                                    rowData += QString::number(index);
                                }
                                else {
                                    rowData += item->text();
                                }
                            }
                            if (col < form.tableWidget->columnCount() - 1)
                            {
                                rowData += ", ";
                            }
                        }
                        if (!isEmptyRow)
                        {
                            data += rowData;
                            data += "; ";
                        }
                    }

                    textEdit->setText(data);
                    emit const_cast<TextBoxButtonDelegate*>(this)->
                        commitData(editor);
                }
            }

        });

        editor->setLayout(layout);
        return editor;
    }

    void setEditorData(QWidget *editor,
                       const QModelIndex &index) const override
    {
        QTextEdit *textEdit = editor->findChild<QTextEdit *>();
        textEdit->setText(index.model()->data(index, Qt::EditRole).toString());
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override
    {
        QTextEdit *textEdit = editor->findChild<QTextEdit *>();
        model->setData(index, textEdit->toPlainText(), Qt::EditRole);
    }

    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override {
        editor->setGeometry(option.rect);
    }

private:
    FormType mFormType;
    FormDetails mFormDetails;
};

#endif // TEXTBOXBUTTONDELEGATE_H
