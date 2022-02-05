//=============================================================================================================
/**
 * @file     sensorset.h
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.0
 * @date     November, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Ruben Dörfel. All rights reserved.
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
 * @brief     SensorSet class declaration.
 *
 */

#ifndef SENSORSET_H
#define SENSORSET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FWDLIB{
    class FwdCoil;
    class FwdCoilSet;
}

namespace FIFFLIB{
    class FiffCoordTransOld;
    class FiffDigPointSet;
    class FiffChInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

/**
 * The strucut specifing the sensor parameters.
 */
struct SensorSet {
    Eigen::MatrixXd ez;
    Eigen::MatrixXd r0;
    Eigen::MatrixXd rmag;
    Eigen::MatrixXd cosmag;
    Eigen::MatrixXd tra;
    Eigen::RowVectorXd w;
    int ncoils;
    int np;
};

//=============================================================================================================
// INVERSELIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Create a SensorSet struct from a channel list with specified accuracy.
 *
 * @brief Brief description of this class.
 */
class INVERSESHARED_EXPORT SensorSetCreator
{

public:
    typedef QSharedPointer<SensorSet> SPtr;            /**< Shared pointer type for SensorSet. */
    typedef QSharedPointer<const SensorSet> ConstSPtr; /**< Const shared pointer type for SensorSet. */

    //=========================================================================================================
    /**
    * Constructs a MEG SensorSet object.
    */
    explicit SensorSetCreator();

    //=========================================================================================================
    /**
     * Update SensorSet from new channel list with new accuracy.
     *
     * @param[in] channelList   The channel list to create the MEG sensor set from.
     * @param[in] iAccuracy     The accuracy level to use for the sensor set.
     *
     */

    SensorSet updateSensorSet(const QList<FIFFLIB::FiffChInfo>& channelList,
                         const int iAccuracy);

    Eigen::MatrixXd ez;
    Eigen::MatrixXd r0;
    Eigen::MatrixXd rmag;
    Eigen::MatrixXd cosmag;
    Eigen::MatrixXd tra;
    Eigen::RowVectorXd w;
    int ncoils;
    int np;

protected:

private:

    //=========================================================================================================
    /**
     * read coil definitions if necessary
     *
     */
    void readCoilDefinitions();

    //=========================================================================================================
    /**
     * convert data from FwdCoilSet to SensorSet.
     *
     * @param[in] pCoilMeg   The initialized fwd coilset to get data from.
     *
     */
    void convertFromFwdCoilSet(const QSharedPointer<FWDLIB::FwdCoilSet> pCoilMeg);

    //=========================================================================================================
    /**
     * initialize member matrices for specific size.
     * @param[in] iNchan   The number of channels.
     * @param[in] iAcc     The number of integration points.
     */
    void initMatrices(const int iNchan, const int iNp);

    SensorSet m_sensors;
    QSharedPointer<FWDLIB::FwdCoilSet>  m_pCoilDefinitions;    // the coil definitions as template
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // namespace INVERSELIB

#endif // SENSORSET_H

