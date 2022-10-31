//=============================================================================================================
/**
 * @file     tfplot.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     September, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lorenz Esch. All rights reserved.
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
 * @brief    TFplot class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tfplot.h"

#include "helpers/colormap.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TFplot::TFplot(Eigen::MatrixXd tf_matrix,
               qreal sample_rate,
               qreal lower_frq,
               qreal upper_frq,
               ColorMaps cmap = Jet)
{
    qreal max_frq = sample_rate/2.0;
    qreal frq_per_px = max_frq/tf_matrix.rows();

    if(upper_frq > max_frq || upper_frq <= 0) upper_frq = max_frq;
    if(lower_frq < 0 || lower_frq >= max_frq) lower_frq = 0;
    if(upper_frq < lower_frq) {
        qreal temp = upper_frq;
        upper_frq = lower_frq;
        lower_frq = temp;
    }

    qint32 lower_px = floor(lower_frq / frq_per_px);
    qint32 upper_px = floor(upper_frq / frq_per_px);

    Eigen::MatrixXd zoomed_tf_matrix = Eigen::MatrixXd::Zero(upper_px-lower_px, tf_matrix.cols());
    //How to print to console here
    //printf(("fff   "+QString::number(zoomed_tf_matrix(12,12))).toUtf8().data());// << ";  " << zoomed_tf_matrix.rows(2) << ";   ";

    qint32 pxls = 0;
    for(qint32 it = lower_px; it < upper_px; it++) {
        zoomed_tf_matrix.row(pxls) = tf_matrix.row(it);
        pxls++;
    }

    //zoomed_tf_matrix = tf_matrix.block(tf_matrix.rows() - upper_px, 0, upper_px-lower_px, tf_matrix.cols());

    calc_plot(zoomed_tf_matrix, sample_rate, cmap, lower_frq, upper_frq);
}

//=============================================================================================================

TFplot::TFplot(Eigen::MatrixXd tf_matrix,
               qreal sample_rate,
               ColorMaps cmap = Jet)
{   
    calc_plot(tf_matrix, sample_rate, cmap, 0, 0);
}

//=============================================================================================================

void TFplot::calc_plot(Eigen::MatrixXd tf_matrix,
                       qreal sample_rate,
                       ColorMaps cmap,
                       qreal lower_frq = 0,
                       qreal upper_frq = 0)
{
    //normalisation of the tf-matrix
    qreal norm1 = tf_matrix.maxCoeff();
    qreal mnorm = tf_matrix.minCoeff();
    if(std::fabs(mnorm) > norm1) norm1 = mnorm;
    tf_matrix /= norm1;

    //setup image
    QImage * image_to_tf_plot = new QImage(tf_matrix.cols(), tf_matrix.rows(), QImage::Format_RGB32);

    //setup pixelcolors in image
    QColor color;
    for ( qint32 y = 0; y < tf_matrix.rows(); y++ ) {
        for ( qint32 x = 0; x < tf_matrix.cols(); x++ ) {
            switch  (cmap) {
                case Jet:
                    color.setRgb(ColorMap::valueToJet(std::fabs(tf_matrix(y, x))));
                    break;
                case Hot:
                    color.setRgb(ColorMap::valueToHot(std::fabs(tf_matrix(y, x))));
                    break;
                case HotNeg1:
                    color.setRgb(ColorMap::valueToHotNegative1(std::fabs(tf_matrix(y, x))));
                    break;
                case HotNeg2:
                    color.setRgb(ColorMap::valueToHotNegative2(std::fabs(tf_matrix(y, x))));
                    break;
                case Bone:
                    color.setRgb(ColorMap::valueToBone(std::fabs(tf_matrix(y, x))));
                    break;
                case RedBlue:
                    color.setRgb(ColorMap::valueToRedBlue(std::fabs(tf_matrix(y, x))));
                    break;
            }
            image_to_tf_plot->setPixel(x, tf_matrix.rows() - 1 -  y,  color.rgb());
        }
    }

     *image_to_tf_plot = image_to_tf_plot->scaled(tf_matrix.cols(), tf_matrix.cols()/2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
     *image_to_tf_plot = image_to_tf_plot->scaledToWidth(/*0.9 **/ 1026, Qt::SmoothTransformation);
    //image to pixmap
    QGraphicsPixmapItem *tf_pixmap = new QGraphicsPixmapItem(QPixmap::fromImage(*image_to_tf_plot));
    //tf_pixmap->setScale(100);
    QGraphicsScene *tf_scene = new QGraphicsScene();
    tf_scene->addItem(tf_pixmap);

    QImage * coeffs_image = new QImage(10, tf_matrix.rows(), QImage::Format_RGB32);
    qreal norm = tf_matrix.maxCoeff();
    for(qint32 it = 0; it < tf_matrix.rows(); it++) {
        for ( qint32 x = 0; x < 10; x++ ) {
            switch  (cmap) {
                case Jet:
                    color.setRgb(ColorMap::valueToJet(it*norm/tf_matrix.rows()));
                    break;
                case Hot:
                    color.setRgb(ColorMap::valueToHot(it*norm/tf_matrix.rows()));
                    break;
                case HotNeg1:
                    color.setRgb(ColorMap::valueToHotNegative1(it*norm/tf_matrix.rows()));
                    break;
                case HotNeg2:
                    color.setRgb(ColorMap::valueToHotNegative2(it*norm/tf_matrix.rows()));
                    break;
                case Bone:
                    color.setRgb(ColorMap::valueToBone(it*norm/tf_matrix.rows()));
                    break;
                case RedBlue:
                    color.setRgb(ColorMap::valueToRedBlue(it*norm/tf_matrix.rows()));
                    break;
              }
              coeffs_image->setPixel(x, tf_matrix.rows() - 1 -  it,  color.rgb());
        }
    }

     *coeffs_image = coeffs_image->scaled(10, tf_matrix.cols()/2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
     *coeffs_image = coeffs_image->scaledToHeight(image_to_tf_plot->height(), Qt::SmoothTransformation);

    QLayout * layout = new QGridLayout();
    QGraphicsView * view = new QGraphicsView();
    view->setObjectName("tf_view");
    view->setScene(tf_scene);
    QLinearGradient lgrad(tf_scene->sceneRect().topLeft(), tf_scene->sceneRect().bottomRight());
              lgrad.setColorAt(0.0, Qt::white);
              lgrad.setColorAt(1.0, Qt::lightGray);

    tf_scene->setBackgroundBrush(lgrad);//Qt::white);

    //setup x-axis
    QGraphicsTextItem *x_axis_name = new QGraphicsTextItem("time [sec]", tf_pixmap);
    x_axis_name->setFont(QFont("arial", 14));

    QList<QGraphicsItem *> x_axis_values;
    QList<QGraphicsItem *> x_axis_lines;

    qreal scaleXText = (tf_matrix.cols() - 1) /  sample_rate / 20.0;                       // divide signallength

    for(qint32 j = 0; j < 21; j++) {
        QGraphicsTextItem *text_item = new QGraphicsTextItem(QString::number(j * scaleXText, 'f', 2), tf_pixmap);
        text_item->setFont(QFont("arial", 10));
        x_axis_values.append(text_item);    // scalevalue as string
        QGraphicsLineItem *x_line_item = new QGraphicsLineItem(tf_pixmap);
        x_line_item->setLine(0,-3,0,3);
        x_line_item->setPen(QPen(Qt::darkGray, 2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        x_axis_lines.append(x_line_item);                    // scalelines
    }

    x_axis_name->setPos(tf_pixmap->boundingRect().width()/2 - x_axis_name->boundingRect().width()/2,
                        tf_pixmap->boundingRect().height() + 0.8 * x_axis_values.at(0)->boundingRect().height());

    qreal scale_x = qreal(tf_pixmap->boundingRect().width()) / qreal(x_axis_values.length()-1);

    for(qint32 i = 0; i < x_axis_values.length(); i++) {
       x_axis_values.at(i)->setPos(qreal(i)*scale_x - x_axis_values.at(0)->boundingRect().width()/2,
                                   tf_pixmap->boundingRect().height());
       x_axis_lines.at(i)->setPos(qreal(i)*scale_x,
                                  tf_pixmap->boundingRect().height());

    }//end x axis

    //y-axis
    QGraphicsTextItem *y_axis_name = new QGraphicsTextItem("frequency [Hz]", tf_pixmap);
    y_axis_name->setFont(QFont("arial", 14));

    QList<QGraphicsItem *> y_axis_values;
    QList<QGraphicsItem *> y_axis_lines;

    qreal scale_y_text = 0;

    if(lower_frq == 0  && upper_frq == 0) {
        scale_y_text = 0.5* sample_rate / 10.0;                       // divide signallength
    } else {
        scale_y_text = (upper_frq - lower_frq) / 10.0;
    }

    for(qint32 j = 0; j < 11; j++) {
        QGraphicsTextItem *text_item = new QGraphicsTextItem(QString::number(lower_frq + j*scale_y_text,//pow(10, j)/pow(10, 11) /*(j+1)/log(12)*/ * max_frequency,//scale_y_text,
                                                                             'f', 0), tf_pixmap);
        text_item->setFont(QFont("arial", 10));
        y_axis_values.append(text_item);    // scalevalue as string
        QGraphicsLineItem *y_line_item = new QGraphicsLineItem(tf_pixmap);
        y_line_item->setLine(-3,0,3,0);
        y_line_item->setPen(QPen(Qt::darkGray, 2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        y_axis_lines.append(y_line_item);                    // scalelines
    }

    y_axis_name->setPos(- x_axis_values.at(0)->boundingRect().width() - y_axis_name->boundingRect().height(),
                        tf_pixmap->boundingRect().height()/2 + y_axis_name->boundingRect().height()/2);
    y_axis_name->setRotation(-90);

    qreal scale_y = qreal(tf_pixmap->boundingRect().height()) / qreal(y_axis_values.length()-1);

    for(qint32 i = 0; i < y_axis_values.length(); i++) {
       y_axis_values.at(i)->setPos( -y_axis_values.last()->boundingRect().width()
                                    -0.5*y_axis_lines.last()->boundingRect().width()
                                    -1
                                   ,tf_pixmap->boundingRect().height()
                                    - y_axis_values.last()->boundingRect().height()/2
                                    - qreal(i)*scale_y);
       y_axis_lines.at(i)->setPos(0, qreal(i)*scale_y);
    }
    //end y axis

    QGraphicsPixmapItem * coeffs_item = tf_scene->addPixmap(QPixmap::fromImage(*coeffs_image));//addItem();
    coeffs_item->setParentItem(tf_pixmap);
    coeffs_item->setPos(tf_pixmap->boundingRect().width() +5, 0);

    QGraphicsSimpleTextItem *axis_name_item = new QGraphicsSimpleTextItem("coefficients", coeffs_item);
    QGraphicsSimpleTextItem *axis_zero_item = new QGraphicsSimpleTextItem("0", coeffs_item);
    QGraphicsSimpleTextItem *axis_one_item = new QGraphicsSimpleTextItem("1", coeffs_item);
    axis_name_item->setFont(QFont("arial", 14));
    axis_zero_item->setFont(QFont("arial", 10));
    axis_one_item->setFont(QFont("arial", 10));

    axis_name_item->setPos(coeffs_item->boundingRect().width() + 1,
                        coeffs_item->boundingRect().height()/2 + axis_name_item->boundingRect().height()/2);
    axis_name_item->setRotation(-90);
    axis_zero_item->setPos( 1 + coeffs_item->boundingRect().width(),
                           coeffs_item->boundingRect().height()- axis_zero_item->boundingRect().height());
    axis_one_item->setPos( 1 + coeffs_item->boundingRect().width(), 0);
    //end coeffs picture

    view->fitInView(layout->contentsRect(),Qt::KeepAspectRatio);
    layout->addWidget(view);
    this->setLayout(layout);
}

//=============================================================================================================

void TFplot::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);   

    QWidget *widget = this->layout()->itemAt(0)-> widget();
    if (widget != NULL ) {
        QGraphicsView* view = (QGraphicsView*)widget;
        view->fitInView(view->sceneRect(),Qt::KeepAspectRatio);
    }
}

