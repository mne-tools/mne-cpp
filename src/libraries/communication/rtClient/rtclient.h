//=============================================================================================================
/**
 * @file     rtclient.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief     declaration of the RtClient Class.
 *
 */

#ifndef RTCLIENT_H
#define RTCLIENT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../communication_global.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMutex>
#include <QSharedPointer>
#include <QString>
#include <QThread>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE COMMUNICATIONLIB
//=============================================================================================================

namespace COMMUNICATIONLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * The real-time client class provides an interface to communicate with a running mne_rt_server.
 *
 * @brief Real-time client
 */
class COMMUNICATIONSHARED_EXPORT RtClient : public QThread
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtClient> SPtr;               /**< Shared pointer type for MNERtClient. */
    typedef QSharedPointer<const RtClient> ConstSPtr;    /**< Const shared pointer type for MNERtClient. */

    //=========================================================================================================
    /**
     * Creates the real-time client.
     *
     * @param[in] p_sRtServerHostname    The IP address of the mne_rt_server.
     * @param[in] p_sClientAlias         The client alias of the data client.
     * @param[in] parent                 Parent QObject (optional).
     */
    explicit RtClient(QString p_sRtServerHostname, QString p_sClientAlias = "rtclient", QObject *parent = 0);
    
    //=========================================================================================================
    /**
     * Destroys the real time client.
     */
    ~RtClient();

    //=========================================================================================================
    /**
     * Request Fiff Info
     */
    inline FIFFLIB::FiffInfo::SPtr& getFiffInfo();

    //=========================================================================================================
    /**
     * Rt Server status, returns true when rt server is started.
     *
     * @return true if started, false otherwise.
     */
    bool getConnectionStatus();

    //=========================================================================================================
    /**
     * Stops the RtClient by stopping the producer's thread.
     *
     * @return true if succeeded, false otherwise.
     */
    virtual bool stop();

protected:
    //=========================================================================================================
    /**
     * The starting point for the thread. After calling start(), the newly created thread calls this function.
     * Returning from this method will end the execution of the thread.
     * Pure virtual method inherited by QThread.
     */
    virtual void run();

private:
    QMutex      mutex;                      /**< Provides access serialization between threads*/
    bool        m_bIsConnected;             /**< Is Connected. */
    bool        m_bIsMeasuring;             /**< Is Measuring. */
    bool        m_bIsRunning;               /**< Holds whether RtClient is running.*/
    QString     m_sClientAlias;             /**< The clien alias of the data client. */
    QString     m_sRtServerHostName;        /**< The IP Adress of mne_rt_server.*/
    FIFFLIB::FiffInfo::SPtr  m_pFiffInfo;   /**< Fiff measurement info.*/
    quint16      m_iDefaultPort;
signals:
    //=========================================================================================================
    /**
     * Emits a received raw buffer - ToDo change the emits to fiff raw data.
     *
     * @param[in] p_rawBuffer    the received raw buffer.
     */
    void rawBufferReceived(Eigen::MatrixXf p_rawBuffer);

    //=========================================================================================================
    /**
     * Emitted when connection status changed
     *
     * @param[in] p_bStatus  connection status.
     */
    void connectionChanged(bool p_bStatus);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline FIFFLIB::FiffInfo::SPtr& RtClient::getFiffInfo()
{
    return m_pFiffInfo;
}
} // NAMESPACE

#ifndef metatype_matrixxf
#define metatype_matrixxf
Q_DECLARE_METATYPE(Eigen::MatrixXf);    /**< Provides QT META type declaration of the Eigen::MatrixXf type. For signal/slot usage.*/
#endif

#endif // RTCLIENT_H
