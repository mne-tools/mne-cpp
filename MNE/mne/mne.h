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
* @brief    ToDo Documentation...
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
    * ctor
    */
    MNE();

    //=========================================================================================================
    /**
    * dtor
    */
    ~MNE()
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
    */
    static inline SparseMatrix<float> make_block_diag(MatrixXf& A, qint32 n)
    {
        return MNEForwardSolution::make_block_diag(A, n);

    }

    //=========================================================================================================
    /**
    * mne_find_source_space_hemi
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the MNESourceSpace::find_source_space_hemi static function
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
    static bool read_source_spaces(QFile*& p_pFile, bool add_geom, FiffDirTree*& p_pTree, MNESourceSpace*& p_pSourceSpace)
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
    */
    static inline void transpose_named_matrix(FiffSolution*& mat)
    {
        return MNEForwardSolution::transpose_named_matrix(mat);
    }

private:



};


} // NAMESPACE

#endif // MNE_H
