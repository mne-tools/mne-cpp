//=============================================================================================================
/**
 * @file     view3d.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.6
 * @date     August, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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
 * @brief    View3D class declaration.
 *
 */

#ifndef MNEANALYZE_VIEW3D_H
#define MNEANALYZE_VIEW3D_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "view3d_global.h"

#include <anShared/Interfaces/IPlugin.h>

#include <rtprocessing/helpers/filterkernel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class Communicator;
    class BemDataModel;
}

namespace DISP3DLIB {
    class View3D;
    class Data3DTreeModel;
    class BemTreeItem;
    class DigitizerSetTreeItem;
}

namespace DISPLIB {
    class Control3DView;
}

//=============================================================================================================
// DEFINE NAMESPACE VIEW3DPLUGIN
//=============================================================================================================

namespace VIEW3DPLUGIN
{

//=============================================================================================================
// View3DPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * View3D Plugin
 *
 * @brief The View3D class provides a plugin for visualizing information in 3D.
 */
class VIEW3DSHARED_EXPORT View3D : public ANSHAREDLIB::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "view3d.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::IPlugin)

public:
    //=========================================================================================================
    /**
     * Constructs an View3D object.
     */
    View3D();

    //=========================================================================================================
    /**
     * Destroys the View3D object.
     */
    ~View3D() override;

    // IPlugin functions
    virtual QSharedPointer<IPlugin> clone() const override;
    virtual void init() override;
    virtual void unload() override;
    virtual QString getName() const override;

    virtual QMenu* getMenu() override;
    virtual QDockWidget* getControl() override;
    virtual QWidget* getView() override;

    virtual void handleEvent(QSharedPointer<ANSHAREDLIB::Event> e) override;
    virtual QVector<ANSHAREDLIB::EVENT_TYPE> getEventSubscriptions() const override;

private:
    //=========================================================================================================
    /**
     * Updates the davailable Bem Models
     */
    void updateCoregBem(QSharedPointer<ANSHAREDLIB::BemDataModel> pNewModel);

    QPointer<ANSHAREDLIB::Communicator>             m_pCommu;               /**< To broadcst signals */

    QSharedPointer<DISP3DLIB::Data3DTreeModel>      m_p3DModel;             /**< The 3D model data */
    DISP3DLIB::BemTreeItem*                         m_pBemTreeCoreg;        /**< TThe BEM head model of the coregistration plugin. */
    QPointer<DISP3DLIB::DigitizerSetTreeItem>       m_pDigitizerCoreg;      /**< The 3D item pointing to the tracked digitizers. */

    DISP3DLIB::View3D*                              m_pView3D;              /**< The Disp3D view. */
    DISPLIB::Control3DView*                         m_pControl3DView;       /**< The 3D Control view */

};

} // NAMESPACE

#endif // MNEANALYZE_VIEW3D_H

