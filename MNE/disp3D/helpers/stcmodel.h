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
#include <QVector3D>


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

    //=========================================================================================================
    /**
    * Data for the row and column and given display role
    *
    * @param [in] row       index row
    * @param [in] column    index column
    * @param [in] role      display role to access
    *
    * @return the accessed data
    */
    inline QVariant data(int row, int column, int role = Qt::DisplayRole) const;

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void addData(const MNESourceEstimate &stc);

    void init(const AnnotationSet &annotationSet, const SurfaceSet &surfSet);

    inline QVector3D getMin() const;
    inline QVector3D getMax() const;

    //1..10000
    void setAverage(qint32 samples);

    //1..100
    void setNormalization(qint32 fraction);

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

    qint32 m_iDownSampling;     /**< Downsampling factor */
    qint32 m_iCurrentSample;    /**< Downsampling */



    VectorXd m_vecCurStc;
    double m_dStcNormMax;
    double m_dStcNorm;
    VectorXd m_vecCurRelStc;

    //ToDo implement this model as a state pattern -> to be used as ROIStc model and full Stc model

    //ROI Stuff


    QList<Label> m_qListLabels;
    QList<RowVector4i> m_qListRGBAs;
    QList<Matrix3Xf> m_qListTriRRs;

    AnnotationSet m_annotationSet;
    SurfaceSet m_surfSet;

    QVector3D m_vecMinRR;                  /**< X, Y, Z minima. */
    QVector3D m_vecMaxRR;                  /**< X, Y, Z maxima. */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QVariant StcModel::data(int row, int column, int role) const
{
    return data(index(row, column), role);
}


//*************************************************************************************************************

inline QVector3D StcModel::getMin() const
{
    return m_vecMinRR;
}


//*************************************************************************************************************

inline QVector3D StcModel::getMax() const
{
    return m_vecMaxRR;
}

Q_DECLARE_METATYPE(Eigen::Matrix3Xf);
Q_DECLARE_METATYPE(FSLIB::Label);

#endif // STCMODEL_H
