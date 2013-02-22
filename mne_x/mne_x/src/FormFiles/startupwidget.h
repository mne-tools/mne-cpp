//=============================================================================================================
/**
* @file		startupwidget.h
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
* @brief	Contains declaration of StartUpWidget class.
*
*/

#ifndef STARTUPWIDGET_H
#define STARTUPWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QAction;
class QLabel;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE ECGSimulatorPlugin
//=============================================================================================================

namespace CSART
{

//=============================================================================================================
/**
* DECLARE CLASS StartUpWidget
*
* @brief The StartUpWidget class provides the widget which is shown at start up in the central widget of the main application.
*/
class StartUpWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a StartUpWidget which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new StartUpWidget becomes a window. If parent is another widget, StartUpWidget becomes a child window inside parent. StartUpWidget is deleted when its parent is deleted.
    */
    StartUpWidget(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the StartUpWidget.
    */
    ~StartUpWidget();

private:

    QLabel* m_pLabel_Info;		/**< Holds the start up widget label. */

};

}//NAMESPACE

#endif // STARTUPWIDGET_H
