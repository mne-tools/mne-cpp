//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     averageselectionview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the AverageSelectionView picker.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averageselectionview.h"
#include "../detail/layout_reset.h"
#include <mne/disp/detail/map_value.h>
#include <mne/base/log.h>

#include <mne/fiff/fiff_evoked_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QCheckBox>
#include <QColorDialog>
#include <QPalette>
#include <QPushButton>
#include <QSettings>
#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AverageSelectionView::AverageSelectionView(const QString& sSettingsPath,
                                           QWidget* parent,
                                           Qt::WindowFlags f)
: AbstractView(parent, f)
, m_iMaxNumAverages(10)
, m_qMapAverageColor(std::make_shared<std::map<std::string, QColor>>())
, m_qMapAverageActivation(std::make_shared<std::map<std::string, bool>>())
{
    m_sSettingsPath = sSettingsPath;
    this->setWindowTitle("Average Selection");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    loadSettings();
    redrawGUI();
}

//=============================================================================================================

AverageSelectionView::~AverageSelectionView()
{
    saveSettings();
}

//=============================================================================================================

std::shared_ptr<std::map<std::string, QColor>> AverageSelectionView::getAverageColor() const
{
    return m_qMapAverageColor;
}

//=============================================================================================================

std::shared_ptr<std::map<std::string, bool>> AverageSelectionView::getAverageActivation() const
{
    return m_qMapAverageActivation;
}

//=============================================================================================================

void AverageSelectionView::setAverageColor(const std::shared_ptr<std::map<std::string, QColor>>& qMapAverageColor)
{
    m_qMapAverageColor = qMapAverageColor;
    redrawGUI();
}

//=============================================================================================================

void AverageSelectionView::setAverageActivation(const std::shared_ptr<std::map<std::string, bool>>& qMapAverageActivation)
{
    m_qMapAverageActivation = qMapAverageActivation;
    redrawGUI();
}

//=============================================================================================================

void AverageSelectionView::saveSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.beginGroup(m_sSettingsPath + QString("/AverageSelectionView/averageColorMap"));
    std::map<std::string, QColor>::const_iterator iColor = m_qMapAverageColor->cbegin();
    while (iColor != m_qMapAverageColor->cend()) {
        settings.setValue(QString::fromStdString(iColor->first), iColor->second);
        ++iColor;
    }
    settings.endGroup();

    settings.beginGroup(m_sSettingsPath + QString("/AverageSelectionView/averageActivationMap"));
    std::map<std::string, bool>::const_iterator iActivation = m_qMapAverageActivation->cbegin();
    while (iActivation != m_qMapAverageActivation->cend()) {
        settings.setValue(QString::fromStdString(iActivation->first), iActivation->second);
        ++iActivation;
    }
    settings.endGroup();
}

//=============================================================================================================

void AverageSelectionView::loadSettings()
{
    if (m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.beginGroup(m_sSettingsPath + QString("/AverageSelectionView/averageColorMap"));
    for (const QString& key : settings.childKeys()) {
        m_qMapAverageColor->insert_or_assign(key.toStdString(), settings.value(key).value<QColor>());
    }
    settings.endGroup();

    settings.beginGroup(m_sSettingsPath + QString("/AverageSelectionView/averageActivationMap"));
    for (const QString& key : settings.childKeys()) {
        m_qMapAverageActivation->insert_or_assign(key.toStdString(), settings.value(key).toBool());
    }
    settings.endGroup();
}

//=============================================================================================================

void AverageSelectionView::updateGuiMode(GuiMode mode)
{
    switch (mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void AverageSelectionView::updateProcessingMode(ProcessingMode mode)
{
    switch (mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void AverageSelectionView::redrawGUI()
{
    if (m_qMapAverageColor->size() != m_qMapAverageActivation->size()) {
        mne::Log::debug("AverageSelectionView::update - m_qMapAverageColor and m_qMapAverageActivation do not match in size. Returning.");
        return;
    }

    //Delete all widgets in the averages layout
    QGridLayout* topLayout = static_cast<QGridLayout*>(this->layout());
    if (!topLayout) {
        topLayout = new QGridLayout(this);
    }

    // The widgets go right away: the view is redrawn synchronously and looked up by name
    detail::clearLayout(topLayout);

    // Create new GUI elements
    int count = 0;
    for (const auto& [key, color] : *m_qMapAverageColor) {
        if (count >= m_iMaxNumAverages) {
            break;
        }

        const QString name = QString::fromStdString(key);

        //Create average active checkbox
        QPointer<QCheckBox> pCheckBox = new QCheckBox(name, this);
        pCheckBox->setChecked(detail::valueOr(*m_qMapAverageActivation, key));
        pCheckBox->setObjectName(name);
        topLayout->addWidget(pCheckBox, count, 0);
        connect(pCheckBox.data(), &QCheckBox::clicked,
                this, &AverageSelectionView::onAverageSelectionColorChanged);

        //Create average color pushbutton
        QPointer<QPushButton> pButton = new QPushButton("Click to change", this);
        pButton->setObjectName(name);
        pButton->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(color.red()).arg(color.green()).arg(color.blue()));
        topLayout->addWidget(pButton, count, 1);
        connect(pButton.data(), &QPushButton::clicked,
                this, &AverageSelectionView::onAverageSelectionColorChanged);

        ++count;
    }
}

//=============================================================================================================

void AverageSelectionView::onAverageSelectionColorChanged()
{
    //Change color for average
    if (QPointer<QPushButton> button = qobject_cast<QPushButton*>(sender())) {
        const std::string sObjectName = button->objectName().toStdString();

        QColor color = QColorDialog::getColor(detail::valueOr(*m_qMapAverageColor, sObjectName), this, "Set average color");
        // Cancelled: getColor() returns an invalid colour
        if (!color.isValid()) {
            return;
        }

        if (button) {
            QPalette palette(QPalette::Button, color);
            button->setPalette(palette);
            button->update();

            //Set color of button new new scene color
            button->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(color.red()).arg(color.green()).arg(color.blue()));
        }

        m_qMapAverageColor->insert_or_assign(sObjectName, color);

        emit newAverageColorMap(m_qMapAverageColor);
    }

    //Change color for average
    if (QPointer<QCheckBox> checkBox = qobject_cast<QCheckBox*>(sender())) {
        const std::string sObjectName = checkBox->objectName().toStdString();

        m_qMapAverageActivation->insert_or_assign(sObjectName, checkBox->isChecked());

        emit newAverageActivationMap(m_qMapAverageActivation);
    }

    saveSettings();
}

//=============================================================================================================

void AverageSelectionView::clearView()
{
}
