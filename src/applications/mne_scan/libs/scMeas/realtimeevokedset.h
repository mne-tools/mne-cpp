//=============================================================================================================
/**
 * @file     realtimeevokedset.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2016
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
 * @brief    Contains the declaration of the RealTimeEvokedSet class.
 *
 */

#ifndef REALTIMEEVOKEDSET_H
#define REALTIMEEVOKEDSET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scmeas_global.h"
#include "measurement.h"
#include "realtimesamplearraychinfo.h"

#include <fiff/fiff_evoked_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMutex>
#include <QMutexLocker>
#include <QSharedPointer>
#include <QVector>
#include <QList>
#include <QColor>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffEvokedSet;
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE SCMEASLIB
//=============================================================================================================

namespace SCMEASLIB
{

//=============================================================================================================
// SCMEASLIB FORWARD DECLARATIONS
//=============================================================================================================

//=========================================================================================================
/**
 * DECLARE CLASS RealTimeEvokedSet
 *
 * @brief The RealTimeEvokedSet class provides a data stream which holds FiffEvokedSet data.
 */
class SCMEASSHARED_EXPORT RealTimeEvokedSet : public Measurement
{
    Q_OBJECT

public:
    typedef QSharedPointer<RealTimeEvokedSet> SPtr;               /**< Shared pointer type for RealTimeEvokedSet. */
    typedef QSharedPointer<const RealTimeEvokedSet> ConstSPtr;    /**< Const shared pointer type for RealTimeEvokedSet. */

    //=========================================================================================================
    /**
     * Constructs a RealTimeEvokedSet.
     */
    explicit RealTimeEvokedSet(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RealTimeEvokedSet.
     */
    virtual ~RealTimeEvokedSet();

    //=========================================================================================================
    /**
     * Returns the file name of the xml layout file.
     *
     * @return the file name of the layout file.
     */
    inline const QString& getXMLLayoutFile() const;

    //=========================================================================================================
    /**
     * Sets the file name of the xml layout.
     *
     * @param[in] layout which should be set.
     */
    inline void setXMLLayoutFile(const QString& layout);

    //=========================================================================================================
    /**
     * Returns the number of channels.
     *
     * @return the number of values which are gathered before a notify() is called.
     */
    inline unsigned int getNumChannels() const;

    //=========================================================================================================
    /**
     * Returns the number of pre-stimulus samples
     *
     * @return the number of pre-stimulus samples.
     */
    inline qint32 getNumPreStimSamples() const;

    //=========================================================================================================
    /**
     * Set the number of pre-stimulus samples
     *
     * @param[in] samples the number of pre-stimulus samples.
     */
    inline void setNumPreStimSamples(qint32 samples);

    //=========================================================================================================
    /**
     * Returns the number of channels.
     *
     * @return the number of values which are gathered before a notify() is called.
     */
    inline QList<QColor>& chColor();

    //=========================================================================================================
    /**
     * Returns the reference to the channel list.
     *
     * @return the reference to the channel list.
     */
    inline QList<RealTimeSampleArrayChInfo>& chInfo();

    //=========================================================================================================
    /**
     * Returns the reference to the current info.
     *
     * @return the reference to the current info.
     */
    inline QSharedPointer<FIFFLIB::FiffInfo> info();

    //=========================================================================================================
    /**
     * New devoked to distribute
     *
     * @param[in] v                         the evoked set which should be distributed.
     * @param[in] p_fiffinfo                the evoked fiff info as shared pointer.
     * @param[in] lResponsibleTriggerTypes  List of all trigger types which lead to the recent emit of a new evoked set.
     */
    virtual void setValue(const FIFFLIB::FiffEvokedSet &v,
                          const QSharedPointer<FIFFLIB::FiffInfo>& p_fiffinfo,
                          const QStringList& lResponsibleTriggerTypes);

    //=========================================================================================================
    /**
     * Returns the current value set.
     * This method is inherited by Measurement.
     *
     * @return the last attached value.
     */
    virtual QSharedPointer<FIFFLIB::FiffEvokedSet>& getValue();

    //=========================================================================================================
    /**
     * Returns the trigger types which lead to the emit of this evoked set.
     *
     * @return the trigger types which lead to the emit of this evoked set.
     */
    const QStringList& getResponsibleTriggerTypes();

    //=========================================================================================================
    /**
     * Returns whether RealTimeEvokedSet contains values
     *
     * @return whether RealTimeEvokedSet contains values.
     */
    inline bool isInitialized() const;

    //=========================================================================================================
    /**
     * Set baseline information
     *
     * @param[in] info             the min max information of the baseline.
     */
    inline void setBaselineInfo(QPair<qint32,qint32> info);

    //=========================================================================================================
    /**
     * Get baseline information
     *
     * @return the min max information of the baseline as a QPair.
     */
    inline QPair<qint32,qint32> getBaselineInfo();

private:
    //=========================================================================================================
    /**
     * Init channel infos using fiff info
     *
     * @param[in] p_fiffInfo     Info to init from.
     */
    void init(QSharedPointer<FIFFLIB::FiffInfo> p_fiffInfo);

    mutable QMutex                          m_qMutex;           /**< Mutex to ensure thread safety. */

    QSharedPointer<FIFFLIB::FiffEvokedSet>  m_pFiffEvokedSet;   /**< Evoked data set*/

    QStringList                             m_lResponsibleTriggerTypes; /**< List of all trigger types which lead to the recent emit of a new evoked set. */

    QSharedPointer<FIFFLIB::FiffInfo>       m_pFiffInfo;        /**< Fiff info. */

    QString                                 m_sXMLLayoutFile;   /**< Layout file name. */

    qint32                                  m_iPreStimSamples;  /**< Number of pre-stimulus samples. */

    QList<QColor>                           m_qListChColors;    /**< Channel color for butterfly plot.*/

    QList<RealTimeSampleArrayChInfo>        m_qListChInfo;      /**< Channel info list.*/

    bool                                    m_bInitialized;     /**< If values are stored.*/

    QPair<qint32,qint32>                    m_pairBaseline;     /**< Baseline information min max.*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const QString& RealTimeEvokedSet::getXMLLayoutFile() const
{
    QMutexLocker locker(&m_qMutex);
    return m_sXMLLayoutFile;
}

//=============================================================================================================

inline void RealTimeEvokedSet::setXMLLayoutFile(const QString& layout)
{
    QMutexLocker locker(&m_qMutex);
    m_sXMLLayoutFile = layout;
}

//=============================================================================================================

inline unsigned int RealTimeEvokedSet::getNumChannels() const
{
    QMutexLocker locker(&m_qMutex);
    return m_pFiffEvokedSet->info.nchan;
}

//=============================================================================================================

inline qint32 RealTimeEvokedSet::getNumPreStimSamples() const
{
    QMutexLocker locker(&m_qMutex);
    return m_iPreStimSamples;
}

//=============================================================================================================

inline void RealTimeEvokedSet::setNumPreStimSamples(qint32 samples)
{
    QMutexLocker locker(&m_qMutex);
    m_iPreStimSamples = samples;
}

//=============================================================================================================

inline QList<QColor>& RealTimeEvokedSet::chColor()
{
    QMutexLocker locker(&m_qMutex);
    return m_qListChColors;
}

//=============================================================================================================

inline QList<RealTimeSampleArrayChInfo>& RealTimeEvokedSet::chInfo()
{
    QMutexLocker locker(&m_qMutex);
    return m_qListChInfo;
}

//=============================================================================================================

inline QSharedPointer<FIFFLIB::FiffInfo> RealTimeEvokedSet::info()
{
    QMutexLocker locker(&m_qMutex);
    return m_pFiffInfo;
}

//=============================================================================================================

inline bool RealTimeEvokedSet::isInitialized() const
{
    QMutexLocker locker(&m_qMutex);
    return m_bInitialized;
}

//=============================================================================================================

inline void RealTimeEvokedSet::setBaselineInfo(QPair<qint32,qint32> info)
{
    m_pairBaseline = info;
}

//=============================================================================================================

inline QPair<qint32,qint32> RealTimeEvokedSet::getBaselineInfo()
{
    return m_pairBaseline;
}
} // NAMESPACE

Q_DECLARE_METATYPE(SCMEASLIB::RealTimeEvokedSet::SPtr)

#endif // RealTimeEvokedSet_H
