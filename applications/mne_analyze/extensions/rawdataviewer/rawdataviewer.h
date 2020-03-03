//=============================================================================================================
/**
 * @file     rawdataviewer.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @version  dev
 * @date     October, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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
 * @brief    Contains the declaration of the RawDataViewer class.
 *
 */

#ifndef RAWDATAVIEWER_H
#define RAWDATAVIEWER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rawdataviewer_global.h"
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

class RawDataViewerControl;

namespace ANSHAREDLIB {
    class Communicator;
    class FiffRawViewModel;
}

//=============================================================================================================
// DEFINE NAMESPACE RAWDATAVIEWEREXTENSION
//=============================================================================================================

namespace RAWDATAVIEWEREXTENSION
{
    class FiffRawView;
    class FiffRawViewDelegate;

//=============================================================================================================
/**
 * RawDataViewer Extension
 *
 * @brief The RawDataViewer class provides a view to display raw fiff data.
 */
class RAWDATAVIEWERSHARED_EXPORT RawDataViewer : public ANSHAREDLIB::IExtension
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "rawdataviewer.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::IExtension)

public:
    //=========================================================================================================
    /**
     * Constructs a RawDataViewer.
     */
    RawDataViewer();

    //=========================================================================================================
    /**
     * Destroys the RawDataViewer.
     */
    virtual ~RawDataViewer();

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

    void onNewModelAvalible(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel);

private:
    // Control
    QPointer<QDockWidget>                m_pControlDock;                 /**< Control Widget */
    QPointer<RawDataViewerControl>       m_pRawDataViewerControl;
    QPointer<ANSHAREDLIB::Communicator>  m_pCommu;

    // Model
    int m_iSamplesPerBlock;
    int m_iVisibleBlocks;
    int m_iBufferBlocks;

    QSharedPointer<ANSHAREDLIB::FiffRawViewModel>   m_pRawModel;
    QSharedPointer<FiffRawViewDelegate>             m_pRawDelegate;

    QPointer<FiffRawView>                           m_pFiffRawView;     /**< View for Fiff data */
    //QPointer<QMdiSubWindow>                         m_pSubWindow;       /**< Window that wraps the display */
    QPointer<QWidget>                         m_pSubWindow;       /**< Window that wraps the display */
    bool                                            m_bDisplayCreated;  /**< Flag for remembering whether or not the display was already created */

    QVBoxLayout*            m_pLayout;              /**< Layout for widgets in the controller view */
    QWidget*                m_pContainer;           /**< Container for widgets in the controller view */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // RAWDATAVIEWER_H
