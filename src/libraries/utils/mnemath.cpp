//=============================================================================================================
/**
 * @file     mnemath.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Christoph Dinh, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the MNEMath Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include "mnemath.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Eigen>
#include <Eigen/Geometry>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

int MNEMath::gcd(int iA, int iB)
{
    if (iB == 0) {
        return iA;
    }

    return gcd(iB, iA % iB);
}

//=============================================================================================================

VectorXd* MNEMath::combine_xyz(const VectorXd& vec)
{
    if (vec.size() % 3 != 0)
    {
        printf("Input must be a row or a column vector with 3N components\n");
        return NULL;
    }

    MatrixXd tmp = MatrixXd(vec.transpose());
    SparseMatrix<double>* s = make_block_diag(tmp,3);

    SparseMatrix<double> sC = *s*s->transpose();
    VectorXd* comb = new VectorXd(sC.rows());

    for(qint32 i = 0; i < sC.rows(); ++i)
        (*comb)[i] = sC.coeff(i,i);

    delete s;
    return comb;
}

//=============================================================================================================

double MNEMath::getConditionNumber(const MatrixXd& A,
                                   VectorXd &s)
{
    JacobiSVD<MatrixXd> svd(A);
    s = svd.singularValues();

    double c = s.maxCoeff()/s.minCoeff();

    return c;
}

//=============================================================================================================

double MNEMath::getConditionSlope(const MatrixXd& A,
                                  VectorXd &s)
{
    JacobiSVD<MatrixXd> svd(A);
    s = svd.singularValues();

    double c = s.maxCoeff()/s.mean();

    return c;
}

//=============================================================================================================

void MNEMath::get_whitener(MatrixXd &A,
                           bool pca,
                           QString ch_type,
                           VectorXd &eig,
                           MatrixXd &eigvec)
{
    // whitening operator
    SelfAdjointEigenSolver<MatrixXd> t_eigenSolver(A);//Can be used because, covariance matrices are self-adjoint matrices.

    eig = t_eigenSolver.eigenvalues();
    eigvec = t_eigenSolver.eigenvectors().transpose();

    MNEMath::sort<double>(eig, eigvec, false);
    qint32 rnk = MNEMath::rank(A);

    for(qint32 i = 0; i < eig.size()-rnk; ++i)
        eig(i) = 0;

    printf("Setting small %s eigenvalues to zero.\n", ch_type.toUtf8().constData());
    if (!pca)  // No PCA case.
        printf("Not doing PCA for %s\n", ch_type.toUtf8().constData());
    else
    {
        printf("Doing PCA for %s.",ch_type.toUtf8().constData());
        // This line will reduce the actual number of variables in data
        // and leadfield to the true rank.
        eigvec = eigvec.block(eigvec.rows()-rnk, 0, rnk, eigvec.cols());
    }
}

//=============================================================================================================

void MNEMath::get_whitener(MatrixXd &A,
                           bool pca,
                           const std::string& ch_type,
                           VectorXd &eig,
                           MatrixXd &eigvec)
{
    // whitening operator
    SelfAdjointEigenSolver<MatrixXd> t_eigenSolver(A);//Can be used because, covariance matrices are self-adjoint matrices.

    eig = t_eigenSolver.eigenvalues();
    eigvec = t_eigenSolver.eigenvectors().transpose();

    MNEMath::sort<double>(eig, eigvec, false);
    qint32 rnk = MNEMath::rank(A);

    for(qint32 i = 0; i < eig.size()-rnk; ++i)
        eig(i) = 0;

    printf("Setting small %s eigenvalues to zero.\n", ch_type.c_str());
    if (!pca)  // No PCA case.
        printf("Not doing PCA for %s\n", ch_type.c_str());
    else
    {
        printf("Doing PCA for %s.",ch_type.c_str());
        // This line will reduce the actual number of variables in data
        // and leadfield to the true rank.
        eigvec = eigvec.block(eigvec.rows()-rnk, 0, rnk, eigvec.cols());
    }
}

//=============================================================================================================

VectorXi MNEMath::intersect(const VectorXi &v1,
                            const VectorXi &v2,
                            VectorXi &idx_sel)
{
    std::vector<int> tmp;

    std::vector< std::pair<int,int> > t_vecIntIdxValue;

    //ToDo:Slow; map VectorXi to stl container
    for(qint32 i = 0; i < v1.size(); ++i)
        tmp.push_back(v1[i]);

    std::vector<int>::iterator it;
    for(qint32 i = 0; i < v2.size(); ++i)
    {
        it = std::search(tmp.begin(), tmp.end(), &v2[i], &v2[i]+1);
        if(it != tmp.end())
            t_vecIntIdxValue.push_back(std::pair<int,int>(v2[i], it-tmp.begin()));//Index and int value are swapped // to sort using the idx
    }

    std::sort(t_vecIntIdxValue.begin(), t_vecIntIdxValue.end(), MNEMath::compareIdxValuePairSmallerThan<int>);

    VectorXi p_res(t_vecIntIdxValue.size());
    idx_sel = VectorXi(t_vecIntIdxValue.size());

    for(quint32 i = 0; i < t_vecIntIdxValue.size(); ++i)
    {
        p_res[i] = t_vecIntIdxValue[i].first;
        idx_sel[i] = t_vecIntIdxValue[i].second;
    }

    return p_res;
}

//=============================================================================================================

//    static inline MatrixXd extract_block_diag(MatrixXd& A, qint32 n)
//    {

//        //
//        // Principal Investigators and Developers:
//        // ** Richard M. Leahy, PhD, Signal & Image Processing Institute,
//        //    University of Southern California, Los Angeles, CA
//        // ** John C. Mosher, PhD, Biophysics Group,
//        //    Los Alamos National Laboratory, Los Alamos, NM
//        // ** Sylvain Baillet, PhD, Cognitive Neuroscience & Brain Imaging Laboratory,
//        //    CNRS, Hopital de la Salpetriere, Paris, France
//        //
//        // Copyright (c) 2005 BrainStorm by the University of Southern California
//        // This software distributed  under the terms of the GNU General Public License
//        // as published by the Free Software Foundation. Further details on the GPL
//        // license can be found at http://www.gnu.org/copyleft/gpl.html .
//        //
//        //FOR RESEARCH PURPOSES ONLY. THE SOFTWARE IS PROVIDED "AS IS," AND THE
//        // UNIVERSITY OF SOUTHERN CALIFORNIA AND ITS COLLABORATORS DO NOT MAKE ANY
//        // WARRANTY, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO WARRANTIES OF
//        // MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, NOR DO THEY ASSUME ANY
//        // LIABILITY OR RESPONSIBILITY FOR THE USE OF THIS SOFTWARE.
//        //
//        // Author: John C. Mosher 1993 - 2004
//        //
//        //
//        // Modifications for mne Matlab toolbox
//        //
//        //   Matti Hamalainen
//        //   2006

//          [mA,na] = size(A);		% matrix always has na columns
//          % how many entries in the first column?
//          bdn = na/n;			% number of blocks
//          ma = mA/bdn;			% rows in first block

//          % blocks may themselves contain zero entries.  Build indexing as above
//          tmp = reshape([1:(ma*bdn)]',ma,bdn);
//          i = zeros(ma*n,bdn);
//          for iblock = 1:n,
//            i((iblock-1)*ma+[1:ma],:) = tmp;
//          end

//          i = i(:); 			% row indices foreach sparse bd

//          j = [0:mA:(mA*(na-1))];
//          j = j(ones(ma,1),:);
//          j = j(:);

//          i = i + j;

//          bd = full(A(i)); 	% column vector
//          bd = reshape(bd,ma,na);	% full matrix

//    }

//=============================================================================================================

bool MNEMath::issparse(VectorXd &v)
{
    //ToDo: Figure out how to accelerate MNEMath::issparse(VectorXd &v)

    qint32 c = 0;
    qint32 n = v.rows();
    qint32 t = n/2;

    for(qint32 i = 0; i < n; ++i)
    {
        if(v(i) == 0)
            ++c;
        if(c > t)
            return true;
    }

    return false;
}

//=============================================================================================================

MatrixXd MNEMath::legendre(qint32 n,
                           const VectorXd &X,
                           QString normalize)
{
    MatrixXd y;

    Q_UNUSED(y);

    Q_UNUSED(n);
    Q_UNUSED(X);
    Q_UNUSED(normalize);

    //ToDo

    return y;
}

//=============================================================================================================

MatrixXd MNEMath::legendre(qint32 n,
                           const VectorXd &X,
                           std::string normalize)
{
    MatrixXd y;

    Q_UNUSED(y);

    Q_UNUSED(n);
    Q_UNUSED(X);
    Q_UNUSED(normalize);

    //ToDo

    return y;
}

//=============================================================================================================

SparseMatrix<double>* MNEMath::make_block_diag(const MatrixXd &A,
                                               qint32 n)
{

    qint32 ma = A.rows();
    qint32 na = A.cols();
    float bdn = ((float)na)/n;      // number of submatrices

//    std::cout << std::endl << "ma " << ma << " na " << na << " bdn " << bdn << std::endl;

    if(bdn - floor(bdn))
    {
        printf("Width of matrix must be even multiple of n\n");
        return NULL;
    }

    typedef Eigen::Triplet<double> T;
    std::vector<T> tripletList;
    tripletList.reserve(bdn*ma*n);

    qint32 current_col, current_row, i, r, c;
    for(i = 0; i < bdn; ++i)
    {
        current_col = i * n;
        current_row = i * ma;

        for(r = 0; r < ma; ++r)
            for(c = 0; c < n; ++c)
                tripletList.push_back(T(r+current_row, c+current_col, A(r, c+current_col)));
    }

    SparseMatrix<double>* bd = new SparseMatrix<double>((int)floor((float)ma*bdn+0.5),na);
//    SparseMatrix<double> p_Matrix(nrow, ncol);
    bd->setFromTriplets(tripletList.begin(), tripletList.end());

    return bd;
}

//=============================================================================================================

int MNEMath::nchoose2(int n)
{

    //nchoosek(n, k) with k = 2, equals n*(n-1)*0.5

    int t_iNumOfCombination = (int)(n*(n-1)*0.5);

    return t_iNumOfCombination;
}

//=============================================================================================================

qint32 MNEMath::rank(const MatrixXd& A,
                     double tol)
{
    JacobiSVD<MatrixXd> t_svdA(A);//U and V are not computed
    VectorXd s = t_svdA.singularValues();
    double t_dMax = s.maxCoeff();
    t_dMax *= tol;
    qint32 sum = 0;
    for(qint32 i = 0; i < s.size(); ++i)
        sum += s[i] > t_dMax ? 1 : 0;
    return sum;
}

//=============================================================================================================

MatrixXd MNEMath::rescale(const MatrixXd &data,
                          const RowVectorXf &times,
                          const QPair<float,float>& baseline,
                          QString mode)
{
    MatrixXd data_out = data;
    QStringList valid_modes;
    valid_modes << "logratio" << "ratio" << "zscore" << "mean" << "percent";
    if(!valid_modes.contains(mode))
    {
        qWarning().noquote() << "[MNEMath::rescale] Mode" << mode << "is not supported. Supported modes are:" << valid_modes << "Returning input data.";
        return data_out;
    }

    qInfo().noquote() << QString("[MNEMath::rescale] Applying baseline correction ... (mode: %1)").arg(mode);

    qint32 imin = 0;
    qint32 imax = times.size();

    if (baseline.second == baseline.first) {
        imin = 0;
    } else {
        float bmin = baseline.first;
        for(qint32 i = 0; i < times.size(); ++i) {
            if(times[i] >= bmin) {
                imin = i;
                break;
            }
        }
    }

    float bmax = baseline.second;

    if (baseline.second == baseline.first) {
        bmax = 0;
    }

    for(qint32 i = times.size()-1; i >= 0; --i) {
        if(times[i] <= bmax) {
            imax = i+1;
            break;
        }
    }

    if(imax < imin) {
        qWarning() << "[MNEMath::rescale] imax < imin. Returning input data.";
        return data_out;
    }

    VectorXd mean = data_out.block(0, imin,data_out.rows(),imax-imin).rowwise().mean();
    if(mode.compare("mean") == 0) {
        data_out -= mean.rowwise().replicate(data.cols());
    } else if(mode.compare("logratio") == 0) {
        for(qint32 i = 0; i < data_out.rows(); ++i)
            for(qint32 j = 0; j < data_out.cols(); ++j)
                data_out(i,j) = log10(data_out(i,j)/mean[i]); // a value of 1 means 10 times bigger
    } else if(mode.compare("ratio") == 0) {
        data_out = data_out.cwiseQuotient(mean.rowwise().replicate(data_out.cols()));
    } else if(mode.compare("zscore") == 0) {
        MatrixXd std_mat = data.block(0, imin, data.rows(), imax-imin) - mean.rowwise().replicate(imax-imin);
        std_mat = std_mat.cwiseProduct(std_mat);
        VectorXd std_v = std_mat.rowwise().mean();
        for(qint32 i = 0; i < std_v.size(); ++i)
            std_v[i] = sqrt(std_v[i] / (float)(imax-imin));

        data_out -= mean.rowwise().replicate(data_out.cols());
        data_out = data_out.cwiseQuotient(std_v.rowwise().replicate(data_out.cols()));
    } else if(mode.compare("percent") == 0) {
        data_out -= mean.rowwise().replicate(data_out.cols());
        data_out = data_out.cwiseQuotient(mean.rowwise().replicate(data_out.cols()));
    }

    return data_out;
}

//=============================================================================================================

MatrixXd MNEMath::rescale(const MatrixXd& data,
                          const RowVectorXf& times,
                          const std::pair<float,float>& baseline,
                          const std::string& mode)
{
    MatrixXd data_out = data;
    std::vector<std::string> valid_modes{"logratio", "ratio", "zscore", "mean", "percent"};
    if(std::find(valid_modes.begin(),valid_modes.end(), mode) == valid_modes.end())
    {
        qWarning().noquote() << "[MNEMath::rescale] Mode" << mode.c_str() << "is not supported. Supported modes are:";
        for(auto& mode : valid_modes){
            std::cout << mode << " ";
        }
        std::cout << "\n" << "Returning input data.\n";
        return data_out;
    }

    qInfo().noquote() << QString("[MNEMath::rescale] Applying baseline correction ... (mode: %1)").arg(mode.c_str());

    qint32 imin = 0;
    qint32 imax = times.size();

    if (baseline.second == baseline.first) {
        imin = 0;
    } else {
        float bmin = baseline.first;
        for(qint32 i = 0; i < times.size(); ++i) {
            if(times[i] >= bmin) {
                imin = i;
                break;
            }
        }
    }

    float bmax = baseline.second;

    if (baseline.second == baseline.first) {
        bmax = 0;
    }

    for(qint32 i = times.size()-1; i >= 0; --i) {
        if(times[i] <= bmax) {
            imax = i+1;
            break;
        }
    }

    if(imax < imin) {
        qWarning() << "[MNEMath::rescale] imax < imin. Returning input data.";
        return data_out;
    }

    VectorXd mean = data_out.block(0, imin,data_out.rows(),imax-imin).rowwise().mean();
    if(mode.compare("mean") == 0) {
        data_out -= mean.rowwise().replicate(data.cols());
    } else if(mode.compare("logratio") == 0) {
        for(qint32 i = 0; i < data_out.rows(); ++i)
            for(qint32 j = 0; j < data_out.cols(); ++j)
                data_out(i,j) = log10(data_out(i,j)/mean[i]); // a value of 1 means 10 times bigger
    } else if(mode.compare("ratio") == 0) {
        data_out = data_out.cwiseQuotient(mean.rowwise().replicate(data_out.cols()));
    } else if(mode.compare("zscore") == 0) {
        MatrixXd std_mat = data.block(0, imin, data.rows(), imax-imin) - mean.rowwise().replicate(imax-imin);
        std_mat = std_mat.cwiseProduct(std_mat);
        VectorXd std_v = std_mat.rowwise().mean();
        for(qint32 i = 0; i < std_v.size(); ++i)
            std_v[i] = sqrt(std_v[i] / (float)(imax-imin));

        data_out -= mean.rowwise().replicate(data_out.cols());
        data_out = data_out.cwiseQuotient(std_v.rowwise().replicate(data_out.cols()));
    } else if(mode.compare("percent") == 0) {
        data_out -= mean.rowwise().replicate(data_out.cols());
        data_out = data_out.cwiseQuotient(mean.rowwise().replicate(data_out.cols()));
    }

    return data_out;
}

//=============================================================================================================

bool MNEMath::compareTransformation(const MatrixX4f& mDevHeadT,
                                    const MatrixX4f& mDevHeadDest,
                                    const float& fThreshRot,
                                    const float& fThreshTrans)
{
    bool bState = false;

    Matrix3f mRot = mDevHeadT.block(0,0,3,3);
    Matrix3f mRotDest = mDevHeadDest.block(0,0,3,3);

    VectorXf vTrans = mDevHeadT.block(0,3,3,1);
    VectorXf vTransDest = mDevHeadDest.block(0,3,3,1);

    Quaternionf quat(mRot);
    Quaternionf quatNew(mRotDest);

    // Compare Rotation
    Quaternionf quatCompare;
    float fAngle;

    // get rotation between both transformations by multiplying with the inverted quaternion
    quatCompare = quat*quatNew.inverse();
    fAngle = quat.angularDistance(quatNew);
    fAngle = fAngle * 180 / M_PI;

    // Compare translation
    float fMove = (vTrans-vTransDest).norm();

    // compare to thresholds and update
    if(fMove > fThreshTrans) {
        qInfo() << "Large movement: " << fMove*1000 << "mm";
        bState = true;

    } else if (fAngle > fThreshRot) {
        qInfo() << "Large rotation: " << fAngle << "degree";
        bState = true;

    } else {
        bState = false;
    }

    return bState;
}
