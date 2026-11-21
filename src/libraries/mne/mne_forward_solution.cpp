//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_forward_solution.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::MNEForwardSolution.
 *
 * Implements FIFF read/write of the gain matrix, source-space attachment,
 * coordinate-frame conversion, label-restricted clustering using
 * @ref mne::KMeans, gain-matrix whitening against an
 * @ref mne::FiffCov noise covariance and SSP/CTF application. Mirrors
 * @c mne.forward of MNE-Python and the @c mne_inverse_operator C tool.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_forward_solution.h"

#include <fiff/fiff_ch_name.h>
#include <base/log.h>
#include <base/thread_pool.h>
#include <base/string_utils.h>
#include <fiff/fiff_coord_trans.h>
#include <fs/fs_colortable.h>
#include <fs/fs_label.h>
#include <fs/fs_surfaceset.h>
#include <math/linalg.h>
#include <math/kmeans.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SVD>
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <unsupported/Eigen/KroneckerProduct>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <memory>
#include <vector>
#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================

namespace
{

/** @return Whether any name of @p chNamesA names a channel in @p chNamesB. */
bool check_matching_chnames(const StringList& chNamesA, const StringList& chNamesB)
{
    return std::ranges::any_of(chNamesA, [&](const std::string& name) { return mne::has_channel(chNamesB, name); });
}

} // anonymous namespace

//=============================================================================================================
// CONSTANTS
//=============================================================================================================

// Return codes and axis indices (kept for documentation).
[[maybe_unused]] constexpr int FAIL = -1;
[[maybe_unused]] constexpr int OK = 0;
[[maybe_unused]] constexpr int X = 0;
[[maybe_unused]] constexpr int Y = 1;
[[maybe_unused]] constexpr int Z = 2;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEForwardSolution::MNEForwardSolution(const std::filesystem::path& path)
: MNEForwardSolution(read(path).value())
{
}

//=============================================================================================================

MNEForwardSolution::MNEForwardSolution(const std::filesystem::path& path, const ReadOptions& options)
: MNEForwardSolution(read(path, options).value())
{
}

//=============================================================================================================

void MNEForwardSolution::clear()
{
    info.clear();
    source_ori = -1;
    surf_ori = false;
    coord_frame = -1;
    nsource = -1;
    nchan = -1;
    sol = FiffNamedMatrix();
    sol_grad = FiffNamedMatrix();
    mri_head_t.clear();
    mri_filename.clear();
    mri_id.clear();
    src.clear();
    source_rr = MatrixX3f(0, 3);
    source_nn = MatrixX3f(0, 3);
}

//=============================================================================================================

Expected<void> MNEForwardSolution::write(const std::filesystem::path& path) const
{
    //
    //   Classify channels into MEG and EEG index sets
    //
    std::vector<int> megIdx, eegIdx;
    for (int k = 0; k < std::ssize(info.chs); ++k) {
        fiff_int_t kind = info.chs[k].kind;
        if (kind == FIFFV_MEG_CH || kind == FIFFV_REF_MEG_CH)
            megIdx.push_back(k);
        else if (kind == FIFFV_EEG_CH)
            eegIdx.push_back(k);
    }
    int nmeg = static_cast<int>(megIdx.size());
    int neeg = static_cast<int>(eegIdx.size());

    //
    //   Compute the total number of active source vertices
    //
    int nvert = 0;
    for (int k = 0; k < src.size(); ++k)
        nvert += src[k].nuse;

    //
    //   Open the file, create the directory
    //
    const std::string name = utf8_from_path(path);
    auto t_pStream = FiffStream::start_file(path);
    if (!t_pStream) {
        return fail(ErrorCode::Io, "cannot write {}", name);
    }
    t_pStream->start_block(FIFFB_MNE);

    //
    //   Information from the MRI file
    //
    {
        t_pStream->start_block(FIFFB_MNE_PARENT_MRI_FILE);

        if (!mri_filename.empty())
            t_pStream->write_string(FIFF_MNE_FILE_NAME, mri_filename);
        if (!mri_id.empty())
            t_pStream->write_id(FIFF_PARENT_FILE_ID, mri_id);
        t_pStream->write_coord_trans(mri_head_t);

        t_pStream->end_block(FIFFB_MNE_PARENT_MRI_FILE);
    }

    //
    //   Information from the measurement file
    //
    {
        t_pStream->start_block(FIFFB_MNE_PARENT_MEAS_FILE);

        if (!info.filename.empty())
            t_pStream->write_string(FIFF_MNE_FILE_NAME, info.filename);
        if (!info.meas_id.empty())
            t_pStream->write_id(FIFF_PARENT_BLOCK_ID, info.meas_id);
        t_pStream->write_coord_trans(info.dev_head_t);

        int totalChan = nmeg + neeg;
        t_pStream->write_int(FIFF_NCHAN, &totalChan);

        //  Write channel infos with sequential scanNo
        std::vector<FiffChInfo> allChs;
        allChs.reserve(static_cast<std::size_t>(nmeg) + static_cast<std::size_t>(neeg));
        for (int k = 0; k < nmeg; ++k)
            allChs.push_back(info.chs[megIdx[k]]);
        for (int k = 0; k < neeg; ++k)
            allChs.push_back(info.chs[eegIdx[k]]);
        for (int p = 0; p < std::ssize(allChs); ++p) {
            allChs[p].scanNo = p + 1;
            t_pStream->write_ch_info(allChs[p]);
        }

        t_pStream->write_bad_channels(info.bads);

        t_pStream->end_block(FIFFB_MNE_PARENT_MEAS_FILE);
    }

    //
    //   Write the source spaces
    //
    for (int k = 0; k < src.size(); ++k) {
        if (src[k].writeToStream(*t_pStream, false) == FIFF_FAIL) {
            t_pStream->close();
            return fail(ErrorCode::Io, "cannot write the source spaces to {}", name);
        }
    }

    //
    //   Extract sub-matrices for MEG and EEG from the combined sol
    //
    auto extractRows = [](const FiffNamedMatrix& combined,
                          const std::vector<int>& rowIdx) -> FiffNamedMatrix {
        int nRows = static_cast<int>(rowIdx.size());
        int nCols = combined.ncol;
        MatrixXd data(nRows, nCols);
        StringList row_names;
        for (int r = 0; r < nRows; ++r) {
            data.row(r) = combined.data.row(rowIdx[r]);
            row_names.push_back(combined.row_names[rowIdx[r]]);
        }
        FiffNamedMatrix sub;
        sub.nrow = nRows;
        sub.ncol = nCols;
        sub.row_names = row_names;
        sub.col_names = combined.col_names;
        sub.data = data;
        return sub;
    };

    int ori_val = (source_ori == FIFFV_MNE_FIXED_ORI) ? FIFFV_MNE_FIXED_ORI : FIFFV_MNE_FREE_ORI;
    int frame = coord_frame;

    //
    //   MEG forward solution
    //
    if (nmeg > 0) {
        t_pStream->start_block(FIFFB_MNE_FORWARD_SOLUTION);

        int val = FIFFV_MNE_MEG;
        t_pStream->write_int(FIFF_MNE_INCLUDED_METHODS, &val);
        t_pStream->write_int(FIFF_MNE_COORD_FRAME, &frame);
        t_pStream->write_int(FIFF_MNE_SOURCE_ORIENTATION, &ori_val);
        t_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NPOINTS, &nvert);
        t_pStream->write_int(FIFF_NCHAN, &nmeg);

        FiffNamedMatrix megSol = extractRows(sol, megIdx);
        megSol.transpose_named_matrix();
        t_pStream->write_named_matrix(FIFF_MNE_FORWARD_SOLUTION, megSol);

        if (!sol_grad.empty()) {
            FiffNamedMatrix megGrad = extractRows(sol_grad, megIdx);
            megGrad.transpose_named_matrix();
            t_pStream->write_named_matrix(FIFF_MNE_FORWARD_SOLUTION_GRAD, megGrad);
        }
        t_pStream->end_block(FIFFB_MNE_FORWARD_SOLUTION);
    }

    //
    //   EEG forward solution
    //
    if (neeg > 0) {
        t_pStream->start_block(FIFFB_MNE_FORWARD_SOLUTION);

        int val = FIFFV_MNE_EEG;
        t_pStream->write_int(FIFF_MNE_INCLUDED_METHODS, &val);
        t_pStream->write_int(FIFF_MNE_COORD_FRAME, &frame);
        t_pStream->write_int(FIFF_MNE_SOURCE_ORIENTATION, &ori_val);
        t_pStream->write_int(FIFF_NCHAN, &neeg);
        t_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NPOINTS, &nvert);

        FiffNamedMatrix eegSol = extractRows(sol, eegIdx);
        eegSol.transpose_named_matrix();
        t_pStream->write_named_matrix(FIFF_MNE_FORWARD_SOLUTION, eegSol);

        if (!sol_grad.empty()) {
            FiffNamedMatrix eegGrad = extractRows(sol_grad, eegIdx);
            eegGrad.transpose_named_matrix();
            t_pStream->write_named_matrix(FIFF_MNE_FORWARD_SOLUTION_GRAD, eegGrad);
        }
        t_pStream->end_block(FIFFB_MNE_FORWARD_SOLUTION);
    }

    t_pStream->end_block(FIFFB_MNE);
    t_pStream->end_file();
    t_pStream->close();
    t_pStream.reset();

    //
    //   Update the directory
    //
    {
        auto t_pStreamIn = FiffStream::open_update(path);
        if (t_pStreamIn) {
            const auto& dir = t_pStreamIn->dir();
            for (const FiffDirEntry& entry : dir) {
                if (entry.kind == FIFF_DIR_POINTER) {
                    fiff_int_t dirpos = static_cast<fiff_int_t>(t_pStreamIn->write_dir_entries(dir));
                    if (dirpos >= 0)
                        t_pStreamIn->write_dir_pointer(dirpos, entry.pos);
                    break;
                }
            }
            t_pStreamIn->close();
        }
    }

    return {};
}

//=============================================================================================================

MNEForwardSolution MNEForwardSolution::cluster_forward_solution(const FsAnnotationSet& p_AnnotationSet,
                                                                std::int32_t p_iClusterSize,
                                                                MatrixXd& p_D,
                                                                const FiffCov& p_pNoise_cov,
                                                                const FiffInfo& p_pInfo,
                                                                const std::string& p_sMethod) const
{
    Log::info("Cluster forward solution using {}.", p_sMethod);

    MNEForwardSolution p_fwdOut = MNEForwardSolution(*this);

    //Check if cov naming conventions are matching
    if (!p_pNoise_cov.names.empty() && !p_pInfo.ch_names.empty() && !check_matching_chnames(p_pNoise_cov.names, p_pInfo.ch_names)) {
        Log::warning("MNEForwardSolution::cluster_forward_solution - Cov channel names do not match with info channel names.");
    }

    //
    // Check consisty
    //
    if (this->isFixedOrient()) {
        Log::warning("Error: Fixed orientation not implemented yet!");
        return p_fwdOut;
    }

    MatrixXd t_G_Whitened(0, 0);
    bool t_bUseWhitened = false;
    //
    //Whiten gain matrix before clustering -> cause diffenerent units Magnetometer, Gradiometer and EEG
    //
    if (!p_pNoise_cov.empty() && !p_pInfo.empty()) {
        FiffInfo p_outFwdInfo;
        FiffCov p_outNoiseCov;
        MatrixXd p_outWhitener;
        std::int32_t p_outNumNonZero;
        //do whitening with noise cov
        this->prepare_forward(p_pInfo, p_pNoise_cov, false, p_outFwdInfo, t_G_Whitened, p_outNoiseCov, p_outWhitener, p_outNumNonZero);
        Log::info("\tWhitening the forward solution.");

        t_G_Whitened = p_outWhitener * t_G_Whitened;
        t_bUseWhitened = true;
    }

    //
    // Assemble input data
    //
    std::int32_t count;
    std::int32_t offset;

    MatrixXd t_G_new;

    for (std::int32_t h = 0; h < this->src.size(); ++h) {
        count = 0;
        offset = 0;

        // Offset for continuous indexing;
        if (h > 0)
            for (std::int32_t j = 0; j < h; ++j)
                offset += this->src[j].nuse;

        if (h == 0)
            Log::info("Cluster Left Hemisphere");
        else
            Log::info("Cluster Right Hemisphere");

        const FsAnnotation& annotation = p_AnnotationSet[h];
        FsColortable t_CurrentColorTable = annotation.getColortable();
        VectorXi label_ids = t_CurrentColorTable.getLabelIds();

        // Get label ids for every vertex
        VectorXi vertno_labeled = VectorXi::Zero(this->src[h].vertno.rows());

        //ToDo make this more universal -> using FsLabel instead of annotations - obsolete when using Labels
        for (std::int32_t i = 0; i < vertno_labeled.rows(); ++i)
            vertno_labeled[i] = p_AnnotationSet[h].getLabelIds()[this->src[h].vertno[i]];

        std::vector<RegionData> regionDataIn;

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

                //get selected G
                MatrixXd t_G(this->sol.data.rows(), idcs.rows() * 3);
                MatrixXd t_G_Whitened_Roi(t_G_Whitened.rows(), idcs.rows() * 3);

                for (std::int32_t j = 0; j < idcs.rows(); ++j) {
                    t_G.block(0, j * 3, t_G.rows(), 3) = this->sol.data.block(0, (idcs[j] + offset) * 3, t_G.rows(), 3);
                    if (t_bUseWhitened)
                        t_G_Whitened_Roi.block(0, j * 3, t_G_Whitened_Roi.rows(), 3) = t_G_Whitened.block(0, (idcs[j] + offset) * 3, t_G_Whitened_Roi.rows(), 3);
                }

                std::int32_t nSens = t_G.rows();
                std::int32_t nSources = t_G.cols() / 3;

                if (nSources > 0) {
                    RegionData t_sensG;

                    t_sensG.idcs = idcs;
                    t_sensG.iLabelIdxIn = i;
                    t_sensG.nClusters = static_cast<int>(ceil(static_cast<double>(nSources) / static_cast<double>(p_iClusterSize)));

                    t_sensG.matRoiGOrig = t_G;

                    Log::info("{} Cluster(s)...", t_sensG.nClusters);

                    // Reshape Input data -> sources rows; sensors columns
                    t_sensG.matRoiG = MatrixXd(t_G.cols() / 3, 3 * nSens);
                    for (std::int32_t j = 0; j < nSens; ++j) {
                        for (std::int32_t k = 0; k < t_sensG.matRoiG.rows(); ++k)
                            t_sensG.matRoiG.block(k, j * 3, 1, 3) = t_G.block(j, k * 3, 1, 3);
                    }
                    // The whitened gain only has the channels prepare_forward() kept, so fewer rows than t_G
                    if (t_bUseWhitened) {
                        const std::int32_t nSensWhitened = static_cast<std::int32_t>(t_G_Whitened_Roi.rows());
                        t_sensG.matRoiGWhitened = MatrixXd(t_G_Whitened_Roi.cols() / 3, 3 * nSensWhitened);
                        for (std::int32_t j = 0; j < nSensWhitened; ++j) {
                            for (std::int32_t k = 0; k < t_sensG.matRoiGWhitened.rows(); ++k)
                                t_sensG.matRoiGWhitened.block(k, j * 3, 1, 3) = t_G_Whitened_Roi.block(j, k * 3, 1, 3);
                        }
                    }

                    t_sensG.bUseWhitened = t_bUseWhitened;

                    t_sensG.sDistMeasure = p_sMethod;

                    regionDataIn.push_back(std::move(t_sensG));

                    Log::info("[added]");
                } else {
                    Log::warning("failed! FsLabel contains no sources.");
                }
            }
        }

        //
        // Calculate clusters
        //
        Log::info("Clustering...");
        const std::vector<RegionDataOut> res = mne::ThreadPool::global().parallel_map(regionDataIn, [](const RegionData& region) { return region.cluster(); });

        //
        // Assign results
        //
        MatrixXd t_G_partial;

        std::int32_t nClusters;
        std::int32_t nSens;
        auto itIn = regionDataIn.cbegin();
        for (auto itOut = res.cbegin(); itOut != res.cend(); ++itOut) {
            nClusters = itOut->ctrs.rows();
            nSens = itOut->ctrs.cols() / 3;
            t_G_partial = MatrixXd::Zero(nSens, nClusters * 3);

            //
            // Assign the centroid for each cluster to the partial G
            //
            //ToDo change this use indeces found with whitened data
            for (std::int32_t j = 0; j < nSens; ++j)
                for (std::int32_t k = 0; k < nClusters; ++k)
                    t_G_partial.block(j, k * 3, 1, 3) = itOut->ctrs.block(k, j * 3, 1, 3);

            //
            // Get cluster indizes and its distances to the centroid
            //
            for (std::int32_t j = 0; j < nClusters; ++j) {
                VectorXi clusterIdcs = VectorXi::Zero(itOut->roiIdx.rows());
                VectorXd clusterDistance = VectorXd::Zero(itOut->roiIdx.rows());
                MatrixX3f clusterSource_rr = MatrixX3f::Zero(itOut->roiIdx.rows(), 3);
                std::int32_t nClusterIdcs = 0;
                for (std::int32_t k = 0; k < itOut->roiIdx.rows(); ++k) {
                    if (itOut->roiIdx[k] == j) {
                        clusterIdcs[nClusterIdcs] = itIn->idcs[k];

                        const std::int32_t hemiOffset = h == 0 ? 0 : this->src[0].nuse;
                        clusterSource_rr.row(nClusterIdcs) = this->source_rr.row(hemiOffset + itIn->idcs[k]);
                        clusterDistance[nClusterIdcs] = itOut->D(k, j);
                        ++nClusterIdcs;
                    }
                }
                clusterIdcs.conservativeResize(nClusterIdcs);
                clusterSource_rr.conservativeResize(nClusterIdcs, 3);
                clusterDistance.conservativeResize(nClusterIdcs);

                VectorXi clusterVertnos = VectorXi::Zero(clusterIdcs.size());
                for (std::int32_t k = 0; k < clusterVertnos.size(); ++k)
                    clusterVertnos(k) = this->src[h].vertno[clusterIdcs(k)];

                p_fwdOut.src.hemisphereAt(h)->cluster_info.clusterVertnos.push_back(clusterVertnos);
                p_fwdOut.src.hemisphereAt(h)->cluster_info.clusterSource_rr.push_back(clusterSource_rr);
                p_fwdOut.src.hemisphereAt(h)->cluster_info.clusterDistances.push_back(clusterDistance);
                p_fwdOut.src.hemisphereAt(h)->cluster_info.clusterLabelIds.push_back(label_ids[itOut->iLabelIdxOut]);
                p_fwdOut.src.hemisphereAt(h)->cluster_info.clusterLabelNames.push_back(t_CurrentColorTable.getNames()[itOut->iLabelIdxOut]);
            }

            //
            // Assign partial G to new LeadField
            //
            if (t_G_partial.rows() > 0 && t_G_partial.cols() > 0) {
                t_G_new.conservativeResize(t_G_partial.rows(), t_G_new.cols() + t_G_partial.cols());
                t_G_new.block(0, t_G_new.cols() - t_G_partial.cols(), t_G_new.rows(), t_G_partial.cols()) = t_G_partial;

                // Map the centroids to the closest rr
                for (std::int32_t k = 0; k < nClusters; ++k) {
                    double sqec = sqrt((itIn->matRoiGOrig.block(0, 0, itIn->matRoiGOrig.rows(), 3) - t_G_partial.block(0, k * 3, t_G_partial.rows(), 3)).array().pow(2).sum());
                    double sqec_min = sqec;
                    std::int32_t j_min = 0;
                    for (std::int32_t j = 1; j < itIn->idcs.rows(); ++j) {
                        sqec = sqrt((itIn->matRoiGOrig.block(0, j * 3, itIn->matRoiGOrig.rows(), 3) - t_G_partial.block(0, k * 3, t_G_partial.rows(), 3)).array().pow(2).sum());

                        if (sqec < sqec_min) {
                            sqec_min = sqec;
                            j_min = j;
                        }
                    }

                    // Take the closest coordinates
                    std::int32_t sel_idx = itIn->idcs[j_min];

                    p_fwdOut.src.hemisphereAt(h)->cluster_info.centroidVertno.push_back(this->src[h].vertno[sel_idx]);
                    p_fwdOut.src.hemisphereAt(h)->cluster_info.centroidSource_rr.push_back(this->src[h].rr.row(this->src[h].vertno[sel_idx]));
                    // Option 2 label ID
                    p_fwdOut.src[h].vertno[count] = p_fwdOut.src.hemisphereAt(h)->cluster_info.clusterLabelIds[count];

                    ++count;
                }
            }

            ++itIn;
        }

        //
        // Assemble new hemisphere information
        //
        p_fwdOut.src[h].vertno.conservativeResize(count);
        p_fwdOut.src[h].nuse = count;
        // Patches belong to the original sources; a cluster's normal is that of its centroid
        p_fwdOut.src.hemisphereAt(h)->patch_inds = VectorXi();

        Log::info("[done]");
    }

    //
    // Cluster operator D (sources x clusters)
    //
    std::int32_t totalNumOfClust = 0;
    for (std::int32_t h = 0; h < 2; ++h)
        totalNumOfClust += static_cast<std::int32_t>(p_fwdOut.src.hemisphereAt(h)->cluster_info.clusterVertnos.size());

    if (this->isFixedOrient())
        p_D = MatrixXd::Zero(this->sol.data.cols(), totalNumOfClust);
    else
        p_D = MatrixXd::Zero(this->sol.data.cols(), totalNumOfClust * 3);

    std::vector<VectorXi> t_vertnos = this->src.get_vertno();

    std::int32_t currentCluster = 0;
    for (std::int32_t h = 0; h < 2; ++h) {
        int hemiOffset = h == 0 ? 0 : t_vertnos[0].size();
        for (std::int32_t i = 0; i < std::ssize(p_fwdOut.src.hemisphereAt(h)->cluster_info.clusterVertnos); ++i) {
            VectorXi idx_sel;
            Linalg::intersect(t_vertnos[h], p_fwdOut.src.hemisphereAt(h)->cluster_info.clusterVertnos[i], idx_sel);

            idx_sel.array() += hemiOffset;

            double selectWeight = 1.0 / idx_sel.size();
            if (this->isFixedOrient()) {
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
    p_fwdOut.sol.data = t_G_new;
    p_fwdOut.sol.ncol = t_G_new.cols();

    p_fwdOut.nsource = p_fwdOut.sol.ncol / 3;

    return p_fwdOut;
}

//=============================================================================================================

MNEForwardSolution MNEForwardSolution::reduce_forward_solution(std::int32_t p_iNumDipoles, MatrixXd& p_D) const
{
    MNEForwardSolution p_fwdOut = MNEForwardSolution(*this);

    bool isFixed = p_fwdOut.isFixedOrient();
    std::int32_t np = isFixed ? p_fwdOut.sol.data.cols() : p_fwdOut.sol.data.cols() / 3;

    if (p_iNumDipoles > np)
        return p_fwdOut;

    VectorXi sel(p_iNumDipoles);

    float t_fStep = static_cast<float>(np) / static_cast<float>(p_iNumDipoles);

    for (std::int32_t i = 0; i < p_iNumDipoles; ++i) {
        float t_fCurrent = static_cast<float>(i) * t_fStep;
        sel[i] = static_cast<int>(std::floor(t_fCurrent));
    }

    if (isFixed) {
        p_D = MatrixXd::Zero(p_fwdOut.sol.data.cols(), p_iNumDipoles);
        for (std::int32_t i = 0; i < p_iNumDipoles; ++i)
            p_D(sel[i], i) = 1;
    } else {
        p_D = MatrixXd::Zero(p_fwdOut.sol.data.cols(), p_iNumDipoles * 3);
        for (std::int32_t i = 0; i < p_iNumDipoles; ++i)
            for (std::int32_t j = 0; j < 3; ++j)
                p_D((sel[i] * 3) + j, (i * 3) + j) = 1;
    }

    // New gain matrix
    p_fwdOut.sol.data = this->sol.data * p_D;

    MatrixX3f rr(p_iNumDipoles, 3);

    MatrixX3f nn(p_iNumDipoles, 3);

    for (std::int32_t i = 0; i < p_iNumDipoles; ++i) {
        rr.row(i) = this->source_rr.row(sel(i));
        nn.row(i) = this->source_nn.row(sel(i));
    }

    p_fwdOut.source_rr = rr;
    p_fwdOut.source_nn = nn;

    p_fwdOut.sol.ncol = p_fwdOut.sol.data.cols();

    p_fwdOut.nsource = p_iNumDipoles;

    return p_fwdOut;
}

//=============================================================================================================

FiffCov MNEForwardSolution::compute_depth_prior(const MatrixXd& Gain, const FiffInfo& gain_info, bool is_fixed_ori, double exp, double limit, const MatrixXd& patch_areas, bool limit_depth_chs)
{
    Log::info("\tCreating the depth weighting matrix...");

    MatrixXd G(Gain);
    // If possible, pick best depth-weighting channels
    if (limit_depth_chs)
        MNEForwardSolution::restrict_gain_matrix(G, gain_info);

    VectorXd d;
    // Compute the gain matrix
    if (is_fixed_ori) {
        d = G.array().square().colwise().sum().transpose();
        // A spherical lead field can vanish at the centre
        const double minNonZero = (d.array() != 0.0).select(d.array(), std::numeric_limits<double>::infinity()).minCoeff();
        d = (d.array() == 0.0).select(minNonZero, d.array());
    } else {
        std::int32_t n_pos = G.cols() / 3;
        d = VectorXd::Zero(n_pos);
        MatrixXd Gk;
        for (std::int32_t k = 0; k < n_pos; ++k) {
            Gk = G.block(0, 3 * k, G.rows(), 3);
            JacobiSVD<MatrixXd> svd(Gk.transpose() * Gk);
            d[k] = svd.singularValues().maxCoeff();
        }
    }

    if (patch_areas.size() > 0) {
        d.array() /= patch_areas.reshaped().array().square();
        Log::info("\tPatch areas taken into account in the depth weighting");
    }

    std::int32_t n_limit;
    VectorXd w = d.cwiseInverse();
    VectorXd ws = w;
    VectorXd wpp;
    Linalg::sort<double>(ws, false);
    double weight_limit = pow(limit, 2);
    if (!limit_depth_chs) {
        // match old mne-python behavor
        std::int32_t ind = 0;
        ws.minCoeff(&ind);
        n_limit = ind;
        limit = ws[ind] * weight_limit;
    } else {
        // match C code behavior
        limit = ws[ws.size() - 1];
        std::int32_t ind = 0;
        n_limit = d.size();
        if (ws[ws.size() - 1] > weight_limit * ws[0]) {
            double th = weight_limit * ws[0];
            for (std::int32_t i = 0; i < ws.size(); ++i) {
                if (ws[i] > th) {
                    ind = i;
                    break;
                }
            }
            limit = ws[ind];
            n_limit = ind;
        }
    }

    Log::info("\tlimit = {}/{} = {:f}", n_limit + 1, d.size(), sqrt(limit / ws[0]));
    double scale = 1.0 / limit;
    Log::info("\tscale = {:g} exp = {:g}", scale, exp);

    VectorXd t_w = w.array() / limit;
    for (std::int32_t i = 0; i < t_w.size(); ++i)
        t_w[i] = t_w[i] > 1 ? 1 : t_w[i];
    wpp = t_w.array().pow(exp);

    FiffCov depth_prior;
    if (is_fixed_ori)
        depth_prior.data = wpp;
    else {
        depth_prior.data.resize(wpp.rows() * 3, 1);
        std::int32_t idx = 0;
        double v;
        for (std::int32_t i = 0; i < wpp.rows(); ++i) {
            idx = i * 3;
            v = wpp[i];
            depth_prior.data(idx, 0) = v;
            depth_prior.data(idx + 1, 0) = v;
            depth_prior.data(idx + 2, 0) = v;
        }
    }

    depth_prior.kind = FIFFV_MNE_DEPTH_PRIOR_COV;
    depth_prior.diag = true;
    depth_prior.dim = depth_prior.data.rows();
    depth_prior.nfree = 1;

    return depth_prior;
}

//=============================================================================================================

FiffCov MNEForwardSolution::compute_orient_prior(float loose)
{
    bool is_fixed_ori = this->isFixedOrient();
    std::int32_t n_sources = this->sol.data.cols();

    if (0 <= loose && loose <= 1) {
        if (loose < 1 && !this->surf_ori) {
            Log::warning("\tForward operator is not oriented in surface coordinates. loose parameter should be None not {:f}.", loose);
            loose = 1;
            Log::info("\tSetting loose to {:f}.", loose);
        }

        if (is_fixed_ori) {
            Log::info("\tIgnoring loose parameter with forward operator with fixed orientation.");
            loose = 0.0;
        }
    } else {
        if (loose < 0 || loose > 1) {
            Log::warning("Warning: Loose value should be in interval [0,1] not {:f}.\n", loose);
            loose = loose > 1 ? 1 : 0;
            Log::info("Setting loose to {:f}.", loose);
        }
    }

    FiffCov orient_prior;
    orient_prior.data = VectorXd::Ones(n_sources);
    if (!is_fixed_ori && (0 <= loose && loose <= 1)) {
        Log::info("\tApplying loose dipole orientations. Loose value of {:f}.", loose);
        for (std::int32_t i = 0; i < n_sources; i += 3)
            orient_prior.data.block(i, 0, 2, 1).array() *= loose;

        orient_prior.kind = FIFFV_MNE_ORIENT_PRIOR_COV;
        orient_prior.diag = true;
        orient_prior.dim = orient_prior.data.size();
        orient_prior.nfree = 1;
    }
    return orient_prior;
}

//=============================================================================================================

MNEForwardSolution MNEForwardSolution::pick_channels(const StringList& include,
                                                     const StringList& exclude) const
{
    MNEForwardSolution fwd(*this);

    if (include.size() == 0 && exclude.size() == 0)
        return fwd;

    RowVectorXi sel = FiffInfo::pick_channels(fwd.sol.row_names, include, exclude);

    // Do we have something?
    const int nuse = static_cast<int>(sel.size());

    if (nuse == 0) {
        Log::info("Nothing remains after picking. Returning original forward solution.");
        return fwd;
    }
    Log::info("\t{} out of {} channels remain after picking", nuse, fwd.nchan);

    //   Pick the correct rows of the forward operator
    MatrixXd newData(nuse, fwd.sol.data.cols());
    for (int i = 0; i < nuse; ++i)
        newData.row(i) = fwd.sol.data.row(sel[i]);

    fwd.sol.data = newData;
    fwd.sol.nrow = nuse;

    StringList ch_names;
    ch_names.reserve(nuse);
    for (std::int32_t i = 0; i < sel.cols(); ++i)
        ch_names.push_back(fwd.sol.row_names[sel(i)]);
    fwd.nchan = nuse;

    std::vector<FiffChInfo> chs;
    chs.reserve(nuse);
    for (std::int32_t i = 0; i < sel.cols(); ++i)
        chs.push_back(fwd.info.chs[sel(i)]);
    fwd.info.chs = std::move(chs);
    fwd.info.ch_names = ch_names;
    fwd.info.nchan = nuse;

    StringList bads;
    for (const std::string& bad : fwd.info.bads)
        if (mne::has_channel(ch_names, bad))
            bads.push_back(bad);
    fwd.info.bads = std::move(bads);
    fwd.sol.row_names = std::move(ch_names);

    if (!fwd.sol_grad.empty()) {
        newData.resize(nuse, fwd.sol_grad.data.cols());
        for (int i = 0; i < nuse; ++i)
            newData.row(i) = fwd.sol_grad.data.row(sel[i]);
        fwd.sol_grad.data = newData;
        fwd.sol_grad.nrow = nuse;
        StringList row_names;
        for (std::int32_t i = 0; i < sel.cols(); ++i)
            row_names.push_back(fwd.sol_grad.row_names[sel(i)]);
        fwd.sol_grad.row_names = row_names;
    }

    return fwd;
}

//=============================================================================================================

MNEForwardSolution MNEForwardSolution::pick_regions(const std::vector<FsLabel>& p_qListLabels) const
{
    VectorXi selVertices;

    std::int32_t iSize = 0;
    for (const auto& label : p_qListLabels) {
        VectorXi currentSelection;
        this->src.label_src_vertno_sel(label, currentSelection);

        selVertices.conservativeResize(iSize + currentSelection.size());
        selVertices.block(iSize, 0, currentSelection.size(), 1) = currentSelection;
        iSize = selVertices.size();
    }

    Linalg::sort(selVertices, false);

    MNEForwardSolution selectedFwd(*this);

    MatrixX3f rr(selVertices.size(), 3);
    for (std::int32_t i = 0; i < selVertices.size(); ++i)
        rr.row(i) = selectedFwd.source_rr.row(selVertices[i]);

    // Free orientation stores three normals and three gain columns per source.
    const VectorXi selSolIdcs = isFixedOrient() ? selVertices : tripletSelection(selVertices);

    MatrixX3f nn(selSolIdcs.size(), 3);
    MatrixXd G(selectedFwd.sol.data.rows(), selSolIdcs.size());
    for (std::int32_t i = 0; i < selSolIdcs.size(); ++i) {
        nn.row(i) = selectedFwd.source_nn.row(selSolIdcs[i]);
        G.col(i) = selectedFwd.sol.data.col(selSolIdcs[i]);
    }

    selectedFwd.source_rr = rr;
    selectedFwd.source_nn = nn;
    selectedFwd.sol.data = G;
    selectedFwd.sol.nrow = selectedFwd.sol.data.rows();
    selectedFwd.sol.ncol = selectedFwd.sol.data.cols();
    selectedFwd.nsource = static_cast<int>(selVertices.size());

    selectedFwd.src = selectedFwd.src.pick_regions(p_qListLabels);

    return selectedFwd;
}

//=============================================================================================================

MNEForwardSolution MNEForwardSolution::pick_types(bool meg, bool eeg, const StringList& include, const StringList& exclude) const
{
    RowVectorXi sel = info.pick_types(meg, eeg, false, include, exclude);

    StringList include_ch_names;
    for (std::int32_t i = 0; i < sel.cols(); ++i)
        include_ch_names.push_back(info.ch_names[sel[i]]);

    return this->pick_channels(include_ch_names);
}

//=============================================================================================================

void MNEForwardSolution::prepare_forward(const FiffInfo& p_info,
                                         const FiffCov& p_noise_cov,
                                         bool p_pca,
                                         FiffInfo& p_outFwdInfo,
                                         MatrixXd& gain,
                                         FiffCov& p_outNoiseCov,
                                         MatrixXd& p_outWhitener,
                                         std::int32_t& p_outNumNonZero) const
{
    StringList fwd_ch_names, ch_names;
    for (std::int32_t i = 0; i < std::ssize(this->info.chs); ++i)
        fwd_ch_names.push_back(this->info.chs[i].ch_name);

    ch_names.clear();
    for (std::int32_t i = 0; i < std::ssize(p_info.chs); ++i)
        if (!mne::has_channel(p_info.bads, p_info.chs[i].ch_name) && !mne::has_channel(p_noise_cov.bads, p_info.chs[i].ch_name) && mne::has_channel(p_noise_cov.names, p_info.chs[i].ch_name) && mne::has_channel(fwd_ch_names, p_info.chs[i].ch_name))
            ch_names.push_back(p_info.chs[i].ch_name);

    std::int32_t n_chan = static_cast<int>(ch_names.size());
    Log::info("Computing inverse operator with {} channels.", n_chan);

    //
    //   Handle noise cov
    //
    p_outNoiseCov = p_noise_cov.prepare_noise_cov(p_info, ch_names);

    //   Omit the zeroes due to projection
    p_outNumNonZero = 0;
    VectorXi t_vecNonZero = VectorXi::Zero(n_chan);
    for (std::int32_t i = 0; i < p_outNoiseCov.eig.rows(); ++i) {
        if (p_outNoiseCov.eig[i] > 0) {
            t_vecNonZero[p_outNumNonZero] = i;
            ++p_outNumNonZero;
        }
    }
    if (p_outNumNonZero > 0)
        t_vecNonZero.conservativeResize(p_outNumNonZero);

    if (p_outNumNonZero > 0) {
        if (p_pca) {
            // Rows of eigvec are the eigenvectors: one whitened component per non-zero eigenvalue (mne _get_whitener)
            p_outWhitener = MatrixXd(p_outNumNonZero, n_chan);
            for (std::int32_t i = 0; i < p_outNumNonZero; ++i)
                p_outWhitener.row(i) = p_outNoiseCov.eigvec.row(t_vecNonZero[i]) / sqrt(p_outNoiseCov.eig(t_vecNonZero[i]));
            Log::info("\tReducing data rank to {}.", p_outNumNonZero);
        } else {
            Log::info("Creating non pca whitener.");
            p_outWhitener = MatrixXd::Zero(n_chan, n_chan);
            for (std::int32_t i = 0; i < p_outNumNonZero; ++i)
                p_outWhitener(t_vecNonZero[i], t_vecNonZero[i]) = 1.0 / sqrt(p_outNoiseCov.eig(t_vecNonZero[i]));
            // Rows of eigvec are the eigenvectors: W = diag(1/sqrt(eig)) * eigvec
            p_outWhitener *= p_outNoiseCov.eigvec;
        }
    }

    VectorXi fwd_idx = VectorXi::Zero(ch_names.size());
    VectorXi info_idx = VectorXi::Zero(ch_names.size());
    std::int32_t idx;
    std::int32_t count_fwd_idx = 0;
    std::int32_t count_info_idx = 0;
    for (std::int32_t i = 0; i < static_cast<int>(ch_names.size()); ++i) {
        idx = mne::channel_index(fwd_ch_names, ch_names[i]);
        if (idx > -1) {
            fwd_idx[count_fwd_idx] = idx;
            ++count_fwd_idx;
        }
        idx = mne::channel_index(p_info.ch_names, ch_names[i]);
        if (idx > -1) {
            info_idx[count_info_idx] = idx;
            ++count_info_idx;
        }
    }
    fwd_idx.conservativeResize(count_fwd_idx);
    info_idx.conservativeResize(count_info_idx);

    gain.resize(count_fwd_idx, this->sol.data.cols());
    for (std::int32_t i = 0; i < count_fwd_idx; ++i)
        gain.row(i) = this->sol.data.row(fwd_idx[i]);

    p_outFwdInfo = p_info.pick_info(info_idx);

    Log::info("\tTotal rank is {}", p_outNumNonZero);
}

//=============================================================================================================

Expected<MNEForwardSolution> MNEForwardSolution::read(const std::filesystem::path& path)
{
    return read(path, ReadOptions{});
}

//=============================================================================================================

Expected<MNEForwardSolution> MNEForwardSolution::read(const std::filesystem::path& path, const ReadOptions& options)
{
    auto opened = FiffStream::open(path);
    if (!opened) {
        return std::unexpected(opened.error());
    }
    FiffStream& t_pStream = *opened;
    const std::string name = utf8_from_path(path);
    Log::info("Reading forward solution from {}...", name);
    //
    //   Find all forward solutions
    //
    std::vector<std::shared_ptr<FiffDirNode>> fwds = t_pStream.dirtree()->dir_tree_find(FIFFB_MNE_FORWARD_SOLUTION);

    if (fwds.size() == 0) {
        return fail(ErrorCode::NotFound, "no forward solution in {}", name);
    }
    //
    //   Parent MRI data
    //
    std::vector<std::shared_ptr<FiffDirNode>> parent_mri = t_pStream.dirtree()->dir_tree_find(FIFFB_MNE_PARENT_MRI_FILE);
    if (parent_mri.size() == 0) {
        return fail(ErrorCode::Format, "no parent MRI information in {}", name);
    }

    auto sourceSpaces = MNESourceSpaces::read(t_pStream);
    if (!sourceSpaces) {
        return std::unexpected(sourceSpaces.error());
    }
    MNESourceSpaces& t_SourceSpace = *sourceSpaces;

    for (std::int32_t k = 0; k < t_SourceSpace.size(); ++k)
        t_SourceSpace[k].id = t_SourceSpace[k].find_source_space_hemi();

    //
    //   Bad channel list
    //
    StringList bads;
    if (options.exclude_bads) {
        bads = t_pStream.read_bad_channels(t_pStream.dirtree());
        if (bads.size() > 0) {
            Log::info("\t{} bad channels ( ", bads.size());
            for (std::int32_t i = 0; i < static_cast<int>(bads.size()); ++i)
                Log::info("\"{}\" ", bads[i]);
            Log::info(") read");
        }
    }

    //
    //   Locate and read the forward solutions
    //
    std::optional<FiffTag> t_pTag;
    std::shared_ptr<FiffDirNode> megnode;
    std::shared_ptr<FiffDirNode> eegnode;
    for (const auto& fwd : fwds) {
        if (t_pTag = fwd->find_tag(t_pStream, FIFF_MNE_INCLUDED_METHODS); !t_pTag) {
            return fail(ErrorCode::Format, "a forward solution in {} lists no methods", name);
        }
        if (*t_pTag->to_int() == FIFFV_MNE_MEG) {
            Log::info("MEG solution found");
            megnode = fwd;
        } else if (*t_pTag->to_int() == FIFFV_MNE_EEG) {
            Log::info("EEG solution found");
            eegnode = fwd;
        }
    }

    MNEForwardSolution megfwd;
    if (read_one(t_pStream, megnode, megfwd)) {
        Log::info("\tRead MEG forward solution ({} sources, {} channels, {} orientations)", megfwd.nsource, megfwd.nchan,
                  megfwd.source_ori == FIFFV_MNE_FIXED_ORI ? "fixed" : "free");
    }
    MNEForwardSolution eegfwd;
    if (read_one(t_pStream, eegnode, eegfwd)) {
        Log::info("\tRead EEG forward solution ({} sources, {} channels, {} orientations)", eegfwd.nsource, eegfwd.nchan,
                  eegfwd.source_ori == FIFFV_MNE_FIXED_ORI ? "fixed" : "free");
    }

    //
    //   Merge the MEG and EEG solutions together
    //
    MNEForwardSolution fwd;

    if (!megfwd.empty() && !eegfwd.empty()) {
        if (megfwd.sol.data.cols() != eegfwd.sol.data.cols() ||
            megfwd.source_ori != eegfwd.source_ori ||
            megfwd.nsource != eegfwd.nsource ||
            megfwd.coord_frame != eegfwd.coord_frame) {
            return fail(ErrorCode::Format, "the MEG and EEG forward solutions in {} do not match", name);
        }

        fwd = MNEForwardSolution(megfwd);
        fwd.sol.data = MatrixXd(megfwd.sol.nrow + eegfwd.sol.nrow, megfwd.sol.ncol);

        fwd.sol.data.block(0, 0, megfwd.sol.nrow, megfwd.sol.ncol) = megfwd.sol.data;
        fwd.sol.data.block(megfwd.sol.nrow, 0, eegfwd.sol.nrow, eegfwd.sol.ncol) = eegfwd.sol.data;
        fwd.sol.nrow = megfwd.sol.nrow + eegfwd.sol.nrow;
        fwd.sol.row_names.insert(fwd.sol.row_names.end(), eegfwd.sol.row_names.begin(), eegfwd.sol.row_names.end());

        if (!fwd.sol_grad.empty()) {
            fwd.sol_grad.data.resize(megfwd.sol_grad.data.rows() + eegfwd.sol_grad.data.rows(), megfwd.sol_grad.data.cols());

            fwd.sol_grad.data.block(0, 0, megfwd.sol_grad.data.rows(), megfwd.sol_grad.data.cols()) = megfwd.sol_grad.data;
            fwd.sol_grad.data.block(megfwd.sol_grad.data.rows(), 0, eegfwd.sol_grad.data.rows(), eegfwd.sol_grad.data.cols()) = eegfwd.sol_grad.data;

            fwd.sol_grad.nrow = megfwd.sol_grad.nrow + eegfwd.sol_grad.nrow;
            fwd.sol_grad.row_names.insert(fwd.sol_grad.row_names.end(), eegfwd.sol_grad.row_names.begin(), eegfwd.sol_grad.row_names.end());
        }
        fwd.nchan = megfwd.nchan + eegfwd.nchan;
        Log::info("\tMEG and EEG forward solutions combined");
    } else if (!megfwd.empty())
        fwd = std::move(megfwd); //not copied for the sake of speed
    else
        fwd = std::move(eegfwd); //not copied for the sake of speed

    //
    //   Get the MRI <-> head coordinate transformation
    //
    if (t_pTag = parent_mri[0]->find_tag(t_pStream, FIFF_COORD_TRANS); !t_pTag) {
        return fail(ErrorCode::Format, "no MRI/head coordinate transformation in {}", name);
    } else {
        fwd.mri_head_t = t_pTag->to_coord_trans();

        if (fwd.mri_head_t.from != FIFFV_COORD_MRI || fwd.mri_head_t.to != FIFFV_COORD_HEAD) {
            fwd.mri_head_t.invert_transform();
            if (fwd.mri_head_t.from != FIFFV_COORD_MRI || fwd.mri_head_t.to != FIFFV_COORD_HEAD) {
                return fail(ErrorCode::Format, "no MRI/head coordinate transformation in {}", name);
            }
        }
    }

    //
    // get parent MEG info -> from python package
    //
    t_pStream.read_meas_info_base(t_pStream.dirtree(), fwd.info);

    t_pStream.close();

    //
    //   Transform the source spaces to the correct coordinate frame
    //   if necessary
    //
    if (fwd.coord_frame != FIFFV_COORD_MRI && fwd.coord_frame != FIFFV_COORD_HEAD) {
        return fail(ErrorCode::Format, "{} holds a forward solution in coordinate frame {}; only MRI and head coordinates are supported", name, fwd.coord_frame);
    }

    //
    std::int32_t nuse = 0;
    t_SourceSpace.transform_source_space_to(fwd.coord_frame, fwd.mri_head_t);
    for (std::int32_t k = 0; k < t_SourceSpace.size(); ++k)
        nuse += t_SourceSpace[k].nuse;

    if (nuse != fwd.nsource) {
        return fail(ErrorCode::Format, "the source spaces of {} do not match its forward solution", name);
    }

    Log::info("\tSource spaces transformed to the forward solution coordinate frame");
    fwd.src = t_SourceSpace; //not new MNESourceSpaces(t_SourceSpace); for sake of speed
    //
    //   Handle the source locations and orientations
    //
    if (fwd.isFixedOrient() || options.force_fixed) {
        // Fixing a free solution uses the average patch normals when available (mne-python use_cps=True).
        const bool patchNormals = !fwd.isFixedOrient() && t_SourceSpace.hemisphereAt(0) && t_SourceSpace.hemisphereAt(0)->patch_inds.size() > 0;
        nuse = 0;
        fwd.source_rr = MatrixXf::Zero(fwd.nsource, 3);
        fwd.source_nn = MatrixXf::Zero(fwd.nsource, 3);
        for (std::int32_t k = 0; k < t_SourceSpace.size(); ++k) {
            for (std::int32_t q = 0; q < t_SourceSpace[k].nuse; ++q) {
                fwd.source_rr.row(nuse + q) = t_SourceSpace[k].rr.row(t_SourceSpace[k].vertno(q));
                if (patchNormals) {
                    auto* hemi = t_SourceSpace.hemisphereAt(k);
                    RowVector3f nn = RowVector3f::Zero();
                    for (int v : hemi->pinfo[hemi->patch_inds[q]])
                        nn += t_SourceSpace[k].nn.row(v);
                    fwd.source_nn.row(nuse + q) = nn.normalized();
                } else {
                    fwd.source_nn.row(nuse + q) = t_SourceSpace[k].nn.row(t_SourceSpace[k].vertno(q));
                }
            }
            nuse += t_SourceSpace[k].nuse;
        }
        //
        //   Modify the forward solution for fixed source orientations
        //
        if (fwd.source_ori != FIFFV_MNE_FIXED_ORI) {
            Log::info("\tChanging to fixed-orientation forward solution...");

            MatrixXd tmp = fwd.source_nn.transpose().cast<double>();
            SparseMatrix<double> fix_rot = Linalg::make_block_diag(tmp, 1);
            fwd.sol.data *= fix_rot;
            fwd.sol.ncol = fwd.nsource;
            fwd.source_ori = FIFFV_MNE_FIXED_ORI;

            if (!fwd.sol_grad.empty()) {
                SparseMatrix<double> t_matKron;
                SparseMatrix<double> t_eye(3, 3);
                for (std::int32_t i = 0; i < 3; ++i)
                    t_eye.insert(i, i) = 1.0f;
                t_matKron = kroneckerProduct(fix_rot, t_eye); //kron(fix_rot,eye(3));
                fwd.sol_grad.data *= t_matKron;
                fwd.sol_grad.ncol = 3 * fwd.nsource;
            }
            Log::info("[done]");
        }
    } else if (options.surf_ori) {
        fwd.surf_ori = false;
        fwd.convert_to_surf_ori();
    } else {
        Log::info("\tCartesian source orientations...");
        nuse = 0;
        fwd.source_rr = MatrixXf::Zero(fwd.nsource, 3);
        for (std::int32_t k = 0; k < t_SourceSpace.size(); ++k) {
            for (std::int32_t q = 0; q < t_SourceSpace[k].nuse; ++q)
                fwd.source_rr.block(q + nuse, 0, 1, 3) = t_SourceSpace[k].rr.block(t_SourceSpace[k].vertno(q), 0, 1, 3);

            nuse += t_SourceSpace[k].nuse;
        }

        MatrixXf t_ones = MatrixXf::Ones(fwd.nsource, 1);
        Matrix3f t_eye = Matrix3f::Identity();
        fwd.source_nn = kroneckerProduct(t_ones, t_eye);

        Log::info("[done]");
    }

    //
    //   Do the channel selection
    //
    StringList exclude_bads = options.exclude;
    if (bads.size() > 0) {
        for (std::int32_t k = 0; k < static_cast<int>(bads.size()); ++k)
            if (std::none_of(exclude_bads.begin(), exclude_bads.end(), [&](const std::string& e) { return mne::iequals(e, bads[k]); }))
                exclude_bads.push_back(bads[k]);
    }

    fwd.surf_ori = options.surf_ori;
    return fwd.pick_channels(options.include, exclude_bads);
}

//=============================================================================================================

bool MNEForwardSolution::read_one(FiffStream& stream,
                                  const std::shared_ptr<FiffDirNode>& p_Node,
                                  MNEForwardSolution& one)
{
    //
    //   Read all interesting stuff for one forward solution
    //
    if (!p_Node)
        return false;

    one.clear();
    std::optional<FiffTag> t_pTag;

    if (t_pTag = p_Node->find_tag(stream, FIFF_MNE_SOURCE_ORIENTATION); !t_pTag) {
        stream.close();
        Log::warning("Source orientation tag not found.");
        return false;
    }

    one.source_ori = *t_pTag->to_int();

    if (t_pTag = p_Node->find_tag(stream, FIFF_MNE_COORD_FRAME); !t_pTag) {
        stream.close();
        Log::warning("Coordinate frame tag not found.");
        return false;
    }

    one.coord_frame = *t_pTag->to_int();

    if (t_pTag = p_Node->find_tag(stream, FIFF_MNE_SOURCE_SPACE_NPOINTS); !t_pTag) {
        stream.close();
        Log::warning("Number of sources not found.");
        return false;
    }

    one.nsource = *t_pTag->to_int();

    if (t_pTag = p_Node->find_tag(stream, FIFF_NCHAN); !t_pTag) {
        stream.close();
        Log::warning("Number of channels not found.");
        return false;
    }

    one.nchan = *t_pTag->to_int();

    if (stream.read_named_matrix(p_Node, FIFF_MNE_FORWARD_SOLUTION, one.sol))
        one.sol.transpose_named_matrix();
    else {
        stream.close();
        Log::warning("Forward solution data not found.");
        //error(me,'Forward solution data not found (%s)',mne_omit_first_line(lasterr));
        return false;
    }

    if (stream.read_named_matrix(p_Node, FIFF_MNE_FORWARD_SOLUTION_GRAD, one.sol_grad))
        one.sol_grad.transpose_named_matrix();
    else
        one.sol_grad.clear();

    if (one.sol.data.rows() != one.nchan ||
        (one.sol.data.cols() != one.nsource && one.sol.data.cols() != 3 * one.nsource)) {
        stream.close();
        Log::warning("Forward solution matrix has wrong dimensions.");
        //error(me,'Forward solution matrix has wrong dimensions');
        return false;
    }
    if (!one.sol_grad.empty()) {
        if (one.sol_grad.data.rows() != one.nchan ||
            (one.sol_grad.data.cols() != 3 * one.nsource && one.sol_grad.data.cols() != 3 * 3 * one.nsource)) {
            stream.close();
            Log::warning("Forward solution gradient matrix has wrong dimensions.");
            //error(me,'Forward solution gradient matrix has wrong dimensions');
        }
    }
    return true;
}

//=============================================================================================================

void MNEForwardSolution::restrict_gain_matrix(MatrixXd& G, const FiffInfo& info)
{
    // Figure out which ones have been used
    if (std::ssize(info.chs) != G.rows()) {
        Log::warning("Error G.rows() and length of info.chs do not match: {} != {}", G.rows(), info.chs.size());
        return;
    }

    RowVectorXi sel = info.pick_types(std::string("grad"));
    if (sel.size() > 0) {
        for (std::int32_t i = 0; i < sel.size(); ++i)
            G.row(i) = G.row(sel[i]);
        G.conservativeResize(sel.size(), G.cols());
        Log::info("\t{} planar channels", sel.size());
    } else {
        sel = info.pick_types(std::string("mag"));
        if (sel.size() > 0) {
            for (std::int32_t i = 0; i < sel.size(); ++i)
                G.row(i) = G.row(sel[i]);
            G.conservativeResize(sel.size(), G.cols());
            Log::info("\t{} magnetometer or axial gradiometer channels", sel.size());
        } else {
            sel = info.pick_types(false, true);
            if (sel.size() > 0) {
                for (std::int32_t i = 0; i < sel.size(); ++i)
                    G.row(i) = G.row(sel[i]);
                G.conservativeResize(sel.size(), G.cols());
                Log::info("\t{} EEG channels", sel.size());
            } else
                Log::warning("Could not find MEG or EEG channels");
        }
    }
}

//=============================================================================================================

void MNEForwardSolution::convert_to_surf_ori()
{
    if (this->surf_ori || this->isFixedOrient())
        return;
    std::int32_t nuse = 0;
    //
    //   Rotate the local source coordinate systems
    //
    Log::info("\tConverting to surface-based source orientations...");

    bool use_ave_nn = false;
    auto* hemi0 = src.hemisphereAt(0);
    if (hemi0 && hemi0->patch_inds.size() > 0) {
        use_ave_nn = true;
        Log::info("\tAverage patch normals will be employed in the rotation to the local surface coordinates...");
    }

    std::int32_t pp = 0;
    this->source_rr = MatrixXf::Zero(this->nsource, 3);
    this->source_nn = MatrixXf::Zero(this->nsource * 3, 3);

    // A clustered source sits at its centroid; its vertno holds the label id
    const auto vertexOf = [this](std::int32_t k, std::int32_t q) {
        auto* hemi = src.hemisphereAt(k);
        return hemi && hemi->isClustered() ? hemi->cluster_info.centroidVertno[q] : src[k].vertno(q);
    };

    for (std::int32_t k = 0; k < src.size(); ++k) {
        for (std::int32_t q = 0; q < src[k].nuse; ++q)
            this->source_rr.block(q + nuse, 0, 1, 3) = src[k].rr.block(vertexOf(k, q), 0, 1, 3);

        for (std::int32_t p = 0; p < src[k].nuse; ++p) {
            //
            //  Project out the surface normal and compute SVD
            //
            Vector3f nn;
            if (use_ave_nn) {
                auto* hemiK = src.hemisphereAt(k);
                VectorXi t_vIdx = hemiK->pinfo[hemiK->patch_inds[p]];
                Matrix3Xf t_nn(3, t_vIdx.size());
                for (std::int32_t i = 0; i < t_vIdx.size(); ++i)
                    t_nn.col(i) = src[k].nn.block(t_vIdx[i], 0, 1, 3).transpose();
                nn = t_nn.rowwise().sum();
                nn.array() /= nn.norm();
            } else
                nn = src[k].nn.block(vertexOf(k, p), 0, 1, 3).transpose();

            Matrix3f tmp = Matrix3f::Identity(nn.rows(), nn.rows()) - nn * nn.transpose();

            JacobiSVD<MatrixXf> t_svd(tmp, Eigen::ComputeThinU);
            //Sort singular values and singular vectors
            VectorXf t_s = t_svd.singularValues();
            MatrixXf U = t_svd.matrixU();
            Linalg::sort<float>(t_s, U);

            //
            //  Make sure that ez is in the direction of nn
            //
            if ((nn.transpose() * U.block(0, 2, 3, 1))(0, 0) < 0)
                U *= -1;
            this->source_nn.block(pp, 0, 3, 3) = U.transpose();
            pp += 3;
        }
        nuse += src[k].nuse;
    }
    MatrixXd tmp = this->source_nn.transpose().cast<double>();
    SparseMatrix<double> surf_rot = Linalg::make_block_diag(tmp, 3);

    this->sol.data *= surf_rot;

    if (!this->sol_grad.empty()) {
        SparseMatrix<double> t_matKron;
        SparseMatrix<double> t_eye(3, 3);
        for (std::int32_t i = 0; i < 3; ++i)
            t_eye.insert(i, i) = 1.0f;
        t_matKron = kroneckerProduct(surf_rot, t_eye); //kron(surf_rot,eye(3));
        this->sol_grad.data *= t_matKron;
    }
    this->surf_ori = true;
    Log::info("[done]");
}

//=============================================================================================================

void MNEForwardSolution::to_fixed_ori()
{
    if (!this->surf_ori || this->isFixedOrient()) {
        Log::warning("Cannot convert to fixed orientation: requires surface-oriented, free-orientation forward solution");
        return;
    }
    std::int32_t count = 0;
    for (std::int32_t i = 2; i < this->sol.data.cols(); i += 3) {
        this->sol.data.col(count) = this->sol.data.col(i);
        if (this->source_nn.rows() == this->sol.data.cols())
            this->source_nn.row(count) = this->source_nn.row(i);
        ++count;
    }
    this->sol.data.conservativeResize(this->sol.data.rows(), count);
    if (this->source_nn.rows() == 3 * count)
        this->source_nn.conservativeResize(count, Eigen::NoChange);
    this->sol.ncol = this->sol.ncol / 3;
    this->source_ori = FIFFV_MNE_FIXED_ORI;
    Log::info("\tConverted the forward solution into the fixed-orientation mode.");
}

//=============================================================================================================

bool MNEForwardSolution::isClustered() const
{
    auto* hemi = src.hemisphereAt(0);
    return hemi && hemi->isClustered();
}

//=============================================================================================================

MatrixX3f MNEForwardSolution::getSourcePositionsByLabel(const std::vector<FsLabel>& lPickedLabels, const FsSurfaceSet& tSurfSetInflated)
{
    MatrixX3f matSourceVertLeft, matSourceVertRight, matSourcePositions;

    if (lPickedLabels.empty()) {
        Log::warning("MNEForwardSolution::getSourcePositionsByLabel - picked label list is empty. Returning.");
        return matSourcePositions;
    }

    if (tSurfSetInflated.empty()) {
        Log::warning("MNEForwardSolution::getSourcePositionsByLabel - tSurfSetInflated is empty. Returning.");
        return matSourcePositions;
    }

    if (isClustered()) {
        for (int j = 0; j < this->src[0].vertno.rows(); ++j) {
            for (int k = 0; k < std::ssize(lPickedLabels); k++) {
                if (this->src[0].vertno(j) == lPickedLabels.at(k).label_id) {
                    matSourceVertLeft.conservativeResize(matSourceVertLeft.rows() + 1, 3);
                    matSourceVertLeft.row(matSourceVertLeft.rows() - 1) = tSurfSetInflated[0].rr().row(this->src.hemisphereAt(0)->cluster_info.centroidVertno.at(j)) - tSurfSetInflated[0].offset().transpose();
                    break;
                }
            }
        }

        for (int j = 0; j < this->src[1].vertno.rows(); ++j) {
            for (int k = 0; k < std::ssize(lPickedLabels); k++) {
                if (this->src[1].vertno(j) == lPickedLabels.at(k).label_id) {
                    matSourceVertRight.conservativeResize(matSourceVertRight.rows() + 1, 3);
                    matSourceVertRight.row(matSourceVertRight.rows() - 1) = tSurfSetInflated[1].rr().row(this->src.hemisphereAt(1)->cluster_info.centroidVertno.at(j)) - tSurfSetInflated[1].offset().transpose();
                    break;
                }
            }
        }
    } else {
        for (int j = 0; j < this->src[0].vertno.rows(); ++j) {
            for (int k = 0; k < std::ssize(lPickedLabels); k++) {
                for (int l = 0; l < lPickedLabels.at(k).vertices.rows(); l++) {
                    if (this->src[0].vertno(j) == lPickedLabels.at(k).vertices(l) && lPickedLabels.at(k).hemi == 0) {
                        matSourceVertLeft.conservativeResize(matSourceVertLeft.rows() + 1, 3);
                        matSourceVertLeft.row(matSourceVertLeft.rows() - 1) = tSurfSetInflated[0].rr().row(this->src[0].vertno(j)) - tSurfSetInflated[0].offset().transpose();
                        break;
                    }
                }
            }
        }

        for (int j = 0; j < this->src[1].vertno.rows(); ++j) {
            for (int k = 0; k < std::ssize(lPickedLabels); k++) {
                for (int l = 0; l < lPickedLabels.at(k).vertices.rows(); l++) {
                    if (this->src[1].vertno(j) == lPickedLabels.at(k).vertices(l) && lPickedLabels.at(k).hemi == 1) {
                        matSourceVertRight.conservativeResize(matSourceVertRight.rows() + 1, 3);
                        matSourceVertRight.row(matSourceVertRight.rows() - 1) = tSurfSetInflated[1].rr().row(this->src[1].vertno(j)) - tSurfSetInflated[1].offset().transpose();
                        break;
                    }
                }
            }
        }
    }

    matSourcePositions.resize(matSourceVertLeft.rows() + matSourceVertRight.rows(), 3);
    matSourcePositions << matSourceVertLeft, matSourceVertRight;

    return matSourcePositions;
}
