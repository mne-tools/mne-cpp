//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     realtimemultisamplearray.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the declaration of the RealTimeMultiSampleArray class.
 */

#ifndef REALTIMEMULTISAMPLEARRAY_H
#define REALTIMEMULTISAMPLEARRAY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scmeas_global.h"
#include "measurement.h"
#include "realtimesamplearraychinfo.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>
#include <QList>
#include <QMutex>
#include <QMutexLocker>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB{
    class FiffInfo;
    class FiffDigitizerData;
}

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
    void initFromFiffInfo(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

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
    inline QSharedPointer<FIFFLIB::FiffInfo> info();

    //=========================================================================================================
    /**
     * Returns digitizer data for measurement
     *
     * @return the current set digitizer data
     */
    inline QSharedPointer<FIFFLIB::FiffDigitizerData> digitizerData();

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
    void setDigitizerData(QSharedPointer<FIFFLIB::FiffDigitizerData> digData);

private:
    mutable QMutex              m_qMutex;           /**< Mutex to ensure thread safety. */

    QSharedPointer<FIFFLIB::FiffInfo>               m_pFiffInfo_orig;           /**< Original Fiff Info if initialized by fiff info. */
    QSharedPointer<FIFFLIB::FiffDigitizerData>      m_pFiffDigitizerData_orig;  /**< Original Fiff Digitizer Data */

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

inline QSharedPointer<FIFFLIB::FiffInfo> RealTimeMultiSampleArray::info()
{
    QMutexLocker locker(&m_qMutex);
    return m_pFiffInfo_orig;
}

inline QSharedPointer<FIFFLIB::FiffDigitizerData> RealTimeMultiSampleArray::digitizerData()
{
    QMutexLocker locker(&m_qMutex);
    return m_pFiffDigitizerData_orig;
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
