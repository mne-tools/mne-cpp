//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     realtimecovwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2014
 * @brief    Definition of the RealTimeCovWidget Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimecovwidget.h"

#include <mne/base/string_utils.h>
#include <scMeas/realtimecov.h>
#include <mne/disp/viewers/modalityselectionview.h>
#include <mne/disp/plots/imagesc.h>
#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QLabel>
#include <QFont>
#include <QDebug>
#include <QVBoxLayout>
#include <QAction>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scan;
using namespace mne::disp;
using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeCovWidget::RealTimeCovWidget(QSharedPointer<QTime>& pTime,
                                     QWidget* parent)
: MeasurementWidget(parent)
{
    Q_UNUSED(pTime)

    //set vertical layout
    m_pRtcLayout = new QVBoxLayout(this);

    m_pLabelInit = new QLabel(this);
    m_pLabelInit->setText("Acquiring Data");
    m_pLabelInit->setAlignment(Qt::AlignCenter);
    QFont font;
    font.setBold(true);
    font.setPointSize(20);
    m_pLabelInit->setFont(font);
    m_pRtcLayout->addWidget(m_pLabelInit);
    m_pRtcLayout->setContentsMargins(3, 0, 3, 0);

    m_pImageSc = new ImageSc(this);
    m_pRtcLayout->addWidget(m_pImageSc);

    //set layouts
    this->setLayout(m_pRtcLayout);

    m_modalityMap.insert_or_assign("EEG", true);
    m_modalityMap.insert_or_assign("MAG", true);
    m_modalityMap.insert_or_assign("GRAD", true);
}

//=============================================================================================================

RealTimeCovWidget::~RealTimeCovWidget()
{
}

//=============================================================================================================

void RealTimeCovWidget::update(mne::scan::Measurement::SPtr pMeasurement)
{
    m_pRTC = qSharedPointerDynamicCast<RealTimeCov>(pMeasurement);

    if (m_pRTC->isInitialized()) {
        m_pFiffInfo = m_pRTC->getFiffInfo();

        if (!m_bDisplayWidgetsInitialized) {
            initDisplayControllWidgets();
        }

        MatrixXd matData(m_qListSelChannel.size(), m_qListSelChannel.size());

        for (int i = 0; i < m_qListSelChannel.size(); i++) {
            for (int j = 0; j < m_qListSelChannel.size(); j++) {
                matData(i, j) = m_pRTC->getValue()->data(m_qListSelChannel.at(i), m_qListSelChannel.at(j));
            }
        }

        m_pImageSc->updateData(matData);
    }
}

//=============================================================================================================

void RealTimeCovWidget::initDisplayControllWidgets()
{
    if (m_pFiffInfo) {
        m_pRtcLayout->removeWidget(m_pLabelInit);
        m_pLabelInit->hide();

        m_pImageSc->setTitle(m_pRTC->getName());

        onNewModalitySelection(m_modalityMap);

        //Init control widgets
        QList<QWidget*> lControlWidgets;

        mne::disp::ModalitySelectionView* pModalitySelectionWidget = new ModalitySelectionView(m_pRTC->getFiffInfo()->chs, QString("MNESCAN/RTCW"), this);
        pModalitySelectionWidget->setObjectName("group_tab_View_Modalities");
        lControlWidgets.append(pModalitySelectionWidget);

        connect(pModalitySelectionWidget, &ModalitySelectionView::modalitiesChanged,
                this, &RealTimeCovWidget::onNewModalitySelection);

        pModalitySelectionWidget->setModalityMap(m_modalityMap);

        emit displayControlWidgetsChanged(lControlWidgets, m_pRTC->getName());

        m_bDisplayWidgetsInitialized = true;
    }
}

//=============================================================================================================

void RealTimeCovWidget::onNewModalitySelection(const std::map<std::string, bool>& modalityMap)
{
    const auto shown = [&modalityMap](const char* modality) {
        const auto it = modalityMap.find(modality);
        return it != modalityMap.end() && it->second;
    };
    if (m_pRTC && m_pFiffInfo) {
        const StringList& chNames = m_pRTC->getValue()->names;
        m_qListSelChannel.clear();

        for (qint32 i = 0; i < static_cast<qint32>(chNames.size()); ++i) {
            int unit = m_pFiffInfo->chs.at(mne::index_of(m_pFiffInfo->ch_names, chNames.at(i))).unit;

            if (unit == FIFF_UNIT_T && shown("MAG")) {
                m_qListSelChannel.append(i);
            }

            if (unit == FIFF_UNIT_T_M && shown("GRAD")) {
                m_qListSelChannel.append(i);
            }

            if (unit == FIFF_UNIT_V && (shown("EEG") || shown("EOG") || shown("STIM") || shown("MISC"))) {
                m_qListSelChannel.append(i);
            }
        }
    }
}
