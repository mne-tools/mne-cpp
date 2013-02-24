//=============================================================================================================
/**
* @file		textwidget.h
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
* @brief	Contains the declaration of the TextWidget class.
*
*/

#ifndef TEXTWIDGET_H
#define TEXTWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp_global.h"
#include "measurementwidget.h"
#include "ui_textwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace RTMEASLIB
{
class Text;
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
* DECLARE CLASS TextWidget
*
* @brief The TextWidget class provides a digital text widget.
*/
class DISPSHARED_EXPORT TextWidget : public MeasurementWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a TextWidget which is a child of parent.
    *
    * @param [in] pText pointer to Text measurement.
    * @param [in] parent pointer to parent widget; If parent is 0, the new TextWidget becomes a window. If parent is another widget, TextWidget becomes a child window inside parent. TextWidget is deleted when its parent is deleted.
    */
    TextWidget(Text* pText, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the TextWidget.
    */
    ~TextWidget();

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
    * Initialise the TextWidget.
    */
    virtual void init();

private:
    Ui::TextWidgetClass ui;			/**< Holds the user interface of the numeric widget. */
    Text*               m_pText;	/**< Holds the pointer to the Text measurement. */
};

} // NAMESPACE

#endif // TEXTWIDGET_H
