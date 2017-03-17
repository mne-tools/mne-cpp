#include "deepviewerwidget.h"
#include "view.h"

#include "node.h"
#include "edge.h"

#include <QHBoxLayout>
#include <QSplitter>

#include <QDebug>

DeepViewerWidget::DeepViewerWidget(CNTK::FunctionPtr model, QWidget *parent)
: QWidget(parent)
, m_pModel(model)
{
    populateScene();

    m_pSplitter = new QSplitter;

    View *view = new View("");
    view->view()->setScene(m_pScene);
    m_pSplitter->addWidget(view);

//    view = new View("Top right view");
//    view->view()->setScene(scene);
//    h1Splitter->addWidget(view);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(m_pSplitter);
    setLayout(layout);



    setWindowTitle(tr("Deep Model Viewer"));
}

void DeepViewerWidget::populateScene()
{
    m_pScene = new QGraphicsScene(this);

    if(!m_pModel)
        return;

    QVector<int> layerDim;

    int inDim = 0;
    int outDim = 0;

    for (int i = static_cast<int>(m_pModel->Parameters().size()) - 1; i >= 0 ; --i) {
        fprintf(stderr,"\n >> Level = %ju <<\n",m_pModel->Parameters().size() - i);
        fprintf(stderr,"Dim: %ls\n",m_pModel->Parameters()[i].Shape().AsString().c_str());

        QString param = QString::fromStdWString(m_pModel->Parameters()[i].Shape().AsString());

        if(param.contains(" x ")) {
            param.replace(QString("["), QString(""));param.replace(QString("]"), QString(""));
            QStringList dimensions = param.split(" x ");
            outDim = dimensions[0].toInt();
            inDim = dimensions[1].toInt();

            layerDim.append(inDim);
        }
    }
    layerDim.append(outDim);

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
            m_pScene->addItem(currentLayer[i]);

            currentPos = layerRoot + QPointF(0,nodeDist * i);
            currentLayer[i]->setPos(currentPos);
        }
        layersList.append(currentLayer);
        currentLayer.clear();

        // Create Edges
        if(layer - 1 >= 0) {
            for(int i = 0; i < layersList[layer-1].size(); ++i ) {
                for(int j = 0; j < layersList[layer].size(); ++j ) {
                    m_pScene->addItem(new Edge(layersList[layer-1][i], layersList[layer][j]));
                }
            }
        }
    }

}
