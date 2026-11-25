//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     bar.cpp
 * @author   Ricky Tjen <ricky270@student.sgu.ac.id>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     April 2016
 * @brief    Implementation of the Bar histogram widget (axis layout and bar drawing).
 *
 * Computes the per-bin pixel rectangles from the class limits and paints
 * the bars, the class-limit labels and the axes in @c paintEvent().
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bar.h"
#include <mne/disp/plots/helpers/scenepainter.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QPaintEvent>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::disp;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Bar::Bar(const QString& title,
         QWidget* parent)
: QWidget(parent)
{
    m_chart.set_title(title.toStdString());
    setMinimumSize(200, 150);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

//=============================================================================================================

void Bar::setXLabel(const QString& label)
{
    m_chart.set_x_label(label.toStdString());
    update();
}

//=============================================================================================================

void Bar::setYLabel(const QString& label)
{
    m_chart.set_y_label(label.toStdString());
    update();
}

//=============================================================================================================

void Bar::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    const QtTextMetrics metrics(font());
    scene::Canvas canvas(width(), height());
    m_chart.draw(canvas, metrics, toSceneFont(font()));
    paintScene(painter, canvas.display_list(), font());
}
