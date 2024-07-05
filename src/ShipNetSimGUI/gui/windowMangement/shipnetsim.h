#ifndef SHIPNETSIM_H
#define SHIPNETSIM_H

#include <QMainWindow>
#include <any>
#include <osgEarth/ExampleResources>
#include "../components/simulationworker.h"
#include "aboutwindow.h"
#include "settingswindow.h"
#include "../../utils/configurationmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ShipNetSim; }
QT_END_NAMESPACE


static QString WATER_BOUNDRIES_FILE = "$${PWD}/ne_110m_ocean.shp";

class ShipNetSim : public QMainWindow
{
    Q_OBJECT

    friend class ShipNetSimUI;

public:
    ShipNetSim(QWidget *parent = nullptr);
    ~ShipNetSim();

    /**
     * @brief load default settings
     *
     * @author	Ahmed Aredah
     * @date	7/1/2023
     */
    void loadDefaults();

    /**
     * @brief NeTrainSim::save Default values
     * @param defaults
     * @return
     */
    bool saveDefaults(QStringList defaults);

    /**
    * Displays a warning message with the given text.
    * @param text The text of the warning message to be displayed.
    *
    * @author Ahmed Aredah
    * @date 6/7/2023
    */
    void showWarning(QString text);

    /**
    * Displays a notification with the given text for 3000 millisecond.
    * @param text The text of the notification to be displayed.
    *
    * @author Ahmed Aredah
    * @date 6/7/2023
    */
    void showNotification(QString text);

private:
    /**
     * Sets up the generals
     *
     * @author	Ahmed Aredah
     * @date	2/28/2023
     */
    void setupGenerals();

    /**
    * Sets up the ships table in the UI.
    * This function initializes and populates the ships table with the
    * appropriate columns and data.
    */
    void setupShipsTable();

    /**
     * @brief this function saves the project file on the hard drive.
     * @param saveAs a boolean to either overwrite the previously saved
     * file or create a new one. The default value is to overwrite if the
     * file existed before.
     */
    void saveProjectFile(bool saveAs = false);

    /**
     * @brief load the Project Files to the simulator GUI
     * @param projectFilename the file SNS that has the files address
     *
     * @author Ahmed Aredah
     * @date 6/7/2023
     */
    void loadProjectFiles(QString projectFilename);


    /**
    * Initiates the simulation process.
    * This function starts the simulation based on the selected trains
    * data and updates the UI accordingly.
    */
    void simulate();

    /**
     * @brief get all ships details from the ships table.
     *
     * @return a vector of maps of ships parameters.
     */
    QVector<QMap<QString, std::any>> getShipsDataFromTables();

    void loadShipsDataToTables(QVector<QMap<QString, std::any>> data);

    void setupPage1();


private slots:
    void on_tabWidget_results_currentChanged(int index);

    /**
     * Slot for clearing the form.
     *
     * @author	Ahmed Aredah
     * @date	6/7/2023
     */
    void clearForm();

    /**
     * Slot for updating the 'visualize trains' combo box.
     *
     * @author	Ahmed Aredah
     * @date	6/7/2023
     */
    void updateCombo_visualizeShips();

    // /**
    //  * Slot for handling errors that occur during the simulation.
    //  *
    //  * @param error     The error message.
    //  *
    //  * @author	Ahmed Aredah
    //  * @date	6/7/2023
    //  */
    // void handleError(QString error);


public slots:
    /**
     * Slot for closing the application.
     *
     * @author	Ahmed Aredah
     * @date	6/7/2023
     */
    void closeApplication();


    // /**
    //  * @brief Slot for handling loading the sample project files
    //  *
    //  * @author	Ahmed Aredah
    //  * @date	6/7/2023
    //  */
    // void handleSampleProject();

    /**
     * @brief Slot for pausing the simulation
     */
    void pauseSimulation();

    /**
     * @brief Slot for resuming the simulation
     */
    void resumeSimulation();

private:
    Ui::ShipNetSim *ui;

    // String to store the project name.
    QString projectName;

    // String to store the network name.
    QString networkName;

    // String to store the author's name.
    QString author;

    // String to store the filename of the project file.
    QString projectFileName;

    // String to store the filename of the trains data.
    QString shipsFilename;

    // Pointer to the SimulationWorker object used for running the
    // simulation in a separate thread.
    SimulationWorker* worker = nullptr;

    // Pointer to the QThread object that is used for running
    // the simulation worker.
    QThread* thread = nullptr;

    // Pointer to the AboutWindow object used to display information
    // about the application.
    std::shared_ptr<AboutWindow> aboutWindow = nullptr;

    // Pointer to the settings window used to display and edit settings of
    // the application.
    std::shared_ptr<settingsWindow> theSettingsWindow = nullptr;


    ConfigurationManager* configManager;




    // std::pair<osg::ref_ptr<osgEarth::MapNode>,
    //           osg::ref_ptr<osg::Group>> earthResults;
public:
    /**
     * @brief default Browse Path
     */
    QString defaultBrowsePath;

    QString userBrowsePath;

    QList<QString> defaultUnits;

};
#endif // SHIPNETSIM_H
