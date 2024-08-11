#ifndef COMBOBOXDELEGATE_H
#define COMBOBOXDELEGATE_H
#include <QComboBox>
#include <QStyledItemDelegate>

class ComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ComboBoxDelegate(const QStringList items,
                     QObject *parent = nullptr)
        : m_items(items), QStyledItemDelegate(parent)
    {}

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override
    {
        QComboBox *editor = new QComboBox(parent);
        editor->addItems(m_items);


        // Calculate the width based on the longest item
//        int maxWidth = 0;
//        QFontMetrics fontMetrics(editor->font());
//        for (const QString& item : m_items) {
//            int width = fontMetrics.horizontalAdvance(item);
//            if (width > maxWidth) {
//                maxWidth = width;
//            }
//        }

        // Set the column width
//        int column = index.column();
//        QTableWidget *tableWidget =
//            qobject_cast<QTableWidget *>(parent->parentWidget());
//        if (tableWidget) {
//            tableWidget->setColumnWidth(column, maxWidth + 20);
//        }

        // Set the default selection
//        editor->setCurrentIndex(0);

        // Connect any necessary signals/slots

        return editor;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        QComboBox *comboBox = qobject_cast<QComboBox *>(editor);
        if (!comboBox)
            return;

        // Set the current value based on the model index
        const QString currentValue = index.data(Qt::EditRole).toString();
        const int currentIndex = comboBox->findText(currentValue);
        comboBox->setCurrentIndex(currentIndex >= 0 ? currentIndex : 0); // Set default to first item if not found

        // Ensure the default selection is displayed in the table widget
        comboBox->showPopup();
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
    {
        QComboBox *comboBox = qobject_cast<QComboBox *>(editor);
        if (!comboBox)
            return;

        // Update the model data based on the selected value in the combobox
        const QString selectedValue = comboBox->currentText();
        model->setData(index, selectedValue, Qt::EditRole);
    }


    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        // Get the current value based on the model index
        const QString currentValue = index.data(Qt::EditRole).toString();

        QPalette palette = opt.palette;
        if (currentValue != "" && currentValue != "Select ...")
            palette.setColor(QPalette::Text, Qt::black);
        else
            palette.setColor(QPalette::Text, Qt::gray);

        opt.palette = palette;
        opt.text = currentValue == "" ? "Select ..." : currentValue;

        QStyledItemDelegate::paint(painter, opt, index);
    }

//    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override
//    {
//        editor->setGeometry(option.rect);

//        // Ensure the default value is visible without entering edit mode
//        QComboBox *comboBox = qobject_cast<QComboBox *>(editor);
//        if (comboBox && comboBox->currentIndex() != -1) {
//            const QRect itemRect = comboBox->view()->visualRect(comboBox->model()->index(comboBox->currentIndex(), 0));
//            comboBox->view()->scrollTo(comboBox->model()->index(comboBox->currentIndex(), 0), QAbstractItemView::PositionAtTop);
//            comboBox->view()->setMinimumHeight(itemRect.height());
//        }
//    }

    QStringList getValues() {
        return m_items;
    }

private:
    QStringList m_items;
};
#endif // COMBOBOXDELEGATE_H
