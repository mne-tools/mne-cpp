//=============================================================================================================
/**
* @file     rtsss.h
* @author   Seok Lew <slew@nmr.mgh.harvard.edu>;
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
* @brief    Contains the declaration of the RTSSS class.
*
*/

#ifndef RTSSS_H
#define RTSSS_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsss_global.h"

#include <scShared/Interfaces/IAlgorithm.h>
#include <utils/generics/circularbuffer.h>
#include <utils/generics/circularmatrixbuffer.h>

#include <scMeas/realtimesamplearray.h>
#include <scMeas/realtimemultisamplearray.h>

#include <fiff/fiff.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_evoked.h>

#include <Eigen/Dense>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RTSSSPlugin
//=============================================================================================================

namespace RTSSSPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;
using namespace FIFFLIB;
using namespace SCMEASLIB;
using namespace IOBUFFER;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS RTSSS
*
* @brief The RtSss class provides a rtsss algorithm structure.
*/
class RTSSSSHARED_EXPORT RtSss : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "rtsss.json") //NEW Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

public:
    //=========================================================================================================
    /**
    * Constructs a RtSss.
    */
//    DummyToolbox();
    RtSss();

    //=========================================================================================================
    /**
    * Destroys the RtSss.
    */
//    ~DummyToolbox();
    ~RtSss();

    //=========================================================================================================
    /**
    * Initialise input and output connectors.
    */
    virtual void init();

    //=========================================================================================================
    /**
    * Is called when plugin is detached of the stage. Can be used to safe settings.
    */
    virtual void unload();

    //=========================================================================================================
    /**
    * Clone the plugin
    */
    virtual QSharedPointer<IPlugin> clone() const;

    virtual bool start();
    virtual bool stop();

    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;

    virtual QWidget* setupWidget();

    void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    void setLinRR(int);
    void setLoutRR(int);
    void setLin(int);
    void setLout(int);

protected:
    virtual void run();

private:
//    PluginInputData<RealTimeSampleArray>::SPtr   m_pDummyInput;      /**< The RealTimeSampleArray of the DummyToolbox input.*/
//    PluginOutputData<RealTimeSampleArray>::SPtr  m_pDummyOutput;    /**< The RealTimeSampleArray of the DummyToolbox output.*/
    PluginInputData<RealTimeSampleArray>::SPtr   m_pRTSAInput;      /**< The RealTimeSampleArray of the RtSss input.*/
    PluginOutputData<RealTimeSampleArray>::SPtr  m_pRTSAOutput;    /**< The RealTimeSampleArray of the RtSss output.*/

    PluginInputData<RealTimeMultiSampleArray>::SPtr  m_pRTMSAInput;  /**< The RealTimeMultiSampleArray input.*/
    PluginOutputData<RealTimeMultiSampleArray>::SPtr      m_pRTMSAOutput;  /**< The RealTimeMultiSampleArray output.*/


    bool m_bIsRunning;      /**< If source lab is running */
    bool m_bReceiveData;    /**< If thread is ready to receive data */
    bool m_bProcessData;    /**< If data should be received for processing */

    FiffInfo::SPtr              m_pFiffInfo;        /**< Fiff information. */

    CircularMatrixBuffer<double>::SPtr m_pRtSssBuffer;   /**< Holds incoming rt server data.*/

    int LinRR, LoutRR, Lin, Lout;

    QMutex m_qMutex;

    //    dBuffer::SPtr   m_pRtSssBuffer;      /**< Holds incoming data.*/
};

} // NAMESPACE

#endif // RTSSS_H
