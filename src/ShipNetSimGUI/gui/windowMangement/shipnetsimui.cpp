#include "shipnetsimui.h"
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include "gui/windowMangement/ui_shipnetsim.h"
#include "utils/guiutils.h"


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
    QColor textColor = parent->palette().color(QPalette::WindowText);
    QString styleSheet = QString("color: %1;").arg(textColor.name());
    parent->ui->label_Notification->setStyleSheet(styleSheet);
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

QString ShipNetSimUI::saveFile(ShipNetSim *parent,
                               const QString& windowTitle,
                               const QString& fileExtensions)
{
    QString saveLoc = parent->userBrowsePath.isEmpty() ?
                          parent->defaultBrowsePath : parent->userBrowsePath;

    QFileDialog dialog(parent);
    dialog.setWindowTitle(windowTitle);
    dialog.setDirectory(saveLoc);
    dialog.setNameFilter(fileExtensions);
    dialog.setAcceptMode(QFileDialog::AcceptSave);  // Set dialog for saving
    dialog.setOption(QFileDialog::DontUseNativeDialog,
                     true);  // Important for OSG compatibility

    QString fname;
    if (dialog.exec() == QDialog::Accepted) {
        fname = dialog.selectedFiles().first();
        if (!fname.isEmpty()) {
            QFileInfo fileInfo(fname);

            // Extract the selected file extension from the filter
            QStringList extensions = fileExtensions.split(";;");
            QString defaultExtension;
            if (!extensions.isEmpty()) {
                QString firstFilter = extensions.first();
                int startIdx = firstFilter.indexOf("(*") + 2;
                int endIdx = firstFilter.indexOf(")");
                if (startIdx > 1 && endIdx > startIdx) {
                    defaultExtension =
                        firstFilter.mid(startIdx, endIdx - startIdx).
                        split(" ").first();
                }
            }

            // Ensure file has an extension
            if (fileInfo.suffix().isEmpty() && !defaultExtension.isEmpty()) {
                fname += defaultExtension;
            }

            parent->userBrowsePath = fileInfo.dir().path();
        }
    }
    return fname;
}

QString ShipNetSimUI::browseFiles(ShipNetSim *parent,
                                  QLineEdit* theLineEdit,
                                  const QString& windowTitle,
                                  const QString& fileExtensions) {
    QString browsLoc = QString();
    if (parent->userBrowsePath.isEmpty()) {
        browsLoc = parent->defaultBrowsePath;
    }
    else {
        browsLoc = parent->userBrowsePath;
    }

    QFileDialog dialog(parent);
    dialog.setWindowTitle(windowTitle);
    dialog.setDirectory(browsLoc);
    dialog.setNameFilter(fileExtensions);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setOption(QFileDialog::DontUseNativeDialog,
                     true);  // Important for OSG compatibility

    QString fname;
    if (dialog.exec() == QDialog::Accepted) {
        fname = dialog.selectedFiles().first();
        if (!fname.isEmpty()) {
            if (theLineEdit) {
                theLineEdit->setText(fname);
            }
            QFileInfo fileInfo(fname);
            parent->userBrowsePath = fileInfo.dir().path();
        }
    }
    return fname;
}

void ShipNetSimUI::browseFolder(ShipNetSim *parent,
                                QLineEdit* theLineEdit,
                                const QString& theHelpMessage) {
    QString browsLoc = QString();
    if (parent->userBrowsePath.isEmpty()) {
        browsLoc = parent->defaultBrowsePath;
    }
    else {
        browsLoc = parent->userBrowsePath;
    }

    QFileDialog dialog(parent);
    dialog.setWindowTitle(theHelpMessage);
    dialog.setDirectory(browsLoc);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly);
    dialog.setOption(QFileDialog::DontResolveSymlinks);
    dialog.setOption(QFileDialog::DontUseNativeDialog,
                     true);  // For OSG compatibility

    if (dialog.exec() == QDialog::Accepted) {
        QString folderPath = dialog.selectedFiles().first();
        if (!folderPath.isEmpty()) {
            theLineEdit->setText(folderPath);
        }
    }
}

void ShipNetSimUI::updateGraphs(ShipNetSim *parent,
                                QComboBox *shipIDsCombobox,
                                QComboBox *axisComboBox,
                                QString trajectoryFilename)
{
    qDebug() << "Trying to plot the trajectory output.";

    // create the processing window if it does not exist
    if (!parent->processingWindow) {
        parent->processingWindow = new ProcessingWindow(parent);
    }

    auto csvReader =
        std::make_shared<ShipNetSimCore::Data::CSV>(trajectoryFilename);

    auto ids =
        csvReader->getDistinctValuesFromCSV(true,
                                            1,
                                            ","); // get all file ship ID's

    shipIDsCombobox->clear();
    shipIDsCombobox->addItem(QString("--"));
    shipIDsCombobox->addItems(ids);

    // hide the processing window
    parent->processingWindow->hide();

    auto updateResultsCurves =
        [csvReader, shipIDsCombobox, axisComboBox, parent]()
    {
        // create the processing window if it does not exist
        if (!parent->processingWindow) {
            parent->processingWindow = new ProcessingWindow(parent);
        }

        auto handleTableDoesNotHaveColumn = [parent](int columnIndex) {
            QString msg =
                QString("Trajectory table does not have column %1")
                    .arg(columnIndex);

            if (parent) {
                showNotification(parent, msg);
            } else {
                qWarning() << "Error: " << msg;
            }
        };

        try {
            auto targetValue = shipIDsCombobox->currentText();
            if (targetValue == "--") {
                return;
            }

            auto selectedShip = csvReader->read(
                true, ",",
                [targetValue](const QString& value) {
                    return value == targetValue;
                },
                1);

            int columnNumber =
                axisComboBox->currentText() == "Distance" ? 11 : 0;
            QString xAxisLabel = axisComboBox->currentText() ==
                                         "Distance" ? "Distance (km)" :
                                     "Time (hr)";
            double xDataFactor = axisComboBox->currentText() ==
                                         "Distance" ? 1.0/1000 : 1.0 / 3600.0;

            if (!selectedShip.hasColumn(columnNumber)) {
                handleTableDoesNotHaveColumn(columnNumber);
                return;
            }
            auto xData =
                GUIUtils::factorQVector(
                    selectedShip.getColumn<double>(columnNumber),
                    xDataFactor);

            if (!selectedShip.hasColumn(13)) {
                handleTableDoesNotHaveColumn(13);
                return;
            }
            auto speeds =
                GUIUtils::factorQVector(
                    selectedShip.getColumn<double>(13),
                    1.94384);

            if (!selectedShip.hasColumn(12)) {
                handleTableDoesNotHaveColumn(12);
                return;
            }
            auto accelerations =
                GUIUtils::factorQVector(
                    selectedShip.getColumn<double>(12),
                    1.94384);

            if (!selectedShip.hasColumn(14)) {
                handleTableDoesNotHaveColumn(14);
                return;
            }
            QVector<double> instantaneousEC;
            auto EC =
                selectedShip.getColumn<double>(14);

            if (!EC.isEmpty()) {
                instantaneousEC.clear();

                // Add first value only if EC is not empty
                instantaneousEC.push_back(EC[0]);
            }

            // Calculate differences between consecutive elements
            for (qsizetype i = 1; i < EC.size(); ++i) {
                double instantaneous = EC[i] - EC[i - 1];
                instantaneousEC.push_back(instantaneous);
            }

            if (!selectedShip.hasColumn(2)) {
                handleTableDoesNotHaveColumn(2);
                return;
            }
            auto salinity = selectedShip.getColumn<double>(2);

            if (!selectedShip.hasColumn(3)) {
                handleTableDoesNotHaveColumn(3);
                return;
            }
            auto waveheight = selectedShip.getColumn<double>(3);

            if (!selectedShip.hasColumn(4)) {
                handleTableDoesNotHaveColumn(4);
                return;
            }
            auto waveFreq = selectedShip.getColumn<double>(4);

            if (!selectedShip.hasColumn(5)) {
                handleTableDoesNotHaveColumn(5);
                return;
            }
            auto waveLen = selectedShip.getColumn<double>(5);

            if (!selectedShip.hasColumn(9)) {
                handleTableDoesNotHaveColumn(9);
                return;
            }
            auto resistance =
                GUIUtils::factorQVector(
                    selectedShip.getColumn<double>(9),
                    1.0/1000.0);

            if (!xData.isEmpty() && !speeds.isEmpty()) {
                parent->ui->plot_trajectory_speed->drawLineGraph(
                    xData, speeds, xAxisLabel, "Knots", "Speed", 0);
            }

            if (!xData.isEmpty() && !accelerations.isEmpty()) {
                parent->ui->plot_trajectory_acceleration->drawLineGraph(
                    xData, accelerations, xAxisLabel, "m/s^2",
                    "Acceleration", 0);
            }

            if (!xData.isEmpty() && !resistance.isEmpty()) {
                parent->ui->plot_trajectory_total_resistance->drawLineGraph(
                    xData, resistance, xAxisLabel, "kN", "Resistance", 0);
            }

            if (!xData.isEmpty() && !instantaneousEC.isEmpty()) {
                parent->ui->plot_trajectory_EC->drawLineGraph(
                    xData, instantaneousEC, xAxisLabel,
                    "kWh", "Energy Consumption", 0);
            }

            if (!xData.isEmpty() && !salinity.isEmpty()) {
                parent->ui->plot_forces_waterSalinity->drawLineGraph(
                    xData, salinity, xAxisLabel, "ppt",
                    "Salinity", 0);
            }

            if (!xData.isEmpty() && !waveFreq.isEmpty()) {
                parent->ui->plot_forces_waveFrequency->drawLineGraph(
                    xData, waveFreq, xAxisLabel, "hz",
                    "Wave Frequency", 0);
            }

            if (!xData.isEmpty() && !waveheight.isEmpty()) {
                parent->ui->plot_forces_waveHeight->drawLineGraph(
                    xData, waveheight, xAxisLabel, "m",
                    "Wave Height", 0);
            }

            if (!xData.isEmpty() && !waveLen.isEmpty()) {
                parent->ui->plot_forces_waveLength->drawLineGraph(
                    xData, waveLen, xAxisLabel, "m",
                    "Wave Length", 0);
            }

            parent->processingWindow->hide();

        } catch (std::exception &e) {
            if (parent) {
                showNotification(parent, e.what());
                parent->processingWindow->hide();
            } else {
                qWarning() << "Error: " << e.what();
            }
        }

    };

    ShipNetSim::connect(shipIDsCombobox,
                        &QComboBox::currentTextChanged, updateResultsCurves);
    ShipNetSim::connect(axisComboBox,
                        &QComboBox::currentTextChanged, updateResultsCurves);
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

    QString filePath = ShipNetSimCore::Utils::getDataFile("sampleProject.sns");
    parent->loadProjectFiles(filePath);
}

void ShipNetSimUI::closeApplication(ShipNetSim *parent) {
    qApp->quit();
}
