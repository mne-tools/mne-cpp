//=============================================================================================================
/**
* @file		ecgsetupwidget.h
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Contains the declaration of the ECGSetupWidget class.
*
*/

#ifndef ECGSETUPWIDGET_H
#define ECGSETUPWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtGui/QWidget>
#include "../ui_ecgsetup.h"


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE ECGSimulatorModule
//=============================================================================================================

namespace ECGSimulatorModule
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class ECGSimulator;


//=============================================================================================================
/**
* DECLARE CLASS ECGSetupWidget
*
* @brief The ECGSetupWidget class provides the ECG configuration window.
*/
class ECGSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a ECGSetupWidget which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new ECGSetupWidget becomes a window. If parent is another widget, ECGSetupWidget becomes a child window inside parent. ECGSetupWidget is deleted when its parent is deleted.
    * @param [in] simulator a pointer to the corresponding ECGSimulator.
    */
    ECGSetupWidget(ECGSimulator* simulator, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the ECGSetupWidget.
    * All ECGSetupWidget's children are deleted first. The application exits if ECGSetupWidget is the main widget.
    */
    ~ECGSetupWidget();

    //=========================================================================================================
    /**
    * Initializes the sampling rate and the downsampling factor.
    *
    */
    void initSamplingFactors();

    //=========================================================================================================
    /**
    * Initializes each channel comboBox with given the channel file.
    *
    */
    void initSelectedChannelFile();

    //=========================================================================================================
    /**
    * Initializes the channel check boxes for visibility and whether channels are enabled.
    *
    */
    void initChannelStates();


private slots:

    //=========================================================================================================
    /**
    * Sets the SamplingRate.
    *
    */
    void setSamplingRate(double value);

    //=========================================================================================================
    /**
    * Sets the SamplingRate.
    *
    */
    void setDownsamplingRate(int value);

    //=========================================================================================================
    /**
    * Enables Channel I.
    *
    */
    void setEnabledChannel_I(bool state);

    //=========================================================================================================
    /**
    * Enables Channel II.
    *
    */
    void setEnabledChannel_II(bool state);

    //=========================================================================================================
    /**
    * Enables Channel III.
    *
    */
    void setEnabledChannel_III(bool state);

    //=========================================================================================================
    /**
    * Sets visibility of Channel I.
    *
    */
    void setVisibleChannel_I(bool state);

    //=========================================================================================================
    /**
    * Sets visibility of Channel II.
    *
    */
    void setVisibleChannel_II(bool state);

    //=========================================================================================================
    /**
    * Sets visibility of Channel III.
    *
    */
    void setVisibleChannel_III(bool state);

    //=========================================================================================================
    /**
    * Sets the selected file of Channel I.
    *
    */
    void setFileOfChannel_I();

    //=========================================================================================================
    /**
    * Sets the selected file of Channel II.
    *
    */
    void setFileOfChannel_II();

    //=========================================================================================================
    /**
    * Sets the selected file of Channel III.
    *
    */
    void setFileOfChannel_III();

    //=========================================================================================================
    /**
    * Shows the About Dialog
    *
    */
    void showAboutDialog();


private:
    ECGSimulator*           m_pECGSimulator;	/**< Holds a pointer to corresponding ECGSimulator.*/

    Ui::ECGSetupClass ui;		/**< Holds the user interface for the ECGSetupWidget.*/
};

} // NAMESPACE

#endif // ECGSETUPWIDGET_H
