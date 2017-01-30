//=============================================================================================================
/**
* @file     averagingsettingswidget.h
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
* @brief    Contains the declaration of the AveragingSettingsWidget class.
*
*/

#ifndef AVERAGINGSETTINGSWIDGET_H
#define AVERAGINGSETTINGSWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../averaging.h"

#include "../ui_averagingsettingswidget.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QSpinBox>
#include <QPair>
#include <QComboBox>
#include <QCheckBox>
#include <QGridLayout>
#include <QSpinBox>
#include <QLabel>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE AveragingPlugin
//=============================================================================================================

namespace AveragingPlugin
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Averaging;


class AveragingSettingsWidget : public QWidget
{
    Q_OBJECT

    friend class Averaging;

public:
    typedef QSharedPointer<AveragingSettingsWidget> SPtr;         /**< Shared pointer type for AveragingAdjustmentWidget. */
    typedef QSharedPointer<AveragingSettingsWidget> ConstSPtr;    /**< Const shared pointer type for AveragingAdjustmentWidget. */

    explicit AveragingSettingsWidget(Averaging *toolbox, QWidget *parent = 0);

    int getStimChannelIdx();

signals:

public slots:

private:
    void changePreStim();
    void changePostStim();
    void changeBaselineFrom();
    void changeBaselineTo();
    void changeArtifactThreshold();
    void changeNumAverages();

    Ui::AverageSettingsWidgetClass ui;		/**< Holds the user interface for the AverageSettingsWidgetClass.*/

    Averaging* m_pAveragingToolbox;
};

} // NAMESPACE

#endif // AVERAGINGSETTINGSWIDGET_H
