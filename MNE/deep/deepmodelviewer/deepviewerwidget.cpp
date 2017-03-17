#include "deepviewerwidget.h"
#include "edge.h"
#include "node.h"

#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QKeyEvent>


#ifndef QT_NO_OPENGL
#include <QtOpenGL>
#else
#include <QtWidgets>
#endif


DeepViewerWidget::DeepViewerWidget(QWidget *parent)
: QGraphicsView(parent)
{
//#ifndef QT_NO_OPENGL
//    this->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
//#else
//#endif

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
    setWindowTitle(tr("Deep Viewer"));

    QVector<int> layerDim;
    layerDim.append(4); //Inputs
    layerDim.append(20); // First hidden
    layerDim.append(10); // Second hidden
    layerDim.append(10); // Third hidden
    layerDim.append(2); // Output

    int numLayers = layerDim.size();

    double layerDist = 400.0;
    double nodeDist = 50.0;

    double x_root = -((numLayers-1.0)*layerDist) / 2.0;


    QList<Node*> currentLayer;
    QPointF layerRoot, currentPos;

    for(int layer = 0; layer < layerDim.size(); ++layer) {
        layerRoot = QPointF( x_root + layer*layerDist, - (layerDim[layer]/2) * nodeDist);

        // Create Nodes
        for(int i = 0; i < layerDim[layer]; ++i ) {
            currentLayer.append(new Node(this));
            scene->addItem(currentLayer[i]);

            currentPos = layerRoot + QPointF(0,nodeDist * i);
            currentLayer[i]->setPos(currentPos);
        }
        layersList.append(currentLayer);
        currentLayer.clear();

        // Create Edges
        if(layer - 1 >= 0) {
            for(int i = 0; i < layersList[layer-1].size(); ++i ) {
                for(int j = 0; j < layersList[layer].size(); ++j ) {
                    scene->addItem(new Edge(layersList[layer-1][i], layersList[layer][j]));
                }
            }
        }
    }
}


//*************************************************************************************************************

void DeepViewerWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Plus:
        zoomIn();
        break;
    case Qt::Key_Minus:
        zoomOut();
        break;
    default:
        QGraphicsView::keyPressEvent(event);
    }
}


//*************************************************************************************************************

void DeepViewerWidget::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);
}


//*************************************************************************************************************

#ifndef QT_NO_WHEELEVENT
void DeepViewerWidget::wheelEvent(QWheelEvent *event)
{
    scaleView(pow((double)2, -event->delta() / 240.0));
}
#endif


//*************************************************************************************************************

void DeepViewerWidget::drawBackground(QPainter *painter, const QRectF &rect)
{
    Q_UNUSED(painter);
    Q_UNUSED(rect);
//    QRectF sceneRect = this->sceneRect();
}


//*************************************************************************************************************

void DeepViewerWidget::scaleView(qreal scaleFactor)
{
    qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < 0.07 || factor > 100)
        return;

    scale(scaleFactor, scaleFactor);
}


//*************************************************************************************************************

void DeepViewerWidget::zoomIn()
{
    scaleView(qreal(1.2));
}


//*************************************************************************************************************

void DeepViewerWidget::zoomOut()
{
    scaleView(1 / qreal(1.2));
}
