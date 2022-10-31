//=============================================================================================================
/**
 * @file     realtimeconnectivityestimate.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     October, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the RealTimeConnectivityEstimate class.
 *
 */

#ifndef REALTIMECONNECTIVITYESTIMATE_H
#define REALTIMECONNECTIVITYESTIMATE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scmeas_global.h"
#include "measurement.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace CONNECTIVITYLIB {
    class Network;
}

namespace FIFFLIB {
    class FiffInfo;
}

namespace MNELIB {
    class MNEForwardSolution;
    class MNEBem;
}

namespace FSLIB {
    class SurfaceSet;
    class AnnotationSet;
}

//=============================================================================================================
// DEFINE NAMESPACE SCMEASLIB
//=============================================================================================================

namespace SCMEASLIB
{

//=========================================================================================================
/**
 * RealTimeConnectivityEstimate
 *
 * @brief Real-time source estimate measurement.
 */
class SCMEASSHARED_EXPORT RealTimeConnectivityEstimate : public Measurement
{
public:
    typedef QSharedPointer<RealTimeConnectivityEstimate> SPtr;               /**< Shared pointer type for RealTimeConnectivityEstimate. */
    typedef QSharedPointer<const RealTimeConnectivityEstimate> ConstSPtr;    /**< Const shared pointer type for RealTimeConnectivityEstimate. */

    //=========================================================================================================
    /**
     * Constructs a RealTimeConnectivityEstimate.
     *
     * @param[in] parent     the QObject parent of this measurement.
     */
    RealTimeConnectivityEstimate(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RealTimeConnectivityEstimate.
     */
    virtual ~RealTimeConnectivityEstimate();

    //=========================================================================================================
    /**
     * Sets the annotation set.
     *
     * @param[in] annotSet   the annotation set to set.
     */
    inline void setAnnotSet(const QSharedPointer<FSLIB::AnnotationSet>& annotSet);

    //=========================================================================================================
    /**
     * Returns the annotation set.
     *
     * @return the annotation set.
     */
    inline QSharedPointer<FSLIB::AnnotationSet>& getAnnotSet();

    //=========================================================================================================
    /**
     * Sets the sensor surface.
     *
     * @param[in] sensorSurface   the sensor surface.
     */
    inline void setSensorSurface(const QSharedPointer<MNELIB::MNEBem>& sensorSurface);

    //=========================================================================================================
    /**
     * Returns the sensor surface.
     *
     * @return the sensor surfac.
     */
    inline QSharedPointer<MNELIB::MNEBem>& getSensorSurface();

    //=========================================================================================================
    /**
     * Sets the sensor surface.
     *
     * @param[in] surfSet   the surface set to set.
     */
    inline void setSurfSet(const QSharedPointer<FSLIB::SurfaceSet>& surfSet);

    //=========================================================================================================
    /**
     * Returns the surface set.
     *
     * @return the surface set.
     */
    inline QSharedPointer<FSLIB::SurfaceSet> &getSurfSet();

    //=========================================================================================================
    /**
     * Sets the forward solution.
     *
     * @param[in] fwdSolution   the forward solution to set.
     */
    inline void setFwdSolution(const QSharedPointer<MNELIB::MNEForwardSolution>& fwdSolution);

    //=========================================================================================================
    /**
     * Returns the forward solution.
     *
     * @return the forward solution.
     */
    inline QSharedPointer<MNELIB::MNEForwardSolution>& getFwdSolution();

    //=========================================================================================================
    /**
     * Attaches a value to the sample array vector.
     * This method is inherited by Measurement.
     *
     * @param[in] v the value which is attached to the sample array vector.
     */
    virtual void setValue(const CONNECTIVITYLIB::Network &v);

    //=========================================================================================================
    /**
     * Returns the current value set.
     * This method is inherited by Measurement.
     *
     * @return the last attached value.
     */
    virtual QSharedPointer<CONNECTIVITYLIB::Network>& getValue();

    //=========================================================================================================
    /**
     * Returns whether RealTimeConnectivityEstimate contains values
     *
     * @return whether RealTimeConnectivityEstimate contains values.
     */
    inline bool isInitialized() const;

    //=========================================================================================================
    /**
     * Sets the current FiffInfo.
     *
     * @param[in] p_fiffInfo the new FiffInfo..
     */
    void setFiffInfo(const QSharedPointer<FIFFLIB::FiffInfo>& p_fiffInfo);

    //=========================================================================================================
    /**
     * Returns the current FiffInfo.
     *
     * @return the current FiffInfo.
     */
    QSharedPointer<FIFFLIB::FiffInfo> getFiffInfo();

private:
    mutable QMutex                              m_qMutex;           /**< Mutex to ensure thread safety. */

    QSharedPointer<FIFFLIB::FiffInfo>           m_pFiffInfo;        /**< The Fiff info. */

    QSharedPointer<FSLIB::AnnotationSet>        m_pAnnotSet;        /**< Annotation set. Needed for visualization. */
    QSharedPointer<FSLIB::SurfaceSet>           m_pSurfSet;         /**< Surface set. Needed for visualization. */
    QSharedPointer<MNELIB::MNEForwardSolution>  m_pFwdSolution;     /**< Forward solution. Needed for visualization. */
    QSharedPointer<MNELIB::MNEBem>              m_pSensorSurface;   /**< The sensor surface. Needed for visualization. */

    QSharedPointer<CONNECTIVITYLIB::Network>    m_pNetwork;         /**< The network/connectivity estimate. */
    bool                                        m_bInitialized;     /**< Is initialized. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void RealTimeConnectivityEstimate::setAnnotSet(const QSharedPointer<FSLIB::AnnotationSet> &annotSet)
{
    QMutexLocker locker(&m_qMutex);
    m_pAnnotSet = annotSet;
}

//=============================================================================================================

inline QSharedPointer<FSLIB::AnnotationSet>& RealTimeConnectivityEstimate::getAnnotSet()
{
    QMutexLocker locker(&m_qMutex);
    return m_pAnnotSet;
}

//=============================================================================================================

inline void RealTimeConnectivityEstimate::setSensorSurface(const QSharedPointer<MNELIB::MNEBem> &annotSet)
{
    QMutexLocker locker(&m_qMutex);
    m_pSensorSurface = annotSet;
}

//=============================================================================================================

inline QSharedPointer<MNELIB::MNEBem>& RealTimeConnectivityEstimate::getSensorSurface()
{
    QMutexLocker locker(&m_qMutex);
    return m_pSensorSurface;
}

//=============================================================================================================

inline void RealTimeConnectivityEstimate::setSurfSet(const QSharedPointer<FSLIB::SurfaceSet> &surfSet)
{
    QMutexLocker locker(&m_qMutex);
    m_pSurfSet = surfSet;
}

//=============================================================================================================

inline QSharedPointer<FSLIB::SurfaceSet>& RealTimeConnectivityEstimate::getSurfSet()
{
    QMutexLocker locker(&m_qMutex);
    return m_pSurfSet;
}

//=============================================================================================================

inline void RealTimeConnectivityEstimate::setFwdSolution(const QSharedPointer<MNELIB::MNEForwardSolution>& fwdSolution)
{
    QMutexLocker locker(&m_qMutex);
    m_pFwdSolution = fwdSolution;
}

//=============================================================================================================

inline QSharedPointer<MNELIB::MNEForwardSolution>& RealTimeConnectivityEstimate::getFwdSolution()
{
    QMutexLocker locker(&m_qMutex);
    return m_pFwdSolution;
}

//=============================================================================================================

inline bool RealTimeConnectivityEstimate::isInitialized() const
{
    QMutexLocker locker(&m_qMutex);
    return m_bInitialized;
}

//=============================================================================================================

inline void RealTimeConnectivityEstimate::setFiffInfo(const QSharedPointer<FIFFLIB::FiffInfo>& p_fiffInfo)
{
    QMutexLocker locker(&m_qMutex);
    m_pFiffInfo = p_fiffInfo;
}

//=============================================================================================================

inline QSharedPointer<FIFFLIB::FiffInfo> RealTimeConnectivityEstimate::getFiffInfo()
{
    QMutexLocker locker(&m_qMutex);
    return m_pFiffInfo;
}
} // NAMESPACE

Q_DECLARE_METATYPE(SCMEASLIB::RealTimeConnectivityEstimate::SPtr)

#endif // REALTIMECONNECTIVITYESTIMATE_H
