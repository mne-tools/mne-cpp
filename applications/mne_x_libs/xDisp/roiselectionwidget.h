#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "xdisp_global.h"

#include <QGraphicsView>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XDISPLIB
//=============================================================================================================

namespace XDISPLIB
{

class XDISPSHARED_EXPORT RoiSelectionWidget : public QGraphicsView
{
    Q_OBJECT

public:
    RoiSelectionWidget(QWidget *parent = 0);

    void itemMoved();

public slots:
    void shuffle();
    void zoomIn();
    void zoomOut();

protected:
    void keyPressEvent(QKeyEvent *event);
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *event);
#endif
//    void drawBackground(QPainter *painter, const QRectF &rect);

    void scaleView(qreal scaleFactor);

private:
    QGraphicsItem *m_svgItem;

    QGraphicsRectItem *m_backgroundItem;
    QGraphicsRectItem *m_outlineItem;

};

} // NAMESPACE

#endif // GRAPHWIDGET_H
