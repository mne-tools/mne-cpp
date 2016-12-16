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
* @brief    FiffCoordTrans class declaration.
*
*/

#ifndef FIFF_COORD_TRANS_H
#define FIFF_COORD_TRANS_H

//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QIODevice>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


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
* Coordinate transformation description which replaces fiffCoordTransRec which had a size of 104
*
* @brief Coordinate transformation description.
*/
class FIFFSHARED_EXPORT FiffCoordTrans
{
public:
    typedef QSharedPointer<FiffCoordTrans> SPtr;            /**< Shared pointer type for FiffCoordTrans. */
    typedef QSharedPointer<const FiffCoordTrans> ConstSPtr; /**< Const shared pointer type for FiffCoordTrans. */

    //=========================================================================================================
    /**
    * Constructs the coordinate transformation descriptor.
    */
    FiffCoordTrans();

    //=========================================================================================================
    /**
    * Constructs a coordinate transformation, by reading from a IO device.
    *
    * @param[in] p_IODevice     IO device to read from the coordinate transform.
    */
    FiffCoordTrans(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
    * Copy constructor.
    *
    * @param[in] p_FiffCoordTrans   Coordinate transformation description which should be copied
    */
    FiffCoordTrans(const FiffCoordTrans &p_FiffCoordTrans);

    //=========================================================================================================
    /**
    * Destroys the coordinate transformation descriptor.
    */
    ~FiffCoordTrans();

    //=========================================================================================================
    /**
    * Initializes the coordinate transformation descriptor.
    */
    void clear();

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
    inline bool isEmpty() const
    {
        return this->from < 0;
    }

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the mne_transform_coordinates function
    *
    * Reads a coordinate transform from a fif file
    *
    * @param[in] p_IODevice    A fiff IO device like a fiff QFile or QTCPSocket
    * @param[out] p_Trans      A coordinate transform from a fif file
    *
    * @return true if succeeded, false otherwise
    */
    static bool read(QIODevice& p_IODevice, FiffCoordTrans& p_Trans);

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the mne_transform_source_space_to function
    *
    * TODO: dest       - The id of the destination coordinate system (FIFFV_COORD_...)
    *
    * Applies the coordinate transform to given coordinates and returns the transformed coordinates
    *
    * @param[in] rr     The coordinates
    *
    * @return Transformed coordinates
    */
    MatrixX3f apply_trans (const MatrixX3f& rr) const;

    //=========================================================================================================
    /**
    * Applies the inverse coordinate transform to given coordinates and returns the transformed coordinates
    *
    * @param[in] rr     The coordinates
    *
    * @return Transformed coordinates
    */
    MatrixX3f apply_inverse_trans (const MatrixX3f& rr) const;

    //=========================================================================================================
    /**
    * ### MNE C root function ###: Implementation of the mne_coord_frame_name function
    *
    * Map coordinate frame integers to human-readable names
    *
    * @param[in] frame  The coordinate frame integer
    *
    * @return Human readable form of the coordinate frame.
    */
    static QString frame_name (int frame);

    //=========================================================================================================
    /**
    * ### MNE C root function ###: Implementation of the fiff_make_transform function
    *
    * Compose the coordinate transformation structure
    * from a known forward transform
    *
    * @param[in] from   Source coordinate system
    * @param[in] to     Destination coordinate system
    * @param[in] rot    The forward transform (rotation part)
    * @param[in] move   The forward transform (translation part)
    *
    * @return the composed transform
    */
    static FiffCoordTrans make(int from, int to, const Matrix3f& rot, const VectorXf& move);

    //=========================================================================================================
    /**
    * ### MNE C root function ###: Implementation of the add_inverse function
    *
    * @param[in] t      Fiff coordinate transform to which the inverse should be added
    *
    * @return True when successful
    */
    static bool addInverse(FiffCoordTrans& t);

    //=========================================================================================================
    /**
    * ### MNE C root function ###: Implementation of the mne_print_coord_transform & mne_print_coord_transform_label function
    *
    * Prints the coordinate transform. TODO: overload stream operator
    */
    void print() const;

    //=========================================================================================================
    /**
    * Size of the old struct (fiffCoordTransRec) 26*int = 26*4 = 104
    *
    * @return the size of the old struct fiffCoordTransRec.
    */
    inline static qint32 storageSize();

public:
    fiff_int_t  from;   /**< Source coordinate system. */
    fiff_int_t  to;     /**< Destination coordinate system. */
    Matrix<float, 4,4, DontAlign>   trans;      /**< The forward transform */
    Matrix<float, 4,4, DontAlign>   invtrans;   /**< The inverse transform */

// ### OLD STRUCT ###
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
