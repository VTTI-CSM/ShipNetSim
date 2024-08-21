#include "settingswindow.h"
#include "./shipnetsim.h"
#include "./shipnetsimui.h"
#include "ui_settingswindow.h"
#include <QFileDialog>

settingsWindow::settingsWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::settingsWindow) {
    ui->setupUi(this);
    this->loadSavedSettings();
}

settingsWindow::~settingsWindow() {
    delete ui;
}

void settingsWindow::on_pushButton_browse_clicked() {
    this->browseFolder(ui->lineEdit_defaultBrowseLocation,
                       "Select the default browse location");
}


void settingsWindow::browseFolder(QLineEdit* theLineEdit,
                                  const QString& theHelpMessage) {
    QString folderPath =
        QFileDialog::getExistingDirectory(this,
                                          theHelpMessage,
                                          QDir::homePath(),
                                          QFileDialog::ShowDirsOnly |
                                              QFileDialog::DontResolveSymlinks);
    // Check if a folder was selected
    if (!folderPath.isEmpty()) {
        theLineEdit->setText(folderPath);
    }
}

void settingsWindow::loadSavedSettings() {

    ShipNetSim* mainWindow = qobject_cast<ShipNetSim*>(parent());
    this->ui->lineEdit_defaultBrowseLocation->
        setText(mainWindow->defaultBrowsePath);

    mainWindow = nullptr;
}


void settingsWindow::on_pushButton_save_clicked() {

    ShipNetSim* mainWindow = qobject_cast<ShipNetSim*>(parent());
    QStringList defaultConfigs;
    defaultConfigs << "default.browseLocation=" +
                          ui->lineEdit_defaultBrowseLocation->text();

    bool result = mainWindow->saveDefaults(defaultConfigs);
    if (result) {
        ShipNetSimUI::showNotification(mainWindow,
                                       "Settings saved successfully!");
        this->close();
    }
    else {
        ShipNetSimUI::showNotification(mainWindow,
                                       "Settings could not be saved!");
    }

    mainWindow = nullptr;
}

