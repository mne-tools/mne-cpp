//=============================================================================================================
/**
* @file		numericwidget.h
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
* @brief	Contains the declaration of the NumericWidget class.
*
*/

#ifndef NUMERICWIDGET_H
#define NUMERICWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp_global.h"
#include "measurementwidget.h"
#include "ui_numericwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace RTMEASLIB
{
class Numeric;
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


//=============================================================================================================
/**
* DECLARE CLASS NumericWidget
*
* @brief The NumericWidget class provides a digital display.
*/
class DISPSHARED_EXPORT NumericWidget : public MeasurementWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a NumericWidget which is a child of parent.
    *
    * @param [in] pNumeric pointer to Numeric measurement.
    * @param [in] parent pointer to parent widget; If parent is 0, the new NumericWidget becomes a window. If parent is another widget, NumericWidget becomes a child window inside parent. NumericWidget is deleted when its parent is deleted.
    */
    NumericWidget(Numeric* pNumeric, QWidget* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the NumericWidget.
    */
    ~NumericWidget();

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
    * Initialise the NumericWidget.
    */
    virtual void init();

private:

    Ui::NumericWidgetClass  ui;					/**< Holds the user interface of the numeric widget. */
    Numeric*                m_pNumeric;			/**< Holds the pointer to the Numeric measurement. */
    QString                 m_qString_Unit;		/**< Holds the unit of the measurement. */
};

} // NAMESPACE

#endif // NUMERICWIDGET_H
