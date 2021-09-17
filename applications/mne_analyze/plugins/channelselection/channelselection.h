//=============================================================================================================
/**
 * @file     channelselection.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu
 * @since    0.1.5
 * @date     July, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Gabriel Motta. All rights reserved.
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
 * @brief    Contains the declaration of the ChannelSelection class.
 *
 */

#ifndef CHANNELSELECTION_H
#define CHANNELSELECTION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channelselection_global.h"

#include <anShared/Plugins/abstractplugin.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>
#include <QPointer>
#include <QHBoxLayout>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class Communicator;
    class AbstractModel;
}

namespace DISPLIB {
    class ChannelSelectionView;
    class ChannelInfoModel;
    class SelectionItem;
    class ApplyToView;
}

namespace FIFFLIB {
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE SURFERPLUGIN
//=============================================================================================================

namespace CHANNELSELECTIONPLUGIN
{

//=============================================================================================================
/**
 * channelselection Plugin
 *
 * @brief The channelselection class provides a view with all currently loaded models.
 */
class CHANNELSELECTIONSHARED_EXPORT ChannelSelection : public ANSHAREDLIB::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "channelselection.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::AbstractPlugin)

public:
    //=========================================================================================================
    /**
     * Constructs a channelselection.
     */
    ChannelSelection();

    //=========================================================================================================
    /**
     * Destroys the channelselection.
     */
    virtual ~ChannelSelection() override;

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

private slots:
    //=========================================================================================================
    /**
     * Receives a list of graphic itemms, stores aparmeters locally and sends them to event manager
     *
     * @param[in] selectedChannelItems     List of parameters of graphics items.
     */
    void onSelectionChanged(const QList<QGraphicsItem*>& selectedChannelItems);

private:
    //=========================================================================================================
    /**
     * Receives new model and updates stored FiffInfo
     *
     * @param[in] pNewModel    new loaded model.
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
     * Sets up channel info model and Channel selection view / subsequently updates with new fiff info
     *
     * @param[in] pFiffInfo    FiffInfo for currently loaded file.
     */
    void setFiffSettings(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    QPointer<ANSHAREDLIB::Communicator>                 m_pCommu;                   /**< To broadcast signals. */

    DISPLIB::SelectionItem*                             m_pSelectionItem;           /**< Stores parameters from list of QGraphicsItems received by channelselectionview. */

    DISPLIB::ChannelSelectionView*                      m_pChannelSelectionView;    /**< View for selecting channels to be displayed. */
    QSharedPointer<DISPLIB::ChannelInfoModel>           m_pChannelInfoModel;        /**< Hold channel info - needed to initialize channelselectionview. */
    QSharedPointer<FIFFLIB::FiffInfo>                   m_pFiffInfo;                /**< Hold Information baout currently loaded file. */

    DISPLIB::ApplyToView*                               m_pApplyToView;

    QHBoxLayout*                                        m_pViewLayout;              /**< Holds the view portion of the channel selection to be displayed. */
    QVBoxLayout*                                        m_pControlLayout;           /**< Holds the control portion of the channel selection to be displayed. */

    bool                                                m_bIsInit;                  /**< Whether channelselectionview and channelinfomodel have been initialized. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // CHANNELSELECTION_H
