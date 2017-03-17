#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../deep_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsView>


class Node;


//! [0]
class DEEPSHARED_EXPORT DeepViewerWidget : public QGraphicsView
{
    Q_OBJECT

public:
    DeepViewerWidget(QWidget *parent = 0);

public slots:
    void zoomIn();
    void zoomOut();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *event) override;
#endif
    void drawBackground(QPainter *painter, const QRectF &rect) override;

    void scaleView(qreal scaleFactor);

private:
    QList< QList<Node*> > layersList;
};
//! [0]

#endif // GRAPHWIDGET_H
