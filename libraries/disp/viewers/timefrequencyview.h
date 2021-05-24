//=============================================================================================================
/**
 * @file     timefrequencyview.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     April, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta. All rights reserved.
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
 * @brief    Declaration of the TimeFrequencyView Class.
 *
 */

#ifndef TIMEFREQUENCYVIEW_H
#define TIMEFREQUENCYVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"
#include <disp/plots/tfplot.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QLayout>
#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================


//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

class EvokedSetModel;
class TimeFrequencyModel;

//=============================================================================================================
/**
 * @brief The TimeFrequencyView class
 */
class DISPSHARED_EXPORT TimeFrequencyView : public AbstractView
{
    Q_OBJECT
public:
    //=========================================================================================================
    TimeFrequencyView();    

    //=========================================================================================================
    TimeFrequencyView(const QString& sSettingsPath,
                      QWidget* parent,
                      Qt::WindowFlags f = Qt::Widget);

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
     * Set the evoked set model.
     *
     * @param[in] model     The new evoked set model.
     */
    void setEvokedSetModel(QSharedPointer<EvokedSetModel> model);

    //=========================================================================================================
    /**
     * Sets spacing for time frequency chart
     *
     * @param[in] iSpacing      Spacing value in pixels
     */
    void setChartBorderSpacing(int iSpacing);

    void setTimeFrequencyModel(QSharedPointer<DISPLIB::TimeFrequencyModel> pModel);

    void setSelectedChannels(QList<int> selectionList);


protected:
    void initQMLView();

    virtual void paintEvent(QPaintEvent *event);

//    void paintChart(QPainter& painter,
//                    const QRect chartBound);

//    void paintAxes(QPainter& painter,
//                   const QRect chartBound);

    void generatePixmap(Eigen::MatrixXd tf_matrix,
                        ColorMaps cmap);

    //=========================================================================================================
    /**
     * call this function whenever the items' data needs to be updated
     */
    void updateData();

    QSharedPointer<EvokedSetModel>              m_pEvokedSetModel;              /**< The evoked model */
    int                                         m_iChartBorderSpacing;
    QSharedPointer<TimeFrequencyModel>          m_pTFModel;

    QPointer<QVBoxLayout>                       m_pLayout;
    QPointer<TFplot>                            m_pPlot;

    QPixmap                                     m_PlotPixmap;

    QPixmap                                     m_CoefficientPixmap;

};

} //namespace
#endif // TIMEFREQUENCYVIEW_H
