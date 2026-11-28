//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     realtimespectrumwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2014
 * @brief    Definition of the RealTimeSpectrumWidget Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimespectrumwidget.h"

#include <mne/viz/backends/qt/kit/panelwidget.h>
#include <mne/viz/panels/spectrum_panel.h>
#include <scMeas/realtimespectrum.h>
#include <math.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVBoxLayout>
#include <QSettings>
#include <QSlider>
#include <QAction>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scan;
using namespace mne::viz;
using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeSpectrumWidget::RealTimeSpectrumWidget(QSharedPointer<RealTimeSpectrum> pFS,
                                               QSharedPointer<QTime>& pTime,
                                               QWidget* parent)
: MeasurementWidget(parent)
, m_pFS(pFS)
, m_fLowerFrqBound(0)
, m_fUpperFrqBound(300)
{
    Q_UNUSED(pTime)

    m_pSpectrum = std::make_shared<viz::SpectrumPanel>();
    m_pSpectrum->set_bounds_callback([this](float lower, float upper) {
        m_fLowerFrqBound = lower;
        m_fUpperFrqBound = upper;
    });
    m_pSpectrumView = new PanelWidget(m_pSpectrum, "MNESCAN", this);

    //set vertical layout
    QVBoxLayout* neLayout = new QVBoxLayout(this);
    neLayout->addWidget(m_pSpectrumView);

    //set layouts
    this->setLayout(neLayout);

    getData();
}

//=============================================================================================================

RealTimeSpectrumWidget::~RealTimeSpectrumWidget()
{
    // Save Settings
    // Same as RealTimeEvokedSetWidget: m_pFS is only set once a measurement
    // arrives, so it is still null if the widget is destroyed before that.
    if (m_pFS && !m_pFS->getName().isEmpty()) {
        QString t_sFSName = m_pFS->getName();

        QSettings settings("MNECPP");

        settings.setValue(QString("MNESCAN/RTSW/lowerFrqBound").arg(t_sFSName), m_fLowerFrqBound);
        settings.setValue(QString("MNESCAN/RTSW/upperFrqBound").arg(t_sFSName), m_fUpperFrqBound);
    }
}

//=============================================================================================================

void RealTimeSpectrumWidget::update(mne::scan::Measurement::SPtr)
{
    getData();
}

//=============================================================================================================

void RealTimeSpectrumWidget::getData()
{
    if (m_pFS->isInit()) {
        if (!m_bDisplayWidgetsInitialized) {
            initDisplayControllWidgets();
        }

        m_pSpectrum->add_data(m_pFS->getValue());
    } else {
        m_pSpectrum->add_data(m_pFS->getValue());
    }
}

//=============================================================================================================

void RealTimeSpectrumWidget::initDisplayControllWidgets()
{
    if (m_pFS->getFiffInfo()) {
        QSettings settings("MNECPP");
        if (!m_pFS->getName().isEmpty()) {
            QString t_sFSName = m_pFS->getName();
            m_fLowerFrqBound = settings.value(QString("MNESCAN/RTSW/lowerFrqBound").arg(t_sFSName), 0).toFloat();
            m_fUpperFrqBound = settings.value(QString("MNESCAN/RTSW/upperFrqBound").arg(t_sFSName), 300).toFloat();
        }

        m_pSpectrum->init(m_pFS->getFiffInfo(), m_pFS->getScaleType() ? viz::SpectrumTable::Scale::Log : viz::SpectrumTable::Scale::Linear);
        initSettingsWidget();

        m_bDisplayWidgetsInitialized = true;
    }
}

//=============================================================================================================

void RealTimeSpectrumWidget::initSettingsWidget()
{
    m_fUpperFrqBound = std::max(m_fLowerFrqBound, m_fUpperFrqBound);
    m_pSpectrum->set_boundaries(m_fLowerFrqBound, m_fUpperFrqBound);
}

//=============================================================================================================

bool RealTimeSpectrumWidget::eventFilter(QObject* object, QEvent* event)
{
    return QWidget::eventFilter(object, event);
}
