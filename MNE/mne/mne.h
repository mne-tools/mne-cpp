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

#include "include/mne_forwardsolution.h"
#include "include/mne_hemisphere.h"
#include "include/mne_sourcespace.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include "../fiff/include/fiff_constants.h"


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
    * mne_make_projector_info
    *
    * ### MNE toolbox root function ###
    *
    * Make a SSP operator using the meas info
    *
    * @param[in] info A Matrix which should be diagonlized
    *
    * @return
    */
    static qint32 make_projector_info(FiffInfo* info)
    {

//        [ proj, nproj ] = mne_make_projector(info.projs,info.ch_names,info.bads);

        return MNE::make_projector(info->projs,info->ch_names,info->bads);

    }






    /**
    * mne_make_projector
    *
    * ### MNE toolbox root function ###
    *
    * Make a SSP operator using the meas info
    *
    * @param[in] info A Matrix which should be diagonlized
    *
    * @return
    */






//    %
//    % [proj,nproj,U] = mne_make_projector(projs,ch_names,bads)
//    %
//    % proj     - The projection operator to apply to the data
//    % nproj    - How many items in the projector
//    % U        - The orthogonal basis of the projection vectors (optional)
//    %
//    % Make an SSP operator
//    %
//    % projs    - A set of projection vectors
//    % ch_names - A cell array of channel names
//    % bads     - Bad channels to exclude
//    %

//    function [proj,nproj,U] =


    static fiff_int_t make_projector(QList<FiffProj*>& projs, QStringList& ch_names, QStringList& bads = defaultQStringList)
    {
        fiff_int_t nchan = ch_names.size();
        if (nchan == 0)
        {
            printf("No channel names specified\n");
            return -1;
        }

        MatrixXf proj = MatrixXf::Identity(nchan,nchan);
        fiff_int_t nproj = 0;
//        U     = [];
        //
        //   Check trivial cases first
        //
        if (projs.size() == 0)
            return -1;

        fiff_int_t nactive = 0;
        fiff_int_t nvec    = 0;
        fiff_int_t k, l;
        for (k = 0; k < projs.size(); ++k)
        {
            if (projs[k]->active)
            {
                ++nactive;
                nvec += projs[k]->data->nrow;
            }
        }

        if (nactive == 0)
            return -1;

        //
        //   Pick the appropriate entries
        //
        MatrixXf vecs = MatrixXf::Zero(nchan,nvec);
        nvec = 0;
        fiff_int_t nonzero = 0;
        qint32 p, c, i, j, v;
        double onesize;
        bool isBad = false;
        MatrixXi sel(1, nchan);
        MatrixXi vecSel(1, nchan);
        sel.setConstant(-1);
        vecSel.setConstant(-1);
        for (k = 0; k < projs.size(); ++k)
        {
            sel.resize(1, nchan);
            vecSel.resize(1, nchan);
            sel.setConstant(-1);
            vecSel.setConstant(-1);
            if (projs[k]->active)
            {
                FiffProj* one = projs[k];
//                sel = [];
//                vecsel = [];

                QMap<QString, int> uniqueMap;
                for(l = 0; l < one->data->col_names.size(); ++l)
                    uniqueMap[one->data->col_names[l] ] = 0;

                if (one->data->col_names.size() != uniqueMap.keys().size())
                {
                    printf("Channel name list in projection item %d contains duplicate items");
                    return -1;
                }

                //
                // Get the two selection vectors to pick correct elements from
                // the projection vectors omitting bad channels
                //
                p = 0;
                for (c = 0; c < nchan; ++c)
                {
                    for (i = 0; i < one->data->col_names.size(); ++i)
                    {
                        if (QString::compare(ch_names.at(c),one->data->col_names.at(i)) == 0)
                        {
                            isBad = false;
                            for (j = 0; j < bads.size(); ++j)
                            {
                                if (QString::compare(ch_names.at(c),bads.at(j)) == 0)
                                {
                                    isBad = true;
                                }
                            }

                            if (!isBad && sel(0,p) != c)
                            {
                                sel(0,p) = c;
                                vecSel(0, p) = i;
                                ++p;
                            }

                        }
                    }
                }
                sel.conservativeResize(1, p);
                vecSel.conservativeResize(1, p);
                //
                // If there is something to pick, pickit
                //
                if (sel.cols() > 0)
                {
                    for (v = 0; v < one->data->nrow; ++v)
                        for (i = 0; i < p; ++i)
                            vecs(sel(0,i),nvec+v) = one->data->data(v,vecSel(i));
                    //
                    //   Rescale for more straightforward detection of small singular values
                    //

                    for (v = 0; v < one->data->nrow; ++v)
                    {
                        onesize = sqrt((vecs.col(nvec+v).transpose()*vecs.col(nvec+v))(0,0));
                        if (onesize > 0.0)
                        {
                            vecs.col(nvec+v) = vecs.col(nvec+v)/onesize;
                            ++nonzero;
                        }
                    }
                    nvec += one->data->nrow;
                }
            }
        }
        //
        //   Check whether all of the vectors are exactly zero
        //
        if (nonzero == 0)
            return -1;

        //
        //   Reorthogonalize the vectors
        //
        qDebug() << "Attention Jacobi SVD is used, not the MATLAB lapack version. Since the SVD is not unique the results might be a bit different!";
        JacobiSVD<MatrixXf> svd(vecs.block(0,0,vecs.rows(),nvec), ComputeThinU);

        VectorXf S = svd.singularValues();

        //
        //   Throw away the linearly dependent guys
        //
        for(k = 0; k < nvec; ++k)
        {
            if (S(k)/S(0) < 1e-2)
            {
                nvec = k+1;
                break;
            }

        }

        MatrixXf U = svd.matrixU().block(0, 0, vecs.rows(), nvec);

        //
        //   Here is the celebrated result
        //
        proj  = proj - U*U.transpose();
        nproj = nvec;

        return nproj;
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
    static qint32 find_source_space_hemi(MNEHemisphere* p_pHemisphere)
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
    * Wrapper for the MNEForwardSolution::transpose_named_matrix static function
    *
    * Transpose a named matrix (FiffNamedMatrix)
    *
    * @param[in, out] mat FiffNamedMatrix which shoul be transposed.
    *
    */
    static inline void transpose_named_matrix(FiffNamedMatrix*& mat)
    {
        return MNEForwardSolution::transpose_named_matrix(mat);
    }

private:



};


} // NAMESPACE

#endif // MNE_H
