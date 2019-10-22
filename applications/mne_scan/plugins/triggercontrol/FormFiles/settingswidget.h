#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
//#include <QtWidgets>
#include <QtSerialPort/QSerialPort>
//#include <QtSerialPort/QSerialPortInfo>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
class SettingsWidget;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TRIGGERCONTROLPLUGIN
//=============================================================================================================

namespace TRIGGERCONTROLPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class TriggerControl;

//=============================================================================================================
/**
* DECLARE CLASS TriggerControlSetupWidget
*
* @brief The TriggerControlSetupWidget class provides the TriggerControlToolbox configuration window.
*/
class SettingsWidget : public QDialog
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a SettingsWidget dialog which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new SettingsWidget becomes a window.
    * If parent is another widget, SettingsWidget becomes a child window inside parent. SettingsWidget is
    *deleted when its parent is deleted.
    */
    explicit SettingsWidget(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the SettingsWidget.
    * All SettingsWidget's children are deleted first. The application exits if SettingsWidget is the main widget.
    */
    ~SettingsWidget();



private slots:
    //=========================================================================================================
    /**
    * Displays information about the serial Port
    *
    */
    void showPortInfo(int idx);

    //=========================================================================================================
    /**
    * Updates the settings and closes the settings widget
    *
    */
    void apply();

private:
    //=========================================================================================================
    /**
    * Adds possible configuration information for the serial port to the combo boxes
    *
    */
    void fillPortsParameters();
    //=========================================================================================================
    /**
    * Checks all available serial ports for information and adds it to the list
    *
    */
    void fillPortsInfo();
    //=========================================================================================================
    /**
    * configures the serial port with the given information from the GUI
    *
    */
    void updateSettings();



    Ui::SettingsWidget *ui;     /**< Holds a pointer to the Settingswidget.*/

};

} // NAMESPACE

#endif // SETTINGSWIDGET_H
