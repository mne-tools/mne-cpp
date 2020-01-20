//=============================================================================================================
/**
 * @file     hpifitdata.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     April, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief    HPIFitData class defintion.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "hpifitdata.h"

#include <utils/mnemath.h>

#include <algorithm>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <qmath.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVERSELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

HPIFitData::HPIFitData()
{

}


//*************************************************************************************************************

void HPIFitData::doDipfitConcurrent()
{
    // Initialize variables
    Eigen::RowVectorXd currentCoil = this->coilPos;
    Eigen::VectorXd currentData = this->sensorData;
    SensorInfo currentSensors = this->sensorPos;

    int display = 0;
    int maxiter = 500;
    int simplex_numitr = 0;

    this->coilPos = fminsearch(currentCoil,
                               maxiter,
                               2 * maxiter * currentCoil.cols(),
                               display,
                               currentData,
                               this->matProjector,
                               currentSensors,
                               simplex_numitr);

    this->errorInfo = dipfitError(currentCoil,
                                  currentData,
                                  currentSensors,
                                  this->matProjector);

    this->errorInfo.numIterations = simplex_numitr;
}


//*************************************************************************************************************

Eigen::MatrixXd HPIFitData::magnetic_dipole(Eigen::MatrixXd pos, Eigen::MatrixXd pnt, Eigen::MatrixXd ori)
{
    double u0 = 1e-7;
    int nchan;
    Eigen::MatrixXd r, r2, r5, x, y, z, mx, my, mz, Tx, Ty, Tz, lf;

    nchan = pnt.rows();

    // Shift the magnetometers so that the dipole is in the origin
    pnt.array().col(0) -= pos(0);
    pnt.array().col(1) -= pos(1);
    pnt.array().col(2) -= pos(2);

    r = pnt.array().square().rowwise().sum().sqrt();

    r2 = r5 = x = y = z = mx = my = mz = Tx = Ty = Tz = lf = Eigen::MatrixXd::Zero(nchan,3);

    for(int i = 0;i < nchan;i++) {
        r2.row(i).array().fill(pow(r(i),2));
        r5.row(i).array().fill(pow(r(i),5));
    }

    for(int i = 0;i < nchan;i++) {
        x.row(i).array().fill(pnt(i,0));
        y.row(i).array().fill(pnt(i,1));
        z.row(i).array().fill(pnt(i,2));
    }

    mx.col(0).array().fill(1);
    my.col(1).array().fill(1);
    mz.col(2).array().fill(1);

    Tx = 3 * x.cwiseProduct(pnt) - mx.cwiseProduct(r2);
    Ty = 3 * y.cwiseProduct(pnt) - my.cwiseProduct(r2);
    Tz = 3 * z.cwiseProduct(pnt) - mz.cwiseProduct(r2);

    for(int i = 0;i < nchan;i++) {
        lf(i,0) = Tx.row(i).dot(ori.row(i));
        lf(i,1) = Ty.row(i).dot(ori.row(i));
        lf(i,2) = Tz.row(i).dot(ori.row(i));
    }

    for(int i = 0;i < nchan;i++) {
        for(int j = 0;j < 3;j++) {
            lf(i,j) = u0 * lf(i,j)/(4 * M_PI * r5(i,j));
        }
    }

    return lf;
}


//*************************************************************************************************************

Eigen::MatrixXd HPIFitData::compute_leadfield(const Eigen::MatrixXd& pos, const struct SensorInfo& sensors)
{

    Eigen::MatrixXd pnt, ori, lf;

    pnt = sensors.coilpos; // position of each coil
    ori = sensors.coilori; // orientation of each coil

    lf = magnetic_dipole(pos, pnt, ori);

    lf = sensors.tra * lf;

    return lf;
}


//*************************************************************************************************************

DipFitError HPIFitData::dipfitError(const Eigen::MatrixXd& pos, const Eigen::MatrixXd& data, const struct SensorInfo& sensors, const Eigen::MatrixXd& matProjectors)
{
    // Variable Declaration
    struct DipFitError e;
    Eigen::MatrixXd lf, dif;

    // Compute lead field for a magnetic dipole in infinite vacuum
    lf = compute_leadfield(pos, sensors);

    e.moment = UTILSLIB::MNEMath::pinv(lf) * data;

    //dif = data - lf * e.moment;
    dif = data - matProjectors * lf * e.moment;

    e.error = dif.array().square().sum()/data.array().square().sum();

    e.numIterations = 0;

    return e;
}


//*************************************************************************************************************

bool HPIFitData::compare(HPISortStruct a, HPISortStruct b)
{
    return (a.base_arr < b.base_arr);
}


//*************************************************************************************************************

Eigen::MatrixXd HPIFitData::fminsearch(const Eigen::MatrixXd& pos,
                                       int maxiter,
                                       int maxfun,
                                       int display,
                                       const Eigen::MatrixXd& data,
                                       const Eigen::MatrixXd& matProjectors,
                                       const struct SensorInfo& sensors,
                                       int &simplex_numitr)
{
    double tolx, tolf, rho, chi, psi, sigma, func_evals, usual_delta, zero_term_delta, temp1, temp2;
    std::string header, how;
    int n, itercount, prnt;
    Eigen::MatrixXd onesn, two2np1, one2n, v, y, v1, tempX1, tempX2, xbar, xr, x, xe, xc, xcc, xin, posCopy;
    std::vector <double> fv, fv1;
    std::vector <int> idx;

    DipFitError tempdip, fxr, fxe, fxc, fxcc;

    //tolx = tolf = 1e-4;
    // Seok
    tolx = tolf = 1e-9;

    switch(display) {
        case 0:
            prnt = 0;
            break;
        default:
            prnt = 1;
    }

    header = " Iteration   Func-count     min f(x) Procedure";

    posCopy = pos;

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

    tempdip = dipfitError(posCopy, data, sensors, matProjectors);
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
        tempdip = dipfitError(posCopy, data, sensors, matProjectors);
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

    while ((func_evals < maxfun) && (itercount < maxiter)) {

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

        fxr = dipfitError(x, data, sensors, matProjectors);

        func_evals = func_evals+1;

        if (fxr.error < fv[0]) {
            // Calculate the expansion point
            xe = (1 + rho * chi) * xbar - rho * chi * v.col(v.cols()-1);
            x = xe.transpose();
            fxe = dipfitError(x, data, sensors, matProjectors);
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
                    fxc = dipfitError(x, data, sensors, matProjectors);
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
                    fxcc = dipfitError(x, data, sensors, matProjectors);
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
                        tempdip = dipfitError(x,data, sensors, matProjectors);
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
    simplex_numitr = itercount;

    return x;
}



