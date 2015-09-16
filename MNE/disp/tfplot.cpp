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



TFplot::TFplot()
{
}

QWidget* TFplot::plotTf(MatrixXd signal_vector, int sample_rate, ColorMaps cmap = ColorMaps::Jet, QWidget *plot_widget = NULL)
{
    //MatrixXd tf_sum = MatrixXd::Zero(floor(signal_vector.rows() / 2), signal_vector.rows());
    //Wignertransform wtransform;
    //MatrixXd tf_matrix = wtransform.wigner_transform(signal_vector);
    MatrixXd tf_matrix = signal_vector;


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
                    color.setRgb(ColorMap::jetR(abs(tf_matrix(y, x))),
                                 ColorMap::jetG(abs(tf_matrix(y, x))),
                                 ColorMap::jetB(abs(tf_matrix(y, x))));
                    break;
                case Hot:
                    color.setRgb(ColorMap::hotR(abs(tf_matrix(y, x))),
                                 ColorMap::hotG(abs(tf_matrix(y, x))),
                                 ColorMap::hotB(abs(tf_matrix(y, x))));
                    break;
                case HotNeg1:
                    color.setRgb(ColorMap::hotRNeg1(abs(tf_matrix(y, x))),
                                 ColorMap::hotGNeg1(abs(tf_matrix(y, x))),
                                 ColorMap::hotBNeg1(abs(tf_matrix(y, x))));
                    break;
                case HotNeg2:
                    color.setRgb(ColorMap::hotRNeg2(abs(tf_matrix(y, x))),
                                 ColorMap::hotGNeg2(abs(tf_matrix(y, x))),
                                 ColorMap::hotBNeg2(abs(tf_matrix(y, x))));
                    break;
                case Bone:
                    color.setRgb(ColorMap::boneR(abs(tf_matrix(y, x))),
                                 ColorMap::boneG(abs(tf_matrix(y, x))),
                                 ColorMap::boneB(abs(tf_matrix(y, x))));
                    break;
                case RedBlue:
                    color.setRgb(ColorMap::rbR(abs(tf_matrix(y, x))),
                                 ColorMap::rbG(abs(tf_matrix(y, x))),
                                 ColorMap::rbB(abs(tf_matrix(y, x))));
                    break;
            }
            image_to_tf_plot->setPixel(x, tf_matrix.rows() - 1 -  y,  color.rgb());
        }

    //image to pixmap
    QGraphicsPixmapItem *tf_pixmap = new QGraphicsPixmapItem(QPixmap::fromImage(*image_to_tf_plot));

    if(plot_widget == NULL)
        plot_widget = new QWidget();

    QGraphicsScene *tf_scene = new QGraphicsScene();
    tf_scene->addItem(tf_pixmap);

    //setup x-axis
    QGraphicsTextItem *x_axis_name = new QGraphicsTextItem("time [sec]", tf_pixmap);
    x_axis_name->setFont(QFont("arial", 3));

    QList<QGraphicsItem *> x_axis_values;
    QList<QGraphicsItem *> x_axis_lines;

    qreal scaleXText = (signal_vector.rows() - 1) /  sample_rate / 20.0;                       // divide signallength

    for(qint32 j = 0; j < 21; j++)
    {
        QGraphicsTextItem *text_item = new QGraphicsTextItem(QString::number(j * scaleXText
                                                                             /*+ _from / _sample_rate*/
                                                                             /*+ _offset_time*/,
                                                                             'f', 2), tf_pixmap);
        text_item->setFont(QFont("arial", 3));
        x_axis_values.append(text_item);    // scalevalue as string
        QGraphicsLineItem *x_line_item = new QGraphicsLineItem(tf_pixmap);
        x_line_item->setLine(0,-1,0,1);
        x_line_item->setPen(QPen(Qt::darkGray, 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        x_axis_lines.append(x_line_item);                    // scalelines
    }

    x_axis_name->setPos(tf_pixmap->boundingRect().width()/2 - x_axis_name->boundingRect().width()/2,
                        tf_pixmap->boundingRect().height() + 0.5*x_axis_values.at(0)->boundingRect().height());

    qreal scale_x = qreal(tf_pixmap->boundingRect().width()) / qreal(x_axis_values.length()-1);

    //dbgout
    /*
    std::cout << "\nimage width=    " << image_to_tf_plot->width() << "\n";
    std::cout << "\npixmap width=    " << tf_pixmap->boundingRect().width() << "\n";
    std::cout << "\nscale x=    " << scale_x << "\n";
    */

    for(qint32 i = 0; i < x_axis_values.length(); i++)
    {
       x_axis_values.at(i)->setPos(qreal(i)*scale_x - x_axis_values.at(0)->boundingRect().width()/2,
                                   tf_pixmap->boundingRect().height());
       x_axis_lines.at(i)->setPos(qreal(i)*scale_x,
                                  tf_pixmap->boundingRect().height());

    }//end x axis

    //y-axis
    QGraphicsTextItem *y_axis_name = new QGraphicsTextItem("frequency [Hz]", tf_pixmap);
    y_axis_name->setFont(QFont("arial", 3));

    QList<QGraphicsItem *> y_axis_values;
    QList<QGraphicsItem *> y_axis_lines;

    qreal scale_y_text = 0.5* sample_rate / 10.0;                       // divide signallength

    for(qint32 j = 0; j < 11; j++)
    {
        QGraphicsTextItem *text_item = new QGraphicsTextItem(QString::number(j*scale_y_text,//pow(10, j)/pow(10, 11) /*(j+1)/log(12)*/ * max_frequency,//scale_y_text,
                                                                             'f', 0), tf_pixmap);
        text_item->setFont(QFont("arial", 3));
        y_axis_values.append(text_item);    // scalevalue as string
        QGraphicsLineItem *y_line_item = new QGraphicsLineItem(tf_pixmap);
        y_line_item->setLine(-1,0,1,0);
        y_line_item->setPen(QPen(Qt::darkGray, 0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        y_axis_lines.append(y_line_item);                    // scalelines
    }

    y_axis_name->setPos(- x_axis_values.at(0)->boundingRect().width() - y_axis_name->boundingRect().height(),
                        tf_pixmap->boundingRect().height()/2 + y_axis_name->boundingRect().height()/2);
    y_axis_name->setRotation(-90);

    /*y_axis_item->setPos(plot_window->tf_scene.width()/2 - test_image->width()/2 - y_axis_item->boundingRect().width()/2 - 10,
                        plot_window->tf_scene.height()/2 - y_axis_item->boundingRect().height()/2);

    */
    qreal scale_y = qreal(tf_pixmap->boundingRect().height()) / qreal(y_axis_values.length()-1);

    //dbgout
    /*
    std::cout << "\nimage heigth=    " << image_to_tf_plot->height() << "\n";
    std::cout << "\npixmap heigth=    " << tf_pixmap->boundingRect().height() << "\n";
    std::cout << "\nscale=    " << scale_y << "\n";
    */

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

    // coefficient colors just for fun whatever
    QImage *coeffs_image = new QImage(10, tf_matrix.rows(), QImage::Format_RGB32);
    qreal norm = tf_matrix.maxCoeff();
    for(qint32 it = 0; it < tf_matrix.rows(); it++)
    {
        for ( qint32 x = 0; x < 10; x++ )
        {
            switch  (cmap)
            {
                case Jet:
                    color.setRgb(ColorMap::jetR(it*norm/tf_matrix.rows()),
                                 ColorMap::jetG(it*norm/tf_matrix.rows()),
                                 ColorMap::jetB(it*norm/tf_matrix.rows()));
                    break;
                case Hot:
                    color.setRgb(ColorMap::hotR(it*norm/tf_matrix.rows()),
                                 ColorMap::hotG(it*norm/tf_matrix.rows()),
                                 ColorMap::hotB(it*norm/tf_matrix.rows()));
                    break;
                case HotNeg1:
                    color.setRgb(ColorMap::hotRNeg1(it*norm/tf_matrix.rows()),
                                 ColorMap::hotGNeg1(it*norm/tf_matrix.rows()),
                                 ColorMap::hotBNeg1(it*norm/tf_matrix.rows()));
                    break;
                case HotNeg2:
                    color.setRgb(ColorMap::hotRNeg2(it*norm/tf_matrix.rows()),
                                 ColorMap::hotGNeg2(it*norm/tf_matrix.rows()),
                                 ColorMap::hotBNeg2(it*norm/tf_matrix.rows()));
                    break;
                case Bone:
                    color.setRgb(ColorMap::boneR(it*norm/tf_matrix.rows()),
                                 ColorMap::boneG(it*norm/tf_matrix.rows()),
                                 ColorMap::boneB(it*norm/tf_matrix.rows()));
                    break;
                case RedBlue:
                    color.setRgb(ColorMap::rbR(it*norm/tf_matrix.rows()),
                                 ColorMap::rbG(it*norm/tf_matrix.rows()),
                                 ColorMap::rbB(it*norm/tf_matrix.rows()));
                    break;
              }
              //color.setRgb(ColorMap::hotR(it*norm/tf_matrix.rows()), ColorMap::hotG(it*norm/tf_sum.rows()), ColorMap::hotB(it*norm/tf_sum.rows()));
              coeffs_image->setPixel(x, tf_matrix.rows() - 1 -  it,  color.rgb());
        }
        /*std::cout << ColorMap::jetR(it*norm/tf_sum.rows()) << " ; " <<
                     ColorMap::jetG(it*norm/tf_sum.rows()) << " ; " <<
                     ColorMap::jetB(it*norm/tf_sum.rows()) << "\n";
        do we have a bug in jet here?
        */
    }
    QGraphicsPixmapItem * coeffs_item = tf_scene->addPixmap(QPixmap::fromImage(*coeffs_image));//addItem();
    coeffs_item->setParentItem(tf_pixmap);
    coeffs_item->setPos(tf_pixmap->boundingRect().width() +5, 0);

    QGraphicsSimpleTextItem *axis_name_item = new QGraphicsSimpleTextItem("coefficients", coeffs_item);
    QGraphicsSimpleTextItem *axis_zero_item = new QGraphicsSimpleTextItem("0", coeffs_item);
    QGraphicsSimpleTextItem *axis_one_item = new QGraphicsSimpleTextItem("1", coeffs_item);
    axis_name_item->setFont(QFont("arial", 3));
    axis_zero_item->setFont(QFont("arial", 3));
    axis_one_item->setFont(QFont("arial", 3));

    axis_name_item->setPos(coeffs_item->boundingRect().width() + 1,
                        coeffs_item->boundingRect().height()/2 + axis_name_item->boundingRect().height()/2);
    axis_name_item->setRotation(-90);
    axis_zero_item->setPos( 1 + coeffs_item->boundingRect().width(),
                           coeffs_item->boundingRect().height()- axis_zero_item->boundingRect().height());
    axis_one_item->setPos( 1 + coeffs_item->boundingRect().width(),
                           0);
    //end coeffs picture

    QLayout * layout = new QGridLayout();
    QGraphicsView * view = new QGraphicsView();
    view->setObjectName("tf_view");
    view->setScene(tf_scene);
    tf_scene->setBackgroundBrush(Qt::lightGray);
    view->scale(plot_widget->size().height() / tf_scene->sceneRect().height()
                                        ,plot_widget->size().height() / tf_scene->sceneRect().height());//scale graphic scene
    layout->addWidget(view);
    plot_widget->setLayout(layout);


    //set Scene to GraphicsView and show
   // plot_widget->
    //plot_window->tf_scene.setSceneRect(plot_window->tf_scene.itemsBoundingRect());                  // Re-shrink the scene to it's bounding contents
   // plot_widget->
    //plot_widget->tf_view->scale(plot_window->size().height() / plot_window->tf_scene.sceneRect().height()
    //                                ,plot_window->size().height() / plot_window->tf_scene.sceneRect().height());//scale graphic scene
    //in the line above you see the problem of scaleing the whole scene after adding all items
    //depending on the signals size the text on the axis is not nice to read or even written over each other
    //otherwise it could happen tohave too small letters to read then.
    //SO SOLVE THIS AS WELL MAN!
    plot_widget->show();

    return plot_widget;
}
