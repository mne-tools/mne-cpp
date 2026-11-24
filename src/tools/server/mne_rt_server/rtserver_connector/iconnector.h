//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     iconnector.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief    Interface of the mne_rt_server connectors.
 *
 * It lives in its own library, shared by the server and its connectors. The server creates the known connectors
 * directly and receives their measurement info and raw buffers through callbacks.
 */

#ifndef RTSERVER_ICONNECTOR_H
#define RTSERVER_ICONNECTOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtserver_connector_global.h"

#include <mne/base/json.h>
#include <mne/fiff/fiff_info.h>
#include <rtserver_connector/command_manager.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

//=============================================================================================================
// ENUMERATIONS
//=============================================================================================================

/**
 * Connector IDs
 */
enum ConnectorID
{
    FIFF_SIMULATOR_CONNECTOR = 1, /**< The FIFF file simulator. */
    DEFAULT_CONNECTOR = -1        /**< No specific connector. */
};

//=============================================================================================================
// DEFINES
//=============================================================================================================

inline constexpr auto RAW_BUFFFER_SIZE = 10;

//=============================================================================================================
/**
 * @brief Interface of a data source of mne_rt_server.
 *
 * A connector produces the measurement info on request and raw buffers while it runs; both reach the server through
 * the callbacks set with @ref setMeasInfoCallback and @ref setRawBufferCallback, possibly from the connector's own
 * thread.
 */
class RTSERVER_CONNECTOR_EXPORT IConnector
{
public:
    /** Receives the measurement info requested for the FiffStreamClient with the given ID. */
    using MeasInfoCallback = std::function<void(std::int32_t, const mne::FiffInfo&)>;
    /** Receives a raw buffer (channels x samples). */
    using RawBufferCallback = std::function<void(std::shared_ptr<const Eigen::MatrixXf>)>;

    IConnector();

    IConnector(const IConnector&) = delete;
    IConnector& operator=(const IConnector&) = delete;

    virtual ~IConnector();

    //=========================================================================================================
    /**
     * @return The command manager of the connector.
     */
    inline RTSERVER::CommandManager& getCommandManager();

    //=========================================================================================================
    /**
     * Connects the commands of the command manager to the handlers of the connector.
     */
    virtual void connectCommandManager() = 0;

    //=========================================================================================================
    /**
     * @return The connector ID.
     */
    virtual ConnectorID getConnectorID() const = 0;

    //=========================================================================================================
    /**
     * @return The connector name.
     */
    virtual const char* getName() const = 0;

    //=========================================================================================================
    /**
     * @return The JSON description of the connector, with the "commands" it understands.
     */
    virtual const char* getMetaData() const = 0;

    //=========================================================================================================
    /**
     * @return Whether the connector is the active one.
     */
    inline bool isActive() const;

    //=========================================================================================================
    /**
     * Starts the acquisition.
     *
     * @return True if the acquisition started.
     */
    virtual bool start() = 0;

    //=========================================================================================================
    /**
     * Stops the acquisition.
     *
     * @return True if the acquisition stopped.
     */
    virtual bool stop() = 0;

    //=========================================================================================================
    /**
     * @param[in] p_MetaData     The meta data of the connector.
     */
    inline void setMetaData(const mne::JsonObject& p_MetaData);

    //=========================================================================================================
    /**
     * Activates or deactivates the connector and its command manager.
     *
     * @param[in] status     The activation status.
     */
    inline void setStatus(bool status);

    //=========================================================================================================
    /**
     * Requests the measurement info for a FiffStreamClient; it arrives through the measurement info callback.
     *
     * @param[in] ID     ID of the FiffStreamClient.
     */
    virtual void info(std::int32_t ID) = 0;

    //=========================================================================================================
    /**
     * @param[in] callback   Receiver of the measurement info; empty to disconnect.
     */
    void setMeasInfoCallback(MeasInfoCallback callback);

    //=========================================================================================================
    /**
     * @param[in] callback   Receiver of the raw buffers; empty to disconnect.
     */
    void setRawBufferCallback(RawBufferCallback callback);

protected:
    //=========================================================================================================
    /**
     * Passes measurement info to the measurement info callback, if any.
     *
     * @param[in] ID         ID of the FiffStreamClient.
     * @param[in] p_info     The measurement info.
     */
    void remitMeasInfo(std::int32_t ID, const mne::FiffInfo& p_info);

    //=========================================================================================================
    /**
     * Passes a raw buffer to the raw buffer callback, if any.
     *
     * @param[in] p_buffer   The raw buffer.
     */
    void remitRawBuffer(std::shared_ptr<const Eigen::MatrixXf> p_buffer);

    mne::JsonObject m_qJsonObjectMetaData; /**< The meta data of the connector. */

    RTSERVER::CommandManager m_commandManager; /**< The CommandManager of the connector. */

private:
    bool m_bIsActive = false;              /**< Holds the activation status. */
    std::mutex m_callbackMutex;            /**< Guards the callbacks. */
    MeasInfoCallback m_measInfoCallback;   /**< Receiver of the measurement info. */
    RawBufferCallback m_rawBufferCallback; /**< Receiver of the raw buffers. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline RTSERVER::CommandManager& IConnector::getCommandManager()
{
    return m_commandManager;
}

//=============================================================================================================

inline bool IConnector::isActive() const
{
    return m_bIsActive;
}

//=============================================================================================================

inline void IConnector::setMetaData(const mne::JsonObject& p_MetaData)
{
    m_qJsonObjectMetaData = p_MetaData;
}

//=============================================================================================================

inline void IConnector::setStatus(bool status)
{
    m_bIsActive = status;
    m_commandManager.setStatus(status);
}

} //Namespace

#endif // RTSERVER_ICONNECTOR_H
