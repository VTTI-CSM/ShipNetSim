#ifndef PROCESSINGWINDOW_H
#define PROCESSINGWINDOW_H

#include <QDialog>

namespace Ui {
class ProcessingWindow;
}

class ProcessingWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ProcessingWindow(QWidget *parent = nullptr);
    ~ProcessingWindow();

protected:
    void showEvent(QShowEvent *event) override; // Override for show
    void hideEvent(QHideEvent *event) override; // Override for hide

private:
    Ui::ProcessingWindow *ui;
};

#endif // PROCESSINGWINDOW_H
