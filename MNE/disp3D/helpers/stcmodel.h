#ifndef STCMODEL_H
#define STCMODEL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include "stcworker.h"

#include <fs/label.h>
#include <fs/surfaceset.h>
#include <fs/annotationset.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QAbstractTableModel>
#include <QThread>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB
{
    class MNESourceEstimate;
}


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;
using namespace FSLIB;


class DISP3DSHARED_EXPORT StcModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    StcModel(QObject *parent = 0);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void addData(const MNESourceEstimate &stc);

    void init(const AnnotationSet &annotationSet, const SurfaceSet &surfSet);


    void setStcSample(const VectorXd &sample);

    void setVertices(const VectorXi &vertnos);

signals:



private:
    QSharedPointer<QThread> m_pThread;
    StcWorker::SPtr         m_pWorker;


    bool m_bRTMode;
    bool m_bIsInit;
    bool m_bIntervallSet;

    VectorXi m_vertices;

    qint32 m_iDownsampling;     /**< Down sampling factor */
    qint32 m_iCurrentSample;    /**< Downsampling */




    //ROI Stuff
    VectorXd m_vecCurStc;
    VectorXd m_vecCurRelStc;
    double m_dStcNorm;

    QList<Label> m_qListLabels;
    QList<RowVector4i> m_qListRGBAs;

    AnnotationSet m_annotationSet;
    SurfaceSet m_surfSet;

};

Q_DECLARE_METATYPE(Eigen::MatrixXd);

#endif // STCMODEL_H
