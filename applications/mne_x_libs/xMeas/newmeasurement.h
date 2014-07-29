//=============================================================================================================
/**
* @file     newmeasurement.h
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
* @brief    Contains the declaration of the NewMeasurement class.
*
*/

#ifndef NEWMEASUREMENT_H
#define NEWMEASUREMENT_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "xmeas_global.h"



//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XMEASLIB
//=============================================================================================================

namespace XMEASLIB
{

class XMEASSHARED_EXPORT NewMeasurement : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<NewMeasurement> SPtr;               /**< Shared pointer type for NewMeasurement. */
    typedef QSharedPointer<const NewMeasurement> ConstSPtr;    /**< Const shared pointer type for NewMeasurement. */

    //=========================================================================================================
    /**
    * Constructs a Measurement.
    *
    * @param[in] type       the QMetaType id of the Measurement.
    * @param[in] parent     the parent object
    */
    explicit NewMeasurement(int type = QMetaType::UnknownType, QObject *parent = 0);

    //=========================================================================================================
    /**
    * Constructs the Measurement.
    */
    virtual ~NewMeasurement();

    //=========================================================================================================
    /**
    * Returns the name of the Measurement.
    *
    * @return the name of the Measurement.
    */
    inline const QString& getName() const;

    //=========================================================================================================
    /**
    * Sets the name of the Measurement.
    *
    * @param[in] name which should be set.
    */
    inline void setName(const QString& name);

    //=========================================================================================================
    /**
    * Returns whether Measurement is visible.
    *
    * @return true if Measurement is visible, otherwise false.
    */
    inline bool isVisible() const;

    //=========================================================================================================
    /**
    * Sets the visibility of the Measurement, whether Measurement is visible at the display or just data are send invisible.
    *
    * @param [in] visibility of the Measurement.
    */
    inline void setVisibility(bool visibility);

    //=========================================================================================================
    /**
    * Returns the type of the Measurement.
    *
    * @return the type of the Measurement.
    */
    inline int type() const;

signals:
    void notify();

protected:
    //=========================================================================================================
    /**
    * Sets the type of the Measurement. Use QMetaType::type("the type") to generate the type.
    *
    * @param[in] type   the QMetaType id of the Measurement.
    */
    inline void setType(int type);

private:
    int     m_iMetaTypeId;      /**< QMetaType id of the Measurement */
    QString m_qString_Name;     /**< Name of the Measurement */
    bool    m_bVisibility;      /**< Visibility status */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const QString& NewMeasurement::getName() const
{
    return m_qString_Name;
}


//*************************************************************************************************************

inline void NewMeasurement::setType(int type)
{
    m_iMetaTypeId = type;
}


//*************************************************************************************************************

inline void NewMeasurement::setName(const QString& name)
{
    m_qString_Name = name;
}


//*************************************************************************************************************

inline bool NewMeasurement::isVisible() const
{
    return m_bVisibility;
}


//*************************************************************************************************************

inline void NewMeasurement::setVisibility(bool visibility)
{
    m_bVisibility = visibility;
}


//*************************************************************************************************************

inline int NewMeasurement::type() const
{
    return m_iMetaTypeId;
}

} //NAMESPACE

Q_DECLARE_METATYPE(XMEASLIB::NewMeasurement::SPtr)

#endif // NEWMEASUREMENT_H
