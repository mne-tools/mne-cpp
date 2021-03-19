//=============================================================================================================
/**
 * @file     displaymanager.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh. All rights reserved.
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
 * @brief    Declaration of the DisplayManager Class.
 *
 */

#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../scshared_global.h"
#include "../Plugins/abstractplugin.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QTime>
#include <QHash>
#include <QWidget>
#include <QLabel>
#include <QString>
#include <QPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QVBoxLayout;
class QHBoxLayout;

namespace SCDISPLIB {
    class RealTime3DWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE SCSHAREDLIB
//=============================================================================================================

namespace SCSHAREDLIB
{

//=============================================================================================================
/**
 * DECLARE CLASS DisplayManager
 *
 * @brief The DisplayManager class handles current displayed widgets.
 */
class SCSHAREDSHARED_EXPORT DisplayManager : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<DisplayManager> SPtr;               /**< Shared pointer type for DisplayManager. */
    typedef QSharedPointer<const DisplayManager> ConstSPtr;    /**< Const shared pointer type for DisplayManager. */

    //=========================================================================================================
    /**
     * Constructs a DisplayManager.
     */
    DisplayManager(QObject* parent = 0);

    //=========================================================================================================
    /**
     * Destroys the DisplayManager.
     */
    virtual ~DisplayManager();

    //=========================================================================================================
    /**
     * Shows a widget containing all current measurement widgets.
     *
     * @param[in] outputConnectorList   output connector list.
     * @param[in] pT                    global timer.
     * @param[in, out] qListActions         a list of actions containing all measurent widget actions.
     *
     * @return a pointer to the widget containing all measurement widgets.
     */
    QWidget* show(AbstractPlugin::OutputConnectorList &outputConnectorList,
                  QSharedPointer<QTime>& pT,
                  QList<QAction*>& qListActions);

    //=========================================================================================================
    /**
     * Cleans all measurement widget hash's.
     */
    void clean();

private:
    QList<QMetaObject::Connection>              m_pListWidgetConnections;       /**< all widget connections.*/

    QPointer<SCDISPLIB::RealTime3DWidget>       m_pRealTime3DWidget;
};
} // NAMESPACE

#endif // DISPLAYMANAGER_H
