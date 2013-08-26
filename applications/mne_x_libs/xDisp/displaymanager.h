//=============================================================================================================
/**
* @file     displaymanager.h
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
* @brief    Declaration of the DisplayManager Class.
*
*/

#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "xdisp_global.h"

#include <generics/circularbuffer.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QHash>
#include <QWidget>
#include <QLabel>
#include <QString>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QVBoxLayout;
class QHBoxLayout;

namespace XMEASLIB
{
class Numeric;
class RealTimeSampleArray;
class RealTimeMultiSampleArray;
class RealTimeMultiSampleArrayNew;
class RealTimeSourceEstimate;
class ProgressBar;
class Text;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XDISPLIB
//=============================================================================================================

namespace XDISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace IOBuffer;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class NumericWidget;
class RealTimeSampleArrayWidget;
class RealTimeMultiSampleArrayWidget;
class RealTimeMultiSampleArrayNewWidget;
class RealTimeSourceEstimateWidget;
class ProgressBarWidget;
class TextWidget;
class MeasurementWidget;


//=============================================================================================================
/**
* DECLARE CLASS DisplayManager
*
* @brief The DisplayManager class handles current displayed widgets.
*/
class XDISPSHARED_EXPORT DisplayManager
{
public:
    typedef QSharedPointer<DisplayManager> SPtr;               /**< Shared pointer type for DisplayManager. */
    typedef QSharedPointer<const DisplayManager> ConstSPtr;    /**< Const shared pointer type for DisplayManager. */

    //=========================================================================================================
    /**
    * Constructs a DisplayManager.
    */
    DisplayManager();

    //=========================================================================================================
    /**
    * Destroys the DisplayManager.
    */
    virtual ~DisplayManager();


    //=========================================================================================================
    /**
    * Initialise the measurement widgets by calling there init() function.
    */
    static void init();

    //=========================================================================================================
    /**
    * Shows a widget containing all current measurement widgets.
    *
    * @return a pointer to the widget containing all measurement widgets.
    */
    static QWidget* show();

    //=========================================================================================================
    /**
    * Cleans all measurement widget hash's.
    */
    static void clean();

private:

};

} // NAMESPACE

#endif // DISPLAYMANAGER_H
