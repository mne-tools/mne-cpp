//=============================================================================================================
/**
* @file     realtimeevoked.h
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
* @brief    Contains the declaration of the RealTimeEvoked class.
*
*/

#ifndef REALTIMEEVOKED_H
#define REALTIMEEVOKED_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "xmeas_global.h"
#include "newmeasurement.h"
#include "realtimesamplearraychinfo.h"

#include <fiff/fiff_evoked.h>


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
* DECLARE CLASS RealTimeEvoked -> ToDo check feasibilty of QAbstractTableModel
*
* @brief The RealTimeMultiSampleArrayNew class is the base class of every RealTimeMultiSampleArrayNew Measurement.
*/
class XMEASSHARED_EXPORT RealTimeEvoked : public NewMeasurement
{
    Q_OBJECT
public:
    typedef QSharedPointer<RealTimeEvoked> SPtr;               /**< Shared pointer type for RealTimeEvoked. */
    typedef QSharedPointer<const RealTimeEvoked> ConstSPtr;    /**< Const shared pointer type for RealTimeEvoked. */

    //=========================================================================================================
    /**
    * Constructs a RealTimeMultiSampleArrayNew.
    */
    explicit RealTimeEvoked(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the RealTimeMultiSampleArrayNew.
    */
    virtual ~RealTimeEvoked();

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
    * @return the number of pre-stimulus samples
    */
    inline qint32 getNumPreStimSamples() const;

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
    * New devoked to distribute
    *
    * @param [in] v     the evoked which should be distributed.
    */
    virtual void setValue(FiffEvoked& v);

    //=========================================================================================================
    /**
    * Returns the current value set.
    * This method is inherited by Measurement.
    *
    * @return the last attached value.
    */
    virtual FiffEvoked::SPtr& getValue();

    //=========================================================================================================
    /**
    * Returns whether RealTimeEvoked contains values
    *
    * @return whether RealTimeEvoked contains values.
    */
    inline bool isInitialized() const;

private:
    //=========================================================================================================
    /**
    * Init channel infos using fiff info
    *
    * @param[in] p_fiffInfo     Info to init from
    */
    void init(FiffInfo &p_fiffInfo);

    FiffEvoked::SPtr            m_pFiffEvoked;      /**< Evoked data set */

    QString                     m_sXMLLayoutFile;   /**< Layout file name. */

    qint32                      m_iPreStimSamples;  /**< Number of pre-stimulus samples */

    QList<QColor>               m_qListChColors;    /**< Channel color for butterfly plot.*/
    QList<RealTimeSampleArrayChInfo> m_qListChInfo; /**< Channel info list.*/
    bool                        m_bInitialized;     /**< If values are stored.*/
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const QString& RealTimeEvoked::getXMLLayoutFile() const
{
    return m_sXMLLayoutFile;
}


//*************************************************************************************************************

inline void RealTimeEvoked::setXMLLayoutFile(const QString& layout)
{
    m_sXMLLayoutFile = layout;
}


//*************************************************************************************************************

inline unsigned int RealTimeEvoked::getNumChannels() const
{
    return m_qListChInfo.size();
}


//*************************************************************************************************************

inline qint32 RealTimeEvoked::getNumPreStimSamples() const
{
    return m_iPreStimSamples;
}


//*************************************************************************************************************

inline QList<QColor>& RealTimeEvoked::chColor()
{
    return m_qListChColors;
}


//*************************************************************************************************************

inline QList<RealTimeSampleArrayChInfo>& RealTimeEvoked::chInfo()
{
    return m_qListChInfo;
}


//*************************************************************************************************************

inline bool RealTimeEvoked::isInitialized() const
{
    return m_bInitialized;
}

} // NAMESPACE

Q_DECLARE_METATYPE(XMEASLIB::RealTimeEvoked::SPtr)

#endif // REALTIMEEVOKED_H
