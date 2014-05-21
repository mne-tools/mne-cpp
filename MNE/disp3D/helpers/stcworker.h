#ifndef STCWORKER_H
#define STCWORKER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QVector>
#include <QThread>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>



using namespace Eigen;


class DISP3DSHARED_EXPORT StcWorker : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<StcWorker> SPtr;            /**< Shared pointer type for StcWorker class. */
    typedef QSharedPointer<const StcWorker> ConstSPtr; /**< Const shared pointer type for StcWorker class. */

    StcWorker(QObject *parent = 0);

//    void setIntervall(int intervall);

    void addData();

    void process();

private:

    QVector<VectorXd> m_data;   /**< List that holds the fiff matrix data <n_channels x n_samples> */



};

#endif // STCWORKER_H
