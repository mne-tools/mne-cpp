//=============================================================================================================
/**
* @file     mne_surface.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the MNESurface class.
*
*/

#ifndef MNE_SURFACE_H
#define MNE_SURFACE_H

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "mne_global.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_types.h>
#include <fiff/fiff_dir_tree.h>
#include <fiff/fiff_stream.h>


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
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
* BEM Surface
*
* @brief BEM Surface
*/
class MNESHARED_EXPORT MNESurface
{
public:
    typedef QSharedPointer<MNESurface> SPtr;            /**< Shared pointer type for MNESurface. */
    typedef QSharedPointer<const MNESurface> ConstSPtr; /**< Const shared pointer type for MNESurface. */

    //=========================================================================================================
    /**
    * Default constructor
    */
    MNESurface();

    //=========================================================================================================
    /**
    * Reads a bem surface from a fif IO device
    *
    * @param[in] p_IODevice     A fiff IO device like a fiff QFile or QTCPSocket
    * @param[out] surf          A forward solution from a fif file
    *
    * @return true if succeeded, false otherwise
    */
    static bool read(QIODevice& p_IODevice, MNESurface& surf);

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the mne_read_bem_surfaces function
    *
    * Reads a BEM surface from a fif stream
    *
    * @param [in] p_pStream     The open fiff file
    * @param [in] add_geom      Add geometry information to the source spaces
    * @param [in] p_Tree        Search for the source spaces here
    *
    * @param [out] p_Surface    The read bem surface
    *
    * @return true if succeeded, false otherwise
    */
    static bool read(FiffStream::SPtr& p_pStream, bool add_geom, FiffDirTree& p_Tree, MNESurface& p_Surface);

public:


};

} // NAMESPACE

#endif // MNE_SURFACE_H
