//=============================================================================================================
/**
 * @file     timefrequency.h
 * @author   Juan Garcia-Prieto <jgarciaprieto@nmr.mgh.harvard.edu>;
 * @since    0.1.0
 * @date     March, 2023
 *
 * @section  LICENSE
 *
 * Copyright (C) 2023, Juan Garcia-Prieto. All rights reserved.
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
 * @brief    Contains the declaration of the TimeFrequency plugin class.
 *
 */

#ifndef TIMEFREQUENCY_H
#define TIMEFREQUENCY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "timefrequency_global.h"
#include <scShared/Plugins/abstractalgorithm.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>
#include <QWidget>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================


//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace SCMEASLIB {
class RealTimeMultiSampleArray;
}

namespace FIFFLIB {
class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE TIMEFREQUENCYPLUGIN
//=============================================================================================================

namespace TIMEFREQUENCYPLUGIN {

//=============================================================================================================
// TIMEFREQUENCYPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS TimeFrequency
 * @brief The TimeFrequency class provides a tools to compute and show time-frequency maps. */
class TIMEFREQUENCYSHARED_EXPORT TimeFrequency : public SCSHAREDLIB::AbstractAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "timefrequency.json")
    Q_INTERFACES(SCSHAREDLIB::AbstractAlgorithm)

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
     * AbstractAlgorithm functions
     */
    virtual QSharedPointer<SCSHAREDLIB::AbstractPlugin> clone() const;
    virtual void init();
    virtual void unload();
    virtual bool start();
    virtual bool stop();
    virtual AbstractPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();
    virtual QString getBuildInfo();
 protected:
    virtual void run();
    //=========================================================================================================

 public:
    //=========================================================================================================
    void update(QSharedPointer<SCMEASLIB::Measurement> pMeasurement);
    /**
     * Other methods can go here...
     */

 private:
    QSharedPointer<SCSHAREDLIB::PluginInputData <SCMEASLIB::RealTimeMultiSampleArray> > m_pRTMSA_In;  /**< The RealTimeMultiSampleArray of the TimeFrequency input.*/
    QSharedPointer<SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray> > m_pRTMSA_Out;  /**< The RealTimeMultiSampleArray to provide time-frequency data to other plugins.*/
    //QSharedPointer<FIFFLIB::FiffInfo> m_pFiffInfo;  /**< Fiff measurement info.*/
};

}  // namespace TIMEFREQUENCYPLUGIN

#endif  // TIMEFREQUENCY_PLUGIN_MNESCAN
