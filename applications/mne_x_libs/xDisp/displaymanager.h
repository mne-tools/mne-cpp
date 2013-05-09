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
#include <xMeas/Nomenclature/nomenclature.h>


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

//add
    //=========================================================================================================
    /**
    * Adds a digital display, so called numeric widget.
    *
    * @param [in] pNum pointer to numeric measurement.
    * @param [in] parent pointer to parent widget.
    * @param [in] id of the numeric measurement provider.
    * @return pointer to the new numeric widget.
    */
    static NumericWidget* addNumericWidget(QSharedPointer<Numeric> pNum, QWidget* parent, MSR_ID::Measurement_ID id);

    //=========================================================================================================
    /**
    * Adds a real-time sample array widget.
    *
    * @param [in] pRTSA pointer to real-time sample array measurement.
    * @param [in] parent pointer to parent widget.
    * @param [in] id of the real-time sample array measurement provider.
    * @param [in] t pointer to application time.
    * @return pointer to the new real-time sample array widget.
    */
    static RealTimeSampleArrayWidget* addRealTimeSampleArrayWidget(QSharedPointer<RealTimeSampleArray> pRTSA, QWidget* parent, MSR_ID::Measurement_ID id, QSharedPointer<QTime> t);

    //=========================================================================================================
    /**
    * Adds a real-time multi sample array widget.
    *
    * @param [in] pRTSM pointer to real-time sample array measurement.
    * @param [in] parent pointer to parent widget.
    * @param [in] id of the real-time sample array measurement provider.
    * @param [in] t pointer to application time.
    * @return pointer to the new real-time multi sample array widget.
    */
    static RealTimeMultiSampleArrayWidget* addRealTimeMultiSampleArrayWidget(QSharedPointer<RealTimeMultiSampleArray> pRTSM, QWidget* parent, MSR_ID::Measurement_ID id, QSharedPointer<QTime> t);

    //=========================================================================================================
    /**
    * Adds a real-time multi sample array widget.
    *
    * @param [in] pRTSM pointer to real-time sample array measurement.
    * @param [in] parent pointer to parent widget.
    * @param [in] id of the real-time sample array measurement provider.
    * @param [in] t pointer to application time.
    * @return pointer to the new real-time multi sample array widget.
    */
    static RealTimeMultiSampleArrayNewWidget* addRealTimeMultiSampleArrayNewWidget(QSharedPointer<RealTimeMultiSampleArrayNew> pRTSM, QWidget* parent, MSR_ID::Measurement_ID id, QSharedPointer<QTime> t);

    //=========================================================================================================
    /**
    * Adds a progress bar widget.
    *
    * @param [in] pProgress pointer to progress bar measurement.
    * @param [in] parent pointer to parent widget.
    * @param [in] id of the progress bar measurement provider.
    * @return pointer to the new progress bar widget.
    */
    static ProgressBarWidget* addProgressBarWidget(QSharedPointer<ProgressBar> pProgress, QWidget* parent, MSR_ID::Measurement_ID id);

    //=========================================================================================================
    /**
    * Adds a text widget.
    *
    * @param [in] pText pointer to text measurement.
    * @param [in] parent pointer to parent widget.
    * @param [in] id of the text measurement provider.
    * @return pointer to the new text widget.
    */
    static TextWidget* addTextWidget(QSharedPointer<Text> pText, QWidget* parent, MSR_ID::Measurement_ID id);

//get
    //=========================================================================================================
    /**
    * Returns a hash of all measurement widgets and their provider id's.
    *
    * @return a reference to a hash containing the measurement widgets and their corresponding measurement provider id's .
    */
    static QHash<MSR_ID::Measurement_ID, MeasurementWidget*>& getMeasurementWidgets() {return s_hashMeasurementWidgets;};

    //=========================================================================================================
    /**
    * Returns a hash of all numeric widgets and their provider id's.
    *
    * @return a reference to a hash containing the numeric widgets and their corresponding numeric provider id's.
    */
    static QHash<MSR_ID::Measurement_ID, NumericWidget*>& getNumericWidgets() {return s_hashNumericWidgets;};

    //=========================================================================================================
    /**
    * Returns a hash of all real-time sample array widgets and their provider id's.
    *
    * @return a reference to a hash containing the real-time sample array widgets and their corresponding real-time sample array provider id's .
    */
    static QHash<MSR_ID::Measurement_ID, RealTimeSampleArrayWidget*>& getRTSAWidgets() {return s_hashRealTimeSampleArrayWidgets;};

    //=========================================================================================================
    /**
    * Returns a hash of all real-time multi sample array widgets and their provider id's.
    *
    * @return a reference to a hash containing the real-time sample array widgets and their corresponding real-time sample array provider id's .
    */
    static QHash<MSR_ID::Measurement_ID, RealTimeMultiSampleArrayWidget*>& getRTMSAWidgets() {return s_hashRealTimeMultiSampleArrayWidgets;};

    //=========================================================================================================
    /**
    * Returns a hash of all real-time multi sample array new widgets and their provider id's.
    *
    * @return a reference to a hash containing the real-time sample array new widgets and their corresponding real-time sample array provider id's .
    */
    static QHash<MSR_ID::Measurement_ID, RealTimeMultiSampleArrayNewWidget*>& getRTMSANewWidgets() {return s_hashRealTimeMultiSampleArrayNewWidgets;};

    //=========================================================================================================
    /**
    * Returns a hash of all progress bar widgets and their provider id's.
    *
    * @return a reference to a hash containing the progress bar widgets and their corresponding progress bar provider id's.
    */
    static QHash<MSR_ID::Measurement_ID, ProgressBarWidget*>& getProgressBarWidgets() {return s_hashProgressBarWidgets;};

    //=========================================================================================================
    /**
    * Returns a hash of all text widgets and their provider id's.
    *
    * @return a reference to a hash containing the text widgets and their corresponding text provider id's.
    */
    static QHash<MSR_ID::Measurement_ID, TextWidget*>& getTextWidgets() {return s_hashTextWidgets;};

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
    static QHash<MSR_ID::Measurement_ID, MeasurementWidget*>              s_hashMeasurementWidgets;                     /**< Holds all measurement widgets and corresponding measurement id's.*/
    static QHash<MSR_ID::Measurement_ID, NumericWidget*>                  s_hashNumericWidgets;                         /**< Holds all numeric widgets and corresponding numeric measurement id's.*/
    static QHash<MSR_ID::Measurement_ID, RealTimeSampleArrayWidget*>      s_hashRealTimeSampleArrayWidgets;             /**< Holds all real-time sample array widgets and corresponding real-time sample array id's.*/
    static QHash<MSR_ID::Measurement_ID, RealTimeMultiSampleArrayWidget*> s_hashRealTimeMultiSampleArrayWidgets;        /**< Holds all real-time multi sample array widgets and corresponding real-time multi sample array id's.*/
    static QHash<MSR_ID::Measurement_ID, RealTimeMultiSampleArrayNewWidget*> s_hashRealTimeMultiSampleArrayNewWidgets;  /**< Holds all real-time multi sample array widgets and corresponding real-time multi sample array id's.*/
    static QHash<MSR_ID::Measurement_ID, ProgressBarWidget*>              s_hashProgressBarWidgets;                     /**< Holds all progress bar widgets and corresponding progress bar id's.*/
    static QHash<MSR_ID::Measurement_ID, TextWidget*>                     s_hashTextWidgets;                            /**< Holds all text widgets and text measurement id's.*/

};

} // NAMESPACE

#endif // DISPLAYMANAGER_H
