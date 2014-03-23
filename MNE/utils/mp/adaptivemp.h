//MATCHING PURSUIT

#ifndef ADAPTIVEMP_H
#define ADAPTIVEMP_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>
#include <utils/mp/atom.h>
#include <utils/utils_global.h>

//Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <Eigen/unsupported/FFT>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//#include <QtCore/QCoreApplication>

namespace UTILSLIB
{
//=============================================================================================================
// NAMESPACES

using namespace Eigen;

//*************************************************************************************************************

class UTILSSHARED_EXPORT adaptiveMP
{

public:
    adaptiveMP();
    VectorXd GaussFunction (qint32 N, qreal s, qint32 p);
    QList<GaborAtom> MatchingPursuit (MatrixXd signal, qint32 max_it, qreal epsilon);
    VectorXcd ModulationFunction(qint32 N, qreal k);
};

}

#endif // ADAPTIVEMP_H
