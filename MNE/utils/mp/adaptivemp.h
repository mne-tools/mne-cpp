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
#include <unsupported/Eigen/FFT>

namespace UTILSLIB
{
//=============================================================================================================
// NAMESPACES

using namespace Eigen;
using namespace std;

//*************************************************************************************************************

enum ReturnValue{RETURNATOM, RETURNPARAMETERS};

//*************************************************************************************************************

class UTILSSHARED_EXPORT adaptiveMP
{

public:
    adaptiveMP();
    QList<GaborAtom> MatchingPursuit (MatrixXd signal, qint32 max_it, qreal epsilon);
    VectorXcd ModulationFunction(qint32 N, qreal k);
    VectorXd adaptiveMP::CalculateAtom(qint32 sampleCount, qreal scale, qint32 translation, qreal modulation, qint32 channel, MatrixXd residuum, ReturnValue returnValue);

};

}



#endif // ADAPTIVEMP_H
