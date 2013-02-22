//=============================================================================================================
/**
* @file		runwidget.h
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
* @brief	Contains declaration of RunWidget class.
*
*/

#ifndef RUNDWIDGET_H
#define RUNDWIDGET_H


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

class QTabWidget;
class QScrollArea;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE ECGSimulatorPlugin
//=============================================================================================================

namespace CSART
{

//=============================================================================================================
/**
* DECLARE CLASS RunWidget
*
* @brief The RunWidget class provides the central widget for the run mode.
*/
class RunWidget : public QWidget //not inherit from QTabWidget cause resizeEvent is slower
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a RunWidget which is a child of parent.
    *
    * @param [in] dispWidget pointer to widget which holds the real time displays.
    * @param [in] parent pointer to parent widget; If parent is 0, the new RunWidget becomes a window. If parent is another widget, RunWidget becomes a child window inside parent. RunWidget is deleted when its parent is deleted.
    */
    RunWidget(QWidget* dispWidget, QWidget* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the RunWidget.
    */
    virtual ~RunWidget();

    //=========================================================================================================
    /**
    * Adds a tab with the given page and label to the RunWidget, and returns the index of the tab in the tab bar.
    *
    * @param [in] page pointer to widget which should be added in a new tab.
    * @param [in] label if the tab's label contains an ampersand, the letter following the ampersand is used as a shortcut for the tab, e.g. if the label is "Bro&wse" then Alt+W becomes a shortcut which will move the focus to this tab.
    * @return the index of the tab in the tab bar.
    */
    int addTab(QWidget* page, const QString& label);

    //=========================================================================================================
    /**
    * Sets zoom of display tab to standard.
    */
    void setStandardZoom();

    //=========================================================================================================
    /**
    * Zooms display tab with given factor in vertical direction.
    *
    * @param [in] factor zoom factor for vertical direction.
    */
    void zoomVert(float factor);

signals:

    //=========================================================================================================
    /**
    * This signal is emitted when RunWidget is closed. Used when full screen mode is terminated.
    */
    void displayClosed();

protected:

    //=========================================================================================================
    /**
    * This event handler is called when RunWidget is resized.
    */
    virtual void resizeEvent(QResizeEvent* );

    //=========================================================================================================
    /**
    * This event handler is called when RunWidget is closed.
    */
    virtual void closeEvent(QCloseEvent* );

private:

    QTabWidget*     m_pTabWidgetMain;	/**< Holds the tab widget. */

    QScrollArea*    m_pScrollArea;		/**< Holds the scroll area inside the display tab. */

};

}//NAMESPACE

#endif // RUNDWIDGET_H
