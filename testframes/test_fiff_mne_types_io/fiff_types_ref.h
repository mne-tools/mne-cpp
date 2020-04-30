//=============================================================================================================
/**
 * @file     fiff_types_ref.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Christoph Dinh. All rights reserved.
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
 * @brief    Old fiff types MNE-C references for testing
 *
 */

#ifndef FIFFTYPESREF_H
#define FIFFTYPESREF_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <qglobal.h>

//=============================================================================================================
// TYPEDEFS Primitive building blocks:
//=============================================================================================================

typedef qint32      fiff_int_t_REF;
typedef float       fiff_float_t_REF;

//=============================================================================================================
// REFERENCES
//=============================================================================================================

/** Coordinate transformation descriptor */
typedef struct _fiffCoordTransRec_REF {
    fiff_int_t_REF      from;                   /**< Source coordinate system. */
    fiff_int_t_REF      to;                     /**< Destination coordinate system. */
    fiff_float_t_REF    rot[3][3];              /**< The forward transform (rotation part) */
    fiff_float_t_REF    move[3];                /**< The forward transform (translation part) */
    fiff_float_t_REF    invrot[3][3];           /**< The inverse transform (rotation part) */
    fiff_float_t_REF    invmove[3];             /**< The inverse transform (translation part) */
} *fiffCoordTrans_REF, fiffCoordTransRec_REF;   /**< Coordinate transformation descriptor */

#endif // FIFFTYPESREF_H
