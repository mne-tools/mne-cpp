//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     fiffrawviewsettings.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     November 2019
 * @brief    Display-style settings for the FIFF raw browser (zoom, time window, signal / background colours).
 *
 * FiffRawViewSettings centralises the visual knobs of
 * @ref RtFiffRawView / @ref ChannelDataView: visible time window in
 * seconds, distance-to-axis spinbox, trigger colour, background and
 * signal pen colour, plus a @ref ApplyToView selector that decides
 * whether the scaling change affects all / visible / selected
 * channels.
 */

#ifndef FIFFRAWVIEWSETTINGS_H
#define FIFFRAWVIEWSETTINGS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStringList>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class FiffRawViewSettingsWidget;
}

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
 * @brief Display-style settings panel for the FIFF raw browser (zoom, colours, distance, scope).
 *
 * Bundles the visible time window, signal / background / trigger
 * colours and the distance-to-axis spinbox with an embedded
 * @ref ApplyToView so the user controls whether changes affect
 * selected / visible / all channels.
 */
class DISPSHARED_EXPORT FiffRawViewSettings : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<FiffRawViewSettings> SPtr;              /**< Shared pointer type for FiffRawViewSettings. */
    typedef QSharedPointer<const FiffRawViewSettings> ConstSPtr;   /**< Const shared pointer type for FiffRawViewSettings. */

    //=========================================================================================================
    /**
     * Constructs a FiffRawViewSettings which is a child of parent.
     *
     * @param[in] parent        parent of widget.
     */
    FiffRawViewSettings(const QString& sSettingsPath = "",
                        QWidget *parent = 0,
                        Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the FiffRawViewSettings.
     */
    ~FiffRawViewSettings();

    //=========================================================================================================
    /**
     * Init the view.
     *
     * @param[in] lVisibleWidgets       The widgets to be visible: numberChannels, windowSize, distanceSpacers,.
     *                                   backgroundcolor, signalColor, screenshot
     */
    void setWidgetList(const QStringList &lVisibleWidgets = QStringList());

    //=========================================================================================================
    /**
     * Sets the values of the windowSize spin box
     *
     * @param[in] windowSize    new window size value.
     */
    void setWindowSize(int windowSize);

    //=========================================================================================================
    /**
     * Sets the values of the zoomFactor spin box
     *
     * @param[in] zoomFactor    new zoomFactor value.
     */
    void setZoom(double zoomFactor);

    //=========================================================================================================
    /**
     * Get current distance time spacer.
     *
     * @return the current distance time spacer.
     */
    int getDistanceTimeSpacer();

    //=========================================================================================================
    /**
     * Set current distance time spacer combo box.
     *
     * @param[in] value     the new value of the combo box.
     */
    void setDistanceTimeSpacer(int value);

    //=========================================================================================================
    /**
     * Set current  background color.
     *
     * @param[in] backgroundColor   The new background color.
     */
    void setBackgroundColor(const QColor& backgroundColor);

    //=========================================================================================================
    /**
     * Set current signal color.
     *
     * @param[in] signalColor       The new signal color.
     */
    void setSignalColor(const QColor& signalColor);

    //=========================================================================================================
    /**
     * Returns the current signal color.
     *
     * @return The current signal color.
     */
    const QColor& getSignalColor();

    //=========================================================================================================
    /**
     * Returns the current background color.
     *
     * @return The current background color.
     */
    const QColor& getBackgroundColor();

    //=========================================================================================================
    /**
     * Returns the current zoom.
     *
     * @return The current zoom.
     */
    double getZoom();

    //=========================================================================================================
    /**
     * Returns the current window size.
     *
     * @return The current window size.
     */
    int getWindowSize();

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

    //=========================================================================================================
    /**
     * Slot called when time window size changes
     */
    void onTimeWindowChanged(int value);

    //=========================================================================================================
    /**
     * Slot called when zoome changes
     */
    void onZoomChanged(double value);

    //=========================================================================================================
    /**
     * Slot called when time spacer distance changes
     *
     * @param[in] value for time spacer distance.
     */
    void onDistanceTimeSpacerChanged(qint32 value);

    //=========================================================================================================
    /**
     * Slot called when the user changes the signal or background color.
     */
    void onViewColorButtonClicked();

    //=========================================================================================================
    /**
     * Call this slot whenever you want to make a screenshot of the butterfly or layout view.
     */
    void onMakeScreenshot();

    Ui::FiffRawViewSettingsWidget* m_pUi;

    QColor      m_colCurrentSignalColor;        /**< Current color of the signal. */
    QColor      m_colCurrentBackgroundColor;    /**< Current color of the background. */
    QString     m_sSettingsPath;                /**< The settings path to store the GUI settings to. */

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever the user changes the window size.
     */
    void timeWindowChanged(int value);

    //=========================================================================================================
    /**
     * Emit this signal whenever the user changes the row height (zoom) of the channels.
     */
    void zoomChanged(double value);

    //=========================================================================================================
    /**
     * Emit this signal whenever the user changed the time spacer distance.
     */
    void distanceTimeSpacerChanged(int value);

    //=========================================================================================================
    /**
     * Emit this signal whenever the user changed the signal color.
     */
    void signalColorChanged(const QColor& signalColor);

    //=========================================================================================================
    /**
     * Emit this signal whenever the user changed the background color.
     */
    void backgroundColorChanged(const QColor& backgroundColor);

    //=========================================================================================================
    /**
     * Emit this signal whenever the user wants to make a screenshot.
     *
     * @param[out] imageType     The current image type: png, svg.
     */
    void makeScreenshot(const QString& imageType);
};
} // NAMESPACE

#endif // FIFFRAWVIEWSETTINGS_H
