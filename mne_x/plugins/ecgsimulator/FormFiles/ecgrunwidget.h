//=============================================================================================================
/**
* @file		ecgrunwidget.h
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
* @brief	Contains the declaration of the ECGRunWidget class.
*
*/

#ifndef ECGRUNWIDGET_H
#define ECGRUNWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_ecgrun.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


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
* DECLARE CLASS ECGRunWidget
*
* @brief The ECGRunWidget class provides the ECG configuration window for the run mode.
*/
class ECGRunWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a ECGRunWidget which is a child of parent.
    *
    * @param [in] simulator a pointer to the corresponding ECG Simulator.
    * @param [in] parent pointer to parent widget; If parent is 0, the new ECGRunWidget becomes a window. If parent is another widget, ECGRunWidget becomes a child window inside parent. ECGRunWidget is deleted when its parent is deleted.
    */
    ECGRunWidget(ECGSimulator* simulator, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the ECGRunWidget.
    * All ECGRunWidget's children are deleted first. The application exits if ECGRunWidget is the main widget.
    */
    ~ECGRunWidget();

private slots:
    //=========================================================================================================
    /**
    * Shows the About Dialog
    *
    */
    void showAboutDialog();

private:
    ECGSimulator* m_pECGSimulator;	/**< Holds a pointer to corresponding ECGSimulator.*/

    Ui::ECGRunClass ui;		/**< Holds the user interface for the ECGRunWidget.*/
};

} // NAMESPACE

#endif // ECGRUNWIDGET_H
