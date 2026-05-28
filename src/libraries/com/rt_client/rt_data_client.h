//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rt_data_client.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    TCP client for the @c mne_rt_server raw-data port (4218): pulls @c FiffInfo, digitizer points and streamed sample buffers.
 *
 * @ref COMLIB::RtDataClient is a thin @c QTcpSocket subclass that speaks
 * the binary FIFF dialect @c mne_rt_server uses on its data port. The
 * server’s wire protocol is a sequence of FIFF tags rather than a custom
 * framing format, so this class reuses @ref FIFFLIB::FiffStream and
 * @ref FIFFLIB::FiffTag to parse what it receives — the same code paths
 * that read measurement files from disk.
 *
 * Three message exchanges are supported. @c getClientId() asks the
 * server to allocate an integer handle the server then uses to route
 * subsequent buffers back to this socket. @c readInfo() /
 * @c readMetadata() pull the channel layout, sampling rate and (when
 * available) the head-shape digitiser point set captured during the
 * measurement session. @c readRawBuffer() blocks until the next sample
 * matrix arrives, reshapes the raw bytes into an @c Eigen::MatrixXf of
 * @c n_channels × @c buffer_size, and reports the FIFF tag kind so the
 * caller can distinguish data buffers from control frames
 * (@c FIFF_BLOCK_START / @c FIFF_BLOCK_END / measurement stop).
 *
 * The companion @ref COMLIB::MetaData struct bundles @c FiffInfo and
 * @c FiffDigitizerData so a single call carries both pieces of the
 * session header without forcing callers to introduce a tuple type.
 */

#ifndef RTDATACLIENT_H
#define RTDATACLIENT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../com_global.h"

#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_digitizer_data.h>

#include <utility>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>
#include <QTcpSocket>

//=============================================================================================================
// DEFINE NAMESPACE COMLIB
//=============================================================================================================

namespace COMLIB
{

//=============================================================================================================
/**
 * @brief Value-type aggregate of the FIFF metadata blocks @c mne_rt_server sends at the start of a measurement.
 *
 * @c mne_rt_server delivers both the per-channel @c FiffInfo header and
 * the head-shape @c FiffDigitizerData block before the first sample
 * buffer; callers usually need both to make sense of the stream
 * (channel positions for visualisation, digitiser points for
 * coregistration). @c MetaData lets @ref RtDataClient::readMetadata
 * return them together without introducing a @c std::pair or splitting
 * the read into two round trips.
 */
struct MetaData{
    MetaData(FIFFLIB::FiffInfo::SPtr pInfo,
             FIFFLIB::FiffDigitizerData::SPtr pDigitizerData)
            :m_pInfo(pInfo)
            ,m_pDigitizerData(pDigitizerData){};

    FIFFLIB::FiffInfo::SPtr m_pInfo;                   /**< Measurement information (channel layout, sampling rate, etc.). */
    FIFFLIB::FiffDigitizerData::SPtr m_pDigitizerData;  /**< Head-shape digitizer point set. */
};

//=============================================================================================================
/**
 * @brief @c QTcpSocket subclass that talks the FIFF wire dialect of the @c mne_rt_server data port (default 4218).
 *
 * Negotiates the per-session client id with @c setClientAlias /
 * @c getClientId, fetches the @c FiffInfo (and optionally the digitiser
 * point set) via @c readInfo / @c readMetadata, and then blocks on
 * @c readRawBuffer to deliver one @c Eigen::MatrixXf sample matrix at a
 * time. Parsing is delegated to @ref FIFFLIB::FiffStream / @c FiffTag so
 * this class never re-implements the binary framing; the only socket
 * state it owns beyond the @c QTcpSocket base is the assigned
 * @c m_clientID.
 */
class COMSHARED_EXPORT RtDataClient : public QTcpSocket
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtDataClient> SPtr;               /**< Shared pointer type for RtDataClient. */
    typedef QSharedPointer<const RtDataClient> ConstSPtr;    /**< Const shared pointer type for RtDataClient. */

    //=========================================================================================================
    /**
     * Creates the real-time data client.
     *
     * @param[in] parent     Parent QObject (optional).
     */
    explicit RtDataClient(QObject *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Attempts to close the socket. If there is pending data waiting to be written, QAbstractSocket will enter
     * ClosingState and wait until all data has been written. Eventually, it will enter UnconnectedState and
     * emit the disconnected() signal.
     */
    virtual void disconnectFromHost();

    //=========================================================================================================
    /**
     * Requests the ID at mne_rt_server and returns it
     *
     * @return the requested id.
     */
    qint32 getClientId();

    //=========================================================================================================
    /**
     * Reads fiff measurement information of a data the connection
     *
     * @return the read fiff measurement information.
     */
    FIFFLIB::FiffInfo::SPtr readInfo();

    //=========================================================================================================
    /**
     * Reads fiff metaata class from data connection
     *
     * @return returns metadata class (FiffInfo and FiffDigitizerData)
     */
    MetaData readMetadata();

    //=========================================================================================================
    /**
     * Reads fiff measurement information of a data the connection
     *
     * @param[in] p_nChannels    Number of channels to reshape the received data.
     * @param[out] data          The read data - ToDo change this to raw buffer data object.
     * @param[out] kind          Data kind.
     */
    void readRawBuffer(qint32 p_nChannels,
                       Eigen::MatrixXf& data,
                       FIFFLIB::fiff_int_t& kind);

    //=========================================================================================================
    /**
     * Sets the alias of the data client
     *
     * @param[in] p_sAlias    The alias of the data client.
     */
    void setClientAlias(const QString &p_sAlias);

private:
    qint32 m_clientID;  /**< Corresponding client id of the data client at mne_rt_server. */
    
};
} // NAMESPACE

#endif // RTDATACLIENT_H
