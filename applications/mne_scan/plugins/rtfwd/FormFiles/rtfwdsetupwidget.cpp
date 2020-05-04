//=============================================================================================================
/**
 * @file     rtfwdsetupwidget.cpp
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.0
 * @date     May, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief    Definition of the RtFwdSetupWidget class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtfwdsetupwidget.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTFWDPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtFwdSetupWidget::RtFwdSetupWidget(RtFwd* toolbox, QWidget *parent)
: QWidget(parent)
, m_pRtFwd(toolbox)
{
    m_ui.setupUi(this);

    // init file Loading
    m_ui.m_qLineEdit_SolName->setText(m_pRtFwd->m_sSolName);
    m_ui.m_qLineEdit_BemName->setText(m_pRtFwd->m_sBemName);
    m_ui.m_qLineEdit_SourceName->setText(m_pRtFwd->m_sSourceName);
    m_ui.m_qLineEdit_MriName->setText(m_pRtFwd->m_sMriName);
    m_ui.m_qLineEdit_MinDistOut->setText(m_pRtFwd->m_sMinDistOutName);
    m_ui.m_qLineEdit_EEGModelFile->setText(m_pRtFwd->m_sEegModelFile);
    m_ui.m_qLineEdit_EEGModelName->setText(m_pRtFwd->m_sEegModelName);

    // init checkboxes
    m_ui.m_check_bDoAll->setChecked(m_pRtFwd->m_bDoAll);
    m_ui.m_check_bIncludeEEG->setChecked(m_pRtFwd->m_bIncludeEEG);
    m_ui.m_check_bIncludeMeg->setChecked(m_pRtFwd->m_bIncludeMEG);
    m_ui.m_check_bComputeGrad->setChecked(m_pRtFwd->m_bComputeGrad);
    m_ui.m_check_bAccurate->setChecked(m_pRtFwd->m_bAccurate);
    m_ui.m_check_bDoAll->setChecked(m_pRtFwd->m_bDoAll);
    m_ui.m_check_bFixedOri->setChecked(m_pRtFwd->m_bFixedOri);
    m_ui.m_check_bFilterSpaces->setChecked(m_pRtFwd->m_bFilterSpaces);
    m_ui.m_check_bMriHeadIdent->setChecked(m_pRtFwd->m_bMriHeadIdent);
    m_ui.m_check_bUseThreads->setChecked(m_pRtFwd->m_bUseThreads);
    m_ui.m_check_bUseEquivEeg->setChecked(m_pRtFwd->m_bUseEquivEeg);

    // init Spin Boxes
    m_ui.m_doubleSpinBox_dMinDist->setValue(m_pRtFwd->m_fMinDist);
    m_ui.m_doubleSpinBox_dEegSphereRad->setValue(m_pRtFwd->m_fEegSphereRad);
    m_ui.m_doubleSpinBox_dvecR0x->setValue(m_pRtFwd->m_vecR0.x());
    m_ui.m_doubleSpinBox_dvecR0y->setValue(m_pRtFwd->m_vecR0.y());
    m_ui.m_doubleSpinBox_dvecR0z->setValue(m_pRtFwd->m_vecR0.z());

}

//=============================================================================================================

RtFwdSetupWidget::~RtFwdSetupWidget()
{
}
