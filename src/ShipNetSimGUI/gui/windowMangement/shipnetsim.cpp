#include <QMessageBox>
#include <QtAlgorithms>
#include <QDesktopServices>
#include <QFileDialog>
#include <any>

#include "shipnetsim.h"
#include "../components/nonemptydelegate.h"
#include "../components/numericdelegate.h"
#include "../components/textboxdelegate.h"
#include "../utils/data.h"
#include "ShipNetSimGUI/gui/windowMangement/shipnetsimui.h"
#include "gui/windowMangement/ui_shipnetsim.h"
#include "ship/shipsList.h"

ShipNetSim::ShipNetSim(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ShipNetSim)
{
    ui->setupUi(this);

    setupGenerals();
}

ShipNetSim::~ShipNetSim()
{
    delete ui;
}

void ShipNetSim::setupGenerals(){

    ui->progressBar->setTextVisible(true);
    ui->progressBar->setAlignment(Qt::AlignCenter);
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setFormat("%p%");
    ui->pushButton_pauseResume->setVisible(false);
    ui->pushButton_pauseResume->setOnOffText("Continue", "Pause");

    this->userBrowsePath = QString();

    // show about window
    connect(ui->actionAbout_ShipNetSim, &QAction::triggered, [this](){
        ShipNetSimUI::openAboutPage(this);
    });

    // show the manual
    connect(ui->actionOpen_user_manual, &QAction::triggered, [this]() {
        ShipNetSimUI::openManual(this);
    });

    //show the settings window
    connect(ui->actionSettings, &QAction::triggered, [this]() {
        ShipNetSimUI::openSettingsPage(this);
    });

    // ########################################################################
    // ########## Connect signals and slots for project management ############
    // ########################################################################

    // create a new project and clear the form
    connect(ui->actionCreate_a_new_project, &QAction::triggered, this,
            &ShipNetSim::clearForm);

    // save the project file
    connect(ui->actionSave, &QAction::triggered, [this](){
        this->saveProjectFile();
    });

    // save as the project file
    connect(ui->actionSave_as, &QAction::triggered, [this](){
        this->saveProjectFile(true);
    });

    // load a project file
    connect(ui->actionOpen_an_existing_project, &QAction::triggered, [this](){
        QString fname = QFileDialog::getOpenFileName(
            nullptr, "Open NeTrainSim Project File", "",
            "NeTrainSim Files (*.STS)");
        this->loadProjectFiles(fname);
    });

    // close the application when the exit app is clicked
    connect(ui->actionExit, &QAction::triggered, this,
            &ShipNetSim::closeApplication);

    // open the sample project
    connect(ui->actionLoad_Sample_Project, &QAction::triggered,
            this, [this] () {
        ShipNetSimUI::handleSampleProject(this);
    });

    // define the next page and simulate buttons
    QObject::connect(ui->pushButton_projectNext,
                     &QPushButton::clicked,
                     [=, this]()
                     {
                         // switch to the next tab page if it is not the last page
                         int nextIndex =
                             ui->tabWidget_project->currentIndex() + 1;
                         if (nextIndex < ui->tabWidget_project->count() - 1) {
                             ui->tabWidget_project->setCurrentIndex(nextIndex);
                         }
                         if (nextIndex == ui->tabWidget_project->count() - 1) {
                             this->simulate();
                         }
                     });

    QObject::connect(ui->pushButton_pauseResume,
                     &QPushButton::clicked,
                     [=, this]()
                     {
                         if (ui->pushButton_pauseResume->isToggled()) {
                             this->pauseSimulation();
                         }
                         else {
                             this->resumeSimulation();
                         }

                     });

    // change next page button text
    QObject::connect(ui->tabWidget_project,
                     &QTabWidget::currentChanged, [=, this](int index) {
                         // check if the last tab page is focused and update the button text accordingly
                         if (index == ui->tabWidget_project->count() - 2) {
                             ui->pushButton_projectNext->setText("Simulate");
                             ui->pushButton_projectNext->setVisible(true);
                         }
                         else if (index <= ui->tabWidget_project->count() - 2) {
                             ui->pushButton_projectNext->setText("Next");
                             ui->pushButton_projectNext->setVisible(true);
                         }
                         else {
                             ui->pushButton_projectNext->setVisible(false);
                         }
                     });



    // show error if tables has only 1 row
    connect(ui->table_newShips, &CustomTableWidget::cannotDeleteRow,
            [this]() {
                ShipNetSimUI::showWarning(this,
                                  "Cannot delete the first row!");
            });
}

void ShipNetSim::setupPage1() {

    // ------------------------------------------------------------------------
    // ------------------------- Ships Table ----------------------------------
    // ------------------------------------------------------------------------

    // get the trains file
    connect(ui->pushButton_trains, &QPushButton::clicked, [this]() {
        shipsFilename = ShipNetSimUI::browseFiles(this, ui->lineEdit_trains,
                                                  "Select Trains File");
    });

    connect(ui->lineEdit_trains, &QLineEdit::textChanged,
            [=, this](const QString& file)
            {
                auto out = ShipNetSimCore::ShipsList::readShipsFile(file);
                this->loadShipsDataToTables(out);
            });

    // write the trains file
    connect(ui->pushButton_saveNewShips, &QPushButton::clicked, [this](){
        // Open a file dialog to choose the save location
        QString saveFilePath =
            QFileDialog::getSaveFileName(this,
                                         "Save Ships File",
                                         QDir::homePath(),
                                         "DAT Files (*.DAT)");

        if (!saveFilePath.isEmpty()) {
            QVector<QMap<QString, std::any>> out;
            try {
                out = this->getShipsDataFromTables();
            } catch (const std::exception& e) {
                ShipNetSimUI::showErrorBox(e.what());
                return;
            }

            if (ShipNetSimCore::ShipsList::writeShipsFile(saveFilePath, out))
            {
                ShipNetSimUI::showNotification(
                    this, "Ships file saved successfully!");
                shipsFilename = saveFilePath;
            }
            else {
                ShipNetSimUI::showWarning(this, "Could not save the file!");
            }
        }
    });




    QObject::connect(ui->table_newShips, &QTableWidget::cellChanged,
                     this, &::ShipNetSim::updateCombo_visualizeShips);

    this->setupShipsTable();

    // create a slot to add a new row to the QTableWidget
    auto addRowToNewTrain = [=, this]() {
        // check if the last row has been edited
        if (ui->table_newShips->currentRow() ==
            ui->table_newShips->rowCount() - 1) {
            // add a new row to the QTableWidget
            int newRow = ui->table_newShips->rowCount();
            ui->table_newShips->insertRow(newRow);

            // set the new id count as default value for the first
            // cell of the new row
            int uniqueID = ui->table_newShips->generateUniqueID();
            std::unique_ptr<QTableWidgetItem> newItemID(
                new QTableWidgetItem(QString::number(uniqueID)));
            ui->table_newShips->setItem(newRow, 0, newItemID.release());

        }
    };
    // add a new row everytime the last row cells are edited
    QObject::connect(ui->table_newShips, &QTableWidget::cellChanged,
                     addRowToNewTrain);

    QObject::connect(ui->table_newShips,
                     &CustomTableWidget::tableCleared,
                     addRowToNewTrain);

}


void ShipNetSim::clearForm() {
    QList<QLineEdit *> lineEdits = findChildren<QLineEdit *>();
    // Clear the contents of each QLineEdit
    for (QLineEdit *lineEdit : lineEdits) {
        lineEdit->clear();
    }

    QList<QCheckBox *> checkboxes = findChildren<QCheckBox *>();
    for (QCheckBox * check: checkboxes) {
        check->setCheckState(Qt::CheckState::Unchecked);
    }
    QList<CustomTableWidget *> tables = findChildren<CustomTableWidget *>();
    for (CustomTableWidget * table: tables) {
        // Remove all rows
        table->setRowCount(0);
    }

    this->setupShipsTable();
}


void ShipNetSim::setupShipsTable() {

    int i = 0;
    // set the delegates for the ships table IDs
    ui->table_newShips->setItemDelegateForColumn(
        i, new NonEmptyDelegate("ID", this));
    QString idToolTip = "The unique identifier of the train.";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(idToolTip);

    ui->table_newShips->
        setItemDelegateForColumn(i, new TextBoxDelegate(this, "1,2"));
    QString nodesToolTip = "All the coordinates the ship should path on. "
                           "You can define either the start and end nodes or "
                           "each point the ship must traverse. "
                           "These can be defined in "
                           "the 'Define Ships Path' tab";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(nodesToolTip);

    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 1, 100,0));
    QString maxSpeed = "Max speed (knots)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(maxSpeed);

    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 1, 100,0));
    QString lwl = "Vessels water line length (m)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(lwl);

    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 1, 100,0));
    QString lpp = "Length between perpendiculars (m)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(lpp);

    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 1, 100,0));
    QString b = "Beam (m)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(b);

    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 1, 100,0));
    QString da = "Draft at aft (m)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(da);

    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 1, 100,0));
    QString df = "Draft at forward (m)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(df);

    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 1, 100,0));
    QString v = "Volumetric Desplacement (c. m)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(v);

    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 1, 100,0));
    QString s = "Hull Surface Area (sq. m)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(s);




    // ui->table_newShips->setItemDelegateForColumn(
    //     4, new IntNumericDelegate(this, 300, 1, 1, 1));
    // QString locosToolTip = "Total number of locomotives in the train.";
    // ui->table_newShips->horizontalHeaderItem(4)->setToolTip(locosToolTip);

    // ui->table_newShips->setItemDelegateForColumn(
    //     5, new IntNumericDelegate(this, 300, 1, 1, 1));
    // QString carsToolTip = "Total number of cars in the train.";
    // ui->table_newShips->horizontalHeaderItem(5)->setToolTip(carsToolTip);

    // ui->table_newShips->setItemDelegateForColumn(
    //     6, new NumericDelegate(this, 1, 0, 2, 0.05, 0.95));
    // QString fricToolTip = "The friction coefficient of the train's "
    //                       "wheels with the track.";
    // ui->table_newShips->horizontalHeaderItem(6)->setToolTip(fricToolTip);

    // ---------- insert a new row to Ships ----------
    ui->table_newShips->insertRow(0);
    // add the train 0 to combo visualize Train
    this->updateCombo_visualizeShips();

    // set the new id count as default value for the first cell of the new row
    std::unique_ptr<QTableWidgetItem> newItemID_train(
        new QTableWidgetItem(QString::number(1)));
    ui->table_newShips->setItem(0, 0, newItemID_train.release());
}

// update the combo_visualizeTrain
void ShipNetSim::updateCombo_visualizeShips() {
    // Clear the current items in the combobox
    ui->combo_visualizeShip->clear();

    // Iterate through each row in the table and add the column
    // value to the combobox
    for (int row = 0; row < ui->table_newShips->rowCount(); ++row) {
        QTableWidgetItem* item = ui->table_newShips->item(row, 0);
        if (item) {
            ui->combo_visualizeShip->addItem(item->text());
        }
    }
};


void ShipNetSim::on_tabWidget_results_currentChanged(int index)
{
    if (index == 1)
    {
        // ui->plot_ships->setLoadedModelToWidget(earthResults.second);
    }
}


void ShipNetSim::saveProjectFile(bool saveAs) {
    if (projectFileName.isEmpty() || saveAs) {
        QString saveFilePath =
            QFileDialog::getSaveFileName(this, "Save Project",
                                         QDir::homePath(),
                                         "NeTrainSim Files (*.STS)");
        if (saveFilePath.isEmpty()) {
            return;
        }
        projectFileName = saveFilePath;
    }

    //    try {
    projectName =
        (!ui->lineEdit_projectName->text().trimmed().
          isEmpty()) ? ui->lineEdit_projectName->text().
              trimmed() : "Not Defined";
    author =
        (!ui->lineEdit_createdBy->text().trimmed().
          isEmpty()) ? ui->lineEdit_createdBy->text().
              trimmed() : "Not Defined";
    networkName =
        (!ui->lineEdit_networkName->text().trimmed().
          isEmpty()) ? ui->lineEdit_networkName->text().
              trimmed() : "Not Defined";
    QString simulationEndTime =
        QString::number(std::max(ui->doubleSpinBox->text().
                                 trimmed().toDouble(), 0.0));
    QString simulationTimestep =
        QString::number(std::max(
            ui->doubleSpinBox_timeStep->text().trimmed().toDouble(), 0.1));
    QString simulationPlotTime =
        QString::number(ui->doubleSpinBox->text().trimmed().toDouble());


    if (shipsFilename.isEmpty()) {
        ShipNetSimUI::showWarning(this, "Save ships file first!");
        return;
    }

    ShipNetSimCore::Data::ProjectFile::ProjectDataFile pf;
    pf.projectName   = projectName;
    pf.authorName    = author;
    pf.networkName   = networkName;
    pf.simEndTime    = simulationEndTime;
    pf.simTimestep   = simulationTimestep;
    pf.simPlotTime   = simulationPlotTime;
    pf.shipsFileName = shipsFilename;


    ShipNetSimCore::Data::ProjectFile::createProjectFile(pf, projectFileName);

    ShipNetSimUI::showNotification(this, "File Saved Successfully");
}

void ShipNetSim::loadProjectFiles(QString projectFilename) {
    QFile pfn(projectFilename);
    if (! pfn.exists()) {
        ShipNetSimUI::showWarning(this, "Project file does not exist!");
        return;
    }
    if (!projectFilename.isEmpty()) {
        QFileInfo fileInfo(projectFilename);
        QString parentDirPath = fileInfo.dir().absolutePath();

        QString executableDirectory = QApplication::applicationDirPath();

        auto out =
            ShipNetSimCore::Data::ProjectFile::
            readProjectFile(projectFilename);

        ui->lineEdit_projectName->setText(out.projectName);
        ui->lineEdit_networkName->setText(out.networkName);
        ui->lineEdit_createdBy->setText(out.authorName);
        QString shipsFile = out.shipsFileName;
        QString PWDString = "$${PWD}";
        shipsFile.replace(PWDString, parentDirPath);

        QString ExecutableString = "$${EXE}";
        shipsFile.replace(ExecutableString, executableDirectory);


        QFile tfile(shipsFile);
        if (tfile.exists()) {
            ui->lineEdit_trains->setText(shipsFile);
            //ui->checkBox_TrainsOD->setCheckState(Qt::Unchecked);
        }
        else {
            ShipNetSimUI::showWarning(this, "ships file does not exist");
            return;
        }
        bool ok;
        double simTime = out.simEndTime.toDouble(&ok);
        if (ok) {
            ui->doubleSpinBox->setValue(simTime);
        }
        else {
            QMessageBox::warning(this, "Error",
                                 "Wrong Project File Structure!");
            return;
        }
        double simTimestep = out.simTimestep.toDouble(&ok);
        if (ok) {
            ui->doubleSpinBox_timeStep->setValue(simTimestep);
        }
        else {
            QMessageBox::warning(this, "Error",
                                 "Wrong Project File Structure!");
            return;
        }
        double simTimestepPlot = out.simPlotTime.toDouble(&ok);
        if (ok) {
            ui->spinBox_plotEvery->setValue(simTimestepPlot);
        }
        else {
            QMessageBox::warning(this, "Error",
                                 "Wrong Project File Structure!");
            return;
        }

    }
}

QVector<QMap<QString, std::any>> ShipNetSim::getShipsDataFromTables()
{
    QVector<QMap<QString, std::any>> ships;

    if (ui->table_newShips->isTableIncomplete({})) {
        throw std::invalid_argument("Locomotives Table is empty!");
        return ships;
    }

    // read ships
    for (int i = 0; i< ui->table_newShips->rowCount(); i++) {
        if (ui->table_newShips->isRowEmpty(i, {})) { continue; }

        QMap<QString, std::any> shipdetails;







    }

    return ships;
}

void ShipNetSim::loadShipsDataToTables(QVector<QMap<QString, std::any>> data)
{

}



void ShipNetSim::simulate() {
    try {

        QVector<QMap<QString, std::any>> shipRecords;

        // read ships from table and generate instances of ships
        try {
            shipRecords = this->getShipsDataFromTables();
        } catch (const std::exception& e) {
            ShipNetSimUI::showErrorBox(e.what());
            return;
        }

        QString exportDir = "";
        if (! ui->lineEdit_outputPath->text().trimmed().isEmpty()) {
            exportDir = ui->lineEdit_outputPath->text().trimmed();
        }
        else {
            ShipNetSimUI::showWarning(this, "Export directory is not set!");
            return;
        }

        QString summaryFilename = "";
        if (! ui->lineEdit_summaryfilename->text().trimmed().isEmpty()) {
            summaryFilename =
                ui->lineEdit_summaryfilename->text().trimmed();
        }
        else {
            ShipNetSimUI::showWarning(this, "Summary filename is not set!");
            return;
        }

        bool exportAllTrainsSummary =
            ui->checkBox_detailedTrainsSummay->checkState() == Qt::Checked;
        bool exportInta =
            ui->checkBox_exportTrajectory->checkState() == Qt::Checked;

        QString instaFilename = "";
        if (! ui->lineEdit_trajectoryFilename->text().trimmed().isEmpty() &&
            exportInta) {
            instaFilename =
                ui->lineEdit_trajectoryFilename->text().trimmed();
        }
        else if (exportInta &&
                 ui->lineEdit_trajectoryFilename->text().trimmed().isEmpty())
        {
            ShipNetSimUI::showWarning(this, "Summary filename is not set!");
            return;
        }
        else {
            instaFilename = "";
        }


        QString netName =
            ui->lineEdit_networkName->text().
                trimmed().isEmpty() ? "Not Defined" :
                ui->lineEdit_networkName->text().
                trimmed();
        double endTime = ui->doubleSpinBox->value();
        double timeStep = ui->doubleSpinBox_timeStep->value();
        int plotFreq = ui->spinBox_plotEvery->value();

        if (this->thread == nullptr) {
            this->thread = new QThread(this);
        }

        if (this->thread->isRunning()) {
            ShipNetSimUI::showNotification(this,
                                           "Simulation is still running!");
            return;
        }

        ui->progressBar->setVisible(true);
        worker = new SimulationWorker(WATER_BOUNDRIES_FILE,
                                      shipRecords,
                                      netName,
                                      units::time::second_t(endTime),
                                      units::time::second_t(timeStep),
                                      plotFreq,
                                      exportDir,
                                      summaryFilename,
                                      exportInta,
                                      instaFilename,
                                      exportAllTrainsSummary);

        if (worker == nullptr) {
            ShipNetSimUI::showErrorBox("Error!");
            return;
        }

        // handle any error that arise from the simulator
        connect(worker, &SimulationWorker::errorOccurred, this,
            [this]( QString errorMessage) {
            ShipNetSimUI::handleError(this, errorMessage);
        });

        // update the progress bar
        connect(worker, &SimulationWorker::simulaionProgressUpdated,
                ui->progressBar, &QProgressBar::setValue);

        // Connect the operationCompleted signal to a slot in your GUI class
        connect(worker, &SimulationWorker::simulationFinished, this,
                &::ShipNetSim::pauseSimulation);

        // replot the ships coordinates
        connect(worker, &SimulationWorker::shipsCoordinatesUpdated, this,
                [this](QVector<
                       std::pair<QString, ShipNetSimCore::GPoint>>
                           shipsStartEndPoints)
                {
                    //updateshipsPlot(shipsStartEndPoints);
                });

        // hide the pause button
        ui->pushButton_pauseResume->setVisible(true);

        // move the worker to the new thread
        worker->moveToThread(thread);

        // disable the simulate button
        connect(thread, &QThread::started, this, [=, this]() {
            this->ui->pushButton_projectNext->setEnabled(false);
        });
        // connect the do work to thread start
        connect(thread, &QThread::started, worker, &SimulationWorker::doWork);

        // start the simulation
        thread->start();


    } catch (const std::exception& e) {
        ShipNetSimUI::showErrorBox(e.what());
        // hide the pause button
        ui->pushButton_pauseResume->setVisible(false);
    }
}


void ShipNetSim::closeApplication() {
    // Close the application
    qApp->quit();
}

void ShipNetSim::pauseSimulation() {
    QMetaObject::invokeMethod(
        worker->sim, "pauseSimulation", Qt::QueuedConnection);
}

void ShipNetSim::resumeSimulation() {
    QMetaObject::invokeMethod(
        worker->sim, "resumeSimulation", Qt::QueuedConnection);
}

void ShipNetSim::loadDefaults() {
    QString executablePath = QCoreApplication::applicationDirPath();
    QString iniFilePath = QDir(executablePath).filePath("config.ini");

    QFile pfn(iniFilePath);
    if (!pfn.exists()) {
        ShipNetSimUI::showWarning(this, "Config file does not exist!");
        return;
    }

    this->configManager = new ConfigurationManager(iniFilePath);

    // Read all configurations in the INI file
    QStringList allKeys = configManager->getConfigKeys("");

    // set all the default settings
    foreach (const QString& fullKey, allKeys) {
        QStringList keyParts = fullKey.split("/");
        QString section = keyParts.first();
        QString key = keyParts.last();

        if (key == "browseLocation") {
            QString value = configManager->getConfigValue(section, key);
            this->defaultBrowsePath = value;
        }
    }
}

bool ShipNetSim::saveDefaults(QStringList defaults) {
    // Loop through the list of default configurations
    foreach (const QString& config, defaults) {
        // Split the configuration string into section, key, and value
        QStringList parts = config.split('.');
        if (parts.size() != 2) {
            // Invalid configuration format, return false
            return false;
        }

        QString section = parts[0].trimmed();
        QString keyValue = parts[1].trimmed();

        // Split the key-value pair
        QStringList keyValueParts = keyValue.split('=');
        if (keyValueParts.size() != 2) {
            // Invalid configuration format, return false
            return false;
        }

        QString key = keyValueParts[0].trimmed();
        QString value = keyValueParts[1].trimmed();

        // Save the default configuration
        configManager->setConfigValue(section, key, value);
    }

    return true;
}
