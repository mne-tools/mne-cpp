//=============================================================================================================
/**
* @file		progressbarwidget.h
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
* @brief	Contains the declaration of the ProgressBarWidget class.
*
*/

#ifndef PROGRESSBARWIDGET_H
#define PROGRESSBARWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp_global.h"
#include "measurementwidget.h"
#include "ui_progressbarwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QBrush>
#include <QFont>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace RTMEASLIB
{
class ProgressBar;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS ProgressBarWidget
*
* @brief The ProgressBarWidget class provides a progress bar display.
*/
class DISPSHARED_EXPORT ProgressBarWidget : public MeasurementWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a ProgressBarWidget which is a child of parent.
    *
    * @param [in] pProgressBar pointer to ProgressBar measurement.
    * @param [in] parent pointer to parent widget; If parent is 0, the new ProgressBarWidget becomes a window. If parent is another widget, ProgressBarWidget becomes a child window inside parent. ProgressBarWidget is deleted when its parent is deleted.
    */
    ProgressBarWidget(ProgressBar* pProgressBar, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the ProgressBarWidget.
    */
    ~ProgressBarWidget();

    //=========================================================================================================
    /**
    * Is called when new data are available.
    * Inherited by IObserver.
    *
    * @param [in] pSubject pointer to Subject -> not used because its direct attached to the measurement.
    */
    virtual void update(Subject* pSubject);

    //=========================================================================================================
    /**
    * Initialise the ProgressBarWidget.
    */
    virtual void init();

    //=========================================================================================================
    /**
    * Is called to paint the progress bar of the ProgressBarWidget.
    *
    * @param [in] event pointer to PaintEvent -> not used.
    */
    virtual void paintEvent(QPaintEvent* event);

private:
    Ui::ProgressBarWidgetClass  ui;					/**< Holds the user interface of the ProgressBar widget. */
    ProgressBar*                m_pProgressBar;		/**< Holds ProgressBar measurement. */
    double                      m_dSegmentSize;		/**< Holds the segment size. */
    unsigned short              m_usXPos;			/**< Holds the horizontal start position. */
    QBrush                      m_Brush;			/**< Holds the progress bar brush. */
    QFont                       m_Font;				/**< Holds the progress bar text font. */
    QString                     m_Text;				/**< Holds the progress bar progress text. */
};

} // NAMESPACE

#endif // PROGRESSBARWIDGET_H
