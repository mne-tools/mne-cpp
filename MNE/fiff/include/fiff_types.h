//=============================================================================================================
/**
* @file     fiff_types.h
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
* @brief    Contains the old fiff_type declarations - replace them.
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
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QList>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

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
typedef void                 fiff_data_t;


//*************************************************************************************************************
//=============================================================================================================
// TYPEDEFS Structured types:
//=============================================================================================================

/** Accurate time stamps used in FIFF files.*/

typedef struct _fiffTimeRec {
 fiff_int_t secs;		/**< GMT time in seconds since epoch */
 fiff_int_t usecs;		/**< Fraction of seconds in microseconds */
} *fiffTime, fiffTimeRec;	/**< Accurate time stamps used in FIFF files.*/


/**
* A file ID.
*
* These universially unique identifiers are also
* used to identify blocks within the files.
*/
//typedef struct _fiffIdRec {
// fiff_int_t version;     /**< File version */
// fiff_int_t machid[2];   /**< Unique machine ID */
// fiffTimeRec time;       /**< Time of the ID creation */
//} fiffIdRec,*fiffId;     /**< This is the file identifier */

//typedef fiffIdRec fiff_id_t;

/** Measurement channel position and coil type. */

typedef struct _fiffChPosRec {
 fiff_int_t   coil_type;		   /**< What kind of coil. */
 fiff_float_t r0[3];			   /**< Coil coordinate system origin */
 fiff_float_t ex[3];			   /**< Coil coordinate system x-axis unit vector */
 fiff_float_t ey[3];			   /**< Coil coordinate system y-axis unit vector */
 fiff_float_t ez[3];	                   /**< Coil coordinate system z-axis unit vector */
} fiffChPosRec,*fiffChPos;                 /**< Measurement channel position and coil type */

typedef fiffChPosRec fiff_ch_pos_t;

///** Directories are composed of these structures. */
////ToDo replace this by a class fiff_dir_entry.h;
//typedef struct _fiffDirEntryRec {
// fiff_int_t  kind;		/**< Tag number */
// fiff_int_t  type;		/**< Data type */
// fiff_int_t  size;		/**< How many bytes */
// fiff_int_t  pos;		/**< Location in file
//                 * Note: the data is located at pos +
//                 * FIFFC_DATA_OFFSET */
//} fiffDirEntryRec,*fiffDirEntry;/**< Directory is composed of these */

///** Alias for fiffDirEntryRec */

//typedef fiffDirEntryRec fiff_dir_entry_t;

/** Digitization point description */

typedef struct _fiffDigPointRec {
 fiff_int_t kind;		 /**< FIFFV_POINT_CARDINAL,
                           *   FIFFV_POINT_HPI, or
                           *   FIFFV_POINT_EEG */
 fiff_int_t ident;		 /**< Number identifying this point */
 fiff_float_t r[3];		 /**< Point location */
 fiff_int_t coord_frame; /**< Newly added to stay consistent with fiff MATLAB implementation */
} fiffDigPointRec, *fiffDigPoint; /**< Digitization point description */


/** Structure representing digitized strings. */

typedef struct _fiffDigStringRec {
 fiff_int_t kind;		  /**< Most commonly FIFF_POINT_EXTRA */
 fiff_int_t ident;		  /**< Number identifying this string */
 fiff_int_t np;		  /**< How many points */
 fiff_float_t **rr;		  /**< Array of point locations */
} fiffDigStringRec, *fiffDigString;/**< Structure representing digitized strings. */

typedef fiffDigPointRec  fiff_dig_point_t;
typedef fiffDigStringRec fiff_dig_string_t;


/////** Coordinate transformation descriptor */
////obsolete just for type convertation
//typedef struct _fiffCoordTransRec {
// fiff_int_t   from;		      /**< Source coordinate system. */
// fiff_int_t   to;		      /**< Destination coordinate system. */
// fiff_float_t rot[3][3];	      /**< The forward transform (rotation part) */
// fiff_float_t move[3];		      /**< The forward transform (translation part) */
// fiff_float_t invrot[3][3];	      /**< The inverse transform (rotation part) */
// fiff_float_t invmove[3];            /**< The inverse transform (translation part) */
//} *fiffCoordTrans, fiffCoordTransRec; /**< Coordinate transformation descriptor */
//todo put this into a class
//typedef fiffCoordTransRec fiff_coord_trans_t;

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

///** Directory tree structure used by the fiff library routines. */
////Obsolete use class fiffDirTree instead
//typedef struct _fiffDirNode {
// qint32              type;	 /**< Block type for this directory */
// fiff_id_t           id;        /**< Id of this block if any */
// /*fiffDirEntry*/
//  fiff_id_t           parent_id;        /**< Newly added to stay consistent with MATLAB implementation */
// QList<fiff_dir_entry_t>    dir;	 /**< Directory of tags in this node */
// qint32              nent;	 /**< Number of entries in this node */
//// fiffDirEntry        dir_tree;	 /**< Directory of tags within this node
////				  * subtrees as well as FIFF_BLOCK_START and FIFF_BLOCK_END
////				  * included. NOTE: While dir is allocated separately
////				  * dir_tree is a pointer to the dirtree field
////				  * in the fiffFile structure. The dir_tree and nent_tree
////				  * fields are only used within the library to facilitate
////				  * certain operations. */
// qint32              nent_tree; /**< Number of entries in the directory tree node */
// //struct _fiffDirNode *parent;	 /**< Parent node */
// //struct _fiffDirNode
// QList<struct _fiffDirNode*>  children;/**< Child nodes */
// qint32              nchild;	 /**< Number of child nodes */
//} *fiffDirNode, fiffDirNodeRec; 	 /**< Directory tree structure used by the fiff library routines. */

//typedef fiffDirNodeRec fiff_dir_node_t;

/** FIFF file handle returned by fiff_open(). */

//typedef struct _fiffFileRec {
//  char         *file_name;	/**< Name of the file */
//  FILE         *fd;		/**< The normal file descriptor */
//  fiffId       id;		/**< The file identifier */
//  fiffDirEntry dir;		/**< This is the directory.
//				 * If no directory exists, fiff_open
//				 * automatically scans the file to create one. */
//  int         nent;	        /**< How many entries? */
//  /*fiffDirNode*/
////  fiffDirTree dirtree;		/**< Directory compiled into a tree */
//  char        *ext_file_name;	/**< Name of the file holding the external data */
//  FILE        *ext_fd;		/**< The file descriptor of the above file if open  */
//} *fiffFile,fiffFileRec;	/**< FIFF file handle. fiff_open() returns this. */


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
    fiff_int_t      type;		/**< Type of the data */
    fiff_int_t      endian;          /**< Are the data in the little or big endian byte order */
    fiff_long_t     size;		/**< Size of the data, can be over 2 GB  */
    fiff_long_t     offset;		/**< Offset to the data in the external file  */
} *fiffDataRef,fiffDataRefRec;

}//NAMESPACE

#endif // FIFF_TYPES_H
