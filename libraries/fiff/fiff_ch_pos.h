//=============================================================================================================
/**
 * @file     fiff_ch_pos.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief     FiffChPos class declaration.
 *
 */

#ifndef FIFF_CH_POS_H
#define FIFF_CH_POS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * Coil position description (Replaces fiffChPosRec,*fiffChPos; struct of MNE-C fiff_types.h).
 *
 * @brief Measurement channel position and coil type.
 */
class FIFFSHARED_EXPORT FiffChPos
{
public:
    typedef QSharedPointer<FiffChPos> SPtr;             /**< Shared pointer type for FiffChPos. */
    typedef QSharedPointer<const FiffChPos> ConstSPtr;  /**< Const shared pointer type for FiffChPos. */

    //=========================================================================================================
    /**
     * Constructors the coil position descriptor.
     */
    FiffChPos();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffChPos  Coil position descriptor which should be copied.
     */
    FiffChPos(const FiffChPos &p_FiffChPos);

    //=========================================================================================================
    /**
     * Destroys the coil position descriptor.
     */
    ~FiffChPos();

    //=========================================================================================================
    /**
     * Size of the old struct (fiffChPosRec) 13*int = 13*4 = 52
     *
     * @return the size of the old struct fiffChPosRec.
     */
    inline static qint32 storageSize();

    //=========================================================================================================
    /**
     * Overloaded == operator to compare an object to this instance.
     *
     * @param[in] object    The object which should be compared to.
     *
     * @return true if equal, false otherwise.
     */
    friend bool operator== (const FiffChPos &a, const FiffChPos &b);

public:
    fiff_int_t   coil_type; /**< What kind of coil. */
    Eigen::Vector3f r0;     /**< Coil coordinate system origin. */
    Eigen::Vector3f ex;     /**< Coil coordinate system x-axis unit vector. */
    Eigen::Vector3f ey;     /**< Coil coordinate system y-axis unit vector. */
    Eigen::Vector3f ez;     /**< Coil coordinate system z-axis unit vector. */

// ### OLD STRUCT ###
// /** Measurement channel position and coil type. *
// typedef struct _fiffChPosRec {
//  fiff_int_t   coil_type;    /**< What kind of coil. *
//  fiff_float_t r0[3];        /**< Coil coordinate system origin *
//  fiff_float_t ex[3];        /**< Coil coordinate system x-axis unit vector *
//  fiff_float_t ey[3];        /**< Coil coordinate system y-axis unit vector *
//  fiff_float_t ez[3];        /**< Coil coordinate system z-axis unit vector *
// } fiffChPosRec,*fiffChPos;  /**< Measurement channel position and coil type *

// typedef fiffChPosRec fiff_ch_pos_t;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 FiffChPos::storageSize()
{
    return 52;
}

//=============================================================================================================

inline bool operator== (const FiffChPos &a, const FiffChPos &b)
{
    return (a.coil_type == b.coil_type &&
            a.r0.isApprox(b.r0, 0.0001f) &&
            a.ex.isApprox(b.ex, 0.0001f) &&
            a.ey.isApprox(b.ey, 0.0001f) &&
            a.ez.isApprox(b.ez, 0.0001f));
}
} // NAMESPACE

#endif // FIFF_CH_POS_H
