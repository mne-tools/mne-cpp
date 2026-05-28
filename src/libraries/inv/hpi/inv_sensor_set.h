//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_sensor_set.h
 * @since 2026
 * @date  March 2026
 * @brief Compact MEG sensor-geometry container (positions, orientations, integration weights) used by the HPI fitter.
 *
 * @ref INVLIB::InvSensorSet replaces the verbose
 * @ref FWDLIB::FwdCoilSet for read-only use by the HPI fit loop: it
 * stores per-channel integration-point positions @c rmag, surface
 * normals @c cosmag, channel transforms @c tra and integration weights
 * @c w in dense Eigen matrices, plus the linear-combination matrix
 * that maps coil integration points back to channels. The
 * @ref InvSensorSetCreator helper builds an @ref InvSensorSet from an
 * @ref FWDLIB::FwdCoilSet, picking the requested integration accuracy
 * (high / medium / low) so the HPI cost function can run on a small
 * hot-loop-friendly data structure.
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

