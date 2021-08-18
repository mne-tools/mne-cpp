//=============================================================================================================
/**
 * @file     controlmanager.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.5
 * @date     August, 2020
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
 * @brief    Contains the declaration of the ControlManager class.
 *
 */

#ifndef CONTROLMANAGER_H
#define CONTROLMANAGER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "controlmanager_global.h"
#include <anShared/Plugins/abstractplugin.h>
#include <utils/buildtime.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>
#include <QPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class Communicator;
}

namespace DISPLIB{
    class ApplyToView;
    class Control3DView;
}

#ifndef WASMBUILD
namespace DISP3DLIB {
    class Data3DTreeModel;
}
#endif

//=============================================================================================================
// DEFINE NAMESPACE SURFERPLUGIN
//=============================================================================================================

namespace CONTROLMANAGERPLUGIN
{

//=============================================================================================================
/**
 * ControlManager Plugin
 *
 * @brief The ControlManager class provides a view with all currently loaded models.
 */
class CONTROLMANAGERSHARED_EXPORT ControlManager : public ANSHAREDLIB::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "controlmanager.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::AbstractPlugin)

public:
    //=========================================================================================================
    /**
     * Constructs a ControlManager.
     */
    ControlManager();

    //=========================================================================================================
    /**
     * Destroys the ControlManager.
     */
    virtual ~ControlManager() override;

    // AbstractPlugin functions
    virtual QSharedPointer<AbstractPlugin> clone() const override;
    virtual void init() override;
    virtual void unload() override;
    virtual QString getName() const override;
    virtual QMenu* getMenu() override;
    virtual QDockWidget* getControl() override;
    virtual QWidget* getView() override;
    virtual QString getBuildDateTime() override;
    virtual void handleEvent(QSharedPointer<ANSHAREDLIB::Event> e) override;
    virtual QVector<ANSHAREDLIB::EVENT_TYPE> getEventSubscriptions() const override;

private:
    //=========================================================================================================
    /**
     * Receives scaling map scalingMap and sends an event to update views.
     *
     * @param[in] scalingMap   new scaling map to be sent out in an event.
     */
    void onScalingChanged(const QMap<qint32, float>& scalingMap);

    //=========================================================================================================
    /**
     * Sends new signal color via the event manager
     *
     * @param[in] signalColor   new signal color.
     */
    void onSignalColorChanged(const QColor& signalColor);

    //=========================================================================================================
    /**
     * Sends new backgroundColor via the event manager
     *
     * @param[in] backgroundColor  new background color.
     */
    void onBackgroundColorChanged(const QColor& backgroundColor);

    //=========================================================================================================
    /**
     * Sends new channel zoom value (number channels shown) via the event manager
     *
     * @param[in] value    new zoom value.
     */
    void onZoomChanged(double dZoomValue);

    //=========================================================================================================
    /**
     * Sends new time window value (number of seconds of data shown) via the event manager
     *
     * @param[in] value    new time window value.
     */
    void onTimeWindowChanged(int iTimeWindow);

    //=========================================================================================================
    /**
     * Sends new spacer distance parameters via the event manager
     *
     * @param[in] value    new time spacer distance value.
     */
    void onDistanceTimeSpacerChanged(int iSpacerDistance);

    //=========================================================================================================
    /**
     * Sends the parameters to take screenshot via event manager
     *
     * @param[in] imageType    screenshot image type.
     */
    void onMakeScreenshot(const QString& imageType);

    #ifndef WASMBUILD
    //=========================================================================================================
    /**
     * Sets 3D model for the 3D controls
     *
     * @param[in] pModel   new 3D Model.
     */
    void init3DGui(QSharedPointer<DISP3DLIB::Data3DTreeModel> pModel);
    #endif

    //=========================================================================================================
    /**
     * Sends scene color via the event manager
     *
     * @param[in] color    new scene color.
     */
    void onSceneColorChange(const QColor& color);

    //=========================================================================================================
    /**
     * Send rotation toggle via event manager
     *
     * @param[in] bRotationChanged     rotation toggle.
     */
    void onRotationChanged(bool bRotationChanged);

    //=========================================================================================================
    /**
     * Send coordinate axis toggle via event manager
     *
     * @param[in] bShowCoordAxis   toggle coord axis.
     */
    void onShowCoordAxis(bool bShowCoordAxis);

    //=========================================================================================================
    /**
     * Send fullscreen toggle via event manager
     *
     * @param[in] bShowFullScreen      toggle fullscreen.
     */
    void onShowFullScreen(bool bShowFullScreen);

    //=========================================================================================================
    /**
     * Send light color via the event manager
     *
     * @param[in] color    new light color.
     */
    void onLightColorChanged(const QColor& color);

    //=========================================================================================================
    /**
     * Send light intensity via the event manager
     *
     * @param[in] value    new light intensity.
     */
    void onLightIntensityChanged(double value);

    //=========================================================================================================
    /**
     * Send screenshot toggle via event manager
     */
    void onTakeScreenshotChanged();

    QPointer<ANSHAREDLIB::Communicator>         m_pCommu;                   /**< Communicator to send events trhoug hevent manager. */

    #ifndef WASMBUILD
    DISPLIB::Control3DView*                     m_pControl3DView;           /**< Controls for 3d View. */
    #endif

    DISPLIB::ApplyToView*                       m_pApplyToView;             /**< Controls for selecting views to apply settings. */
    ANSHAREDLIB::ScalingParameters              m_ScalingParameters;        /**< Controls for scaling. */
    ANSHAREDLIB::ViewParameters                 m_ViewParameters;           /**< Struct for 2D view settings. */
    ANSHAREDLIB::View3DParameters               m_View3DParameters;         /**< Struct for 3D view settings. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // CONTROLMANAGER_H
