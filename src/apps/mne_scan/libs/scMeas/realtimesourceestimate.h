//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     realtimesourceestimate.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the declaration of the RealTimeSourceEstimate class.
 */

#ifndef REALTIMESOURCEESTIMATE_H
#define REALTIMESOURCEESTIMATE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scmeas_global.h"
#include "measurement.h"
#include <mne/fs/fs_label.h>
#include <mne/fs/fs_surfaceset.h>
#include <mne/fs/fs_annotationset.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_coord_trans.h>
#include <mne/mne/mne_source_spaces.h>
#include <mne/inv/inv_source_estimate.h>
#include <mne/mne/mne_forward_solution.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>
#include <QList>
#include <QMutex>
#include <QMutexLocker>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE mne::scan
//=============================================================================================================

namespace mne::scan
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
    typedef QSharedPointer<RealTimeSourceEstimate> SPtr;            /**< Shared pointer type for RealTimeSourceEstimate. */
    typedef QSharedPointer<const RealTimeSourceEstimate> ConstSPtr; /**< Const shared pointer type for RealTimeSourceEstimate. */

    //=========================================================================================================
    /**
     * Constructs a RealTimeSourceEstimate.
     *
     * @param[in] parent     the QObject parent of this measurement.
     */
    RealTimeSourceEstimate(QObject* parent = 0);

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
    inline void setAnnotSet(std::shared_ptr<mne::FsAnnotationSet>& annotSet);

    //=========================================================================================================
    /**
     * Returns the annotation set.
     *
     * @return the annotation set.
     */
    inline std::shared_ptr<mne::FsAnnotationSet>& getAnnotSet();

    //=========================================================================================================
    /**
     * Sets the surface set.
     *
     * @param[in] surfSet   the surface set to set.
     */
    inline void setSurfSet(std::shared_ptr<mne::FsSurfaceSet>& surfSet);

    //=========================================================================================================
    /**
     * Returns the surface set.
     *
     * @return the surface set.
     */
    inline std::shared_ptr<mne::FsSurfaceSet>& getSurfSet();

    //=========================================================================================================
    /**
     * Sets the mri head transformation.
     *
     * @param[in] mriHeadTrans   The transformation to set.
     */
    inline void setMriHeadTrans(mne::FiffCoordTrans& mriHeadTrans);

    //=========================================================================================================
    /**
     * Returns the mri head transformation.
     *
     * @return the mri head transformation.
     */
    inline mne::FiffCoordTrans& getMriHeadTrans();

    //=========================================================================================================
    /**
     * Sets the forward solution.
     *
     * @param[in] fwdSolution   the forward solution to set.
     */
    inline void setFwdSolution(const std::shared_ptr<mne::MNEForwardSolution>& fwdSolution);

    //=========================================================================================================
    /**
     * Returns the forward solution.
     *
     * @return the forward solution.
     */
    inline std::shared_ptr<mne::MNEForwardSolution>& getFwdSolution();

    //=========================================================================================================
    /**
     * Attaches a value to the sample array vector.
     * This method is inherited by Measurement.
     *
     * @param[in] v the value which is attached to the sample array vector.
     */
    virtual void setValue(mne::InvSourceEstimate& v);

    //=========================================================================================================
    /**
     * Returns the current value set.
     * This method is inherited by Measurement.
     *
     * @return the last attached value.
     */
    virtual QList<std::shared_ptr<mne::InvSourceEstimate>>& getValue();

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
    void setFiffInfo(std::shared_ptr<mne::FiffInfo> p_fiffInfo);

    //=========================================================================================================
    /**
     * Returns the current FiffInfo.
     *
     * @return the current FiffInfo.
     */
    std::shared_ptr<mne::FiffInfo> getFiffInfo();

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
    mutable QMutex m_qMutex; /**< Mutex to ensure thread safety. */

    std::shared_ptr<mne::FiffInfo> m_pFiffInfo; /**< The Fiff info. */
    mne::FiffCoordTrans m_mriHeadTrans;         /**< Mri to head transformation. */

    std::shared_ptr<mne::FsAnnotationSet> m_pAnnotSet;       /**< FsAnnotation set. */
    std::shared_ptr<mne::FsSurfaceSet> m_pSurfSet;           /**< FsSurface set. */
    std::shared_ptr<mne::MNEForwardSolution> m_pFwdSolution; /**< Forward solution. */

    qint32 m_iSourceEstimateSize; /**< Sample size of the multi sample array.*/

    QList<std::shared_ptr<mne::InvSourceEstimate>> m_pMNEStc; /**< The source estimates. */
    bool m_bInitialized;                                      /**< Is initialized. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void RealTimeSourceEstimate::setAnnotSet(std::shared_ptr<mne::FsAnnotationSet>& annotSet)
{
    QMutexLocker locker(&m_qMutex);
    m_pAnnotSet = annotSet;
}

//=============================================================================================================

inline std::shared_ptr<mne::FsAnnotationSet>& RealTimeSourceEstimate::getAnnotSet()
{
    QMutexLocker locker(&m_qMutex);
    return m_pAnnotSet;
}

//=============================================================================================================

inline void RealTimeSourceEstimate::setSurfSet(std::shared_ptr<mne::FsSurfaceSet>& surfSet)
{
    QMutexLocker locker(&m_qMutex);
    m_pSurfSet = surfSet;
}

//=============================================================================================================

inline std::shared_ptr<mne::FsSurfaceSet>& RealTimeSourceEstimate::getSurfSet()
{
    QMutexLocker locker(&m_qMutex);
    return m_pSurfSet;
}

//=============================================================================================================

inline void RealTimeSourceEstimate::setMriHeadTrans(mne::FiffCoordTrans& mriHeadTrans)
{
    QMutexLocker locker(&m_qMutex);
    m_mriHeadTrans = mriHeadTrans;
}

//=============================================================================================================

inline mne::FiffCoordTrans& RealTimeSourceEstimate::getMriHeadTrans()
{
    QMutexLocker locker(&m_qMutex);
    return m_mriHeadTrans;
}
//=============================================================================================================

inline void RealTimeSourceEstimate::setFwdSolution(const std::shared_ptr<mne::MNEForwardSolution>& fwdSolution)
{
    QMutexLocker locker(&m_qMutex);
    m_pFwdSolution = fwdSolution;
}

//=============================================================================================================

inline std::shared_ptr<mne::MNEForwardSolution>& RealTimeSourceEstimate::getFwdSolution()
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

inline void RealTimeSourceEstimate::setFiffInfo(std::shared_ptr<mne::FiffInfo> p_fiffInfo)
{
    QMutexLocker locker(&m_qMutex);
    m_pFiffInfo = std::move(p_fiffInfo);
}

//=============================================================================================================

inline std::shared_ptr<mne::FiffInfo> RealTimeSourceEstimate::getFiffInfo()
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

#endif // REALTIMESOURCEESTIMATE_H
