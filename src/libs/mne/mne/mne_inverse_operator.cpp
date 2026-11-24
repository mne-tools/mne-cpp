//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     mne_inverse_operator.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Implementation of @ref mne::MNEInverseOperator.
 *
 * Implements FIFF read of the @c FIFFB_MNE_INVERSE_SOLUTION block, the
 * @c prepare_inverse_operator step (whitener + SVD scaling for a given
 * SNR / depth weighting), conversion between fixed and free orientations
 * and the cluster-then-rebuild path used to evaluate clustered inverse
 * solutions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_inverse_operator.h"

#include <mne/fiff/fiff_ch_name.h>
#include <mne/base/log.h>
#include <mne/base/thread_pool.h>
#include <mne/base/string_utils.h>
#include <mne/fs/fs_label.h>
#include <mne/math/linalg.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SVD>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <memory>
#include <vector>
#include <iostream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEInverseOperator::MNEInverseOperator(const std::filesystem::path& path)
: MNEInverseOperator(read(path).value())
{
}

//=============================================================================================================

MNEInverseOperator::MNEInverseOperator(const FiffInfo& info,
                                       const MNEForwardSolution& forward,
                                       const FiffCov& p_noise_cov,
                                       float loose,
                                       float depth,
                                       bool fixed,
                                       bool limit_depth_chs)
: MNEInverseOperator(make_inverse_operator(info, forward, p_noise_cov, loose, depth, fixed, limit_depth_chs))
{
}

//=============================================================================================================

bool MNEInverseOperator::assemble_kernel(const FsLabel& label,
                                         const std::string& method,
                                         bool pick_normal,
                                         MatrixXd& K,
                                         SparseMatrix<double>& noise_norm,
                                         std::vector<VectorXi>& vertno)
{
    MatrixXd t_eigen_leads = eigen_leads.data;
    MatrixXd t_source_cov = source_cov.data;
    if (method != "MNE")
        noise_norm = noisenorm;

    vertno = src.get_vertno();

    typedef Eigen::Triplet<double> T;
    std::vector<T> tripletList;

    if (!label.empty()) {
        VectorXi src_sel;
        vertno = src.label_src_vertno_sel(label, src_sel);

        if (method != "MNE" && noise_norm.rows() > 0) {
            // noisenorm is diagonal, one entry per source.
            tripletList.clear();
            tripletList.reserve(src_sel.size());
            for (std::int32_t i = 0; i < src_sel.size(); ++i)
                tripletList.push_back(T(i, i, noise_norm.coeff(src_sel[i], src_sel[i])));

            noise_norm = SparseMatrix<double>(src_sel.size(), src_sel.size());
            noise_norm.setFromTriplets(tripletList.begin(), tripletList.end());
        }

        if (source_ori == FIFFV_MNE_FREE_ORI) {
            VectorXi src_sel_new(src_sel.size() * 3);

            for (std::int32_t i = 0; i < src_sel.size(); ++i) {
                src_sel_new[i * 3] = src_sel[i] * 3;
                src_sel_new[i * 3 + 1] = src_sel[i] * 3 + 1;
                src_sel_new[i * 3 + 2] = src_sel[i] * 3 + 2;
            }
            src_sel = src_sel_new;
        }

        // src_sel is ascending with src_sel[i] >= i, so in-place compaction is safe.
        for (std::int32_t i = 0; i < src_sel.size(); ++i) {
            t_eigen_leads.row(i) = t_eigen_leads.row(src_sel[i]);
            t_source_cov.row(i) = t_source_cov.row(src_sel[i]);
        }
        t_eigen_leads.conservativeResize(src_sel.size(), t_eigen_leads.cols());
        t_source_cov.conservativeResize(src_sel.size(), t_source_cov.cols());
    }

    if (pick_normal) {
        if (source_ori != FIFFV_MNE_FREE_ORI) {
            Log::warning("Warning: Pick normal can only be used with a free orientation inverse operator.\n");
            return false;
        }

        bool is_loose = (0 < orient_prior.data(0, 0)) && (orient_prior.data(0, 0) < 1);
        if (!is_loose) {
            Log::warning("The pick_normal parameter is only valid when working with loose orientations.\n");
            return false;
        }

        // keep only the normal components
        std::int32_t count = 0;
        for (std::int32_t i = 2; i < t_eigen_leads.rows(); i += 3) {
            t_eigen_leads.row(count) = t_eigen_leads.row(i);
            ++count;
        }
        t_eigen_leads.conservativeResize(count, t_eigen_leads.cols());

        count = 0;
        for (std::int32_t i = 2; i < t_source_cov.rows(); i += 3) {
            t_source_cov.row(count) = t_source_cov.row(i);
            ++count;
        }
        t_source_cov.conservativeResize(count, t_source_cov.cols());
    }

    tripletList.clear();
    tripletList.reserve(reginv.rows());
    for (std::int32_t i = 0; i < reginv.rows(); ++i)
        tripletList.push_back(T(i, i, reginv(i, 0)));
    SparseMatrix<double> t_reginv(reginv.rows(), reginv.rows());
    t_reginv.setFromTriplets(tripletList.begin(), tripletList.end());

    MatrixXd trans = t_reginv * eigen_fields.data * whitener * proj;
    //
    //   Transformation into current distributions by weighting the eigenleads
    //   with the weights computed above
    //
    if (eigen_leads_weighted) {
        //
        //     R^0.5 has been already factored in
        //
        Log::info("(eigenleads already weighted)...\n");
        K = t_eigen_leads * trans;
    } else {
        //
        //     R^0.5 has to factored in
        //
        Log::info("(eigenleads need to be weighted)...\n");

        std::vector<T> tripletList2;
        tripletList2.reserve(t_source_cov.rows());
        for (std::int32_t i = 0; i < t_source_cov.rows(); ++i)
            tripletList2.push_back(T(i, i, sqrt(t_source_cov(i, 0))));
        SparseMatrix<double> t_sourceCov(t_source_cov.rows(), t_source_cov.rows());
        t_sourceCov.setFromTriplets(tripletList2.begin(), tripletList2.end());

        K = t_sourceCov * t_eigen_leads * trans;
    }

    if (method == "MNE")
        noise_norm = SparseMatrix<double>();

    //store assembled kernel
    m_K = K;

    return true;
}

//=============================================================================================================

bool MNEInverseOperator::check_ch_names(const FiffInfo& measInfo) const
{
    StringList inv_ch_names = this->eigen_fields.col_names;

    bool t_bContains = true;
    if (this->eigen_fields.col_names.size() != this->noise_cov.names.size())
        t_bContains = false;
    else {
        for (std::int32_t i = 0; i < static_cast<int>(this->noise_cov.names.size()); ++i) {
            if (inv_ch_names[i] != this->noise_cov.names[i]) {
                t_bContains = false;
                break;
            }
        }
    }

    if (!t_bContains) {
        Log::critical("Channels in inverse operator eigen fields do not match noise covariance channels.");
        return false;
    }

    StringList data_ch_names = measInfo.ch_names;

    StringList missing_ch_names;
    for (std::int32_t i = 0; i < static_cast<int>(inv_ch_names.size()); ++i)
        if (!mne::has_channel(data_ch_names, inv_ch_names[i]))
            missing_ch_names.push_back(inv_ch_names[i]);

    std::int32_t n_missing = static_cast<int>(missing_ch_names.size());

    if (n_missing > 0) {
        Log::critical("{} channels in inverse operator are not present in the data ( {} )", n_missing, join(missing_ch_names, " "));
        return false;
    }

    return true;
}

//=============================================================================================================

MatrixXd MNEInverseOperator::cluster_kernel(const FsAnnotationSet& p_AnnotationSet, std::int32_t p_iClusterSize, MatrixXd& p_D, const std::string& p_sMethod) const
{
    Log::info("Cluster kernel using {}.\n", p_sMethod);

    MatrixXd p_outMT = m_K.transpose();

    std::vector<MNEClusterInfo> t_qListMNEClusterInfo;
    MNEClusterInfo t_MNEClusterInfo;
    t_qListMNEClusterInfo.push_back(t_MNEClusterInfo);
    t_qListMNEClusterInfo.push_back(t_MNEClusterInfo);

    //
    // Check consistency
    //
    if (isFixedOrient()) {
        Log::critical("Error: Fixed orientation not implemented yet!\n");
        return p_outMT;
    }

    //
    // Assemble input data
    //
    std::int32_t offset;

    MatrixXd t_MT_new;

    for (std::int32_t h = 0; h < this->src.size(); ++h) {
        offset = 0;

        if (h > 0)
            for (std::int32_t j = 0; j < h; ++j)
                offset += this->src[j].nuse;

        if (h == 0)
            Log::info("Cluster Left Hemisphere\n");
        else
            Log::info("Cluster Right Hemisphere\n");

        FsColortable t_CurrentColorTable = p_AnnotationSet[h].getColortable();
        VectorXi label_ids = t_CurrentColorTable.getLabelIds();

        // Get label ids for every vertex
        VectorXi vertno_labeled = VectorXi::Zero(this->src[h].vertno.rows());

        for (std::int32_t i = 0; i < vertno_labeled.rows(); ++i)
            vertno_labeled[i] = p_AnnotationSet[h].getLabelIds()[this->src[h].vertno[i]];

        // Work items of the parallel clustering
        std::vector<RegionMT> m_qListRegionMTIn;

        //
        // Generate cluster input data
        //
        for (std::int32_t i = 0; i < label_ids.rows(); ++i) {
            if (label_ids[i] != 0) {
                const std::string& curr_name = t_CurrentColorTable.struct_names[static_cast<std::size_t>(i)]; //obj.label2AtlasName(label(i));
                Log::info("\tCluster {} / {} {}...", i + 1, label_ids.rows(), curr_name);

                //
                // Get source space indeces
                //
                VectorXi idcs = VectorXi::Zero(vertno_labeled.rows());
                std::int32_t c = 0;

                //Select ROIs //change this use label info with a hash tabel
                for (std::int32_t j = 0; j < vertno_labeled.rows(); ++j) {
                    if (vertno_labeled[j] == label_ids[i]) {
                        idcs[c] = j;
                        ++c;
                    }
                }
                idcs.conservativeResize(c);

                //get selected MT
                MatrixXd t_MT(p_outMT.rows(), idcs.rows() * 3);

                for (std::int32_t j = 0; j < idcs.rows(); ++j)
                    t_MT.block(0, j * 3, t_MT.rows(), 3) = p_outMT.block(0, (idcs[j] + offset) * 3, t_MT.rows(), 3);

                std::int32_t nSens = t_MT.rows();
                std::int32_t nSources = t_MT.cols() / 3;

                if (nSources > 0) {
                    RegionMT t_sensMT;

                    t_sensMT.idcs = idcs;
                    t_sensMT.iLabelIdxIn = i;
                    t_sensMT.nClusters = ceil(static_cast<double>(nSources) / static_cast<double>(p_iClusterSize));

                    t_sensMT.matRoiMTOrig = t_MT;

                    Log::info("{} Cluster(s)... ", t_sensMT.nClusters);

                    // Reshape Input data -> sources rows; sensors columns
                    t_sensMT.matRoiMT = MatrixXd(t_MT.cols() / 3, 3 * nSens);

                    for (std::int32_t j = 0; j < nSens; ++j)
                        for (std::int32_t k = 0; k < t_sensMT.matRoiMT.rows(); ++k)
                            t_sensMT.matRoiMT.block(k, j * 3, 1, 3) = t_MT.block(j, k * 3, 1, 3);

                    t_sensMT.sDistMeasure = p_sMethod;

                    m_qListRegionMTIn.push_back(t_sensMT);

                    Log::info("[added]\n");
                } else {
                    Log::info("failed! FsLabel contains no sources.\n");
                }
            }
        }

        //
        // Calculate clusters
        //
        Log::info("Clustering... ");
        const std::vector<RegionMTOut> res = mne::ThreadPool::global().parallel_map(m_qListRegionMTIn, [](const RegionMT& region) { return region.cluster(); });

        //
        // Assign results
        //
        MatrixXd t_MT_partial;

        std::int32_t nClusters;
        std::int32_t nSens;
        std::vector<RegionMT>::const_iterator itIn;
        itIn = m_qListRegionMTIn.begin();
        for (auto itOut = res.cbegin(); itOut != res.cend(); ++itOut) {
            nClusters = itOut->ctrs.rows();
            nSens = itOut->ctrs.cols() / 3;
            t_MT_partial = MatrixXd::Zero(nSens, nClusters * 3);

            //
            // Assign the centroid for each cluster to the partial G
            //
            for (std::int32_t j = 0; j < nSens; ++j)
                for (std::int32_t k = 0; k < nClusters; ++k)
                    t_MT_partial.block(j, k * 3, 1, 3) = itOut->ctrs.block(k, j * 3, 1, 3);

            //
            // Get cluster indices and their distances to the centroid
            //
            for (std::int32_t j = 0; j < nClusters; ++j) {
                VectorXi clusterIdcs = VectorXi::Zero(itOut->roiIdx.rows());
                VectorXd clusterDistance = VectorXd::Zero(itOut->roiIdx.rows());
                std::int32_t nClusterIdcs = 0;
                for (std::int32_t k = 0; k < itOut->roiIdx.rows(); ++k) {
                    if (itOut->roiIdx[k] == j) {
                        clusterIdcs[nClusterIdcs] = itIn->idcs[k];
                        clusterDistance[nClusterIdcs] = itOut->D(k, j);
                        ++nClusterIdcs;
                    }
                }
                clusterIdcs.conservativeResize(nClusterIdcs);
                clusterDistance.conservativeResize(nClusterIdcs);

                VectorXi clusterVertnos = VectorXi::Zero(clusterIdcs.size());
                for (std::int32_t k = 0; k < clusterVertnos.size(); ++k)
                    clusterVertnos(k) = this->src[h].vertno[clusterIdcs(k)];

                t_qListMNEClusterInfo[h].clusterVertnos.push_back(clusterVertnos);
            }

            //
            // Assign partial G to new LeadField
            //
            if (t_MT_partial.rows() > 0 && t_MT_partial.cols() > 0) {
                t_MT_new.conservativeResize(t_MT_partial.rows(), t_MT_new.cols() + t_MT_partial.cols());
                t_MT_new.block(0, t_MT_new.cols() - t_MT_partial.cols(), t_MT_new.rows(), t_MT_partial.cols()) = t_MT_partial;
            }

            ++itIn;
        }

        Log::info("[done]\n");
    }

    //
    // Cluster operator D (sources x clusters)
    //
    std::int32_t totalNumOfClust = 0;
    for (std::int32_t h = 0; h < 2; ++h)
        totalNumOfClust += static_cast<std::int32_t>(t_qListMNEClusterInfo[h].clusterVertnos.size());

    if (isFixedOrient())
        p_D = MatrixXd::Zero(p_outMT.cols(), totalNumOfClust);
    else
        p_D = MatrixXd::Zero(p_outMT.cols(), totalNumOfClust * 3);

    std::vector<VectorXi> t_vertnos = src.get_vertno();

    std::int32_t currentCluster = 0;
    for (std::int32_t h = 0; h < 2; ++h) {
        int hemiOffset = h == 0 ? 0 : t_vertnos[0].size();
        for (std::int32_t i = 0; i < std::ssize(t_qListMNEClusterInfo[h].clusterVertnos); ++i) {
            VectorXi idx_sel;
            Linalg::intersect(t_vertnos[h], t_qListMNEClusterInfo[h].clusterVertnos[i], idx_sel);

            idx_sel.array() += hemiOffset;

            double selectWeight = 1.0 / idx_sel.size();
            if (isFixedOrient()) {
                for (std::int32_t j = 0; j < idx_sel.size(); ++j)
                    p_D.col(currentCluster)[idx_sel(j)] = selectWeight;
            } else {
                std::int32_t clustOffset = currentCluster * 3;
                for (std::int32_t j = 0; j < idx_sel.size(); ++j) {
                    std::int32_t idx_sel_Offset = idx_sel(j) * 3;
                    //x
                    p_D(idx_sel_Offset, clustOffset) = selectWeight;
                    //y
                    p_D(idx_sel_Offset + 1, clustOffset + 1) = selectWeight;
                    //z
                    p_D(idx_sel_Offset + 2, clustOffset + 2) = selectWeight;
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

MNEInverseOperator MNEInverseOperator::make_inverse_operator(const FiffInfo& info,
                                                             MNEForwardSolution forward,
                                                             const FiffCov& p_noise_cov,
                                                             float loose,
                                                             float depth,
                                                             bool fixed,
                                                             bool limit_depth_chs)
{
    bool is_fixed_ori = forward.isFixedOrient();
    MNEInverseOperator inv;

    // Loose and fixed constraints are relative to the cortical normal, as in mne-python's _prepare_forward.
    if (!is_fixed_ori && (fixed || loose < 1.0f)) {
        forward.convert_to_surf_ori();
    }

    //Check parameters
    if (fixed && loose > 0) {
        Log::warning("Warning: When invoking make_inverse_operator with fixed = true, the loose parameter is ignored.\n");
        loose = 0.0f;
    }

    if (is_fixed_ori && !fixed) {
        Log::warning("Warning: Setting fixed parameter = true. Because the given forward operator has fixed orientation and can only be used to make a fixed-orientation inverse operator.\n");
        fixed = true;
    }

    if (forward.source_ori == -1 && loose > 0) {
        Log::critical("Forward solution is not oriented in surface coordinates. loose parameter should be 0 not {:f}.", loose);
        return inv;
    }

    if (loose < 0 || loose > 1) {
        Log::warning("Warning: Loose value should be in interval [0,1] not {:f}.\n", loose);
        loose = loose > 1 ? 1 : 0;
        Log::info("Setting loose to {:f}.\n", loose);
    }

    if (depth < 0 || depth > 1) {
        Log::warning("Warning: Depth value should be in interval [0,1] not {:f}.\n", depth);
        depth = depth > 1 ? 1 : 0;
        Log::info("Setting depth to {:f}.\n", depth);
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
    std::int32_t n_nzero;
    FiffCov p_outNoiseCov;
    forward.prepare_forward(info, p_noise_cov, false, gain_info, gain, p_outNoiseCov, whitener, n_nzero);

    //
    // 5. Compose the depth weight matrix
    //
    FiffCov p_depth_prior;
    MatrixXd patch_areas;
    if (depth > 0) {
        // TODO: load patch_areas from forward solution
        p_depth_prior = MNEForwardSolution::compute_depth_prior(gain, gain_info, is_fixed_ori, depth, 10.0, patch_areas, limit_depth_chs);
    } else {
        p_depth_prior.data = MatrixXd::Ones(gain.cols(), 1);
        p_depth_prior.kind = FIFFV_MNE_DEPTH_PRIOR_COV;
        p_depth_prior.diag = true;
        p_depth_prior.dim = gain.cols();
        p_depth_prior.nfree = 1;
    }

    // Deal with fixed orientation forward / inverse
    if (fixed) {
        if (depth < 0 || depth > 1) {
            // TODO: convert free-orientation depth prior to fixed-orientation
            Log::info("\tPicking elements from free-orientation depth prior into fixed-orientation one.\n");
        }
        if (!is_fixed_ori) {
            if (!forward.surf_ori) {
                Log::warning("Warning: For a fixed-orientation inverse, the forward solution must be surface-oriented. Skipping fixed conversion.\n");
            } else {
                // Convert to the fixed orientation forward solution now
                std::int32_t count = 0;
                for (std::int32_t i = 2; i < p_depth_prior.data.rows(); i += 3) {
                    p_depth_prior.data.row(count) = p_depth_prior.data.row(i);
                    gain.col(count) = gain.col(i);
                    ++count;
                }
                p_depth_prior.data.conservativeResize(count, 1);
                p_depth_prior.dim = count;
                gain.conservativeResize(Eigen::NoChange, count);

                forward.to_fixed_ori();
                is_fixed_ori = forward.isFixedOrient();
            }
        }
    }
    Log::info("\tComputing inverse operator with {} channels.\n", gain_info.ch_names.size());

    //
    // 6. Compose the source covariance matrix
    //
    Log::info("\tCreating the source covariance matrix\n");
    FiffCov p_source_cov = p_depth_prior;

    // apply loose orientations
    FiffCov p_orient_prior;
    if (!is_fixed_ori) {
        p_orient_prior = forward.compute_orient_prior(loose);
        p_source_cov.data.array() *= p_orient_prior.data.array();
    }

    // 7. Apply fMRI weighting (not done)

    //
    // 8. Apply the linear projection to the forward solution
    // 9. Apply whitening to the forward computation matrix
    //
    Log::info("\tWhitening the forward solution.\n");
    gain = Linalg::parallel_product(whitener, gain);

    // 10. Exclude the source space points within the labels (not done)

    //
    // 11. Do appropriate source weighting to the forward computation matrix
    //

    // Adjusting Source Covariance matrix to make trace of G*R*G' equal
    // to number of sensors.
    Log::info("\tAdjusting source covariance matrix.\n");
    RowVectorXd source_std = p_source_cov.data.array().sqrt().transpose();

    gain.array().rowwise() *= source_std.array();

    double trace_GRGT = gain.squaredNorm();
    double scaling_source_cov = static_cast<double>(n_nzero) / trace_GRGT;

    p_source_cov.data.array() *= scaling_source_cov;

    gain.array() *= sqrt(scaling_source_cov);

    //
    // 12. Decompose the combined matrix
    //
    Log::info("Computing SVD of whitened and weighted lead field matrix.\n");
    // Parallel tall-skinny QR followed by divide and conquer; singular values come in descending order
    Linalg::ThinSvd svd = Linalg::thin_svd(gain);
    VectorXd p_sing = std::move(svd.singularValues);
    FiffNamedMatrix p_eigen_fields(svd.U.cols(),
                                   svd.U.rows(),
                                   {},
                                   gain_info.ch_names,
                                   svd.U.transpose());

    FiffNamedMatrix p_eigen_leads(svd.V.rows(),
                                  svd.V.cols(),
                                  {},
                                  {},
                                  svd.V);
    Log::info("\tlargest singular value = {:f}\n", p_sing.maxCoeff());
    Log::info("\tscaling factor to adjust the trace = {:f}\n", trace_GRGT);

    std::int32_t p_nave = 1.0;

    // Handle methods
    bool has_meg = false;
    bool has_eeg = false;

    RowVectorXd ch_idx(info.chs.size());
    std::int32_t count = 0;
    for (std::int32_t i = 0; i < std::ssize(info.chs); ++i) {
        if (mne::has_channel(gain_info.ch_names, info.chs[i].ch_name)) {
            ch_idx[count] = i;
            ++count;
        }
    }
    ch_idx.conservativeResize(count);

    for (std::int32_t i = 0; i < ch_idx.size(); ++i) {
        const std::string ch_type = info.channel_type(ch_idx[i]);
        if (ch_type == "eeg")
            has_eeg = true;
        if ((ch_type == "mag") || (ch_type == "grad"))
            has_meg = true;
    }

    std::int32_t p_iMethods;

    if (has_eeg && has_meg)
        p_iMethods = FIFFV_MNE_MEG_EEG;
    else if (has_meg)
        p_iMethods = FIFFV_MNE_MEG;
    else
        p_iMethods = FIFFV_MNE_EEG;

    // We set this for consistency with mne C code written inverses
    if (depth == 0)
        p_depth_prior = FiffCov();

    inv.eigen_fields = std::move(p_eigen_fields);
    inv.eigen_leads = std::move(p_eigen_leads);
    inv.sing = p_sing;
    inv.nchan = p_sing.rows();
    inv.nave = p_nave;
    inv.depth_prior = std::move(p_depth_prior);
    // Fix the kind so I/O round-trips write FIFFV_MNE_SOURCE_COV.
    p_source_cov.kind = FIFFV_MNE_SOURCE_COV;
    inv.source_cov = std::move(p_source_cov);
    inv.noise_cov = std::move(p_outNoiseCov);
    inv.orient_prior = std::move(p_orient_prior);
    inv.projs = info.projs;
    inv.eigen_leads_weighted = false;
    inv.source_ori = forward.source_ori;
    inv.mri_head_t = forward.mri_head_t;
    inv.methods = p_iMethods;
    inv.nsource = forward.nsource;
    inv.coord_frame = forward.coord_frame;
    inv.source_nn = forward.source_nn;
    inv.src = std::move(forward.src);
    inv.info = forward.info;
    inv.info.bads = info.bads;

    return inv;
}

//=============================================================================================================

MNEInverseOperator MNEInverseOperator::prepare_inverse_operator(std::int32_t nAve, float lambda2, bool dSPM, bool sLORETA) const
{
    if (nAve <= 0) {
        Log::critical("The number of averages should be positive\n");
        return MNEInverseOperator();
    }
    Log::info("Preparing the inverse operator for use...\n");
    MNEInverseOperator inv(*this);
    //
    //   Scale some of the stuff
    //
    float scale = static_cast<float>(inv.nave) / static_cast<float>(nAve);
    inv.noise_cov.data *= scale;
    inv.noise_cov.eig *= scale;
    inv.source_cov.data *= scale;
    //
    if (inv.eigen_leads_weighted)
        inv.eigen_leads.data *= sqrt(scale);
    //
    Log::info("\tScaled noise and source covariance from nave = {} to nave = {}\n", inv.nave, nAve);
    inv.nave = nAve;
    //
    //   Create the diagonal matrix for computing the regularized inverse
    //
    VectorXd tmp = inv.sing.cwiseProduct(inv.sing) + VectorXd::Constant(inv.sing.size(), lambda2);
    inv.reginv = VectorXd(inv.sing.cwiseQuotient(tmp));
    Log::info("\tCreated the regularized inverter\n");
    //
    //   Create the projection operator
    //

    std::int32_t ncomp = FiffProj::make_projector(inv.projs, inv.noise_cov.names, inv.proj);
    if (ncomp > 0)
        Log::info("\tCreated an SSP operator (subspace dimension = {})\n", ncomp);

    //
    //   Create the whitener
    //
    inv.whitener = MatrixXd::Zero(inv.noise_cov.dim, inv.noise_cov.dim);

    std::int32_t nnzero, k;
    if (inv.noise_cov.diag == 0) {
        //
        //   Omit the zeroes due to projection. The eigenvalues are sorted per channel type, not
        //   globally, so the zeroes are not necessarily the first ncomp (mne-python compute_whitener).
        //
        nnzero = 0;

        for (k = 0; k < inv.noise_cov.dim; ++k) {
            if (inv.noise_cov.eig[k] > 0) {
                inv.whitener(k, k) = 1.0 / sqrt(inv.noise_cov.eig[k]);
                ++nnzero;
            }
        }
        //
        //   Rows of eigvec are the eigenvectors
        //
        inv.whitener *= inv.noise_cov.eigvec;
        Log::info("\tCreated the whitener using a full noise covariance matrix ({} small eigenvalues omitted)\n", inv.noise_cov.dim - nnzero);
    } else {
        //
        //   No need to omit the zeroes due to projection
        //
        for (k = 0; k < inv.noise_cov.dim; ++k)
            inv.whitener(k, k) = 1.0 / sqrt(inv.noise_cov.data(k, 0));

        Log::info("\tCreated the whitener using a diagonal noise covariance matrix ({} small eigenvalues discarded)\n", ncomp);
    }
    //
    //   Finally, compute the noise-normalization factors
    //
    if (dSPM || sLORETA) {
        VectorXd noise_norm = VectorXd::Zero(inv.eigen_leads.nrow);
        VectorXd noise_weight;
        if (dSPM) {
            Log::info("\tComputing noise-normalization factors (dSPM)...");
            noise_weight = VectorXd(inv.reginv);
        } else {
            Log::info("\tComputing noise-normalization factors (sLORETA)...");
            VectorXd sLoretaScale = (VectorXd::Constant(inv.sing.size(), 1) + inv.sing.cwiseProduct(inv.sing) / lambda2);
            noise_weight = inv.reginv.cwiseProduct(sLoretaScale.cwiseSqrt());
        }
        VectorXd one;
        if (inv.eigen_leads_weighted) {
            for (k = 0; k < inv.eigen_leads.nrow; ++k) {
                one = inv.eigen_leads.data.block(k, 0, 1, inv.eigen_leads.data.cols()).cwiseProduct(noise_weight);
                noise_norm[k] = sqrt(one.dot(one));
            }
        } else {
            double c;
            for (k = 0; k < inv.eigen_leads.nrow; ++k) {
                c = sqrt(inv.source_cov.data(k, 0));
                one = c * (inv.eigen_leads.data.row(k).transpose()).cwiseProduct(noise_weight);
                noise_norm[k] = sqrt(one.dot(one));
            }
        }

        //
        //   Compute the final result
        //
        VectorXd noise_norm_new = noise_norm;
        if (inv.source_ori == FIFFV_MNE_FREE_ORI) {
            // For free orientations the variances at three consecutive entries
            // must be squared and summed, yielding one factor per source location.
            VectorXd t = Linalg::combine_xyz(noise_norm.transpose());
            noise_norm_new = t.cwiseSqrt();
        }
        VectorXd vOnes = VectorXd::Ones(noise_norm_new.size());
        VectorXd noiseNormInv = vOnes.cwiseQuotient(noise_norm_new.cwiseAbs());

        typedef Eigen::Triplet<double> T;
        std::vector<T> tripletList;
        tripletList.reserve(noise_norm_new.size());
        for (std::int32_t i = 0; i < noise_norm_new.size(); ++i)
            tripletList.push_back(T(i, i, noiseNormInv[i]));

        inv.noisenorm = SparseMatrix<double>(noise_norm_new.size(), noise_norm_new.size());
        inv.noisenorm.setFromTriplets(tripletList.begin(), tripletList.end());

        Log::info("[done]\n");
    } else {
        inv.noisenorm = SparseMatrix<double>();
    }

    return inv;
}

//=============================================================================================================

Expected<MNEInverseOperator> MNEInverseOperator::read(const std::filesystem::path& path)
{
    auto opened = FiffStream::open(path);
    if (!opened) {
        return std::unexpected(opened.error());
    }
    FiffStream& t_pStream = *opened;
    const std::string name = utf8_from_path(path);
    Log::info("Reading inverse operator decomposition from {}...", name);
    //
    //   Find all inverse operators
    //
    std::vector<std::shared_ptr<FiffDirNode>> invs_list = t_pStream.dirtree()->dir_tree_find(FIFFB_MNE_INVERSE_SOLUTION);
    if (invs_list.size() == 0) {
        return fail(ErrorCode::NotFound, "no inverse solution in {}", name);
    }
    const std::shared_ptr<FiffDirNode>& invs = invs_list[0];
    //
    //   Parent MRI data
    //
    std::vector<std::shared_ptr<FiffDirNode>> parent_mri = t_pStream.dirtree()->dir_tree_find(FIFFB_MNE_PARENT_MRI_FILE);
    if (parent_mri.size() == 0) {
        return fail(ErrorCode::Format, "no parent MRI information in {}", name);
    }
    Log::info("\tReading inverse operator info...");
    //
    //   Methods and source orientations
    //
    MNEInverseOperator inv;
    std::optional<FiffTag> t_pTag;
    if (t_pTag = invs->find_tag(t_pStream, FIFF_MNE_INCLUDED_METHODS); !t_pTag) {
        return fail(ErrorCode::Format, "no included methods in {}", name);
    }

    inv.methods = *t_pTag->to_int();
    //
    if (t_pTag = invs->find_tag(t_pStream, FIFF_MNE_SOURCE_ORIENTATION); !t_pTag) {
        return fail(ErrorCode::Format, "no source orientation constraints in {}", name);
    }
    inv.source_ori = *t_pTag->to_int();
    //
    if (t_pTag = invs->find_tag(t_pStream, FIFF_MNE_SOURCE_SPACE_NPOINTS); !t_pTag) {
        return fail(ErrorCode::Format, "no source count in {}", name);
    }
    inv.nsource = *t_pTag->to_int();
    inv.nchan = 0;
    //
    //   Coordinate frame
    //
    if (t_pTag = invs->find_tag(t_pStream, FIFF_MNE_COORD_FRAME); !t_pTag) {
        return fail(ErrorCode::Format, "no coordinate frame in {}", name);
    }
    inv.coord_frame = *t_pTag->to_int();
    //
    //   Units of the source estimates. Optional: files written before this was
    //   recorded simply do not carry it, so a missing tag is not an error.
    //
    if (t_pTag = invs->find_tag(t_pStream, FIFF_MNE_INVERSE_SOURCE_UNIT); t_pTag)
        inv.units = *t_pTag->to_int();
    else
        inv.units = -1;
    //
    //   The actual source orientation vectors
    //
    if (t_pTag = invs->find_tag(t_pStream, FIFF_MNE_INVERSE_SOURCE_ORIENTATIONS); !t_pTag) {
        return fail(ErrorCode::Format, "no source orientation information in {}", name);
    }

    inv.source_nn = t_pTag->to_float_matrix();
    inv.source_nn.transposeInPlace();

    Log::info("[done]\n");
    //
    //   The SVD decomposition...
    //
    Log::info("\tReading inverse operator decomposition...");
    if (t_pTag = invs->find_tag(t_pStream, FIFF_MNE_INVERSE_SING); !t_pTag) {
        return fail(ErrorCode::Format, "no singular values in {}", name);
    }

    inv.sing = Map<const VectorXf>(t_pTag->to_float(), t_pTag->size() / 4).cast<double>();
    inv.nchan = inv.sing.rows();
    //
    //   The eigenleads and eigenfields
    //
    inv.eigen_leads_weighted = false;
    if (!t_pStream.read_named_matrix(invs, FIFF_MNE_INVERSE_LEADS, inv.eigen_leads)) {
        inv.eigen_leads_weighted = true;
        if (!t_pStream.read_named_matrix(invs, FIFF_MNE_INVERSE_LEADS_WEIGHTED, inv.eigen_leads)) {
            return fail(ErrorCode::Format, "cannot read the eigenleads of {}", name);
        }
    }
    //
    //   Having the eigenleads as columns is better for the inverse calculations
    //
    inv.eigen_leads.transpose_named_matrix();

    if (!t_pStream.read_named_matrix(invs, FIFF_MNE_INVERSE_FIELDS, inv.eigen_fields)) {
        return fail(ErrorCode::Format, "cannot read the eigenfields of {}", name);
    }
    Log::info("[done]\n");
    //
    //   Read the covariance matrices
    //
    if (t_pStream.read_cov(invs, FIFFV_MNE_NOISE_COV, inv.noise_cov)) {
        Log::info("\tNoise covariance matrix read.\n");
    } else {
        return fail(ErrorCode::Format, "cannot read the noise covariance of {}", name);
    }

    if (t_pStream.read_cov(invs, FIFFV_MNE_SOURCE_COV, inv.source_cov)) {
        Log::info("\tSource covariance matrix read.\n");
    } else {
        return fail(ErrorCode::Format, "cannot read the source covariance of {}", name);
    }
    //
    //   Read the various priors
    //
    if (t_pStream.read_cov(invs, FIFFV_MNE_ORIENT_PRIOR_COV, inv.orient_prior)) {
        Log::info("\tOrientation priors read.\n");
    } else
        inv.orient_prior.clear();

    if (t_pStream.read_cov(invs, FIFFV_MNE_DEPTH_PRIOR_COV, inv.depth_prior)) {
        Log::info("\tDepth priors read.\n");
    } else {
        inv.depth_prior.clear();
    }
    if (t_pStream.read_cov(invs, FIFFV_MNE_FMRI_PRIOR_COV, inv.fmri_prior)) {
        Log::info("\tfMRI priors read.\n");
    } else {
        inv.fmri_prior.clear();
    }
    //
    //   Read the source spaces
    //
    auto sourceSpaces = MNESourceSpaces::read(t_pStream, false);
    if (!sourceSpaces) {
        return std::unexpected(sourceSpaces.error());
    }
    inv.src = std::move(*sourceSpaces);
    for (std::int32_t k = 0; k < inv.src.size(); ++k)
        inv.src[k].id = inv.src[k].find_source_space_hemi();
    //
    //   Get the MRI <-> head coordinate transformation
    //
    FiffCoordTrans mri_head_t;
    if (t_pTag = parent_mri[0]->find_tag(t_pStream, FIFF_COORD_TRANS); !t_pTag) {
        return fail(ErrorCode::Format, "no MRI/head coordinate transformation in {}", name);
    } else {
        mri_head_t = t_pTag->to_coord_trans();
        if (mri_head_t.from != FIFFV_COORD_MRI || mri_head_t.to != FIFFV_COORD_HEAD) {
            mri_head_t.invert_transform();
            if (mri_head_t.from != FIFFV_COORD_MRI || mri_head_t.to != FIFFV_COORD_HEAD) {
                return fail(ErrorCode::Format, "no MRI/head coordinate transformation in {}", name);
            }
        }
    }
    inv.mri_head_t = mri_head_t;

    //
    // get parent MEG info
    //
    t_pStream.read_meas_info_base(t_pStream.dirtree(), inv.info);

    //
    //   Transform the source spaces to the correct coordinate frame
    //   if necessary
    //
    if (inv.coord_frame != FIFFV_COORD_MRI && inv.coord_frame != FIFFV_COORD_HEAD) {
        return fail(ErrorCode::Format, "{} holds an inverse solution in coordinate frame {}; only MRI and head coordinates are supported", name, inv.coord_frame);
    }
    //
    //  Number of averages is initially one
    //
    inv.nave = 1;
    //
    //  We also need the SSP operator
    //
    inv.projs = t_pStream.read_proj(t_pStream.dirtree());

    // proj, whitener, reginv, and noisenorm are filled in by prepare_inverse_operator()

    if (!inv.src.transform_source_space_to(inv.coord_frame, mri_head_t)) {
        return fail(ErrorCode::Format, "cannot transform the source spaces of {}", name);
    }
    Log::info("\tSource spaces transformed to the inverse solution coordinate frame\n");
    return inv;
}

//=============================================================================================================

Expected<void> MNEInverseOperator::write(const std::filesystem::path& path) const
{
    auto stream = FiffStream::start_file(path);
    if (!stream) {
        return fail(ErrorCode::Io, "cannot write {}", utf8_from_path(path));
    }
    write_to_stream(*stream);
    stream->end_file();
    return {};
}

//=============================================================================================================

void MNEInverseOperator::write_to_stream(FiffStream& stream) const
{
    stream.start_block(FIFFB_MNE_INVERSE_SOLUTION);

    Log::info("\tWriting inverse operator info...\n");

    stream.write_int(FIFF_MNE_INCLUDED_METHODS, &this->methods);
    stream.write_int(FIFF_MNE_SOURCE_ORIENTATION, &this->source_ori);
    stream.write_int(FIFF_MNE_SOURCE_SPACE_NPOINTS, &this->nsource);
    stream.write_int(FIFF_MNE_COORD_FRAME, &this->coord_frame);

    // mne-python reads this to label the source estimates. It is optional
    // there, but omitting it leaves inv["units"] as None, so an operator that
    // travelled through MNE-CPP lost information the original file carried.
    if (this->units > 0)
        stream.write_int(FIFF_MNE_INVERSE_SOURCE_UNIT, &this->units);
    stream.write_float_matrix(FIFF_MNE_INVERSE_SOURCE_ORIENTATIONS, this->source_nn);
    VectorXf tmp_sing = this->sing.cast<float>();
    stream.write_float(FIFF_MNE_INVERSE_SING, tmp_sing.data(), tmp_sing.size());

    //
    //   The eigenleads and eigenfields
    //
    // The file stores the eigenleads as rows; transpose them in a single copy
    FiffNamedMatrix leads;
    leads.row_names = eigen_leads.col_names;
    leads.col_names = eigen_leads.row_names;
    leads.data = eigen_leads.data.transpose();
    leads.nrow = static_cast<fiff_int_t>(leads.data.rows());
    leads.ncol = static_cast<fiff_int_t>(leads.data.cols());
    stream.write_named_matrix(this->eigen_leads_weighted ? FIFF_MNE_INVERSE_LEADS_WEIGHTED : FIFF_MNE_INVERSE_LEADS, leads);

    stream.write_named_matrix(FIFF_MNE_INVERSE_FIELDS, eigen_fields);
    Log::info("\t[done]\n");
    //
    //   write the covariance matrices
    //
    Log::info("\tWriting noise covariance matrix.");
    stream.write_cov(noise_cov);

    Log::info("\tWriting source covariance matrix.\n");
    stream.write_cov(source_cov);
    //
    //   write the various priors
    //
    Log::info("\tWriting orientation priors.\n");
    if (!orient_prior.empty())
        stream.write_cov(orient_prior);
    if (!depth_prior.empty())
        stream.write_cov(depth_prior);
    if (!fmri_prior.empty())
        stream.write_cov(fmri_prior);

    //
    //   Parent MRI data
    //
    stream.start_block(FIFFB_MNE_PARENT_MRI_FILE);
    //   write the MRI <-> head coordinate transformation
    stream.write_coord_trans(this->mri_head_t);
    stream.end_block(FIFFB_MNE_PARENT_MRI_FILE);

    //
    //   Parent MEG measurement info
    //
    stream.write_info_base(this->info);

    //
    //   Write the source spaces
    //
    if (!src.empty())
        this->src.write_to_stream(stream);

    //
    //  We also need the SSP operator
    //
    stream.write_proj(this->projs);
    //
    //   Done!
    //
    stream.end_block(FIFFB_MNE_INVERSE_SOLUTION);
}
