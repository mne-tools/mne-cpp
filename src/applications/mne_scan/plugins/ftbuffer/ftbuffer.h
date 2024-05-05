//=============================================================================================================
/**
 * @file     ftbuffer.h
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch, Gabriel B Motta. All rights reserved.
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
 * @brief    Contains the declaration of the FtBuffer class.
 *
 */

#ifndef FTBUFFER_H
#define FTBUFFER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffer_global.h"

#include "FormFiles/ftbuffersetupwidget.h"

#include "ftbuffproducer.h"

#include <scShared/Plugins/abstractsensor.h>
#include <scShared/Plugins/abstractalgorithm.h>

#include <scMeas/realtimemultisamplearray.h>

#include <fiff/fiff_raw_data.h>

#include <utils/generics/circularbuffer.h>

#include <thread>
#include <atomic>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>
#include <QSharedPointer>
#include <QThread>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATION
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE FTBUFFERPLUGIN
//=============================================================================================================

namespace FTBUFFERPLUGIN {

//=============================================================================================================
// FTBUFFERPLUGIN FORWARD DECLARATION
//=============================================================================================================

class FtBuffProducer;

//=============================================================================================================
/**
 * Starts new thread for FtBuffProducer which collects data, then outputs that data.
 *
 * @brief Handles Ftbuffer data received from FtBuffProducer and outputs it.
 */
class FTBUFFER_EXPORT FtBuffer : public SCSHAREDLIB::AbstractSensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "ftbuffer.json")
    Q_INTERFACES(SCSHAREDLIB::AbstractSensor)

    friend class FtBufferSetupWidget;
    friend class FtBuffProducer;
    friend class FtConnector;

public:
    //=========================================================================================================
    /**
     * Creates an instance of FtBuffer
     */
    FtBuffer();

    //=========================================================================================================
    /**
     * Destroys an instace of FtBuffer
     */
    ~FtBuffer();

    //=========================================================================================================
    /**
     * Clone the plugin
     */
    virtual QSharedPointer<AbstractPlugin> clone() const;

    //=========================================================================================================
    /**
     * Initializes the plugin.
     */
    virtual void init();

    //=========================================================================================================
    /**
     * Is called when plugin is detached of the stage. Can be used to safe settings.
     */
    virtual void unload();

    //=========================================================================================================
    /**
     * Starts the AbstractSensor.
     * Pure virtual method inherited by IModule.
     *
     * @return true if success, false otherwise.
     */
    virtual bool start();

    //=========================================================================================================
    /**
     * Stops the AbstractSensor.
     * Pure virtual method inherited by IModule.
     *
     * @return true if success, false otherwise.
     */
    virtual bool stop();

    //=========================================================================================================
    /**
     * Returns the plugin type.
     * Pure virtual method inherited by IModule.
     *
     * @return type of the AbstractSensor.
     */
    virtual PluginType getType() const;

    //=========================================================================================================
    /**
     * Returns the plugin name.
     * Pure virtual method inherited by IModule.
     *
     * @return the name of the AbstractSensor.
     */
    virtual QString getName() const;

    //=========================================================================================================
    /**
     * Returns the set up widget for configuration of AbstractSensor.
     * Pure virtual method inherited by IModule.
     *
     * @return the setup widget.
     */
    virtual QWidget* setupWidget();

    virtual QString getBuildInfo();

    //=========================================================================================================
    /**
     * Sets address used to connect to buffer (if starting plugin without an established connection).
     *
     * @param[in] sAddress      IP address of the FieldTrip buffer
     */
    void setBufferAddress(const QString& sAddress);

    //=========================================================================================================
    /**
     * Sets port used to connect to buffer (if starting plugin without an established connection).
     *
     * @param[in] iPort         Port of the FieldTrip buffer
     */
    void setBufferPort(int iPort);

signals:
    //=========================================================================================================
    /**
     * Sends signal to FtBuffProducer to start data aquisition
     */
    void workCommand();

private:
    //=========================================================================================================
    /**
     * Gets extecuted after start(), currently does nothing
     */
    virtual void run();    

    //=========================================================================================================
    /**
     * Receives new data from producer, publishes to plugin output rtmsa
     *
     * @param[in] matData   New data from FtBuffProducer.
     */
    void onNewDataAvailable(const Eigen::MatrixXd &matData);

    //=========================================================================================================
    /**
     * Sets up Fiff info from and uses it to initialize m_pRTMSA_BufferOutput
     */
    bool setupRTMSA();

    //=========================================================================================================
    /**
     * Sets up Fiff info from passed FiffInfo and uses it to initialize m_pRTMSA_BufferOutput
     *
     * @param[in] FiffInfo  FiffInfo used to set up buffer output.
     */
    bool setupRTMSA(const FIFFLIB::FiffInfo& FiffInfo);

    //=========================================================================================================
    /**
     * Initializes output based on input measuremnt data object
     *
     * @param[in] metadata  object with fiffinfo and optionally digitizer data
     */
    bool setupRTMSA(const MetaData& metadata);

    bool                                                                                m_bIsConfigured;                /**< Whether the buffer output has been configured. */

    QMutex                                                                              m_mutex;                        /**< Guards shared data from being accessed at the same time. */

    QThread                                                                             m_pProducerThread;              /**< Producer thread for the FtBuffProducer object. */

    QSharedPointer<FtBuffProducer>                                                      m_pFtBuffProducer;              /**< Pointer to producer object that handles data from FtConnector*/
    QSharedPointer<FIFFLIB::FiffInfo>                                                   m_pFiffInfo;                    /**< Fiff measurement info.*/
    QSharedPointer<FIFFLIB::FiffRawData>                                                m_pNeuromagHeadChunkData;       /**< Fiff into parser for header data collected from Neuromag extended header. */
    QSharedPointer<SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray> > m_pRTMSA_BufferOutput;          /**< The RealTimeSampleArray to provide the plugin output data.*/
    QSharedPointer<UTILSLIB::CircularBuffer_Matrix_double>                              m_pCircularBuffer;              /**< Holds incoming raw data. */

    QString                                                                             m_sBufferAddress;               /**< The address used to connect to the buffer if starting without being connected */
    int                                                                                 m_iBufferPort;                  /**< The port used to connect to the buffer if starting without being connected */

    std::thread             m_OutputProcessingThread;
    std::atomic_bool        m_bProcessOutput;
};
}//namespace end brace

#endif // FTBUFFER_H
