//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     rawdataviewer.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2018
 * @brief    Contains the declaration of the RawDataViewer class.
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
    void onSendSamplePos(int iSample);

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
