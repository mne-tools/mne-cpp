//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     natussetup.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     June, 2018
 * @brief    Definition of the NatusSetup class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "natussetup.h"
#include "../natus.h"

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

using namespace NATUSPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NatusSetup::NatusSetup(Natus* pNatus, QWidget* parent)
: QWidget(parent)
{
    auto* address = new QLineEdit(QString::fromStdString(pNatus->m_address));
    address->setObjectName("m_lineEdit_Address");
    address->setToolTip(tr("Local IPv4 address the datagrams arrive on; 0.0.0.0 for all"));
    connect(address, &QLineEdit::textChanged, this, [pNatus](const QString& text) { pNatus->m_address = text.trimmed().toStdString(); });

    auto* port = new QSpinBox;
    port->setObjectName("m_spinBox_Port");
    port->setRange(1, 65535);
    port->setValue(pNatus->m_port);
    port->setToolTip(tr("The port the streaming machine sends to"));
    connect(port, &QSpinBox::valueChanged, this, [pNatus](int value) { pNatus->m_port = static_cast<std::uint16_t>(value); });

    auto* channels = new QSpinBox;
    channels->setObjectName("m_spinBox_NumberChannels");
    channels->setRange(1, 1024);
    channels->setValue(pNatus->m_iNumberChannels);
    connect(channels, &QSpinBox::valueChanged, this, [pNatus](int value) { pNatus->m_iNumberChannels = value; });

    auto* rate = new QSpinBox;
    rate->setObjectName("m_spinBox_SamplingFreq");
    rate->setRange(1, 100000);
    rate->setSuffix(" Hz");
    rate->setValue(pNatus->m_iSamplingFreq);
    rate->setToolTip(tr("The sampling rate set on the amplifier; the datagrams do not carry it"));
    connect(rate, &QSpinBox::valueChanged, this, [pNatus](int value) { pNatus->m_iSamplingFreq = value; });

    auto* block = new QSpinBox;
    block->setObjectName("m_spinBox_SamplesPerBlock");
    block->setRange(1, 100000);
    block->setValue(pNatus->m_iSamplesPerBlock);
    connect(block, &QSpinBox::valueChanged, this, [pNatus](int value) { pNatus->m_iSamplesPerBlock = value; });

    auto* simulate = new QCheckBox(tr("Simulate the streaming machine"));
    simulate->setObjectName("m_checkBox_Simulate");
    simulate->setChecked(pNatus->m_bSimulate);
    connect(simulate, &QCheckBox::toggled, this, [pNatus](bool on) { pNatus->m_bSimulate = on; });

    auto* form = new QFormLayout(this);
    form->addRow(tr("Address"), address);
    form->addRow(tr("Port"), port);
    form->addRow(tr("Channels"), channels);
    form->addRow(tr("Sampling rate"), rate);
    form->addRow(tr("Samples per block"), block);
    form->addRow(simulate);
}
