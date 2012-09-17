//=============================================================================================================
/**
* @file     mne_forwardsolution.h
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
* @brief    Contains the MNEForwardSolution class declaration, which provides the forward solution including
*           the source space (MNESourceSpace).
*
*/

#ifndef MNE_FORWARDSOLUTION_H
#define MNE_FORWARDSOLUTION_H


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include "../../fiff/include/fiff_constants.h"
#include "../../fiff/include/fiff_coord_trans.h"
#include "../../fiff/include/fiff_types.h"


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "../mne_global.h"
#include "mne_sourcespace.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include "../../../include/3rdParty/Eigen/Core"
#include "../../../include/3rdParty/Eigen/SVD"
#include "../../../include/3rdParty/Eigen/Sparse"
#include "../../../include/3rdParty/Eigen/unsupported/KroneckerProduct"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDataStream>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

static QStringList defaultQStringList = QStringList();


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;


//=============================================================================================================
/**
* DECLARE CLASS SourceSpace
*
* @brief The SourceSpace class provides
*/
class MNESHARED_EXPORT MNEForwardSolution {
public:

    //=========================================================================================================
    /**
    * ctor
    */
    MNEForwardSolution();

    //=========================================================================================================
    /**
    * Copy ctor
    */
    MNEForwardSolution(MNEForwardSolution* p_pMNEForwardSolution);

    //=========================================================================================================
    /**
    * dtor
    */
    ~MNEForwardSolution();

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the mne_read_forward_solution function
    *
    * Reads a forward solution from a fif file
    *
    * @param [in] p_sFile       The name of the fif file
    * @param [out] fwd A forward solution from a fif file
    * @param [in] force_fixed   Force fixed source orientation mode? (optional)
    * @param [in] surf_ori      Use surface based source coordinate system? (optional)
    * @param [in] include       Include these channels (optional)
    * @param [in] exclude       Exclude these channels (optional)
    *
    * @return true if succeeded, false otherwise
    */
    static bool read_forward_solution(QString& p_sFile, MNEForwardSolution*& fwd, bool force_fixed = false, bool surf_ori = false, QStringList& include = defaultQStringList, QStringList& exclude = defaultQStringList);

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the mne_block_diag function - decoding part
    */
//    static inline MatrixXf extract_block_diag(MatrixXf& A, qint32 n);

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the mne_block_diag function - encoding part
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
    static inline SparseMatrix<float> make_block_diag(MatrixXf& A, qint32 n);

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the mne_transpose_named_matrix function
    *
    * Transpose a named matrix (FiffSolution)
    *
    * @param[in, out] mat FiffSolution which shoul be transposed.
    */
    static void transpose_named_matrix(FiffSolution*& mat);

private:
    //=========================================================================================================
    /**
    * Implementation of the read_one function in mne_read_forward_solution.m
    *
    * Reads all interesting stuff for one forward solution
    *
    * @param[in] p_pFile The opened fif file to read from
    * @param[in] node The forward solution node
    * @param[out] one The read forward solution
    *
    * @return True if succeeded, false otherwise
    */
    static bool read_one(QFile* p_pFile, FiffDirTree* node, MNEForwardSolution*& one);

public:
    fiff_int_t source_ori;
    fiff_int_t coord_frame;
    fiff_int_t nsource;
    fiff_int_t nchan;
    FiffSolution* sol;
    FiffSolution* sol_grad;
    FiffCoordTrans mri_head_t;
    MNESourceSpace* src;
    MatrixX3f source_rr;
    MatrixX3f source_nn;
};

} // NAMESPACE

#endif // MNE_FORWARDSOLUTION_H
