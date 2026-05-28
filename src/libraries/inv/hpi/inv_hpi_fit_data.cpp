//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_hpi_fit_data.cpp
 * @since 2026
 * @date  March 2026
 * @brief Implementation of the HPI per-coil magnetic-dipole fitter.
 *
 * Implements the analytic infinite-medium magnetic-dipole leadfield,
 * the per-iteration residual / error functional, the Nelder-Mead
 * simplex search (@c fminsearch), the concurrent dispatcher
 * (@c doDipfitConcurrent) and the @c HPISortStruct comparator used to
 * match fitted coils to the digitised reference layout.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_hpi_fit_data.h"
#include "inv_hpi_fit.h"
#include "inv_sensor_set.h"

#include <math/linalg.h>

#include <iostream>
#include <algorithm>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <qmath.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvHpiFitData::InvHpiFitData()
    : m_sensors(InvSensorSet())
{
}

//=============================================================================================================

void InvHpiFitData::doDipfitConcurrent()
{
    // Initialize variables
    Eigen::RowVectorXd vecCurrentCoil = this->m_coilPos;
    Eigen::VectorXd vecCurrentData = this->m_sensorData;
    InvSensorSet currentSensors = this->m_sensors;

    int iDisplay = 0;
    int iMaxiter = m_iMaxIterations;
    int iSimplexNumitr = 0;

    this->m_coilPos = fminsearch(vecCurrentCoil,
                                iMaxiter,
                                2 * iMaxiter * vecCurrentCoil.cols(),
                                iDisplay,
                                vecCurrentData,
                                this->m_matProjector,
                                currentSensors,
                                iSimplexNumitr);

    this->m_errorInfo = dipfitError(vecCurrentCoil,
                                    vecCurrentData,
                                    currentSensors,
                                    this->m_matProjector);

    this->m_errorInfo.numIterations = iSimplexNumitr;
}

//=============================================================================================================

Eigen::MatrixXd InvHpiFitData::magnetic_dipole(Eigen::MatrixXd matPos,
                                            Eigen::MatrixXd matPnt,
                                            Eigen::MatrixXd matOri)
{
    double u0 = 1e-7;
    int iNchan;
    Eigen::MatrixXd r, r2, r5, x, y, z, mx, my, mz, Tx, Ty, Tz, lf;

    iNchan = matPnt.rows();

    // Shift the magnetometers so that the dipole is in the origin
    matPnt.array().col(0) -=matPos(0);
    matPnt.array().col(1) -=matPos(1);
    matPnt.array().col(2) -=matPos(2);

    r = matPnt.array().square().rowwise().sum().sqrt();

    r2 = r5 = x = y = z = mx = my = mz = Tx = Ty = Tz = lf = Eigen::MatrixXd::Zero(iNchan,3);

    for(int i = 0;i < iNchan;i++) {
        r2.row(i).array().fill(pow(r(i),2));
        r5.row(i).array().fill(pow(r(i),5));
    }

    for(int i = 0;i < iNchan;i++) {
        x.row(i).array().fill(matPnt(i,0));
        y.row(i).array().fill(matPnt(i,1));
        z.row(i).array().fill(matPnt(i,2));
    }

    mx.col(0).array().fill(1);
    my.col(1).array().fill(1);
    mz.col(2).array().fill(1);

    Tx = 3 * x.cwiseProduct(matPnt) - mx.cwiseProduct(r2);
    Ty = 3 * y.cwiseProduct(matPnt) - my.cwiseProduct(r2);
    Tz = 3 * z.cwiseProduct(matPnt) - mz.cwiseProduct(r2);

    for(int i = 0;i < iNchan;i++) {
        lf(i,0) = Tx.row(i).dot(matOri.row(i));
        lf(i,1) = Ty.row(i).dot(matOri.row(i));
        lf(i,2) = Tz.row(i).dot(matOri.row(i));
    }

    for(int i = 0;i < iNchan;i++) {
        for(int j = 0;j < 3;j++) {
            lf(i,j) = u0 * lf(i,j)/(4 * M_PI * r5(i,j));
        }
    }

    return lf;
}

//=============================================================================================================

Eigen::MatrixXd InvHpiFitData::compute_leadfield(const Eigen::MatrixXd& matPos, const InvSensorSet& sensors)
{

    Eigen::MatrixXd matPnt, matOri, matLf;
    matPnt = sensors.rmag(); // position of each integrationpoint
    matOri = sensors.cosmag(); // mOrientation of each coil

    matLf = magnetic_dipole(matPos, matPnt, matOri);

    return matLf;
}

//=============================================================================================================

DipFitError InvHpiFitData::dipfitError(const Eigen::MatrixXd& matPos,
                                    const Eigen::MatrixXd& matData,
                                    const InvSensorSet& sensors,
                                    const Eigen::MatrixXd& matProjectors)
{
    // Variable Declaration
    struct DipFitError e;
    Eigen::MatrixXd matLfSensor, matDif;
    Eigen::MatrixXd matLf(matData.size(),3);
    int iNp = sensors.np();

    // calculate lf for all sensorpoints
    matLfSensor = compute_leadfield(matPos, sensors);

    // apply averaging per coil
    for(int i = 0; i < sensors.ncoils(); i++){
        matLf.row(i) = sensors.w(i) * matLfSensor.block(i*iNp,0,iNp,matLfSensor.cols());
    }
    //matLf = sensors.tra * matLf;

    // Compute lead field for a magnetic dipole in infinite vacuum
    e.moment = UTILSLIB::Linalg::pinv(matLf) * matData;

    //matDif = matData - matLf * e.moment;
    matDif = matData - matProjectors * matLf * e.moment;

    e.error = matDif.array().square().sum()/matData.array().square().sum();

    e.numIterations = 0;

    return e;
}

//=============================================================================================================

bool InvHpiFitData::compare(HPISortStruct a, HPISortStruct b)
{
    return (a.base_arr < b.base_arr);
}

//=============================================================================================================

Eigen::MatrixXd InvHpiFitData::fminsearch(const Eigen::MatrixXd& matPos,
                                       int iMaxiter,
                                       int iMaxfun,
                                       int iDisplay,
                                       const Eigen::MatrixXd& matData,
                                       const Eigen::MatrixXd& matProjectors,
                                       const InvSensorSet& sensors,
                                       int &iSimplexNumitr)
{
    double tolx, tolf, rho, chi, psi, sigma, func_evals, usual_delta, zero_term_delta, temp1, temp2;
    std::string header, how;
    int n, itercount, prnt;
    Eigen::MatrixXd onesn, two2np1, one2n, v, y, v1, tempX1, tempX2, xbar, xr, x, xe, xc, xcc, xin,posCopy;
    std::vector <double> fv, fv1;
    std::vector <int> idx;

    DipFitError tempdip, fxr, fxe, fxc, fxcc;

    tolx = tolf = m_fAbortError;

    switch(iDisplay) {
        case 0:
            prnt = 0;
            break;
        default:
            prnt = 1;
    }

    header = " Iteration   Func-count     min f(x) Procedure";

    posCopy = matPos;

    n = posCopy.cols();

    // Initialize parameters
    rho = 1; chi = 2; psi = 0.5; sigma = 0.5;
    onesn = Eigen::MatrixXd::Ones(1,n);
    two2np1 = one2n = Eigen::MatrixXd::Zero(1,n);

    for(int i = 0;i < n;i++) {
        two2np1(i) = 1 + i;
        one2n(i) = i;
    }

    v = v1 = Eigen::MatrixXd::Zero(n, n+1);
    fv.resize(n+1);
    idx.resize(n+1);
    fv1.resize(n+1);

    for(int i = 0;i < n; i++) {
        v(i,0) = posCopy(i);
    }

    tempdip = dipfitError(posCopy, matData, sensors, matProjectors);
    fv[0] = tempdip.error;

    func_evals = 1;
    itercount = 0;
    how = "";

    // Continue setting up the initial simplex.
    // Following improvement suggested by L.Pfeffer at Stanford
    usual_delta = 0.05;             // 5 percent deltas for non-zero terms
    zero_term_delta = 0.00025;      // Even smaller delta for zero elements of x
    xin = posCopy.transpose();

    for(int j = 0;j < n;j++) {
        y = xin;

        if(y(j) != 0) {
            y(j) = (1 + usual_delta) * y(j);
        } else {
            y(j) = zero_term_delta;
        }

        v.col(j+1).array() = y;
        posCopy = y.transpose();
        tempdip = dipfitError(posCopy, matData, sensors, matProjectors);
        fv[j+1] = tempdip.error;
    }

    // Sort elements of fv
    std::vector<HPISortStruct> vecSortStruct;

    for (int i = 0; i < fv.size(); i++) {
        HPISortStruct structTemp;
        structTemp.base_arr = fv[i];
        structTemp.idx = i;
        vecSortStruct.push_back(structTemp);
    }

    std::sort(vecSortStruct.begin(), vecSortStruct.end(), compare);

    for (int i = 0; i < vecSortStruct.size(); i++) {
        idx[i] = vecSortStruct[i].idx;
    }

    for (int i = 0;i < n+1;i++) {
        v1.col(i) = v.col(idx[i]);
        fv1[i] = fv[idx[i]];
    }

    v = v1;fv = fv1;

    how = "initial simplex";
    itercount = itercount + 1;
    func_evals = n + 1;

    tempX1 = Eigen::MatrixXd::Zero(1,n);

    while ((func_evals < iMaxfun) && (itercount < iMaxiter)) {

        for (int i = 0;i < n;i++) {
            tempX1(i) = std::fabs(fv[0] - fv[i+1]);
        }

        temp1 = tempX1.maxCoeff();

        tempX2 = Eigen::MatrixXd::Zero(n,n);

        for(int i = 0;i < n;i++) {
            tempX2.col(i) = v.col(i+1) -  v.col(0);
        }

        tempX2 = tempX2.array().abs();

        temp2 = tempX2.maxCoeff();

        if((temp1 <= tolf) && (temp2 <= tolx)) {
            break;
        }

        xbar = v.block(0,0,n,n).rowwise().sum();
        xbar /= n;

        xr = (1+rho) * xbar - rho * v.block(0,n,v.rows(),1);

        x = xr.transpose();
        //std::cout << "Iteration Count: " << itercount << ":" << x << std::endl;

        fxr = dipfitError(x, matData, sensors, matProjectors);

        func_evals = func_evals+1;

        if (fxr.error < fv[0]) {
            // Calculate the expansion point
            xe = (1 + rho * chi) * xbar - rho * chi * v.col(v.cols()-1);
            x = xe.transpose();
            fxe = dipfitError(x, matData, sensors, matProjectors);
            func_evals = func_evals+1;

            if(fxe.error < fxr.error) {
                v.col(v.cols()-1) = xe;
                fv[n] = fxe.error;
                how = "expand";
            } else {
                v.col(v.cols()-1) = xr;
                fv[n] = fxr.error;
                how = "reflect";
            }
        }
        else {
            if(fxr.error < fv[n-1]) {
                v.col(v.cols()-1) = xr;
                fv[n] = fxr.error;
                how = "reflect";
            } else { // fxr.error >= fv[:,n-1]
                // Perform contraction
                if(fxr.error < fv[n]) {
                    // Perform an outside contraction
                    xc = (1 + psi * rho) * xbar - psi * rho * v.col(v.cols()-1);
                    x = xc.transpose();
                    fxc = dipfitError(x, matData, sensors, matProjectors);
                    func_evals = func_evals + 1;

                    if(fxc.error <= fxr.error) {
                        v.col(v.cols()-1) = xc;
                        fv[n] = fxc.error;
                        how = "contract outside";
                    } else {
                        // perform a shrink
                        how = "shrink";
                    }
                } else {
                    xcc = (1 - psi) * xbar + psi * v.col(v.cols()-1);
                    x = xcc.transpose();
                    fxcc = dipfitError(x, matData, sensors, matProjectors);
                    func_evals = func_evals+1;
                    if(fxcc.error < fv[n]) {
                        v.col(v.cols()-1) = xcc;
                        fv[n] = fxcc.error;
                        how = "contract inside";
                    } else {
                        // perform a shrink
                        how = "shrink";
                    }
                }

                if(how.compare("shrink") == 0) {
                    for(int j = 1;j < n+1;j++) {
                        v.col(j).array() = v.col(0).array() + sigma * (v.col(j).array() - v.col(0).array());
                        x = v.col(j).array().transpose();
                        tempdip = dipfitError(x,matData, sensors, matProjectors);
                        fv[j] = tempdip.error;
                    }
                }
            }
        }

        // Sort elements of fv
        vecSortStruct.clear();

        for (int i = 0; i < fv.size(); i++) {
            HPISortStruct structTemp;
            structTemp.base_arr = fv[i];
            structTemp.idx = i;
            vecSortStruct.push_back(structTemp);
        }

        std::sort(vecSortStruct.begin(), vecSortStruct.end(), compare);
        for (int i = 0; i < vecSortStruct.size(); i++) {
            idx[i] = vecSortStruct[i].idx;
        }

        for (int i = 0;i < n+1;i++) {
            v1.col(i) = v.col(idx[i]);
            fv1[i] = fv[idx[i]];
        }

        v = v1;
        fv = fv1;
        itercount = itercount + 1;
    }

    x = v.col(0).transpose();

    // Seok
    iSimplexNumitr = itercount;

    return x;
}

