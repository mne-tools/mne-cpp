//=============================================================================================================
/**
 * @file     realtimeneurofeedbackresult.h
 * @author   Simon Marxgut <simon.marxgut@umit-tirol.at>
 * @since    0.1.0
 * @date     November, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Simon Marxgut. All rights reserved.
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
 * @brief    Contains the declaration of the RealTimeNeurofeedbackResult class.
 *
 */

#ifndef REALTIMENEUROFEEDBACKRESULT_H
#define REALTIMENEUROFEEDBACKRESULT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scmeas_global.h"
#include "measurement.h"
#include "realtimesamplearraychinfo.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>
#include <QPixmap>
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
 * DECLARE CLASS RealTimeNeurofeedbackResult
 */
class SCMEASSHARED_EXPORT RealTimeNeurofeedbackResult : public Measurement
{
    Q_OBJECT

public:
    typedef QSharedPointer<RealTimeNeurofeedbackResult> SPtr;               /**< Shared pointer type for RealTimeNeurofeedbackResult. */
    typedef QSharedPointer<const RealTimeNeurofeedbackResult> ConstSPtr;    /**< Const shared pointer type for RealTimeNeurofeedbackResult. */

    //=========================================================================================================
    /**
     * Constructs a RealTimeNeurofeedbackResult.
     */
    explicit RealTimeNeurofeedbackResult(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RealTimeNeurofeedbackResult.
     */
    virtual ~RealTimeNeurofeedbackResult();

    //=========================================================================================================
    /**
     * Clears all the data stored in the buffer.
     */
    void clear();

    //=========================================================================================================
    /**
     * Inits RealTimeNeurofeedbackResult and adds uiNumChannels empty channel information
     *
     * @param [in] uiNumChannels     the number of channels to init.
     */
    void init(QList<RealTimeSampleArrayChInfo> &chInfo);

    //=========================================================================================================
    /**
     * Init channel infos using fiff info
     *
     * @param[in] pFiffInfo     Info to init from
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
     * Sets the sampling rate of the RealTimeNeurofeedbackResult Measurement.
     *
     * @param[in] fSamplingRate the sampling rate of the RealTimeNeurofeedbackResult.
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
     * Returns the gathered Neurofeedback.
     *
     * @return the current Neurofeedback.
     */
    inline const QList<Eigen::MatrixXd>& getNeurofeedback();

    //=========================================================================================================
    /**
     * Attaches a value to the Neurofeedback list.
     *
     * @param [in] mat   the value which is attached to the Neurofeedback list.
     */
    virtual void setValue(const Eigen::MatrixXd& mat);

    //=========================================================================================================
    /**
     * Sets the Neurofeedback Output.
     *
     * @param[in] routput       Neurofeedback Output.
     */
    inline void setNeuroOutput(QString routput);

    //=========================================================================================================
    /**
     * Sets the textoutput for the Classifications.
     *
     * @param[in] Class         textoutput for Classification.
     * @param[in] value         Classification.
     */
    inline void setClassL(QString Class, qint32 value);

    //=========================================================================================================
    /**
     * Sets the Integer for the Classifications.
     *
     * @param[in] Class         Integer for Classification.
     * @param[in] value         Classification.
     */
    inline void setClassI(qint32 Class, qint32 value);

    //=========================================================================================================
    /**
     * Sets the colour for the Textoutput (Classification Output).
     *
     * @param[in] ColClass      Colour for Textoutput.
     * @param[in] value         Classification.
     */
    inline void setClassCol(QString ColClass, qint32 value);

    //=========================================================================================================
    /**
     * Sets the Image for the Classifications.
     *
     * @param[in] ImgClass      Pixmap for Imageoutput.
     * @param[in] value         Classification.
     */
    inline void setClassImg(QPixmap ImgClass, qint32 value);

    //=========================================================================================================
    /**
     * Sets the Image for the Background (Balloon Output).
     *
     * @param[in] ImgBackground Pixmap for Background.
     */
    inline void setImgBackground(QPixmap ImgBackground);

    //=========================================================================================================
    /**
     * Sets the Image for the Object (Balloon Output).
     *
     * @param[in] ImgObject     Pixmap for Object.
     */
    inline void setImgObject(QPixmap ImgObject);

    //=========================================================================================================
    /**
     * Sets the number of sliders (Frequency Output).
     *
     * @param[in] value         number of sliders.
     */
    inline void setSlider(qint32 value);

    //=========================================================================================================
    /**
     * Sets the number of Classifications (Classification Output).
     *
     * @param[in] value         number of classifications.
     */
    inline void setNumbofClass(qint32 value);

    //=========================================================================================================
    /**
     * Sets the bool for all Channels (Frequency Output).
     *
     * @param[in] ballCh        bool for all Channels selected.
     */
    inline void setallCh(bool ballCh);

    //=========================================================================================================
    /**
     * Sets the bool for AutoScale for the minimum (Frequency Output).
     *
     * @param[in] bMinAutoScale  bool for Autoscale for the minimum.
     */
    inline void setMinAutoScale(bool bMinAutoScale);

    //=========================================================================================================
    /**
     * Sets the bool for Autoscale for the maximum (Frequency Output).
     *
     * @param[in] bMaxAutoScale  bool for Autoscale for the maximum.
     */
    inline void setMaxAutoScale(bool bMaxAutoScale);

    //=========================================================================================================
    /**
     * Sets the bool for Reset Autoscale (Frequency Output).
     *
     * @param[in] bResetAutoscale  bool for Reset Autoscale.
     */
    inline void setResetAutoScale(bool bResetAutoScale);

    //=========================================================================================================
    /**
     * Sets the slidermaximum (Frequency Output).
     *
     * @param[in] value         slidermaximum.
     */
    inline void setFMax(qint32 value);

    //=========================================================================================================
    /**
     * Sets the sliderminimum (Frequency Output).
     *
     * @param[in] value         sliderminimum.
     */
    inline void setFMin(qint32 value);

    //=========================================================================================================
    /**
     * Sets the maximum height of the balloon (Balloon Output).
     *
     * @param[in] value         height maximum of the balloon.
     */
    inline void setBMax(qint32 value);

    //=========================================================================================================
    /**
     * Sets the minimum height of the balloon (Balloon Output).
     *
     * @param[in] value         height minimum of the balloon.
     */
    inline void setBMin(qint32 value);

    //=========================================================================================================
    /**
     * Sets the channel (Balloon Output).
     *
     * @param[in] value         channelnumber.
     */
    inline void setChannel(qint32 value);

    //=========================================================================================================
    /**
     * Sets the Bool for new AutoScale Start.
     *
     * @param[in] bScaleStart   new AutoScale Start.
     */
    inline void setScaleStart(bool bScaleStart);

    //=========================================================================================================
    /**
     * Returns the Neurofeedback Output.
     *
     * @return the current Neurofeedback.
     */
    inline QString getNeuroOutput();

    //=========================================================================================================
    /**
     * Returns the label/textoutput (Classification Output).
     *
     * @param[in] value         Classification.
     * @return textoutput for Classification.
     */
    inline QString getClassL(qint32 value);

    //=========================================================================================================
    /**
     * Returns the integer (Classification Output).
     *
     * @param[in] value         Classification.
     * @return integers for Classification.
     */
    inline qint32 getClassI(qint32 value);

    //=========================================================================================================
    /**
     * Returns the colour for textoutput (Classification Output).
     *
     * @param[in] value         Classification.
     * @return colour for textoutput.
     */
    inline QString getClassCol(qint32 value);

    //=========================================================================================================
    /**
     * Returns the image (Classification Output).
     *
     * @param[in] value         Classification.
     * @return pixmap for Classification.
     */
    inline QPixmap getClassImg(qint32 value);

    //=========================================================================================================
    /**
     * Returns the background (Balloon Output).
     *
     * @return pixmap for background.
     */
    inline QPixmap getImgBackground();

    //=========================================================================================================
    /**
     * Returns the object (Balloon Output).
     *
     * @return pixmap for object.
     */
    inline QPixmap getImgObject();

    //=========================================================================================================
    /**
     * Returns the number of sliders (Frequency Output).
     *
     * @return number of sliders.
     */
    inline qint32 getSlider();

    //=========================================================================================================
    /**
     * Returns the number of Classifications (Classification Output).
     *
     * @return number of Classifications.
     */
    inline qint32 getNumbofClass();

    //=========================================================================================================
    /**
     * Returns the slidermaximum (Frequency Output).
     *
     * @return slidermaximum.
     */
    inline qint32 getFMax();

    //=========================================================================================================
    /**
     * Returns the sliderminimum (Frequency Output).
     *
     * @return sliderminimum.
     */
    inline qint32 getFMin();

    //=========================================================================================================
    /**
     * Returns the maximum height of the balloon (Balloon Output).
     *
     * @return maximum height of the balloon.
     */
    inline qint32 getBMax();

    //=========================================================================================================
    /**
     * Returns the minimum height of the balloon (Balloon Output).
     *
     * @return minimum height of the balloon.
     */
    inline qint32 getBMin();

    //=========================================================================================================
    /**
     * Returns the current channel (Balloon Output).
     *
     * @return current channel selected.
     */
    inline qint32 getChannel();

    //=========================================================================================================
    /**
     * Returns the bool for all Channels selected (Frequency Output).
     *
     * @return bool for all channels selected.
     */
    inline bool getallCh();

    //=========================================================================================================
    /**
     * Returns the bool for Minimum Autoscale (Frequency Output).
     *
     * @return bool for minimum Autoscale.
     */
    inline bool getMinAutoScale();

    //=========================================================================================================
    /**
     * Returns the bool for Maximum Autoscale (Frequency Output).
     *
     * @return bool for maximum Autoscale.
     */
    inline bool getMaxAutoScale();

    //=========================================================================================================
    /**
     * Returns the bool for Reset Autoscale (Frequency Output).
     *
     * @return bool for reset Autoscale.
     */
    inline bool getResetAutoscale();

private:
    mutable QMutex              m_qMutex;           /**< Mutex to ensure thread safety */

    FIFFLIB::FiffInfo::SPtr     m_pFiffInfo_orig;   /**< Original Fiff Info if initialized by fiff info. */

    QString                     m_sXMLLayoutFile;   /**< Layout file name. */
    float                       m_fSamplingRate;    /**< Sampling rate of the RealTimeSampleArray.*/
    qint32                      m_iMultiArraySize;  /**< Sample size of the multi sample array.*/
    QList<Eigen::MatrixXd>      m_Neurolist;       /**< The multi sample array.*/

    bool                        m_bChInfoIsInit;    /**< If channel info is initialized.*/

    qint32                      m_iActionres;
    QString                     m_sNeuroOutput;
    qint32                      m_iSlider;
    qint32                      m_iNumbofClass;
    qint32                      m_iFMax;
    qint32                      m_iFMin;
    qint32                      m_iBMax;
    qint32                      m_iBMin;
    qint32                      m_iChannel;

    QString                     m_sClass0;
    QString                     m_sClass1;
    QString                     m_sClass2;
    QString                     m_sClass3;
    QString                     m_sClass4;
    QString                     m_sDist;

    bool                        m_ballCh;
    bool                        m_bMinAutoScale;
    bool                        m_bMaxAutoScale;
    bool                        m_bResetAutoScale;

    QVector<QString>            m_vClassL;
    QVector<qint32>             m_vClassI;
    QVector<QString>            m_vClassCol;
    QVector<QPixmap>            m_vClassImg;

    QPixmap                     m_ImgBackground;
    QPixmap                     m_ImgObject;

    QList<RealTimeSampleArrayChInfo> m_qListChInfo; /**< Channel info list.*/

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void RealTimeNeurofeedbackResult::clear()
{
    QMutexLocker locker(&m_qMutex);
    m_Neurolist.clear();
}

//=============================================================================================================

inline bool RealTimeNeurofeedbackResult::isChInit() const
{
    QMutexLocker locker(&m_qMutex);
    return m_bChInfoIsInit;
}

//=============================================================================================================

inline const QString& RealTimeNeurofeedbackResult::getXMLLayoutFile() const
{
    QMutexLocker locker(&m_qMutex);
    return m_sXMLLayoutFile;
}

//=============================================================================================================

inline void RealTimeNeurofeedbackResult::setXMLLayoutFile(const QString& layout)
{
    QMutexLocker locker(&m_qMutex);
    m_sXMLLayoutFile = layout;
}

//=============================================================================================================

inline void RealTimeNeurofeedbackResult::setSamplingRate(float fSamplingRate)
{
    QMutexLocker locker(&m_qMutex);
    m_fSamplingRate = fSamplingRate;
}

//=============================================================================================================

inline float RealTimeNeurofeedbackResult::getSamplingRate() const
{
    QMutexLocker locker(&m_qMutex);
    return m_fSamplingRate;
}

//=============================================================================================================

inline unsigned int RealTimeNeurofeedbackResult::getNumChannels() const
{
    QMutexLocker locker(&m_qMutex);
    return m_qListChInfo.size();
}

//=============================================================================================================

inline QList<RealTimeSampleArrayChInfo>& RealTimeNeurofeedbackResult::chInfo()
{
    QMutexLocker locker(&m_qMutex);
    return m_qListChInfo;
}

//=============================================================================================================

inline FIFFLIB::FiffInfo::SPtr RealTimeNeurofeedbackResult::info()
{
    QMutexLocker locker(&m_qMutex);
    return m_pFiffInfo_orig;
}

//=============================================================================================================

inline void RealTimeNeurofeedbackResult::setMultiArraySize(qint32 iMultiArraySize)
{
    m_iMultiArraySize = iMultiArraySize;
}

//=============================================================================================================

qint32 RealTimeNeurofeedbackResult::getMultiArraySize() const
{
    QMutexLocker locker(&m_qMutex);
    return m_iMultiArraySize;
}

//=============================================================================================================

inline void RealTimeNeurofeedbackResult::setNeuroOutput(QString routput)
{
    QMutexLocker locker(&m_qMutex);
    m_sNeuroOutput = routput;
}

//=============================================================================================================

inline QString RealTimeNeurofeedbackResult::getNeuroOutput()
{
    QMutexLocker locker(&m_qMutex);
    return m_sNeuroOutput;
}

//=============================================================================================================

inline void RealTimeNeurofeedbackResult::setNumbofClass(qint32 value)
{
    QMutexLocker locker(&m_qMutex);
    m_iNumbofClass = value;
}

//=============================================================================================================

inline int RealTimeNeurofeedbackResult::getNumbofClass()
{
    QMutexLocker locker(&m_qMutex);
    return m_iNumbofClass;
}

//============================================================================================================

inline void RealTimeNeurofeedbackResult::setallCh(bool ballCh)
{
    QMutexLocker locker(&m_qMutex);
    m_ballCh = ballCh;

}

//============================================================================================================

inline bool RealTimeNeurofeedbackResult::getallCh()
{
    QMutexLocker locker(&m_qMutex);
    return m_ballCh;
}

//============================================================================================================

inline void RealTimeNeurofeedbackResult::setMaxAutoScale(bool bMaxAutoScale)
{
    QMutexLocker locker(&m_qMutex);
    m_bMaxAutoScale = bMaxAutoScale;

}

//============================================================================================================

inline bool RealTimeNeurofeedbackResult::getMaxAutoScale()
{
    QMutexLocker locker(&m_qMutex);
    return m_bMaxAutoScale;
}

//============================================================================================================

inline void RealTimeNeurofeedbackResult::setMinAutoScale(bool bMinAutoScale)
{
    QMutexLocker locker(&m_qMutex);
    m_bMinAutoScale = bMinAutoScale;

}

//============================================================================================================

inline bool RealTimeNeurofeedbackResult::getMinAutoScale()
{
    QMutexLocker locker(&m_qMutex);
    return m_bMinAutoScale;
}

//============================================================================================================

inline void RealTimeNeurofeedbackResult::setResetAutoScale(bool bResetAutoScale)
{
    QMutexLocker locker(&m_qMutex);
    m_bResetAutoScale = bResetAutoScale;

}

//============================================================================================================

inline bool RealTimeNeurofeedbackResult::getResetAutoscale()
{
    QMutexLocker locker(&m_qMutex);
    return m_bResetAutoScale;
}

//=============================================================================================================

inline void RealTimeNeurofeedbackResult::setClassL(QString Class, qint32 value)
{
    QMutexLocker locker(&m_qMutex);
    if(m_vClassL.isEmpty()){
        m_vClassL.resize(5);
        m_vClassL.fill("Empty");
    }
    if(m_vClassCol.isEmpty()){
        m_vClassCol.resize(5);
        m_vClassCol.fill("black");
    }
    if(m_vClassImg.isEmpty()){
        m_vClassImg.resize(5);
    }
    m_vClassL[value] = Class;
}

//=============================================================================================================

inline QString RealTimeNeurofeedbackResult::getClassL(qint32 value)
{
    QMutexLocker locker(&m_qMutex);
    return m_vClassL[value];
}

//=============================================================================================================

inline void RealTimeNeurofeedbackResult::setClassI(qint32 Class, qint32 value)
{
    QMutexLocker locker(&m_qMutex);
    if(m_vClassI.isEmpty()){
        m_vClassI.resize(5);
        m_vClassI.fill(0);
    }
    m_vClassI[value] = Class;
}

//=============================================================================================================

inline int RealTimeNeurofeedbackResult::getClassI(qint32 value)
{
    QMutexLocker locker(&m_qMutex);
    return m_vClassI[value];
}


//=============================================================================================================

inline void RealTimeNeurofeedbackResult::setClassCol(QString ColClass, qint32 value)
{
    QMutexLocker locker(&m_qMutex);
    m_vClassCol[value] = ColClass;
}

//=============================================================================================================

inline QString RealTimeNeurofeedbackResult::getClassCol(qint32 value)
{
    QMutexLocker locker(&m_qMutex);
    return m_vClassCol[value];
}

//=============================================================================================================

inline void RealTimeNeurofeedbackResult::setClassImg(QPixmap ImgClass, qint32 value)
{
    QMutexLocker locker(&m_qMutex);
    m_vClassImg[value] = ImgClass;
}

//=============================================================================================================

inline QPixmap RealTimeNeurofeedbackResult::getClassImg(qint32 value)
{
    QMutexLocker locker(&m_qMutex);
    return m_vClassImg[value];
}

//=============================================================================================================

inline void RealTimeNeurofeedbackResult::setImgBackground(QPixmap ImgBackground)
{
    QMutexLocker locker(&m_qMutex);
    m_ImgBackground = ImgBackground;
}

//=============================================================================================================

inline QPixmap RealTimeNeurofeedbackResult::getImgBackground()
{
    QMutexLocker locker(&m_qMutex);
    return m_ImgBackground;
}

//=============================================================================================================

inline void RealTimeNeurofeedbackResult::setImgObject(QPixmap ImgObject)
{
    QMutexLocker locker(&m_qMutex);
    m_ImgObject = ImgObject;
}

//=============================================================================================================

inline QPixmap RealTimeNeurofeedbackResult::getImgObject()
{
    QMutexLocker locker(&m_qMutex);
    return m_ImgObject;
}

//=============================================================================================================


inline void RealTimeNeurofeedbackResult::setSlider(qint32 value)
{
    QMutexLocker locker(&m_qMutex);
    m_iSlider = value;
}

//=============================================================================================================

inline int RealTimeNeurofeedbackResult::getSlider()
{
    QMutexLocker locker(&m_qMutex);
    return m_iSlider;
}

//=============================================================================================================


inline void RealTimeNeurofeedbackResult::setFMax(qint32 value)
{
    QMutexLocker locker(&m_qMutex);
    m_iFMax = value;
}

//=============================================================================================================

inline int RealTimeNeurofeedbackResult::getFMax()
{
    QMutexLocker locker(&m_qMutex);
    return m_iFMax;
}

//=============================================================================================================


inline void RealTimeNeurofeedbackResult::setFMin(qint32 value)
{
    QMutexLocker locker(&m_qMutex);
    m_iFMin = value;
}

//=============================================================================================================

inline int RealTimeNeurofeedbackResult::getFMin()
{
    QMutexLocker locker(&m_qMutex);
    return m_iFMin;
}

//=============================================================================================================


inline void RealTimeNeurofeedbackResult::setBMax(qint32 value)
{
    QMutexLocker locker(&m_qMutex);
    m_iBMax = value;
}

//=============================================================================================================

inline int RealTimeNeurofeedbackResult::getBMax()
{
    QMutexLocker locker(&m_qMutex);
    return m_iBMax;
}

//=============================================================================================================

inline void RealTimeNeurofeedbackResult::setBMin(qint32 value)
{
    QMutexLocker locker(&m_qMutex);
    m_iBMin = value;
}

//=============================================================================================================

inline int RealTimeNeurofeedbackResult::getBMin()
{
    QMutexLocker locker(&m_qMutex);
    return m_iBMin;
}


//=============================================================================================================

inline void RealTimeNeurofeedbackResult::setChannel(qint32 value)
{
    QMutexLocker locker(&m_qMutex);
    m_iChannel = value;
}

//=============================================================================================================

inline qint32 RealTimeNeurofeedbackResult::getChannel()
{
    QMutexLocker locker(&m_qMutex);
    return m_iChannel;
}

//=============================================================================================================

inline const QList<Eigen::MatrixXd>& RealTimeNeurofeedbackResult::getNeurofeedback()
{
    QMutexLocker locker(&m_qMutex);
    return m_Neurolist;
}



} // NAMESPACE

Q_DECLARE_METATYPE(SCMEASLIB::RealTimeNeurofeedbackResult::SPtr)

#endif // REALTIMENEUROFEEDBACKRESULT_H
