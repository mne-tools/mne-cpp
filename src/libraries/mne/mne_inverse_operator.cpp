//=============================================================================================================
/**
 * @file     mne_inverse_operator.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Gabriel B Motta, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the MNEInverseOperator Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_inverse_operator.h"
#include <fs/fs_label.h>
#include <math/linalg.h>

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFuture>
#include <QtConcurrent>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SVD>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace MNELIB;
using namespace MNELIB;
using namespace FIFFLIB;
using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEInverseOperator::MNEInverseOperator()
: methods(-1)
, source_ori(-1)
, nsource(-1)
, nchan(-1)
, coord_frame(-1)
, eigen_leads_weighted(false)
, eigen_leads(new FiffNamedMatrix)
, eigen_fields(new FiffNamedMatrix)
, noise_cov(new FiffCov)
, source_cov(new FiffCov)
, orient_prior(new FiffCov)
, depth_prior(new FiffCov)
, fmri_prior(new FiffCov)
, nave(-1)
{
    qRegisterMetaType<QSharedPointer<MNELIB::MNEInverseOperator> >("QSharedPointer<MNELIB::MNEInverseOperator>");
    qRegisterMetaType<MNELIB::MNEInverseOperator>("MNELIB::MNEInverseOperator");
}

//=============================================================================================================

MNEInverseOperator::MNEInverseOperator(QIODevice& p_IODevice)
{
    MNEInverseOperator::read_inverse_operator(p_IODevice, *this);
    qRegisterMetaType<QSharedPointer<MNELIB::MNEInverseOperator> >("QSharedPointer<MNELIB::MNEInverseOperator>");
    qRegisterMetaType<MNELIB::MNEInverseOperator>("MNELIB::MNEInverseOperator");
}

//=============================================================================================================

MNEInverseOperator::MNEInverseOperator(const FiffInfo &info,
                                       const MNEForwardSolution& forward,
                                       const FiffCov& p_noise_cov,
                                       float loose,
                                       float depth,
                                       bool fixed,
                                       bool limit_depth_chs)
{
     *this = MNEInverseOperator::make_inverse_operator(info, forward, p_noise_cov, loose, depth, fixed, limit_depth_chs);
    qRegisterMetaType<QSharedPointer<MNELIB::MNEInverseOperator> >("QSharedPointer<MNELIB::MNEInverseOperator>");
    qRegisterMetaType<MNELIB::MNEInverseOperator>("MNELIB::MNEInverseOperator");
}

//=============================================================================================================

MNEInverseOperator::MNEInverseOperator(const MNEInverseOperator &other)
: info(other.info)
, methods(other.methods)
, source_ori(other.source_ori)
, nsource(other.nsource)
, nchan(other.nchan)
, coord_frame(other.coord_frame)
, source_nn(other.source_nn)
, sing(other.sing)
, eigen_leads_weighted(other.eigen_leads_weighted)
, eigen_leads(other.eigen_leads)
, eigen_fields(other.eigen_fields)
, noise_cov(other.noise_cov)
, source_cov(other.source_cov)
, orient_prior(other.orient_prior)
, depth_prior(other.depth_prior)
, fmri_prior(other.fmri_prior)
, src(other.src)
, mri_head_t(other.mri_head_t)
, nave(other.nave)
, projs(other.projs)
, proj(other.proj)
, whitener(other.whitener)
, reginv(other.reginv)
, noisenorm(other.noisenorm)
{
    qRegisterMetaType<QSharedPointer<MNELIB::MNEInverseOperator> >("QSharedPointer<MNELIB::MNEInverseOperator>");
    qRegisterMetaType<MNELIB::MNEInverseOperator>("MNELIB::MNEInverseOperator");
}

//=============================================================================================================

MNEInverseOperator::~MNEInverseOperator() = default;

//=============================================================================================================

bool MNEInverseOperator::assemble_kernel(const FsLabel &label,
                                         const QString &method,
                                         bool pick_normal,
                                         MatrixXd &K,
                                         SparseMatrix<double> &noise_norm,
                                         QList<VectorXi> &vertno)
{
    MatrixXd t_eigen_leads = eigen_leads->data;
    MatrixXd t_source_cov = source_cov->data;
    if(method.compare(QLatin1String("MNE")) != 0)
        noise_norm = noisenorm;

    vertno = src.get_vertno();

    typedef Eigen::Triplet<double> T;
    std::vector<T> tripletList;

    if(!label.isEmpty())
    {
        qWarning("Label selection needs further debugging.");
        VectorXi src_sel;
        vertno = src.label_src_vertno_sel(label, src_sel);

        if(method.compare(QLatin1String("MNE")) != 0)
        {
            tripletList.clear();
            tripletList.reserve(noise_norm.nonZeros());

            for (qint32 k = 0; k < noise_norm.outerSize(); ++k)
            {
                for (SparseMatrix<double>::InnerIterator it(noise_norm,k); it; ++it)
                {
                    qint32 row = -1;
                    for(qint32 i = 0; i < src_sel.size(); ++i)
                    {
                        if(src_sel[i] == it.row())
                        {
                            row = i;
                            break;
                        }
                    }
                    if(row != -1)
                        tripletList.push_back(T(it.row(), it.col(), it.value()));
                }
            }

            noise_norm = SparseMatrix<double>(src_sel.size(),noise_norm.cols());
            noise_norm.setFromTriplets(tripletList.begin(), tripletList.end());
        }

        if(source_ori == FIFFV_MNE_FREE_ORI)
        {
            VectorXi src_sel_new(src_sel.size()*3);

            for(qint32 i = 0; i < src_sel.size(); ++i)
            {
                src_sel_new[i*3] = src_sel[i]*3;
                src_sel_new[i*3+1] = src_sel[i]*3+1;
                src_sel_new[i*3+2] = src_sel[i]*3+2;
            }
            src_sel = src_sel_new;
        }

        for(qint32 i = 0; i < src_sel.size(); ++i)
        {
            t_eigen_leads.row(i) = t_eigen_leads.row(src_sel[i]);
            t_source_cov = t_source_cov.row(src_sel[i]);
        }
        t_eigen_leads.conservativeResize(src_sel.size(), t_eigen_leads.cols());
        t_source_cov.conservativeResize(src_sel.size(), t_source_cov.cols());
    }

    if(pick_normal)
    {
        if(source_ori != FIFFV_MNE_FREE_ORI)
        {
            qWarning("Warning: Pick normal can only be used with a free orientation inverse operator.\n");
            return false;
        }

        bool is_loose = (0 < orient_prior->data(0,0)) && (orient_prior->data(0,0) < 1);
        if(!is_loose)
        {
            qWarning("The pick_normal parameter is only valid when working with loose orientations.\n");
            return false;
        }

        // keep only the normal components
        qint32 count = 0;
        for(qint32 i = 2; i < t_eigen_leads.rows(); i+=3)
        {
            t_eigen_leads.row(count) = t_eigen_leads.row(i);
            ++count;
        }
        t_eigen_leads.conservativeResize(count, t_eigen_leads.cols());

        count = 0;
        for(qint32 i = 2; i < t_source_cov.rows(); i+=3)
        {
            t_source_cov.row(count) = t_source_cov.row(i);
            ++count;
        }
        t_source_cov.conservativeResize(count, t_source_cov.cols());
    }

    tripletList.clear();
    tripletList.reserve(reginv.rows());
    for(qint32 i = 0; i < reginv.rows(); ++i)
        tripletList.push_back(T(i, i, reginv(i,0)));
    SparseMatrix<double> t_reginv(reginv.rows(),reginv.rows());
    t_reginv.setFromTriplets(tripletList.begin(), tripletList.end());

    MatrixXd trans = t_reginv*eigen_fields->data*whitener*proj;
    //
    //   Transformation into current distributions by weighting the eigenleads
    //   with the weights computed above
    //
    if (eigen_leads_weighted)
    {
        //
        //     R^0.5 has been already factored in
        //
        printf("(eigenleads already weighted)...\n");
        K = t_eigen_leads*trans;
    }
    else
    {
        //
        //     R^0.5 has to factored in
        //
       printf("(eigenleads need to be weighted)...\n");

       std::vector<T> tripletList2;
       tripletList2.reserve(t_source_cov.rows());
       for(qint32 i = 0; i < t_source_cov.rows(); ++i)
           tripletList2.push_back(T(i, i, sqrt(t_source_cov(i,0))));
       SparseMatrix<double> t_sourceCov(t_source_cov.rows(),t_source_cov.rows());
       t_sourceCov.setFromTriplets(tripletList2.begin(), tripletList2.end());

       K = t_sourceCov*t_eigen_leads*trans;
    }

    if(method.compare(QLatin1String("MNE")) == 0)
        noise_norm = SparseMatrix<double>();

    //store assembled kernel
    m_K = K;

    return true;
}

//=============================================================================================================

bool MNEInverseOperator::check_ch_names(const FiffInfo &info) const
{
    QStringList inv_ch_names = this->eigen_fields->col_names;

    bool t_bContains = true;
    if(this->eigen_fields->col_names.size() != this->noise_cov->names.size())
        t_bContains = false;
    else
    {
        for(qint32 i = 0; i < this->noise_cov->names.size(); ++i)
        {
            if(inv_ch_names[i] != this->noise_cov->names[i])
            {
                t_bContains = false;
                break;
            }
        }
    }

    if(!t_bContains)
    {
        qCritical("Channels in inverse operator eigen fields do not match noise covariance channels.");
        return false;
    }

    QStringList data_ch_names = info.ch_names;

    QStringList missing_ch_names;
    for(qint32 i = 0; i < inv_ch_names.size(); ++i)
        if(!data_ch_names.contains(inv_ch_names[i]))
            missing_ch_names.append(inv_ch_names[i]);

    qint32 n_missing = missing_ch_names.size();

    if(n_missing > 0)
    {
        qCritical() << n_missing << "channels in inverse operator are not present in the data (" << missing_ch_names << ")";
        return false;
    }

    return true;
}

//=============================================================================================================

MatrixXd MNEInverseOperator::cluster_kernel(const FsAnnotationSet &p_AnnotationSet, qint32 p_iClusterSize, MatrixXd& p_D, const QString &p_sMethod) const
{
    printf("Cluster kernel using %s.\n", p_sMethod.toUtf8().constData());

    MatrixXd p_outMT = m_K.transpose();

    QList<MNEClusterInfo> t_qListMNEClusterInfo;
    MNEClusterInfo t_MNEClusterInfo;
    t_qListMNEClusterInfo.append(t_MNEClusterInfo);
    t_qListMNEClusterInfo.append(t_MNEClusterInfo);

    //
    // Check consistency
    //
    if(isFixedOrient())
    {
        printf("Error: Fixed orientation not implemented yet!\n");
        return p_outMT;
    }

    //
    // Assemble input data
    //
    qint32 count;
    qint32 offset;

    MatrixXd t_MT_new;

    for(qint32 h = 0; h < this->src.size(); ++h )
    {

        count = 0;
        offset = 0;

        if(h > 0)
            for(qint32 j = 0; j < h; ++j)
                offset += this->src[j].nuse;

        if(h == 0)
            printf("Cluster Left Hemisphere\n");
        else
            printf("Cluster Right Hemisphere\n");

        FsColortable t_CurrentColorTable = p_AnnotationSet[h].getColortable();
        VectorXi label_ids = t_CurrentColorTable.getLabelIds();

        // Get label ids for every vertex
        VectorXi vertno_labeled = VectorXi::Zero(this->src[h].vertno.rows());

        for(qint32 i = 0; i < vertno_labeled.rows(); ++i)
            vertno_labeled[i] = p_AnnotationSet[h].getLabelIds()[this->src[h].vertno[i]];

        //Qt Concurrent List
        QList<RegionMT> m_qListRegionMTIn;

        //
        // Generate cluster input data
        //
        for (qint32 i = 0; i < label_ids.rows(); ++i)
        {
            if (label_ids[i] != 0)
            {
                QString curr_name = t_CurrentColorTable.struct_names[i];//obj.label2AtlasName(label(i));
                printf("\tCluster %d / %ld %s...", i+1, label_ids.rows(), curr_name.toUtf8().constData());

                //
                // Get source space indeces
                //
                VectorXi idcs = VectorXi::Zero(vertno_labeled.rows());
                qint32 c = 0;

                //Select ROIs //change this use label info with a hash tabel
                for(qint32 j = 0; j < vertno_labeled.rows(); ++j)
                {
                    if(vertno_labeled[j] == label_ids[i])
                    {
                        idcs[c] = j;
                        ++c;
                    }
                }
                idcs.conservativeResize(c);

                //get selected MT
                MatrixXd t_MT(p_outMT.rows(), idcs.rows()*3);

                for(qint32 j = 0; j < idcs.rows(); ++j)
                    t_MT.block(0, j*3, t_MT.rows(), 3) = p_outMT.block(0, (idcs[j]+offset)*3, t_MT.rows(), 3);

                qint32 nSens = t_MT.rows();
                qint32 nSources = t_MT.cols()/3;

                if (nSources > 0)
                {
                    RegionMT t_sensMT;

                    t_sensMT.idcs = idcs;
                    t_sensMT.iLabelIdxIn = i;
                    t_sensMT.nClusters = ceil(static_cast<double>(nSources)/static_cast<double>(p_iClusterSize));

                    t_sensMT.matRoiMTOrig = t_MT;

                    printf("%d Cluster(s)... ", t_sensMT.nClusters);

                    // Reshape Input data -> sources rows; sensors columns
                    t_sensMT.matRoiMT = MatrixXd(t_MT.cols()/3, 3*nSens);

                    for(qint32 j = 0; j < nSens; ++j)
                        for(qint32 k = 0; k < t_sensMT.matRoiMT.rows(); ++k)
                            t_sensMT.matRoiMT.block(k,j*3,1,3) = t_MT.block(j,k*3,1,3);

                    t_sensMT.sDistMeasure = p_sMethod;

                    m_qListRegionMTIn.append(t_sensMT);

                    printf("[added]\n");
                }
                else
                {
                    printf("failed! FsLabel contains no sources.\n");
                }
            }
        }

        //
        // Calculate clusters
        //
        printf("Clustering... ");
        QFuture< RegionMTOut > res;
        res = QtConcurrent::mapped(m_qListRegionMTIn, &RegionMT::cluster);
        res.waitForFinished();

        //
        // Assign results
        //
        MatrixXd t_MT_partial;

        qint32 nClusters;
        qint32 nSens;
        QList<RegionMT>::const_iterator itIn;
        itIn = m_qListRegionMTIn.begin();
        QFuture<RegionMTOut>::const_iterator itOut;
        for (itOut = res.constBegin(); itOut != res.constEnd(); ++itOut)
        {
            nClusters = itOut->ctrs.rows();
            nSens = itOut->ctrs.cols()/3;
            t_MT_partial = MatrixXd::Zero(nSens, nClusters*3);

            //
            // Assign the centroid for each cluster to the partial G
            //
            for(qint32 j = 0; j < nSens; ++j)
                for(qint32 k = 0; k < nClusters; ++k)
                    t_MT_partial.block(j, k*3, 1, 3) = itOut->ctrs.block(k,j*3,1,3);

            //
            // Get cluster indices and their distances to the centroid
            //
            for(qint32 j = 0; j < nClusters; ++j)
            {
                VectorXi clusterIdcs = VectorXi::Zero(itOut->roiIdx.rows());
                VectorXd clusterDistance = VectorXd::Zero(itOut->roiIdx.rows());
                qint32 nClusterIdcs = 0;
                for(qint32 k = 0; k < itOut->roiIdx.rows(); ++k)
                {
                    if(itOut->roiIdx[k] == j)
                    {
                        clusterIdcs[nClusterIdcs] = itIn->idcs[k];
                        clusterDistance[nClusterIdcs] = itOut->D(k,j);
                        ++nClusterIdcs;
                    }
                }
                clusterIdcs.conservativeResize(nClusterIdcs);
                clusterDistance.conservativeResize(nClusterIdcs);

                VectorXi clusterVertnos = VectorXi::Zero(clusterIdcs.size());
                for(qint32 k = 0; k < clusterVertnos.size(); ++k)
                    clusterVertnos(k) = this->src[h].vertno[clusterIdcs(k)];

                t_qListMNEClusterInfo[h].clusterVertnos.append(clusterVertnos);

            }

            //
            // Assign partial G to new LeadField
            //
            if(t_MT_partial.rows() > 0 && t_MT_partial.cols() > 0)
            {
                t_MT_new.conservativeResize(t_MT_partial.rows(), t_MT_new.cols() + t_MT_partial.cols());
                t_MT_new.block(0, t_MT_new.cols() - t_MT_partial.cols(), t_MT_new.rows(), t_MT_partial.cols()) = t_MT_partial;

                // Map the centroids to the closest rr
                for(qint32 k = 0; k < nClusters; ++k)
                {
                    qint32 j = 0;

                    double sqec = sqrt((itIn->matRoiMTOrig.block(0, j*3, itIn->matRoiMTOrig.rows(), 3) - t_MT_partial.block(0, k*3, t_MT_partial.rows(), 3)).array().pow(2).sum());
                    double sqec_min = sqec;
                    qint32 j_min = 0;
                    for(qint32 j = 1; j < itIn->idcs.rows(); ++j)
                    {
                        sqec = sqrt((itIn->matRoiMTOrig.block(0, j*3, itIn->matRoiMTOrig.rows(), 3) - t_MT_partial.block(0, k*3, t_MT_partial.rows(), 3)).array().pow(2).sum());

                        if(sqec < sqec_min)
                        {
                            sqec_min = sqec;
                            j_min = j;
                        }
                    }
                    (void)j_min;

                    ++count;
                }
            }

            ++itIn;
        }

        printf("[done]\n");
    }

    //
    // Cluster operator D (sources x clusters)
    //
    qint32 totalNumOfClust = 0;
    for (qint32 h = 0; h < 2; ++h)
        totalNumOfClust += t_qListMNEClusterInfo[h].clusterVertnos.size();

    if(isFixedOrient())
        p_D = MatrixXd::Zero(p_outMT.cols(), totalNumOfClust);
    else
        p_D = MatrixXd::Zero(p_outMT.cols(), totalNumOfClust*3);

    QList<VectorXi> t_vertnos = src.get_vertno();

    qint32 currentCluster = 0;
    for (qint32 h = 0; h < 2; ++h)
    {
        int hemiOffset = h == 0 ? 0 : t_vertnos[0].size();
        for(qint32 i = 0; i < t_qListMNEClusterInfo[h].clusterVertnos.size(); ++i)
        {
            VectorXi idx_sel;
            Linalg::intersect(t_vertnos[h], t_qListMNEClusterInfo[h].clusterVertnos[i], idx_sel);

            idx_sel.array() += hemiOffset;

            double selectWeight = 1.0/idx_sel.size();
            if(isFixedOrient())
            {
                for(qint32 j = 0; j < idx_sel.size(); ++j)
                    p_D.col(currentCluster)[idx_sel(j)] = selectWeight;
            }
            else
            {
                qint32 clustOffset = currentCluster*3;
                for(qint32 j = 0; j < idx_sel.size(); ++j)
                {
                    qint32 idx_sel_Offset = idx_sel(j)*3;
                    //x
                    p_D(idx_sel_Offset,clustOffset) = selectWeight;
                    //y
                    p_D(idx_sel_Offset+1, clustOffset+1) = selectWeight;
                    //z
                    p_D(idx_sel_Offset+2, clustOffset+2) = selectWeight;
                }
            }
            ++currentCluster;
        }
    }

    //
    // Put it all together
    //
    p_outMT = t_MT_new;

    return p_outMT;
}

//=============================================================================================================

MNEInverseOperator MNEInverseOperator::make_inverse_operator(const FiffInfo &info,
                                                             MNEForwardSolution forward,
                                                             const FiffCov &p_noise_cov,
                                                             float loose,
                                                             float depth,
                                                             bool fixed,
                                                             bool limit_depth_chs)
{
    bool is_fixed_ori = forward.isFixedOrient();
    MNEInverseOperator inv;

    //
    // Surface-orientation sanity check
    //
    if(fixed && !forward.surf_ori)
    {
        qWarning("Warning: Forward solution is not surface-oriented. A surface-oriented solution is recommended for fixed-orientation inverse operators.");
    }

    //Check parameters
    if(fixed && loose > 0)
    {
        qWarning("Warning: When invoking make_inverse_operator with fixed = true, the loose parameter is ignored.\n");
        loose = 0.0f;
    }

    if(is_fixed_ori && !fixed)
    {
        qWarning("Warning: Setting fixed parameter = true. Because the given forward operator has fixed orientation and can only be used to make a fixed-orientation inverse operator.\n");
        fixed = true;
    }

    if(forward.source_ori == -1 && loose > 0)
    {
        qCritical("Forward solution is not oriented in surface coordinates. loose parameter should be 0 not %f.", loose);
        return inv;
    }

    if(loose < 0 || loose > 1)
    {
        qWarning("Warning: Loose value should be in interval [0,1] not %f.\n", loose);
        loose = loose > 1 ? 1 : 0;
        printf("Setting loose to %f.\n", loose);
    }

    if(depth < 0 || depth > 1)
    {
        qWarning("Warning: Depth value should be in interval [0,1] not %f.\n", depth);
        depth = depth > 1 ? 1 : 0;
        printf("Setting depth to %f.\n", depth);
    }

    //
    // 1. Read the bad channels
    // 2. Read the necessary data from the forward solution matrix file
    // 3. Load the projection data
    // 4. Load the sensor noise covariance matrix and attach it to the forward
    //
    FiffInfo gain_info;
    MatrixXd gain;
    MatrixXd whitener;
    qint32 n_nzero;
    FiffCov p_outNoiseCov;
    forward.prepare_forward(info, p_noise_cov, false, gain_info, gain, p_outNoiseCov, whitener, n_nzero);

    //
    // 5. Compose the depth weight matrix
    //
    FiffCov::SDPtr p_depth_prior;
    MatrixXd patch_areas;
    if(depth > 0)
    {
        // TODO: load patch_areas from forward solution
        p_depth_prior = FiffCov::SDPtr(new FiffCov(MNEForwardSolution::compute_depth_prior(gain, gain_info, is_fixed_ori, depth, 10.0, patch_areas, limit_depth_chs)));
    }
    else
    {
        p_depth_prior->data = MatrixXd::Ones(gain.cols(), gain.cols());
        p_depth_prior->kind = FIFFV_MNE_DEPTH_PRIOR_COV;
        p_depth_prior->diag = true;
        p_depth_prior->dim = gain.cols();
        p_depth_prior->nfree = 1;
    }

    // Deal with fixed orientation forward / inverse
    if(fixed)
    {
        if(depth < 0 || depth > 1)
        {
            // TODO: convert free-orientation depth prior to fixed-orientation
            printf("\tPicking elements from free-orientation depth prior into fixed-orientation one.\n");
        }
        if(!is_fixed_ori)
        {
            if(!forward.surf_ori)
            {
                qWarning("Warning: For a fixed-orientation inverse, the forward solution must be surface-oriented. Skipping fixed conversion.\n");
            }
            else
            {
                // Convert to the fixed orientation forward solution now
                qint32 count = 0;
                for(qint32 i = 2; i < p_depth_prior->data.rows(); i+=3)
                {
                    p_depth_prior->data.row(count) = p_depth_prior->data.row(i);
                    ++count;
                }
                p_depth_prior->data.conservativeResize(count, 1);

                forward.to_fixed_ori();
                is_fixed_ori = forward.isFixedOrient();
                forward.prepare_forward(info, p_outNoiseCov, false, gain_info, gain, p_outNoiseCov, whitener, n_nzero);
            }
        }
    }
    printf("\tComputing inverse operator with %lld channels.\n", gain_info.ch_names.size());

    //
    // 6. Compose the source covariance matrix
    //
    printf("\tCreating the source covariance matrix\n");
    FiffCov::SDPtr p_source_cov = p_depth_prior;

    // apply loose orientations
    FiffCov::SDPtr p_orient_prior;
    if(!is_fixed_ori)
    {
        p_orient_prior = FiffCov::SDPtr(new FiffCov(forward.compute_orient_prior(loose)));
        p_source_cov->data.array() *= p_orient_prior->data.array();
    }

    // 7. Apply fMRI weighting (not done)

    //
    // 8. Apply the linear projection to the forward solution
    // 9. Apply whitening to the forward computation matrix
    //
    printf("\tWhitening the forward solution.\n");
    gain = whitener*gain;

    // 10. Exclude the source space points within the labels (not done)

    //
    // 11. Do appropriate source weighting to the forward computation matrix
    //

    // Adjusting Source Covariance matrix to make trace of G*R*G' equal
    // to number of sensors.
    printf("\tAdjusting source covariance matrix.\n");
    RowVectorXd source_std = p_source_cov->data.array().sqrt().transpose();

    for(qint32 i = 0; i < gain.rows(); ++i)
        gain.row(i) = gain.row(i).array() * source_std.array();

    double trace_GRGT = (gain * gain.transpose()).trace();
    double scaling_source_cov = static_cast<double>(n_nzero) / trace_GRGT;

    p_source_cov->data.array() *= scaling_source_cov;

    gain.array() *= sqrt(scaling_source_cov);

    //
    // 12. Decompose the combined matrix
    //
    printf("Computing SVD of whitened and weighted lead field matrix.\n");
    JacobiSVD<MatrixXd> svd(gain, ComputeThinU | ComputeThinV);
    // TODO: verify whether explicit sorting is necessary
    VectorXd p_sing = svd.singularValues();
    MatrixXd t_U = svd.matrixU();
    Linalg::sort<double>(p_sing, t_U);
    FiffNamedMatrix::SDPtr p_eigen_fields = FiffNamedMatrix::SDPtr(new FiffNamedMatrix( svd.matrixU().cols(),
                                                                                        svd.matrixU().rows(),
                                                                                        defaultQStringList,
                                                                                        gain_info.ch_names,
                                                                                        t_U.transpose() ));

    p_sing = svd.singularValues();
    MatrixXd t_V = svd.matrixV();
    Linalg::sort<double>(p_sing, t_V);
    FiffNamedMatrix::SDPtr p_eigen_leads = FiffNamedMatrix::SDPtr(new FiffNamedMatrix( svd.matrixV().rows(),
                                                                                       svd.matrixV().cols(),
                                                                                       defaultQStringList,
                                                                                       defaultQStringList,
                                                                                       t_V ));
    printf("\tlargest singular value = %f\n", p_sing.maxCoeff());
    printf("\tscaling factor to adjust the trace = %f\n", trace_GRGT);

    qint32 p_nave = 1.0;

    // Handle methods
    bool has_meg = false;
    bool has_eeg = false;

    RowVectorXd ch_idx(info.chs.size());
    qint32 count = 0;
    for(qint32 i = 0; i < info.chs.size(); ++i)
    {
        if(gain_info.ch_names.contains(info.chs[i].ch_name))
        {
            ch_idx[count] = i;
            ++count;
        }
    }
    ch_idx.conservativeResize(count);

    for(qint32 i = 0; i < ch_idx.size(); ++i)
    {
        QString ch_type = info.channel_type(ch_idx[i]);
        if (ch_type == "eeg")
            has_eeg = true;
        if ((ch_type == "mag") || (ch_type == "grad"))
            has_meg = true;
    }

    qint32 p_iMethods;

    if(has_eeg && has_meg)
        p_iMethods = FIFFV_MNE_MEG_EEG;
    else if(has_meg)
        p_iMethods = FIFFV_MNE_MEG;
    else
        p_iMethods = FIFFV_MNE_EEG;

    // We set this for consistency with mne C code written inverses
    if(depth == 0)
        p_depth_prior = FiffCov::SDPtr();

    inv.eigen_fields = p_eigen_fields;
    inv.eigen_leads = p_eigen_leads;
    inv.sing = p_sing;
    inv.nchan = p_sing.rows();
    inv.nave = p_nave;
    inv.depth_prior = p_depth_prior;
    // Fix the kind so I/O round-trips write FIFFV_MNE_SOURCE_COV.
    p_source_cov->kind = FIFFV_MNE_SOURCE_COV;
    inv.source_cov = p_source_cov;
    inv.noise_cov = FiffCov::SDPtr(new FiffCov(p_outNoiseCov));
    inv.orient_prior = p_orient_prior;
    inv.projs = info.projs;
    inv.eigen_leads_weighted = false;
    inv.source_ori = forward.source_ori;
    inv.mri_head_t = forward.mri_head_t;
    inv.methods = p_iMethods;
    inv.nsource = forward.nsource;
    inv.coord_frame = forward.coord_frame;
    inv.source_nn = forward.source_nn;
    inv.src = forward.src;
    inv.info = forward.info;
    inv.info.bads = info.bads;

    return inv;
}

//=============================================================================================================

MNEInverseOperator MNEInverseOperator::prepare_inverse_operator(qint32 nave ,float lambda2, bool dSPM, bool sLORETA) const
{
    if(nave <= 0)
    {
        printf("The number of averages should be positive\n");
        return MNEInverseOperator();
    }
    printf("Preparing the inverse operator for use...\n");
    MNEInverseOperator inv(*this);
    //
    //   Scale some of the stuff
    //
    float scale     = static_cast<float>(inv.nave)/static_cast<float>(nave);
    inv.noise_cov->data  *= scale;
    inv.noise_cov->eig   *= scale;
    inv.source_cov->data *= scale;
    //
    if (inv.eigen_leads_weighted)
        inv.eigen_leads->data *= sqrt(scale);
    //
    printf("\tScaled noise and source covariance from nave = %d to nave = %d\n",inv.nave,nave);
    inv.nave = nave;
    //
    //   Create the diagonal matrix for computing the regularized inverse
    //
    VectorXd tmp = inv.sing.cwiseProduct(inv.sing) + VectorXd::Constant(inv.sing.size(), lambda2);
    inv.reginv = VectorXd(inv.sing.cwiseQuotient(tmp));
    printf("\tCreated the regularized inverter\n");
    //
    //   Create the projection operator
    //

    qint32 ncomp = FiffProj::make_projector(inv.projs, inv.noise_cov->names, inv.proj);
    if (ncomp > 0)
        printf("\tCreated an SSP operator (subspace dimension = %d)\n",ncomp);

    //
    //   Create the whitener
    //
    inv.whitener = MatrixXd::Zero(inv.noise_cov->dim, inv.noise_cov->dim);

    qint32 nnzero, k;
    if (inv.noise_cov->diag == 0)
    {
        //
        //   Omit the zeroes due to projection
        //
        nnzero = 0;

        for (k = ncomp; k < inv.noise_cov->dim; ++k)
        {
            if (inv.noise_cov->eig[k] > 0)
            {
                inv.whitener(k,k) = 1.0/sqrt(inv.noise_cov->eig[k]);
                ++nnzero;
            }
        }
        //
        //   Rows of eigvec are the eigenvectors
        //
        inv.whitener *= inv.noise_cov->eigvec;
        printf("\tCreated the whitener using a full noise covariance matrix (%d small eigenvalues omitted)\n", inv.noise_cov->dim - nnzero);
    }
    else
    {
        //
        //   No need to omit the zeroes due to projection
        //
        for (k = 0; k < inv.noise_cov->dim; ++k)
            inv.whitener(k,k) = 1.0/sqrt(inv.noise_cov->data(k,0));

        printf("\tCreated the whitener using a diagonal noise covariance matrix (%d small eigenvalues discarded)\n",ncomp);
    }
    //
    //   Finally, compute the noise-normalization factors
    //
    if (dSPM || sLORETA)
    {
        VectorXd noise_norm = VectorXd::Zero(inv.eigen_leads->nrow);
        VectorXd noise_weight;
        if (dSPM)
        {
           printf("\tComputing noise-normalization factors (dSPM)...");
           noise_weight = VectorXd(inv.reginv);
        }
        else
        {
           printf("\tComputing noise-normalization factors (sLORETA)...");
           VectorXd tmp = (VectorXd::Constant(inv.sing.size(), 1) + inv.sing.cwiseProduct(inv.sing)/lambda2);
           noise_weight = inv.reginv.cwiseProduct(tmp.cwiseSqrt());
        }
        VectorXd one;
        if (inv.eigen_leads_weighted)
        {
           for (k = 0; k < inv.eigen_leads->nrow; ++k)
           {
              one = inv.eigen_leads->data.block(k,0,1,inv.eigen_leads->data.cols()).cwiseProduct(noise_weight);
              noise_norm[k] = sqrt(one.dot(one));
           }
        }
        else
        {
            double c;
            for (k = 0; k < inv.eigen_leads->nrow; ++k)
            {
                c = sqrt(inv.source_cov->data(k,0));
                one = c*(inv.eigen_leads->data.row(k).transpose()).cwiseProduct(noise_weight);
                noise_norm[k] = sqrt(one.dot(one));
            }
        }

        //
        //   Compute the final result
        //
        VectorXd noise_norm_new;
        if (inv.source_ori == FIFFV_MNE_FREE_ORI)
        {
            // For free orientations the variances at three consecutive entries
            // must be squared and summed, yielding one factor per source location.
            VectorXd t = Linalg::combine_xyz(noise_norm.transpose());
            noise_norm_new = t.cwiseSqrt();
        }
        VectorXd vOnes = VectorXd::Ones(noise_norm_new.size());
        VectorXd tmp = vOnes.cwiseQuotient(noise_norm_new.cwiseAbs());

        typedef Eigen::Triplet<double> T;
        std::vector<T> tripletList;
        tripletList.reserve(noise_norm_new.size());
        for(qint32 i = 0; i < noise_norm_new.size(); ++i)
            tripletList.push_back(T(i, i, tmp[i]));

        inv.noisenorm = SparseMatrix<double>(noise_norm_new.size(),noise_norm_new.size());
        inv.noisenorm.setFromTriplets(tripletList.begin(), tripletList.end());

        printf("[done]\n");
    }
    else
    {
        inv.noisenorm = SparseMatrix<double>();
    }

    return inv;
}

//=============================================================================================================

bool MNEInverseOperator::read_inverse_operator(QIODevice& p_IODevice, MNEInverseOperator& inv)
{
    //
    //   Open the file, create directory
    //
    FiffStream::SPtr t_pStream(new FiffStream(&p_IODevice));
    printf("Reading inverse operator decomposition from %s...\n",t_pStream->streamName().toUtf8().constData());

    if(!t_pStream->open())
        return false;
    //
    //   Find all inverse operators
    //
    QList <FiffDirNode::SPtr> invs_list = t_pStream->dirtree()->dir_tree_find(FIFFB_MNE_INVERSE_SOLUTION);
    if ( invs_list.size()== 0)
    {
        printf("No inverse solutions in %s\n", t_pStream->streamName().toUtf8().constData());
        return false;
    }
    FiffDirNode::SPtr invs = invs_list[0];
    //
    //   Parent MRI data
    //
    QList <FiffDirNode::SPtr> parent_mri = t_pStream->dirtree()->dir_tree_find(FIFFB_MNE_PARENT_MRI_FILE);
    if (parent_mri.size() == 0)
    {
        printf("No parent MRI information in %s", t_pStream->streamName().toUtf8().constData());
        return false;
    }
    printf("\tReading inverse operator info...");
    //
    //   Methods and source orientations
    //
    FiffTag::UPtr t_pTag;
    if (!invs->find_tag(t_pStream, FIFF_MNE_INCLUDED_METHODS, t_pTag))
    {
        printf("Modalities not found\n");
        return false;
    }

    inv = MNEInverseOperator();
    inv.methods = *t_pTag->toInt();
    //
    if (!invs->find_tag(t_pStream, FIFF_MNE_SOURCE_ORIENTATION, t_pTag))
    {
        printf("Source orientation constraints not found\n");
        return false;
    }
    inv.source_ori = *t_pTag->toInt();
    //
    if (!invs->find_tag(t_pStream, FIFF_MNE_SOURCE_SPACE_NPOINTS, t_pTag))
    {
        printf("Number of sources not found\n");
        return false;
    }
    inv.nsource = *t_pTag->toInt();
    inv.nchan   = 0;
    //
    //   Coordinate frame
    //
    if (!invs->find_tag(t_pStream, FIFF_MNE_COORD_FRAME, t_pTag))
    {
        printf("Coordinate frame tag not found\n");
        return false;
    }
    inv.coord_frame = *t_pTag->toInt();
    //
    //   The actual source orientation vectors
    //
    if (!invs->find_tag(t_pStream, FIFF_MNE_INVERSE_SOURCE_ORIENTATIONS, t_pTag))
    {
        printf("Source orientation information not found\n");
        return false;
    }

    inv.source_nn = t_pTag->toFloatMatrix();
    inv.source_nn.transposeInPlace();

    printf("[done]\n");
    //
    //   The SVD decomposition...
    //
    printf("\tReading inverse operator decomposition...");
    if (!invs->find_tag(t_pStream, FIFF_MNE_INVERSE_SING, t_pTag))
    {
        printf("Singular values not found\n");
        return false;
    }

    inv.sing = Map<VectorXf>(t_pTag->toFloat(), t_pTag->size()/4).cast<double>();
    inv.nchan = inv.sing.rows();
    //
    //   The eigenleads and eigenfields
    //
    inv.eigen_leads_weighted = false;
    if(!t_pStream->read_named_matrix(invs, FIFF_MNE_INVERSE_LEADS, *inv.eigen_leads.data()))
    {
        inv.eigen_leads_weighted = true;
        if(!t_pStream->read_named_matrix(invs, FIFF_MNE_INVERSE_LEADS_WEIGHTED, *inv.eigen_leads.data()))
        {
            printf("Error reading eigenleads named matrix.\n");
            return false;
        }
    }
    //
    //   Having the eigenleads as columns is better for the inverse calculations
    //
    inv.eigen_leads->transpose_named_matrix();

    if(!t_pStream->read_named_matrix(invs, FIFF_MNE_INVERSE_FIELDS, *inv.eigen_fields.data()))
    {
        printf("Error reading eigenfields named matrix.\n");
        return false;
    }
    printf("[done]\n");
    //
    //   Read the covariance matrices
    //
    if(t_pStream->read_cov(invs, FIFFV_MNE_NOISE_COV, *inv.noise_cov.data()))
    {
        printf("\tNoise covariance matrix read.\n");
    }
    else
    {
        printf("\tError: Not able to read noise covariance matrix.\n");
        return false;
    }

    if(t_pStream->read_cov(invs, FIFFV_MNE_SOURCE_COV, *inv.source_cov.data()))
    {
        printf("\tSource covariance matrix read.\n");
    }
    else
    {
        printf("\tError: Not able to read source covariance matrix.\n");
        return false;
    }
    //
    //   Read the various priors
    //
    if(t_pStream->read_cov(invs, FIFFV_MNE_ORIENT_PRIOR_COV, *inv.orient_prior.data()))
    {
        printf("\tOrientation priors read.\n");
    }
    else
        inv.orient_prior->clear();

    if(t_pStream->read_cov(invs, FIFFV_MNE_DEPTH_PRIOR_COV, *inv.depth_prior.data()))
    {
        printf("\tDepth priors read.\n");
    }
    else
    {
        inv.depth_prior->clear();
    }
    if(t_pStream->read_cov(invs, FIFFV_MNE_FMRI_PRIOR_COV, *inv.fmri_prior.data()))
    {
        printf("\tfMRI priors read.\n");
    }
    else
    {
        inv.fmri_prior->clear();
    }
    //
    //   Read the source spaces
    //
    if(!MNESourceSpaces::readFromStream(t_pStream, false, inv.src))
    {
        printf("\tError: Could not read the source spaces.\n");
        return false;
    }
    for (qint32 k = 0; k < inv.src.size(); ++k)
       inv.src[k].id = inv.src[k].find_source_space_hemi();
    //
    //   Get the MRI <-> head coordinate transformation
    //
    FiffCoordTrans mri_head_t;
    if (!parent_mri[0]->find_tag(t_pStream, FIFF_COORD_TRANS, t_pTag))
    {
        printf("MRI/head coordinate transformation not found\n");
        return false;
    }
    else
    {
        mri_head_t = t_pTag->toCoordTrans();
        if (mri_head_t.from != FIFFV_COORD_MRI || mri_head_t.to != FIFFV_COORD_HEAD)
        {
            mri_head_t.invert_transform();
            if (mri_head_t.from != FIFFV_COORD_MRI || mri_head_t.to != FIFFV_COORD_HEAD)
            {
                printf("MRI/head coordinate transformation not found");
                return false;
            }
        }
    }
    inv.mri_head_t  = mri_head_t;

    //
    // get parent MEG info
    //
    t_pStream->read_meas_info_base(t_pStream->dirtree(), inv.info);

    //
    //   Transform the source spaces to the correct coordinate frame
    //   if necessary
    //
    if (inv.coord_frame != FIFFV_COORD_MRI && inv.coord_frame != FIFFV_COORD_HEAD)
        printf("Only inverse solutions computed in MRI or head coordinates are acceptable");
    //
    //  Number of averages is initially one
    //
    inv.nave = 1;
    //
    //  We also need the SSP operator
    //
    inv.projs     = t_pStream->read_proj(t_pStream->dirtree());

    // proj, whitener, reginv, and noisenorm are filled in by prepare_inverse_operator()

    if(!inv.src.transform_source_space_to(inv.coord_frame, mri_head_t))
    {
        printf("Could not transform source space.\n");
    }
    printf("\tSource spaces transformed to the inverse solution coordinate frame\n");
    //
    //   Done!
    //

    return true;
}

//=============================================================================================================

void MNEInverseOperator::write(QIODevice &p_IODevice)
{
    //
    //   Open the file, create directory
    //

    // Create the file and save the essentials
    FiffStream::SPtr t_pStream = FiffStream::start_file(p_IODevice);
    printf("Write inverse operator decomposition in %s...", t_pStream->streamName().toUtf8().constData());
    this->writeToStream(t_pStream.data());
    t_pStream->end_file();
}

//=============================================================================================================

void MNEInverseOperator::writeToStream(FiffStream* p_pStream)
{
    p_pStream->start_block(FIFFB_MNE_INVERSE_SOLUTION);

    printf("\tWriting inverse operator info...\n");

    p_pStream->write_int(FIFF_MNE_INCLUDED_METHODS, &this->methods);
    p_pStream->write_int(FIFF_MNE_SOURCE_ORIENTATION, &this->source_ori);
    p_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NPOINTS, &this->nsource);
    p_pStream->write_int(FIFF_MNE_COORD_FRAME, &this->coord_frame);
    p_pStream->write_float_matrix(FIFF_MNE_INVERSE_SOURCE_ORIENTATIONS, this->source_nn);
    VectorXf tmp_sing = this->sing.cast<float>();
    p_pStream->write_float(FIFF_MNE_INVERSE_SING, tmp_sing.data(), tmp_sing.size());

    //
    //   The eigenleads and eigenfields
    //
    if(this->eigen_leads_weighted)
    {
        FiffNamedMatrix tmpMatrix(*this->eigen_leads.data());
        tmpMatrix.transpose_named_matrix();
        p_pStream->write_named_matrix(FIFF_MNE_INVERSE_LEADS_WEIGHTED, tmpMatrix);
    }
    else
    {
        FiffNamedMatrix tmpMatrix(*this->eigen_leads.data());
        tmpMatrix.transpose_named_matrix();
        p_pStream->write_named_matrix(FIFF_MNE_INVERSE_LEADS, tmpMatrix);
    }

    p_pStream->write_named_matrix(FIFF_MNE_INVERSE_FIELDS, *this->eigen_fields.data());
    printf("\t[done]\n");
    //
    //   write the covariance matrices
    //
    printf("\tWriting noise covariance matrix.");
    p_pStream->write_cov(*this->noise_cov.data());

    printf("\tWriting source covariance matrix.\n");
    p_pStream->write_cov(*this->source_cov.data());
    //
    //   write the various priors
    //
    printf("\tWriting orientation priors.\n");
    if(this->orient_prior && !this->orient_prior->isEmpty())
        p_pStream->write_cov(*this->orient_prior.data());
    if(this->depth_prior && !this->depth_prior->isEmpty())
        p_pStream->write_cov(*this->depth_prior.data());
    if(this->fmri_prior && !this->fmri_prior->isEmpty())
        p_pStream->write_cov(*this->fmri_prior.data());

    //
    //   Parent MRI data
    //
    p_pStream->start_block(FIFFB_MNE_PARENT_MRI_FILE);
    //   write the MRI <-> head coordinate transformation
    p_pStream->write_coord_trans(this->mri_head_t);
    p_pStream->end_block(FIFFB_MNE_PARENT_MRI_FILE);

    //
    //   Parent MEG measurement info
    //
    p_pStream->write_info_base(this->info);

    //
    //   Write the source spaces
    //
    if(!src.isEmpty())
        this->src.writeToStream(p_pStream);

    //
    //  We also need the SSP operator
    //
    p_pStream->write_proj(this->projs);
    //
    //   Done!
    //
    p_pStream->end_block(FIFFB_MNE_INVERSE_SOLUTION);
}
