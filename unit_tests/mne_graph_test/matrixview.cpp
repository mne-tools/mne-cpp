#include "matrixview.h"

#include <QResizeEvent>
#include <QPainter>
#include <QDebug>

#include <iostream>


MatrixView::MatrixView(QWidget *parent)
: QWidget(parent)
, pixmap(NULL)
, m_iBorderTopBottom(50)
, m_iBorderLeftRight(50)
{
    m_qFontAxes.setPixelSize(12);
    m_qPenAxes = QPen(Qt::black);

    m_qFontTitle.setPixelSize(20);
    m_qFontTitle.setBold(true);
    m_qPenTitle = QPen(Qt::black);
}

MatrixView::~MatrixView()
{
//    delete ui;
    if(pixmap)
    {
        delete pixmap;
    }

}

void MatrixView::updateMatrix(MatrixXd &data)
{
    if(pixmap)
    {
        delete pixmap;
        pixmap = NULL;
    }

    if(data.rows() > 0 && data.cols())
    {
        m_sTitle = QString("Test Matrix");

        m_sXLabel = QString("XXX");

        m_sYLabel = QString("YYY");

        MatrixXd dataNormalized = data.cast<double>();

        double minValue = dataNormalized.minCoeff();
        dataNormalized.array() -= minValue;

        double maxValue = dataNormalized.maxCoeff();
        dataNormalized.array() /= maxValue;

        qint32 x = data.cols();
        qint32 y = data.rows();
        QImage image(x, y, QImage::Format_RGB32);
        QRgb t_qRgb;
        double value;
        for(qint32 i = 0; i < x; ++i)
        {
            for(qint32 j = 0; j < y; ++j)
            {
                value = dataNormalized(j,i);
                t_qRgb = qRgb(R(value), G(value), B(value));
                image.setPixel(i, j, t_qRgb);
            }
        }

        pixmap = new QPixmap(QPixmap::fromImage(image));
    }
    update();
}

void MatrixView::resizeEvent (QResizeEvent* event)
{
    widgetSize = event->size();
    // Call base class impl
    QWidget::resizeEvent(event);
}


void MatrixView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if (!pixmap->isNull())
    {
        QPoint centerPoint(0,0);

        QSize pixmapSize = widgetSize;

        pixmapSize.setHeight(pixmapSize.height()-m_iBorderTopBottom*2);
        pixmapSize.setWidth(pixmapSize.width()-m_iBorderLeftRight*2);

        // Scale new image which size is widgetSize
        QPixmap scaledPixmap = pixmap->scaled(pixmapSize, Qt::KeepAspectRatio);
        // Calculate image center position into screen
        centerPoint.setX((widgetSize.width()-scaledPixmap.width())/2);
        centerPoint.setY((widgetSize.height()-scaledPixmap.height())/2);
        // Draw image
        painter.drawPixmap(centerPoint,scaledPixmap);

        painter.setPen(m_qPenAxes);
        painter.setFont(m_qFontAxes);


        qint32 t_iLabelWidth = scaledPixmap.width();
        qint32 t_iLabelHeight = 100;

        painter.save();

        // -- Title --
        painter.setPen(m_qPenTitle);
        painter.setFont(m_qFontTitle);

        painter.translate((widgetSize.width()-t_iLabelWidth)/2, (widgetSize.height()-scaledPixmap.height())/2 - m_iBorderTopBottom*1.5);
        painter.drawText(QRect(0, 0, t_iLabelWidth, t_iLabelHeight), Qt::AlignCenter, m_sTitle);

        painter.restore();
        painter.save();

        // -- Axes --

        painter.setPen(m_qPenAxes);
        painter.setFont(m_qFontAxes);

        //X Label
        painter.translate((widgetSize.width()-t_iLabelWidth)/2, scaledPixmap.height()+((widgetSize.height()-scaledPixmap.height()-m_iBorderTopBottom)/2));
        painter.drawText(QRect(0, 0, t_iLabelWidth, t_iLabelHeight), Qt::AlignCenter, m_sXLabel);

        painter.restore();

        //Y Label
        painter.rotate(270);
        painter.translate(-(widgetSize.height()+t_iLabelWidth)/2,(widgetSize.width()-scaledPixmap.width())/2-t_iLabelHeight*0.75);
        painter.drawText(QRect(0, 0, t_iLabelWidth, t_iLabelHeight), Qt::AlignCenter, m_sYLabel);


//        painter.rotate(90);

//        painter.drawText(QRect(pixmapSize.width()/2, pixmapSize.height(), 100, 100), Qt::AlignCenter, m_sYLabel);


    }
}



//HSV Colormap
int MatrixView::R(double x)
{
    //Describe the red fuzzy set
    if(x < 0.375)
        return 0;
    else if(x >= 0.375 && x < 0.625)
        return (int)floor(slopeMRaising(x, -1.5)*255);
    else if(x >= 0.625 && x < 0.875)
        return (int)floor(1.0*255);
    else if(x >= 0.875)
        return (int)floor(slopeMFalling(x, 4.5)*255);
    else
        return 0;
}

int MatrixView::G(double x)
{
    //Describe the green fuzzy set
    if(x < 0.125)
        return 0;
    else if(x >= 0.125 && x < 0.375)
        return (int)floor(slopeMRaising(x, -0.5)*255);
    else if(x >= 0.375 && x < 0.625)
        return (int)floor(1.0*255);
    else if(x >= 0.625 && x < 0.875)
        return (int)floor(slopeMFalling(x, 2.5)*255);
    else
        return 0;
}

int MatrixView::B(double x)
{
    //Describe the blue fuzzy set
    if(x < 0.125)
        return (int)floor(slopeMRaising(x, 0.5)*255);
    else if(x >= 0.125 && x < 0.375)
        return (int)floor(1.0*255);
    else if(x >= 0.375 && x < 0.625)
        return (int)floor(slopeMFalling(x, 2.5)*255);
    else
        return 0;
}

double MatrixView::slopeMRaising(double x, double n)
{
    //f = m*x + n
    return 4*x + n;
}

double MatrixView::slopeMFalling(double x,double n)
{
    //f = m*x + n
    return -4*x + n;
}
