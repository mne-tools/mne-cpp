//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     fiff_types_ref.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     December, 2015
 * @brief    Old fiff types MNE-C references for testing
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
