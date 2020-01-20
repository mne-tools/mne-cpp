//=============================================================================================================
/**
 * @file     rtssssetupwidget.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the ECGSetupWidget class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtssssetupwidget.h"
#include "rtsssaboutwidget.h"
#include "../rtsss.h"
#include "../rtsssalgo.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSSSPLUGIN;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSssSetupWidget::RtSssSetupWidget(RtSss* toolbox, QWidget *parent)
: QWidget(parent)
, m_pRtSss(toolbox)
{
    ui.setupUi(this);

    connect(ui.m_qPushButton_About, SIGNAL(released()), this, SLOT(showAboutDialog()));

    connect(ui.m_qSpinBox_LinRR, SIGNAL(valueChanged (int)), this, SLOT(setNewLinRR(int)));

    connect(ui.m_qSpinBox_LinRR, SIGNAL(valueChanged (int)), this, SLOT(setNewLinRR(int)));
    connect(ui.m_qSpinBox_LoutRR, SIGNAL(valueChanged (int)), this, SLOT(setNewLoutRR(int)));
    connect(ui.m_qSpinBox_Lin, SIGNAL(valueChanged (int)), this, SLOT(setNewLin(int)));
    connect(ui.m_qSpinBox_Lout, SIGNAL(valueChanged (int)), this, SLOT(setNewLout(int)));
}


//*************************************************************************************************************

RtSssSetupWidget::~RtSssSetupWidget()
{

}


//*************************************************************************************************************

void RtSssSetupWidget::setNewLinRR(int val)
{
    std::cout << "###### Emitted LinRR(set): " << val << std::endl;
    emit signalNewLinRR(val);
}

void RtSssSetupWidget::setNewLoutRR(int val)
{
    std::cout << "###### Emitted LoutRR(set): " << val << std::endl;
    emit signalNewLoutRR(val);
}

void RtSssSetupWidget::setNewLin(int val)
{
    std::cout << "###### Emitted Lin(set): " << val << std::endl;
    emit signalNewLin(val);
}

void RtSssSetupWidget::setNewLout(int val)
{
    std::cout << "###### Emitted Lout(set): " << val << std::endl;
    emit signalNewLout(val);
}


//*************************************************************************************************************

int RtSssSetupWidget::getLinRR()
{
    return ui.m_qSpinBox_LinRR->value();
}

int RtSssSetupWidget::getLoutRR()
{
    return ui.m_qSpinBox_LoutRR->value();
}

int RtSssSetupWidget::getLin()
{
    return ui.m_qSpinBox_Lin->value();
}

int RtSssSetupWidget::getLout()
{
    return ui.m_qSpinBox_Lout->value();
}


//*************************************************************************************************************

void RtSssSetupWidget::showAboutDialog()
{
    RtSssAboutWidget aboutDialog(this);
    aboutDialog.exec();
}
