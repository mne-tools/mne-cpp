//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     spectrumsettingsview.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Lower / upper frequency-bound sliders for the FFT spectrum viewer.
 *
 * SpectrumSettingsView holds two @c QSliders that crop the frequency
 * axis of @ref SpectrumView; slider changes are emitted as
 * @c lowerFreqChanged / @c upperFreqChanged signals.
 */

#ifndef SPECTRUMSETTINGSVIEW_H
#define SPECTRUMSETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QSlider;

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief Lower / upper-bound frequency sliders for the FFT spectrum viewer.
 *
 * Two @c QSliders emit @c lowerFreqChanged / @c upperFreqChanged so
 * the connected @ref SpectrumView can crop the displayed frequency
 * axis.
 */
class DISPSHARED_EXPORT SpectrumSettingsView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<SpectrumSettingsView> SPtr;              /**< Shared pointer type for SpectrumSettingsView. */
    typedef QSharedPointer<const SpectrumSettingsView> ConstSPtr;   /**< Const shared pointer type for SpectrumSettingsView. */

    //=========================================================================================================
    /**
     * Constructs a SpectrumSettingsView which is a child of parent.
     *
     * @param[in] parent    parent of widget.
     */
    SpectrumSettingsView(const QString& sSettingsPath = "",
                         QWidget *parent = Q_NULLPTR,
                         Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the SpectrumSettingsView.
     */
    ~SpectrumSettingsView();

    //=========================================================================================================
    /**
     * Update slider value
     *
     * @param[in] value    slider value.
     */
    void updateValue(qint32 value);

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
     * Sets the sampling frequency as well as boundaries and configure the GUI elements accordingly
     *
     * @param[in] fSFreq           The sampling frequency.
     * @param[in] fLowerBound      The lower bound of the spectrum.
     * @param[in] fUpperBound      The upper bound of the spectrum.
     */
    void setBoundaries(float fSFreq,
                       float fLowerBound,
                       float fUpperBound);

    //=========================================================================================================
    /**
     * Returns the lower bound
     *
     * @return Returns the lower bound as a float.
     */
    float getLowerBound();

    //=========================================================================================================
    /**
     * Returns the upper bound
     *
     * @return Returns the upper bound as a float.
     */
    float getUpperBound();

    //=========================================================================================================
    /**
     * Clears the view
     */
    void clearView();

protected:
    //=========================================================================================================
    /**
     * Update the views GUI based on the set GuiMode (Clinical=0, Research=1).
     *
     * @param[in] mode     The new mode (Clinical=0, Research=1).
     */
    void updateGuiMode(GuiMode mode);

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param[in] mode     The new mode (RealTime=0, Offline=1).
     */
    void updateProcessingMode(ProcessingMode mode);

    QPointer<QSlider>   m_pSliderLowerBound;    /**< Lower bound frequency. */
    QPointer<QSlider>   m_pSliderUpperBound;    /**< Upper bound frequency. */

signals:
    //=========================================================================================================
    /**
     * Emitted whenever the settings changed and are ready to be retreived.
     */
    void settingsChanged();
};
} // NAMESPACE

#endif // SPECTRUMSETTINGSVIEW_H
