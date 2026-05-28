//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     compensatorview.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Gradient-compensation grade selector for Neuromag / CTF systems.
 *
 * CompensatorView lists every compensation matrix advertised in the
 * active @c FiffInfo and exposes them as mutually-exclusive
 * @c QCheckBoxes; switching the active grade emits a signal that the
 * real-time pipeline uses to apply (or undo) the corresponding
 * balance/derivation matrix on the live stream.
 */

#ifndef COMPENSATORVIEW_H
#define COMPENSATORVIEW_H

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

class QCheckBox;

namespace FIFFLIB {
    class FiffCtfComp;
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
 * @brief Mutually-exclusive checkbox list picking the active MEG gradient compensation grade.
 *
 * One checkbox per compensation matrix advertised in the active
 * @c FiffInfo; toggling emits @c compSelectionChanged with the new
 * grade so the live pipeline can (un)apply the matrix.
 */
class DISPSHARED_EXPORT CompensatorView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<CompensatorView> SPtr;              /**< Shared pointer type for CompensatorView. */
    typedef QSharedPointer<const CompensatorView> ConstSPtr;   /**< Const shared pointer type for CompensatorView. */

    //=========================================================================================================
    /**
     * Constructs a CompensatorView which is a child of parent.
     *
     * @param[in] parent        parent of widget.
     */
    CompensatorView(const QString& sSettingsPath = "",
                    QWidget *parent = 0,
                    Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the CompensatorView.
     */
    ~CompensatorView();

    //=========================================================================================================
    /**
     * Get the current compensators
     *
     * @return The current compensators.
     */
    QList<FIFFLIB::FiffCtfComp> getCompensators() const;

    //=========================================================================================================
    /**
     * Get the last value of comp.to
     *
     * @return The last value of comp.to.
     */
    int getLastTo() const;

    //=========================================================================================================
    /**
     * Set the current compensators
     *
     * @param[in] comps    The new compensators.
     */
    void setCompensators(const QList<FIFFLIB::FiffCtfComp>& comps);

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
     * Clears the view
     */
    void clearView();

protected:
    //=========================================================================================================
    /**
     * Redraw the selection.
     */
    void redrawGUI();

    //=========================================================================================================
    /**
     * Slot called when the compensator check state changes
     */
    void onCheckCompStatusChanged();

    QList<QCheckBox*>                                   m_qListCompCheckBox;            /**< List of compensator CheckBox. */

    QList<FIFFLIB::FiffCtfComp>                         m_pComps;                       /**< The current compensators. */

    QString                                             m_sSettingsPath;                /**< The settings path to store the GUI settings to. */

    int                                                 m_iLastTo;

    QMap<int,bool>                                      m_mapCompActive;

signals:
    //=========================================================================================================
    /**
     * Signal mapper signal for compensator changes.
     */
    void compClicked(const QString& text);

    //=========================================================================================================
    /**
     * Emit this signal whenever the user changes the compensator.
     */
    void compSelectionChanged(int to);
};
} // NAMESPACE

#endif // COMPENSATORVIEW_H
