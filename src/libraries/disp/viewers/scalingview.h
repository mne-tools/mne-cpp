//=============================================================================================================
/**
 * @file     scalingview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief    Declaration of the ScalingView Class.
 *
 */

#ifndef SCALINGVIEW_H
#define SCALINGVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMap>
#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QCheckBox;
namespace Ui {
    class ScalingViewWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================
class ScaleControl;

//=========================================================================================================
/**
 * Get the default scaling values by channel type
 *
 * @param[in] iChannelKind     The channel kind to return the default scaling value for.
 * @param[in] iChannelUnit     The channel unit to return the default scaling value for.
 *                              The unit is only important when dealing with MEG channels in order to
 *                              distinguish between magnetometers and gradiometers.
 *
 * @return The defaults scaling value.
 */
DISPSHARED_EXPORT float getDefaultScalingValue(int iChannelKind,
                                               int iChannelUnit);

//=========================================================================================================
/**
 * Get the scaling value from suplied scale map, and check if there is a float asigned to the scale.
 * Otherwise returns default scaling values for the channel type.
 *
 * @param[in] qMapChScaling    The map containing the scaling values for different channel types and units.
 * @param[in] iChannelKind     The channel kind to return the default scaling value for.
 * @param[in] iChannelUnit     The channel unit to return the default scaling value for.
 *                              The unit is only important when dealing with MEG channels in order to
 *                              distinguish between magnetometers and gradiometers.
 *
 * @return The scaling value as a float.
 */
DISPSHARED_EXPORT float getScalingValue(const QMap<qint32, float>& qMapChScaling,
                                        int iChannelKind,
                                        int iChannelUnit);


//=============================================================================================================
/**
 * DECLARE CLASS ScalingView
 *
 * @brief The ScalingView class provides a view to select the scaling for different channels modalities
 */
class DISPSHARED_EXPORT ScalingView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<ScalingView> SPtr;              /**< Shared pointer type for ScalingView. */
    typedef QSharedPointer<const ScalingView> ConstSPtr;   /**< Const shared pointer type for ScalingView. */

    //=========================================================================================================
    /**
     * Constructs a ScalingView which is a child of parent.
     *
     * @param[in] parent Parent of widget.
     */
    ScalingView(const QString& sSettingsPath = "",
                QWidget *parent = 0,
                Qt::WindowFlags f = Qt::Widget,
                const QStringList& lChannelsToShow = QStringList() << "all");

    //=========================================================================================================
    /**
     * Destroys the ScalingView.
     */
    ~ScalingView();

    //=========================================================================================================
    /**
     * Get the current scaling map
     *
     * @return The current scaling map.
     */
    QMap<qint32,float> getScaleMap() const;

    //=========================================================================================================
    /**
     * Set the current scaling map. This also recreates the GUI.
     */
    void setScaleMap(const QMap<qint32, float> &qMapChScaling);

    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     */
    void saveSettings() override;

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     */
    void loadSettings() override;

    //=========================================================================================================
    /**
     * Clears the view
     */
    void clearView() override;

    //=========================================================================================================
    /**
     * Link the Magnetometers to the Gradiometers scale.
     *
     * @param[in] l Set the link between MAGs and GRADs active or inactive.
     */
    void setMagGradLink(int l);

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever the scaling sliders or spin boxes changed.
     *
     * @param[in] scalingMap this is a list of scaling values ordered by integer keys. These keys are resolved with
     * the use of defined macros with names of channel-types, vendors, etc.
     */
    void scalingChanged(const QMap<qint32, float>& scalingMap);

private:

    //=============================================================================================================
    /**
     * Emit signal to save scale status and update views.
     */
    void processScalingChange();

    //=============================================================================================================
    /**
     * Renders visible the control object that sets the link between Magnetometers and Gradiometers used in the controlView.
     */
    void showLinkControl();

    //=============================================================================================================
    /**
     * Callback to process a change in the MAGs scale spinbox.
     *
     * @param[in] dScale
     */
    void updateMAGScale(double dScale);

    //=========================================================================================================
    /**
     * Callback to process a change in the gradiometers scale spinbox.
     *
     * @param[in] dScale The new Scale.
     */
    void updateGRADScale(double dScale);

    //=========================================================================================================
    /**
     * Link Magnetometers and Gradiometers through this ratio. Just for viewing purposes.
     *
     * @param[in] dScale The new Scale.
     */
    void updateMAGtoGRADlink(double dScale);

    //=========================================================================================================
    /**
     * Callback to process a change in the EEG scale spinbox.
     *
     * @param[in] dScale The new Scale.
     */
    void updateEEGScale(double dScale);

    //=========================================================================================================
    /**
     * Callback to process a change in the EOG scale spinbox.
     *
     * @param[in] dScale The new Scale.
     */
    void updateEOGScale(double dScale);

    //=========================================================================================================
    /**
     * Callback to process a change in the EMG scale spinbox.
     *
     * @param[in] dScale The new Scale.
     */
    void updateEMGScale(double dScale);

    //=========================================================================================================
    /**
     * Callback to process a change in the ECG scale spinbox.
     *
     * @param[in] dScale The new Scale.
     */
    void updateECGScale(double dScale);

    //=========================================================================================================
    /**
     * Callback to process a change in the MISC scale spinbox.
     *
     * @param[in] dScale The new Scale.
     */
    void updateMISCScale(double dScale);

    //=========================================================================================================
    /**
     * Callback to process a change in the STIM scale spinbox.
     *
     * @param[in] dScale The new Scale.
     */
    void updateSTIMScale(double dScale);

    //=========================================================================================================
    /**
     * Update the views GUI based on the set GuiMode (Clinical=0, Research=1).
     *
     * @param[in] mode The new mode (Clinical=0, Research=1).
     */
    void updateGuiMode(GuiMode mode) override;

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param[in] mode The new mode (RealTime=0, Offline=1).
     */
    void updateProcessingMode(ProcessingMode mode) override;

    //=========================================================================================================
    /**
     * Create the controls.
     */
    void createScaleControls();

    //=========================================================================================================
    /**
     * Create the controls.
     */
    void drawScalingGUI();

    //=========================================================================================================
    /**
     * Link Gradiometers scale with magnetometers.
     */
    void linkMagToGrad();

    //=========================================================================================================
    /**
     * Link Gradiometers scale with magnetometers.
     */
    void linkGradToMag();

    //=========================================================================================================
    /**
     * Create the checkbox for enabling or disabling the link between Magnetometers and Gradiometers.
     */
    void createLinkMagGradCheckBox();

    //=========================================================================================================
    /**
     * Slot for processing the use of shortcut keypresses.
     *
     * @param[in] event An keyboard-release event to be processed.
     */
    void keyReleaseEvent(QKeyEvent* event) override;

    //=========================================================================================================
    /**
     * Slot for processing the use of shortcut releases.
     *
     * @param[in] event A keyboard-press event to be processed.
     */
    void keyPressEvent(QKeyEvent* event) override;

    QMap<qint32, float>                     m_qMapChScaling;                /**< Channel scaling values. */
    QMap<qint32, QPointer<ScaleControl> >   m_qMapScaleControls;            /**< Map of channel scaling controls. */

    QString                                 m_sSettingsPath;                /**< The settings path to store the GUI settings to. */

    QStringList                             m_lChannelTypesToShow;          /**< The channel types as strings to show the sliders for. */

    Ui::ScalingViewWidget*                  m_pUi;                          /**< Pointer to the user interface object. */
    bool                                    m_bLinkMAGtoGRAD;               /**< If this member is set, we link MAGs and GRad scales. */
    bool                                    m_bIsShiftKeyPressed;           /**< Bool member value to store the use of the shiftkey. */
    bool                                    m_bManagingSpinBoxChange;       /**< Bool member mutex the state of the spinbox. */
    bool                                    m_bManagingSliderChange;        /**< Bool member mutex the state of the slider. */
    bool                                    m_bManagingLinkMagToGrad;       /**< Bool member mutex the link between MAGs and GRADs. */
    QPointer<QCheckBox>                     m_pCheckBox;                    /**< Stores the conversion ratio between MAGs and GRADs. */
};

} // NAMESPACE

#endif // SCALINGVIEW_H
