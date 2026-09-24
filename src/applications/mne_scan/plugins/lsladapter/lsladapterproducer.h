//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     lsladapterproducer.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2019
 * @brief    Contains the declaration of the LSLAdapterProducer class.
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

#include <lsl/lsl.h>

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
    void setStreamInfo(const LSLLIB::stream_info& stream);

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
    LSLLIB::stream_info                m_StreamInfo;
    LSLLIB::stream_inlet*              m_StreamInlet;
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
