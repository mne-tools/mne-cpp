//=============================================================================================================
/**
 * @file     inv_sensor_set.h
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.9
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
 * @brief     InvSensorSet class declaration.
 *
 */

#ifndef INV_SENSOR_SET_H
#define INV_SENSOR_SET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"

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
    class FiffCoordTrans;
    class FiffDigPointSet;
    class FiffChInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE HPILIB
//=============================================================================================================

namespace INVLIB
{
enum class Accuracy : int{high = 2, medium = 1, low = 0};

/**
 * @brief Stores MEG sensor geometry (positions, orientations, weights, coil count) for a single sensor type
 */
class INVSHARED_EXPORT InvSensorSet {

public:
    typedef QSharedPointer<InvSensorSet> SPtr;            /**< Shared pointer type for InvSensorSet. */
    typedef QSharedPointer<const InvSensorSet> ConstSPtr; /**< Const shared pointer type for InvSensorSet. */

    //=========================================================================================================
    /**
     * Default Constructor.
     */
    explicit InvSensorSet() = default;

    //=========================================================================================================
    /**
     * InvSensorSet that yiealds sensor position, orientation and weights for geometric averaging.
     *
     * @param[in] pFwdCoilSet   The FwdCoilSet to extract the data from.
     *
     */
    explicit InvSensorSet(const QSharedPointer<FWDLIB::FwdCoilSet> pFwdCoilSet);

    inline int np() const;
    inline int ncoils() const;
    inline Eigen::VectorXd ez(int iSensor) const;
    inline Eigen::MatrixXd ez() const;

    inline Eigen::VectorXd r0(int iSensor) const;
    inline Eigen::MatrixXd r0() const;

    inline Eigen::MatrixXd rmag(int iSensor) const;
    inline Eigen::MatrixXd rmag() const;

    inline Eigen::MatrixXd cosmag(int iSensor) const;
    inline Eigen::MatrixXd cosmag() const;

    inline Eigen::MatrixXd tra(int iSensor) const;
    inline Eigen::MatrixXd tra() const;

    inline Eigen::RowVectorXd w(int iSensor) const;
    inline Eigen::RowVectorXd w() const;

    inline bool operator== (const InvSensorSet &b) const;
    inline bool operator!= (const InvSensorSet &b) const;

private:
    //=========================================================================================================
    /**
     * Initialize data from FwdCoilSet to the sensorset format.
     * @param[in] iNchan   The number of channels.
     * @param[in] iAcc     The number of integration points.
     */
    void initFromFwdCoilSet(const QSharedPointer<FWDLIB::FwdCoilSet> pFwdCoilSet);

    //=========================================================================================================
    /**
     * Initialize member matrices for specific size.
     * @param[in] iNchan   The number of channels.
     * @param[in] iAcc     The number of integration points.
     */
    void initMatrices(int iNchan, int iNp);

    Eigen::MatrixXd m_ez{Eigen::MatrixXd(0,0)};
    Eigen::MatrixXd m_r0{Eigen::MatrixXd(0,0)};
    Eigen::MatrixXd m_rmag{Eigen::MatrixXd(0,0)};
    Eigen::MatrixXd m_cosmag{Eigen::MatrixXd(0,0)};
    Eigen::MatrixXd m_tra{Eigen::MatrixXd(0,0)};
    Eigen::RowVectorXd m_w{Eigen::RowVectorXd(0)};
    int m_ncoils{0};
    int m_np{0};
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline int InvSensorSet::np() const
{
    return m_np;
}

inline int InvSensorSet::ncoils() const
{
    return m_ncoils;
}

inline Eigen::VectorXd InvSensorSet::ez(int iSensor) const
{
    return m_ez.row(iSensor);
}

inline Eigen::MatrixXd InvSensorSet::ez() const
{
    return m_ez;
}

inline Eigen::VectorXd InvSensorSet::r0(int iSensor) const
{
    return m_r0.row(iSensor);
}

inline Eigen::MatrixXd InvSensorSet::r0() const
{
    return m_r0;
}

inline Eigen::RowVectorXd InvSensorSet::w(int iSensor) const
{
    return m_w.segment(iSensor*m_np,m_np);
}

inline Eigen::RowVectorXd InvSensorSet::w() const
{
    return m_w;
}

inline Eigen::MatrixXd InvSensorSet::rmag(int iSensor) const
{
    return m_rmag.block(iSensor*m_np,0,m_np,3);
}

inline Eigen::MatrixXd InvSensorSet::rmag() const
{
    return m_rmag;
}

inline Eigen::MatrixXd InvSensorSet::cosmag(int iSensor) const
{
    return m_cosmag.block(iSensor*m_np,0,m_np,3);
}

inline Eigen::MatrixXd InvSensorSet::cosmag() const
{
    return m_cosmag;
}

inline Eigen::MatrixXd InvSensorSet::tra() const
{
    return m_tra;
}
//=============================================================================================================

inline bool InvSensorSet::operator== (const InvSensorSet &b) const
{
    return (this->ez() == b.ez() &&
            this->r0() == b.r0() &&
            this->rmag() == b.rmag() &&
            this->cosmag() == b.cosmag() &&
            this->tra() == b.tra() &&
            this->w() == b.w() &&
            this->np() == b.np() &&
            this->ncoils() == b.ncoils());
}

//=============================================================================================================

inline bool InvSensorSet::operator!= (const InvSensorSet &b) const
{
    bool equal = this==&b;
    return !(equal);
}

//=============================================================================================================
/**
 * Create a InvSensorSet struct from a channel list with specified accuracy.
 *
 * @brief Builds InvSensorSet objects from FiffInfo channel definitions, applying SSP projections and compensation
 */
class INVSHARED_EXPORT InvSensorSetCreator
{

public:
    typedef QSharedPointer<InvSensorSetCreator> SPtr;            /**< Shared pointer type for InvSensorSet. */
    typedef QSharedPointer<const InvSensorSetCreator> ConstSPtr; /**< Const shared pointer type for InvSensorSet. */

    //=========================================================================================================
    /**
    * Constructs a MEG InvSensorSet object.
    */
    explicit InvSensorSetCreator();

    //=========================================================================================================
    /**
     * Update InvSensorSet from new channel list with new accuracy.
     *
     * @param[in] channelList   The channel list to create the MEG sensor set from.
     * @param[in] iAccuracy     The accuracy level to use for the sensor set.
     *
     */
    InvSensorSet updateSensorSet(const QList<FIFFLIB::FiffChInfo>& channelList,
                              const Accuracy& accuracy);

private:
    QSharedPointer<FWDLIB::FwdCoilSet>  m_pCoilDefinitions{nullptr};    // the coil definitions as template
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // namespace INVLIB

#endif // INV_SENSOR_SET_H

