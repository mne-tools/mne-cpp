//=============================================================================================================
/**
 * @file     applytoview.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.5
 * @date     August, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the ApplyToView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "applytoview.h"

#include "ui_applytoview.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ApplyToView::ApplyToView(const QString& sSettingsPath,
                         QWidget *parent,
                         Qt::WindowFlags f)
: AbstractView(parent, f)
, m_sSettingsPath(sSettingsPath)
, m_pUi(new Ui::ApplyToViewWidget)
{
    m_pUi->setupUi(this);
    this->setMinimumWidth(330);

    connect(m_pUi->m_pushButtonAll, &QPushButton::clicked,
            this, &ApplyToView::selectAll);

    connect(m_pUi->m_pushButtonClear, &QPushButton::clicked,
            this, &ApplyToView::selectClear);
}

//=============================================================================================================

ApplyToView::~ApplyToView()
{
    delete m_pUi;
}

//=============================================================================================================

void ApplyToView::saveSettings()
{

}

//=============================================================================================================

void ApplyToView::loadSettings()
{

}

//=============================================================================================================

void ApplyToView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void ApplyToView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void ApplyToView::selectAll(bool bChecked)
{
    Q_UNUSED(bChecked);

    m_pUi->m_checkBoxSignaViewer->setChecked(true);
    m_pUi->m_checkBoxButterfly->setChecked(true);
    m_pUi->m_checkBoxLayout->setChecked(true);
}

//=============================================================================================================

void ApplyToView::selectClear(bool bChecked)
{
    Q_UNUSED(bChecked);

    m_pUi->m_checkBoxSignaViewer->setChecked(false);
    m_pUi->m_checkBoxButterfly->setChecked(false);
    m_pUi->m_checkBoxLayout->setChecked(false);
}

//=============================================================================================================

QList<QString> ApplyToView::getSelectedViews()
{
    m_lViewList.clear();

    m_lViewList.append("null");

    if (m_pUi->m_checkBoxSignaViewer->isChecked()) {
        m_lViewList.append("signalview");
    }
    if (m_pUi->m_checkBoxButterfly->isChecked()) {
        m_lViewList.append("butterflyview");
    }
    if (m_pUi->m_checkBoxLayout->isChecked()){
        m_lViewList.append("layoutview");
    }

    return m_lViewList;
}

//=============================================================================================================

void ApplyToView::clearView()
{

}
