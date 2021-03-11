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

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QDoubleSpinBox;
class QSlider;

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

//=========================================================================================================
/**
 * Get the default scaling values by channel type
 *
 * @param [in] iChannelKind     The channel kind to return the default scaling value for.
 * @param [in] iChannelUnit     The channel unit to return the default scaling value for.
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
 * @param [in] qMapChScaling    The map containing the scaling values for different channel types and units.
 * @param [in] iChannelKind     The channel kind to return the default scaling value for.
 * @param [in] iChannelUnit     The channel unit to return the default scaling value for.
 *                              The unit is only important when dealing with MEG channels in order to
 *                              distinguish between magnetometers and gradiometers.
 *
 * @return The scaling value as a float.
 */
DISPSHARED_EXPORT float getScalingValue(const QMap<qint32, float>& qMapChScaling,
                                        int iChannelKind,
                                        int iChannelUnit);

//=============================================================================================================

class DISPSHARED_EXPORT ScaleControl
{
    ScaleControl();


};


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
     * @param [in] parent        parent of widget
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
    void saveSettings();

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     */
    void loadSettings();

    //=========================================================================================================
    /**
     * Clears the view
     */
    void clearView();

    //=========================================================================================================
    /**
     * Slot for processing the use of shortcut keypresses.
     * @param event
     */
    void keyReleaseEvent(QKeyEvent* event);

    //=========================================================================================================
    /**
     * Slot for processing the use of shortcut releases.
     * @param event
     */
    void keyPressEvent(QKeyEvent* event);

    //=========================================================================================================
    void MAGScaleSpinBoxChanged(double value);

    //=========================================================================================================
    void GRADScaleSpinBoxChanged(double value);

    //=========================================================================================================
    void EEGScaleSpinBoxChanged(double value);

    //=========================================================================================================
    void EOGScaleSpinBoxChanged(double value);

    //=========================================================================================================
    void EMGScaleSpinBoxChanged(double value);

    //=========================================================================================================
    void ECGScaleSpinBoxChanged(double value);

    //=========================================================================================================
    void MISCScaleSpinBoxChanged(double value);

    //=========================================================================================================
    void STIMScaleSpinBoxChanged(double value);

    //=============================================================================================================
    void MAGScaleSliderChanged(int value);

    //=============================================================================================================
    void GRADScaleSliderChanged(int value);

    //=============================================================================================================
    void EEGScaleSliderChanged(int value);

    //=============================================================================================================
    void EOGScaleSliderChanged(int value);

    //=============================================================================================================
    void EMGScaleSliderChanged(int value);

    //=============================================================================================================
    void ECGScaleSliderChanged(int value);

    //=============================================================================================================
    void MISCScaleSliderChanged(int value);

    //=============================================================================================================
    void STIMScaleSliderChanged(int value);

    //=============================================================================================================
    void emitScalingChangedAndSaveSettings();

protected:
    //=========================================================================================================
    /**
     * Update the views GUI based on the set GuiMode (Clinical=0, Research=1).
     *
     * @param mode     The new mode (Clinical=0, Research=1).
     */
    void updateGuiMode(GuiMode mode);

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param mode     The new mode (RealTime=0, Offline=1).
     */
    void updateProcessingMode(ProcessingMode mode);

    //=========================================================================================================
    /**
     * Redraw the GUI.
     */
    void redrawGUI();

    //=========================================================================================================
    /**
     * Slot called when scaling spin boxes change
     */
//    void onUpdateSpinBoxScaling(double value);

    //=========================================================================================================
    /**
     * Slot called when slider scaling change
     */
//    void onUpdateSliderScaling(int value);

    QMap<qint32, float>                 m_qMapChScaling;                /**< Channel scaling values. */
    QMap<qint32, QDoubleSpinBox*>       m_qMapScalingDoubleSpinBox;     /**< Map of types and channel scaling line edits. */
    QMap<qint32, QSlider*>              m_qMapScalingSlider;            /**< Map of types and channel scaling line edits. */

    QString                             m_sSettingsPath;                /**< The settings path to store the GUI settings to. */

    QStringList                         m_lChannelTypesToShow;          /**< The channel types as strings to show the sliders for. */

    Ui::ScalingViewWidget*              m_pUi;                          /**< Pointer to the user interface object. */
    bool                                m_bIsShiftKeyPressed;           /**< Bool member value to store the use of the shiftkey. */
    bool                                m_bManagingSpinBoxChange;
    bool                                m_bManagingSliderChange;
signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever the scaling sliders or spin boxes changed.
     */
    void scalingChanged(const QMap<qint32, float>& scalingMap);
};
} // NAMESPACE

#endif // SCALINGVIEW_H
