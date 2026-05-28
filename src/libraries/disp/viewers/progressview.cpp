//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Gabriel B Motta <gbmotta@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file progressview.cpp
 * @since 2022
 * @date  January 2024
 * @brief Implementation of the ProgressView progress widget.
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
