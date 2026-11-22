//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     neuromagsetupwidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2014
 * @brief    Contains the implementation of the NeuromagSetupWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuromagsetupwidget.h"
#include "../neuromag.h"

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

using namespace NEUROMAGPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NeuromagSetupWidget::NeuromagSetupWidget(Neuromag* pNeuromag, QWidget* parent)
: QWidget(parent)
{
    auto* root = new QLineEdit(QString::fromStdString(pNeuromag->m_root));
    root->setObjectName("m_lineEdit_Root");
    root->setToolTip(tr("Directory of the acquisition's sockets, shared memory and files"));
    connect(root, &QLineEdit::textChanged, this, [pNeuromag](const QString& text) { pNeuromag->m_root = text.trimmed().toStdString(); });

    auto* buffer = new QSpinBox;
    buffer->setObjectName("m_spinBox_BufferSize");
    buffer->setRange(0, 100000);
    buffer->setSpecialValueText(tr("as set up"));
    buffer->setValue(pNeuromag->m_iBufferSize);
    connect(buffer, &QSpinBox::valueChanged, this, [pNeuromag](int value) { pNeuromag->m_iBufferSize = value; });

    auto* block = new QSpinBox;
    block->setObjectName("m_spinBox_SamplesPerBlock");
    block->setRange(1, 100000);
    block->setValue(pNeuromag->m_iSamplesPerBlock);
    connect(block, &QSpinBox::valueChanged, this, [pNeuromag](int value) { pNeuromag->m_iSamplesPerBlock = value; });

    auto* autostart = new QCheckBox(tr("Start and stop the measurement"));
    autostart->setObjectName("m_checkBox_Autostart");
    autostart->setChecked(pNeuromag->m_bAutostart);
    connect(autostart, &QCheckBox::toggled, this, [pNeuromag](bool on) { pNeuromag->m_bAutostart = on; });

    auto* simulate = new QCheckBox(tr("Simulate the acquisition"));
    simulate->setObjectName("m_checkBox_Simulate");
    simulate->setChecked(pNeuromag->m_bSimulate);
    connect(simulate, &QCheckBox::toggled, this, [pNeuromag](bool on) { pNeuromag->m_bSimulate = on; });

    auto* form = new QFormLayout(this);
    form->addRow(tr("Acquisition directory"), root);
    form->addRow(tr("Samples per data buffer"), buffer);
    form->addRow(tr("Samples per block"), block);
    form->addRow(autostart);
    form->addRow(simulate);
}
