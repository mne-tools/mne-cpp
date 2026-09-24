//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     displaymanager.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    Declaration of the DisplayManager Class.
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
