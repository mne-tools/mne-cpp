//=============================================================================================================
/**
* @file     mne_bem.h
* @author   Jana Kiesel<jana.kiesel@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
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
* @brief    MNEBEM class declaration.
*
*/

#ifndef MNE_BEM_H
#define MNE_BEM_H

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
#include <QSharedPointer>
#include <QDataStream>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FSLIB
{
class Label;
}


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

using namespace FSLIB;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
* BEM descritpion
*
* @brief BEM descritpion
*/
class MNESHARED_EXPORT MNEBem
{
public:
    typedef QSharedPointer<MNEBem> SPtr;            /**< Shared pointer type for MNEBem. */
    typedef QSharedPointer<const MNEBem> ConstSPtr; /**< Const shared pointer type for MNEBem. */

    //=========================================================================================================
    /**
    * Default constructor
    */
    MNEBem();


    //=========================================================================================================
    /**
    * Copy constructor.
    *
    * @param[in] p_MNEBem   MNE BEM
    */
    MNEBem(const MNEBem &p_MNEBem);


    //=========================================================================================================
    /**
    * Default constructor
    */
    MNEBem(QIODevice &p_IODevice);


    //=========================================================================================================
    /**
    * Destroys the MNE forward solution
    */
    ~MNEBem();

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the mne_read_bem_surface function
    *
    * Reads Bem surface from a fif file
    *
    * @param [in,out] p_pStream     The opened fif file
    * @param [in] add_geom          Add geometry information to the source spaces       Note: not used at the moment
    * @param [in, out] p_Tree       Search for the bem surface here
    *
    * @return true if succeeded, false otherwise
    */
    static bool readFromStream(FiffStream::SPtr& p_pStream, FiffDirTree& p_Tree);


private:
//    QList<MNEBemSurface> m_qListBemSurface;    /**< List of the BEM Surfaces. */
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================



} // NAMESPACE


#endif // MNE_BEM_H
