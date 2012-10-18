//=============================================================================================================
/**
* @file     mne.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
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
* @brief    Contains the MNE class declaration, which provides static wrapper functions to stay consistent with
*           mne matlab toolbox
*
*/

#ifndef MNE_H
#define MNE_H

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include "mne_inverse_operator.h"
#include "mne_forwardsolution.h"
#include "mne_hemisphere.h"
#include "mne_sourcespace.h"
#include "mne_cov.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include "../fiff/fiff_constants.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QMap>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

//using namespace SOURCELAB;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE MNE WRAPPER CLASS
* @brief The MNE class provides wrapper functions to stay consistent with mne matlab toolbox.
*/

class MNESHARED_EXPORT MNE
{

public:

    //=========================================================================================================
    /**
    * dtor
    */
    virtual ~MNE()
    { }


    //=========================================================================================================
    /**
    * mne_block_diag - decoding part
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the MNEForwardSolution::extract_block_diag static function
    */
    //    static inline MatrixXf extract_block_diag(MatrixXf& A, qint32 n);



    //=========================================================================================================
    /**
    * mne_get_current_comp
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffInfo get_current_comp member function
    *
    * Get the current compensation in effect in the data
    *
    * @param[in] info   Fiff measurement info
    *
    * @return the current compensation
    */
    static inline qint32 get_current_comp(FiffInfo* info)
    {
        return info->get_current_comp();
    }


    //=========================================================================================================
    /**
    * mne_block_diag - encoding part
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the MNEForwardSolution::make_block_diag static function
    *
    * Make a sparse block diagonal matrix
    *
    * Returns a sparse block diagonal, diagonalized from the elements in "A". "A" is ma x na, comprising
    * bdn=(na/"n") blocks of submatrices. Each submatrix is ma x "n", and these submatrices are placed down
    * the diagonal of the matrix.
    *
    * @param[in, out] A Matrix which should be diagonlized
    * @param[in, out] n Columns of the submatrices
    *
    * @return A sparse block diagonal, diagonalized from the elements in "A".
    */
    static inline SparseMatrix<float> make_block_diag(MatrixXf& A, qint32 n)
    {
        return MNEForwardSolution::make_block_diag(A, n);

    }


    //=========================================================================================================
    /**
    * mne_make_compensator
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffInfo make_compensator member function
    *
    * Create a compensation matrix to bring the data from one compensation state to another
    *
    * @param[in] info               measurement info as returned by the fif reading routines
    * @param[in] from               compensation in the input data
    * @param[in] to                 desired compensation in the output
    * @param[out] comp              Compensation Matrix
    * @param[in] exclude_comp_chs   exclude compensation channels from the output (optional)
    *
    * @return true if succeeded, false otherwise
    */
    inline static bool make_compensator(FiffInfo* info, fiff_int_t from, fiff_int_t to, FiffCtfComp& ctf_comp, bool exclude_comp_chs = false)
    {
        return info->make_compensator(from, to, ctf_comp, exclude_comp_chs);
    }


    //=========================================================================================================
    /**
    * make_projector
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffInfo::make_projector static function
    * There exists also a member function which should be preferred:
    * make_projector(MatrixXf& proj, MatrixXf& U = defaultUMatrix)
    *
    * Make an SSP operator
    *
    * @param[in] projs      A set of projection vectors
    * @param[in] ch_names   A cell array of channel names
    * @param[out] proj      The projection operator to apply to the data
    * @param[in] bads       Bad channels to exclude
    * @param[out] U         The orthogonal basis of the projection vectors (optional)
    *
    * @return nproj - How many items in the projector
    */
    inline static fiff_int_t make_projector(QList<FiffProj*>& projs, QStringList& ch_names, MatrixXf& proj, QStringList& bads = defaultQStringList, MatrixXf& U = defaultMatrixXf)
    {
        return FiffInfo::make_projector(projs, ch_names, proj, bads, U);
    }


    //=========================================================================================================
    /**
    * mne_make_projector_info
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffInfo make_projector_info(MatrixXf& proj) member function
    *
    * Make a SSP operator using the meas info
    *
    * @param[in] info       Fiff measurement info
    * @param[out] proj      The projection operator to apply to the data
    *
    * @return nproj - How many items in the projector
    */
    static inline qint32 make_projector_info(FiffInfo* info, MatrixXf& proj)
    {
        return info->make_projector_info(proj);
    }


    //=========================================================================================================
    /**
    * mne_find_source_space_hemi
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the MNESourceSpace::find_source_space_hemi static function
    *
    * Returns the hemisphere id (FIFFV_MNE_SURF_LEFT_HEMI or FIFFV_MNE_SURF_RIGHT_HEMI) for a source space.
    *
    * @param[in] p_pHemisphere the hemisphere to investigate
    *
    * @return the deduced hemisphere id
    */
    inline static qint32 find_source_space_hemi(MNEHemisphere* p_pHemisphere)
    {
        return MNESourceSpace::find_source_space_hemi(p_pHemisphere);
    }

    //=========================================================================================================
    /**
    * mne_patch_info
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the MNESourceSpace::patch_info static function
    *
    * @param [in] nearest   The nearest vector of the source space.
    * @param [out] pinfo    The requested patch information.
    *
    * @return true if succeeded, false otherwise
    */
    static bool patch_info(VectorXi& nearest, QList<VectorXi>& pinfo)
    {
        return MNESourceSpace::patch_info(nearest, pinfo);
    }

// ToDo Eventlist Class??
    //=========================================================================================================
    /**
    * mne_read_events
    *
    * ### MNE toolbox root function ###
    *
    * Read an event list from a fif file
    *
    * @param [in] p_sFileName   The name of the file
    * @param [out] eventlist    The read eventlist m x 3; with m events; colum: 1 - position in samples, 3 - eventcode
    *
    * @return true if succeeded, false otherwise
    */
    static bool read_events(QString& p_sFileName, MatrixXi& eventlist);



















//    function [cov] = mne_read_cov(fid,node,cov_kind)
//    %
//    % [cov] = mne_read_cov(fid,node,kind)
//    %
//    % Reads a covariance matrix from a fiff file
//    %
//    % fid       - an open file descriptor
//    % node      - look for the matrix in here
//    % cov_kind  - what kind of a covariance matrix do we want?
//    %


    static bool read_cov(FiffFile* p_pFile, FiffDirTree* node, fiff_int_t cov_kind, MNECov*& p_covData)
    {
        if(p_covData)
            delete p_covData;
        p_covData = NULL;

        //
        //   Find all covariance matrices
        //
        QList<FiffDirTree*> covs = node->dir_tree_find(FIFFB_MNE_COV);
        if (covs.size() == 0)
        {
            printf("No covariance matrices found");
            return false;
        }
        //
        //   Is any of the covariance matrices a noise covariance
        //
        qint32 p = 0;
        FiffTag* tag = NULL;
        bool success = false;
        fiff_int_t dim, nfree, nn;
        QStringList names;
        bool diagmat = false;
        VectorXd eig;
        MatrixXf eigvec;
        VectorXd* cov_diag = NULL;
        VectorXd* cov = NULL;
        VectorXd* cov_sparse = NULL;
        QStringList bads;
        for(p = 0; p < covs.size(); ++p)
        {
            success = covs[p]->find_tag(p_pFile, FIFF_MNE_COV_KIND, tag);
            if (success && *tag->toInt() == cov_kind)
            {
                FiffDirTree* current = covs[p];
                //
                //   Find all the necessary data
                //
                if (!current->find_tag(p_pFile, FIFF_MNE_COV_DIM, tag))
                {
                    printf("Covariance matrix dimension not found.\n");
                    return false;
                }
                dim = *tag->toInt();
                if (!current->find_tag(p_pFile, FIFF_MNE_COV_NFREE, tag))
                    nfree = -1;
                else
                    nfree = *tag->toInt();

                if (current->find_tag(p_pFile, FIFF_MNE_ROW_NAMES, tag))
                {
                    names = FiffFile::split_name_list(tag->toString());
                    if (names.size() != dim)
                    {
                        printf("Number of names does not match covariance matrix dimension\n");
                        return false;
                    }
                }
                if (!current->find_tag(p_pFile, FIFF_MNE_COV, tag))
                {
                    if (!current->find_tag(p_pFile, FIFF_MNE_COV_DIAG, tag))
                    {
                        printf("No covariance matrix data found\n");
                        return false;
                    }
                    else
                    {
                        //
                        //   Diagonal is stored
                        //
                        if (tag->type == FIFFT_DOUBLE)
                        {
                            cov_diag = new VectorXd(Map<VectorXd>(tag->toDouble(),dim));
                        }
                        else if (tag->type == FIFFT_FLOAT)
                        {
                            cov_diag = new VectorXd(Map<VectorXf>(tag->toFloat(),dim).cast<double>());
                        }
                        else {
                            printf("Illegal data type for covariance matrix\n");
                            return false;
                        }

                        diagmat = true;
                        printf("\t%d x %d diagonal covariance (kind = %d) found.\n", dim, dim, cov_kind);
                    }
                }
                else
                {
                    diagmat = false;
                    nn = dim*(dim+1)/2;
                    if (tag->type == FIFFT_DOUBLE)
                    {
                        cov =  new VectorXd(Map<VectorXd>(tag->toDouble(),nn));
                    }
                    else if (tag->type == FIFFT_FLOAT)
                    {
                        cov = new VectorXd(Map<VectorXf>(tag->toFloat(),nn).cast<double>());
                    }
                    else
                    {
                        qDebug() << tag->getInfo();
                        return false;
                    }


//                    if ~issparse(tag.data)
//                        //
//                        //   Lower diagonal is stored
//                        //
//                        qDebug() << tag->getInfo();
//                        vals = tag.data;
//                        data = zeros(dim,dim);
//                        % XXX : should remove for loops
//                        q = 1;
//                        for j = 1:dim
//                            for k = 1:j
//                                data(j,k) = vals(q);
//                                q = q + 1;
//                            end
//                        end
//                        for j = 1:dim
//                            for k = j+1:dim
//                                data(j,k) = data(k,j);
//                            end
//                        end
//                        diagmat = false;
//                        fprintf('\t%d x %d full covariance (kind = %d) found.\n',dim,dim,cov_kind);
//                    else
//                        diagmat = false;
//                        data = tag.data;
//                        fprintf('\t%d x %d sparse covariance (kind = %d) found.\n',dim,dim,cov_kind);
//                    end
                }
                //
                //   Read the possibly precomputed decomposition
                //
                FiffTag* tag1 = NULL;
                FiffTag* tag2 = NULL;
                if (current->find_tag(p_pFile, FIFF_MNE_COV_EIGENVALUES, tag1) && current->find_tag(p_pFile, FIFF_MNE_COV_EIGENVECTORS, tag2))
                {
                    eig =       VectorXd(Map<VectorXd>(tag1->toDouble(),dim));
                    eigvec =    tag2->toFloatMatrix();
                }
                //
                //   Read the projection operator
                //
                QList<FiffProj*> projs = p_pFile->read_proj(current);
                //
                //   Read the bad channel list
                //
                bads = p_pFile->read_bad_channels(current);
                //
                //   Put it together
                //
                p_covData = new MNECov();

                p_covData->kind   = cov_kind;
                p_covData->diag   = diagmat;
                p_covData->dim    = dim;
                p_covData->names  = names;

                if(cov_diag)
                    p_covData->data   = MatrixXd(*cov_diag);
                else if(cov)
                    p_covData->data   = MatrixXd(*cov);
                else if(cov_sparse)
                    p_covData->data   = MatrixXd(*cov_sparse);

                p_covData->projs  = projs;
                p_covData->bads   = bads;
                p_covData->nfree  = nfree;
                p_covData->eig    = eig;
                p_covData->eigvec = eigvec;

                if (cov_diag)
                    delete cov_diag;
                if (cov)
                    delete cov;
                if (cov_sparse)
                    delete cov_sparse;

                if (tag)
                    delete tag;
                //
                return true;
            }
        }

        printf("Did not find the desired covariance matrix\n");
        return false;
    }































//    function [inv] = mne_read_inverse_operator(fname)
//    %
//    % [inv] = mne_read_inverse_operator(fname)
//    %
//    % Reads the inverse operator decomposition from a fif file
//    %
//    % fname        - The name of the file
//    %

    static bool read_inverse_operator(QString& p_sFileName)
    {
        //
        //   Open the file, create directory
        //
        printf("Reading inverse operator decomposition from %s...\n",p_sFileName.toUtf8().constData());
        FiffFile* t_pFile = new FiffFile(p_sFileName);
        FiffDirTree* t_pTree = NULL;
        QList<FiffDirEntry>* t_pDir = NULL;

        if(!t_pFile->open(t_pTree, t_pDir))
        {
            if(t_pFile)
                delete t_pFile;
            if(t_pTree)
                delete t_pTree;
            if(t_pDir)
                delete t_pDir;

            return false;
        }
        //
        //   Find all inverse operators
        //
        QList <FiffDirTree *> invs_list = t_pTree->dir_tree_find(FIFFB_MNE_INVERSE_SOLUTION);
        if ( invs_list.size()== 0)
        {
            printf("No inverse solutions in %s\n", p_sFileName.toUtf8().constData());
            if(t_pFile)
                delete t_pFile;
            if(t_pTree)
                delete t_pTree;
            if(t_pDir)
                delete t_pDir;
            return false;
        }
        FiffDirTree* invs = invs_list[0];
        //
        //   Parent MRI data
        //
        QList <FiffDirTree *> parent_mri = t_pTree->dir_tree_find(FIFFB_MNE_PARENT_MRI_FILE);
        if (parent_mri.size() == 0)
        {
            printf("No parent MRI information in %s", p_sFileName.toUtf8().constData());
            if(t_pFile)
                delete t_pFile;
            if(t_pTree)
                delete t_pTree;
            if(t_pDir)
                delete t_pDir;
            return false;
        }
        printf("\tReading inverse operator info...");
        //
        //   Methods and source orientations
        //
        FiffTag* tag = NULL;
        invs->find_tag(t_pFile, FIFF_MNE_INCLUDED_METHODS, tag);
        if (tag->data == NULL)
        {
            printf("Modalities not found\n");
            if(t_pFile)
                delete t_pFile;
            if(t_pTree)
                delete t_pTree;
            if(t_pDir)
                delete t_pDir;
            return false;
        }
        MNEInverseOperator* inv = new MNEInverseOperator();
        inv->methods = *tag->toInt();
        //
        invs->find_tag(t_pFile, FIFF_MNE_SOURCE_ORIENTATION, tag);
        if (tag->data == NULL)
        {
            printf("Source orientation constraints not found\n");
            if(t_pFile)
                delete t_pFile;
            if(t_pTree)
                delete t_pTree;
            if(t_pDir)
                delete t_pDir;
            return false;
        }
        inv->source_ori = *tag->toInt();
        //
        invs->find_tag(t_pFile, FIFF_MNE_SOURCE_SPACE_NPOINTS, tag);
        if (tag->data == NULL)
        {
            printf("Number of sources not found\n");
            if(t_pFile)
                delete t_pFile;
            if(t_pTree)
                delete t_pTree;
            if(t_pDir)
                delete t_pDir;
            return false;
        }
        inv->nsource = *tag->toInt();
        inv->nchan   = 0;
        //
        //   Coordinate frame
        //
        invs->find_tag(t_pFile, FIFF_MNE_COORD_FRAME, tag);
        if (tag == NULL)
        {
            printf("Coordinate frame tag not found\n");
            if(t_pFile)
                delete t_pFile;
            if(t_pTree)
                delete t_pTree;
            if(t_pDir)
                delete t_pDir;
            return false;
        }
        inv->coord_frame = *tag->toInt();
        //
        //   The actual source orientation vectors
        //
        invs->find_tag(t_pFile, FIFF_MNE_INVERSE_SOURCE_ORIENTATIONS, tag);
        if (tag->data == NULL)
        {
            printf("Source orientation information not found\n");
            if(t_pFile)
                delete t_pFile;
            if(t_pTree)
                delete t_pTree;
            if(t_pDir)
                delete t_pDir;
            return false;
        }

        if(inv->source_nn)
            delete inv->source_nn;
        inv->source_nn = new MatrixXf(tag->toFloatMatrix().transpose());

        printf("[done]\n");
        //
        //   The SVD decomposition...
        //
        printf("\tReading inverse operator decomposition...");
        invs->find_tag(t_pFile, FIFF_MNE_INVERSE_SING, tag);
        if (tag == NULL)
        {
            printf("Singular values not found\n");
            if(t_pFile)
                delete t_pFile;
            if(t_pTree)
                delete t_pTree;
            if(t_pDir)
                delete t_pDir;
            return false;
        }

        if(inv->sing)
            delete inv->sing;
        inv->sing = new VectorXf(Map<VectorXf>(tag->toFloat(), tag->size/4));
        inv->nchan = inv->sing->rows();
        //
        //   The eigenleads and eigenfields
        //
        inv->eigen_leads_weighted = false;
        if(!Fiff::read_named_matrix(t_pFile, invs, FIFF_MNE_INVERSE_LEADS, inv->eigen_leads))
        {
            inv->eigen_leads_weighted = true;
            if(!Fiff::read_named_matrix(t_pFile, invs, FIFF_MNE_INVERSE_LEADS_WEIGHTED, inv->eigen_leads))
            {
                printf("Error reading eigenleads named matrix.\n");
                if(t_pFile)
                    delete t_pFile;
                if(t_pTree)
                    delete t_pTree;
                if(t_pDir)
                    delete t_pDir;
                return false;
            }
        }
        //
        //   Having the eigenleads as columns is better for the inverse calculations
        //
        inv->eigen_leads->transpose_named_matrix();


        if(!Fiff::read_named_matrix(t_pFile, invs, FIFF_MNE_INVERSE_FIELDS, inv->eigen_fields))
        {
            printf("Error reading eigenfields named matrix.\n");
            if(t_pFile)
                delete t_pFile;
            if(t_pTree)
                delete t_pTree;
            if(t_pDir)
                delete t_pDir;
            return false;
        }
        printf("[done]\n");
        //
        //   Read the covariance matrices
        //
        MNE::read_cov(t_pFile,invs,FIFFV_MNE_NOISE_COV, inv->noise_cov);
//        try
//            inv.noise_cov = mne_read_cov(fid,invs,FIFF.FIFFV_MNE_NOISE_COV);
//            fprintf('\tNoise covariance matrix read.\n');
//        catch
//            fclose(fid);
//            error(me,'%s',mne_omit_first_line(lasterr));
//        end
//        try
//            inv.source_cov = mne_read_cov(fid,invs,FIFF.FIFFV_MNE_SOURCE_COV);
//            fprintf('\tSource covariance matrix read.\n');
//        catch
//            fclose(fid);
//            error(me,'%s',mne_omit_first_line(lasterr));
//        end
//        %
//        %   Read the various priors
//        %
//        try
//            inv.orient_prior = mne_read_cov(fid,invs,FIFF.FIFFV_MNE_ORIENT_PRIOR_COV);
//            fprintf('\tOrientation priors read.\n');
//        catch
//            inv.orient_prior = [];
//        end
//        try
//            inv.depth_prior = mne_read_cov(fid,invs,FIFF.FIFFV_MNE_DEPTH_PRIOR_COV);
//            fprintf('\tDepth priors read.\n');
//        catch
//            inv.depth_prior = [];
//        end
//        try
//            inv.fmri_prior = mne_read_cov(fid,invs,FIFF.FIFFV_MNE_FMRI_PRIOR_COV);
//            fprintf('\tfMRI priors read.\n');
//        catch
//            inv.fmri_prior = [];
//        end
//        %
//        %   Read the source spaces
//        %
//        try
//            inv.src = mne_read_source_spaces(fid,false,tree);
//        catch
//            fclose(fid);
//            error(me,'Could not read the source spaces (%s)',mne_omit_first_line(lasterr));
//        end
//        for k = 1:length(inv.src)
//           inv.src(k).id = mne_find_source_space_hemi(inv.src(k));
//        end
//        %
//        %   Get the MRI <-> head coordinate transformation
//        %
//        tag = find_tag(parent_mri,FIFF.FIFF_COORD_TRANS);
//        if isempty(tag)
//            fclose(fid);
//            error(me,'MRI/head coordinate transformation not found');
//        else
//            mri_head_t = tag.data;
//            if mri_head_t.from ~= FIFF.FIFFV_COORD_MRI || mri_head_t.to ~= FIFF.FIFFV_COORD_HEAD
//                mri_head_t = fiff_invert_transform(mri_head_t);
//                if mri_head_t.from ~= FIFF.FIFFV_COORD_MRI || mri_head_t.to ~= FIFF.FIFFV_COORD_HEAD
//                    fclose(fid);
//                    error(me,'MRI/head coordinate transformation not found');
//                end
//            end
//        end
//        inv.mri_head_t  = mri_head_t;
//        %
//        %   Transform the source spaces to the correct coordinate frame
//        %   if necessary
//        %
//        if inv.coord_frame ~= FIFF.FIFFV_COORD_MRI && ...
//                inv.coord_frame ~= FIFF.FIFFV_COORD_HEAD
//            fclose(fid);
//            error(me,'Only inverse solutions computed in MRI or head coordinates are acceptable');
//        end
//        %
//        %  Number of averages is initially one
//        %
//        inv.nave = 1;
//        %
//        %  We also need the SSP operator
//        %
//        inv.projs     = fiff_read_proj(fid,tree);
//        %
//        %  Some empty fields to be filled in later
//        %
//        inv.proj      = [];      %   This is the projector to apply to the data
//        inv.whitener  = [];      %   This whitens the data
//        inv.reginv    = [];      %   This the diagonal matrix implementing
//                                 %   regularization and the inverse
//        inv.noisenorm = [];      %   These are the noise-normalization factors
//        %
//        nuse = 0;
//        for k = 1:length(inv.src)
//           try
//              inv.src(k) = mne_transform_source_space_to(inv.src(k),inv.coord_frame,mri_head_t);
//           catch
//              fclose(fid);
//              error(me,'Could not transform source space (%s)',mne_omit_first_line(lasterr));
//           end
//           nuse = nuse + inv.src(k).nuse;
//        end
//        fprintf(1,'\tSource spaces transformed to the inverse solution coordinate frame\n');
//        %
//        %   Done!
//        %
//        fclose(fid);

        return true;
    }










    //=========================================================================================================
    /**
    * mne_read_forward_solution
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the MNEForwardSolution::read_forward_solution static function
    *
    * Reads a forward solution from a fif file
    *
    * @param [in] p_sFile       The name of the file
    * @param [out] fwd A forward solution from a fif file
    * @param [in] force_fixed   Force fixed source orientation mode? (optional)
    * @param [in] surf_ori      Use surface based source coordinate system? (optional)
    * @param [in] include       Include these channels (optional)
    * @param [in] exclude       Exclude these channels (optional)
    *
    * @return true if succeeded, false otherwise
    */
    static inline bool read_forward_solution(QString& p_sFile, MNEForwardSolution*& fwd, bool force_fixed = false, bool surf_ori = false, QStringList& include = defaultQStringList, QStringList& exclude = defaultQStringList)
    {
        return MNEForwardSolution::read_forward_solution(p_sFile, fwd, force_fixed, surf_ori, include, exclude);
    }

    //=========================================================================================================
    /**
    * mne_read_forward_solution
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the MNESourceSpace::read_source_spaces static function
    *
    * Reads source spaces from a fif file
    *
    * @param [in] p_pFile   The open fiff file
    * @param [in] add_geom  Add geometry information to the source spaces
    * @param [in] p_pTree   Search for the source spaces here
    *
    * @param [out] p_pSourceSpace     The read source spaces
    *
    * @return true if succeeded, false otherwise
    */
    static bool read_source_spaces(FiffFile*& p_pFile, bool add_geom, FiffDirTree*& p_pTree, MNESourceSpace*& p_pSourceSpace)
    {
        return MNESourceSpace::read_source_spaces(p_pFile, add_geom, p_pTree, p_pSourceSpace);
    }

    //ToDo FiffChInfoList Class
    //=========================================================================================================
    /**
    * mne_set_current_comp
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffInfo::set_current_comp static function
    * Consider taking the member function of a FiffInfo set_current_comp(fiff_int_t value),
    * when compensation should be applied to the channels of FiffInfo
    *
    * Set the current compensation value in the channel info structures
    *
    * @param[in] chs    fiff channel info list
    * @param[in] value  compensation value
    *
    * @return the current compensation
    */
    static QList<FiffChInfo> set_current_comp(QList<FiffChInfo>& chs, fiff_int_t value)
    {
        return FiffInfo::set_current_comp(chs, value);
    }

    //=========================================================================================================
    /**
    * mne_transform_source_space_to
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the MNESourceSpace transform_source_space_to member function
    *
    * Transforms source space data to the desired coordinate system
    *
    * @param [in, out] p_pMNESourceSpace the source space which is should be transformed
    * @param [in] dest destination check code
    * @param [in] trans transformation information
    */
    static inline void transform_source_space_to(MNESourceSpace* p_pMNESourceSpace, fiff_int_t dest, FiffCoordTrans* trans)
    {
        return p_pMNESourceSpace->transform_source_space_to(dest, trans);
    }

    //=========================================================================================================
    /**
    * mne_transpose_named_matrix
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffNamedMatrix transpose_named_matrix member  function
    *
    * Transpose a named matrix (FiffNamedMatrix)
    *
    * @param[in, out] mat FiffNamedMatrix which shoul be transposed.
    *
    */
    static inline void transpose_named_matrix(FiffNamedMatrix* mat)
    {
        mat->transpose_named_matrix();
    }

private:



};


} // NAMESPACE

#endif // MNE_H
