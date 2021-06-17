//=============================================================================================================
/**
 * @file     realtimemultisamplearray.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Contains the declaration of the RealTimeMultiSampleArray class.
 *
 */

#ifndef REALTIMEMULTISAMPLEARRAY_H
#define REALTIMEMULTISAMPLEARRAY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scmeas_global.h"
#include "measurement.h"
#include "realtimesamplearraychinfo.h"

#include <fiff/fiff_info.h>
#include <fiff/c/fiff_digitizer_data.h>

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
 * DECLARE CLASS RealTimeMultiSampleArray -> ToDo check feasibilty of QAbstractTableModel
 *
 * @brief The RealTimeMultiSampleArray class is the base class of every RealTimeMultiSampleArray Measurement.
 */
class SCMEASSHARED_EXPORT RealTimeMultiSampleArray : public Measurement
{
    Q_OBJECT

public:
    typedef QSharedPointer<RealTimeMultiSampleArray> SPtr;               /**< Shared pointer type for RealTimeMultiSampleArray. */
    typedef QSharedPointer<const RealTimeMultiSampleArray> ConstSPtr;    /**< Const shared pointer type for RealTimeMultiSampleArray. */

    //=========================================================================================================
    /**
     * Constructs a RealTimeMultiSampleArray.
     */
    explicit RealTimeMultiSampleArray(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RealTimeMultiSampleArray.
     */
    virtual ~RealTimeMultiSampleArray();

    //=========================================================================================================
    /**
     * Clears all the data stored in the buffer.
     */
    void clear();

    //=========================================================================================================
    /**
     * Inits RealTimeMultiSampleArray and adds uiNumChannels empty channel information
     *
     * @param[in] uiNumChannels     the number of channels to init.
     */
    void init(QList<RealTimeSampleArrayChInfo> &chInfo);

    //=========================================================================================================
    /**
     * Init channel infos using fiff info
     *
     * @param[in] pFiffInfo     Info to init from.
     */
    void initFromFiffInfo(FIFFLIB::FiffInfo::SPtr pFiffInfo);

    //=========================================================================================================
    /**
     * Returns whether channel info is initialized
     *
     * @return true whether the channel info is available.
     */
    inline bool isChInit() const;

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
     * Sets the sampling rate of the RealTimeMultiSampleArray Measurement.
     *
     * @param[in] fSamplingRate the sampling rate of the RealTimeMultiSampleArray.
     */
    inline void setSamplingRate(float fSamplingRate);

    //=========================================================================================================
    /**
     * Returns the sampling rate of the RealTimeMultiSampleArray Measurement.
     *
     * @return the sampling rate of the RealTimeMultiSampleArray.
     */
    inline float getSamplingRate() const;

    //=========================================================================================================
    /**
     * Returns the number of channels.
     *
     * @return the number of values which are gathered before a notify() is called.
     */
    inline unsigned int getNumChannels() const;

    //=========================================================================================================
    /**
     * Returns the reference to the channel list.
     *
     * @return the reference to the channel list.
     */
    inline QList<RealTimeSampleArrayChInfo>& chInfo();

    //=========================================================================================================
    /**
     * Returns the reference to the orig FiffInfo.
     *
     * @return the reference to the orig FiffInfo.
     */
    inline FIFFLIB::FiffInfo::SPtr info();

    //=========================================================================================================
    /**
     * Sets the number of sample vectors which should be gathered before attached observers are notified by calling the Subject notify() method.
     *
     * @param[in] iMultiArraySize the number of values.
     */
    inline void setMultiArraySize(qint32 iMultiArraySize);

    //=========================================================================================================
    /**
     * Returns the number of values which should be gathered before attached observers are notified by calling the Subject notify() method.
     *
     * @return the number of values which are gathered before a notify() is called.
     */
    inline qint32 getMultiArraySize() const;

    //=========================================================================================================
    /**
     * Returns the gathered multi sample array.
     *
     * @return the current multi sample array.
     */
    inline const QList<Eigen::MatrixXd>& getMultiSampleArray();

    //=========================================================================================================
    /**
     * Attaches a value to the sample array list.
     *
     * @param[in] mat   the value which is attached to the sample array list.
     */
    virtual void setValue(const Eigen::MatrixXd& mat);

    //=========================================================================================================
    /**
     * Sets digitizer data for measurement
     *
     * @param[in] digData   digitizer data from measurment
     */
    void setDigitizerData(FIFFLIB::FiffDigitizerData::SPtr digData);

private:
    mutable QMutex              m_qMutex;           /**< Mutex to ensure thread safety. */

    FIFFLIB::FiffInfo::SPtr     m_pFiffInfo_orig;   /**< Original Fiff Info if initialized by fiff info. */
    FIFFLIB::FiffDigitizerData::SPtr m_pFiffDigitizerData_orig;

    QString                     m_sXMLLayoutFile;   /**< Layout file name. */
    float                       m_fSamplingRate;    /**< Sampling rate of the RealTimeSampleArray.*/
    qint32                      m_iMultiArraySize;  /**< Sample size of the multi sample array.*/
    QList<Eigen::MatrixXd>      m_matSamples;       /**< The multi sample array.*/
    bool                        m_bChInfoIsInit;    /**< If channel info is initialized.*/

    QList<RealTimeSampleArrayChInfo> m_qListChInfo; /**< Channel info list.*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void RealTimeMultiSampleArray::clear()
{
    QMutexLocker locker(&m_qMutex);
    m_matSamples.clear();
}

//=============================================================================================================

inline bool RealTimeMultiSampleArray::isChInit() const
{
    QMutexLocker locker(&m_qMutex);
    return m_bChInfoIsInit;
}

//=============================================================================================================

inline const QString& RealTimeMultiSampleArray::getXMLLayoutFile() const
{
    QMutexLocker locker(&m_qMutex);
    return m_sXMLLayoutFile;
}

//=============================================================================================================

inline void RealTimeMultiSampleArray::setXMLLayoutFile(const QString& layout)
{
    QMutexLocker locker(&m_qMutex);
    m_sXMLLayoutFile = layout;
}

//=============================================================================================================

inline void RealTimeMultiSampleArray::setSamplingRate(float fSamplingRate)
{
    QMutexLocker locker(&m_qMutex);
    m_fSamplingRate = fSamplingRate;
}

//=============================================================================================================

inline float RealTimeMultiSampleArray::getSamplingRate() const
{
    QMutexLocker locker(&m_qMutex);
    return m_fSamplingRate;
}

//=============================================================================================================

inline unsigned int RealTimeMultiSampleArray::getNumChannels() const
{
    QMutexLocker locker(&m_qMutex);
    return m_qListChInfo.size();
}

//=============================================================================================================

inline QList<RealTimeSampleArrayChInfo>& RealTimeMultiSampleArray::chInfo()
{
    QMutexLocker locker(&m_qMutex);
    return m_qListChInfo;
}

//=============================================================================================================

inline FIFFLIB::FiffInfo::SPtr RealTimeMultiSampleArray::info()
{
    QMutexLocker locker(&m_qMutex);
    return m_pFiffInfo_orig;
}

//=============================================================================================================

inline void RealTimeMultiSampleArray::setMultiArraySize(qint32 iMultiArraySize)
{
    QMutexLocker locker(&m_qMutex);
    //Obsolete unsigned char can't be bigger
//    if(ucArraySize > 255)
//        m_ucArraySize = 255;
//    else
        m_iMultiArraySize = iMultiArraySize;
}

//=============================================================================================================

qint32 RealTimeMultiSampleArray::getMultiArraySize() const
{
    QMutexLocker locker(&m_qMutex);
    return m_iMultiArraySize;
}

//=============================================================================================================

inline const QList<Eigen::MatrixXd>& RealTimeMultiSampleArray::getMultiSampleArray()
{
    return m_matSamples;
}
} // NAMESPACE

Q_DECLARE_METATYPE(SCMEASLIB::RealTimeMultiSampleArray::SPtr)

#endif // REALTIMEMULTISAMPLEARRAY_H
