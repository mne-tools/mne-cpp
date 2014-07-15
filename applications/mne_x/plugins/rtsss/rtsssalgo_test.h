#ifndef RTSSSALGO_TEST_H
#define RTSSSALGO_TEST_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QtGlobal>
#include <QtCore/qmath.h>
#include <QList>
#include <Eigen/Dense>
#include <iostream>
#include <QString>
#include <QDebug>
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <math.h>
#include <complex>

#include <fiff/fiff_info.h>
//#include <xMeas/Measurement/realtimemultisamplearray_new.h>

//using namespace MNEX;


using namespace Eigen;
using namespace std;
using namespace FIFFLIB;
//using namespace XMEASLIB;

typedef std::complex<double> cplxd;

MatrixXd legendre(int, VectorXd);
float plgndr(int l, int m, float x);
double factorial(int);
QList<MatrixXd> getSSSRR(MatrixXd, MatrixXd, MatrixXd, MatrixXd, MatrixXd);
VectorXd hypot(VectorXd, VectorXd);
VectorXd atan2vec(VectorXd, VectorXd);
VectorXd find(MatrixXd, int);

double stdev(VectorXd);
VectorXd eigen_LTE(VectorXd V, double tol);
VectorXd eigen_LT_index(VectorXd V, double tol);
VectorXd eigen_LT_index_test(VectorXd V, int);
VectorXd eigen_GT(VectorXd V, double tol);
VectorXd eigen_AND(VectorXd V1, VectorXd V2);

class RtSssAlgoTest
{
public:
    RtSssAlgoTest();
    ~RtSssAlgoTest();

    QList<MatrixXd> buildLinearEqn();
//    QList<MatrixXd> getSSSRR();
//    QList<MatrixXd> getSSSOLS();
    QList<MatrixXd> getSSSRR(MatrixXd EqnIn, MatrixXd EqnOut, MatrixXd EqnARR, MatrixXd EqnA, MatrixXd EqnB);
    QList<MatrixXd> getSSSOLS(MatrixXd EqnIn, MatrixXd EqnOut, MatrixXd EqnA, MatrixXd EqnB);
    QList<MatrixXd> getLinEqn();

private:
    void getCoilInfoVectorView4Sim();
    void getCoilInfoBabyMEG4Sim();
    QList<MatrixXd> getSSSEqn(int, int);
    void getSSSBasis(VectorXd, VectorXd, VectorXd, int, int);
    void getCartesianToSpherCoordinate(VectorXd, VectorXd, VectorXd);
    void getSphereToCartesianVector();
    int strmatch(char, char);

    int NumCoil;
    QList<MatrixXd> CoilT;
    QList<string> CoilName, CoilTk;
    QList<MatrixXd> CoilRk, CoilWk;
    VectorXi CoilNk, CoilGrad;
    VectorXd MEGIn, MEGOut, MEGNoise, MEGData;

    Vector3d Origin;
    MatrixXd BInX, BInY, BInZ, BOutX, BOutY, BOutZ;
    MatrixXd EqnInRR, EqnOutRR, EqnIn, EqnOut, EqnARR, EqnA, EqnB;

    VectorXd R, PHI, THETA;
    VectorXd R_X, R_Y, R_Z;
    VectorXd PHI_X, PHI_Y, PHI_Z;
    VectorXd THETA_X, THETA_Y, THETA_Z;

    FiffInfo::SPtr m_pFiffInfo;     /**< Fiff information. */
};

#endif // RTSSSALGO_TEST_H
