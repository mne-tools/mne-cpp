//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     applytoview.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.6
 * @date     August 2020
 * @brief    Implementation of the ApplyToView selector widget.
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
