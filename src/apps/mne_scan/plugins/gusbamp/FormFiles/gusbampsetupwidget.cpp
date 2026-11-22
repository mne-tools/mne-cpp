//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     gusbampsetupwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     November, 2015
 * @brief    Contains the implementation of the GUSBAmpSetupWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gusbampsetupwidget.h"
#include "../gusbamp.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace GUSBAMPPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GUSBAmpSetupWidget::GUSBAmpSetupWidget(GUSBAmp* pGUSBAmp, QWidget* parent)
: QWidget(parent)
{
    auto* serials = new QLineEdit(QString::fromStdString(pGUSBAmp->m_serials));
    serials->setObjectName("m_lineEdit_Serials");
    serials->setPlaceholderText(tr("first connected unit"));
    serials->setToolTip(tr("Serial numbers separated by commas, master first, e.g. UB-2015.05.16,UB-2015.05.17"));
    connect(serials, &QLineEdit::textChanged, this, [pGUSBAmp](const QString& text) { pGUSBAmp->m_serials = QString(text).remove(QLatin1Char(' ')).toStdString(); });

    auto* inputs = new QLineEdit(QString::fromStdString(pGUSBAmp->m_inputs));
    inputs->setObjectName("m_lineEdit_Inputs");
    inputs->setToolTip(tr("Inputs 1..16 of every unit, e.g. 1-8,11"));
    connect(inputs, &QLineEdit::textChanged, this, [pGUSBAmp](const QString& text) { pGUSBAmp->m_inputs = QString(text).remove(QLatin1Char(' ')).toStdString(); });

    auto* rate = new QComboBox;
    rate->setObjectName("m_comboBox_SampleRate");
    for (const int hz : {32, 64, 128, 256, 512, 600, 1200, 2400, 4800, 9600, 19200, 38400}) {
        rate->addItem(QString::number(hz), hz);
    }
    rate->setCurrentIndex(rate->findData(pGUSBAmp->m_iSampleRate));
    connect(rate, &QComboBox::currentIndexChanged, this, [pGUSBAmp, rate] { pGUSBAmp->m_iSampleRate = rate->currentData().toInt(); });

    auto* block = new QSpinBox;
    block->setObjectName("m_spinBox_SamplesPerBlock");
    block->setRange(1, 10000);
    block->setValue(pGUSBAmp->m_iSamplesPerBlock);
    connect(block, &QSpinBox::valueChanged, this, [pGUSBAmp](int value) { pGUSBAmp->m_iSamplesPerBlock = value; });

    auto* trigger = new QCheckBox(tr("Trigger line of the master as STI 014"));
    trigger->setObjectName("m_checkBox_Trigger");
    trigger->setChecked(pGUSBAmp->m_bTrigger);
    connect(trigger, &QCheckBox::toggled, this, [pGUSBAmp](bool on) { pGUSBAmp->m_bTrigger = on; });

    auto* simulate = new QCheckBox(tr("Simulate the amplifiers"));
    simulate->setObjectName("m_checkBox_Simulate");
    simulate->setChecked(pGUSBAmp->m_bSimulate);
    connect(simulate, &QCheckBox::toggled, this, [pGUSBAmp](bool on) { pGUSBAmp->m_bSimulate = on; });

    auto* form = new QFormLayout(this);
    form->addRow(tr("Serial numbers"), serials);
    form->addRow(tr("Inputs"), inputs);
    form->addRow(tr("Sample rate [Hz]"), rate);
    form->addRow(tr("Samples per block"), block);
    form->addRow(trigger);
    form->addRow(simulate);
}
