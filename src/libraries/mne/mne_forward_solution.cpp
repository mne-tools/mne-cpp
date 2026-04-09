//=============================================================================================================
/**
 * @file     mne_forward_solution.cpp
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
 * @brief    MNEForwardSolution class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_forward_solution.h"

#include <fiff/fiff.h>
#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SVD>
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <unsupported/Eigen/KroneckerProduct>

//=============================================================================================================

#include <fs/fs_colortable.h>
#include <fs/fs_label.h>
#include <fs/fs_surfaceset.h>
#include <math/linalg.h>
#include <math/kmeans.h>

#include <algorithm>
#include <QtConcurrent>
#include <QFuture>
#include <QRegularExpression>

#include <stdexcept>
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FSLIB;
using namespace UTILSLIB;
using namespace Eigen;
using namespace FIFFLIB;

//=============================================================================================================

namespace {

bool check_matching_chnames_conventions(const QStringList& chNamesA, const QStringList& chNamesB, bool bCheckForNewNamingConvention = false)
{
    bool bMatching = false;

    if(chNamesA.isEmpty()) {
        qWarning("Warning in check_matching_chnames_conventions - chNamesA list is empty. Nothing to compare");
    }
    if(chNamesB.isEmpty()) {
        qWarning("Warning in check_matching_chnames_conventions - chNamesB list is empty. Nothing to compare");
    }

    QString replaceStringOldConv, replaceStringNewConv;

    for(int i = 0; i < chNamesA.size(); ++i) {
        if(chNamesB.contains(chNamesA.at(i))) {
            bMatching = true;
        } else if(bCheckForNewNamingConvention) {
            replaceStringNewConv = chNamesA.at(i);
            replaceStringNewConv.replace(" ","");

            if(chNamesB.contains(replaceStringNewConv)) {
                bMatching = true;
            } else {
                QRegularExpression xRegExp("[0-9]{1,100}");
                QRegularExpressionMatch match = xRegExp.match(chNamesA.at(i));
                QStringList xList = match.capturedTexts();

                for(int k = 0; k < xList.size(); ++k) {
                    replaceStringOldConv = chNamesA.at(i);
                    replaceStringOldConv.replace(xList.at(k),QString("%1%2").arg(" ").arg(xList.at(k)));

                    if(chNamesB.contains(replaceStringNewConv) || chNamesB.contains(replaceStringOldConv)) {
                        bMatching = true;
                    } else {
                        bMatching = false;
                    }
                }
            }
        }
    }

    return bMatching;
}

} // anonymous namespace

//=============================================================================================================
// CONSTANTS
//=============================================================================================================

constexpr int FAIL = -1;
constexpr int OK   =  0;

constexpr int X = 0;
constexpr int Y = 1;
constexpr int Z = 2;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEForwardSolution::MNEForwardSolution()
: source_ori(-1)
, surf_ori(false)
, coord_frame(-1)
, nsource(-1)
, nchan(-1)
, sol(new FiffNamedMatrix)
, sol_grad(new FiffNamedMatrix)
, source_rr(MatrixX3f::Zero(0,3))
, source_nn(MatrixX3f::Zero(0,3))
{
}

//=============================================================================================================

MNEForwardSolution::MNEForwardSolution(QIODevice &p_IODevice, bool force_fixed, bool surf_ori, const QStringList& include, const QStringList& exclude, bool bExcludeBads)
: source_ori(-1)
, surf_ori(surf_ori)
, coord_frame(-1)
, nsource(-1)
, nchan(-1)
, sol(new FiffNamedMatrix)
, sol_grad(new FiffNamedMatrix)
, source_rr(MatrixX3f::Zero(0,3))
, source_nn(MatrixX3f::Zero(0,3))
{
    if(!read(p_IODevice, *this, force_fixed, surf_ori, include, exclude, bExcludeBads))
    {
        qWarning("\tForward solution not found.");
        return;
    }
}

//=============================================================================================================

MNEForwardSolution::MNEForwardSolution(const MNEForwardSolution &p_MNEForwardSolution)
: info(p_MNEForwardSolution.info)
, source_ori(p_MNEForwardSolution.source_ori)
, surf_ori(p_MNEForwardSolution.surf_ori)
, coord_frame(p_MNEForwardSolution.coord_frame)
, nsource(p_MNEForwardSolution.nsource)
, nchan(p_MNEForwardSolution.nchan)
, sol(p_MNEForwardSolution.sol)
, sol_grad(p_MNEForwardSolution.sol_grad)
, mri_head_t(p_MNEForwardSolution.mri_head_t)
, mri_filename(p_MNEForwardSolution.mri_filename)
, mri_id(p_MNEForwardSolution.mri_id)
, src(p_MNEForwardSolution.src)
, source_rr(p_MNEForwardSolution.source_rr)
, source_nn(p_MNEForwardSolution.source_nn)
{
}

//=============================================================================================================

MNEForwardSolution& MNEForwardSolution::operator=(const MNEForwardSolution &other)
{
    if (this != &other) {
        info = other.info;
        source_ori = other.source_ori;
        surf_ori = other.surf_ori;
        coord_frame = other.coord_frame;
        nsource = other.nsource;
        nchan = other.nchan;
        sol = other.sol;
        sol_grad = other.sol_grad;
        mri_head_t = other.mri_head_t;
        mri_filename = other.mri_filename;
        mri_id = other.mri_id;
        src = other.src;
        source_rr = other.source_rr;
        source_nn = other.source_nn;
    }
    return *this;
}

//=============================================================================================================

MNEForwardSolution::~MNEForwardSolution()
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
    sol = FiffNamedMatrix::SDPtr(new FiffNamedMatrix());
    sol_grad = FiffNamedMatrix::SDPtr(new FiffNamedMatrix());
    mri_head_t.clear();
    mri_filename.clear();
    mri_id.clear();
    src.clear();
    source_rr = MatrixX3f(0,3);
    source_nn = MatrixX3f(0,3);
}

//=============================================================================================================

bool MNEForwardSolution::write(QIODevice& p_IODevice) const
{
    //
    //   Classify channels into MEG and EEG index sets
    //
    std::vector<int> megIdx, eegIdx;
    for (int k = 0; k < info.chs.size(); ++k) {
        fiff_int_t kind = info.chs[k].kind;
        if (kind == FIFFV_MEG_CH || kind == FIFFV_REF_MEG_CH)
            megIdx.push_back(k);
        else if (kind == FIFFV_EEG_CH)
            eegIdx.push_back(k);
    }
    int nmeg = megIdx.size();
    int neeg = eegIdx.size();

    //
    //   Compute the total number of active source vertices
    //
    int nvert = 0;
    for (int k = 0; k < src.size(); ++k)
        nvert += src[k].nuse;

    //
    //   Open the file, create the directory
    //
    FiffStream::SPtr t_pStream = FiffStream::start_file(p_IODevice);
    t_pStream->start_block(FIFFB_MNE);

    //
    //   Information from the MRI file
    //
    {
        t_pStream->start_block(FIFFB_MNE_PARENT_MRI_FILE);

        if (!mri_filename.isEmpty())
            t_pStream->write_string(FIFF_MNE_FILE_NAME, mri_filename);
        if (!mri_id.isEmpty())
            t_pStream->write_id(FIFF_PARENT_FILE_ID, mri_id);
        t_pStream->write_coord_trans(mri_head_t);

        t_pStream->end_block(FIFFB_MNE_PARENT_MRI_FILE);
    }

    //
    //   Information from the measurement file
    //
    {
        t_pStream->start_block(FIFFB_MNE_PARENT_MEAS_FILE);

        if (!info.filename.isEmpty())
            t_pStream->write_string(FIFF_MNE_FILE_NAME, info.filename);
        if (!info.meas_id.isEmpty())
            t_pStream->write_id(FIFF_PARENT_BLOCK_ID, info.meas_id);
        t_pStream->write_coord_trans(info.dev_head_t);

        int totalChan = nmeg + neeg;
        t_pStream->write_int(FIFF_NCHAN, &totalChan);

        //  Write channel infos with sequential scanNo
        QList<FiffChInfo> allChs;
        for (int k = 0; k < nmeg; ++k)
            allChs.append(info.chs[megIdx[k]]);
        for (int k = 0; k < neeg; ++k)
            allChs.append(info.chs[eegIdx[k]]);
        for (int p = 0; p < allChs.size(); ++p) {
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
        if (src[k].writeToStream(t_pStream, false) == FIFF_FAIL) {
            t_pStream->close();
            return false;
        }
    }

    //
    //   Extract sub-matrices for MEG and EEG from the combined sol
    //
    auto extractRows = [](const FiffNamedMatrix& combined,
                          const std::vector<int>& rowIdx) -> FiffNamedMatrix
    {
        int nRows = rowIdx.size();
        int nCols = combined.ncol;
        MatrixXd data(nRows, nCols);
        QStringList row_names;
        for (int r = 0; r < nRows; ++r) {
            data.row(r) = combined.data.row(rowIdx[r]);
            row_names.append(combined.row_names[rowIdx[r]]);
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

        FiffNamedMatrix megSol = extractRows(*sol.data(), megIdx);
        megSol.transpose_named_matrix();
        t_pStream->write_named_matrix(FIFF_MNE_FORWARD_SOLUTION, megSol);

        if (!sol_grad->isEmpty()) {
            FiffNamedMatrix megGrad = extractRows(*sol_grad.data(), megIdx);
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

        FiffNamedMatrix eegSol = extractRows(*sol.data(), eegIdx);
        eegSol.transpose_named_matrix();
        t_pStream->write_named_matrix(FIFF_MNE_FORWARD_SOLUTION, eegSol);

        if (!sol_grad->isEmpty()) {
            FiffNamedMatrix eegGrad = extractRows(*sol_grad.data(), eegIdx);
            eegGrad.transpose_named_matrix();
            t_pStream->write_named_matrix(FIFF_MNE_FORWARD_SOLUTION_GRAD, eegGrad);
        }
        t_pStream->end_block(FIFFB_MNE_FORWARD_SOLUTION);
    }

    t_pStream->end_block(FIFFB_MNE);
    t_pStream->end_file();
    t_pStream->close();
    t_pStream.clear();

    //
    //   Update the directory
    //
    if (auto* qf = dynamic_cast<QFile*>(&p_IODevice)) {
        QFile fileIn(qf->fileName());
        FiffStream::SPtr t_pStreamIn = FiffStream::open_update(fileIn);
        if (t_pStreamIn) {
            const auto& dir = t_pStreamIn->dir();
            for (int i = 0; i < dir.size(); ++i) {
                if (dir[i]->kind == FIFF_DIR_POINTER) {
                    fiff_int_t dirpos = (fiff_int_t)t_pStreamIn->write_dir_entries(dir);
                    if (dirpos >= 0)
                        t_pStreamIn->write_dir_pointer(dirpos, dir[i]->pos);
                    break;
                }
            }
            t_pStreamIn->close();
        }
    }

    return true;
}

//=============================================================================================================

MNEForwardSolution MNEForwardSolution::cluster_forward_solution(const FsAnnotationSet &p_AnnotationSet,
                                                                qint32 p_iClusterSize,
                                                                MatrixXd& p_D,
                                                                const FiffCov &p_pNoise_cov,
                                                                const FiffInfo &p_pInfo,
                                                                QString p_sMethod) const
{
    qInfo("Cluster forward solution using %s.", p_sMethod.toUtf8().constData());

    MNEForwardSolution p_fwdOut = MNEForwardSolution(*this);

    //Check if cov naming conventions are matching
    if(!check_matching_chnames_conventions(p_pNoise_cov.names, p_pInfo.ch_names) && !p_pNoise_cov.names.isEmpty() && !p_pInfo.ch_names.isEmpty()) {
        if(check_matching_chnames_conventions(p_pNoise_cov.names, p_pInfo.ch_names, true)) {
            qWarning("MNEForwardSolution::cluster_forward_solution - Cov names do match with info channel names but have a different naming convention.");
            //return p_fwdOut;
        } else {
            qWarning("MNEForwardSolution::cluster_forward_solution - Cov channel names do not match with info channel names.");
            //return p_fwdOut;
        }
    }

    //
    // Check consisty
    //
    if(this->isFixedOrient())
    {
        qWarning("Error: Fixed orientation not implemented yet!");
        return p_fwdOut;
    }

    MatrixXd t_G_Whitened(0,0);
    bool t_bUseWhitened = false;
    //
    //Whiten gain matrix before clustering -> cause diffenerent units Magnetometer, Gradiometer and EEG
    //
    if(!p_pNoise_cov.isEmpty() && !p_pInfo.isEmpty())
    {
        FiffInfo p_outFwdInfo;
        FiffCov p_outNoiseCov;
        MatrixXd p_outWhitener;
        qint32 p_outNumNonZero;
        //do whitening with noise cov
        this->prepare_forward(p_pInfo, p_pNoise_cov, false, p_outFwdInfo, t_G_Whitened, p_outNoiseCov, p_outWhitener, p_outNumNonZero);
        qInfo("\tWhitening the forward solution.");

        t_G_Whitened = p_outWhitener*t_G_Whitened;
        t_bUseWhitened = true;
    }

    //
    // Assemble input data
    //
    qint32 count;
    qint32 offset;

    MatrixXd t_G_new;

    for(qint32 h = 0; h < this->src.size(); ++h )
    {

        count = 0;
        offset = 0;

        // Offset for continuous indexing;
        if(h > 0)
            for(qint32 j = 0; j < h; ++j)
                offset += this->src[j].nuse;

        if(h == 0)
            qInfo("Cluster Left Hemisphere");
        else
            qInfo("Cluster Right Hemisphere");

        const FsAnnotation annotation = p_AnnotationSet[h];
        FsColortable t_CurrentColorTable = annotation.getColortable();
        VectorXi label_ids = t_CurrentColorTable.getLabelIds();

        // Get label ids for every vertex
        VectorXi vertno_labeled = VectorXi::Zero(this->src[h].vertno.rows());

        //ToDo make this more universal -> using FsLabel instead of annotations - obsolete when using Labels
        for(qint32 i = 0; i < vertno_labeled.rows(); ++i)
            vertno_labeled[i] = p_AnnotationSet[h].getLabelIds()[this->src[h].vertno[i]];

        std::vector<RegionData> regionDataIn;

        //
        // Generate cluster input data
        //
        for (qint32 i = 0; i < label_ids.rows(); ++i)
        {
            if (label_ids[i] != 0)
            {
                QString curr_name = t_CurrentColorTable.struct_names[i];//obj.label2AtlasName(label(i));
                qInfo("\tCluster %d / %ld %s...", i+1, label_ids.rows(), curr_name.toUtf8().constData());

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

                //get selected G
                MatrixXd t_G(this->sol->data.rows(), idcs.rows()*3);
                MatrixXd t_G_Whitened_Roi(t_G_Whitened.rows(), idcs.rows()*3);

                for(qint32 j = 0; j < idcs.rows(); ++j)
                {
                    t_G.block(0, j*3, t_G.rows(), 3) = this->sol->data.block(0, (idcs[j]+offset)*3, t_G.rows(), 3);
                    if(t_bUseWhitened)
                        t_G_Whitened_Roi.block(0, j*3, t_G_Whitened_Roi.rows(), 3) = t_G_Whitened.block(0, (idcs[j]+offset)*3, t_G_Whitened_Roi.rows(), 3);
                }

                qint32 nSens = t_G.rows();
                qint32 nSources = t_G.cols()/3;

                if (nSources > 0)
                {
                    RegionData t_sensG;

                    t_sensG.idcs = idcs;
                    t_sensG.iLabelIdxIn = i;
                    t_sensG.nClusters = static_cast<int>(ceil(static_cast<double>(nSources) / static_cast<double>(p_iClusterSize)));

                    t_sensG.matRoiGOrig = t_G;

                    qInfo("%d Cluster(s)...", t_sensG.nClusters);

                    // Reshape Input data -> sources rows; sensors columns
                    t_sensG.matRoiG = MatrixXd(t_G.cols()/3, 3*nSens);
                    if(t_bUseWhitened)
                        t_sensG.matRoiGWhitened = MatrixXd(t_G_Whitened_Roi.cols()/3, 3*nSens);

                    for(qint32 j = 0; j < nSens; ++j)
                    {
                        for(qint32 k = 0; k < t_sensG.matRoiG.rows(); ++k)
                            t_sensG.matRoiG.block(k,j*3,1,3) = t_G.block(j,k*3,1,3);
                        if(t_bUseWhitened)
                            for(qint32 k = 0; k < t_sensG.matRoiGWhitened.rows(); ++k)
                                t_sensG.matRoiGWhitened.block(k,j*3,1,3) = t_G_Whitened_Roi.block(j,k*3,1,3);
                    }

                    t_sensG.bUseWhitened = t_bUseWhitened;

                    t_sensG.sDistMeasure = p_sMethod;

                    regionDataIn.push_back(std::move(t_sensG));

                    qInfo("[added]");
                }
                else
                {
                    qWarning("failed! FsLabel contains no sources.");
                }
            }
        }

        //
        // Calculate clusters
        //
        qInfo("Clustering...");
        QFuture< RegionDataOut > res;
        res = QtConcurrent::mapped(regionDataIn, &RegionData::cluster);
        res.waitForFinished();

        //
        // Assign results
        //
        MatrixXd t_G_partial;

        qint32 nClusters;
        qint32 nSens;
        auto itIn = regionDataIn.cbegin();
        QFuture<RegionDataOut>::const_iterator itOut;
        for (itOut = res.constBegin(); itOut != res.constEnd(); ++itOut)
        {
            nClusters = itOut->ctrs.rows();
            nSens = itOut->ctrs.cols()/3;
            t_G_partial = MatrixXd::Zero(nSens, nClusters*3);

            //
            // Assign the centroid for each cluster to the partial G
            //
            //ToDo change this use indeces found with whitened data
            for(qint32 j = 0; j < nSens; ++j)
                for(qint32 k = 0; k < nClusters; ++k)
                    t_G_partial.block(j, k*3, 1, 3) = itOut->ctrs.block(k,j*3,1,3);

            //
            // Get cluster indizes and its distances to the centroid
            //
            for(qint32 j = 0; j < nClusters; ++j)
            {
                VectorXi clusterIdcs = VectorXi::Zero(itOut->roiIdx.rows());
                VectorXd clusterDistance = VectorXd::Zero(itOut->roiIdx.rows());
                MatrixX3f clusterSource_rr = MatrixX3f::Zero(itOut->roiIdx.rows(), 3);
                qint32 nClusterIdcs = 0;
                for(qint32 k = 0; k < itOut->roiIdx.rows(); ++k)
                {
                    if(itOut->roiIdx[k] == j)
                    {
                        clusterIdcs[nClusterIdcs] = itIn->idcs[k];

                        qint32 offset = h == 0 ? 0 : this->src[0].nuse;
                        clusterSource_rr.row(nClusterIdcs) = this->source_rr.row(offset + itIn->idcs[k]);
                        clusterDistance[nClusterIdcs] = itOut->D(k,j);
                        ++nClusterIdcs;
                    }
                }
                clusterIdcs.conservativeResize(nClusterIdcs);
                clusterSource_rr.conservativeResize(nClusterIdcs,3);
                clusterDistance.conservativeResize(nClusterIdcs);

                VectorXi clusterVertnos = VectorXi::Zero(clusterIdcs.size());
                for(qint32 k = 0; k < clusterVertnos.size(); ++k)
                    clusterVertnos(k) = this->src[h].vertno[clusterIdcs(k)];

                p_fwdOut.src.hemisphereAt(h)->cluster_info.clusterVertnos.append(clusterVertnos);
                p_fwdOut.src.hemisphereAt(h)->cluster_info.clusterSource_rr.append(clusterSource_rr);
                p_fwdOut.src.hemisphereAt(h)->cluster_info.clusterDistances.append(clusterDistance);
                p_fwdOut.src.hemisphereAt(h)->cluster_info.clusterLabelIds.append(label_ids[itOut->iLabelIdxOut]);
                p_fwdOut.src.hemisphereAt(h)->cluster_info.clusterLabelNames.append(t_CurrentColorTable.getNames()[itOut->iLabelIdxOut]);
            }

            //
            // Assign partial G to new LeadField
            //
            if(t_G_partial.rows() > 0 && t_G_partial.cols() > 0)
            {
                t_G_new.conservativeResize(t_G_partial.rows(), t_G_new.cols() + t_G_partial.cols());
                t_G_new.block(0, t_G_new.cols() - t_G_partial.cols(), t_G_new.rows(), t_G_partial.cols()) = t_G_partial;

                // Map the centroids to the closest rr
                for(qint32 k = 0; k < nClusters; ++k)
                {
                    qint32 j = 0;

                    double sqec = sqrt((itIn->matRoiGOrig.block(0, j*3, itIn->matRoiGOrig.rows(), 3) - t_G_partial.block(0, k*3, t_G_partial.rows(), 3)).array().pow(2).sum());
                    double sqec_min = sqec;
                    qint32 j_min = 0;
                    for(qint32 j = 1; j < itIn->idcs.rows(); ++j)
                    {
                        sqec = sqrt((itIn->matRoiGOrig.block(0, j*3, itIn->matRoiGOrig.rows(), 3) - t_G_partial.block(0, k*3, t_G_partial.rows(), 3)).array().pow(2).sum());

                        if(sqec < sqec_min)
                        {
                            sqec_min = sqec;
                            j_min = j;
                        }
                    }

                    // Take the closest coordinates
                    qint32 sel_idx = itIn->idcs[j_min];

                    p_fwdOut.src.hemisphereAt(h)->cluster_info.centroidVertno.append(this->src[h].vertno[sel_idx]);
                    p_fwdOut.src.hemisphereAt(h)->cluster_info.centroidSource_rr.append(this->src[h].rr.row(this->src[h].vertno[sel_idx]));
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

        qInfo("[done]");
    }

    //
    // Cluster operator D (sources x clusters)
    //
    qint32 totalNumOfClust = 0;
    for (qint32 h = 0; h < 2; ++h)
        totalNumOfClust += p_fwdOut.src.hemisphereAt(h)->cluster_info.clusterVertnos.size();

    if(this->isFixedOrient())
        p_D = MatrixXd::Zero(this->sol->data.cols(), totalNumOfClust);
    else
        p_D = MatrixXd::Zero(this->sol->data.cols(), totalNumOfClust*3);

    QList<VectorXi> t_vertnos = this->src.get_vertno();

    qint32 currentCluster = 0;
    for (qint32 h = 0; h < 2; ++h)
    {
        int hemiOffset = h == 0 ? 0 : t_vertnos[0].size();
        for(qint32 i = 0; i < p_fwdOut.src.hemisphereAt(h)->cluster_info.clusterVertnos.size(); ++i)
        {
            VectorXi idx_sel;
            Linalg::intersect(t_vertnos[h], p_fwdOut.src.hemisphereAt(h)->cluster_info.clusterVertnos[i], idx_sel);

            idx_sel.array() += hemiOffset;

            double selectWeight = 1.0/idx_sel.size();
            if(this->isFixedOrient())
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
    p_fwdOut.sol->data = t_G_new;
    p_fwdOut.sol->ncol = t_G_new.cols();

    p_fwdOut.nsource = p_fwdOut.sol->ncol/3;

    return p_fwdOut;
}

//=============================================================================================================

MNEForwardSolution MNEForwardSolution::reduce_forward_solution(qint32 p_iNumDipoles, MatrixXd& p_D) const
{
    MNEForwardSolution p_fwdOut = MNEForwardSolution(*this);

    bool isFixed = p_fwdOut.isFixedOrient();
    qint32 np = isFixed ? p_fwdOut.sol->data.cols() : p_fwdOut.sol->data.cols()/3;

    if(p_iNumDipoles > np)
        return p_fwdOut;

    VectorXi sel(p_iNumDipoles);

    float t_fStep = static_cast<float>(np) / static_cast<float>(p_iNumDipoles);

    for(qint32 i = 0; i < p_iNumDipoles; ++i)
    {
        float t_fCurrent = static_cast<float>(i) * t_fStep;
        sel[i] = (quint32)floor(t_fCurrent);
    }

    if(isFixed)
    {
        p_D = MatrixXd::Zero(p_fwdOut.sol->data.cols(), p_iNumDipoles);
        for(qint32 i = 0; i < p_iNumDipoles; ++i)
            p_D(sel[i], i) = 1;
    }
    else
    {
        p_D = MatrixXd::Zero(p_fwdOut.sol->data.cols(), p_iNumDipoles*3);
        for(qint32 i = 0; i < p_iNumDipoles; ++i)
            for(qint32 j = 0; j < 3; ++j)
                p_D((sel[i]*3)+j, (i*3)+j) = 1;
    }

    // New gain matrix
    p_fwdOut.sol->data = this->sol->data * p_D;

    MatrixX3f rr(p_iNumDipoles,3);

    MatrixX3f nn(p_iNumDipoles,3);

    for(qint32 i = 0; i < p_iNumDipoles; ++i)
    {
        rr.row(i) = this->source_rr.row(sel(i));
        nn.row(i) = this->source_nn.row(sel(i));
    }

    p_fwdOut.source_rr = rr;
    p_fwdOut.source_nn = nn;

    p_fwdOut.sol->ncol =  p_fwdOut.sol->data.cols();

    p_fwdOut.nsource = p_iNumDipoles;

    return p_fwdOut;
}

//=============================================================================================================

FiffCov MNEForwardSolution::compute_depth_prior(const MatrixXd &Gain, const FiffInfo &gain_info, bool is_fixed_ori, double exp, double limit, const MatrixXd &patch_areas, bool limit_depth_chs)
{
    qInfo("\tCreating the depth weighting matrix...");

    MatrixXd G(Gain);
    // If possible, pick best depth-weighting channels
    if(limit_depth_chs)
        MNEForwardSolution::restrict_gain_matrix(G, gain_info);

    VectorXd d;
    // Compute the gain matrix
    if(is_fixed_ori)
    {
        d = (G.array().square()).rowwise().sum();
    }
    else
    {
        qint32 n_pos = G.cols() / 3;
        d = VectorXd::Zero(n_pos);
        MatrixXd Gk;
        for (qint32 k = 0; k < n_pos; ++k)
        {
            Gk = G.block(0,3*k, G.rows(), 3);
            JacobiSVD<MatrixXd> svd(Gk.transpose()*Gk);
            d[k] = svd.singularValues().maxCoeff();
        }
    }

    // ToDo Currently the fwd solns never have "patch_areas" defined
    if(patch_areas.cols() > 0)
    {
        qWarning("\tToDo!!!!! >>> Patch areas taken into account in the depth weighting");
    }

    qint32 n_limit;
    VectorXd w = d.cwiseInverse();
    VectorXd ws = w;
    VectorXd wpp;
    Linalg::sort<double>(ws, false);
    double weight_limit = pow(limit, 2);
    if (!limit_depth_chs)
    {
        // match old mne-python behavor
        qint32 ind = 0;
        ws.minCoeff(&ind);
        n_limit = ind;
        limit = ws[ind] * weight_limit;
    }
    else
    {
        // match C code behavior
        limit = ws[ws.size()-1];
        qint32 ind = 0;
        n_limit = d.size();
        if (ws[ws.size()-1] > weight_limit * ws[0])
        {
            double th = weight_limit * ws[0];
            for(qint32 i = 0; i < ws.size(); ++i)
            {
                if(ws[i] > th)
                {
                    ind = i;
                    break;
                }
            }
            limit = ws[ind];
            n_limit = ind;
        }
    }

    qInfo("\tlimit = %d/%ld = %f", n_limit + 1, d.size(), sqrt(limit / ws[0]));
    double scale = 1.0 / limit;
    qInfo("\tscale = %g exp = %g", scale, exp);

    VectorXd t_w = w.array() / limit;
    for(qint32 i = 0; i < t_w.size(); ++i)
        t_w[i] = t_w[i] > 1 ? 1 : t_w[i];
    wpp = t_w.array().pow(exp);

    FiffCov depth_prior;
    if(is_fixed_ori)
        depth_prior.data = wpp;
    else
    {
        depth_prior.data.resize(wpp.rows()*3, 1);
        qint32 idx = 0;
        double v;
        for(qint32 i = 0; i < wpp.rows(); ++i)
        {
            idx = i*3;
            v = wpp[i];
            depth_prior.data(idx, 0) = v;
            depth_prior.data(idx+1, 0) = v;
            depth_prior.data(idx+2, 0) = v;
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
    qint32 n_sources = this->sol->data.cols();

    if (0 <= loose && loose <= 1)
    {
        qDebug() << "this->surf_ori" << this->surf_ori;
        if(loose < 1 && !this->surf_ori)
        {
            qWarning("\tForward operator is not oriented in surface coordinates. loose parameter should be None not %f.", loose);
            loose = 1;
            qInfo("\tSetting loose to %f.", loose);
        }

        if(is_fixed_ori)
        {
            qInfo("\tIgnoring loose parameter with forward operator with fixed orientation.");
            loose = 0.0;
        }
    }
    else
    {
        if(loose < 0 || loose > 1)
        {
            qWarning("Warning: Loose value should be in interval [0,1] not %f.\n", loose);
            loose = loose > 1 ? 1 : 0;
            qInfo("Setting loose to %f.", loose);
        }
    }

    FiffCov orient_prior;
    orient_prior.data = VectorXd::Ones(n_sources);
    if(!is_fixed_ori && (0 <= loose && loose <= 1))
    {
        qInfo("\tApplying loose dipole orientations. Loose value of %f.", loose);
        for(qint32 i = 0; i < n_sources; i+=3)
            orient_prior.data.block(i,0,2,1).array() *= loose;

        orient_prior.kind = FIFFV_MNE_ORIENT_PRIOR_COV;
        orient_prior.diag = true;
        orient_prior.dim = orient_prior.data.size();
        orient_prior.nfree = 1;
    }
    return orient_prior;
}

//=============================================================================================================

MNEForwardSolution MNEForwardSolution::pick_channels(const QStringList& include,
                                                     const QStringList& exclude) const
{
    MNEForwardSolution fwd(*this);

    if(include.size() == 0 && exclude.size() == 0)
        return fwd;

    RowVectorXi sel = FiffInfo::pick_channels(fwd.sol->row_names, include, exclude);

    // Do we have something?
    quint32 nuse = sel.size();

    if (nuse == 0)
    {
        qInfo("Nothing remains after picking. Returning original forward solution.");
        return fwd;
    }
    qInfo("\t%d out of %d channels remain after picking", nuse, fwd.nchan);

    //   Pick the correct rows of the forward operator
    MatrixXd newData(nuse, fwd.sol->data.cols());
    for(quint32 i = 0; i < nuse; ++i)
        newData.row(i) = fwd.sol->data.row(sel[i]);

    fwd.sol->data = newData;
    fwd.sol->nrow = nuse;

    QStringList ch_names;
    for(qint32 i = 0; i < sel.cols(); ++i)
        ch_names << fwd.sol->row_names[sel(i)];
    fwd.nchan = nuse;
    fwd.sol->row_names = ch_names;

    QList<FiffChInfo> chs;
    for(qint32 i = 0; i < sel.cols(); ++i)
        chs.append(fwd.info.chs[sel(i)]);
    fwd.info.chs = chs;
    fwd.info.nchan = nuse;

    QStringList bads;
    for(qint32 i = 0; i < fwd.info.bads.size(); ++i)
        if(ch_names.contains(fwd.info.bads[i]))
            bads.append(fwd.info.bads[i]);
    fwd.info.bads = bads;

    if(!fwd.sol_grad->isEmpty())
    {
        newData.resize(nuse, fwd.sol_grad->data.cols());
        for(quint32 i = 0; i < nuse; ++i)
            newData.row(i) = fwd.sol_grad->data.row(sel[i]);
        fwd.sol_grad->data = newData;
        fwd.sol_grad->nrow = nuse;
        QStringList row_names;
        for(qint32 i = 0; i < sel.cols(); ++i)
            row_names << fwd.sol_grad->row_names[sel(i)];
        fwd.sol_grad->row_names = row_names;
    }

    return fwd;
}

//=============================================================================================================

MNEForwardSolution MNEForwardSolution::pick_regions(const QList<FsLabel> &p_qListLabels) const
{
    VectorXi selVertices;

    qint32 iSize = 0;
    for(qint32 i = 0; i < p_qListLabels.size(); ++i)
    {
        VectorXi currentSelection;
        this->src.label_src_vertno_sel(p_qListLabels[i], currentSelection);

        selVertices.conservativeResize(iSize+currentSelection.size());
        selVertices.block(iSize,0,currentSelection.size(),1) = currentSelection;
        iSize = selVertices.size();
    }

Linalg::sort(selVertices, false);

    MNEForwardSolution selectedFwd(*this);

    MatrixX3f rr(selVertices.size(),3);
    MatrixX3f nn(selVertices.size(),3);

    for(qint32 i = 0; i < selVertices.size(); ++i)
    {
        rr.block(i, 0, 1, 3) = selectedFwd.source_rr.row(selVertices[i]);
        nn.block(i, 0, 1, 3) = selectedFwd.source_nn.row(selVertices[i]);
    }

    selectedFwd.source_rr = rr;
    selectedFwd.source_nn = nn;

    VectorXi selSolIdcs = tripletSelection(selVertices);
    MatrixXd G(selectedFwd.sol->data.rows(),selSolIdcs.size());
    qint32 rows = G.rows();

    for(qint32 i = 0; i < selSolIdcs.size(); ++i)
        G.block(0, i, rows, 1) = selectedFwd.sol->data.col(selSolIdcs[i]);

    selectedFwd.sol->data = G;
    selectedFwd.sol->nrow = selectedFwd.sol->data.rows();
    selectedFwd.sol->ncol = selectedFwd.sol->data.cols();
    selectedFwd.nsource = selectedFwd.sol->ncol / 3;

    selectedFwd.src = selectedFwd.src.pick_regions(p_qListLabels);

    return selectedFwd;
}

//=============================================================================================================

MNEForwardSolution MNEForwardSolution::pick_types(bool meg, bool eeg, const QStringList& include, const QStringList& exclude) const
{
    RowVectorXi sel = info.pick_types(meg, eeg, false, include, exclude);

    QStringList include_ch_names;
    for(qint32 i = 0; i < sel.cols(); ++i)
        include_ch_names << info.ch_names[sel[i]];

    return this->pick_channels(include_ch_names);
}

//=============================================================================================================

void MNEForwardSolution::prepare_forward(const FiffInfo &p_info,
                                         const FiffCov &p_noise_cov,
                                         bool p_pca,
                                         FiffInfo &p_outFwdInfo,
                                         MatrixXd &gain,
                                         FiffCov &p_outNoiseCov,
                                         MatrixXd &p_outWhitener,
                                         qint32 &p_outNumNonZero) const
{
    QStringList fwd_ch_names, ch_names;
    for(qint32 i = 0; i < this->info.chs.size(); ++i)
        fwd_ch_names << this->info.chs[i].ch_name;

    ch_names.clear();
    for(qint32 i = 0; i < p_info.chs.size(); ++i)
        if(!p_info.bads.contains(p_info.chs[i].ch_name)
            && !p_noise_cov.bads.contains(p_info.chs[i].ch_name)
            && p_noise_cov.names.contains(p_info.chs[i].ch_name)
            && fwd_ch_names.contains(p_info.chs[i].ch_name))
            ch_names << p_info.chs[i].ch_name;

    qint32 n_chan = ch_names.size();
    qInfo("Computing inverse operator with %d channels.", n_chan);

    //
    //   Handle noise cov
    //
    p_outNoiseCov = p_noise_cov.prepare_noise_cov(p_info, ch_names);

    //   Omit the zeroes due to projection
    p_outNumNonZero = 0;
    VectorXi t_vecNonZero = VectorXi::Zero(n_chan);
    for(qint32 i = 0; i < p_outNoiseCov.eig.rows(); ++i)
    {
        if(p_outNoiseCov.eig[i] > 0)
        {
            t_vecNonZero[p_outNumNonZero] = i;
            ++p_outNumNonZero;
        }
    }
    if(p_outNumNonZero > 0)
        t_vecNonZero.conservativeResize(p_outNumNonZero);

    if(p_outNumNonZero > 0)
    {
        if (p_pca)
        {
            qWarning("Warning in MNEForwardSolution::prepare_forward: if (p_pca) havent been debugged.");
            p_outWhitener = MatrixXd::Zero(n_chan, p_outNumNonZero);
            // Rows of eigvec are the eigenvectors
            for(qint32 i = 0; i < p_outNumNonZero; ++i)
                p_outWhitener.col(t_vecNonZero[i]) = p_outNoiseCov.eigvec.col(t_vecNonZero[i]).array() / sqrt(p_outNoiseCov.eig(t_vecNonZero[i]));
            qInfo("\tReducing data rank to %d.", p_outNumNonZero);
        }
        else
        {
            qInfo("Creating non pca whitener.");
            p_outWhitener = MatrixXd::Zero(n_chan, n_chan);
            for(qint32 i = 0; i < p_outNumNonZero; ++i)
                p_outWhitener(t_vecNonZero[i],t_vecNonZero[i]) = 1.0 / sqrt(p_outNoiseCov.eig(t_vecNonZero[i]));
            // Cols of eigvec are the eigenvectors
            p_outWhitener *= p_outNoiseCov.eigvec;
        }
    }

    VectorXi fwd_idx = VectorXi::Zero(ch_names.size());
    VectorXi info_idx = VectorXi::Zero(ch_names.size());
    qint32 idx;
    qint32 count_fwd_idx = 0;
    qint32 count_info_idx = 0;
    for(qint32 i = 0; i < ch_names.size(); ++i)
    {
        idx = fwd_ch_names.indexOf(ch_names[i]);
        if(idx > -1)
        {
            fwd_idx[count_fwd_idx] = idx;
            ++count_fwd_idx;
        }
        idx = p_info.ch_names.indexOf(ch_names[i]);
        if(idx > -1)
        {
            info_idx[count_info_idx] = idx;
            ++count_info_idx;
        }
    }
    fwd_idx.conservativeResize(count_fwd_idx);
    info_idx.conservativeResize(count_info_idx);

    gain.resize(count_fwd_idx, this->sol->data.cols());
    for(qint32 i = 0; i < count_fwd_idx; ++i)
        gain.row(i) = this->sol->data.row(fwd_idx[i]);

    p_outFwdInfo = p_info.pick_info(info_idx);

    qInfo("\tTotal rank is %d", p_outNumNonZero);
}

//=============================================================================================================

bool MNEForwardSolution::read(QIODevice& p_IODevice,
                              MNEForwardSolution& fwd,
                              bool force_fixed,
                              bool surf_ori,
                              const QStringList& include,
                              const QStringList& exclude,
                              bool bExcludeBads)
{
    FiffStream::SPtr t_pStream(new FiffStream(&p_IODevice));

    qInfo("Reading forward solution from %s...", t_pStream->streamName().toUtf8().constData());
    if(!t_pStream->open())
        return false;
    //
    //   Find all forward solutions
    //
    QList<FiffDirNode::SPtr> fwds = t_pStream->dirtree()->dir_tree_find(FIFFB_MNE_FORWARD_SOLUTION);

    if (fwds.size() == 0)
    {
        t_pStream->close();
        qWarning("No forward solutions in %s", t_pStream->streamName().toUtf8().constData());
        return false;
    }
    //
    //   Parent MRI data
    //
    QList<FiffDirNode::SPtr> parent_mri = t_pStream->dirtree()->dir_tree_find(FIFFB_MNE_PARENT_MRI_FILE);
    if (parent_mri.size() == 0)
    {
        t_pStream->close();
        qWarning("No parent MRI information in %s", t_pStream->streamName().toUtf8().constData());
        return false;
    }

    MNELIB::MNESourceSpaces t_SourceSpace;
    if(!MNELIB::MNESourceSpaces::readFromStream(t_pStream, true, t_SourceSpace))
    {
        t_pStream->close();
        qWarning("Could not read the source spaces");
        //ToDo error(me,'Could not read the source spaces (%s)',mne_omit_first_line(lasterr));
        return false;
    }

    for(qint32 k = 0; k < t_SourceSpace.size(); ++k)
        t_SourceSpace[k].id = t_SourceSpace[k].find_source_space_hemi();

    //
    //   Bad channel list
    //
    QStringList bads;
    if(bExcludeBads)
    {
        bads = t_pStream->read_bad_channels(t_pStream->dirtree());
        if(bads.size() > 0)
        {
            qInfo("\t%lld bad channels ( ", bads.size());
            for(qint32 i = 0; i < bads.size(); ++i)
                qInfo("\"%s\" ", bads[i].toUtf8().constData());
            qInfo(") read");
        }
    }

    //
    //   Locate and read the forward solutions
    //
    FiffTag::UPtr t_pTag;
    FiffDirNode::SPtr megnode;
    FiffDirNode::SPtr eegnode;
    for(qint32 k = 0; k < fwds.size(); ++k)
    {
        if(!fwds[k]->find_tag(t_pStream, FIFF_MNE_INCLUDED_METHODS, t_pTag))
        {
            t_pStream->close();
            qWarning("Methods not listed for one of the forward solutions");
            return false;
        }
        if (*t_pTag->toInt() == FIFFV_MNE_MEG)
        {
            qInfo("MEG solution found");
            megnode = fwds[k];
        }
        else if(*t_pTag->toInt() == FIFFV_MNE_EEG)
        {
            qInfo("EEG solution found");
            eegnode = fwds[k];
        }
    }

    MNEForwardSolution megfwd;
    QString ori;
    if (read_one(t_pStream, megnode, megfwd))
    {
        if (megfwd.source_ori == FIFFV_MNE_FIXED_ORI)
            ori = QString("fixed");
        else
            ori = QString("free");
        qInfo("\tRead MEG forward solution (%d sources, %d channels, %s orientations)", megfwd.nsource,megfwd.nchan,ori.toUtf8().constData());
    }
    MNEForwardSolution eegfwd;
    if (read_one(t_pStream, eegnode, eegfwd))
    {
        if (eegfwd.source_ori == FIFFV_MNE_FIXED_ORI)
            ori = QString("fixed");
        else
            ori = QString("free");
        qInfo("\tRead EEG forward solution (%d sources, %d channels, %s orientations)", eegfwd.nsource,eegfwd.nchan,ori.toUtf8().constData());
    }

    //
    //   Merge the MEG and EEG solutions together
    //
    fwd.clear();

    if (!megfwd.isEmpty() && !eegfwd.isEmpty())
    {
        if (megfwd.sol->data.cols() != eegfwd.sol->data.cols() ||
                megfwd.source_ori != eegfwd.source_ori ||
                megfwd.nsource != eegfwd.nsource ||
                megfwd.coord_frame != eegfwd.coord_frame)
        {
            t_pStream->close();
            qWarning("The MEG and EEG forward solutions do not match");
            return false;
        }

        fwd = std::move(MNEForwardSolution(megfwd));
        fwd.sol->data = MatrixXd(megfwd.sol->nrow + eegfwd.sol->nrow, megfwd.sol->ncol);

        fwd.sol->data.block(0,0,megfwd.sol->nrow,megfwd.sol->ncol) = megfwd.sol->data;
        fwd.sol->data.block(megfwd.sol->nrow,0,eegfwd.sol->nrow,eegfwd.sol->ncol) = eegfwd.sol->data;
        fwd.sol->nrow = megfwd.sol->nrow + eegfwd.sol->nrow;
        fwd.sol->row_names.append(eegfwd.sol->row_names);

        if (!fwd.sol_grad->isEmpty())
        {
            fwd.sol_grad->data.resize(megfwd.sol_grad->data.rows() + eegfwd.sol_grad->data.rows(), megfwd.sol_grad->data.cols());

            fwd.sol->data.block(0,0,megfwd.sol_grad->data.rows(),megfwd.sol_grad->data.cols()) = megfwd.sol_grad->data;
            fwd.sol->data.block(megfwd.sol_grad->data.rows(),0,eegfwd.sol_grad->data.rows(),eegfwd.sol_grad->data.cols()) = eegfwd.sol_grad->data;

            fwd.sol_grad->nrow      = megfwd.sol_grad->nrow + eegfwd.sol_grad->nrow;
            fwd.sol_grad->row_names.append(eegfwd.sol_grad->row_names);
        }
        fwd.nchan  = megfwd.nchan + eegfwd.nchan;
        qInfo("\tMEG and EEG forward solutions combined");
    }
    else if (!megfwd.isEmpty())
        fwd = std::move(megfwd); //not copied for the sake of speed
    else
        fwd = std::move(eegfwd); //not copied for the sake of speed

    //
    //   Get the MRI <-> head coordinate transformation
    //
    if(!parent_mri[0]->find_tag(t_pStream, FIFF_COORD_TRANS, t_pTag))
    {
        t_pStream->close();
        qWarning("MRI/head coordinate transformation not found");
        return false;
    }
    else
    {
        fwd.mri_head_t = t_pTag->toCoordTrans();

        if (fwd.mri_head_t.from != FIFFV_COORD_MRI || fwd.mri_head_t.to != FIFFV_COORD_HEAD)
        {
            fwd.mri_head_t.invert_transform();
            if (fwd.mri_head_t.from != FIFFV_COORD_MRI || fwd.mri_head_t.to != FIFFV_COORD_HEAD)
            {
                t_pStream->close();
                qWarning("MRI/head coordinate transformation not found");
                return false;
            }
        }
    }

    //
    // get parent MEG info -> from python package
    //
    t_pStream->read_meas_info_base(t_pStream->dirtree(), fwd.info);

    t_pStream->close();

    //
    //   Transform the source spaces to the correct coordinate frame
    //   if necessary
    //
    if (fwd.coord_frame != FIFFV_COORD_MRI && fwd.coord_frame != FIFFV_COORD_HEAD)
    {
        qWarning("Only forward solutions computed in MRI or head coordinates are acceptable");
        return false;
    }

    //
    qint32 nuse = 0;
    t_SourceSpace.transform_source_space_to(fwd.coord_frame,fwd.mri_head_t);
    for(qint32 k = 0; k < t_SourceSpace.size(); ++k)
        nuse += t_SourceSpace[k].nuse;

    if (nuse != fwd.nsource){
        qDebug() << "Source spaces do not match the forward solution.\n";
        return false;
    }

    qInfo("\tSource spaces transformed to the forward solution coordinate frame");
    fwd.src = t_SourceSpace; //not new MNESourceSpaces(t_SourceSpace); for sake of speed
    //
    //   Handle the source locations and orientations
    //
    if (fwd.isFixedOrient() || force_fixed == true)
    {
        nuse = 0;
        fwd.source_rr = MatrixXf::Zero(fwd.nsource,3);
        fwd.source_nn = MatrixXf::Zero(fwd.nsource,3);
        for(qint32 k = 0; k < t_SourceSpace.size();++k)
        {
            for(qint32 q = 0; q < t_SourceSpace[k].nuse; ++q)
            {
                fwd.source_rr.block(q,0,1,3) = t_SourceSpace[k].rr.block(t_SourceSpace[k].vertno(q),0,1,3);
                fwd.source_nn.block(q,0,1,3) = t_SourceSpace[k].nn.block(t_SourceSpace[k].vertno(q),0,1,3);
            }
            nuse += t_SourceSpace[k].nuse;
        }
        //
        //   Modify the forward solution for fixed source orientations
        //
        if (fwd.source_ori != FIFFV_MNE_FIXED_ORI)
        {
            qInfo("\tChanging to fixed-orientation forward solution...");

            MatrixXd tmp = fwd.source_nn.transpose().cast<double>();
            SparseMatrix<double> fix_rot = Linalg::make_block_diag(tmp,1);
            fwd.sol->data *= fix_rot;
            fwd.sol->ncol  = fwd.nsource;
            fwd.source_ori = FIFFV_MNE_FIXED_ORI;

            if (!fwd.sol_grad->isEmpty())
            {
                SparseMatrix<double> t_matKron;
                SparseMatrix<double> t_eye(3,3);
                for (qint32 i = 0; i < 3; ++i)
                    t_eye.insert(i,i) = 1.0f;
                t_matKron = kroneckerProduct(fix_rot,t_eye);//kron(fix_rot,eye(3));
                fwd.sol_grad->data *= t_matKron;
                fwd.sol_grad->ncol   = 3*fwd.nsource;
            }
            qInfo("[done]");
        }
    }
    else if (surf_ori)
    {
        //
        //   Rotate the local source coordinate systems
        //
        qInfo("\tConverting to surface-based source orientations...");

        bool use_ave_nn = false;
        auto* hemi0 = t_SourceSpace.hemisphereAt(0);
        if(hemi0 && hemi0->patch_inds.size() > 0)
        {
            use_ave_nn = true;
            qInfo("\tAverage patch normals will be employed in the rotation to the local surface coordinates...");
        }

        nuse = 0;
        qint32 pp = 0;
        fwd.source_rr = MatrixXf::Zero(fwd.nsource,3);
        fwd.source_nn = MatrixXf::Zero(fwd.nsource*3,3);

        qWarning("Warning source_ori: Rotating the source coordinate system haven't been verified --> Singular Vectors U are different from MATLAB!");

        for(qint32 k = 0; k < t_SourceSpace.size();++k)
        {

            for (qint32 q = 0; q < t_SourceSpace[k].nuse; ++q)
                fwd.source_rr.block(q+nuse,0,1,3) = t_SourceSpace[k].rr.block(t_SourceSpace[k].vertno(q),0,1,3);

            for (qint32 p = 0; p < t_SourceSpace[k].nuse; ++p)
            {
                //
                //  Project out the surface normal and compute SVD
                //
                Vector3f nn;
                if(use_ave_nn)
                {
                    auto* hemiK = t_SourceSpace.hemisphereAt(k);
                    VectorXi t_vIdx = hemiK->pinfo[hemiK->patch_inds[p]];
                    Matrix3Xf t_nn(3, t_vIdx.size());
                    for(qint32 i = 0; i < t_vIdx.size(); ++i)
                        t_nn.col(i) = t_SourceSpace[k].nn.block(t_vIdx[i],0,1,3).transpose();
                    nn = t_nn.rowwise().sum();
                    nn.array() /= nn.norm();
                }
                else
                    nn = t_SourceSpace[k].nn.block(t_SourceSpace[k].vertno(p),0,1,3).transpose();

                Matrix3f tmp = Matrix3f::Identity(nn.rows(), nn.rows()) - nn*nn.transpose();

                JacobiSVD<MatrixXf> t_svd(tmp, Eigen::ComputeThinU);
                //Sort singular values and singular vectors
                VectorXf t_s = t_svd.singularValues();
                MatrixXf U = t_svd.matrixU();
                Linalg::sort<float>(t_s, U);

                //
                //  Make sure that ez is in the direction of nn
                //
                if ((nn.transpose() * U.block(0,2,3,1))(0,0) < 0)
                    U *= -1;
                fwd.source_nn.block(pp, 0, 3, 3) = U.transpose();
                pp += 3;
            }
            nuse += t_SourceSpace[k].nuse;
        }
        MatrixXd tmp = fwd.source_nn.transpose().cast<double>();
        SparseMatrix<double> surf_rot = Linalg::make_block_diag(tmp,3);

        fwd.sol->data *= surf_rot;

        if (!fwd.sol_grad->isEmpty())
        {
            SparseMatrix<double> t_matKron;
            SparseMatrix<double> t_eye(3,3);
            for (qint32 i = 0; i < 3; ++i)
                t_eye.insert(i,i) = 1.0f;
            t_matKron = kroneckerProduct(surf_rot,t_eye);//kron(surf_rot,eye(3));
            fwd.sol_grad->data *= t_matKron;
        }
        qInfo("[done]");
    }
    else
    {
        qInfo("\tCartesian source orientations...");
        nuse = 0;
        fwd.source_rr = MatrixXf::Zero(fwd.nsource,3);
        for(qint32 k = 0; k < t_SourceSpace.size(); ++k)
        {
            for (qint32 q = 0; q < t_SourceSpace[k].nuse; ++q)
                fwd.source_rr.block(q+nuse,0,1,3) = t_SourceSpace[k].rr.block(t_SourceSpace[k].vertno(q),0,1,3);

            nuse += t_SourceSpace[k].nuse;
        }

        MatrixXf t_ones = MatrixXf::Ones(fwd.nsource,1);
        Matrix3f t_eye = Matrix3f::Identity();
        fwd.source_nn = kroneckerProduct(t_ones,t_eye);

        qInfo("[done]");
    }

    //
    //   Do the channel selection
    //
    QStringList exclude_bads = exclude;
    if (bads.size() > 0)
    {
        for(qint32 k = 0; k < bads.size(); ++k)
            if(!exclude_bads.contains(bads[k],Qt::CaseInsensitive))
                exclude_bads << bads[k];
    }

    fwd.surf_ori = surf_ori;
    fwd = std::move(fwd.pick_channels(include, exclude_bads));

    //garbage collecting
    t_pStream->close();

    return true;
}

//=============================================================================================================

bool MNEForwardSolution::read_one(FiffStream::SPtr& p_pStream,
                                  const FiffDirNode::SPtr& p_Node,
                                  MNEForwardSolution& one)
{
    //
    //   Read all interesting stuff for one forward solution
    //
    if(!p_Node)
        return false;

    one.clear();
    FiffTag::UPtr t_pTag;

    if(!p_Node->find_tag(p_pStream, FIFF_MNE_SOURCE_ORIENTATION, t_pTag))
    {
        p_pStream->close();
        qWarning("Source orientation tag not found.");
        return false;
    }

    one.source_ori = *t_pTag->toInt();

    if(!p_Node->find_tag(p_pStream, FIFF_MNE_COORD_FRAME, t_pTag))
    {
        p_pStream->close();
        qWarning("Coordinate frame tag not found.");
        return false;
    }

    one.coord_frame = *t_pTag->toInt();

    if(!p_Node->find_tag(p_pStream, FIFF_MNE_SOURCE_SPACE_NPOINTS, t_pTag))
    {
        p_pStream->close();
        qWarning("Number of sources not found.");
        return false;
    }

    one.nsource = *t_pTag->toInt();

    if(!p_Node->find_tag(p_pStream, FIFF_NCHAN, t_pTag))
    {
        p_pStream->close();
        qWarning("Number of channels not found.");
        return false;
    }

    one.nchan = *t_pTag->toInt();

    if(p_pStream->read_named_matrix(p_Node, FIFF_MNE_FORWARD_SOLUTION, *one.sol.data()))
        one.sol->transpose_named_matrix();
    else
    {
        p_pStream->close();
        qWarning("Forward solution data not found.");
        //error(me,'Forward solution data not found (%s)',mne_omit_first_line(lasterr));
        return false;
    }

    if(p_pStream->read_named_matrix(p_Node, FIFF_MNE_FORWARD_SOLUTION_GRAD, *one.sol_grad.data()))
        one.sol_grad->transpose_named_matrix();
    else
        one.sol_grad->clear();

    if (one.sol->data.rows() != one.nchan ||
            (one.sol->data.cols() != one.nsource && one.sol->data.cols() != 3*one.nsource))
    {
        p_pStream->close();
        qWarning("Forward solution matrix has wrong dimensions.");
        //error(me,'Forward solution matrix has wrong dimensions');
        return false;
    }
    if (!one.sol_grad->isEmpty())
    {
        if (one.sol_grad->data.rows() != one.nchan ||
                (one.sol_grad->data.cols() != 3*one.nsource && one.sol_grad->data.cols() != 3*3*one.nsource))
        {
            p_pStream->close();
            qWarning("Forward solution gradient matrix has wrong dimensions.");
            //error(me,'Forward solution gradient matrix has wrong dimensions');
        }
    }
    return true;
}

//=============================================================================================================

void MNEForwardSolution::restrict_gain_matrix(MatrixXd &G, const FiffInfo &info)
{
    // Figure out which ones have been used
    if(info.chs.size() != G.rows())
    {
        qWarning("Error G.rows() and length of info.chs do not match: %ld != %lli", G.rows(), info.chs.size());
        return;
    }

    RowVectorXi sel = info.pick_types(QString("grad"));
    if(sel.size() > 0)
    {
        for(qint32 i = 0; i < sel.size(); ++i)
            G.row(i) = G.row(sel[i]);
        G.conservativeResize(sel.size(), G.cols());
        qInfo("\t%ld planar channels", sel.size());
    }
    else
    {
        sel = info.pick_types(QString("mag"));
        if (sel.size() > 0)
        {
            for(qint32 i = 0; i < sel.size(); ++i)
                G.row(i) = G.row(sel[i]);
            G.conservativeResize(sel.size(), G.cols());
            qInfo("\t%ld magnetometer or axial gradiometer channels", sel.size());
        }
        else
        {
            sel = info.pick_types(false, true);
            if(sel.size() > 0)
            {
                for(qint32 i = 0; i < sel.size(); ++i)
                    G.row(i) = G.row(sel[i]);
                G.conservativeResize(sel.size(), G.cols());
                qInfo("\t%ld EEG channels", sel.size());
            }
            else
                qWarning("Could not find MEG or EEG channels");
        }
    }
}

//=============================================================================================================

void MNEForwardSolution::to_fixed_ori()
{
    if(!this->surf_ori || this->isFixedOrient())
    {
        throw std::logic_error("Only surface-oriented, free-orientation forward solutions can be converted to fixed orientation");
    }
    qint32 count = 0;
    for(qint32 i = 2; i < this->sol->data.cols(); i += 3)
        this->sol->data.col(count) = this->sol->data.col(i);//ToDo: is this right? - just take z?
    this->sol->data.conservativeResize(this->sol->data.rows(), count);
    this->sol->ncol = this->sol->ncol / 3;
    this->source_ori = FIFFV_MNE_FIXED_ORI;
    qInfo("\tConverted the forward solution into the fixed-orientation mode.");
}

//=============================================================================================================

bool MNEForwardSolution::isClustered() const
{
    auto* hemi = src.hemisphereAt(0);
    return hemi && hemi->isClustered();
}

//=============================================================================================================

MatrixX3f MNEForwardSolution::getSourcePositionsByLabel(const QList<FsLabel> &lPickedLabels, const FsSurfaceSet& tSurfSetInflated)
{
    MatrixX3f matSourceVertLeft, matSourceVertRight, matSourcePositions;

    if(lPickedLabels.isEmpty()) {
        qWarning() << "MNEForwardSolution::getSourcePositionsByLabel - picked label list is empty. Returning.";
        return  matSourcePositions;
    }

    if(tSurfSetInflated.isEmpty()) {
        qWarning() << "MNEForwardSolution::getSourcePositionsByLabel - tSurfSetInflated is empty. Returning.";
        return  matSourcePositions;
    }

    if(isClustered()) {
        for(int j = 0; j < this->src[0].vertno.rows(); ++j) {
            for(int k = 0; k < lPickedLabels.size(); k++) {
                if(this->src[0].vertno(j) == lPickedLabels.at(k).label_id) {
                    matSourceVertLeft.conservativeResize(matSourceVertLeft.rows()+1,3);
                    matSourceVertLeft.row(matSourceVertLeft.rows()-1) = tSurfSetInflated[0].rr().row(this->src.hemisphereAt(0)->cluster_info.centroidVertno.at(j)) - tSurfSetInflated[0].offset().transpose();
                    break;
                }
            }
        }

        for(int j = 0; j < this->src[1].vertno.rows(); ++j) {
            for(int k = 0; k < lPickedLabels.size(); k++) {
                if(this->src[1].vertno(j) == lPickedLabels.at(k).label_id) {
                    matSourceVertRight.conservativeResize(matSourceVertRight.rows()+1,3);
                    matSourceVertRight.row(matSourceVertRight.rows()-1) = tSurfSetInflated[1].rr().row(this->src.hemisphereAt(1)->cluster_info.centroidVertno.at(j)) - tSurfSetInflated[1].offset().transpose();
                    break;
                }
            }
        }
    } else {
        for(int j = 0; j < this->src[0].vertno.rows(); ++j) {
            for(int k = 0; k < lPickedLabels.size(); k++) {
                for(int l = 0; l < lPickedLabels.at(k).vertices.rows(); l++) {
                    if(this->src[0].vertno(j) == lPickedLabels.at(k).vertices(l) && lPickedLabels.at(k).hemi == 0) {
                        matSourceVertLeft.conservativeResize(matSourceVertLeft.rows()+1,3);
                        matSourceVertLeft.row(matSourceVertLeft.rows()-1) = tSurfSetInflated[0].rr().row(this->src[0].vertno(j)) - tSurfSetInflated[0].offset().transpose();
                        break;
                    }
                }
            }
        }

        for(int j = 0; j < this->src[1].vertno.rows(); ++j) {
            for(int k = 0; k < lPickedLabels.size(); k++) {
                for(int l = 0; l < lPickedLabels.at(k).vertices.rows(); l++) {
                    if(this->src[1].vertno(j) == lPickedLabels.at(k).vertices(l) && lPickedLabels.at(k).hemi == 1) {
                        matSourceVertRight.conservativeResize(matSourceVertRight.rows()+1,3);
                        matSourceVertRight.row(matSourceVertRight.rows()-1) = tSurfSetInflated[1].rr().row(this->src[1].vertno(j)) - tSurfSetInflated[1].offset().transpose();
                        break;
                    }
                }
            }
        }
    }

    matSourcePositions.resize(matSourceVertLeft.rows()+matSourceVertRight.rows(),3);
    matSourcePositions << matSourceVertLeft, matSourceVertRight;

    return matSourcePositions;
}
