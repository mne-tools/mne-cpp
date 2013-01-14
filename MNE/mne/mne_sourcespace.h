//=============================================================================================================
/**
* @file     mne_sourcespace.h
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
* @brief    MNESourceSpace class declaration.
*
*/

#ifndef MNE_SOURCESPACE_H
#define MNE_SOURCESPACE_H

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_hemisphere.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_types.h>
#include <fiff/fiff_dir_tree.h>
#include <fiff/fiff.h>


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <vector>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QFile>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNE
//=============================================================================================================

namespace MNELIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// TYPEDEFS
//=============================================================================================================

typedef std::pair<int,int> intpair;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
* Source Space descritpion
*
* @brief Source Space descritpion
*/
class MNESHARED_EXPORT MNESourceSpace
{
public:
    typedef QSharedPointer<MNESourceSpace> SPtr;            /**< Shared pointer type for MNESourceSpace. */
    typedef QSharedPointer<const MNESourceSpace> ConstSPtr; /**< Const shared pointer type for MNESourceSpace. */

    //=========================================================================================================
    /**
    * Default constructor
    */
    MNESourceSpace();

    //=========================================================================================================
    /**
    * Copy constructor.
    *
    * @param[in] p_MNESourceSpace   MNE forward solution
    */
    MNESourceSpace(const MNESourceSpace &p_MNESourceSpace);

    //=========================================================================================================
    /**
    * Destroys the MNE forward solution
    */
    ~MNESourceSpace();

    //=========================================================================================================
    /**
    * Initializes MNE source space.
    */
    void clear();

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the mne_find_source_space_hemi function
    *
    * Returns the hemisphere id ( FIFFV_MNE_SURF_LEFT_HEMI or FIFFV_MNE_SURF_RIGHT_HEMI) for a source space.
    *
    * @param[in] p_pHemisphere the hemisphere to investigate
    *
    * @return the deduced hemisphere id
    */
    static qint32 find_source_space_hemi(MNEHemisphere& p_Hemisphere);

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the mne_patch_info function
    *
    * Generate the patch information from the 'nearest' vector in a source space
    *
    * @param [in] nearest   The nearest vector of the source space.
    * @param [out] pinfo    The requested patch information.
    *
    * @return true if succeeded, false otherwise
    */
    static bool patch_info(VectorXi& nearest, QList<VectorXi>& pinfo);

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the mne_read_source_spaces function
    *
    * Reads source spaces from a fif file
    *
    * @param [in] p_pStream         The opened fif file
    * @param [in] add_geom          Add geometry information to the source spaces
    * @param [in, out] p_Tree       Search for the source spaces here
    * @param [out] p_pSourceSpace   the read source spaces
    *
    * @return true if succeeded, false otherwise
    */
    static bool read_source_spaces(FiffStream*& p_pStream, bool add_geom, FiffDirTree& p_Tree, MNESourceSpace& p_SourceSpace);

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the mne_transform_source_space_to function
    * Wrapper for the MNESourceSpace transform_source_space_to member function
    *
    * Note: In difference to mne-matlab this is not a static function. This is a method of the MNESourceSpace
    *       class, that's why a tree object doesn't need to be handed to the function.
    *
    * Transforms source space data to the desired coordinate system
    *
    * @param [in] dest destination check code
    * @param [in] trans transformation information
    *
    * @return true if succeeded, false otherwise
    */
    bool transform_source_space_to(fiff_int_t dest, FiffCoordTrans& trans);

private:
    //=========================================================================================================
    /**
    * Implementation of the read_source_space function in e.g. mne_read_source_spaces.m, mne_read_bem_surfaces.m
    *
    * Reads a single source space (hemisphere)
    *
    * @param [in] p_pStream         The opened fif file
    * @param [in] p_Tree            Search for the source space here
    * @param [out] p_pHemisphere    The read source space (hemisphere)
    *
    * @return true if succeeded, false otherwise
    */
    static bool read_source_space(FiffStream* p_pStream, const FiffDirTree& p_Tree, MNEHemisphere& p_Hemisphere);

    //=========================================================================================================
    /**
    * Compeartor of two int pairs
    *
    * @param [in]   l   pair one
    * @param [in]   r   pair two
    *
    * @return true if pair one is bigger, false otherwise
    */
    static bool intPairComparator ( const intpair& l, const intpair& r);

    //=========================================================================================================
    /**
    * Implementation of the complete_source_space_info function in e.g. mne_read_source_spaces.m, mne_read_bem_surfaces.m
    *
    * Completes triangulation info
    *
    * @param [in, out] p_pHemisphere   Hemisphere to be completed
    *
    * @return true if succeeded, false otherwise
    */
    static bool complete_source_space_info(MNEHemisphere& p_Hemisphere);

public:
    QList<MNEHemisphere> hemispheres;   /**< List of the hemispheres containing the source space information. */
};

} // NAMESPACE


#endif // MNE_SOURCESPACE_H
