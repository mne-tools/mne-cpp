//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_hpi_data_updater.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Pre-processing front-end for HPI fitting — re-shapes raw MEG data, projectors and digitised coils into per-fit inputs.
 *
 * @ref mne::InvHpiDataUpdater isolates everything that has to
 * happen before @ref mne::InvHpiFit can run on a new measurement block:
 * re-deriving the good-channel list from the @c FiffInfo, slicing the
 * SSP projector to those channels, projecting the data buffer, lifting
 * the digitised HPI coil positions out of the dig-point list and
 * rebuilding the @ref mne::InvSensorSet whenever the channel layout changes.
 * The class caches the most recent state so consecutive HPI fits on
 * the same configuration only re-run the steps that actually changed.
 */

#ifndef INV_HPI_DATA_UPDATER_H
#define INV_HPI_DATA_UPDATER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"
#include <memory>

#include <vector>
#include "inv_sensor_set.h"
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_dig_point.h>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffInfo;
class FiffDigPointSet;
}

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// Declare all structures to be used
//=============================================================================================================

//=============================================================================================================
// mne FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * This class brings all the data and objects used for HPI fitting into the right format.
 *
 * @brief Preprocesses raw HPI coil data (SSP projection, compensation, sinusoidal model fitting) before dipole localization
 *
 * @snippet ex_inv_hpi/main.cpp inv_hpi_usage
 */
class INVSHARED_EXPORT InvHpiDataUpdater
{
public:
    //=========================================================================================================
    /**
    * Constructs a InvHpiDataUpdater object.
    *
    * @param[in] pFiffInfo     Measurement info used to initialize channels, bads, HPI digitizer points and sensors.
    */
    InvHpiDataUpdater(const std::shared_ptr<mne::FiffInfo>& pFiffInfo);

    //=========================================================================================================
    /**
     * Check if information in FiffInfo changed and update if necessary.
     *
     * @param[in] pFiffInfo     The FiffInfo to check for changes.
     *
     */
    void checkForUpdate(const std::shared_ptr<mne::FiffInfo>& pFiffInfo);

    //=========================================================================================================
    /**
     * Reduce data to only use good channels.
     *
     * @param[in] matData Data matrix (channels x samples).
     * @param[in] matProjectors     The projector matrix.
     *
     */
    void prepareDataAndProjectors(const Eigen::MatrixXd& matData, const Eigen::MatrixXd& matProjectors);

    //=========================================================================================================
    /**
     * inline get functions for private member variables.
     *
     * @return Const reference to the channel list with bad channels excluded.
     */
    inline const std::vector<mne::FiffChInfo>& getChannels();
    inline const Eigen::MatrixXd& getProjectors();
    inline const Eigen::MatrixXd& getHpiDigitizer();
    inline const Eigen::MatrixXd& getData();
    inline const Eigen::MatrixXd& getProjectedData();
    inline const InvSensorSet& getSensors();

private:
    //=========================================================================================================
    /**
     * Update the channellist for init and if bads changed
     *
     * @param[in] pFiffInfo         The FiffInfo file from the measurement.
     *
     */
    void updateChannels(const std::shared_ptr<mne::FiffInfo>& pFiffInfo);

    //=========================================================================================================
    /**
     * Update the list of bad channels
     *
     * @param[in] pFiffInfo         The FiffInfo file from the measurement.
     *
     */
    void updateBadChannels(const std::shared_ptr<mne::FiffInfo>& pFiffInfo);

    //=========================================================================================================
    /**
     * Update the digitized HPI coils.
     * @param[in]   dig             The digitizer points to extract the hpi coils from.
     *
     */
    void updateHpiDigitizer(const mne::FiffDigPointSet& dig);

    //=========================================================================================================
    /**
     * Update sensors struct.
     *
     * @param[in] lChannels       The channel list to update the sensors with.
     *
     */
    void updateSensors(const std::vector<mne::FiffChInfo>& lChannels);

    //=========================================================================================================
    /**
     * Check if channel data in FiffInfo has changed. Compare to members and return true if changed.
     *
     * @param[in] lBads       The bad channel list to check.
     * @param[in] lChannels   The channel list to check.
     *
     * @return true if changed
     */
    bool checkIfChanged(const StringList& lBads, const std::vector<mne::FiffChInfo>& lChannels);

    //=========================================================================================================
    /**
     * Reduce data to only use good channels.
     *
     * @param[in] matData     The projector matrix.
     *
     */
    void prepareData(const Eigen::MatrixXd& matData);

    //=========================================================================================================
    /**
     * Reduce projectors to only use good channels.
     *
     * @param[in] matProjectors     The projector matrix.
     *
     */
    void prepareProjectors(const Eigen::MatrixXd& matProjectors);

    std::vector<mne::FiffChInfo> m_lChannels; /**< Channellist with bads excluded. */
    std::vector<int> m_vecInnerind;           /**< index of inner channels . */
    StringList m_lBads;                       /**< contains bad channels . */
    Eigen::MatrixXd m_matHpiDigitizer;        /**< The coordinates of the digitized HPI coils in head space*/
    Eigen::MatrixXd m_matProjectors;          /**< The projectors ready to use*/
    Eigen::MatrixXd m_matInnerdata;           /**< The data ready to use*/
    Eigen::MatrixXd m_matDataProjected;       /**< The data with projectros applied*/
    InvSensorSetCreator m_sensorSetCreator;   /**< ThesensorSetCreator to create a sensorset*/
    InvSensorSet m_sensors;                   /**< The most recent InvSensorSet*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const std::vector<mne::FiffChInfo>& InvHpiDataUpdater::getChannels()
{
    return m_lChannels;
}

inline const Eigen::MatrixXd& InvHpiDataUpdater::getProjectors()
{
    return m_matProjectors;
}

inline const Eigen::MatrixXd& InvHpiDataUpdater::getData()
{
    return m_matInnerdata;
}

inline const Eigen::MatrixXd& InvHpiDataUpdater::getProjectedData()
{
    return m_matDataProjected;
}

inline const Eigen::MatrixXd& InvHpiDataUpdater::getHpiDigitizer()
{
    return m_matHpiDigitizer;
}

inline const InvSensorSet& InvHpiDataUpdater::getSensors()
{
    return m_sensors;
}

} // namespace mne

#endif // INV_HPI_DATA_UPDATER_H
