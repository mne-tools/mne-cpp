//=============================================================================================================
/**
* @file     sensorlayout.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the SensorLayout Class.
*
*/

#ifndef SENSORLAYOUT_H
#define SENSORLAYOUT_H

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QStringList>
#include <QPointF>
#include <QSizeF>
#include <QtXml/QDomElement>


//=============================================================================================================
/**
* DECLARE CLASS SensorLayout
*
* @brief The SensorLayout class represents a channel layout
*/
class SensorLayout
{
public:
    //=========================================================================================================
    /**
    * Default constructor
    */
    SensorLayout();

    //=========================================================================================================
    /**
    * Returns the parsed sensor layout
    *
    * @param [in] sensorLayoutElement   the xml element which contains the sensor layout
    *
    * @return the parsed sensor layout
    */
    static SensorLayout parseSensorLayout(const QDomElement &sensorLayoutElement);

    //=========================================================================================================
    /**
    * Returns the name of the sensor layout
    *
    * @return the name of the layout
    */
    inline const QString& getName() const;

    //=========================================================================================================
    /**
    * Returns the full channel names of the layout sensors
    *
    * @return the list of full channel names
    */
    inline QStringList fullChNames() const;

    //=========================================================================================================
    /**
    * Returns the list of short channel names of the layout sensors for diplay purposes
    *
    * @return the list of short display channel names
    */
    inline QStringList shortChNames() const;

    //=========================================================================================================
    /**
    * Returns the number of channels
    *
    * @return the number of channels
    */
    inline qint32 numChannels() const;

    //=========================================================================================================
    /**
    * Returns the location for each channel
    *
    * @return the location for each channel
    */
    inline QList<QPointF> loc() const;

    //=========================================================================================================
    /**
    * Returns the dimension for each channel
    *
    * @return the dimension for each channel
    */
    inline QList<QSizeF> dim() const;

private:
    QString m_sName;    /**< Sensor layout name */

    QStringList m_qListFullChannelNames;    /**< List of full channel names */
    QStringList m_qListShortChannelNames;   /**< List of short channel names */
    QList<QPointF> m_qListLocations;        /**< List of channel locations */
    QList<QSizeF> m_qListDimensions;        /**< List of channel dimensions */

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const QString& SensorLayout::getName() const
{
    return m_sName;
}


//*************************************************************************************************************

inline QStringList SensorLayout::fullChNames() const
{
    return m_qListFullChannelNames;
}


//*************************************************************************************************************

inline QStringList SensorLayout::shortChNames() const
{
    return m_qListShortChannelNames;
}


//*************************************************************************************************************

inline qint32 SensorLayout::numChannels() const
{
    return m_qListFullChannelNames.size();
}


//*************************************************************************************************************

inline QList<QPointF> SensorLayout::loc() const
{
    return m_qListLocations;
}

#endif // SENSORLAYOUT_H
