//=============================================================================================================
/**
 * @file     lsladapterproducer.h
 * @author   Simon Heinke <Simon.Heinke@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Simon Heinke, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the LSLAdapterProducer class.
 *
 */

#ifndef LSLADAPTERPRODUCER_H
#define LSLADAPTERPRODUCER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsladapter_global.h"

#include <vector>

#include <scMeas/realtimemultisamplearray.h>
#include <scShared/Management/pluginoutputdata.h>

#include <lsl_cpp.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QVector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE LSLADAPTERPLUGIN
//=============================================================================================================

namespace LSLADAPTERPLUGIN
{

//=============================================================================================================
// LSLPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * The LSLAdapterProducer class.
 *
 * @brief The LSLAdapterProducer class forwards data to the main plugin object
 */
class LSLADAPTERSHARED_EXPORT LSLAdapterProducer : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a LSLAdapterProducer which is a child of parent.
     */
    LSLAdapterProducer(QSharedPointer<SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray> > pRTMSA,
                       int iOutputBlockSize = 100,
                       QObject *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Destructor
     */
    ~LSLAdapterProducer();

    //=========================================================================================================
    /**
     * Call this to provide the stream info for the producer.
     */
    void setStreamInfo(const lsl::stream_info& stream);

    //=========================================================================================================
    /**
     * Stops the streaming.
     */
    void stop();

    //=========================================================================================================
    /**
     * Resets the producer.
     */
    void reset();

    //=========================================================================================================
    /**
     * Whether or not the producer is running.
     */
    inline bool isRunning() const;

    //=========================================================================================================
    /**
     * Setter for output block size.
     */
    void setOutputBlockSize(const int iNewBlockSize);

public slots:
    //=========================================================================================================
    /**
     * The background thread of the LSLAdapter will run this function.
     */
    void readStream();

private:
    // LSL stuff
    lsl::stream_info                m_StreamInfo;
    lsl::stream_inlet*              m_StreamInlet;
    bool                            m_bHasStreamInfo;

    // synchronization with main thread
    volatile bool                   m_bIsRunning;

    // buffering and output parameters
    int                             m_iOutputBlockSize;
    std::vector<std::vector<float>> m_vBufferedSamples;
    QSharedPointer<SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray> > m_pRTMSA;

signals:
    //=========================================================================================================
    /**
     * This tells the LSLAdapter that the stream was stopped.
     */
    void finished();
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool LSLAdapterProducer::isRunning() const
{
    return m_bIsRunning;
}
} // NAMESPACE

#endif // LSLADAPTERPRODUCER_H
