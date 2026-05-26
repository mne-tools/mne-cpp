//=============================================================================================================
/**
 * @file     writetofilestatuswidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, MNE-CPP Authors. All rights reserved. (BSD-3-Clause)
 *
 * @brief    Definition of WriteToFileStatusWidget.
 */

#include "writetofilestatuswidget.h"
#include "../writetofile.h"

#include <QHBoxLayout>
#include <QLabel>

using namespace WRITETOFILEPLUGIN;

WriteToFileStatusWidget::WriteToFileStatusWidget(WriteToFile* pPlugin, QWidget* parent)
: QWidget(parent)
, m_pDot(new QLabel(this))
, m_pText(new QLabel(this))
, m_bActive(false)
{
    auto* pLayout = new QHBoxLayout(this);
    pLayout->setContentsMargins(6, 0, 6, 0);
    pLayout->setSpacing(4);

    // Red recording dot; hidden until the plugin reports active recording.
    m_pDot->setText(QStringLiteral("\u25CF"));
    m_pDot->setStyleSheet(QStringLiteral("color: #e11d48; font-weight: bold;"));
    m_pDot->setVisible(false);

    m_pText->setText(tr("Not recording"));

    pLayout->addWidget(m_pDot);
    pLayout->addWidget(m_pText);

    if (pPlugin) {
        connect(pPlugin, &WriteToFile::recordingStatus,
                this, &WriteToFileStatusWidget::onRecordingStatus);
        connect(pPlugin, &WriteToFile::recordingActiveChanged,
                this, &WriteToFileStatusWidget::onRecordingActiveChanged);
    }
}

QString WriteToFileStatusWidget::currentText() const
{
    return m_pText ? m_pText->text() : QString();
}

bool WriteToFileStatusWidget::isActive() const
{
    return m_bActive;
}

void WriteToFileStatusWidget::onRecordingStatus(const QString& sSummary)
{
    if (m_pText) {
        m_pText->setText(sSummary);
    }
}

void WriteToFileStatusWidget::onRecordingActiveChanged(bool bActive)
{
    m_bActive = bActive;
    if (m_pDot) {
        m_pDot->setVisible(bActive);
    }
    if (!bActive && m_pText) {
        m_pText->setText(tr("Not recording"));
    }
}
