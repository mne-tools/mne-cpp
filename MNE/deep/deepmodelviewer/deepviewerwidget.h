#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>

#include "../deep_global.h"



//*************************************************************************************************************
//=============================================================================================================
// CNTK INCLUDES
//=============================================================================================================

#include <CNTKLibrary.h>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QSplitter;
QT_END_NAMESPACE

class Node;
class Edge;


class DEEPSHARED_EXPORT DeepViewerWidget : public QWidget
{
    Q_OBJECT
public:
    DeepViewerWidget(CNTK::FunctionPtr model, QWidget *parent = 0);

private:
    void populateScene();

    CNTK::FunctionPtr m_pModel;  /**< The CNTK model v2 */

    QGraphicsScene* m_pScene;

    QSplitter* m_pSplitter;

    QList< QList<Node*> > m_listLayers;
    QList< QList<Edge*> > m_listEdges;
};

#endif // MAINWINDOW_H
