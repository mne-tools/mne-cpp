#include "stcview.h"

#include "qglbuilder.h"
#include "qglcube.h"

#include "stcmodel.h"

#include <disp/colormap.h>

#include <QMouseEvent>

using namespace DISPLIB;


StcView::StcView(QWindow *parent)
: QGLView(parent)
, m_pModel(NULL)
, m_bStereo(true)
, m_stereoType(QGLView::StretchedLeftRight)
{
    m_fOffsetZ = -100.0f;
    m_fOffsetZEye = 60.0f;
}


//*************************************************************************************************************

void StcView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    //check wether realtive stc data column (3) has changed
    if(topLeft.column() > 3 || bottomRight.column() < 3)
        return;

    for(qint32 i = 0; i < m_pSceneNode->palette()->size(); ++i)
    {
        //ToDo label id check -> necessary?
        //qint32 colorIdx = m_qListMapLabelIdIndex[h][labelId];

        qint32 iVal = m_pModel->data(i,3).toDouble() * 255;

        iVal = iVal > 255 ? 255 : iVal < 0 ? 0 : iVal;

        QRgb qRgb;
        qRgb = ColorMap::valueToHotNegative1((double)iVal/255.0);
//        qRgb = ColorMap::valueToHotNegative2((double)iVal/255.0);
//        qRgb = ColorMap::valueToHot((double)iVal/255.0);

        m_pSceneNode->palette()->material(i)->setSpecularColor(QColor(qRgb));
    }

    this->update();
}


//*************************************************************************************************************

void StcView::setModel(StcModel* model)
{
    m_pModel = model;
    connect(m_pModel, &StcModel::dataChanged, this, &StcView::dataChanged);
}


//*************************************************************************************************************

void StcView::initializeGL(QGLPainter *painter)
{
    if(!m_pModel)
        return;

    // in the constructor construct a builder on the stack
    QGLBuilder builder;

    float fac = 100.0f; // too small vertices distances cause clipping errors --> 100 is a good value for freesurfer brain measures

    builder << QGL::Faceted;
    m_pSceneNodeBrain = builder.currentNode();

    builder.pushNode();

    // Collor palette
    qint32 index;
    QSharedPointer<QGLMaterialCollection> palette = builder.sceneNode()->palette(); // register color palette within the root node

    m_qMapLabelIdIndex.clear();

//    //
//    // Build each hemisphere in its separate node
//    //
//    for(qint32 h = 0; h < 1; ++h)
//    {
        builder.newNode();//create new hemisphere node
        {
            //
            // Create each ROI in its own node
            //
            for(qint32 k = 0; k < m_pModel->rowCount(); ++k)
            {
                // add new ROI node when current ROI node is not empty
                if(builder.currentNode()->count() > 0)
                    builder.newNode();

                QGeometryData t_GeometryDataTri;

                Matrix3Xf t_TriCoords = m_pModel->data(k,6,Qt::DisplayRole).value<Matrix3Xf>();

                t_TriCoords *= fac;
                t_GeometryDataTri.appendVertexArray(QArray<QVector3D>::fromRawData( reinterpret_cast<const QVector3D*>(t_TriCoords.data()), t_TriCoords.cols() ));

                //
                // If triangles are available.
                //
                if (t_GeometryDataTri.count() > 0)
                {

                    //
                    //  Add triangles to current node
                    //
                    builder.addTriangles(t_GeometryDataTri);

                    //
                    // Colorize ROI
                    //
                    QGLMaterial *t_pMaterialROI = new QGLMaterial();

                    t_pMaterialROI->setColor(m_pModel->data(k,5,Qt::DisplayRole).value<QColor>());

                    index = palette->addMaterial(t_pMaterialROI);
                    builder.currentNode()->setMaterialIndex(index);

                    m_qMapLabelIdIndex.insert(m_pModel->data(k,4,Qt::DisplayRole).value<Label>().label_id, index);
                }
            }
        }
        // Go one level up
        builder.popNode();
//    }
//    // Go one level up
//    builder.popNode();

    // Optimze current scene for display and calculate lightning normals
    m_pSceneNode = builder.finalizedSceneNode();
    m_pSceneNode->setParent(this);

    //
    // Create light models
    //
    m_pLightModel = new QGLLightModel(this);
    m_pLightModel->setAmbientSceneColor(Qt::white);
    m_pLightModel->setViewerPosition(QGLLightModel::LocalViewer);

    m_pLightModel = new QGLLightModel(this);

    m_pLightParametersScene = new QGLLightParameters(this);
    m_pLightParametersScene->setPosition(QVector3D(0.0f, 0.0f, 3.0f));
    painter->setMainLight(m_pLightParametersScene);

    //
    // Set stereo type
    //
    if (m_bStereo) {
//        this->setStereoType(QGLView::RedCyanAnaglyph);
        this->setStereoType(m_stereoType);
//        camera()->setEyeSeparation(0.4f);
//        m_pCameraFrontal->setEyeSeparation(0.1f);

        //LNdT DEMO
        camera()->setCenter(QVector3D(0,0,m_fOffsetZ));//0.8f*fac));
        camera()->setEyeSeparation(0.4f);
        camera()->setFieldOfView(30);
        camera()->setEye(QVector3D(0,0,m_fOffsetZEye));
        //LNdT DEMO end

    }

//    //set background to light grey-blue
//    glClearColor(0.8f, 0.8f, 1.0f, 0.0f);

//    //set background to light white
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

}


//*************************************************************************************************************

void StcView::paintGL(QGLPainter *painter)
{
    glEnable(GL_BLEND); // enable transparency

    //    painter->modelViewMatrix().rotate(45.0f, 1.0f, 1.0f, 1.0f);


    painter->modelViewMatrix().push();
    painter->projectionMatrix().push();

    painter->setStandardEffect(QGL::LitMaterial);
//        painter->setCamera(m_pCameraFrontal);
    painter->setLightModel(m_pLightModel);

//        material.bind(painter);
//        material.prepareToDraw(painter, painter->attributes());

    m_pSceneNode->draw(painter);


    painter->modelViewMatrix().pop();
    painter->projectionMatrix().pop();
}


//*************************************************************************************************************

void StcView::keyPressEvent(QKeyEvent *e)
{
    camera()->setCenter(QVector3D(0,0,0));

    float normEyeOld = sqrt(pow(camera()->eye().x(),2) + pow(camera()->eye().y(),2) + pow(camera()->eye().z(),2));

    QGLView::keyPressEvent(e);

    float dx = (camera()->eye().x()*m_fOffsetZ)/m_fOffsetZEye;
    float dy = (camera()->eye().y()*m_fOffsetZ)/m_fOffsetZEye;
    float dz = (camera()->eye().z()*m_fOffsetZ)/m_fOffsetZEye;

    float normEye = sqrt(pow(camera()->eye().x(),2) + pow(camera()->eye().y(),2) + pow(camera()->eye().z(),2));
    float scaleEye = normEyeOld/normEye;//m_fOffsetZEye/normEye;
    camera()->setEye(QVector3D(camera()->eye().x()*scaleEye,camera()->eye().y()*scaleEye,camera()->eye().z()*scaleEye));

    camera()->setCenter(QVector3D(dx,dy,dz));
}


//*************************************************************************************************************

void StcView::mouseMoveEvent(QMouseEvent *e)
{
    camera()->setCenter(QVector3D(0,0,0));

    float normEyeOld = sqrt(pow(camera()->eye().x(),2) + pow(camera()->eye().y(),2) + pow(camera()->eye().z(),2));

    QGLView::mouseMoveEvent(e);

    float dx = (camera()->eye().x()*m_fOffsetZ)/m_fOffsetZEye;
    float dy = (camera()->eye().y()*m_fOffsetZ)/m_fOffsetZEye;
    float dz = (camera()->eye().z()*m_fOffsetZ)/m_fOffsetZEye;

    float normEye = sqrt(pow(camera()->eye().x(),2) + pow(camera()->eye().y(),2) + pow(camera()->eye().z(),2));
    float scaleEye = normEyeOld/normEye;//m_fOffsetZEye/normEye;
    camera()->setEye(QVector3D(camera()->eye().x()*scaleEye,camera()->eye().y()*scaleEye,camera()->eye().z()*scaleEye));

    camera()->setCenter(QVector3D(dx,dy,dz));
}


//*************************************************************************************************************

void StcView::mousePressEvent(QMouseEvent *e)
{
    if(e->buttons() & Qt::RightButton)
    {
        float normEye = sqrt(pow(camera()->eye().x(),2) + pow(camera()->eye().y(),2) + pow(camera()->eye().z(),2));
        camera()->setCenter(QVector3D(0,0,m_fOffsetZ));
        camera()->setEye(QVector3D(0,0,normEye));
    }

    QGLView::mousePressEvent(e);
}
