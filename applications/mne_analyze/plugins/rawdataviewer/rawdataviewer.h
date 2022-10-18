//=============================================================================================================
/**
 * @file     rawdataviewer.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
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
#include <anShared/Plugins/abstractplugin.h>

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

namespace DISPLIB {
    class ScalingView;
    class FiffRawViewSettings;
}

namespace ANSHAREDLIB {
    class Communicator;
    class FiffRawViewModel;
    class AbstractModel;
}

//=============================================================================================================
// DEFINE NAMESPACE RAWDATAVIEWERPLUGIN
//=============================================================================================================

namespace RAWDATAVIEWERPLUGIN
{
    class FiffRawView;
    class FiffRawViewDelegate;

//=============================================================================================================
/**
 * RawDataViewer Plugin
 *
 * @brief The RawDataViewer class provides a view to display raw fiff data.
 */
class RAWDATAVIEWERSHARED_EXPORT RawDataViewer : public ANSHAREDLIB::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "rawdataviewer.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::AbstractPlugin)

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

    // AbstractPlugin functions
    virtual QSharedPointer<AbstractPlugin> clone() const override;
    virtual void init() override;
    virtual void unload() override;
    virtual QString getName() const override;
    virtual QMenu* getMenu() override;
    virtual QDockWidget* getControl() override;
    virtual QWidget* getView() override;
    virtual QString getBuildInfo() override;
    virtual void handleEvent(QSharedPointer<ANSHAREDLIB::Event> e) override;
    virtual QVector<ANSHAREDLIB::EVENT_TYPE> getEventSubscriptions() const override;

private:
    void onModelIsEmpty();

    //=========================================================================================================
    /**
     * Handles if a new model is present. Only works on FiffRawViewModels
     *
     * @param[in] pNewModel    The new model.
     */
    void onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel);

    //=========================================================================================================
    /**
     * Handles clearing view if currently used model is being removed
     *
     * @param[in] pRemovedModel    Pointer to model being removed.
     */
    void onModelRemoved(QSharedPointer<ANSHAREDLIB::AbstractModel> pRemovedModel);

    //=========================================================================================================
    /**
     * Handles if a new sample position should be dispatched
     *
     * @param[in] iSample    The sample to be send.
     */
    void sendNewEventSample(int iSample);

    //=========================================================================================================
    /**
     * Handles if a new sample position should be dispatched
     *
     * @param[in] iSample    The sample to be send.
     */
    void sendSampleSelected(int iSample);

    //=========================================================================================================
    /**
     * Updates the signal view parameters based on incoming pViewParameters object
     *
     * @param[in] pViewParameters  container for singal viewer display settings with new desired settings.
     */
    void updateViewParameters(ANSHAREDLIB::ViewParameters viewParameters);

    //=========================================================================================================
    void onNewRealtimeData();

    // Control
    QPointer<ANSHAREDLIB::Communicator>  m_pCommu;           /**< The communicator object to communicate with other plugins. */

    // Model
    int                    m_iSamplesPerBlock; /**< The samples per data block. Default is set to sampling frequency. */
    int                    m_iVisibleBlocks;   /**< The amount of visible data blocks. Default is set to 10. */
    int                    m_iBufferBlocks;    /**< The amount of buffered data blocks. Default is set to 10. */

    QPointer<FiffRawView>  m_pFiffRawView;     /**< View for Fiff data. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // RAWDATAVIEWER_H
