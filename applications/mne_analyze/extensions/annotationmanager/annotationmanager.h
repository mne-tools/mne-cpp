//=============================================================================================================
/**
 * @file     annotationmanager.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     November, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the annotationmanager class.
 *
 */

#ifndef ANNOTATIONMANAGER_H
#define ANNOTATIONMANAGER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "annotationmanager_global.h"
#include "annotationdelegate.h"
#include "annotationview.h"

#include <anShared/Interfaces/IExtension.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class Communicator;
}

//=============================================================================================================
// DEFINE NAMESPACE annotationmanagerEXTENSION
//=============================================================================================================

namespace ANNOTATIONMANAGEREXTENSION
{

//=============================================================================================================
// ANNOTATIONMANAGEREXTENSION FORWARD DECLARATIONS
//=============================================================================================================

class annotationmanagerControl;

//=============================================================================================================
/**
 * annotationmanager Extension
 *
 * @brief The annotationmanager class provides input and output capabilities for the fiff file format.
 */
class ANNOTATIONMANAGERSHARED_EXPORT AnnotationManager : public ANSHAREDLIB::IExtension
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "annotationmanager.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::IExtension)

public:
    //=========================================================================================================
    /**
     * Constructs an AnnotationManager.
     */
    AnnotationManager();

    //=========================================================================================================
    /**
     * Destroys the AnnotationManager.
     */
    ~AnnotationManager() override;

    // IExtension functions
    virtual QSharedPointer<IExtension> clone() const override;
    virtual void init() override;
    virtual void unload() override;
    virtual QString getName() const override;

    virtual QMenu* getMenu() override;
    virtual QDockWidget* getControl() override;
    virtual QWidget* getView() override;

    virtual void handleEvent(QSharedPointer<ANSHAREDLIB::Event> e) override;
    virtual QVector<ANSHAREDLIB::EVENT_TYPE> getEventSubscriptions() const override;

    //=========================================================================================================
    void onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel);

    //=========================================================================================================
    void setUpControls();

    //=========================================================================================================
    void toggleDisplayEvent(const int& iToggle);

    //=============================================================================================================
    void onTriggerRedraw();

private:

    QPointer<QDockWidget>               m_pControl;             /**< Control Widget */

    QPointer<ANSHAREDLIB::Communicator> m_pCommu;

    QSharedPointer<AnnotationView>                  m_pAnnotationView;
    QSharedPointer<ANSHAREDLIB::AnnotationModel>    m_pAnnotationModel;

    QSharedPointer<ANSHAREDLIB::FiffRawViewModel>   m_pFiffRawModel;

    QString                             m_sCurrentlySelectedModel;

    int m_iToggle;

};

} // NAMESPACE

#endif // ANNOTATIONMANAGER_H
