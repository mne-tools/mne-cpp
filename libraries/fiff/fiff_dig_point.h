//=============================================================================================================
/**
 * @file     fiff_dig_point.h
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
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
 * @brief    FiffDigPoint class declaration.
 *
 */

#ifndef FIFF_DIG_POINT_H
#define FIFF_DIG_POINT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QDebug>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * Replaces fiffDigPointRec, *fiffDigPoint struct (analyze_types.h).
 *
 * @brief Digitization point description
 */
class FIFFSHARED_EXPORT FiffDigPoint
{
public:
    typedef QSharedPointer<FiffDigPoint> SPtr;              /**< Shared pointer type for FiffDigPoint. */
    typedef QSharedPointer<const FiffDigPoint> ConstSPtr;   /**< Const shared pointer type for FiffDigPoint. */

    //=========================================================================================================
    /**
     * Constructs the digitization point description
     */
    FiffDigPoint();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffDigPoint   Digitization point descriptor which should be copied.
     */
    FiffDigPoint(const FiffDigPoint& p_FiffDigPoint);

    //=========================================================================================================
    /**
     * Destroys the digitization point description
     */
    ~FiffDigPoint();

    //=========================================================================================================
    /**
     * Size of the old struct (fiffDigPointRec) 5*int = 5*4 = 20
     *
     * @return the size of the old struct fiffDigPointRec.
     */
    inline static qint32 storageSize();

    //=========================================================================================================
    /**
     * @brief operator =.
     */
    FiffDigPoint& operator=(FiffDigPoint& );

public:
    fiff_int_t      kind;           /**< FIFFV_POINT_CARDINAL, FIFFV_POINT_HPI, FIFFV_POINT_EXTRA or FIFFV_POINT_EEG. */
    fiff_int_t      ident;          /**< Number identifying this point. */
    fiff_float_t    r[3];           /**< Point location. */
    fiff_int_t      coord_frame;    /**< Newly added to stay consistent with fiff MATLAB implementation. */

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 FiffDigPoint::storageSize()
{
    return 20;
}
} // NAMESPACE

#endif // FIFF_DIG_POINT_H
