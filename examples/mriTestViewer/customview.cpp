#include "customview.h"

CustomView::CustomView(QWidget *parent) : QGraphicsView(parent)
{

}

void CustomView::wheelEvent(QWheelEvent *event)
{
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    if(event->delta() > 0)
    {
        scale(scaleFactor, scaleFactor);
        scaleSize = scaleSize*scaleFactor;
    }
    else if (event->delta() < 0)
    {
        scale(1/scaleFactor, 1/scaleFactor);
        scaleSize = scaleSize/scaleFactor;
    }
    qDebug() << "scale to" << scaleSize;
}

CustomView::~CustomView()
{

}

