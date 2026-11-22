//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     brainflowsetupwidget.cpp
 * @author   Andrey Parfenov <a1994ndrey@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     February, 2020
 * @brief    Definition of the BrainFlowSetupWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainflowsetupwidget.h"
#include "../brainflowboard.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QComboBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BRAINFLOWBOARDPLUGIN;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

/** @return A line edit named @p name showing @p value, which it keeps up to date. */
QLineEdit* textField(std::string& value, const char* name, const QString& toolTip, QWidget* parent)
{
    auto* edit = new QLineEdit(QString::fromStdString(value), parent);
    edit->setObjectName(name);
    edit->setToolTip(toolTip);
    QObject::connect(edit, &QLineEdit::textChanged, parent, [&value](const QString& text) { value = text.trimmed().toStdString(); });
    return edit;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainFlowSetupWidget::BrainFlowSetupWidget(BrainFlowBoard* board, QWidget* parent)
: QWidget(parent)
{
    auto* boardId = textField(board->m_board, "m_lineEdit_Board", tr("BrainFlow board id or name, e.g. -1, synthetic, cyton_daisy"), this);
    auto* serialPort = textField(board->m_serial_port, "m_lineEdit_SerialPort", tr("Serial port, e.g. /dev/ttyUSB0 or COM3"), this);
    auto* macAddress = textField(board->m_mac_address, "m_lineEdit_MacAddress", tr("Bluetooth MAC address"), this);
    auto* ipAddress = textField(board->m_ip_address, "m_lineEdit_IpAddress", tr("IP address of the board"), this);
    auto* otherInfo = textField(board->m_other_info, "m_lineEdit_OtherInfo", tr("Board-specific settings"), this);
    auto* serialNumber = textField(board->m_serial_number, "m_lineEdit_SerialNumber", tr("Serial number of the board"), this);
    auto* file = textField(board->m_file, "m_lineEdit_File", tr("File, e.g. a recording for the playback board"), this);
    auto* library = textField(board->m_library, "m_lineEdit_Library",
                              tr("BrainFlow's BoardController library; empty to search MNE_CPP_BRAINFLOW_LIB, pip's brainflow package "
                                 "and the system"),
                              this);

    auto* ipPort = new QSpinBox(this);
    ipPort->setObjectName("m_spinBox_IpPort");
    ipPort->setRange(0, 65535);
    ipPort->setSpecialValueText(tr("none"));
    ipPort->setValue(board->m_ip_port);
    connect(ipPort, &QSpinBox::valueChanged, this, [board](int value) { board->m_ip_port = value; });

    auto* ipProtocol = new QComboBox(this);
    ipProtocol->setObjectName("m_comboBox_IpProtocol");
    ipProtocol->addItems({tr("none"), tr("UDP"), tr("TCP")}); // BrainFlow's IpProtocolTypes
    ipProtocol->setCurrentIndex(board->m_ip_protocol);
    connect(ipProtocol, &QComboBox::currentIndexChanged, this, [board](int index) { board->m_ip_protocol = index; });

    auto* block = new QSpinBox(this);
    block->setObjectName("m_spinBox_SamplesPerBlock");
    block->setRange(1, 100000);
    block->setValue(board->m_samples_per_block);
    connect(block, &QSpinBox::valueChanged, this, [board](int value) { board->m_samples_per_block = value; });

    auto* form = new QFormLayout(this);
    form->addRow(tr("Board"), boardId);
    form->addRow(tr("Serial port"), serialPort);
    form->addRow(tr("MAC address"), macAddress);
    form->addRow(tr("IP address"), ipAddress);
    form->addRow(tr("IP port"), ipPort);
    form->addRow(tr("IP protocol"), ipProtocol);
    form->addRow(tr("Other info"), otherInfo);
    form->addRow(tr("Serial number"), serialNumber);
    form->addRow(tr("File"), file);
    form->addRow(tr("BrainFlow library"), library);
    form->addRow(tr("Samples per block"), block);
}
