//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     averageselectionview.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Tabular picker for choosing which averaged conditions are visible and their plot colours.
 *
 * AverageSelectionView shows a row per evoked condition (active flag,
 * label, colour swatch) and re-emits the resulting visibility +
 * colour map so any plot listening to it — typically
 * @ref ButterflyView and @ref AverageLayoutView — can update its
 * rendering accordingly.
 */

#ifndef AVERAGESELECTIONVIEW_H
#define AVERAGESELECTIONVIEW_H

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
 * @brief Tabular picker for active / colour of each averaged condition in the current evoked set.
 *
 * Emits @c newAverageActivationMap and @c newAverageColorMap whenever
 * the user toggles a row or changes a colour; consumers refresh
 * their evoked plots accordingly.
 */
class DISPSHARED_EXPORT AverageSelectionView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<AverageSelectionView> SPtr;              /**< Shared pointer type for AverageSelectionView. */
    typedef QSharedPointer<const AverageSelectionView> ConstSPtr;   /**< Const shared pointer type for AverageSelectionView. */

    //=========================================================================================================
    /**
     * Constructs a AverageSelectionView which is a child of parent.
     *
     * @param[in] parent        parent of widget.
     */
    AverageSelectionView(const QString &sSettingsPath="",
                         QWidget *parent = 0,
                         Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the AverageSelectionView.
     */
    ~AverageSelectionView();

    //=========================================================================================================
    /**
     * Get the current average colors
     *
     * @return Pointer to the current average colors.
     */
    QSharedPointer<QMap<QString, QColor> > getAverageColor() const;

    //=========================================================================================================
    /**
     * Get the current average activations
     *
     * @return Pointer to the current average activations.
     */
    QSharedPointer<QMap<QString, bool> > getAverageActivation() const;

    //=========================================================================================================
    /**
     * Set the average colors
     *
     * @param[in] qMapAverageColor      Pointer to the new average colors.
     */
    void setAverageColor(const QSharedPointer<QMap<QString, QColor> > qMapAverageColor);

    //=========================================================================================================
    /**
     * Set the average activations
     *
     * @param[in] qMapAverageActivation      Pointer to the new average activations.
     */
    void setAverageActivation(const QSharedPointer<QMap<QString, bool> > qMapAverageActivation);

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
     * Call this slot whenever the average selection or color changed.
     */
    void onAverageSelectionColorChanged();

    int m_iMaxNumAverages;

    QSharedPointer<QMap<QString, QColor> >      m_qMapAverageColor;             /**< Average colors. */
    QSharedPointer<QMap<QString, bool> >        m_qMapAverageActivation;        /**< Average activation status. */

signals:
    //=========================================================================================================
    /**
     * Emmited when new average color is available
     *
     * @param[in] qMapAverageColor     the average color map.
     */
    void newAverageColorMap(const QSharedPointer<QMap<QString, QColor> > qMapAverageColor);

    //=========================================================================================================
    /**
     * Emmited when new average activation is available
     *
     * @param[in] qMapAverageActivation     the average activation map.
     */
    void newAverageActivationMap(const QSharedPointer<QMap<QString, bool> > qMapAverageActivation);
};
} // NAMESPACE

#endif // AVERAGESELECTIONVIEW_H
