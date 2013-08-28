//=============================================================================================================
/**
* @file     realtimesamplearraychinfo.h
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
* @brief    Contains the declaration of the RealTimeSampleArrayChInfo class.
*
*/

#ifndef REALTIMESAMPLEARRAYCHINFO_H
#define REALTIMESAMPLEARRAYCHINFO_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "xmeas_global.h"

#include <fiff/fiff_constants.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>


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


//=========================================================================================================
/**
* Channel Info for RealTimeSampleArray, used in RealTimeMultiSampleArray
*
* @brief Channel Info for RealTimeSampleArray
*/
class XMEASSHARED_EXPORT RealTimeSampleArrayChInfo
{
public:
    typedef QSharedPointer<RealTimeSampleArrayChInfo> SPtr;               /**< Shared pointer type for RealTimeSampleArrayChInfo. */
    typedef QSharedPointer<const RealTimeSampleArrayChInfo> ConstSPtr;    /**< Const shared pointer type for RealTimeSampleArrayChInfo. */

    //=========================================================================================================
    /**
    * Constructs a RealTimeSampleArrayChInfo.
    */
    RealTimeSampleArrayChInfo();

    //=========================================================================================================
    /**
    * Destroys the RealTimeSampleArrayChInfo.
    */
    virtual ~RealTimeSampleArrayChInfo();

    //=========================================================================================================
    /**
    * Sets the channel name
    *
    * @param[in] p_qStringChName    channel name
    */
    inline void setChannelName(QString p_qStringChName);

    //=========================================================================================================
    /**
    * Returns the channel name
    *
    * @return the channel name.
    */
    inline QString getChannelName() const;

    //=========================================================================================================
    /**
    * Sets the channel kind
    *
    * @param[in] p_iKind    channel kind
    */
    inline void setKind(qint32 p_iKind);

    //=========================================================================================================
    /**
    * Returns the channel kind
    *
    * @return the channel kind.
    */
    inline qint32 getKind() const;

    //=========================================================================================================
    /**
    * Sets the minimal value. If current value to set is smaller, current value is set to minimal value.
    *
    * @param[in] minValue minimal value.
    */
    inline void setMinValue(double minValue);

    //=========================================================================================================
    /**
    * Returns the minimal value.
    *
    * @return the minimal value.
    */
    inline double getMinValue() const;

    //=========================================================================================================
    /**
    * Sets the maximal value. If value to set is bigger, current value is set to maximal value.
    *
    * @param [in] maxValue maximal value.
    */
    inline void setMaxValue(double maxValue);

    //=========================================================================================================
    /**
    * Returns the maximal value.
    *
    * @return the maximal value.
    */
    inline double getMaxValue() const;

    //=========================================================================================================
    /**
    * Sets the unit of the RealTimeSampleArray data.
    *
    * @param [in] unit of the data.
    */
    inline void setUnit(const QString& unit);

    //=========================================================================================================
    /**
    * Returns the unit of the RealTimeSampleArray measurement.
    *
    * @return the unit of the data of measurement.
    */
    inline const QString& getUnit() const;

private:
    QString     m_qStringChName;    /**< The channel name.*/
    double      m_dMinValue;        /**< The minimal value.*/
    double      m_dMaxValue;        /**< The maximal value.*/
    qint32      m_iKind;            /**< The channel kind.*/
    QString     m_qString_Unit;     /**< Unit of the data of the measurement.*/

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void RealTimeSampleArrayChInfo::setChannelName(QString p_qStringChName)
{
    m_qStringChName = p_qStringChName;
}


//*************************************************************************************************************

inline QString RealTimeSampleArrayChInfo::getChannelName() const
{
    return m_qStringChName;
}


//*************************************************************************************************************

inline void RealTimeSampleArrayChInfo::setKind(qint32 p_iKind)
{
    m_iKind = p_iKind;
}


//*************************************************************************************************************

inline qint32 RealTimeSampleArrayChInfo::getKind() const
{
    return m_iKind;
}


//*************************************************************************************************************

inline void RealTimeSampleArrayChInfo::setMinValue(double minValue)
{
    m_dMinValue = minValue;
}


//*************************************************************************************************************

inline double RealTimeSampleArrayChInfo::getMinValue() const
{
    return m_dMinValue;
}


//*************************************************************************************************************

inline void RealTimeSampleArrayChInfo::setMaxValue(double maxValue)
{
    if(m_iKind != FIFFV_STIM_CH) // ToDo dirty hack // don't set max for stim channels
        m_dMaxValue = maxValue;
}


//*************************************************************************************************************

inline double RealTimeSampleArrayChInfo::getMaxValue() const
{
    return m_dMaxValue;
}


//*************************************************************************************************************

inline void RealTimeSampleArrayChInfo::setUnit(const QString& unit)
{
    m_qString_Unit = unit;
}


//*************************************************************************************************************

inline const QString& RealTimeSampleArrayChInfo::getUnit() const
{
    return m_qString_Unit;
}

} // NAMESPACE

#endif // REALTIMESAMPLEARRAYCHINFO_H
