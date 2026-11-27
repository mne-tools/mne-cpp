//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     imagesc.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Daniel Strohmeier <daniel.strohmeier@gmail.com>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March 2013
 * @brief    Implementation of the ImageSc heat-map widget (matrix-to-image colour mapping and painting).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "imagesc.h"
#include "helpers/scenepainter.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::viz;
using namespace mne::viz::qt;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ImageSc::ImageSc(QWidget* parent)
: Graph(parent)
{
    init();
}

//=============================================================================================================

ImageSc::ImageSc(MatrixXd& p_dMat, QWidget* parent)
: Graph(parent)
{
    init();
    updateData(p_dMat);
}

//=============================================================================================================

ImageSc::ImageSc(MatrixXf& p_fMat, QWidget* parent)
: Graph(parent)
{
    init();
    updateData(p_fMat);
}

//=============================================================================================================

ImageSc::ImageSc(MatrixXi& p_iMat, QWidget* parent)
: Graph(parent)
{
    init();
    updateData(p_iMat);
}

//=============================================================================================================

ImageSc::~ImageSc() = default;

//=============================================================================================================

void ImageSc::init()
{
    Graph::init();
    m_plot = viz::ImagePlot();
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

//=============================================================================================================

void ImageSc::updateData(MatrixXd& p_dMat)
{
    if (p_dMat.rows() > 0 && p_dMat.cols() > 0) {
        std::vector<double> values(static_cast<std::size_t>(p_dMat.size()));
        Map<Matrix<double, Dynamic, Dynamic, RowMajor>>(values.data(), p_dMat.rows(), p_dMat.cols()) = p_dMat;
        m_plot.set_data(static_cast<int>(p_dMat.rows()), static_cast<int>(p_dMat.cols()), std::move(values));
        update();
    }
}

//=============================================================================================================

void ImageSc::updateData(MatrixXf& p_fMat)
{
    MatrixXd t_dMat = p_fMat.cast<double>();
    updateData(t_dMat);
}

//=============================================================================================================

void ImageSc::updateData(MatrixXi& p_iMat)
{
    MatrixXd t_dMat = p_iMat.cast<double>();
    updateData(t_dMat);
}

//=============================================================================================================

void ImageSc::setColorMap(const QString& p_sColorMap)
{
    m_plot.set_colormap(p_sColorMap.toStdString());
    update();
}

//=============================================================================================================

void ImageSc::setExtent(double left, double right, double bottom, double top)
{
    m_plot.set_extent(left, right, bottom, top);
    update();
}

//=============================================================================================================

void ImageSc::setColorRange(double min, double max)
{
    m_plot.set_color_range(min, max);
    update();
}

//=============================================================================================================

void ImageSc::setColorbarLabel(const QString& label)
{
    m_plot.set_colorbar_label(label.toStdString());
    update();
}

//=============================================================================================================

void ImageSc::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    if (!m_plot.has_data()) {
        return;
    }
    m_plot.set_title(m_sTitle.toStdString());
    m_plot.set_x_label(m_sXLabel.toStdString());
    m_plot.set_y_label(m_sYLabel.toStdString());
    QPainter painter(this);
    painter.setRenderHint(QPainter::TextAntialiasing);
    const QtTextMetrics metrics(font());
    viz::Canvas canvas(width(), height());
    m_plot.draw(canvas, metrics, toSceneFont(font()));
    paintScene(painter, canvas.display_list(), font());
}
