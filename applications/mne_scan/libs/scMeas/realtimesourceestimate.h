//=============================================================================================================
/**
 * @file     realtimesourceestimate.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the RealTimeSourceEstimate class.
 *
 */

#ifndef REALTIMESOURCEESTIMATE_H
#define REALTIMESOURCEESTIMATE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scmeas_global.h"
#include "measurement.h"

#include <fs/label.h>
#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include <fiff/fiff_info.h>
#include <fiff/fiff_coord_trans.h>

#include <mne/mne_sourcespace.h>
#include <mne/mne_sourceestimate.h>
#include <mne/mne_forwardsolution.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>
#include <QList>
#include <QMutex>
#include <QMutexLocker>

//=============================================================================================================
// DEFINE NAMESPACE SCMEASLIB
//=============================================================================================================

namespace SCMEASLIB
{

//=========================================================================================================
/**
 * RealTimeSourceEstimate
 *
 * @brief Real-time source estimate measurement.
 */
class SCMEASSHARED_EXPORT RealTimeSourceEstimate : public Measurement
{
public:
    typedef QSharedPointer<RealTimeSourceEstimate> SPtr;               /**< Shared pointer type for RealTimeSourceEstimate. */
    typedef QSharedPointer<const RealTimeSourceEstimate> ConstSPtr;    /**< Const shared pointer type for RealTimeSourceEstimate. */

    //=========================================================================================================
    /**
     * Constructs a RealTimeSourceEstimate.
     *
     * @param[in] parent     the QObject parent of this measurement.
     */
    RealTimeSourceEstimate(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RealTimeSourceEstimate.
     */
    virtual ~RealTimeSourceEstimate();

    //=========================================================================================================
    /**
     * Sets the annotation set.
     *
     * @param[in] annotSet   the annotation set to set.
     */
    inline void setAnnotSet(FSLIB::AnnotationSet::SPtr& annotSet);

    //=========================================================================================================
    /**
     * Returns the annotation set.
     *
     * @return the annotation set.
     */
    inline FSLIB::AnnotationSet::SPtr& getAnnotSet();

    //=========================================================================================================
    /**
     * Sets the surface set.
     *
     * @param[in] surfSet   the surface set to set.
     */
    inline void setSurfSet(FSLIB::SurfaceSet::SPtr& surfSet);

    //=========================================================================================================
    /**
     * Returns the surface set.
     *
     * @return the surface set.
     */
    inline FSLIB::SurfaceSet::SPtr& getSurfSet();

    //=========================================================================================================
    /**
     * Sets the mri head transformation.
     *
     * @param[in] mriHeadTrans   The transformation to set.
     */
    inline void setMriHeadTrans(FIFFLIB::FiffCoordTrans& mriHeadTrans);

    //=========================================================================================================
    /**
     * Returns the mri head transformation.
     *
     * @return the mri head transformation.
     */
    inline FIFFLIB::FiffCoordTrans& getMriHeadTrans();

    //=========================================================================================================
    /**
     * Sets the forward solution.
     *
     * @param[in] fwdSolution   the forward solution to set.
     */
    inline void setFwdSolution(MNELIB::MNEForwardSolution::SPtr& fwdSolution);

    //=========================================================================================================
    /**
     * Returns the forward solution.
     *
     * @return the forward solution.
     */
    inline MNELIB::MNEForwardSolution::SPtr& getFwdSolution();

    //=========================================================================================================
    /**
     * Attaches a value to the sample array vector.
     * This method is inherited by Measurement.
     *
     * @param[in] v the value which is attached to the sample array vector.
     */
    virtual void setValue(MNELIB::MNESourceEstimate &v);

    //=========================================================================================================
    /**
     * Returns the current value set.
     * This method is inherited by Measurement.
     *
     * @return the last attached value.
     */
    virtual QList<MNELIB::MNESourceEstimate::SPtr>& getValue();

    //=========================================================================================================
    /**
     * Returns whether RealTimeSourceEstimate contains values
     *
     * @return whether RealTimeSourceEstimate contains values.
     */
    inline bool isInitialized() const;

    //=========================================================================================================
    /**
     * Sets the current FiffInfo.
     *
     * @param[in] p_fiffInfo the new FiffInfo..
     */
    void setFiffInfo(FIFFLIB::FiffInfo::SPtr p_fiffInfo);

    //=========================================================================================================
    /**
     * Returns the current FiffInfo.
     *
     * @return the current FiffInfo.
     */
    FIFFLIB::FiffInfo::SPtr getFiffInfo();

    //=========================================================================================================
    /**
     * Sets the number of sample vectors which should be gathered before attached observers are notified by calling the Subject notify() method.
     *
     * @param[in] iSourceEstimateSize the number of values.
     */
    inline void setSourceEstimateSize(qint32 iSourceEstimateSize);

    //=========================================================================================================
    /**
     * Returns the number of values which should be gathered before attached observers are notified by calling the Subject notify() method.
     *
     * @return the number of values which are gathered before a notify() is called.
     */
    inline qint32 getSourceEstimateSize() const;

private:
    mutable QMutex                          m_qMutex;               /**< Mutex to ensure thread safety. */

    FIFFLIB::FiffInfo::SPtr                 m_pFiffInfo;            /**< The Fiff info. */
    FIFFLIB::FiffCoordTrans                 m_mriHeadTrans;         /**< Mri to head transformation. */

    FSLIB::AnnotationSet::SPtr              m_pAnnotSet;            /**< Annotation set. */
    FSLIB::SurfaceSet::SPtr                 m_pSurfSet;             /**< Surface set. */
    MNELIB::MNEForwardSolution::SPtr        m_pFwdSolution;         /**< Forward solution. */

    qint32                                  m_iSourceEstimateSize;  /**< Sample size of the multi sample array.*/

    QList<MNELIB::MNESourceEstimate::SPtr>  m_pMNEStc;              /**< The source estimates. */
    bool                                    m_bInitialized;         /**< Is initialized. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void RealTimeSourceEstimate::setAnnotSet(FSLIB::AnnotationSet::SPtr& annotSet)
{
    QMutexLocker locker(&m_qMutex);
    m_pAnnotSet = annotSet;
}

//=============================================================================================================

inline FSLIB::AnnotationSet::SPtr& RealTimeSourceEstimate::getAnnotSet()
{
    QMutexLocker locker(&m_qMutex);
    return m_pAnnotSet;
}

//=============================================================================================================

inline void RealTimeSourceEstimate::setSurfSet(FSLIB::SurfaceSet::SPtr& surfSet)
{
    QMutexLocker locker(&m_qMutex);
    m_pSurfSet = surfSet;
}

//=============================================================================================================

inline FSLIB::SurfaceSet::SPtr& RealTimeSourceEstimate::getSurfSet()
{
    QMutexLocker locker(&m_qMutex);
    return m_pSurfSet;
}

//=============================================================================================================

inline void RealTimeSourceEstimate::setMriHeadTrans(FIFFLIB::FiffCoordTrans& mriHeadTrans)
{
    QMutexLocker locker(&m_qMutex);
    m_mriHeadTrans = mriHeadTrans;
}

//=============================================================================================================

inline FIFFLIB::FiffCoordTrans& RealTimeSourceEstimate::getMriHeadTrans()
{
    QMutexLocker locker(&m_qMutex);
    return m_mriHeadTrans;
}
//=============================================================================================================

inline void RealTimeSourceEstimate::setFwdSolution(MNELIB::MNEForwardSolution::SPtr& fwdSolution)
{
    QMutexLocker locker(&m_qMutex);
    m_pFwdSolution = fwdSolution;
}

//=============================================================================================================

inline MNELIB::MNEForwardSolution::SPtr& RealTimeSourceEstimate::getFwdSolution()
{
    QMutexLocker locker(&m_qMutex);
    return m_pFwdSolution;
}

//=============================================================================================================

inline bool RealTimeSourceEstimate::isInitialized() const
{
    QMutexLocker locker(&m_qMutex);
    return m_bInitialized;
}

//=============================================================================================================

inline void RealTimeSourceEstimate::setFiffInfo(FIFFLIB::FiffInfo::SPtr p_fiffInfo)
{
    QMutexLocker locker(&m_qMutex);
    m_pFiffInfo = p_fiffInfo;
}

//=============================================================================================================

inline FIFFLIB::FiffInfo::SPtr RealTimeSourceEstimate::getFiffInfo()
{
    QMutexLocker locker(&m_qMutex);
    return m_pFiffInfo;
}

//=============================================================================================================

inline void RealTimeSourceEstimate::setSourceEstimateSize(qint32 iSourceEstimateSize)
{
    QMutexLocker locker(&m_qMutex);
    //Obsolete unsigned char can't be bigger
//    if(ucArraySize > 255)
//        m_ucArraySize = 255;
//    else
        m_iSourceEstimateSize = iSourceEstimateSize;
}

//=============================================================================================================

qint32 RealTimeSourceEstimate::getSourceEstimateSize() const
{
    QMutexLocker locker(&m_qMutex);
    return m_iSourceEstimateSize;
}
} // NAMESPACE

Q_DECLARE_METATYPE(SCMEASLIB::RealTimeSourceEstimate::SPtr)

#endif // REALTIMESOURCEESTIMATE_H
