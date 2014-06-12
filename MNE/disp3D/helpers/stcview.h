#ifndef STCVIEW_H
#define STCVIEW_H


#include "../disp3D_global.h"

#include "qglview.h"
#include <QGeometryData>
#include <QGLColorMaterial>
#include <QSharedPointer>
#include <QList>
#include <QMap>


class StcModel;


class DISP3DSHARED_EXPORT StcView : public QGLView
{
    Q_OBJECT
public:
    StcView(QWindow *parent = 0);

    void setModel(StcModel* model);

protected:
    //=========================================================================================================
    /**
    * Initializes the current GL context represented by painter.
    *
    * @param[in] painter    GL painter which should be initialized
    */
    void initializeGL(QGLPainter *painter);

    //=========================================================================================================
    /**
    * Paints the scene onto painter. The color and depth buffers will have already been cleared, and the camera() position set.
    *
    * @param[in] painter    GL painter which is updated
    */
    void paintGL(QGLPainter *painter);

    //=========================================================================================================
    /**
    * Processes the key press event e.
    *
    * @param[in] e      the key press event.
    */
    void keyPressEvent(QKeyEvent *e);

    //=========================================================================================================
    /**
    * Processes the mouse move event e.
    *
    * @param[in] e      the mouse move event.
    */
    void mouseMoveEvent(QMouseEvent *e);

    //=========================================================================================================
    /**
    * Processes the mouse press event e.
    *
    * @param[in] e      the mouse press event.
    */
    void mousePressEvent(QMouseEvent *e);



private:
    StcModel* m_pModel;

    bool m_bStereo;

    float m_fOffsetZ;                               /**< Z offset for pop-out effect. */
    float m_fOffsetZEye;                            /**< Z offset eye. */
    QGLSceneNode *m_pSceneNodeBrain;                /**< Scene node of the hemisphere models. */
    QGLSceneNode *m_pSceneNode;                     /**< Node of the scene. */

    QGLLightModel *m_pLightModel;                   /**< The selected light model. */
    QGLLightParameters *m_pLightParametersScene;    /**< The selected light parameters. */

    QGLColorMaterial material;


    QVector3D m_vecBoundingBoxMin;                  /**< X, Y, Z minima. */
    QVector3D m_vecBoundingBoxMax;                  /**< X, Y, Z maxima. */
    QVector3D m_vecBoundingBoxCenter;               /**< X, Y, Z center. */

    QMap<qint32, qint32> m_qMapLabelIdIndex;

};

#endif // STCVIEW_H
