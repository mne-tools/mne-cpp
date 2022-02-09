//=============================================================================================================
/**
 * @file     hpidataupdater.h
 * @author   Ruben DÖrfel <doerfelruben@aol.com>
 * @since    0.1.9
 * @date     December, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Ruben DÖrfel. All rights reserved.
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
 * @brief     HpiDataUpdater class declaration.
 *
 */

#ifndef HPIDATAUPDATER_H
#define HPIDATAUPDATER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include "sensorset.h"
#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_dig_point.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB{
    class FiffInfo;
    class FiffChInfo;
    class FiffDigPointSet;
}

//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

//=============================================================================================================
// Declare all structures to be used
//=============================================================================================================

//=============================================================================================================
// INVERSELIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * This class brings all the data and objects used for HPI fitting into the right format.
 *
 * @brief Brief description of this class.
 */
class INVERSESHARED_EXPORT HpiDataUpdater
{

public:
    typedef QSharedPointer<HpiDataUpdater> SPtr;            /**< Shared pointer type for HpiDataUpdater. */
    typedef QSharedPointer<const HpiDataUpdater> ConstSPtr; /**< Const shared pointer type for HpiDataUpdater. */

    //=========================================================================================================
    /**
    * Constructs a HpiDataUpdater object.
    */
    HpiDataUpdater(const QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * Check if information in FiffInfo changed and update if necessary.
     *
     * @param[in] pFiffInfo     The FiffInfo to check for changes.
     *
     */
    void checkForUpdate(const QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * Reduce data to only use good channels.
     *
     * @param[in] matProjectors     The projector matrix.
     *
     */
    void prepareDataAndProjectors(const Eigen::MatrixXd& matData, const Eigen::MatrixXd& matProjectors);

    //=========================================================================================================
    /**
     * inline get functions for private member variables.
     *
     */
    inline const QList<FIFFLIB::FiffChInfo>& getChannels();
    inline const Eigen::MatrixXd& getProjectors();
    inline const Eigen::MatrixXd& getHpiDigitizer();
    inline const Eigen::MatrixXd& getData();
    inline const Eigen::MatrixXd& getProjectedData();
    inline const SensorSet& getSensors();

protected:

private:
    //=========================================================================================================
    /**
     * Update the channellist for init and if bads changed
     *
     * @param[in] pFiffInfo         The FiffInfo file from the measurement.
     *
     */
    void updateChannels(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * Update the list of bad channels
     *
     * @param[in] pFiffInfo         The FiffInfo file from the measurement.
     *
     */
    void updateBadChannels(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
     * Update the digitized HPI coils.
     * @param[in]   lDig            The digitizer list to extract the hpi coils from.
     *
     */
    void updateHpiDigitizer(const QList<FIFFLIB::FiffDigPoint>& lDig);

    //=========================================================================================================
    /**
     * Update sensors struct.
     *
     * @param[in] lChannels       The channel list to update the sensors with.
     *
     */
    void updateSensors(const QList<FIFFLIB::FiffChInfo>& lChannels);

    //=========================================================================================================
    /**
     * Check if channel data in FiffInfo has changed. Compare to members and return true if changed.
     *
     * @param[in] lBads       The bad channel list to check.
     * @param[in] lChannels   The channel list to check.
     *
     * @return true if changed
     */
    bool checkIfChanged(const QList<QString>& lBads, const QList<FIFFLIB::FiffChInfo>& lChannels);

    //=========================================================================================================
    /**
     * Reduce data to only use good channels.
     *
     * @param[in] matProjectors     The projector matrix.
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

    QList<FIFFLIB::FiffChInfo> m_lChannels; /**< Channellist with bads excluded. */
    QVector<int> m_vecInnerind;             /**< index of inner channels . */
    QList<QString> m_lBads;                 /**< contains bad channels . */
    Eigen::MatrixXd m_matHpiDigitizer;      /**< The coordinates of the digitized HPI coils in head space*/
    Eigen::MatrixXd m_matProjectors;        /**< The projectors ready to use*/
    Eigen::MatrixXd m_matInnerdata;         /**< The data ready to use*/
    Eigen::MatrixXd m_matDataProjected;     /**< The data with projectros applied*/
    SensorSetCreator m_sensorSetCreator;    /**< ThesensorSetCreator to create a sensorset*/
    SensorSet m_sensors;                    /**< The most recent SensorSet*/

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const QList<FIFFLIB::FiffChInfo>& HpiDataUpdater::getChannels()
{
    return m_lChannels;
}

inline const Eigen::MatrixXd& HpiDataUpdater::getProjectors()
{
    return m_matProjectors;
}

inline const Eigen::MatrixXd& HpiDataUpdater::getData()
{
    return m_matInnerdata;
}

inline const Eigen::MatrixXd& HpiDataUpdater::getProjectedData()
{
    return m_matDataProjected;
}

inline const Eigen::MatrixXd& HpiDataUpdater::getHpiDigitizer()
{
    return m_matHpiDigitizer;
}

inline const SensorSet& HpiDataUpdater::getSensors()
{
    return m_sensors;
}

} // namespace INVERSELIB

#endif // HPIDATAUPDATER_H

