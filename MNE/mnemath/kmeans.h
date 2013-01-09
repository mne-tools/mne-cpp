#ifndef KMEANS_H
#define KMEANS_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mnemath_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QString>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNEMATHLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


class MNEMATHSHARED_EXPORT KMeans
{
public:
    typedef QSharedPointer<KMeans> SPtr;            /**< Shared pointer type for KMeans. */
    typedef QSharedPointer<const KMeans> ConstSPtr; /**< Const shared pointer type for KMeans. */

    //distance {'sqeuclidean','cityblock','cosine','correlation','hamming'};
    //startNames = {'uniform','sample','cluster'};
    //emptyactNames = {'error','drop','singleton'};

    KMeans(QString distance = QString("sqeuclidean") , QString start = QString("sample"), qint32 replicates = 1, QString emptyact = QString("error"), bool online = true, qint32 maxit = 100);



    bool calculate(    MatrixXd X, qint32 kClusters,
                        VectorXi& idx, MatrixXd& C, VectorXd& sumD, MatrixXd& D);


private:
    MatrixXd distfun(MatrixXd& X, MatrixXd& C, qint32 iter);


    bool batchUpdate(MatrixXd& X, MatrixXd& C, VectorXi& idx);


    void gcentroids(MatrixXd& X, VectorXi& index, VectorXi& clusts,
                                        MatrixXd& centroids, VectorXi& counts);

    bool onlineUpdate(MatrixXd& X, MatrixXd& C,  VectorXi& idx);

    double unifrnd(double a, double b);


    QString m_sDistance;
    QString m_sStart;
    qint32 m_iReps;
    QString m_sEmptyact;
    qint32 m_iMaxit;
    bool m_bOnline;

    qint32 emptyErrCnt;

    qint32 iter;
    qint32 k;
    qint32 n;
    qint32 p;

    MatrixXd Del;

    VectorXd d;

    VectorXi m;

    double totsumD;

    double prevtotsumD;

    VectorXi previdx;

};

} // NAMESPACE

#endif // KMEANS_H
