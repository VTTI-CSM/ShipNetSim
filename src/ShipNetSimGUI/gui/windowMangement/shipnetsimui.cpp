#include "shipnetsimui.h"
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include "gui/windowMangement/ui_shipnetsim.h"


#include <QCoreApplication>
#include <QDir>

void ShipNetSimUI::showNotificationBox(QString msg) {
    QMessageBox::information(nullptr, "NeTrainSim - Notification", msg);
}

void ShipNetSimUI::showWarningBox(QString msg) {
    QMessageBox::warning(nullptr, "NeTrainSim - Warning", msg);
}

void ShipNetSimUI::showErrorBox(QString msg) {
    QMessageBox::critical(nullptr, "NeTrainSim - Error", msg);
}


void ShipNetSimUI::showWarning(ShipNetSim *parent, QString text) {
    parent->ui->label_Notification->setTextWithTimeout(text, 3000);
    parent->ui->label_Notification->setStyleSheet("color: red;");
}

void ShipNetSimUI::showNotification(ShipNetSim *parent, QString text) {
    parent->ui->label_Notification->setTextWithTimeout(text, 3000);
    parent->ui->label_Notification->setStyleSheet("color: black;");
}

void ShipNetSimUI::handleError(ShipNetSim *parent, QString error) {
    parent->ui->pushButton_projectNext->setEnabled(true);
    showErrorBox(error);
}

void ShipNetSimUI::openManual(ShipNetSim *parent) {
    QString executablePath = QCoreApplication::applicationDirPath();
    QString fileName = QDir(executablePath).filePath("Manual.pdf");
    QFile pfn(fileName);
    if (!pfn.exists()) {
        ShipNetSimUI::showWarning(parent, "File does not exist!");
        return;
    }

    QUrl fileUrl = QUrl::fromLocalFile(fileName);
    if (! QDesktopServices::openUrl(fileUrl)) {
        ShipNetSimUI::showWarning(parent, "Failed to open the PDF file!");
    }
}

void ShipNetSimUI::openSettingsPage(ShipNetSim *parent) {
    if (parent->theSettingsWindow == nullptr) {
        parent->theSettingsWindow =
            std::make_shared<settingsWindow>(parent);
    }
    // Check if the initialization was successful
    if (parent->theSettingsWindow != nullptr) {
        parent->theSettingsWindow->show();
    }
}

QString ShipNetSimUI::browseFiles(ShipNetSim *parent,
                                  QLineEdit* theLineEdit,
                                const QString& theFileName) {
    QString browsLoc = QString();
    if ( parent->userBrowsePath.isEmpty()) {
        browsLoc = parent->defaultBrowsePath;
    }
    else {
        browsLoc = parent->userBrowsePath;
    }
    QString fname = QFileDialog::getOpenFileName(parent, theFileName,
                                                 browsLoc, "DAT Files (*.DAT)");
    if (!fname.isEmpty()) {
        theLineEdit->setText(fname);
        QFileInfo fileInfo(fname);
        parent->userBrowsePath = fileInfo.dir().path();
    }
    return fname;
}

void ShipNetSimUI::browseFolder(ShipNetSim *parent,
                                QLineEdit* theLineEdit,
                                const QString& theHelpMessage) {
    QString browsLoc = QString();
    if ( parent->userBrowsePath.isEmpty()) {
        browsLoc = parent->defaultBrowsePath;
    }
    else {
        browsLoc = parent->userBrowsePath;
    }
    QString folderPath =
        QFileDialog::getExistingDirectory(parent,
                                          theHelpMessage,
                                          browsLoc,
                                          QFileDialog::ShowDirsOnly |
                                              QFileDialog::DontResolveSymlinks);
    // Check if a folder was selected
    if (!folderPath.isEmpty()) {
        theLineEdit->setText(folderPath);
    }
}

void ShipNetSimUI::openAboutPage(ShipNetSim *parent) {
    if (parent->aboutWindow == nullptr) {
        parent->aboutWindow = std::make_shared<AboutWindow>(parent);
    }
    // Check if the initialization was successful
    if (parent->aboutWindow != nullptr) {
        parent->aboutWindow->show();
    }
}

void ShipNetSimUI::handleSampleProject(ShipNetSim *parent) {
    QString executablePath = QCoreApplication::applicationDirPath();
    QString filePath =
        QDir(QDir(executablePath).filePath("sampleProject")).filePath("sampleProject.SNS");
    parent->loadProjectFiles(filePath);
}

void ShipNetSimUI::closeApplication(ShipNetSim *parent) {
    qApp->quit();
}
