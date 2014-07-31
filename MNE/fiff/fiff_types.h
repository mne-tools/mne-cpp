//=============================================================================================================
/**
* @file     fiff_types.h
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
* @brief    Old fiff_type declarations - replace them.
*
*/

#ifndef FIFF_TYPES_H
#define FIFF_TYPES_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_constants.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QList>
#include <QStringList>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

const static QStringList defaultQStringList = QStringList();
static Eigen::MatrixXd defaultMatrixXd = Eigen::MatrixXd::Constant(1,1,-1);
const static Eigen::MatrixXd defaultConstMatrixXd(0,0);
const static Eigen::MatrixXi defaultMatrixXi(0,0);
const static Eigen::VectorXi defaultVectorXi;
const static Eigen::RowVectorXi defaultRowVectorXi;
const static QPair<QVariant,QVariant> defaultVariantPair;

typedef Eigen::Matrix<qint16, Eigen::Dynamic, Eigen::Dynamic> MatrixDau16;


//*************************************************************************************************************
//=============================================================================================================
// TYPEDEFS Primitive building blocks:
//=============================================================================================================

typedef unsigned char        fiff_byte_t;
typedef char                 fiff_char_t;
typedef qint16               fiff_short_t;
typedef quint16              fiff_ushort_t;
typedef qint32               fiff_int_t;
typedef quint32              fiff_uint_t;
typedef qint64               fiff_long_t;
typedef quint64              fiff_ulong_t;
typedef float                fiff_float_t;
typedef double               fiff_double_t;
typedef quint16              fiff_dau_pack13_t;
typedef quint16              fiff_dau_pack14_t;
typedef qint16               fiff_dau_pack16_t;
typedef qint32               fiff_julian_t;
typedef char                 fiff_data_t; //unsig char instead of void -> avoid void in C++ cause of its undefined behaviour using delete -> this can happen during lots of casting


//*************************************************************************************************************
//=============================================================================================================
// TYPEDEFS Structured types:
//=============================================================================================================

/** Accurate time stamps used in FIFF files.*/

typedef struct _fiffTimeRec {
 fiff_int_t secs;           /**< GMT time in seconds since epoch */
 fiff_int_t usecs;          /**< Fraction of seconds in microseconds */
} *fiffTime, fiffTimeRec;   /**< Accurate time stamps used in FIFF files.*/


/** Structure representing digitized strings. */

typedef struct _fiffDigStringRec {
 fiff_int_t kind;		  /**< Most commonly FIFF_POINT_EXTRA */
 fiff_int_t ident;		  /**< Number identifying this string */
 fiff_int_t np;		  /**< How many points */
 fiff_float_t **rr;		  /**< Array of point locations */
} fiffDigStringRec, *fiffDigString;/**< Structure representing digitized strings. */

typedef fiffDigStringRec fiff_dig_string_t;


/*
* The layered sphere model
*/

/** Layer descriptor for a layered sphere model */

typedef struct _fiffLayerRec {
 fiff_int_t   id;		/**< Id # of this layer (see below) */
 fiff_float_t rad;		/**< Radius of this layer (m) */
} *fiffLayer, fiffLayerRec;      /**< Layer descriptor for a layered sphere model */


//*************************************************************************************************************
//=============================================================================================================
// TYPEDEF Following types are used by the fiff library. They are not used within the files.:
//=============================================================================================================

/** Structure for sparse matrices */

//typedef struct _fiff_sparse_matrix {
// fiff_int_t   coding;          /**< coding (storage) type of the sparse matrix */
// fiff_int_t   m;	        /**< m rows */
// fiff_int_t   n;               /**< n columns */
// fiff_int_t   nz;              /**< nz nonzeros */
// fiff_float_t *data;           /**< owns the data */
// fiff_int_t   *inds;           /**< index list, points into data, no dealloc! */
// fiff_int_t   *ptrs;           /**< pointer list, points into data, no dealloc! */
//} *fiffSparseMatrix, fiffSparseMatrixRec;

//typedef fiffSparseMatrixRec  fiff_sparse_matrix_t;

/** Structure for event bits */

typedef struct _fiff_event_bits {
 fiff_int_t from_mask;         /**< from mask */
 fiff_int_t from_state;        /**< from state */
 fiff_int_t to_mask;           /**< to mask */
 fiff_int_t to_state;          /**< to state */
} *fiffEventBits, fiffEventBitsRec;

/** Structure for hpi coil */

//typedef struct _fiff_hpi_coil {
// char *event_channel;          /**< event channel */
// fiffEventBitsRec event_bits;  /**< event bits */
// char *signal_channel;         /**< signal channel */
//} *fiffHpiCoil, fiffHpiCoilRec;

/** Structure for hpi subsystem */

//typedef struct _fiff_hpi_subsys {
// fiff_int_t   ncoils;          /**< number of hpi coils */
// fiffHpiCoil  coils;           /**< hpi coils */
//} *fiffHpiSubsys, fiffHpiSubsysRec;

/** Structure for external file references */

typedef struct _fiff_data_ref {
    fiff_int_t      type;       /**< Type of the data */
    fiff_int_t      endian;     /**< Are the data in the little or big endian byte order */
    fiff_long_t     size;       /**< Size of the data, can be over 2 GB  */
    fiff_long_t     offset;     /**< Offset to the data in the external file  */
} *fiffDataRef,fiffDataRefRec;

}//NAMESPACE

#endif // FIFF_TYPES_H
