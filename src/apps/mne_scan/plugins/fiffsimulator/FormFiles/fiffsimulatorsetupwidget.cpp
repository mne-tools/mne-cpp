//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     fiffsimulatorsetupwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Definition of the FiffSimulatorSetupWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffsimulatorsetupwidget.h"
#include "../fiffsimulator.h"

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

using namespace FIFFSIMULATORPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffSimulatorSetupWidget::FiffSimulatorSetupWidget(FiffSimulator* pFiffSimulator, QWidget* parent)
: QWidget(parent)
{
    auto* host = new QLineEdit(QString::fromStdString(pFiffSimulator->m_host));
    host->setObjectName("m_lineEdit_Host");
    host->setToolTip(tr("The computer that runs mne_rt_server"));
    connect(host, &QLineEdit::textChanged, this, [pFiffSimulator](const QString& text) { pFiffSimulator->m_host = text.trimmed().toStdString(); });

    auto* port = new QSpinBox;
    port->setObjectName("m_spinBox_Port");
    port->setRange(1, 65534);
    port->setValue(pFiffSimulator->m_port);
    port->setToolTip(tr("The command port of mne_rt_server; the data port is the next one"));
    connect(port, &QSpinBox::valueChanged, this, [pFiffSimulator](int value) { pFiffSimulator->m_port = static_cast<std::uint16_t>(value); });

    auto* file = new QLineEdit(QString::fromStdString(pFiffSimulator->m_simfile));
    file->setObjectName("m_lineEdit_SimFile");
    file->setPlaceholderText(tr("The server's file"));
    file->setToolTip(tr("FIFF file on the server's computer to replay"));
    connect(file, &QLineEdit::textChanged, this, [pFiffSimulator](const QString& text) { pFiffSimulator->m_simfile = text.trimmed().toStdString(); });

    auto* block = new QSpinBox;
    block->setObjectName("m_spinBox_SamplesPerBlock");
    block->setRange(1, 100000);
    block->setValue(pFiffSimulator->m_iSamplesPerBlock);
    connect(block, &QSpinBox::valueChanged, this, [pFiffSimulator](int value) { pFiffSimulator->m_iSamplesPerBlock = value; });

    auto* simulate = new QCheckBox(tr("Simulate the server"));
    simulate->setObjectName("m_checkBox_Simulate");
    simulate->setChecked(pFiffSimulator->m_bSimulate);
    connect(simulate, &QCheckBox::toggled, this, [pFiffSimulator](bool on) { pFiffSimulator->m_bSimulate = on; });

    auto* form = new QFormLayout(this);
    form->addRow(tr("Host"), host);
    form->addRow(tr("Port"), port);
    form->addRow(tr("File"), file);
    form->addRow(tr("Samples per block"), block);
    form->addRow(simulate);
}
