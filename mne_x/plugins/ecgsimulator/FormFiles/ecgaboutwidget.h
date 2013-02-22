//=============================================================================================================
/**
* @file		ecgaboutwidget.h
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
* @brief	Contains the declaration of the ECGAboutWidget class.
*
*/

#ifndef ECGABOUTWIDGET_H
#define ECGABOUTWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_ecgabout.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtGui/QDialog>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE GaborParticleToolboxModule
//=============================================================================================================

namespace ECGSimulatorModule
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS ECGAboutWidget
*
* @brief The ECGAboutWidget class provides the about dialog for the ECGSimulator.
*/
class ECGAboutWidget : public QDialog
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a ECGAboutWidget dialog which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new ECGAboutWidget becomes a window. If parent is another widget, ECGAboutWidget becomes a child window inside parent. ECGAboutWidget is deleted when its parent is deleted.
    */
    ECGAboutWidget(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the ECGAboutWidget.
    * All ECGAboutWidget's children are deleted first. The application exits if ECGAboutWidget is the main widget.
    */
    ~ECGAboutWidget();

private:
    Ui::ECGAboutWidgetClass ui;		/**< Holds the user interface for the DummyAboutWidget.*/
};

} // NAMESPACE

#endif // ECGABOUTWIDGET_H
