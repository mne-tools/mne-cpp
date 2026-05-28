//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file rt_client.h
 * @since March 2026
 * @brief Self-contained worker thread that owns the command and data sockets to a running @c mne_rt_server.
 *
 * @ref COMLIB::RtClient is the high-level convenience wrapper around the
 * two lower-level sockets in this directory: it constructs a private
 * @ref COMLIB::RtCmdClient and @ref COMLIB::RtDataClient, performs the
 * full negotiation handshake (request a client id on the command port,
 * register the alias against the same id on the data port, query the
 * @c FiffInfo, request a buffer size) and then sits in a @c run() loop
 * pulling raw sample matrices off the data socket and re-emitting them
 * as Qt signals on the owning thread.
 *
 * The class inherits @c QThread directly rather than the now-preferred
 * worker-object pattern because it predates that idiom and because the
 * tight read/parse/emit loop benefits from running entirely inside
 * @c run(); subclassing remains backwards-compatible with the
 * acquisition plugins that already use it. Communication with the
 * owning thread is one-way: rawBufferReceived() carries an
 * @c Eigen::MatrixXf of @c n_channels × @c buffer_size samples,
 * connectionChanged() flips on the first successful TCP connect and on
 * any subsequent disconnect.
 *
 * Lifetime is cooperative: @c stop() clears the running flag, the
 * @c run() loop notices, the sockets are disconnected and the thread
 * exits, after which @c wait() / destruction is safe. Reconnects after a
 * server restart are not handled here — callers destroy the client and
 * construct a new one.
 */

#ifndef RTCLIENT_H
#define RTCLIENT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../com_global.h"

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
// DEFINE NAMESPACE COMLIB
//=============================================================================================================

namespace COMLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief Threaded façade that owns a paired command/data connection to @c mne_rt_server and re-emits streamed buffers.
 *
 * Constructs and drives both @ref RtCmdClient (port 4217) and
 * @ref RtDataClient (port 4218) from inside its own @c run() loop,
 * handles the initial id/alias/info/buffer-size handshake, and surfaces
 * each received raw buffer as @c rawBufferReceived(Eigen::MatrixXf) so
 * consumers in the GUI / acquisition stack never touch the sockets
 * directly. State changes on the underlying TCP connection are mirrored
 * through @c connectionChanged(bool).
 */
class COMSHARED_EXPORT RtClient : public QThread
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
