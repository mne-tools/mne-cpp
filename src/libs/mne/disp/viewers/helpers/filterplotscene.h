//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     filterplotscene.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2014
 * @brief    QGraphicsScene that draws the magnitude (and optional phase) response of a designed FIR / IIR filter.
 *
 * FilterPlotScene is the canvas used by @ref mne::disp::FilterDesignView to
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

#include <mne/dsp/filterkernel.h>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QGraphicsPathItem;
class QGraphicsView;

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

namespace mne::disp
{

//=============================================================================================================
// mne::disp FORWARD DECLARATIONS
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
    //=========================================================================================================
    /**
     * Constructs a FilterPlotScene dialog which is a child of parent.
     *
     * @param[in, out] view Graphics view that displays the scene.
     * @param[in] parent pointer to parent widget; If parent is 0, the new FilterPlotScene becomes a window. If parent is another widget, FilterPlotScene becomes a child window inside parent. FilterPlotScene is deleted when its parent is deleted.
     */
    FilterPlotScene(QGraphicsView* view,
                    QObject* parent = 0);

    //=========================================================================================================
    /**
     * Updates the current filter.
     *
     * @param[in] operatorFilter pointer to the current filter operator which is to be plotted.
     * @param[in] samplingFreq holds the current sampling frequency.
     * @param[in] cutOffLow cut off frequqency lowpass or lower cut off when filter is a bandpass.
     * @param[in] cutOffHigh cut off frequqency highpass or higher cut off when filter is a bandpass.
     */
    void updateFilter(const mne::FilterKernel& operatorFilter,
                      int samplingFreq,
                      int cutOffLow,
                      int cutOffHigh);

protected:
    //=========================================================================================================
    /**
     * Draws the diagram to plot the magnitude.
     *
     * @param[in] samplingFreq Current sampling frequency in Hz.
     * @param[in] filtername Name of the filter shown in the plot (default: empty).
     *
     */
    void plotMagnitudeDiagram(int samplingFreq,
                              const QString& filtername = QString());

    //=========================================================================================================
    /**
     * Plots the filter's frequency response.
     */
    void plotFilterFrequencyResponse();

    mne::FilterKernel m_pCurrentFilter; /**< Pointer to the filter operator. */

    QGraphicsPathItem* m_pGraphicsItemPath; /**< Pointer to the graphics path item in the filterplotscene. */

    int m_iMaxMagnitude;          /**< the maximum magnitude shown in the diagram. */
    int m_iScalingFactor;         /**< Scales the db filter magnitudes by the specified factor in order to provide better plotting. */
    int m_iNumberHorizontalLines; /**< number of plotted horizontal ()lines. */
    int m_iNumberVerticalLines;   /**< number of plotted vertical lines. */
    int m_iAxisTextSize;          /**< point size of the plotted text. */
    int m_iDiagramMarginsHoriz;   /**< horizontal space between the filter and diagram plot. */
    int m_iDiagramMarginsVert;    /**< vertical space between the filter and diagram plot. */
    int m_iCutOffLow;             /**< cut off frequqency lowpass or lower cut off when filter is a bandpass. */
    int m_iCutOffHigh;            /**< cut off frequqency highpass or higher cut off when filter is a bandpass. */
    int m_iCutOffMarkerWidth;     /**< cut off marker width. */
    int m_iPlotLength;            /**< Length of current filter impulse response plot. */
    QColor m_cPenColor;           /**< Color of the text and plot of the filter freq response plot. */
};
} // NAMESPACE mne_disp

#endif // FILTERPLOTSCENE_H
