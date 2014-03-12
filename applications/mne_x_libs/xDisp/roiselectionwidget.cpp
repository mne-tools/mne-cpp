#include "roiselectionwidget.h"
#include "roi.h"

#include <math.h>

#include <QKeyEvent>

#include <QGraphicsSvgItem>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XDISPLIB;


RoiSelectionWidget::RoiSelectionWidget(QWidget *parent)
: QGraphicsView(parent)
{
    QGraphicsScene *scene = new QGraphicsScene(this);
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    scene->setSceneRect(-200, -200, 400, 400);
    setScene(scene);
    setCacheMode(CacheBackground);
    setViewportUpdateMode(BoundingRectViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(AnchorUnderMouse);
    scale(qreal(0.8), qreal(0.8));
    setMinimumSize(400, 400);
    setWindowTitle(tr("Elastic Nodes"));

    Roi *frontal_left = new Roi(":/images/frontal_left", this);
    Roi *frontal_right = new Roi(":/images/frontal_right", this);
    Roi *parietal_left = new Roi(":/images/parietal_left", this);
    Roi *parietal_right = new Roi(":/images/parietal_right", this);
    Roi *occipital_left = new Roi(":/images/occipital_left", this);
    Roi *occipital_right = new Roi(":/images/occipital_right", this);

    scene->addItem(frontal_left);
    scene->addItem(frontal_right);
    scene->addItem(parietal_left);
    scene->addItem(parietal_right);
    scene->addItem(occipital_left);
    scene->addItem(occipital_right);

    frontal_left->setPos(118, 140);
    frontal_right->setPos(-134, 140);
    parietal_left->setPos(126, 0);
    parietal_right->setPos(-126, 0);
    occipital_left->setPos(124, -136);
    occipital_right->setPos(-76, -136);

    scene->setSceneRect(-154.0f, -156.0f, 540, 670);

}

void RoiSelectionWidget::itemMoved()
{

}

void RoiSelectionWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Plus:
        zoomIn();
        break;
    case Qt::Key_Minus:
        zoomOut();
        break;
    case Qt::Key_Space:
    case Qt::Key_Enter:
        shuffle();
        break;
    default:
        QGraphicsView::keyPressEvent(event);
    }
}

#ifndef QT_NO_WHEELEVENT
void RoiSelectionWidget::wheelEvent(QWheelEvent *event)
{
    scaleView(pow((double)2, -event->delta() / 240.0));
}
#endif

//void RoiSelectionWidget::drawBackground(QPainter *painter, const QRectF &rect)
//{
//    Q_UNUSED(rect);

//    // Shadow
//    QRectF sceneRect = this->sceneRect();
//    QRectF rightShadow(sceneRect.right(), sceneRect.top() + 5, 5, sceneRect.height());
//    QRectF bottomShadow(sceneRect.left() + 5, sceneRect.bottom(), sceneRect.width(), 5);
//    if (rightShadow.intersects(rect) || rightShadow.contains(rect))
//        painter->fillRect(rightShadow, Qt::darkGray);
//    if (bottomShadow.intersects(rect) || bottomShadow.contains(rect))
//        painter->fillRect(bottomShadow, Qt::darkGray);

//    // Fill
//    QLinearGradient gradient(sceneRect.topLeft(), sceneRect.bottomRight());
//    gradient.setColorAt(0, Qt::white);
//    gradient.setColorAt(1, Qt::lightGray);
//    painter->fillRect(rect.intersected(sceneRect), gradient);
//    painter->setBrush(Qt::NoBrush);
//    painter->drawRect(sceneRect);

////    // Text
////    QRectF textRect(sceneRect.left() + 4, sceneRect.top() + 4,
////                    sceneRect.width() - 4, sceneRect.height() - 4);
////    QString message(tr("Click and drag the nodes around, and zoom with the mouse "
////                       "wheel or the '+' and '-' keys"));

////    QFont font = painter->font();
////    font.setBold(true);
////    font.setPointSize(14);
////    painter->setFont(font);
////    painter->setPen(Qt::lightGray);
////    painter->drawText(textRect.translated(2, 2), message);
////    painter->setPen(Qt::black);
////    painter->drawText(textRect, message);
//}

void RoiSelectionWidget::scaleView(qreal scaleFactor)
{
    qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < 0.07 || factor > 100)
        return;

    scale(scaleFactor, scaleFactor);
}

void RoiSelectionWidget::shuffle()
{
    foreach (QGraphicsItem *item, scene()->items()) {
        if (qgraphicsitem_cast<Roi *>(item))
            item->setPos(-150 + qrand() % 300, -150 + qrand() % 300);
    }
}

void RoiSelectionWidget::zoomIn()
{
    scaleView(qreal(1.2));
}

void RoiSelectionWidget::zoomOut()
{
    scaleView(1 / qreal(1.2));
}
