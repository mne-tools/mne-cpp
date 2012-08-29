//=============================================================================================================
/**
* @file     mne_sourcespace.h
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

#ifndef MNE_SOURCESPACE_H
#define MNE_SOURCESPACE_H

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "../mne_global.h"
#include "mne_hemisphere.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include "../../fiff/include/fiff_types.h"
#include "../../fiff/include/fiff_dir_tree.h"
#include "../../fiff/fiff.h"


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
* DECLARE CLASS SourceSpace
*
* @brief The SourceSpace class provides
*/
class MNESHARED_EXPORT MNESourceSpace {
public:

    MNESourceSpace();

    //=========================================================================================================
    /**
    * Copy ctor
    */
    MNESourceSpace(MNESourceSpace* p_pMNESourceSpace);

    //=========================================================================================================
    /**
    * dtor
    */
    ~MNESourceSpace();

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the mne_read_source_spaces function
    */
    static bool read_source_spaces(QFile*& p_pFile, bool add_geom, FiffDirTree*& p_pTree, MNESourceSpace*& p_pSourceSpace);

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the mne_find_source_space_hemi function
    */
    static qint32 find_source_space_hemi(MNEHemisphere* p_pHemisphere);

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the mne_transform_source_space_to function
    */
    void transform_source_space_to(fiff_int_t dest, FiffCoordTrans* trans);

private:
    //=========================================================================================================
    /**
    * Implementation of the read_source_space function in e.g. mne_read_source_spaces, mne_read_bem_surfaces.m
    */
    static bool read_source_space(QFile* p_pFile, FiffDirTree* p_pTree, MNEHemisphere*& p_pHemisphere);


    //=========================================================================================================
    /**
    * Generate the patch information from the 'nearest' vector in a source space
    *
    */
    static bool patch_info(VectorXi& nearest, QList<VectorXi>& pinfo);


    //=========================================================================================================
    static bool intPairComparator ( const intpair& l, const intpair& r);


    //=========================================================================================================
    static bool complete_source_space_info(MNEHemisphere* p_pHemisphere);

public:
    QList<MNEHemisphere*> hemispheres;
};


} // NAMESPACE



#endif // MNE_SOURCESPACE_H
