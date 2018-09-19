//=============================================================================================================
/**
* @file     averagingsettingsview.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Christoph Dinh, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the AveragingSettingsView class.
*
*/

#ifndef AVERAGINGSETTINGSVIEW_H
#define AVERAGINGSETTINGSVIEW_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QWidget>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
}

namespace Ui {
    class AverageSettingsViewWidget;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

class DISPSHARED_EXPORT AveragingSettingsView : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<AveragingSettingsView> SPtr;         /**< Shared pointer type for AveragingAdjustmentWidget. */
    typedef QSharedPointer<AveragingSettingsView> ConstSPtr;    /**< Const shared pointer type for AveragingAdjustmentWidget. */

    explicit AveragingSettingsView(QWidget *parent,
                                   QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo,
                                   const QList<qint32>& qListStimChs,
                                   int iStimChan,
                                   int iNumAverages,
                                   int iAverageMode,
                                   int iPreStimSeconds,
                                   int iPostStimSeconds,
                                   bool bDoArtifactThresholdReduction,
                                   bool bDoArtifactVarianceReduction,
                                   double dArtifactThresholdFirst,
                                   int iArtifactThresholdSecond,
                                   double dArtifactVariance,
                                   bool bDoBaselineCorrection,
                                   int iBaselineFromSeconds,
                                   int iBaselineToSeconds);

    void setStimChannels(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo,
                            QList<qint32> qListStimChs,
                            int iStimChan);

    int getStimChannelIdx();

private:
    void onChangePreStim();
    void onChangePostStim();
    void onChangeBaselineFrom();
    void onChangeBaselineTo();
    void onChangeArtifactThreshold();
    void onChangeNumAverages();

    Ui::AverageSettingsViewWidget* ui;		/**< Holds the user interface for the AverageSettingsWidgetClass.*/

signals:
    void changePreStim(qint32 value);
    void changePostStim(qint32 value);
    void changeBaselineFrom(qint32 value);
    void changeBaselineTo(qint32 value);
    void changeArtifactThreshold(qint32 first, qint32 second);
    void changeNumAverages(qint32 value);
    void changeStimChannel(qint32 index);
    void changeArtifactThresholdReductionActive(bool state);
    void changeArtifactVarianceReductionActive(bool state);
    void changeArtifactVariance(double dVariance);
    void changeBaselineActive(bool state);
    void resetAverage(bool state);
    void changeAverageMode(qint32 index);

};

} // NAMESPACE

#endif // AVERAGINGSETTINGSVIEW_H
