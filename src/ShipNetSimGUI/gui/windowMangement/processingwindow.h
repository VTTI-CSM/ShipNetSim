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

    // Methods for progress display
    void setTitle(const QString& title);
    void setProgress(int percentage);
    void setStatusText(const QString& status);
    void reset();  // Reset to initial state (spinner mode)

    // Format elapsed time professionally (e.g., "2.5s", "1m 30s", "2h 15m", "1d 3h")
    static QString formatElapsedTime(double seconds);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    Ui::ProcessingWindow *ui;
};

#endif // PROCESSINGWINDOW_H
