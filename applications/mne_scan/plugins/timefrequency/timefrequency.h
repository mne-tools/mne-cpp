//=============================================================================================================
/**
 * @file     timefrequency.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     April, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Contains the declaration of the TimeFrequency class.
 *
 */

#ifndef TIMEFREQUENCY_H
#define TIMEFREQUENCY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "timefrequency_global.h"

#include <scShared/Plugins/abstractalgorithm.h>
#include <utils/generics/circularbuffer.h>

#include <fiff/fiff_evoked_set.h>

#include <deque>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFuture>
#include <QFutureWatcher>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace SCMEASLIB{
    class RealTimeEvokedSet;
    class RealTimeTimeFrequency;
    class RealTimeMultiSampleArray;
}

namespace RTPROCESSINGLIB{
    class RtTimeFrequency;
}

//=============================================================================================================
// DEFINE NAMESPACE TIMEFREQUENCYPLUGIN
//=============================================================================================================

namespace TIMEFREQUENCYPLUGIN
{

//=============================================================================================================
// TIMEFREQUENCYPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS TimeFrequency
 *
 * @brief The TimeFrequency class provides a TimeFrequency algorithm structure.
 */
class TIMEFREQUENCYSHARED_EXPORT TimeFrequency : public SCSHAREDLIB::AbstractAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "timefrequency.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractAlgorithm)

    friend class TimeFrequencySettingsWidget;

public:
    //=========================================================================================================
    /**
     * Constructs a TimeFrequency.
     */
    TimeFrequency();

    //=========================================================================================================
    /**
     * Destroys the TimeFrequency.
     */
    ~TimeFrequency();

    //=========================================================================================================
    /**
     * Inits widgets which are used to control this plugin, then emits them in form of a QList.
     */
    void initPluginControlWidgets();

    //=========================================================================================================
    /**
     * Reimplemented virtual functions
     */
    virtual void unload();
    virtual QSharedPointer<SCSHAREDLIB::AbstractPlugin> clone() const;
    virtual bool start();
    virtual bool stop();
    virtual SCSHAREDLIB::AbstractPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();
    void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Initialise input and output connectors.
     */
    virtual void init();

private:
    virtual void run();

    void computeTimeFrequency();

    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr     m_pTimeFrequencyTimeSeriesInput;        /**< The RealTimeMultiSampleArray of the NoiseReduction input.*/
    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeEvokedSet>::SPtr            m_pTimeFrequencyEvokedInput;            /**< The RealTimeSampleArray of the TimeFrequency input.*/
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeTimeFrequency>::SPtr       m_pTimeFrequencyOutput;                 /**< The RealTimeEvoked of the TimeFrequency output.*/

    QSharedPointer<UTILSLIB::CircularBuffer<FIFFLIB::FiffEvoked>>               m_pCircularEvokedBuffer;                /**< Holds incoming RealTimeMultiSampleArray data.*/
    QSharedPointer<UTILSLIB::CircularBuffer_Matrix_double>                      m_pCircularTimeSeriesBuffer;      /**< Holds incoming raw data. */

    QSharedPointer<RTPROCESSINGLIB::RtTimeFrequency>                            m_pRTTF;

    QMutex                                                                      m_qMutex;                               /**< Provides access serialization between threads. */

    FIFFLIB::FiffInfo::SPtr                                                     m_pFiffInfo;                            /**< Fiff measurement info.*/

    std::deque<Eigen::MatrixXd>                                                 m_DataQueue;

    QMap<QString,int>                                                           m_mapStimChsIndexNames;                 /**< The currently available stim channels and their corresponding index in the data. */

    QFuture<Eigen::MatrixXcd>                                                   m_Future;
    QFutureWatcher<Eigen::MatrixXcd>                                            m_FutureWatcher;

    int                                                                         m_iDataQueueBlockSize;
signals:

};
} // NAMESPACE

#endif // TIMEFREQUENCY_H
