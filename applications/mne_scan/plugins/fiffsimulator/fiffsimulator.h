//=============================================================================================================
/**
 * @file     fiffsimulator.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the FiffSimulator class.
 *
 */

#ifndef FIFFSIMULATOR_H
#define FIFFSIMULATOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffsimulator_global.h"

#include <scShared/Interfaces/ISensor.h>
#include <utils/generics/circularmatrixbuffer.h>
#include <communication/rtClient/rtcmdclient.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QVector>
#include <QTimer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace SCMEASLIB {
    class RealTimeMultiSampleArray;
}

namespace FIFFLIB {
    class FiffInfo;
}

namespace DISP3DLIB {
    class HpiView;
}

//=============================================================================================================
// DEFINE NAMESPACE FIFFSIMULATORPLUGIN
//=============================================================================================================

namespace FIFFSIMULATORPLUGIN
{

//=============================================================================================================
// FIFFSIMULATORPLUGIN FORWARD DECLARATIONS 
//=============================================================================================================

class FiffSimulatorProducer;

//=============================================================================================================
/**
 * DECLARE CLASS FiffSimulator
 *
 * @brief The FiffSimulator class provides a RT server connection.
 */
class FIFFSIMULATORSHARED_EXPORT FiffSimulator : public SCSHAREDLIB::ISensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "fiffsimulator.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::ISensor)

    friend class FiffSimulatorProducer;
    friend class FiffSimulatorSetupWidget;

public:

    //=========================================================================================================
    /**
     * Constructs a FiffSimulator.
     */
    FiffSimulator();

    //=========================================================================================================
    /**
     * Destroys the FiffSimulator.
     */
    virtual ~FiffSimulator();

    //=========================================================================================================
    /**
     * Clears the rt server
     */
    void clear();

    //=========================================================================================================
    /**
     * Clone the plugin
     */
    virtual QSharedPointer<IPlugin> clone() const;

    //=========================================================================================================
    /**
     * Initialise the FiffSimulator.
     */
    virtual void init();

    //=========================================================================================================
    /**
     * Is called when plugin is detached of the stage. Can be used to safe settings.
     */
    virtual void unload();

    virtual bool start();
    virtual bool stop();

    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;

    //=========================================================================================================
    /**
     * Creates the setup widget.
     *
     * @return Returns the setup widget.
     */
    virtual QWidget* setupWidget();

protected:
    virtual void run();

    //=========================================================================================================
    /**
     * Initialises the output connector.
     */
    void initConnector();

    //=========================================================================================================
    /**
     * Change connector
     *
     * @param[in] p_iNewConnectorId      new connector ID
     */
    void changeConnector(qint32 p_iNewConnectorId);

    //=========================================================================================================
    /**
     * Connects the cmd client.
     */
    void connectCmdClient();

    //=========================================================================================================
    /**
     * Disconnects the cmd client.
     */
    void disconnectCmdClient();

    //=========================================================================================================
    /**
     * Request FiffInfo using cmd client and producer (data client)
     */
    void requestInfo();

    //=========================================================================================================
    /**
     * Set HPI fiff information.
     */
    void showHPIDialog();

    //=========================================================================================================
    /**
     * Sends the current data block to the HPI dialog.
     *
     * @param [in] matData   The new data block.
     */
    void updateHPI(const Eigen::MatrixXf &matData);    

    //=========================================================================================================
    /**
     * Sends the current data block to the HPI dialog and performs a fit.
     *
     * @param [in] matData   The data block to which the HPI information is to be written.
     */
    void doContinousHPI(Eigen::MatrixXf& matData);

    //=========================================================================================================
    /**
     * Toggles teh continous HPI flag.
     *
     * @param [in] bDoContinousHPI   Whether to do continous HPI.
     */
    void onContinousHPIToggled(bool bDoContinousHPI);

    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr m_pRTMSA_FiffSimulator;     /**< The RealTimeMultiSampleArray to provide the rt_server Channels.*/

    QSharedPointer<FiffSimulatorProducer>           m_pFiffSimulatorProducer;   /**< Holds the FiffSimulatorProducer.*/
    QSharedPointer<IOBUFFER::RawMatrixBuffer>       m_pRawMatrixBuffer_In;      /**< Holds incoming raw data. */
    QSharedPointer<FIFFLIB::FiffInfo>               m_pFiffInfo;                /**< Fiff measurement info.*/
    QSharedPointer<COMMUNICATIONLIB::RtCmdClient>   m_pRtCmdClient;             /**< The command client.*/
    QSharedPointer<DISP3DLIB::HpiView>              m_pHPIWidget;               /**< HPI widget. */

    QAction*                m_pActionComputeHPI;            /**< Update HPI info into Fiff Info action */

    bool                    m_bCmdClientIsConnected;        /**< If the command client is connected.*/
    bool                    m_bDoContinousHPI;              /**< Whether to do continous HPI.*/

    QString                 m_sFiffSimulatorIP;             /**< The IP Adress of mne_rt_server.*/
    QString                 m_sFiffSimulatorClientAlias;    /**< The rt server client alias.*/

    qint32                  m_iActiveConnectorId;           /**< The active connector.*/
    qint32                  m_iBufferSize;                  /**< The raw data buffer size.*/

    QMap<qint32, QString>   m_qMapConnectors;               /**< Connector map.*/

    QTimer                  m_cmdConnectionTimer;           /**< Timer for convinient command client connection. When timer times out a connection is tried to be established. */

    QMutex                  m_qMutex;

signals:
    //=========================================================================================================
    /**
     * Emitted when command clients connection status changed
     *
     * @param[in] p_bStatus  connection status
     */
    void cmdConnectionChanged(bool p_bStatus);

    //=========================================================================================================
    /**
     * Emitted when fiffInfo is available
     */
    void fiffInfoAvailable();
};
} // NAMESPACE

#endif // FIFFSIMULATOR_H
