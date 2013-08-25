//=============================================================================================================
/**
* @file     newdisplaymanager.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2013
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
* @brief    Declaration of the NewDisplayManager Class.
*
*/

#ifndef NEWDISPLAYMANAGER_H
#define NEWDISPLAYMANAGER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_x_global.h"
#include "../Interfaces/IPlugin.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QTime>
#include <QHash>
#include <QWidget>
#include <QLabel>
#include <QString>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QVBoxLayout;
class QHBoxLayout;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEX
//=============================================================================================================

namespace MNEX
{

//=============================================================================================================
/**
* DECLARE CLASS NewDisplayManager
*
* @brief The NewDisplayManager class handles current displayed widgets.
*/
class MNE_X_SHARED_EXPORT NewDisplayManager : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<NewDisplayManager> SPtr;               /**< Shared pointer type for NewDisplayManager. */
    typedef QSharedPointer<const NewDisplayManager> ConstSPtr;    /**< Const shared pointer type for NewDisplayManager. */

    //=========================================================================================================
    /**
    * Constructs a NewDisplayManager.
    */
    NewDisplayManager(QSharedPointer<QTime> pT, QObject* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the NewDisplayManager.
    */
    virtual ~NewDisplayManager();

    //=========================================================================================================
    /**
    * Initialise the measurement widgets by calling there init() function.
    */
    void init();

    //=========================================================================================================
    /**
    * Shows a widget containing all current measurement widgets.
    *
    * @return a pointer to the widget containing all measurement widgets.
    */
    QWidget* show(IPlugin::OutputConnectorList &pOutputConnectorList);

    //=========================================================================================================
    /**
    * Cleans all measurement widget hash's.
    */
    void clean();

private:
    QSharedPointer<QTime> m_pT;

    QList<QMetaObject::Connection>   m_pListWidgetConnections;       /**< all widget connections.*/

};

} // NAMESPACE

#endif // NEWDISPLAYMANAGER_H
