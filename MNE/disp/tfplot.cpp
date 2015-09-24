//=============================================================================================================
/**
* @file     mainwindow.cpp
* @author   Martin Henfling <martin.henfling@tu-ilmenau.de>;
*           Daniel Knobl <daniel.knobl@tu-ilmenau.de>;
* @version  1.0
* @date     September, 2015
*
* @section  LICENSE
*
* Copyright (C) 2014, Martin Henfling and Daniel Knobl. All rights reserved.
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
* @brief    Implementation of time-frequency plot class.
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tfplot.h"
#include "math.h"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

TFplot::TFplot(MatrixXd tf_matrix, qreal sample_rate, qint32 width, ColorMaps cmap = ColorMaps::Jet)
{
    //setup image
    QImage *image_to_tf_plot = new QImage(tf_matrix.cols(), tf_matrix.rows(), QImage::Format_RGB32);

    //setup pixelcolors in image
    QColor color;
    for ( qint32 y = 0; y < tf_matrix.rows(); y++ )
        for ( qint32 x = 0; x < tf_matrix.cols(); x++ )
        {
            switch  (cmap)
            {
                case Jet:
                    color.setRgb(ColorMap::valueToJet(abs(tf_matrix(y, x))));
                    break;
                case Hot:
                    color.setRgb(ColorMap::valueToHot(abs(tf_matrix(y, x))));
                    break;
                case HotNeg1:
                    color.setRgb(ColorMap::valueToHotNegative1(abs(tf_matrix(y, x))));
                    break;
                case HotNeg2:
                    color.setRgb(ColorMap::valueToHotNegative2(abs(tf_matrix(y, x))));
                    break;
                case Bone:
                    color.setRgb(ColorMap::valueToBone(abs(tf_matrix(y, x))));
                    break;
                case RedBlue:
                    color.setRgb(ColorMap::valueToRedBlue(abs(tf_matrix(y, x))));
                    break;
            }
            image_to_tf_plot->setPixel(x, tf_matrix.rows() - 1 -  y,  color.rgb());
        }

    *image_to_tf_plot = image_to_tf_plot->scaledToWidth(0.9 * width, Qt::SmoothTransformation);
    //image to pixmap
    QGraphicsPixmapItem *tf_pixmap = new QGraphicsPixmapItem(QPixmap::fromImage(*image_to_tf_plot));
    //tf_pixmap->setScale(6);
    QGraphicsScene *tf_scene = new QGraphicsScene();
    tf_scene->addItem(tf_pixmap);

    QImage *coeffs_image = new QImage(10, tf_matrix.rows(), QImage::Format_RGB32);
    qreal norm = tf_matrix.maxCoeff();
    for(qint32 it = 0; it < tf_matrix.rows(); it++)
    {
        for ( qint32 x = 0; x < 10; x++ )
        {
            switch  (cmap)
            {
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

    qreal scaleXText = (tf_matrix.rows() - 1) /  sample_rate / 20.0;                       // divide signallength

    for(qint32 j = 0; j < 21; j++)
    {
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

    for(qint32 i = 0; i < x_axis_values.length(); i++)
    {
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

    qreal scale_y_text = 0.5* sample_rate / 10.0;                       // divide signallength

    for(qint32 j = 0; j < 11; j++)
    {
        QGraphicsTextItem *text_item = new QGraphicsTextItem(QString::number(j*scale_y_text,//pow(10, j)/pow(10, 11) /*(j+1)/log(12)*/ * max_frequency,//scale_y_text,
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


    for(qint32 i = 0; i < y_axis_values.length(); i++)
    {
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
    axis_one_item->setPos( 1 + coeffs_item->boundingRect().width(),
                           0);
    //end coeffs picture


    layout->addWidget(view);
    this->setLayout(layout);
}

//-----------------------------------------------------------------------------------------------------------------

inline VectorXd TFplot::gauss_window (qint32 sample_count, qreal scale, quint32 translation)
{
    VectorXd gauss = VectorXd::Zero(sample_count);

    for(qint32 n = 0; n < sample_count; n++)
    {
        qreal t = (qreal(n) - translation) / scale;
        gauss[n] = exp(-3.14 * pow(t, 2))*pow(sqrt(scale),(-1))*pow(qreal(2),(0.25));
    }

    return gauss;
}

//-----------------------------------------------------------------------------------------------------------------

inline MatrixXd TFplot::make_STFT(VectorXd signal)
{
    Eigen::FFT<double> fft;
    MatrixXd tf_matrix = MatrixXd::Zero(signal.rows(), signal.rows());

    for(qint32 translate = 0; translate < signal.rows(); translate++)
    {
        //VectorXd windowed_sig = VectorXd::Zero(signal.rows());
        VectorXcd fft_sig = VectorXcd::Zero(signal.rows());
        VectorXcd fft_env = VectorXcd::Zero(signal.rows());
        VectorXcd fft_e_sig = VectorXcd::Zero(signal.rows());
        VectorXd envelope = gauss_window(signal.rows(), signal.rows(), translate);
        VectorXd coeffs = VectorXd::Zero(signal.rows());

        //for(qint32 sample = 0; sample < signal_vector.rows(); sample++)
        //{
        //    windowed_sig = signal_vector[sample] * envelope[sample];
        //}
        fft.fwd(fft_sig, signal);
        fft.fwd(fft_env, envelope);
        for( qint32 m = 0; m < signal.rows(); m++)
            fft_e_sig[m] = fft_sig[m] * conj(fft_env[m]);

        fft.inv(coeffs, fft_e_sig);
        tf_matrix.row(translate) = coeffs;
    }
    return tf_matrix;
}
