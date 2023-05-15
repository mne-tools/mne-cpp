//=============================================================================================================
/**
 * @file     fiff_proj.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the FiffProj Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_proj.h"
#include <stdio.h>
#include <utils/mnemath.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SVD>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffProj::FiffProj()
: kind(-1)
, active(false)
, desc("")
, data(new FiffNamedMatrix)
{

}

//=============================================================================================================

FiffProj::FiffProj(const FiffProj& p_FiffProj)
: kind(p_FiffProj.kind)
, active(p_FiffProj.active)
, desc(p_FiffProj.desc)
, data(p_FiffProj.data)
{

}

//=============================================================================================================

FiffProj::FiffProj( fiff_int_t p_kind, bool p_active, QString p_desc, FiffNamedMatrix& p_data)
: kind(p_kind)
, active(p_active)
, desc(p_desc)
, data(new FiffNamedMatrix(p_data))
{

}

//=============================================================================================================

FiffProj::~FiffProj()
{

}

//=============================================================================================================

void FiffProj::activate_projs(QList<FiffProj> &p_qListFiffProj)
{
    // Activate the projection items
    QList<FiffProj>::Iterator it;
    for(it = p_qListFiffProj.begin(); it != p_qListFiffProj.end(); ++it)
        it->active = true;

    printf("\t%lld projection items activated.\n", p_qListFiffProj.size());
}

//=============================================================================================================

fiff_int_t FiffProj::make_projector(const QList<FiffProj>& projs, const QStringList& ch_names, MatrixXd& proj, const QStringList& bads, MatrixXd& U)
{
    fiff_int_t nchan = ch_names.size();
    if (nchan == 0)
    {
        printf("No channel names specified\n");//ToDo throw here
        return 0;
    }

//    if(proj)
//        delete proj;
    proj = MatrixXd::Identity(nchan,nchan);
    fiff_int_t nproj = 0;
    U = MatrixXd();

    //
    //   Check trivial cases first
    //
    if (projs.size() == 0)
        return 0;

    fiff_int_t nvec    = 0;
    fiff_int_t k, l;
    for (k = 0; k < projs.size(); ++k)
    {
        if (projs[k].active)
        {
            ++nproj;
            nvec += projs[k].data->nrow;
        }
    }

    if (nproj == 0) {
        printf("FiffProj::make_projector - No projectors nproj=0\n");
        return 0;
    }

    if (nvec <= 0) {
        printf("FiffProj::make_projector - No rows in projector matrices found nvec<=0\n");
        return 0;
    }

    //
    //   Pick the appropriate entries
    //
    MatrixXd vecs = MatrixXd::Zero(nchan,nvec);
    nvec = 0;
    fiff_int_t nonzero = 0;
    qint32 p, c, i, j, v;
    double onesize;
    bool isBad = false;
    RowVectorXi sel(nchan);
    RowVectorXi vecSel(nchan);
    sel.setConstant(-1);
    vecSel.setConstant(-1);
    for (k = 0; k < projs.size(); ++k)
    {
        if (projs[k].active)
        {
            FiffProj one = projs[k];

            QMap<QString, int> uniqueMap;
            for(l = 0; l < one.data->col_names.size(); ++l)
                uniqueMap[one.data->col_names[l] ] = 0;

            if (one.data->col_names.size() != uniqueMap.keys().size())
            {
                printf("Channel name list in projection item %d contains duplicate items",k);
                return 0;
            }

            //
            // Get the two selection vectors to pick correct elements from
            // the projection vectors omitting bad channels
            //
            sel.resize(nchan);
            vecSel.resize(nchan);
            sel.setConstant(-1);
            vecSel.setConstant(-1);
            p = 0;
            for (c = 0; c < nchan; ++c)
            {
                for (i = 0; i < one.data->col_names.size(); ++i)
                {
                    if (QString::compare(ch_names.at(c),one.data->col_names[i]) == 0)
                    {
                        isBad = false;
                        for (j = 0; j < bads.size(); ++j)
                        {
                            if (QString::compare(ch_names.at(c),bads.at(j)) == 0)
                            {
                                isBad = true;
                            }
                        }

                        if (!isBad && sel[p] != c)
                        {
                            sel[p] = c;
                            vecSel[p] = i;
                            ++p;
                        }

                    }
                }
            }
            sel.conservativeResize(p);
            vecSel.conservativeResize(p);
            //
            // If there is something to pick, pickit
            //
            if (sel.cols() > 0)
                for (v = 0; v < one.data->nrow; ++v)
                    for (i = 0; i < p; ++i)
                        vecs(sel[i],nvec+v) = one.data->data(v,vecSel[i]);

            //
            //   Rescale for more straightforward detection of small singular values
            //
            for (v = 0; v < one.data->nrow; ++v)
            {
                onesize = sqrt((vecs.col(nvec+v).transpose()*vecs.col(nvec+v))(0,0));
                if (onesize > 0.0)
                {
                    vecs.col(nvec+v) = vecs.col(nvec+v)/onesize;
                    ++nonzero;
                }
            }
            nvec += one.data->nrow;
        }
    }
    //
    //   Check whether all of the vectors are exactly zero
    //
    if (nonzero == 0)
        return 0;

    //
    //   Reorthogonalize the vectors
    //
    JacobiSVD<MatrixXd> svd(vecs.block(0,0,vecs.rows(),nvec), ComputeFullU);
    //Sort singular values and singular vectors
    VectorXd S = svd.singularValues();
    MatrixXd t_U = svd.matrixU();
    MNEMath::sort<double>(S, t_U);

    //
    //   Throw away the linearly dependent guys
    //
    nproj = 0;
    for(k = 0; k < S.size(); ++k)
        if (S[k]/S[0] > 1e-2)
            ++nproj;

    U = t_U.block(0, 0, t_U.rows(), nproj);

    //
    //   Here is the celebrated result
    //
    proj -= U*U.transpose();

    return nproj;
}
