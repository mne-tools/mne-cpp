//=============================================================================================================
/**
 * @file     sensorset.h
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
enum class Accuracy : int{high = 2, medium = 1, low = 0};

class INVERSESHARED_EXPORT SensorSet {

public:
    typedef QSharedPointer<SensorSet> SPtr;            /**< Shared pointer type for SensorSet. */
    typedef QSharedPointer<const SensorSet> ConstSPtr; /**< Const shared pointer type for SensorSet. */

    //=========================================================================================================
    /**
     * Default Constructor.
     */
    explicit SensorSet() = default;

    //=========================================================================================================
    /**
     * SensorSet that yiealds sensor position, orientation and weights for geometric averaging.
     *
     * @param[in] pFwdCoilSet   The FwdCoilSet to extract the data from.
     *
     */
    explicit SensorSet(const QSharedPointer<FWDLIB::FwdCoilSet> pFwdCoilSet);

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

    inline bool operator== (const SensorSet &b) const;
    inline bool operator!= (const SensorSet &b) const;

private:
    //=========================================================================================================
    /**
     * Convert data from FwdCoilSet to the sensorset format.
     * @param[in] iNchan   The number of channels.
     * @param[in] iAcc     The number of integration points.
     */
    void convertFromFwdCoilSet(const QSharedPointer<FWDLIB::FwdCoilSet> pFwdCoilSet);

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

inline int SensorSet::np() const
{
    return m_np;
}

inline int SensorSet::ncoils() const
{
    return m_ncoils;
}

inline Eigen::VectorXd SensorSet::ez(int iSensor) const
{
    return m_ez.row(iSensor);
}

inline Eigen::MatrixXd SensorSet::ez() const
{
    return m_ez;
}

inline Eigen::VectorXd SensorSet::r0(int iSensor) const
{
    return m_r0.row(iSensor);
}

inline Eigen::MatrixXd SensorSet::r0() const
{
    return m_r0;
}

inline Eigen::RowVectorXd SensorSet::w(int iSensor) const
{
    return m_w.segment(iSensor*m_np,m_np);
}

inline Eigen::RowVectorXd SensorSet::w() const
{
    return m_w;
}

inline Eigen::MatrixXd SensorSet::rmag(int iSensor) const
{
    return m_rmag.block(iSensor*m_np,0,m_np,3);
}

inline Eigen::MatrixXd SensorSet::rmag() const
{
    return m_rmag;
}

inline Eigen::MatrixXd SensorSet::cosmag(int iSensor) const
{
    return m_cosmag.block(iSensor*m_np,0,m_np,3);
}

inline Eigen::MatrixXd SensorSet::cosmag() const
{
    return m_cosmag;
}

inline Eigen::MatrixXd SensorSet::tra() const
{
    return m_tra;
}
//=============================================================================================================

inline bool SensorSet::operator== (const SensorSet &b) const
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

inline bool SensorSet::operator!= (const SensorSet &b) const
{
    bool equal = this==&b;
    return !(equal);
}

//=============================================================================================================
/**
 * Create a SensorSet struct from a channel list with specified accuracy.
 *
 * @brief Brief description of this class.
 */
class INVERSESHARED_EXPORT SensorSetCreator
{

public:
    typedef QSharedPointer<SensorSetCreator> SPtr;            /**< Shared pointer type for SensorSet. */
    typedef QSharedPointer<const SensorSetCreator> ConstSPtr; /**< Const shared pointer type for SensorSet. */

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
                              const Accuracy accuracy);

private:
    QSharedPointer<FWDLIB::FwdCoilSet>  m_pCoilDefinitions{nullptr};    // the coil definitions as template
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // namespace INVERSELIB

#endif // SENSORSET_H

