//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     babymegsetupwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    BabyMEGSetupWidget class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymegsetupwidget.h"
#include "../babymeg.h"

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

using namespace BABYMEGPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEGSetupWidget::BabyMEGSetupWidget(BabyMEG* pBabyMEG, QWidget* parent)
: QWidget(parent)
{
    auto* host = new QLineEdit(QString::fromStdString(pBabyMEG->m_host));
    host->setObjectName("m_lineEdit_Host");
    host->setToolTip(tr("The computer that runs the BabyMEG acquisition server"));
    connect(host, &QLineEdit::textChanged, this, [pBabyMEG](const QString& text) { pBabyMEG->m_host = text.trimmed().toStdString(); });

    auto* port = new QSpinBox;
    port->setObjectName("m_spinBox_Port");
    port->setRange(1, 65535);
    port->setValue(pBabyMEG->m_port);
    connect(port, &QSpinBox::valueChanged, this, [pBabyMEG](int value) { pBabyMEG->m_port = static_cast<std::uint16_t>(value); });

    auto* commandPort = new QSpinBox;
    commandPort->setObjectName("m_spinBox_CommandPort");
    commandPort->setRange(1, 65535);
    commandPort->setValue(pBabyMEG->m_commandPort);
    commandPort->setToolTip(tr("The port on which the server describes its channels and gains"));
    connect(commandPort, &QSpinBox::valueChanged, this, [pBabyMEG](int value) { pBabyMEG->m_commandPort = static_cast<std::uint16_t>(value); });

    auto* block = new QSpinBox;
    block->setObjectName("m_spinBox_SamplesPerBlock");
    block->setRange(1, 100000);
    block->setValue(pBabyMEG->m_iSamplesPerBlock);
    connect(block, &QSpinBox::valueChanged, this, [pBabyMEG](int value) { pBabyMEG->m_iSamplesPerBlock = value; });

    auto* simulate = new QCheckBox(tr("Simulate the acquisition server"));
    simulate->setObjectName("m_checkBox_Simulate");
    simulate->setChecked(pBabyMEG->m_bSimulate);
    connect(simulate, &QCheckBox::toggled, this, [pBabyMEG](bool on) { pBabyMEG->m_bSimulate = on; });

    auto* form = new QFormLayout(this);
    form->addRow(tr("Host"), host);
    form->addRow(tr("Data port"), port);
    form->addRow(tr("Command port"), commandPort);
    form->addRow(tr("Samples per block"), block);
    form->addRow(simulate);
}
