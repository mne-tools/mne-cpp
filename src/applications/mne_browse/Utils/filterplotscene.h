//=============================================================================================================
/**
 * @file     filterplotscene.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  2.1.0
 * @date     September, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the FilterPlotScene class.
 *
 */

#ifndef FILTERPLOTSCENE_H
#define FILTERPLOTSCENE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../Utils/sessionfilter.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsScene>
#include <QPainterPath>
#include <QGraphicsPathItem>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

/**
 * DECLARE CLASS FilterPlotScene
 *
 * @brief The FilterPlotScene class provides the scene where a filter respone can be plotted.
 */
class FilterPlotScene : public QGraphicsScene
{
    Q_OBJECT
public:
    //=========================================================================================================
    /**
     * Constructs a FilterPlotScene dialog which is a child of parent.
     *
     * @param [in] parent pointer to parent widget; If parent is 0, the new FilterPlotScene becomes a window. If parent is another widget, FilterPlotScene becomes a child window inside parent. FilterPlotScene is deleted when its parent is deleted.
     */
    FilterPlotScene(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Updates the current filter.
     *
     * @param [in] filter          Shared filter definition to be plotted.
     * @param [in] samplingFreq    Current sampling frequency in Hz.
     */
    void updateFilter(const QSharedPointer<SessionFilter>& filter, int samplingFreq);

protected:
    //=========================================================================================================
    /**
     * Draws the diagram to plot the magnitude.
     *
     * @param [in] samplingFreq  Current sampling frequency in Hz.
     * @param [in] xOffset       Left edge of the magnitude panel.
     * @param [in] diagramWidth  Width of the plotted response in samples/pixels.
     */
    void plotMagnitudeDiagram(int samplingFreq, int xOffset, int diagramWidth);

    //=========================================================================================================
    /**
     * Draws the diagram to plot the phase.
     *
     * @param [in] samplingFreq  Current sampling frequency in Hz.
     * @param [in] xOffset       Left edge of the phase panel.
     * @param [in] diagramWidth  Width of the plotted response in samples/pixels.
     */
    void plotPhaseDiagram(int samplingFreq, int xOffset, int diagramWidth);

    //=========================================================================================================
    /**
     * Draws the filter's magnitude response.
     *
     * @param [in] xOffset       Left edge of the magnitude panel.
     * @param [in] diagramWidth  Width of the plotted response in samples/pixels.
     */
    void plotFilterFrequencyResponse(int xOffset, int diagramWidth);

    //=========================================================================================================
    /**
     * Draws the filter's phase response.
     *
     * @param [in] xOffset       Left edge of the phase panel.
     * @param [in] diagramWidth  Width of the plotted response in samples/pixels.
     */
    void plotFilterPhaseResponse(int xOffset, int diagramWidth);

    QSharedPointer<SessionFilter>       m_pCurrentFilter;       /**< Pointer to the current session filter. */

    QGraphicsPathItem*                  m_pGraphicsItemPath;    /**< Pointer to the graphics path item in the filterplotscene */

    int             m_iScalingFactor;           /**< Scales the db filter magnitudes by the specified factor in order to provide better plotting. */
    double          m_dMaxMagnitude;            /**< the maximum magnirutde shown in the diagram. */
    int             m_iNumberHorizontalLines;   /**< number of plotted horizontal ()lines. */
    int             m_iNumberVerticalLines;     /**< number of plotted vertical lines. */
    int             m_iAxisTextSize;            /**< point size of the plotted text. */
    int             m_iDiagramMarginsHoriz;     /**< horizontal space between the filter and diagram plot.  */
    int             m_iDiagramMarginsVert;      /**< vertical space between the filter and diagram plot. */
    int             m_iDiagramSpacing;          /**< Horizontal spacing between magnitude and phase panels. */
    int             m_iCutOffMarkerWidth;       /**< cut off marker width. */

};

} // NAMESPACE MNEBROWSE

#endif // FILTERPLOTSCENE_H
