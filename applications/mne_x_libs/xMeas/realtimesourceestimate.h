//=============================================================================================================
/**
* @file     realtimesourceestimate.h
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
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
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


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "xmeas_global.h"
#include "newmeasurement.h"

#include <fs/label.h>
#include <fs/surfaceset.h>
#include <fs/annotationset.h>
#include <fiff/fiff_info.h>
#include <mne/mne_sourcespace.h>
#include <mne/mne_sourceestimate.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>
#include <QList>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XMEASLIB
//=============================================================================================================

namespace XMEASLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;
using namespace FSLIB;


//=========================================================================================================
/**
* RealTimeSourceEstimate
*
* @brief Real-time source estimate measurement.
*/
class XMEASSHARED_EXPORT RealTimeSourceEstimate : public NewMeasurement
{
public:
    typedef QSharedPointer<RealTimeSourceEstimate> SPtr;               /**< Shared pointer type for RealTimeSourceEstimate. */
    typedef QSharedPointer<const RealTimeSourceEstimate> ConstSPtr;    /**< Const shared pointer type for RealTimeSourceEstimate. */

    //=========================================================================================================
    /**
    * Constructs a RealTimeSourceEstimate.
    *
    * @param[in] parent     the QObject parent of this measurement
    */
    RealTimeSourceEstimate(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the RealTimeSourceEstimate.
    */
    virtual ~RealTimeSourceEstimate();

    //=========================================================================================================
    /**
    * Returns the list of labels.
    *
    * @return the list of labels
    */
    inline QList<Label> getLabels();

    //=========================================================================================================
    /**
    * Returns the list of RGBAs.
    *
    * @return the list of RGBAs
    */
    inline QList<RowVector4i> getRGBAs();

    //=========================================================================================================
    /**
    * Sets the annotation set.
    *
    * @param[in] annotSet   the annotation set to set
    */
    inline void setAnnotSet(AnnotationSet::SPtr& annotSet);

    //=========================================================================================================
    /**
    * Returns the annotation set.
    *
    * @return the annotation set
    */
    inline AnnotationSet::SPtr& getAnnotSet();

    //=========================================================================================================
    /**
    * Sets the surface set.
    *
    * @param[in] surfSet   the surface set to set
    */
    inline void setSurfSet(SurfaceSet::SPtr& surfSet);

    //=========================================================================================================
    /**
    * Returns the surface set.
    *
    * @return the surface set
    */
    inline SurfaceSet::SPtr& getSurfSet();

    //=========================================================================================================
    /**
    * Sets the sampling rate of the NewRealTimeSampleArray Measurement.
    *
    * @param [in] dSamplingRate the sampling rate of the NewRealTimeSampleArray.
    */
    inline void setSamplingRate(double dSamplingRate);

    //=========================================================================================================
    /**
    * Returns the sampling rate of the NewRealTimeSampleArray Measurement.
    *
    * @return the sampling rate of the NewRealTimeSampleArray.
    */
    inline double getSamplingRate() const;

    //=========================================================================================================
    /**
    * Sets the number of sample vectors which should be gathered before attached observers are notified by calling the Subject notify() method.
    *
    * @param[in] iArraySize the number of values.
    */
    inline void setArraySize(qint32 iArraySize);

    //=========================================================================================================
    /**
    * Returns the number of values which should be gathered before attached observers are notified by calling the Subject notify() method.
    *
    * @return the number of values which are gathered before a notify() is called.
    */
    inline qint32 getArraySize() const;

    //=========================================================================================================
    /**
    * Attaches a value to the sample array vector.
    * This method is inherited by Measurement.
    *
    * @param [in] v the value which is attached to the sample array vector.
    */
    virtual void setValue(VectorXd v);

    //=========================================================================================================
    /**
    * Returns the current value set.
    * This method is inherited by Measurement.
    *
    * @return the last attached value.
    */
    virtual VectorXd getValue() const;

    //=========================================================================================================
    /**
    * Sets the source space
    *
    * @param[in] src    sets the source space
    */
    inline void setSrc(MNESourceSpace& src);

    //=========================================================================================================
    /**
    * Returns the source space if source space is set.
    *
    * @return the connected source space
    */
    inline MNESourceSpace& getSrc();

    //=========================================================================================================
    /**
    * Returns if source space is set.
    *
    * @return the connected source space
    */
    inline MNESourceEstimate& getStc();

private:
    AnnotationSet::SPtr     m_pAnnotSet;    /**< Annotation set. */
    SurfaceSet::SPtr        m_pSurfSet;     /**< Surface set. */
    MNESourceSpace          m_pSrc;         /**< Source space. */

    QList<Label> m_pListLabel;          /**< List of labels. */
    QList<RowVector4i> m_pListRGBA;     /**< List of RGBAs corresponding to labels. */

    double                      m_dSamplingRate;    /**< Sampling rate of the RealTimeSampleArray.*/
    float                       m_fT;               /**< Time between two samples.*/
    VectorXd                    m_vecValue;         /**< The current attached sample vector.*/
    qint32                      m_iArraySize;      /**< Sample size of the source estimate.*/
    qint32                      m_iCurIdx;         /**< Sample size of the multi sample array.*/

    float                       m_fCurTimePoint;    /**< The current time point.*/

    MNESourceEstimate           m_MNEStc;           /**< The source estimate. */

    bool                    m_bStcSend;
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QList<Label> RealTimeSourceEstimate::getLabels()
{
    if(m_pListLabel.size() == 0 && m_pAnnotSet.isNull() && m_pSurfSet.isNull())
        m_pAnnotSet->toLabels(*m_pSurfSet.data(), m_pListLabel, m_pListRGBA);
    return m_pListLabel;
}


//*************************************************************************************************************

inline QList<RowVector4i> RealTimeSourceEstimate::getRGBAs()
{
    if(m_pListRGBA.size() == 0 && m_pAnnotSet.isNull() && m_pSurfSet.isNull())
        m_pAnnotSet->toLabels(*m_pSurfSet.data(), m_pListLabel, m_pListRGBA);
    return m_pListRGBA;
}


//*************************************************************************************************************

inline void RealTimeSourceEstimate::setAnnotSet(AnnotationSet::SPtr& annotSet)
{
    m_pAnnotSet = annotSet;
}


//*************************************************************************************************************

inline AnnotationSet::SPtr& RealTimeSourceEstimate::getAnnotSet()
{
    return m_pAnnotSet;
}


//*************************************************************************************************************

inline void RealTimeSourceEstimate::setSurfSet(SurfaceSet::SPtr& surfSet)
{
    m_pSurfSet = surfSet;
}


//*************************************************************************************************************

inline SurfaceSet::SPtr& RealTimeSourceEstimate::getSurfSet()
{
    return m_pSurfSet;
}


//*************************************************************************************************************

inline void RealTimeSourceEstimate::setSamplingRate(double dSamplingRate)
{
    m_dSamplingRate = dSamplingRate;
    m_fT = 1.0f/(float)dSamplingRate;

    m_MNEStc.tstep = m_fT;
}


//*************************************************************************************************************

inline double RealTimeSourceEstimate::getSamplingRate() const
{
    return m_dSamplingRate;
}


//*************************************************************************************************************

inline void RealTimeSourceEstimate::setArraySize(qint32 iArraySize)
{
    //Obsolete unsigned char can't be bigger
    if(iArraySize > 255)
        m_iArraySize = 255;
    else
        m_iArraySize = iArraySize;

    //reset data
    m_MNEStc.data = MatrixXd(0,0);
    m_MNEStc.times = RowVectorXf::Zero(m_iArraySize);
}


//*************************************************************************************************************

qint32 RealTimeSourceEstimate::getArraySize() const
{
    return m_iArraySize;
}


//*************************************************************************************************************

inline void RealTimeSourceEstimate::setSrc(MNESourceSpace& src)
{
    m_pSrc = src;
}


//*************************************************************************************************************

inline MNESourceSpace& RealTimeSourceEstimate::getSrc()
{
    return m_pSrc;
}


//*************************************************************************************************************

inline MNESourceEstimate& RealTimeSourceEstimate::getStc()
{
    return m_MNEStc;
}

} // NAMESPACE

Q_DECLARE_METATYPE(XMEASLIB::RealTimeSourceEstimate::SPtr)

#endif // REALTIMESOURCEESTIMATE_H
