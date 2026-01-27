#include <QMessageBox>
#include <QtAlgorithms>
#include <QDesktopServices>
#include <QFileDialog>
#include <any>
#include <limits>

#include "shipnetsim.h"
#include "../components/nonemptydelegate.h"
#include "../components/numericdelegate.h"
#include "../components/textboxdelegate.h"
#include "gui/components/globalmapmanager.h"
#include "utils/data.h"
#include "../components/checkboxdelegate.h"
#include "../components/comboboxdelegate.h"
#include "../components/textboxbuttondelegate.h"
#include "../windowMangement/shipnetsimui.h"
#include "gui/windowMangement/ui_shipnetsim.h"
#include "ship/shipsList.h"
#include "ship/ship.h"

#include "simulatorapi.h"
#include "utils/guiutils.h"

static QString MAIN_SIMULATION_NAME = "MAIN";


ShipNetSim::ShipNetSim(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ShipNetSim)
{
    ui->setupUi(this);

    OPTIONAL_SHIPS_TABLE_COLUMNS.clear();
    for (int i = 0;
         i < ShipNetSimCore::ShipsList::FileOrderedparameters.size();
         i++)
    {
        if (ShipNetSimCore::ShipsList::FileOrderedparameters[i].isOptional) {
            OPTIONAL_SHIPS_TABLE_COLUMNS.push_back(i);
        }
    }

    setupGenerals();

    setupPage1();
    setupPage2();
    setupPage3();
}

ShipNetSim::~ShipNetSim()
{
    // Terminate any running simulation before destroying the GUI
    // to prevent signals being sent to destroyed objects
    try {
        SimulatorAPI::ContinuousMode::terminateSimulation({"*"});
    } catch (...) {
        // Ignore errors during shutdown
    }

    if (processingWindow) {
        delete processingWindow;
    }
    delete ui;
}

void ShipNetSim::setupGenerals(){

    ui->progressBar->setTextVisible(true);
    ui->progressBar->setAlignment(Qt::AlignCenter);
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setFormat("%p%");
    ui->pushButton_pauseResume->setVisible(false);
    ui->pushButton_pauseResume->setOnOffText("Continue", "Pause");
    ui->pushButton_terminate->setVisible(false);

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
        QString fname = ShipNetSimUI::browseFiles(
            this, nullptr,
            "Open ShipNetSim Project File",
            "ShipNetSim Files (*.STS, *.sts)");
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
                         // switch to the next tab page if it
                         // is not the last page
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

    QObject::connect(ui->pushButton_terminate,
                     &QPushButton::clicked,
                     [=, this]()
                     {
                         this->terminateSimulation();
                     });

    // change next page button text
    QObject::connect(ui->tabWidget_project,
                     &QTabWidget::currentChanged, [=, this](int index) {
                         // check if the last tab page is focused and update
                         // the button text accordingly
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


    SimulatorAPI::ContinuousMode::loadNetwork("Default",
                                              MAIN_SIMULATION_NAME);
    SimulatorAPI::ContinuousMode::createNewSimulationEnvironment(
        MAIN_SIMULATION_NAME, {}, units::time::second_t(1.0), false,
        SimulatorAPI::Mode::Sync);
}

void ShipNetSim::setupPage1() {

    // ------------------------------------------------------------------------
    // ------------------------- Ships Table ----------------------------------
    // ------------------------------------------------------------------------

    // get the ships file
    connect(ui->pushButton_trains, &QPushButton::clicked, [this]() {

        shipsFilename = ShipNetSimUI::browseFiles(this, ui->lineEdit_trains,
                                                  "Select Ships File");
    });

    connect(ui->lineEdit_trains, &QLineEdit::textChanged,
            [=, this](const QString& file)
            {
        // if empty, do nothing
                if (file.trimmed().isEmpty()) {
                    return;
                }

                // check if the file exists first before proceeding
                if (QFile::exists(file)) {

                    // load the ships to the table
                    try {
                    auto out =
                        ShipNetSimCore::ShipsList::
                        readShipsFileToStrings(file);
                    this->loadShipsDataToTables(out);
                    } catch (std::exception &e) {
                        ShipNetSimUI::showWarning(this, e.what());
                    }
                }
                else {

                    // if the file does not exist, show the warning
                    ShipNetSimUI::showWarning(this,
                                              "Ships file does not exist");
                }
            });

    // write the ships file
    connect(ui->pushButton_saveNewShips, &QPushButton::clicked, [this]() {

        QVector<QMap<QString, QString>> out;
        try {
            out = this->getShipsDataFromTables();
        } catch (const std::exception& e) {
            ShipNetSimUI::showErrorBox(e.what());
            return;
        }

        // Open a file dialog to choose the save location
        QString saveFilePath =
            QFileDialog::getSaveFileName(this,
                                         "Save Ships File",
                                         QDir::homePath(),
                                         "DAT Files (*.DAT *.dat)");
        if (!saveFilePath.isEmpty()) {
            QVector<QString> headers = {"This file is autogenerated "
                                        "using ShipNetSimGUI"};
            try {
            if (ShipNetSimCore::ShipsList::writeShipsFile(saveFilePath,
                                                          out, headers))
            {
                ShipNetSimUI::showNotification(
                    this, "Ships file saved successfully!");
                shipsFilename = saveFilePath;
            }
            else {
                ShipNetSimUI::showWarning(this, "Could not save the file!");
            }
            } catch (std::exception &e) {
                ShipNetSimUI::showWarning(this, e.what());
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

    QObject::connect(ui->combo_visualizeShip,
                     QOverload<int>::of(&QComboBox::currentIndexChanged),
                     [this](int index) {

                         if (index == -1) return;  // Skip programmatic changes
                         QString text = ui->combo_visualizeShip->currentText();

                         GlobalMapManager::getInstance()->clearAllHighlights();
                         GlobalMapManager::getInstance()->removeTemporaryPort();

                         auto rows =
                             ui->table_newShips->findRowsWithData(text, 0);
                         if (rows.isEmpty()) return;

                         QTableWidgetItem* item =
                             ui->table_newShips->item(rows[0], 1);
                         if (!item) return;

                         auto points = item->text().split(";",
                                                          Qt::SkipEmptyParts);
                         if (points.isEmpty()) return;
                         for (auto p: points) {
                             if (p.trimmed().isEmpty()) continue;
                             auto pointCords = p.split(",");
                             if (pointCords.size() != 2) continue;

                             bool ok1, ok2;
                             double pc1, pc2;
                             pc1 = pointCords[0].toDouble(&ok1);
                             pc2 = pointCords[1].toDouble(&ok2);
                             if (ok1 && ok2) {
                                 units::angle::degree_t p1 =
                                     units::angle::degree_t(pc1);
                                 units::angle::degree_t p2 =
                                     units::angle::degree_t(pc2);
                                 OGRSpatialReference srs;
                                 srs.SetWellKnownGeogCS("WGS84");
                                 auto referencePoint =
                                     ShipNetSimCore::GPoint(p1, p2, srs);
                                 auto highlighted =
                                     GlobalMapManager::getInstance()->
                                     toggleHighlightNode(referencePoint);

                                 if (!highlighted) {
                                     GlobalMapManager::getInstance()->addTemporaryPort(
                                         referencePoint,
                                         QString("Point on Path of Ship %1").
                                         arg(text)
                                         );
                                 }

                             }
                         }
                     });

}

void ShipNetSim::setupPage2() {

    QList<int> simulatorWidgetSizes;
    simulatorWidgetSizes << 229 << 700;
    ui->splitter_simulator->setSizes(simulatorWidgetSizes);

    // make the trajectory lineedit not visible
    ui->horizontalWidget_TrajFile->setVisible(false);

    // Set default output location (consistent with CLI)
    ui->lineEdit_outputPath->setText(ShipNetSimCore::Utils::getHomeDirectory());

    // select the output location
    connect(ui->pushButton_selectoutputPath, &QPushButton::clicked, [this]() {
        ShipNetSimUI::browseFolder(this,
                                   ui->lineEdit_outputPath,
                                   "Select the output path");
    });

    connect(ui->checkBox_exportTrajectory, &QCheckBox::stateChanged, [this]() {
        ui->horizontalWidget_TrajFile->setVisible(
            ui->checkBox_exportTrajectory->checkState() == Qt::Checked);
    });

}

void ShipNetSim::setupPage3() {
    connect(ui->pushButton_trajectoryViewBrowse, &QPushButton::clicked,
            [this]() {
                ShipNetSimUI::browseFiles(this,
                                  ui->lineEdit_trajectoryViewBrowse,
                                  "Select the trajectory file",
                                  "CSV Files (*.CSV *.csv)");
    });

    connect(ui->lineEdit_trajectoryViewBrowse, &QLineEdit::textChanged,
            [this](QString filePath) {
                handleViewTrajectoryFile(filePath);
    });
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

    ui->spinBox_plotEvery->setValue(1000.0);
    ui->doubleSpinBox_timeStep->setValue(1.0);
    ui->doubleSpinBox->setValue(0.0);
}


void ShipNetSim::setupShipsTable() {

    int i = 0;

    // 1. ID column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NonEmptyDelegate("ID", this));
    QString idToolTip = "The unique identifier of the ship.";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(idToolTip);

    // 2. Path column
    ui->table_newShips->setItemDelegateForColumn(
        i, new TextBoxDelegate(this, "0.0, 0.0; 1.0, 1.0, "));
    QString nodesToolTip = "All the coordinates the ship should path on. "
                           "You can define either the start and end nodes or "
                           "each point the ship must traverse. "
                           "These can be defined in "
                           "the 'Define Ships Path' tab";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(nodesToolTip);

    // 3. Max Speed column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 3, 0.1, 0));
    QString maxSpeedToolTip = "Max speed (knots)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(maxSpeedToolTip);

    // 4. LWL column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 3, 0.1, 0));
    QString lwlToolTip = "Vessel's waterline length (m)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(lwlToolTip);

    // 5. LPP column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 3, 0.1, 0));
    QString lppToolTip = "Length between perpendiculars (m)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(lppToolTip);

    // 6. Beam (B) column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 3, 0.1, 0));
    QString bToolTip = "Beam (m)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(bToolTip);

    // 7. Draft Forward (TF) column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 3, 0.1, 0));
    QString tfToolTip = "Draft at forward (m)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(tfToolTip);

    // 8. Draft Aft (TA) column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 3, 0.1, 0));
    QString taToolTip = "Draft at aft (m)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(taToolTip);

    // 9. Volume (V) column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 3, 0.1, 0));
    QString vToolTip = "Volumetric Displacement (cubic meters)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(vToolTip);

    // 10. Surface Area (S) column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 3, 0.1, 0));
    QString sToolTip = "Hull Surface Area (square meters)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(sToolTip);

    // 11. AV column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 3, 0.1, 0));
    QString avToolTip = "Cargo Vertical Projected Area in motion "
                        "direction (square meters)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(avToolTip);

    // 12. hB column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 3, 0.1, 0));
    QString hbToolTip = "Height of the center of Area of the transverse "
                        "section at the bow (m)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(hbToolTip);

    // 13. ABT column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 3, 0.1, 0));
    QString abtToolTip = "Area of the transverse section at the "
                         "bow (square meters)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(abtToolTip);

    // 14. ABT column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 3, 0.1, 0));
    QString atToolTip = "Area of the transom (square meters)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(atToolTip);

    // 15. iE column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 360.0, 0, 1, 1.0, 0));
    QString ieToolTip = "Entrance angle of the bow (degrees)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(ieToolTip);

    // 16. kS column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000.0, 0, 3, 1.0, 0));
    QString ksToolTip = "Roughness coefficient (μm)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(ksToolTip);

    // 17. lCB column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 100.0, -100.0, 3, 0.10, 0));
    QString lcbToolTip = "Longitudinal center of buoyancy (fraction of LPP)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(lcbToolTip);

    // 18. stern column
    QStringList sternTypes = ShipNetSimCore::Ship::getAllSternTypes();
    ui->table_newShips->setItemDelegateForColumn(
        i, new ComboBoxDelegate(sternTypes, this));
    QString sternToolTip = "Stern type";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(sternToolTip);

    // 19. CM column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1.0, 0, 3, 0.01, 0));
    QString cmToolTip = "Midship section coefficient (fraction)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(cmToolTip);

    // 20. CWP column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1.0, 0, 3, 0.01, 0));
    QString cwpToolTip = "Waterplane area coefficient (fraction)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(cwpToolTip);

    // 21. CP column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1.0, 0, 3, 0.01, 0));
    QString cpToolTip = "Prismatic coefficient (fraction)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(cpToolTip);

    // 22. CB column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1.0, 0, 3, 0.01, 0));
    QString cbToolTip = "Block coefficient (fraction)";
    ui->table_newShips->horizontalHeaderItem(i++)->setToolTip(cbToolTip);

    // 23. Fuel Type column
    QVector<QStringList> formData;

    QStringList fuelTypes = QStringList() << "comboBox";
    fuelTypes << ShipNetSimCore::ShipFuel::getFuelTypeList();
    formData.push_back(fuelTypes);

    QStringList tankSizesDetails= QStringList() << "numericSpin";
    tankSizesDetails << "1000000000.0" << "0.0" << "2" << "100" << "1000.0";
    formData.push_back(tankSizesDetails);

    QStringList tankcapDetails= QStringList() << "numericSpin";
    tankcapDetails << "1.0" << "0.0" << "3" << "0.05" << "0.85";
    formData.push_back(tankcapDetails);
    formData.push_back(tankcapDetails);


    TextBoxButtonDelegate::FormDetails fFuelTypes =
        TextBoxButtonDelegate::FormDetails(
            "Enter Tank Details",
            QStringList() <<"Fuel Type" << "Max Capacity (Liters)"
                          << "Initial Capacity (%)" << "Depth (%)",
            {}, formData);
    ui->table_newShips->setItemDelegateForColumn(
        i, new TextBoxButtonDelegate(TextBoxButtonDelegate::FormType::General,
                                     this, fFuelTypes));
    QString fuelTypesToolTip = "tank details";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(fuelTypesToolTip);


    // 24. Engines Count Per Propeller column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 2, 0, 0, 1.0, 0));
    QString enginesCountToolTip = "Number of engines per propeller";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(enginesCountToolTip);

    // 25. Engine Details for Tier II
    ui->table_newShips->setItemDelegateForColumn(
        i, new TextBoxButtonDelegate(
            TextBoxButtonDelegate::FormType::RPMEfficiency,
            this, TextBoxButtonDelegate::FormDetails(true)));
    QString engineBPRPMToolTip = "Engine edge points definition "
                                 "for Tier II (kW, RPM, #)";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(engineBPRPMToolTip);

    // 26. Engine Details for Tier III
    ui->table_newShips->setItemDelegateForColumn(
        i, new TextBoxButtonDelegate(
            TextBoxButtonDelegate::FormType::RPMEfficiency,
            this, TextBoxButtonDelegate::FormDetails(true)));
    QString engineBPRPMToolTipIII = "Engine edge points definition for Tier "
                                    "III (kW, RPM, #)";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(engineBPRPMToolTipIII);

    // 27. Engine Performance Curve for Tier II
    ui->table_newShips->setItemDelegateForColumn(
        i, new TextBoxButtonDelegate(
            TextBoxButtonDelegate::FormType::RPMEfficiency,
            this, TextBoxButtonDelegate::FormDetails(false)));
    QString engineBPEffToolTip = "Engine brake power to efficiency map"
                                 " for Tier II (kW, %)";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(engineBPEffToolTip);

    // 28. Engine Performance Curve for Tier III
    ui->table_newShips->setItemDelegateForColumn(
        i, new TextBoxButtonDelegate(
            TextBoxButtonDelegate::FormType::RPMEfficiency,
            this, TextBoxButtonDelegate::FormDetails(false)));
    QString engineBPEffTier3ToolTip = "Engine brake power to efficiency map "
                                      "for Tier III (kW, %)";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(engineBPEffTier3ToolTip);

    // 29. Gearbox Ratio column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 100.0, 0, 3, 0.01, 0));
    QString gearboxRatioToolTip = "Gearbox ratio to 1";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(gearboxRatioToolTip);

    // 30. Gearbox Efficiency column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1.0, 0, 3, 0.01, 0));
    QString gearboxEffToolTip = "Gearbox efficiency (%)";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(gearboxEffToolTip);

    // 31. Shaft Efficiency column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1.0, 0, 3, 0.01, 0));
    QString shaftEffToolTip = "Shaft efficiency (%)";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(shaftEffToolTip);

    // 32. Propeller Count column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 100, 0, 0, 1.0, 0));
    QString propellerCountToolTip = "Number of propellers";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(propellerCountToolTip);

    // 33. Propeller Diameter column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 100.0, 0, 3, 0.01, 0));
    QString propellerDiameterToolTip = "Propeller diameter (m)";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(propellerDiameterToolTip);

    // 34. Propeller Pitch column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 100.0, 0, 3, 0.01, 0));
    QString propellerPitchToolTip = "Propeller pitch (m)";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(propellerPitchToolTip);

    // 35. Propeller Blade Count column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 100, 0, 0, 1.0, 0));
    QString propellerBladeCountToolTip = "Number of propeller blades";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(propellerBladeCountToolTip);

    // 36. Propeller Expanded Area Ratio column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1.0, 0, 3, 0.01, 0));
    QString propellerAreaRatioToolTip = "Propeller expanded area ratio";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(propellerAreaRatioToolTip);

    // 37. Stop if No Energy column
    ui->table_newShips->setItemDelegateForColumn(
        i, new CheckboxDelegate(this));
    QString stopIfNoEnergyToolTip = "Stop if there is no energy available?";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(stopIfNoEnergyToolTip);

    // 38. Max Rudder Angle column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 360.0, 0, 1, 1.0, 0));
    QString maxRudderAngleToolTip = "Max rudder angle (degrees)";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(maxRudderAngleToolTip);

    // 39. Vessel Weight column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 3, 1.0, 0));
    QString vesselWeightToolTip = "Vessel weight (ton)";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(vesselWeightToolTip);

    // 40. Cargo Weight column
    ui->table_newShips->setItemDelegateForColumn(
        i, new NumericDelegate(this, 1000000000000.0, 0, 3, 1.0, 0));
    QString cargoWeightToolTip = "Cargo weight (ton)";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(cargoWeightToolTip);

    QStringList appDetails1 = QStringList() << "comboBox";
    appDetails1 << ShipNetSimCore::Ship::getAllAppendageTypes();
    QStringList appDetails2 = QStringList() << "numericSpin";
    appDetails2 << "10000.0" << "0.0" << "3" << "5" << "2.0";
    QVector<QStringList> details;
    details.push_back(appDetails1);
    details.push_back(appDetails2);
    // Appendages Surface Area Map column
    TextBoxButtonDelegate::FormDetails fappText =
        TextBoxButtonDelegate::FormDetails(
            "Add appendages and their corresponding areas (sq. m):",
        QStringList() << "Appendage" << "Area (sq. m)",
            QStringList(), details);
    ui->table_newShips->setItemDelegateForColumn(
        i, new TextBoxButtonDelegate(
            TextBoxButtonDelegate::FormType::General,
            this, fappText));
    QString appendagesToolTip = "Appendages surface area map "
                                "(ID, square meters)";
    ui->table_newShips->horizontalHeaderItem(i++)->
        setToolTip(appendagesToolTip);

    // ---------- insert a new row to Ships ----------
    ui->table_newShips->insertRow(0);
    // add the ship 0 to combo visualize ship
    this->updateCombo_visualizeShips();

    // set the new id count as default value for the first cell of the new row
    std::unique_ptr<QTableWidgetItem> newItemID_ship(
        new QTableWidgetItem(QString::number(1)));
    ui->table_newShips->setItem(0, 0, newItemID_ship.release());

    ui->table_newShips->horizontalHeader()->
        setSectionResizeMode(QHeaderView::ResizeToContents);
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

void ShipNetSim::handleSimulationResultsAvailable(ShipsResults results) {
    auto reportTable =
        ShipNetSimCore::Data::Table::createFromQPairRows<QString, QString>(
            {"Key", "Value"}, results.getSummaryData());
    ui->widget_SummaryReport->createReport(reportTable);

    // this->showDetailedReport(results.getTrajectoryFileName());
}

void ShipNetSim::handleViewTrajectoryFile(QString trajectoryFile)
{
    QFileInfo fileInfo(trajectoryFile);

    // Create processing window if it doesn't exist
    if (!processingWindow) {
        processingWindow = new ProcessingWindow(this);
    }
    processingWindow->show();

    if (fileInfo.exists()) {
        ShipNetSimUI::updateGraphs(
            this, ui->comboBox_shipsResults,
            ui->comboBox_resultsXAxis, trajectoryFile);
    }
}

void ShipNetSim::saveProjectFile(bool saveAs) {
    if (projectFileName.isEmpty() || saveAs) {
        QString saveFilePath =
            ShipNetSimUI::saveFile(this, "Save project",
                                   "ShipNetSim Files (*.STS)");

        if (saveFilePath.isEmpty()) {
            return;
        }
        projectFileName = saveFilePath;
    }

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
        QString dataDir = ShipNetSimCore::Utils::getDataDirectory();

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

        QString dataDirString = "$${DATADIR}";
        shipsFile.replace(dataDirString, dataDir);


        QFile tfile(shipsFile);
        if (tfile.exists()) {
            ui->lineEdit_trains->setText(shipsFile);
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

ShipNetSimCore::GPoint ShipNetSim::findPortCoords(QString portCode) {
    auto ports = ShipNetSimCore::SeaPortLoader::loadFirstAvailableSeaPorts();
    for (const auto &port : ports) {
        if (port->getPortCode() == portCode) {
            return port->getPortCoordinate();
        }
    }

    // Throw an exception if the port is not found
    throw std::invalid_argument(
        QString("Port %1 is not found").arg(portCode).toStdString()
        );
}


QString ShipNetSim::convertFromPortCodesToCoords(QString portsString) {
    QString final = "";
    // Split the string by ","
    QStringList portParts = portsString.split(",");

    for (const QString &port : portParts) {
        QString trimmedPort = port.trimmed();
        try {
        final += findPortCoords(trimmedPort).toString("%x, %y") + ";";
        } catch(std::exception &e) {
            qWarning("%s", e.what());
        }
    }

    return final;
};

QVector<QMap<QString, QString>> ShipNetSim::getShipsDataFromTables()
{
    QVector<QMap<QString, QString>> shipsDetails;

    if (ui->table_newShips->isTableIncomplete(OPTIONAL_SHIPS_TABLE_COLUMNS)) {
        throw std::invalid_argument("Ships Table is empty!");
        return shipsDetails;
    }

    // check if the ships table has any empty cell
    if (ui->table_newShips->hasEmptyCell(OPTIONAL_SHIPS_TABLE_COLUMNS)) {
        throw std::invalid_argument("Ships Table has empty cells!");
        return shipsDetails;
    }

    // read ships
    for (int i = 0; i< ui->table_newShips->rowCount(); i++) {
        if (ui->table_newShips->isRowEmpty(i, OPTIONAL_SHIPS_TABLE_COLUMNS))
        {
            continue;
        }

        QString sternTypeStr = (ui->table_newShips->item(i, 17)
                                   ? ui->table_newShips->item(i, 17)->text().
                                     trimmed() : "NoData");
        QString sternTypeIndex =
            QString::number(ShipNetSimCore::Ship::getAllSternTypes().indexOf(
            sternTypeStr, 0));

        QMap<QString, QString> shipdetails;

        for (int c = 0;  // column / parameter
             c < ShipNetSimCore::ShipsList::FileOrderedparameters.size();
             c++) {
            QString data = ui->table_newShips->item(i, c)
            ? ui->table_newShips->item(i, c)->text().
                trimmed() : "-1";
            QString readingValue =
                ShipNetSimCore::ShipsList::FileOrderedparameters[c].name;
            if (data == "-1" &&
                !ShipNetSimCore::ShipsList::FileOrderedparameters[c].isOptional)
            {
                ErrorHandler::showError(QString("%1 must be provided.").
                                        arg(readingValue));
            }
            else if(data == "-1") {
                data = "NA";
            }

            if (readingValue == "SternShapeParam") {
                data = sternTypeIndex;
            }
            shipdetails[readingValue] = data;
        }

        shipsDetails.push_back(shipdetails);

    }

    if (OPTIONAL_SHIPS_TABLE_COLUMNS.isEmpty()) {
        throw std::invalid_argument("Ships Table is empty!");
    }

    return shipsDetails;
}

void ShipNetSim::loadShipsDataToTables(QVector<QMap<QString, QString>> data)
{
    if (data.empty()) {
        return;
    }
    // clear the current table data
    ui->table_newShips->clearContent();

    // iterate over all ships data
    for (int i = 0; i < data.size(); i++) {
        // if not equal to sorted parameters, show errors
        if (data[i].size() !=
            ShipNetSimCore::ShipsList::FileOrderedparameters.size()) {
            ErrorHandler::showWarning("Data is not fully populated "
                                      "for the ships table");
            return;
        }

        // iterate over all the ship details
        for (int j = 0;
             j < ShipNetSimCore::ShipsList::FileOrderedparameters.size();
             j++)
        {
            QString parameter = ShipNetSimCore::ShipsList::
                                FileOrderedparameters[j].name;
            QString cellData = data[i][parameter];
            if (cellData.contains("NA", Qt::CaseInsensitive)) {
                cellData = "";
            }
            if (parameter == "SternShapeParam") {
                QStringList sternTypes =
                    ShipNetSimCore::Ship::getAllSternTypes();
                bool isNumeric;
                int index = cellData.toInt(&isNumeric);
                if (isNumeric) {
                    cellData = sternTypes[index];
                }
                else {
                    cellData = sternTypes[0];
                }

            }
            QModelIndex index = ui->table_newShips->model()->index(i, j);
            ui->table_newShips->model()->
                setData(index, cellData, Qt::EditRole);
        }
    }
}



void ShipNetSim::simulate() {
    try {
        // Disconnect any existing signal connections from previous simulate() calls
        // to prevent duplicate handlers from accumulating
        disconnect(&SimulatorAPI::ContinuousMode::getInstance(),
                   nullptr, this, nullptr);

        if (SimulatorAPI::ContinuousMode::
            isWorkerBusy(MAIN_SIMULATION_NAME)) {
            ShipNetSimUI::showWarning(this, "Worker is busy, "
                                            "wait a little bit!");
            return;
        }
        if (!SimulatorAPI::ContinuousMode::
            isNetworkLoaded(MAIN_SIMULATION_NAME))
        {
            SimulatorAPI::ContinuousMode::loadNetwork("Default",
                                                      MAIN_SIMULATION_NAME);
            SimulatorAPI::ContinuousMode::createNewSimulationEnvironment(
                MAIN_SIMULATION_NAME, {}, units::time::second_t(1.0), false,
                SimulatorAPI::Mode::Sync);
        }


        QVector<QMap<QString, QString>> shipsRecords;

        // read ships from table and generate instances of ships
        try {
            shipsRecords = this->getShipsDataFromTables();
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

        bool exportAllShipsSummary =
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

        if (SimulatorAPI::ContinuousMode::isWorkerBusy(MAIN_SIMULATION_NAME)) {
            ShipNetSimUI::showNotification(this,
                                           "Simulation is still running!");
            return;
        }

        ui->progressBar->setVisible(true);

        // -------------------------------------------------------------
        // USING ShipNetSim API
        // -------------------------------------------------------------

        // setting simulator definitions
        SimulatorAPI::ContinuousMode::getSimulator(MAIN_SIMULATION_NAME)->
            setTimeStep(units::time::second_t(timeStep));
        // Zero means run until all ships reach destination (no time limit)
        SimulatorAPI::ContinuousMode::getSimulator(MAIN_SIMULATION_NAME)->
            setEndTime(units::time::second_t(
                endTime == 0 ? std::numeric_limits<double>::infinity()
                             : endTime));
        SimulatorAPI::ContinuousMode::getSimulator(MAIN_SIMULATION_NAME)->
            setOutputFolderLocation(exportDir);
        SimulatorAPI::ContinuousMode::getSimulator(MAIN_SIMULATION_NAME)->
            setSummaryFilename(summaryFilename);
        SimulatorAPI::ContinuousMode::getSimulator(MAIN_SIMULATION_NAME)->
            setExportIndividualizedShipsSummary(exportAllShipsSummary);
        SimulatorAPI::ContinuousMode::getSimulator(MAIN_SIMULATION_NAME)->
            setExportInstantaneousTrajectory(exportInta, instaFilename);

        // Reset the simulator state (time, progress, ships) before starting new simulation
        // This is necessary after a previous termination
        SimulatorAPI::ContinuousMode::getSimulator(MAIN_SIMULATION_NAME)->
            restartSimulation();

        QVector<std::shared_ptr<ShipNetSimCore::Ship>> ships =
            SimulatorAPI::loadShips(shipsRecords, MAIN_SIMULATION_NAME);

        // Clear any existing ships from previous simulation runs
        GlobalMapManager::getInstance()->clearAllShips();

        for (auto &ship : ships) {
            GlobalMapManager::getInstance()->createShipNode(
                ship->getUserID(), ship->getCurrentPosition());
        }

        // handle any error that arise from the simulator
        connect(&SimulatorAPI::ContinuousMode::getInstance(),
                &SimulatorAPI::errorOccurred, this,
                [this](QString errorMessage)
                {
                    ShipNetSimUI::handleError(this, errorMessage);

                    // Reset UI state on error
                    ui->pushButton_pauseResume->setVisible(false);
                    ui->pushButton_terminate->setVisible(false);
                    ui->progressBar->setVisible(false);
                    ui->pushButton_projectNext->setEnabled(true);

                    // Terminate the simulation
                    SimulatorAPI::ContinuousMode::terminateSimulation(
                        {MAIN_SIMULATION_NAME});
                }, Qt::QueuedConnection);

        connect(&SimulatorAPI::ContinuousMode::getInstance(),
                &SimulatorAPI::simulationProgressUpdated, this,[this]
                (QString networkName, int progress)
                {
                    if (networkName == MAIN_SIMULATION_NAME) {
                        int prgs = progress;
                        ui->progressBar->setValue(prgs);
                    }
                }, Qt::QueuedConnection);

        connect(&SimulatorAPI::ContinuousMode::getInstance(),
                &SimulatorAPI::simulationFinished, this,
                [this, instaFilename, exportDir](QString networkName)
                {
                    if (networkName != MAIN_SIMULATION_NAME) {
                        return;
                    }
                    // enable the results window
                    ui->tabWidget_project->setTabEnabled(3, true);
                    ui->pushButton_projectNext->setEnabled(true);
                    this->ui->progressBar->setVisible(false);
                    ShipNetSimUI::showNotification(
                        this, "Simulation finished Successfully!");
                    ui->tabWidget_project->setCurrentIndex(3);

                    ui->pushButton_pauseResume->setVisible(false);
                    ui->pushButton_terminate->setVisible(false);

                    // view the trajectory only if trajectory export was enabled
                    if (!instaFilename.isEmpty()) {
                        QString trajectoryFile =
                            GUIUtils::constructFullPath(exportDir,
                                                        instaFilename,
                                                        "csv");
                        ui->lineEdit_trajectoryViewBrowse->setText(trajectoryFile);
                    }
                }, Qt::QueuedConnection);

        // Handle simulation termination (user clicked terminate)
        connect(&SimulatorAPI::ContinuousMode::getInstance(),
                &SimulatorAPI::simulationsTerminated, this,
                [this](QVector<QString> networkNames)
                {
                    if (!networkNames.contains(MAIN_SIMULATION_NAME)) {
                        return;
                    }
                    // Reset UI state
                    ui->pushButton_pauseResume->setVisible(false);
                    ui->pushButton_terminate->setVisible(false);
                    ui->progressBar->setVisible(false);
                    ui->pushButton_projectNext->setEnabled(true);

                    ShipNetSimUI::showNotification(
                        this, "Simulation was terminated by user.");
                }, Qt::QueuedConnection);

        connect(&SimulatorAPI::ContinuousMode::getInstance(),
                &SimulatorAPI::simulationResultsAvailable, this,
                [this](QPair<QString, ShipsResults> results)
                {
                    if (results.first != MAIN_SIMULATION_NAME) {
                        return;
                    }
                    ShipsResults SR = results.second;
                    auto summaryData = SR.getSummaryData();
                    auto summaryDataTable =
                        ShipNetSimCore::Data::Table::
                        createFromQPairRows<QString, QString>(
                            {"Key", "Value"}, summaryData);
                    ui->widget_SummaryReport->createReport(summaryDataTable);
                }, Qt::QueuedConnection);

        if (plotFreq != 0) {
            // replot the ships coordinates
            connect(&SimulatorAPI::ContinuousMode::getInstance(),
                    &SimulatorAPI::shipCoordinatesUpdated, this,
                    [plotFreq](
                        QString shipID,
                        ShipNetSimCore::GPoint currentPosition,
                        units::angle::degree_t heading,
                        QVector<std::shared_ptr<ShipNetSimCore::GLine>> lines)
                    {
                        static QMap<QString, int> updateCounters;

                        // Initialize counter for new ship
                        if (!updateCounters.contains(shipID)) {
                            updateCounters[shipID] = 0;
                        }

                        // Increment counter
                        updateCounters[shipID]++;

                        // Update position only every 100 times
                        if (updateCounters[shipID] >= plotFreq) {
                            GlobalMapManager::getInstance()->
                                updateShipPosition(shipID,
                                                   currentPosition,
                                                   lines);
                            updateCounters[shipID] = 0;  // Reset counter
                        }
                    }, Qt::QueuedConnection);
        }

        // Show the pause button
        ui->pushButton_pauseResume->setVisible(true);
        ui->pushButton_terminate->setVisible(true);

        // disable the simulate button
        this->ui->pushButton_projectNext->setEnabled(false);

        connect(&SimulatorAPI::ContinuousMode::getInstance(),
                &SimulatorAPI::shipsAddedToSimulation, this,
                [](const QString networkName,
                   const QVector<QString> shipIDs) {
                    Q_UNUSED(networkName);
                    Q_UNUSED(shipIDs);
                    SimulatorAPI::ContinuousMode::
                        runSimulation({MAIN_SIMULATION_NAME}, true);
                }, Qt::QueuedConnection);
        SimulatorAPI::ContinuousMode::addShipToSimulation(MAIN_SIMULATION_NAME,
                                                          ships);

    } catch (const std::exception& e) {
        ShipNetSimUI::showErrorBox(e.what());

        // Reset UI state on error
        ui->pushButton_pauseResume->setVisible(false);
        ui->pushButton_terminate->setVisible(false);
        ui->progressBar->setVisible(false);
        ui->pushButton_projectNext->setEnabled(true);
    }
}


void ShipNetSim::closeApplication() {
    // Close the application
    qApp->quit();
}

void ShipNetSim::pauseSimulation() {
    SimulatorAPI::ContinuousMode::pauseSimulation({MAIN_SIMULATION_NAME});
}

void ShipNetSim::resumeSimulation() {
    SimulatorAPI::ContinuousMode::resumeSimulation({MAIN_SIMULATION_NAME});
}

void ShipNetSim::terminateSimulation() {
    SimulatorAPI::ContinuousMode::terminateSimulation({MAIN_SIMULATION_NAME});

    // hide the terminate and pause buttons
    ui->pushButton_pauseResume->setVisible(false);
    ui->pushButton_terminate->setVisible(false);
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
