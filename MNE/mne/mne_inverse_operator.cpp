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
* @brief    Contains the implementation of the MNEInverseOperator Class.
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
//, source_nn(MatrixXd())//NULL)
//, sing(NULL)
, eigen_leads_weighted(false)
, eigen_leads(new FiffNamedMatrix)
, eigen_fields(new FiffNamedMatrix)
, noise_cov(new FiffCov)
, source_cov(new FiffCov)
, orient_prior(new FiffCov)
, depth_prior(new FiffCov)
, fmri_prior(new FiffCov)
//, src(NULL)
//, mri_head_t(NULL)
, nave(-1)
//, proj(NULL)
//, whitener(NULL)
//, reginv(NULL)
//, noisenorm(NULL)
{
}


//*************************************************************************************************************

MNEInverseOperator::MNEInverseOperator(const MNEInverseOperator* p_pMNEInverseOperator)
: methods(p_pMNEInverseOperator->methods)
, source_ori(p_pMNEInverseOperator->source_ori)
, nsource(p_pMNEInverseOperator->nsource)
, nchan(p_pMNEInverseOperator->nchan)
, coord_frame(p_pMNEInverseOperator->coord_frame)
, source_nn(p_pMNEInverseOperator->source_nn)//p_pMNEInverseOperator->source_nn ? new MatrixXd(*p_pMNEInverseOperator->source_nn) : NULL)
, sing(p_pMNEInverseOperator->sing)//p_pMNEInverseOperator->sing ? new VectorXd(*p_pMNEInverseOperator->sing) : NULL)
, eigen_leads_weighted(p_pMNEInverseOperator->eigen_leads_weighted)
, eigen_leads(p_pMNEInverseOperator->eigen_leads)//p_pMNEInverseOperator->eigen_leads ? new FiffNamedMatrix(p_pMNEInverseOperator->eigen_leads) : NULL)
, eigen_fields(p_pMNEInverseOperator->eigen_fields)//p_pMNEInverseOperator->eigen_fields ? new FiffNamedMatrix(p_pMNEInverseOperator->eigen_fields) : NULL)
, noise_cov(p_pMNEInverseOperator->noise_cov)
, source_cov(p_pMNEInverseOperator->source_cov)
, orient_prior(p_pMNEInverseOperator->orient_prior)
, depth_prior(p_pMNEInverseOperator->depth_prior)
, fmri_prior(p_pMNEInverseOperator->fmri_prior)
, src(p_pMNEInverseOperator->src) //p_pMNEInverseOperator->src ? new MNESourceSpace(p_pMNEInverseOperator->src) : NULL)
, mri_head_t(p_pMNEInverseOperator->mri_head_t)//p_pMNEInverseOperator->mri_head_t ? new FiffCoordTrans(p_pMNEInverseOperator->mri_head_t) : NULL)
, nave(p_pMNEInverseOperator->nave)
, projs(p_pMNEInverseOperator->projs)
, proj(p_pMNEInverseOperator->proj)//p_pMNEInverseOperator->proj ? new MatrixXd(*p_pMNEInverseOperator->proj) : NULL)
, whitener(p_pMNEInverseOperator->whitener)//p_pMNEInverseOperator->whitener ? new MatrixXd(*p_pMNEInverseOperator->whitener) : NULL)
, reginv(p_pMNEInverseOperator->reginv)//p_pMNEInverseOperator->reginv ? new VectorXd(*p_pMNEInverseOperator->reginv) : NULL)
, noisenorm(p_pMNEInverseOperator->noisenorm)//p_pMNEInverseOperator->noisenorm ? new SparseMatrix<double>(*p_pMNEInverseOperator->noisenorm) : NULL)
{

}


//*************************************************************************************************************

MNEInverseOperator::~MNEInverseOperator()
{
//    if(source_nn)
//        delete source_nn;
//    if(sing)
//        delete sing;
//    if(eigen_leads)
//        delete eigen_leads;
//    if(eigen_fields)
//        delete eigen_fields;
//    if(noise_cov)
//        delete noise_cov;
//    if(source_cov)
//        delete source_cov;
//    if(orient_prior)
//        delete orient_prior;
//    if(depth_prior)
//        delete depth_prior;
//    if(fmri_prior)
//        delete fmri_prior;
//    if(src)
//        delete src;
//    if(mri_head_t)
//        delete mri_head_t;
//    if(proj)
//        delete proj;
//    if(whitener)
//        delete whitener;
//    if(reginv)
//        delete reginv;
//    if(noisenorm)
//        delete noisenorm;
}


//*************************************************************************************************************

MNEInverseOperator MNEInverseOperator::prepare_inverse_operator(qint32 nave ,float lambda2, bool dSPM, bool sLORETA)
{
    if(nave <= 0)
    {
        printf("The number of averages should be positive\n");
        return NULL;
    }
    printf("Preparing the inverse operator for use...\n");
    MNEInverseOperator inv(this);
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

    qint32 ncomp = FiffInfo::make_projector(inv.projs, inv.noise_cov->names, inv.proj);
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
        inv.noisenorm = SparseMatrix<double>(noise_norm_new.size(),noise_norm_new.size());

        for(qint32 i = 0; i < noise_norm_new.size(); ++i)
            inv.noisenorm.insert(i,i) = tmp[i];

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
//        if(t_pTree)
//            delete t_pTree;
//        if(t_pDir)
//            delete t_pDir;

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
//        if(t_pTree)
//            delete t_pTree;
//        if(t_pDir)
//            delete t_pDir;
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
//        if(t_pTree)
//            delete t_pTree;
//        if(t_pDir)
//            delete t_pDir;
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
//        if(t_pTree)
//            delete t_pTree;
//        if(t_pDir)
//            delete t_pDir;
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
//        if(t_pTree)
//            delete t_pTree;
//        if(t_pDir)
//            delete t_pDir;
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
//        if(t_pTree)
//            delete t_pTree;
//        if(t_pDir)
//            delete t_pDir;
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
//        if(t_pTree)
//            delete t_pTree;
//        if(t_pDir)
//            delete t_pDir;
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
//        if(t_pTree)
//            delete t_pTree;
//        if(t_pDir)
//            delete t_pDir;
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
//        if(t_pTree)
//            delete t_pTree;
//        if(t_pDir)
//            delete t_pDir;
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
//            if(t_pTree)
//                delete t_pTree;
//            if(t_pDir)
//                delete t_pDir;
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
//        if(t_pTree)
//            delete t_pTree;
//        if(t_pDir)
//            delete t_pDir;
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
//        if(t_pTree)
//            delete t_pTree;
//        if(t_pDir)
//            delete t_pDir;
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
//        if(t_pTree)
//            delete t_pTree;
//        if(t_pDir)
//            delete t_pDir;
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
//        if(t_pTree)
//            delete t_pTree;
//        if(t_pDir)
//            delete t_pDir;
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
//        if(t_pTree)
//            delete t_pTree;
//        if(t_pDir)
//            delete t_pDir;
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
//                if(t_pTree)
//                    delete t_pTree;
//                if(t_pDir)
//                    delete t_pDir;
                return false;
            }
        }
    }
    inv.mri_head_t  = mri_head_t;
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
//        if(t_pTree)
//            delete t_pTree;
//        if(t_pDir)
//            delete t_pDir;
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
//    if(t_pTree)
//        delete t_pTree;
//    if(t_pDir)
//        delete t_pDir;

    return true;
}
