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
class Controls;


class DEEPSHARED_EXPORT DeepViewerWidget : public QWidget
{
    Q_OBJECT
public:
    DeepViewerWidget(CNTK::FunctionPtr model, QWidget *parent = Q_NULLPTR);

    DeepViewerWidget(CNTK::FunctionPtr model, Controls *controls, QWidget *parent = Q_NULLPTR);


private:
    void populateScene();

    CNTK::FunctionPtr m_pModel;  /**< The CNTK model v2 */

    QGraphicsScene* m_pScene;

    QSplitter* m_pSplitter;

    QList< QList<Node*> > m_listLayers;
    QList< QList<Edge*> > m_listEdges;
};

#endif // MAINWINDOW_H
