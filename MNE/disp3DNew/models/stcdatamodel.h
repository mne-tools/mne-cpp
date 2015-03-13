//=============================================================================================================
/**
* @file     stcdatamodel.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    StcDataModel class declaration
*
*/

#ifndef STCDATAMODEL_H
#define STCDATAMODEL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3DNew_global.h"

#include "stcdataworker.h"

#include <fs/label.h>
#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include <iostream>

#include "MNE/mne_forwardsolution.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QAbstractTableModel>
#include <QVector3D>
#include <QMap>


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
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DNEWLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;
using namespace FSLIB;


//*************************************************************************************************************
//=============================================================================================================
// Define model roles
//=============================================================================================================

namespace StcDataModelRoles
{
    enum ItemRole{GetIndexLH = Qt::UserRole + 1001,
                  GetIndexRH = Qt::UserRole + 1002,
                  GetStcValLH = Qt::UserRole + 1003,
                  GetStcValRH = Qt::UserRole + 1004,
                  GetRelStcValLH = Qt::UserRole + 1005,
                  GetRelStcValRH = Qt::UserRole + 1006};
}

//=============================================================================================================
/**
* Source estimation informations are provided in a table. They can be accessed via data(row, col).
*
* @brief Table model which prepares source estimate information
*/
class DISP3DNEWSHARED_EXPORT StcDataModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    typedef QSharedPointer<StcDataModel> SPtr;            /**< Shared pointer type for StcDataModel class. */
    typedef QSharedPointer<const StcDataModel> ConstSPtr; /**< Const shared pointer type for StcDataModel class. */

    StcDataModel(QObject *parent = 0);

    ~StcDataModel();

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

    void init(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir, const QString &atlas, const MNEForwardSolution &forwardSolution);

    inline QVector3D getMin() const;
    inline QVector3D getMax() const;

    //1..10000
    void setAverage(qint32 samples);

    void setLoop(bool looping);

    //1..100
    void setNormalization(qint32 fraction);

    void setStcSample(const VectorXd &sample);

    void setVertLabelIDs(const VectorXi &vertLabelIDs);

private:
    StcDataWorker::SPtr    m_pWorker;

    bool m_bRTMode;
    bool m_bModelInit;
    bool m_bDataInit;
    bool m_bIntervallSet;

    VectorXi m_vertLabelIds;

    QMap<qint32, qint32> m_qMapLabelIdChannelLH;
    QMap<qint32, qint32> m_qMapLabelIdChannelRH;

    VectorXd m_vecCurStc;
    double m_dStcNormMax;
    double m_dStcNorm;
    VectorXd m_vecCurRelStc;

    //ToDo implement this model as a state pattern -> to be used as ROIStc model and full Stc model

    //ROI Stuff
    QList<Label> m_qListLabels;
    qint32 m_iLHSize;
    QList<RowVector4i> m_qListRGBAs;
    QList<Matrix3Xf> m_qListTriRRs;

    AnnotationSet m_annotationSet;
    SurfaceSet m_surfSet;
    MNEForwardSolution m_forwardSolution;

    QVector3D m_vecMinRR;                  /**< X, Y, Z minima. */
    QVector3D m_vecMaxRR;                  /**< X, Y, Z maxima. */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QVariant StcDataModel::data(int row, int column, int role) const
{
    return data(index(row, column), role);
}


//*************************************************************************************************************

inline QVector3D StcDataModel::getMin() const
{
    return m_vecMinRR;
}


//*************************************************************************************************************

inline QVector3D StcDataModel::getMax() const
{
    return m_vecMaxRR;
}

} // NAMESPACE

Q_DECLARE_METATYPE(Eigen::Matrix3Xf);
Q_DECLARE_METATYPE(Eigen::VectorXd);
Q_DECLARE_METATYPE(FSLIB::Label);

#endif // STCDATAMODEL_H
