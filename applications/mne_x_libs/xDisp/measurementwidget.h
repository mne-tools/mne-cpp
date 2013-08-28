//=============================================================================================================
/**
* @file     measurementwidget.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Declaration of the MeasurementWidget Class.
*
*/

#ifndef MEASUREMENTWIDGET_H
#define MEASUREMENTWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "xdisp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <generics/observerpattern.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XDISPLIB
//=============================================================================================================

namespace XDISPLIB
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//*************************************************************************************************************
//=============================================================================================================
// ENUMERATIONS
//=============================================================================================================

////=============================================================================================================
///**
//* Tool enumeration.
//*/
//enum Tool
//{
//    Freeze     = 0,     /**< Freezing tool. */
//    Annotation = 1      /**< Annotation tool. */
//};


//=============================================================================================================
/**
* DECLARE CLASS MeasurementWidget
*
* @brief The MeasurementWidget class is the base class of all measurement widgets.
*/
class XDISPSHARED_EXPORT MeasurementWidget : public QWidget, public IObserver
{
    Q_OBJECT
public:

    //=========================================================================================================
    /**
    * Constructs a MeasurementWidget which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new MeasurementWidget becomes a window. If parent is another widget, MeasurementWidget becomes a child window inside parent. MeasurementWidget is deleted when its parent is deleted.
    */
    MeasurementWidget(QWidget* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the MeasurementWidget.
    */
    virtual ~MeasurementWidget();

    //=========================================================================================================
    /**
    * Is called when new data are available.
    * Pure virtual method inherited by IObserver.
    *
    * @param [in] pSubject  pointer to Subject -> not used because its direct attached to the measurement.
    */
    virtual void update(Subject* pSubject) = 0;

    //=========================================================================================================
    /**
    * Initialise the MeasurementWidget.
    * Pure virtual method.
    */
    virtual void init() = 0;
};

}

#endif // MEASUREMENTWIDGET_H
