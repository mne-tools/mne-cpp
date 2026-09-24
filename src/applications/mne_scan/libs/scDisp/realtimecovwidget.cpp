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

#include <scMeas/realtimecov.h>

#include <disp/viewers/modalityselectionview.h>
#include <disp/plots/imagesc.h>

#include <fiff/fiff_info.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QLabel>
#include <QFont>
#include <QDebug>
#include <QVBoxLayout>
#include <QSharedPointer>
#include <QAction>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace SCMEASLIB;
using namespace DISPLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeCovWidget::RealTimeCovWidget(QSharedPointer<QTime> &pTime,
                                     QWidget* parent)
: MeasurementWidget(parent)
{
    Q_UNUSED(pTime)

    //set vertical layout
    m_pRtcLayout = new QVBoxLayout(this);

    m_pLabelInit= new QLabel(this);
    m_pLabelInit->setText("Acquiring Data");
    m_pLabelInit->setAlignment(Qt::AlignCenter);
    QFont font;font.setBold(true);font.setPointSize(20);
    m_pLabelInit->setFont(font);
    m_pRtcLayout->addWidget(m_pLabelInit);
    m_pRtcLayout->setContentsMargins(3,0,3,0);

    m_pImageSc = new ImageSc;
    m_pRtcLayout->addWidget(m_pImageSc);

    //set layouts
    this->setLayout(m_pRtcLayout);

    m_modalityMap.insert("EEG", true);
    m_modalityMap.insert("MAG", true);
    m_modalityMap.insert("GRAD", true);
}

//=============================================================================================================

RealTimeCovWidget::~RealTimeCovWidget()
{
}

//=============================================================================================================

void RealTimeCovWidget::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    m_pRTC = qSharedPointerDynamicCast<RealTimeCov>(pMeasurement);

    if(m_pRTC->isInitialized()) {
        m_pFiffInfo = m_pRTC->getFiffInfo();

        if(!m_bDisplayWidgetsInitialized) {
            initDisplayControllWidgets();
        }

        MatrixXd matData(m_qListSelChannel.size(), m_qListSelChannel.size());

        for(int i = 0; i < m_qListSelChannel.size(); i++) {
            for(int j = 0; j < m_qListSelChannel.size(); j++) {
                matData(i,j) = m_pRTC->getValue()->data(m_qListSelChannel.at(i),m_qListSelChannel.at(j));
            }
        }

        m_pImageSc->updateData(matData);
    }
}

//=============================================================================================================

void RealTimeCovWidget::initDisplayControllWidgets()
{
    if(m_pFiffInfo) {
        m_pRtcLayout->removeWidget(m_pLabelInit);
        m_pLabelInit->hide();

        m_pImageSc->setTitle(m_pRTC->getName());

        onNewModalitySelection(m_modalityMap);

        //Init control widgets
        QList<QWidget*> lControlWidgets;

        DISPLIB::ModalitySelectionView* pModalitySelectionWidget = new ModalitySelectionView(m_pRTC->getFiffInfo()->chs,
                                                                                             QString("MNESCAN/RTCW"));
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

void RealTimeCovWidget::onNewModalitySelection(const QMap<QString, bool> &modalityMap)
{
    if(m_pRTC && m_pFiffInfo) {
        QStringList chNames = m_pRTC->getValue()->names;
        m_qListSelChannel.clear();

        for(qint32 i = 0; i < chNames.size(); ++i) {
            int unit = m_pFiffInfo->chs.at(m_pFiffInfo->ch_names.indexOf(chNames.at(i))).unit;

            if(unit == FIFF_UNIT_T && modalityMap["MAG"]) {
                m_qListSelChannel.append(i);
            }

            if(unit == FIFF_UNIT_T_M && modalityMap["GRAD"]) {
                m_qListSelChannel.append(i);
            }

            if(unit == FIFF_UNIT_V && (modalityMap["EEG"] ||
                                      modalityMap["EOG"] ||
                                      modalityMap["STIM"] ||
                                      modalityMap["MISC"])) {
                m_qListSelChannel.append(i);
            }
        }
    }
}
