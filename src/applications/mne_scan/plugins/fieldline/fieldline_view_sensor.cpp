//=============================================================================================================
/**
 * @file     fieldline_view_sensor.cpp
 * @author   Juan Garcia-Prieto <jgarciaprieto@mgh.harvard.edu>
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     February, 2023
 *
 * @section  LICENSE
 *
 * Copyright (C) 2023, Gabriel Motta, Juan Garcia-Prieto. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief     FieldlineView Sensor class declaration.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================


#include <iostream>

// #include "fieldline/fieldline.h"
// #include "fieldline/fieldline_view.h"
#include "fieldline/fieldline_view_sensor.h"
#include "fieldline/fieldline_view_chassis.h"
#include "formfiles/ui_fieldline_view_sensor.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================
//
#include <QWidget>
#include <QGraphicsScene>
#include <QScrollBar>
// #include <QLabel>
// #include <QVBoxLayout>
// #include <QMouseEvent>
// #include <QDebug>
// #include <QMenu>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================


namespace FIELDLINEPLUGIN {

//=============================================================================================================

FieldlineViewSensor::FieldlineViewSensor(FieldlineViewChassis *parent, int index)
: QWidget(parent),
  m_pFieldlineViewChassis(parent),
  m_pUi(new Ui::uiFieldlineViewSensor),
  m_sensorIndex(index)
{
    m_pUi->setupUi(this);


    m_pScene = new QGraphicsScene();
    m_pUi->ledQGraphView->setStyleSheet("background:transparent");
    m_pUi->ledQGraphView->horizontalScrollBar()->hide();
    m_pUi->ledQGraphView->verticalScrollBar()->hide();
    m_pUi->label->setText(QString::number(index));
    m_pUi->ledQGraphView->setScene(m_pScene);
    // m_pUi->ledQGraphView->fitInView(m_pScene, Qt::KeepAspectRatio);

    m_pCircleLed = m_pScene->addEllipse(0, 0, this->width()/3., this->width()/3.,
                                        QPen(Qt::black), QBrush(QColor(200, 1, 1)));
}

FieldlineViewSensor::~FieldlineViewSensor()
{
    delete m_pUi;
}

void FieldlineViewSensor::resizeEvent(QResizeEvent *event)
{
    // m_pUi->ledQGraphView->fitInView(, Qt::KeepAspectRatio);
    auto bounds = m_pScene->itemsBoundingRect();
    bounds.setWidth(bounds.width() * 1.2);
    bounds.setHeight(bounds.height() * 1.2);
    std::cout << "Resize: " << bounds.width() << " , " << bounds.height() << "\n"; std::cout.flush();
    m_pUi->ledQGraphView->fitInView(bounds, Qt::KeepAspectRatio);

    QWidget::resizeEvent(event);
}


void FieldlineViewSensor::setState(FieldlineSensorStatusType state) {
    m_state = state;
}

FieldlineSensorStatusType FieldlineViewSensor::getState() const {
    return m_state;
}

}  // namespace FIELDLINEPLUGIN


