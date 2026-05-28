//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file filterplotscene.h
 * @since 2022
 * @date  March 2026
 * @brief QGraphicsScene that draws the magnitude (and optional phase) response of a designed FIR / IIR filter.
 *
 * FilterPlotScene is the canvas used by @ref FilterDesignView to
 * preview the currently designed filter. It paints axis grids,
 * interpolated magnitude / phase curves and the cut-off frequency
 * markers, and re-renders itself every time the host view edits a
 * kernel parameter.
 */

#ifndef FILTERPLOTSCENE_H
#define FILTERPLOTSCENE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"
#include "layoutscene.h"

#include <dsp/filterkernel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QGraphicsPathItem;
class QGraphicsView;

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
 * @brief QGraphicsScene rendering the magnitude (and optional phase) response of a designed FIR / IIR filter.
 *
 * Used by @ref FilterDesignView to preview the currently designed
 * filter; re-renders itself every time the host view edits a kernel
 * parameter.
 */
class DISPSHARED_EXPORT FilterPlotScene : public LayoutScene
{
    Q_OBJECT

public:
    typedef QSharedPointer<FilterPlotScene> SPtr;            /**< Shared pointer type for FilterPlotScene class. */
    typedef QSharedPointer<const FilterPlotScene> ConstSPtr; /**< Const shared pointer type for FilterPlotScene class. */

    //=========================================================================================================
    /**
     * Constructs a FilterPlotScene dialog which is a child of parent.
     *
     * @param[in] parent pointer to parent widget; If parent is 0, the new FilterPlotScene becomes a window. If parent is another widget, FilterPlotScene becomes a child window inside parent. FilterPlotScene is deleted when its parent is deleted.
     */
    FilterPlotScene(QGraphicsView* view,
                    QObject *parent = 0);

    //=========================================================================================================
    /**
     * Updates the current filter.
     *
     * @param[in] operatorFilter pointer to the current filter operator which is to be plotted.
     * @param[in] samplingFreq holds the current sampling frequency.
     * @param[in] cutOffLow cut off frequqency lowpass or lower cut off when filter is a bandpass.
     * @param[in] cutOffHigh cut off frequqency highpass or higher cut off when filter is a bandpass.
     */
    void updateFilter(const UTILSLIB::FilterKernel &operatorFilter,
                      int samplingFreq,
                      int cutOffLow,
                      int cutOffHigh);

protected:
    //=========================================================================================================
    /**
     * Draws the diagram to plot the magnitude.
     *
     * @param[in] holds the current sampling frequency.
     * @param[in] holds the current name of the filter. Default is an empty QString.
     */
    void plotMagnitudeDiagram(int samplingFreq,
                              const QString &filtername = QString());

    //=========================================================================================================
    /**
     * Plots the filter's frequency response.
     */
    void plotFilterFrequencyResponse();

    UTILSLIB::FilterKernel    m_pCurrentFilter;       /**< Pointer to the filter operator. */

    QGraphicsPathItem*      m_pGraphicsItemPath;    /**< Pointer to the graphics path item in the filterplotscene. */

    int             m_iMaxMagnitude;                /**< the maximum magnitude shown in the diagram. */
    int             m_iScalingFactor;               /**< Scales the db filter magnitudes by the specified factor in order to provide better plotting. */
    int             m_iNumberHorizontalLines;       /**< number of plotted horizontal ()lines. */
    int             m_iNumberVerticalLines;         /**< number of plotted vertical lines. */
    int             m_iAxisTextSize;                /**< point size of the plotted text. */
    int             m_iDiagramMarginsHoriz;         /**< horizontal space between the filter and diagram plot. */
    int             m_iDiagramMarginsVert;          /**< vertical space between the filter and diagram plot. */
    int             m_iCutOffLow;                   /**< cut off frequqency lowpass or lower cut off when filter is a bandpass. */
    int             m_iCutOffHigh;                  /**< cut off frequqency highpass or higher cut off when filter is a bandpass. */
    int             m_iCutOffMarkerWidth;           /**< cut off marker width. */
    int             m_iPlotLength;                  /**< Length of current filter impulse response plot. */
    QColor          m_cPenColor;                    /**< Color of the text and plot of the filter freq response plot. */
};
} // NAMESPACE DISPLIB

#endif // FILTERPLOTSCENE_H
