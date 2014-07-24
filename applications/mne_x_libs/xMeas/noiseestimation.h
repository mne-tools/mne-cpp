//=============================================================================================================
/**
* @file     noiseestimation.h
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
* @brief    Contains the declaration of the NoiseEstimation class.
*
*/

#ifndef NOISEESTIMATION_H
#define NOISEESTIMATION_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "xmeas_global.h"
#include "newmeasurement.h"
#include "realtimesamplearraychinfo.h"

#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>
#include <QList>
#include <QColor>


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
* DECLARE CLASS NoiseEstimation
*
* @brief The RealTimeMultiSampleArrayNew class is the base class of every RealTimeMultiSampleArrayNew Measurement.
*/
class XMEASSHARED_EXPORT NoiseEstimation : public NewMeasurement
{
    Q_OBJECT
public:
    typedef QSharedPointer<NoiseEstimation> SPtr;               /**< Shared pointer type for NoiseEstimation. */
    typedef QSharedPointer<const NoiseEstimation> ConstSPtr;    /**< Const shared pointer type for NoiseEstimation. */

    //=========================================================================================================
    /**
    * Constructs a RealTimeMultiSampleArrayNew.
    */
    explicit NoiseEstimation(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the RealTimeMultiSampleArrayNew.
    */
    virtual ~NoiseEstimation();

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
    inline FiffInfo::SPtr& getFiffInfo();

    //=========================================================================================================
    /**
    * New data block to distribute
    *
    * @param [in] v the value which is should be distributed.
    */
    virtual void setValue(MatrixXd& v);

    //=========================================================================================================
    /**
    * Returns the current value set.
    * This method is inherited by Measurement.
    *
    * @return the last attached value.
    */
    virtual MatrixXd getValue() const;

    //=========================================================================================================
    /**
    * Returns whether NoiseEstimation contains values
    *
    * @return whether NoiseEstimation contains values.
    */
    inline bool containsValues() const;

private:
    FiffInfo::SPtr              m_pFiffInfo_orig;   /**< Original Fiff Info if initialized by fiff info. */

    MatrixXd                    m_matValue;         /**< The current attached sample vector.*/

    QList<RealTimeSampleArrayChInfo> m_qListChInfo; /**< Channel info list.*/

    bool                        m_bChInfoIsInit;    /**< If channel info is initialized.*/
    bool                        m_bContainsValues;  /**< If values are stored.*/
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


inline bool NoiseEstimation::isChInit() const
{
    return m_bChInfoIsInit;
}


//*************************************************************************************************************

inline unsigned int NoiseEstimation::getNumChannels() const
{
    return m_qListChInfo.size();
}


//*************************************************************************************************************

inline QList<RealTimeSampleArrayChInfo>& NoiseEstimation::chInfo()
{
    return m_qListChInfo;
}


//*************************************************************************************************************

inline FiffInfo::SPtr& NoiseEstimation::getFiffInfo()
{
    return m_pFiffInfo_orig;
}


//*************************************************************************************************************

inline bool NoiseEstimation::containsValues() const
{
    return m_bContainsValues;
}

} // NAMESPACE

Q_DECLARE_METATYPE(XMEASLIB::NoiseEstimation::SPtr)

#endif // NOISEESTIMATION_H
