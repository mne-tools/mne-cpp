//=============================================================================================================
/**
* @file     fiff_coord_trans.h
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
* @brief    Contains the FiffCoordTrans class declaration.
*
*/

#ifndef FIFF_COORD_TRANS_H
#define FIFF_COORD_TRANS_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/LU>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//=============================================================================================================
/**
* Replaces fiffCoordTransRec which had a size of 104
*
* @brief Coordinate transformation description.
*/
class FIFFSHARED_EXPORT FiffCoordTrans {

public:
    //=========================================================================================================
    /**
    * ctor
    */
    FiffCoordTrans();

    //=========================================================================================================
    /**
    * copy ctor
    */
    FiffCoordTrans(const FiffCoordTrans* t_pFiffCoordTrans);

    //=========================================================================================================
    /**
    * Destroys the fiffTag.
    */
    ~FiffCoordTrans();

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: implementation of the fiff_invert_transform function
    *
    * Invert a coordinate transformation
    * (actual obsolete - cause trans and inverse are both stored)
    *
    * @return true if succeeded, false otherwise
    */
    bool invert_transform();

    //=========================================================================================================
    /**
    * Returns true if coordinate transform contains no data.
    *
    * @return true if coordinate transform is empty.
    */
    inline bool isEmpty()
    {
        return this->from < 0;
    }

    //=========================================================================================================
    /**
    * Size of the old struct (fiffCoordTransRec) 26*int = 26*4 = 104
    *
    * @return the size of the old struct fiffCoordTransRec.
    */
    inline static qint32 storageSize();

public:
    //EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    fiff_int_t  from;   /**< Source coordinate system. */
    fiff_int_t  to;     /**< Destination coordinate system. */
    Matrix<double, 4,4, DontAlign>   trans;      /**< The forward transform */
    Matrix<double, 4,4, DontAlign>   invtrans;   /**< The inverse transform */

// Coordinate transformation descriptor

// typedef struct _fiffCoordTransRec {
//  fiff_int_t   from;                    /< Source coordinate system. /
//  fiff_int_t   to;                      /< Destination coordinate system. /
//  fiff_float_t rot[3][3];               /< The forward transform (rotation part) /
//  fiff_float_t move[3];                 /< The forward transform (translation part) /
//  fiff_float_t invrot[3][3];            /< The inverse transform (rotation part) /
//  fiff_float_t invmove[3];              /< The inverse transform (translation part) /
// } *fiffCoordTrans, fiffCoordTransRec;  /< Coordinate transformation descriptor /

// typedef fiffCoordTransRec fiff_coord_trans_t;
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 FiffCoordTrans::storageSize()
{
    return 104;
}

} // NAMESPACE

#endif // FIFF_COORD_TRANS_H
