//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     ftbuffersetupwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     January, 2020
 * @brief    Definition of the FtBufferSetupWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffersetupwidget.h"
#include "../ftbuffer.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCheckBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FTBUFFERPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FtBufferSetupWidget::FtBufferSetupWidget(FtBuffer* pFtBuffer, QWidget* parent)
: QWidget(parent)
{
    auto* host = new QLineEdit(QString::fromStdString(pFtBuffer->m_host));
    host->setObjectName("m_lineEdit_Host");
    host->setToolTip(tr("The computer that runs the buffer, e.g. the Neuromag acquisition computer with neuromag2ft"));
    connect(host, &QLineEdit::textChanged, this, [pFtBuffer](const QString& text) { pFtBuffer->m_host = text.trimmed().toStdString(); });

    auto* port = new QSpinBox;
    port->setObjectName("m_spinBox_Port");
    port->setRange(1, 65535);
    port->setValue(pFtBuffer->m_port);
    connect(port, &QSpinBox::valueChanged, this, [pFtBuffer](int value) { pFtBuffer->m_port = static_cast<std::uint16_t>(value); });

    auto* block = new QSpinBox;
    block->setObjectName("m_spinBox_SamplesPerBlock");
    block->setRange(1, 100000);
    block->setValue(pFtBuffer->m_iSamplesPerBlock);
    connect(block, &QSpinBox::valueChanged, this, [pFtBuffer](int value) { pFtBuffer->m_iSamplesPerBlock = value; });

    auto* simulate = new QCheckBox(tr("Simulate neuromag2ft and its buffer"));
    simulate->setObjectName("m_checkBox_Simulate");
    simulate->setChecked(pFtBuffer->m_bSimulate);
    connect(simulate, &QCheckBox::toggled, this, [pFtBuffer](bool on) { pFtBuffer->m_bSimulate = on; });

    auto* form = new QFormLayout(this);
    form->addRow(tr("Host"), host);
    form->addRow(tr("Port"), port);
    form->addRow(tr("Samples per block"), block);
    form->addRow(simulate);
}
