//=============================================================================================================
/**
 * @file     progressview.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.6
 * @date     September, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Gabriel B Motta. All rights reserved.
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
 * @brief    Definition of the ProgressView class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "progressview.h"
#include "ui_progressview.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ProgressView::ProgressView(bool bHorizontalMessage,
                           const QString& sStyleSheet)
: AbstractView()
, m_pUi(new Ui::ProgressViewWidget)
{
    m_pUi->setupUi(this);

    (bHorizontalMessage) ? setHorizontal() : setVertical();

    if(sStyleSheet != ""){
        m_pUi->m_VerticalLabel->setStyleSheet(sStyleSheet);
        m_pUi->m_HorizonatlLabel->setStyleSheet(sStyleSheet);
    }

    m_pUi->m_progressBar->setMinimum(0);
    m_pUi->m_progressBar->setMaximum(0);
    m_pUi->m_progressBar->setValue(0);
    m_pUi->m_progressBar->setTextVisible(false);
    m_pUi->m_progressBar->setMaximumWidth(300);

    m_pUi->m_progressBar->setAttribute(Qt::WA_Hover);
}

//=============================================================================================================

ProgressView::~ProgressView()
{
    delete m_pUi;
}

//=============================================================================================================

void ProgressView::saveSettings()
{
}

//=============================================================================================================

void ProgressView::loadSettings()
{
}

//=============================================================================================================

void ProgressView::updateGuiMode(GuiMode mode)
{
    Q_UNUSED(mode);
}

//=============================================================================================================

void ProgressView::updateProcessingMode(ProcessingMode mode)
{
    Q_UNUSED(mode);
}

//=============================================================================================================

void ProgressView::setHorizontal()
{
    m_pUi->m_VerticalLabel->hide();
    m_pUi->m_HorizonatlLabel->show();
}

//=============================================================================================================

void ProgressView::setVertical()
{
    m_pUi->m_VerticalLabel->show();
    m_pUi->m_HorizonatlLabel->hide();
}

//=============================================================================================================

void ProgressView::setMessage(const QString &sMessage)
{
    m_pUi->m_VerticalLabel->setText(sMessage);
    m_pUi->m_HorizonatlLabel->setText(sMessage);
}

//=============================================================================================================

void ProgressView::updateProgress(int iPercentage,
                                  const QString& sMessage)
{
    m_pUi->m_progressBar->show();

    if (m_pUi->m_progressBar->maximum() == 0){
        m_pUi->m_progressBar->setMaximum(100);
    }

    m_pUi->m_progressBar->setValue(iPercentage);

    if(sMessage != ""){
        setMessage(sMessage);
    }
}

//=============================================================================================================

void ProgressView::setLoadingBarVisible(bool bVisible)
{
    if (bVisible){
        m_pUi->m_progressBar->show();
    } else {
        m_pUi->m_progressBar->hide();
    }
}

//=============================================================================================================

void ProgressView::clearView()
{

}
