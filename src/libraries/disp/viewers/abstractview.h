//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file abstractview.h
 * @since June 2020
 * @brief Common base widget for every dockable Quick-Control settings panel in DISPLIB.
 *
 * AbstractView is the shared QWidget super-class used by all the
 * viewers in this directory. It standardises the three runtime modes
 * every panel must react to — @c StyleMode (Default / Dark),
 * @c GuiMode (Clinical / Research) and @c ProcessingMode (RealTime /
 * Offline) — and exposes the @c QSettings persistence helpers
 * (@c saveSettings / @c loadSettings) so derived views only have to
 * implement their own state serialisation. Concrete panels override
 * the protected virtuals to repaint themselves when the active mode
 * changes.
 */

#ifndef ABSTRACTVIEW_H
#define ABSTRACTVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

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
 * @brief Base widget for every dockable settings panel in DISPLIB; centralises GUI / processing mode and settings persistence.
 *
 * Subclasses override @c updateGuiMode, @c updateProcessingMode and
 * @c saveSettings / @c loadSettings to react to mode changes and to
 * round-trip their state through @c QSettings. The class deliberately
 * exposes no domain-specific API.
 */
class DISPSHARED_EXPORT AbstractView : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<AbstractView> SPtr;              /**< Shared pointer type for AbstractView. */
    typedef QSharedPointer<const AbstractView> ConstSPtr;   /**< Const shared pointer type for AbstractView. */

    enum StyleMode {
        Default,
        Dark
    };

    enum GuiMode {
        Clinical,
        Research
    };

    enum ProcessingMode {
        RealTime,
        Offline
    };

    //=========================================================================================================
    /**
     * Constructs a AbstractView which is a child of parent.
     *
     * @param[in] parent        parent of widget.
     */
    AbstractView(QWidget *parent = 0,
                 Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Sets the GUI of this view to a specific mode. Clinical = 0, Research = 0.
     *
     * @param mode     The new mode.
     */
    virtual void setGuiMode(GuiMode mode);

    //=========================================================================================================
    /**
     * Sets the GUI of this view to a specific mode. RealTime = 0, Offline = 0.
     *
     * @param mode     The new mode.
     */
    virtual void setProcessingMode(ProcessingMode mode);

    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     */
    virtual void saveSettings() = 0;

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     */
    virtual void loadSettings() = 0;

    //=========================================================================================================
    /**
     * Update the views GUI based on the set GuiMode (Clinical=0, Research=1).
     *
     * @param mode     The new mode (Clinical=0, Research=1).
     */
    virtual void updateGuiMode(GuiMode mode) = 0;

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param mode     The new mode (RealTime=0, Offline=1).
     */
    virtual void updateProcessingMode(ProcessingMode mode) = 0;

    //=========================================================================================================
    /**
     * Clears the view
     */
    virtual void clearView() = 0;

signals:
    //=========================================================================================================
    /**
     * Signal emited whenever the gui style mode changes.
     */
    void guiStyleChanged(DISPLIB::AbstractView::StyleMode style);

protected:

    bool            m_bResearchModeIsActive;  /**< The flag describing whether the research mode of the view is active or not. */
    bool            m_bOfflineModeIsActive;  /**< The flag describing whether offline mode of the view is active or not. */

    QString         m_sSettingsPath;            /**< The settings path to store the GUI settings to. */

};
} // NAMESPACE

#endif // ABSTRACTVIEW_H
