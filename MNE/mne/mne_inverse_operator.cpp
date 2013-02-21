//=============================================================================================================
/**
* @file     mne_inverse_operator.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implementation of the MNEInverseOperator Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_inverse_operator.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEMATHLIB;
using namespace MNELIB;


//*************************************************************************************************************
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
}


//*************************************************************************************************************

MNEInverseOperator::MNEInverseOperator(QIODevice& p_IODevice)
{
    MNEInverseOperator::read_inverse_operator(p_IODevice, *this);
}


//*************************************************************************************************************

MNEInverseOperator::MNEInverseOperator(const MNEInverseOperator &p_MNEInverseOperator)
: info(p_MNEInverseOperator.info)
, methods(p_MNEInverseOperator.methods)
, source_ori(p_MNEInverseOperator.source_ori)
, nsource(p_MNEInverseOperator.nsource)
, nchan(p_MNEInverseOperator.nchan)
, coord_frame(p_MNEInverseOperator.coord_frame)
, source_nn(p_MNEInverseOperator.source_nn)
, sing(p_MNEInverseOperator.sing)
, eigen_leads_weighted(p_MNEInverseOperator.eigen_leads_weighted)
, eigen_leads(p_MNEInverseOperator.eigen_leads)
, eigen_fields(p_MNEInverseOperator.eigen_fields)
, noise_cov(p_MNEInverseOperator.noise_cov)
, source_cov(p_MNEInverseOperator.source_cov)
, orient_prior(p_MNEInverseOperator.orient_prior)
, depth_prior(p_MNEInverseOperator.depth_prior)
, fmri_prior(p_MNEInverseOperator.fmri_prior)
, src(p_MNEInverseOperator.src)
, mri_head_t(p_MNEInverseOperator.mri_head_t)
, nave(p_MNEInverseOperator.nave)
, projs(p_MNEInverseOperator.projs)
, proj(p_MNEInverseOperator.proj)
, whitener(p_MNEInverseOperator.whitener)
, reginv(p_MNEInverseOperator.reginv)
, noisenorm(p_MNEInverseOperator.noisenorm)
{

}


//*************************************************************************************************************

MNEInverseOperator::~MNEInverseOperator()
{

}


//*************************************************************************************************************

MNEInverseOperator MNEInverseOperator::make_inverse_operator(FiffInfo &info, MNEForwardSolution &forward, FiffCov &p_noise_cov, float loose, float depth, bool fixed, bool limit_depth_chs)
{
    bool is_fixed_ori = forward.isFixedOrient();
    MNEInverseOperator p_MNEInverseOperator;

    qDebug() << "ToDo MNEInverseOperator::make_inverse_operator: do surf_ori check";

    //Check parameters
    if(fixed && loose > 0)
    {
        printf("Warning: When invoking make_inverse_operator with fixed = true, the loose parameter is ignored.\n");
        loose = 0.0f;
    }

    if(is_fixed_ori && !fixed)
    {
        printf("Warning: Setting fixed parameter = true. Because the given forward operator has fixed orientation and can only be used to make a fixed-orientation inverse operator.\n");
        fixed = true;
    }

    if(forward.source_ori == -1 && loose > 0)
    {
        printf("Error: Forward solution is not oriented in surface coordinates. loose parameter should be 0 not %f.\n", loose);
        return p_MNEInverseOperator;
    }

    if(loose < 0 || loose > 1)
    {
        printf("Warning: Loose value should be in interval [0,1] not %f.\n", loose);
        loose = loose > 1 ? 1 : 0;
        printf("Setting loose to %f.\n", loose);
    }

    if(depth < 0 || depth > 1)
    {
        printf("Warning: Depth value should be in interval [0,1] not %f.\n", depth);
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
    forward.prepare_forward(info, p_noise_cov, false, gain_info, gain, p_noise_cov, whitener, n_nzero);

    //
    // 5. Compose the depth weight matrix
    //
    FiffCov::SDPtr p_depth_prior;
    MatrixXd patch_areas;
    if(depth > 0)
    {
        qDebug() << "ToDo: patch_areas";
//        patch_areas = forward.get('patch_areas', None)
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
            // Convert the depth prior into a fixed-orientation one
            printf("\tToDo: Picked elements from a free-orientation depth-weighting prior into the fixed-orientation one.\n");
        }
        if(!is_fixed_ori)
        {
            // Convert to the fixed orientation forward solution now
            qint32 count = 0;
            for(qint32 i = 2; i < p_depth_prior->data.rows(); i+=3)
            {
                p_depth_prior->data.row(count) = p_depth_prior->data.row(i);
                ++count;
            }
            p_depth_prior->data.conservativeResize(count, p_depth_prior->data.cols());

//            forward = deepcopy(forward)
            forward.to_fixed_ori();
            is_fixed_ori = forward.isFixedOrient();
            forward.prepare_forward(info, p_noise_cov, false, gain_info, gain, p_noise_cov, whitener, n_nzero);
        }
    }
    printf("\tComputing inverse operator with %d channels.\n", gain_info.ch_names.size());

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

    double trace_GRGT = (gain * gain.transpose()).trace();//pow(gain.norm(), 2);
    double scaling_source_cov = (double)n_nzero / trace_GRGT;

    p_source_cov->data.array() *= scaling_source_cov;

    gain.array() *= sqrt(scaling_source_cov);

    // now np.trace(np.dot(gain, gain.T)) == n_nzero
    // logger.info(np.trace(np.dot(gain, gain.T)), n_nzero)

    //
    // 12. Decompose the combined matrix
    //
    printf("Computing SVD of whitened and weighted lead field matrix.\n");
    JacobiSVD<MatrixXd> svd(gain, ComputeThinU | ComputeThinV);
    FiffNamedMatrix::SDPtr p_eigen_fields = FiffNamedMatrix::SDPtr(new FiffNamedMatrix( svd.matrixU().cols(),
                                                                                        svd.matrixU().rows(),
                                                                                        defaultQStringList,
                                                                                        gain_info.ch_names,
                                                                                        svd.matrixU().transpose()));

    VectorXd p_sing = svd.singularValues();
    FiffNamedMatrix::SDPtr p_eigen_leads = FiffNamedMatrix::SDPtr(new FiffNamedMatrix( svd.matrixV().cols(),
                                                                                       svd.matrixV().rows(),
                                                                                       defaultQStringList,
                                                                                       defaultQStringList,
                                                                                       svd.matrixV().transpose()));
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

    p_MNEInverseOperator.eigen_fields = p_eigen_fields;
    p_MNEInverseOperator.eigen_leads = p_eigen_leads;
    p_MNEInverseOperator.sing = p_sing;
    p_MNEInverseOperator.nave = p_nave;
    p_MNEInverseOperator.depth_prior = p_depth_prior;
    p_MNEInverseOperator.source_cov = p_source_cov;
    p_MNEInverseOperator.noise_cov = FiffCov::SDPtr(new FiffCov(p_noise_cov));
    p_MNEInverseOperator.orient_prior = p_orient_prior;
    p_MNEInverseOperator.projs = info.projs;
    p_MNEInverseOperator.eigen_leads_weighted = false;
    p_MNEInverseOperator.source_ori = forward.source_ori;
    p_MNEInverseOperator.mri_head_t = forward.mri_head_t;
    p_MNEInverseOperator.methods = p_iMethods;
    p_MNEInverseOperator.nsource = forward.nsource;
    p_MNEInverseOperator.coord_frame = forward.coord_frame;
    p_MNEInverseOperator.source_nn = forward.source_nn;
    p_MNEInverseOperator.src = forward.src;
    p_MNEInverseOperator.info = forward.info;
    p_MNEInverseOperator.info.bads = info.bads;

    return p_MNEInverseOperator;
}


//*************************************************************************************************************

MNEInverseOperator MNEInverseOperator::prepare_inverse_operator(qint32 nave ,float lambda2, bool dSPM, bool sLORETA)
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
    float scale     = ((float)inv.nave)/((float)nave);
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
//    if(inv.reginv)
//        delete inv.reginv;
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
//    if(inv.whitener)
//        delete inv.whitener;
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
        VectorXd* noise_norm = new VectorXd(VectorXd::Zero(inv.eigen_leads->nrow));
        VectorXd* noise_weight = NULL;
        if (dSPM)
        {
           printf("\tComputing noise-normalization factors (dSPM)...");
           noise_weight = new VectorXd(inv.reginv);
        }
        else
        {
           printf("\tComputing noise-normalization factors (sLORETA)...");
           VectorXd tmp = (VectorXd::Constant(inv.sing.size(), 1) + inv.sing.cwiseProduct(inv.sing)/lambda2);
           noise_weight = new VectorXd(inv.reginv.cwiseProduct(tmp.cwiseSqrt()));
        }
        VectorXd one;
        if (inv.eigen_leads_weighted)
        {
           for (k = 0; k < inv.eigen_leads->nrow; ++k)
           {
              one = inv.eigen_leads->data.block(k,0,1,inv.eigen_leads->data.cols()).cwiseProduct(*noise_weight);
              (*noise_norm)[k] = sqrt(one.dot(one));
           }
        }
        else
        {
            float c;
            for (k = 0; k < inv.eigen_leads->nrow; ++k)
            {
                c = sqrt(inv.source_cov->data(k,0));
                one = c*(inv.eigen_leads->data.block(k,0,1,inv.eigen_leads->data.cols()).transpose()).cwiseProduct(*noise_weight);//ToDo eigenleads data -> pointer
                (*noise_norm)[k] = sqrt(one.dot(one));
            }
        }
        //
        //   Compute the final result
        //
        VectorXd noise_norm_new;
        if (inv.source_ori == FIFFV_MNE_FREE_ORI)
        {
            //
            //   The three-component case is a little bit more involved
            //   The variances at three consequtive entries must be squeared and
            //   added together
            //
            //   Even in this case return only one noise-normalization factor
            //   per source location
            //
            VectorXd* t = MNEMath::combine_xyz(noise_norm->transpose());
            noise_norm_new = t->cwiseSqrt();//double otherwise values are getting too small
            delete t;
            //
            //   This would replicate the same value on three consequtive
            //   entries
            //
            //   noise_norm = kron(sqrt(mne_combine_xyz(noise_norm)),ones(3,1));
        }
        VectorXd vOnes = VectorXd::Ones(noise_norm_new.size());
        VectorXd tmp = vOnes.cwiseQuotient(noise_norm_new.cwiseAbs());
//        if(inv.noisenorm)
//            delete inv.noisenorm;

        typedef Eigen::Triplet<double> T;
        std::vector<T> tripletList;
        tripletList.reserve(noise_norm_new.size());
        for(qint32 i = 0; i < noise_norm_new.size(); ++i)
            tripletList.push_back(T(i, i, tmp[i]));

        inv.noisenorm = SparseMatrix<double>(noise_norm_new.size(),noise_norm_new.size());
        inv.noisenorm.setFromTriplets(tripletList.begin(), tripletList.end());

        delete noise_weight;
        delete noise_norm;
        printf("[done]\n");
    }
    else
    {
//        if(inv.noisenorm)
//            delete inv.noisenorm;
        inv.noisenorm = SparseMatrix<double>();
    }

    return inv;
}


//*************************************************************************************************************

bool MNEInverseOperator::read_inverse_operator(QIODevice& p_IODevice, MNEInverseOperator& inv)
{
    //
    //   Open the file, create directory
    //
    FiffStream* t_pStream = new FiffStream(&p_IODevice);
    printf("Reading inverse operator decomposition from %s...\n",t_pStream->streamName().toUtf8().constData());
    FiffDirTree t_Tree;
    QList<FiffDirEntry> t_Dir;

    if(!t_pStream->open(t_Tree, t_Dir))
    {
        if(t_pStream)
            delete t_pStream;

        return false;
    }
    //
    //   Find all inverse operators
    //
    QList <FiffDirTree> invs_list = t_Tree.dir_tree_find(FIFFB_MNE_INVERSE_SOLUTION);
    if ( invs_list.size()== 0)
    {
        printf("No inverse solutions in %s\n", t_pStream->streamName().toUtf8().constData());
        if(t_pStream)
            delete t_pStream;
        return false;
    }
    FiffDirTree* invs = &invs_list[0];
    //
    //   Parent MRI data
    //
    QList <FiffDirTree> parent_mri = t_Tree.dir_tree_find(FIFFB_MNE_PARENT_MRI_FILE);
    if (parent_mri.size() == 0)
    {
        printf("No parent MRI information in %s", t_pStream->streamName().toUtf8().constData());
        if(t_pStream)
            delete t_pStream;
        return false;
    }
    printf("\tReading inverse operator info...");
    //
    //   Methods and source orientations
    //
    FiffTag* t_pTag = NULL;
    if (!invs->find_tag(t_pStream, FIFF_MNE_INCLUDED_METHODS, t_pTag))
    {
        printf("Modalities not found\n");
        if(t_pTag)
            delete t_pTag;
        if(t_pStream)
            delete t_pStream;
        return false;
    }

    inv = MNEInverseOperator();
    inv.methods = *t_pTag->toInt();
    //
    if (!invs->find_tag(t_pStream, FIFF_MNE_SOURCE_ORIENTATION, t_pTag))
    {
        printf("Source orientation constraints not found\n");
        if(t_pTag)
            delete t_pTag;
        if(t_pStream)
            delete t_pStream;
        return false;
    }
    inv.source_ori = *t_pTag->toInt();
    //
    if (!invs->find_tag(t_pStream, FIFF_MNE_SOURCE_SPACE_NPOINTS, t_pTag))
    {
        printf("Number of sources not found\n");
        if(t_pTag)
            delete t_pTag;
        if(t_pStream)
            delete t_pStream;
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
        if(t_pTag)
            delete t_pTag;
        if(t_pStream)
            delete t_pStream;
        return false;
    }
    inv.coord_frame = *t_pTag->toInt();
    //
    //   The actual source orientation vectors
    //
    if (!invs->find_tag(t_pStream, FIFF_MNE_INVERSE_SOURCE_ORIENTATIONS, t_pTag))
    {
        printf("Source orientation information not found\n");
        if(t_pTag)
            delete t_pTag;
        if(t_pStream)
            delete t_pStream;
        return false;
    }

//    if(inv.source_nn)
//        delete inv.source_nn;
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
        if(t_pTag)
            delete t_pTag;
        if(t_pStream)
            delete t_pStream;
        return false;
    }

//    if(inv.sing)
//        delete inv.sing;
    inv.sing = Map<VectorXf>(t_pTag->toFloat(), t_pTag->size()/4).cast<double>();
    inv.nchan = inv.sing.rows();
    //
    //   The eigenleads and eigenfields
    //
    inv.eigen_leads_weighted = false;
    if(!Fiff::read_named_matrix(t_pStream, *invs, FIFF_MNE_INVERSE_LEADS, *inv.eigen_leads.data()))
    {
        inv.eigen_leads_weighted = true;
        if(!Fiff::read_named_matrix(t_pStream, *invs, FIFF_MNE_INVERSE_LEADS_WEIGHTED, *inv.eigen_leads.data()))
        {
            printf("Error reading eigenleads named matrix.\n");
            if(t_pTag)
                delete t_pTag;
            if(t_pStream)
                delete t_pStream;
            return false;
        }
    }
    //
    //   Having the eigenleads as columns is better for the inverse calculations
    //
    inv.eigen_leads->transpose_named_matrix();


    if(!Fiff::read_named_matrix(t_pStream, *invs, FIFF_MNE_INVERSE_FIELDS, *inv.eigen_fields.data()))
    {
        printf("Error reading eigenfields named matrix.\n");
        if(t_pTag)
            delete t_pTag;
        if(t_pStream)
            delete t_pStream;
        return false;
    }
    printf("[done]\n");
    //
    //   Read the covariance matrices
    //
    if(t_pStream->read_cov(*invs, FIFFV_MNE_NOISE_COV, *inv.noise_cov.data()))
    {
        printf("\tNoise covariance matrix read.\n");
    }
    else
    {
        printf("\tError: Not able to read noise covariance matrix.\n");
        if(t_pTag)
            delete t_pTag;
        if(t_pStream)
            delete t_pStream;
        return false;
    }

    if(t_pStream->read_cov(*invs, FIFFV_MNE_SOURCE_COV, *inv.source_cov.data()))
    {
        printf("\tSource covariance matrix read.\n");
    }
    else
    {
        printf("\tError: Not able to read source covariance matrix.\n");
        if(t_pTag)
            delete t_pTag;
        if(t_pStream)
            delete t_pStream;
        return false;
    }
    //
    //   Read the various priors
    //
    if(t_pStream->read_cov(*invs, FIFFV_MNE_ORIENT_PRIOR_COV, *inv.orient_prior.data()))
    {
        printf("\tOrientation priors read.\n");
    }
    else
        inv.orient_prior->clear();

    if(t_pStream->read_cov(*invs, FIFFV_MNE_DEPTH_PRIOR_COV, *inv.depth_prior.data()))
    {
        printf("\tDepth priors read.\n");
    }
    else
    {
        inv.depth_prior->clear();
    }
    if(t_pStream->read_cov(*invs, FIFFV_MNE_FMRI_PRIOR_COV, *inv.fmri_prior.data()))
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
    if(!MNESourceSpace::read_source_spaces(t_pStream, false, t_Tree, inv.src))
    {
        printf("\tError: Could not read the source spaces.\n");
        if(t_pTag)
            delete t_pTag;
        if(t_pStream)
            delete t_pStream;
        return false;
    }
    for (qint32 k = 0; k < inv.src.hemispheres.size(); ++k)
       inv.src.hemispheres[k].id = MNESourceSpace::find_source_space_hemi(inv.src.hemispheres[k]);
    //
    //   Get the MRI <-> head coordinate transformation
    //
    FiffCoordTrans mri_head_t;// = NULL;
    if (!parent_mri[0].find_tag(t_pStream, FIFF_COORD_TRANS, t_pTag))
    {
        printf("MRI/head coordinate transformation not found\n");
        if(t_pTag)
            delete t_pTag;
        if(t_pStream)
            delete t_pStream;
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
//                if(mri_head_t)
//                    delete mri_head_t;
                if(t_pTag)
                    delete t_pTag;
                if(t_pStream)
                    delete t_pStream;
                return false;
            }
        }
    }
    inv.mri_head_t  = mri_head_t;

    //
    // get parent MEG info
    //
    t_pStream->read_meas_info_base(t_Tree, inv.info);

    //
    //   Transform the source spaces to the correct coordinate frame
    //   if necessary
    //
    if (inv.coord_frame != FIFFV_COORD_MRI && inv.coord_frame != FIFFV_COORD_HEAD)
    {
        printf("Only inverse solutions computed in MRI or head coordinates are acceptable");
        if(t_pTag)
            delete t_pTag;
        if(t_pStream)
            delete t_pStream;
    }
    //
    //  Number of averages is initially one
    //
    inv.nave = 1;
    //
    //  We also need the SSP operator
    //
    inv.projs     = t_pStream->read_proj(t_Tree);
    //
    //  Some empty fields to be filled in later
    //
//        inv.proj      = [];      %   This is the projector to apply to the data
//        inv.whitener  = [];      %   This whitens the data
//        inv.reginv    = [];      %   This the diagonal matrix implementing
//                                 %   regularization and the inverse
//        inv.noisenorm = [];      %   These are the noise-normalization factors
    //
    if(!inv.src.transform_source_space_to(inv.coord_frame, mri_head_t))
    {
        printf("Could not transform source space.\n");
    }
    printf("\tSource spaces transformed to the inverse solution coordinate frame\n");
    //
    //   Done!
    //
    if(t_pTag)
        delete t_pTag;
    if(t_pStream)
        delete t_pStream;

    return true;
}
