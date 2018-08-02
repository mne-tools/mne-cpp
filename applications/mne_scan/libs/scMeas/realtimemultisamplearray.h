//=============================================================================================================
/**
* @file     realtimemultisamplearray.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scmeas_global.h"
#include "measurement.h"
#include "realtimesamplearraychinfo.h"

#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>
#include <QList>
#include <QMutex>
#include <QMutexLocker>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE SCMEASLIB
//=============================================================================================================

namespace SCMEASLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//=========================================================================================================
/**
* DECLARE CLASS RealTimeMultiSampleArray -> ToDo check feasibilty of QAbstractTableModel
*
* @brief The RealTimeMultiSampleArrayNew class is the base class of every RealTimeMultiSampleArrayNew Measurement.
*/
class SCMEASSHARED_EXPORT RealTimeMultiSampleArray : public Measurement
{
    Q_OBJECT
public:
    typedef QSharedPointer<RealTimeMultiSampleArray> SPtr;               /**< Shared pointer type for RealTimeMultiSampleArray. */
    typedef QSharedPointer<const RealTimeMultiSampleArray> ConstSPtr;    /**< Const shared pointer type for RealTimeMultiSampleArray. */

    //=========================================================================================================
    /**
    * Constructs a RealTimeMultiSampleArrayNew.
    */
    explicit RealTimeMultiSampleArray(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the RealTimeMultiSampleArrayNew.
    */
    virtual ~RealTimeMultiSampleArray();

    //=========================================================================================================
    /**
    * Clears all the data stored in the buffer.
    */
    void clear();

    //=========================================================================================================
    /**
    * Inits RealTimeMultiSampleArrayNew and adds uiNumChannels empty channel information
    *
    * @param [in] uiNumChannels     the number of channels to init.
    */
    void init(QList<RealTimeSampleArrayChInfo> &chInfo);

    //=========================================================================================================
    /**
    * Init channel infos using fiff info
    *
    * @param[in] p_pFiffInfo     Info to init from
    */
    void initFromFiffInfo(FiffInfo::SPtr &p_pFiffInfo);

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
    * Sets the flags used in the QuickControl widget.
    *
    * @param[in] layout which should be set.
    */
    inline void setDisplayFlags(const QStringList& slFlags);

    //=========================================================================================================
    /**
    * Sets the flags used in the QuickControl widget.
    *
    * @param[in] layout which should be set.
    */
    inline const QStringList& getDisplayFlags();

    //=========================================================================================================
    /**
    * Sets the sampling rate of the RealTimeMultiSampleArrayNew Measurement.
    *
    * @param[in] dSamplingRate the sampling rate of the RealTimeMultiSampleArrayNew.
    */
    inline void setSamplingRate(double dSamplingRate);

    //=========================================================================================================
    /**
    * Returns the sampling rate of the RealTimeMultiSampleArrayNew Measurement.
    *
    * @return the sampling rate of the RealTimeMultiSampleArrayNew.
    */
    inline double getSamplingRate() const;

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
    inline FiffInfo::SPtr& info();

    //=========================================================================================================
    /**
    * Sets the number of sample vectors which should be gathered before attached observers are notified by calling the Subject notify() method.
    *
    * @param [in] iMultiArraySize the number of values.
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
    inline const QList< MatrixXd >& getMultiSampleArray();

    //=========================================================================================================
    /**
    * Attaches a value to the sample array list.
    *
    * @param [in] mat   the value which is attached to the sample array list.
    */
    virtual void setValue(const MatrixXd& mat);

private:
    mutable QMutex              m_qMutex;           /**< Mutex to ensure thread safety */

    FiffInfo::SPtr              m_pFiffInfo_orig;   /**< Original Fiff Info if initialized by fiff info. */

    QStringList                 m_slDisplayFlag;    /**< The flags to use in the displays quick control widget. Possible flags are: projections, compensators, view,filter, triggerdetection, modalities, scaling, sphara. */
    QString                     m_sXMLLayoutFile;   /**< Layout file name. */
    double                      m_dSamplingRate;    /**< Sampling rate of the RealTimeSampleArray.*/
    qint32                      m_iMultiArraySize;  /**< Sample size of the multi sample array.*/
    QList<MatrixXd>             m_matSamples;       /**< The multi sample array.*/
    bool                        m_bChInfoIsInit;    /**< If channel info is initialized.*/

    QList<RealTimeSampleArrayChInfo> m_qListChInfo; /**< Channel info list.*/
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void RealTimeMultiSampleArray::clear()
{
    QMutexLocker locker(&m_qMutex);
    m_matSamples.clear();
}


//*************************************************************************************************************

inline bool RealTimeMultiSampleArray::isChInit() const
{
    QMutexLocker locker(&m_qMutex);
    return m_bChInfoIsInit;
}


//*************************************************************************************************************

inline const QString& RealTimeMultiSampleArray::getXMLLayoutFile() const
{
    QMutexLocker locker(&m_qMutex);
    return m_sXMLLayoutFile;
}


//*************************************************************************************************************

inline void RealTimeMultiSampleArray::setXMLLayoutFile(const QString& layout)
{
    QMutexLocker locker(&m_qMutex);
    m_sXMLLayoutFile = layout;
}


//*************************************************************************************************************

inline void RealTimeMultiSampleArray::setDisplayFlags(const QStringList& slFlags)
{
    QMutexLocker locker(&m_qMutex);
    m_slDisplayFlag = slFlags;
}


//*************************************************************************************************************

inline const QStringList& RealTimeMultiSampleArray::getDisplayFlags()
{
    QMutexLocker locker(&m_qMutex);
    return m_slDisplayFlag;
}


//*************************************************************************************************************

inline void RealTimeMultiSampleArray::setSamplingRate(double dSamplingRate)
{
    QMutexLocker locker(&m_qMutex);
    m_dSamplingRate = dSamplingRate;
}


//*************************************************************************************************************

inline double RealTimeMultiSampleArray::getSamplingRate() const
{
    QMutexLocker locker(&m_qMutex);
    return m_dSamplingRate;
}


//*************************************************************************************************************

inline unsigned int RealTimeMultiSampleArray::getNumChannels() const
{
    QMutexLocker locker(&m_qMutex);
    return m_qListChInfo.size();
}


//*************************************************************************************************************

inline QList<RealTimeSampleArrayChInfo>& RealTimeMultiSampleArray::chInfo()
{
    QMutexLocker locker(&m_qMutex);
    return m_qListChInfo;
}


//*************************************************************************************************************

inline FiffInfo::SPtr& RealTimeMultiSampleArray::info()
{
    QMutexLocker locker(&m_qMutex);
    return m_pFiffInfo_orig;
}


//*************************************************************************************************************

inline void RealTimeMultiSampleArray::setMultiArraySize(qint32 iMultiArraySize)
{
    QMutexLocker locker(&m_qMutex);
    //Obsolete unsigned char can't be bigger
//    if(ucArraySize > 255)
//        m_ucArraySize = 255;
//    else
        m_iMultiArraySize = iMultiArraySize;
}


//*************************************************************************************************************

qint32 RealTimeMultiSampleArray::getMultiArraySize() const
{
    QMutexLocker locker(&m_qMutex);
    return m_iMultiArraySize;
}


//*************************************************************************************************************

inline const QList< MatrixXd >& RealTimeMultiSampleArray::getMultiSampleArray()
{
    return m_matSamples;
}

} // NAMESPACE

Q_DECLARE_METATYPE(SCMEASLIB::RealTimeMultiSampleArray::SPtr)

#endif // REALTIMEMULTISAMPLEARRAYNEW_H
