/*
 * Copyright (c) 2001-2008 Elekta Neuromag Oy
 *
 * No part of this program may be photocopied, reproduced,
 * or translated to another program language without the
 * prior written consent of the author.
 *
 *
 * $Id: fiff_types.h 2696 2009-05-23 18:47:52Z msh $
 *
 * Revision 1.17  2008/04/15 10:59:42  mjk
 * Defines to compile on Apple.
 *
 * Revision 1.16  2008/01/25 08:25:05  jne
 * added FIFFV_COIL_CTF_GRAD
 *
 * Revision 1.15  2007/02/28 08:25:01  jne
 * Some cleaning of comments
 *
 * Revision 1.14  2007/01/09 12:41:40  jne
 * new structs fiffEventBitsRec, fiffHpiCoilRec, fiffHpiSubsysRec
 * new defs FIFFV_XFIT_MAP_SURF_SENSORS, FIFFV_XFIT_MAP_SURF_HEAD, FIFFV_XFIT_MAP_SURF_SPHERE
 *
 * Revision 1.13  2006/05/22  11:24:24  11:24:24  jne (Jukka Nenonen)
 * Added fiffSparseMatrixRec (equivalence fiff_sparse_matrix_t)
 * Added FIFFV_QUAT_CH0...7, FIFFV_ESTIM_ERROR_CH, FIFFV_HEAD_MOVEMENT_CH
 * 
 * Revision 1.12  2005/11/03 14:27:40  skesti
 * Renumbered candela
 *
 * Revision 1.11  2005/07/01  09:11:03  09:11:03  skesti (Sami Kesti)
 * Check in mjk changes
 * 
 * Revision 1.10  2004/05/18 11:52:42  mjk
 * Added some Doxygen tags.
 *
 * Revision 1.9  2003/10/23 08:38:05  mjk
 * Fixed headers to work on Lynxos.
 *
 * Revision 1.8  2003/06/27 14:52:29  mjk
 * Fixed header problem in linux.
 *
 * Revision 1.7  2003/01/23 15:25:55  mjk
 * Fixed FIFFC_DATA_OFFSET and some comments having FIFF_ instead of FIFFV_.
 *
 * Revision 1.6  2002/12/18 15:36:45  mjk
 * Added extractable comments.
 *
 * Revision 1.5  2002/08/23  22:47:40  22:47:40  mjk (Matti Kajola)
 * Rev 1.5.1
 * 
 * Revision 1.4  2002/08/19  11:56:42  11:56:42  mjk (Matti Kajola)
 * Librev 1.5.0
 * 
 * Revision 1.3  2001/09/18  09:36:25  09:36:25  mjk (Matti Kajola)
 * Trying to close 1.4.0.
 * 
 *----------------------------------------------------------------------*/
/** \file  fiff_types.h
 *  \brief Definitions for describing the objects in a FIFF file.
 *
 * This header file defines all the structures used in the FIFF files,
 * and some structures used by the fiff library. The structures
 * are defind with a name starting with a undescore, and a typedef
 * is used to give the names that are intended to be used in the programs.
 * For the documentation of the records themselves, use the name with
 * the undescore.
 * 
 */

#ifndef _fiff_types_h
#define _fiff_types_h

#include <stdio.h>
#include <sys/types.h>

#define  FIFFC_FAIL -1
#define  FIFFC_OK 0

#define  FIFFV_TRUE  1
#define  FIFFV_FALSE 0

/* Compatibility definitions */

#define  FIFF_FAIL -1
#define  FIFF_OK    0

/* We need to define in objects of known size! */
/* Unfortunetely there seems to be no consensus among */
/* different operating systems how to do this. */

#if defined(__linux) || defined(__Lynx__) || defined(__APPLE__)
#include <stdint.h>
#endif
#if defined(HPRT)
typedef int int32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
#endif 
#if defined(WIN32)
typedef __int32 int32_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
#endif

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <qglobal.h>


//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

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
typedef qint64               fiff_julian_t;
typedef char                 fiff_data_t; //unsig char instead of void -> avoid void in C++ cause of its undefined behaviour using delete -> this can happen during lots of casting

/*----------------------------------------------------------------------
 * 
 * Primitive building blocks:
 *
 *---------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *
 * Structured types
 *
 *---------------------------------------------------------------------*/

/**
 * FIFF data tag
 * 
 * Tags are used in front of data items to tell what they are.
 */

typedef struct _fiffTagRec {
 fiff_int_t  kind;		/**< Tag number.
				 *   This defines the meaning of the item */
 fiff_int_t  type;		/**< Data type.
				 *   This defines the reperentation of the data. */
 fiff_int_t  size;		/**< Size of the data.
				 *   The size is given in bytes and defines the 
				 *   total size of the data. */
 fiff_int_t  next;		/**< Pointer to the next object.
				 *   Zero if the object follows
				 *   sequentially in file.
				 *   Negative at the end of file */
 fiff_data_t *data;		/**< Pointer to the data.
				 *   This point to the data read or to be written. */
} *fiffTag,fiffTagRec;   /**< FIFF data tag */

#define FIFFV_NEXT_SEQ   0
#define FIFFV_NEXT_NONE -1

#define FIFFC_TAG_INFO_SIZE (sizeof(fiffTagRec) - sizeof(fiff_data_t *))
#define FIFFC_DATA_OFFSET FIFFC_TAG_INFO_SIZE
#define FIFFM_TAG_INFO(x) &((x)->kind)

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

/** Structure for event bits */

typedef struct _fiff_event_bits {
 fiff_int_t from_mask;         /**< from mask */
 fiff_int_t from_state;        /**< from state */
 fiff_int_t to_mask;           /**< to mask */
 fiff_int_t to_state;          /**< to state */
} *fiffEventBits, fiffEventBitsRec;

/**
 * A file ID.
 * 
 * These universially unique identifiers are also
 * used to identify blocks within fthe files.
 */

typedef struct _fiffIdRec {
 fiff_int_t version;	   /**< File version */
 fiff_int_t machid[2];	   /**< Unique machine ID */
 fiffTimeRec time;	   /**< Time of the ID creation */
} *fiffId,fiffIdRec;	   /**< This is the file identifier */

typedef fiffIdRec fiff_id_t;

#define FIFFV_MAGN_CH    1
#define FIFFV_EL_CH      2
#define FIFFV_MEG_CH     FIFFV_MAGN_CH
#define FIFFV_MCG_CH     201
#define FIFFV_EEG_CH     FIFFV_EL_CH
#define FIFFV_STIM_CH    3

#define FIFFV_EOG_CH   202
#define FIFFV_EMG_CH   302
#define FIFFV_ECG_CH   402
#define FIFFV_MISC_CH  502
#define FIFFV_RESP_CH  602		     /* Respiration monitoring */
/*
 * Quaternion channels for head position monitoring
 */
#define FIFFV_QUAT_0   700                 /* Quaternion parameter q0; obsolete for unit quaternion */
#define FIFFV_QUAT_1   701                 /* Quaternion parameter q1; rotation */
#define FIFFV_QUAT_2   702                 /* Quaternion parameter q2; rotation */
#define FIFFV_QUAT_3   703                 /* Quaternion parameter q3; rotation */
#define FIFFV_QUAT_4   704                 /* Quaternion parameter q4; translation */
#define FIFFV_QUAT_5   705                 /* Quaternion parameter q5; translation */
#define FIFFV_QUAT_6   706                 /* Quaternion parameter q6; translation */
#define FIFFV_HPI_G    707                 /* Goodness-of-fit in continuous hpi */
#define FIFFV_HPI_ERR  708                 /* Estimation error in continuous hpi */
#define FIFFV_HPI_MOV  709                 /* Estimated head movement speed in continuous hpi */

#define FIFFM_QUAT_CH(X) ((X >= FIFFV_QUAT_0) && (X <= FIFFV_HPI_MOV))   /* Check for a quaternion channel */ 

#define FIFFV_DIPOLE_WAVE_CH   1000	   /* Dipole time curve */
#define FIFFV_GOODNESS_FIT_CH  1001	   /* Goodness of fit */

/** Measurement channel position and coil type. */

typedef struct _fiffChPosRec {
 fiff_int_t   coil_type;		   /**< What kind of coil. */
 fiff_float_t r0[3];			   /**< Coil coordinate system origin */
 fiff_float_t ex[3];			   /**< Coil coordinate system x-axis unit vector */
 fiff_float_t ey[3];			   /**< Coil coordinate system y-axis unit vector */
 fiff_float_t ez[3];	                   /**< Coil coordinate system z-axis unit vector */
} fiffChPosRec,*fiffChPos;                 /**< Measurement channel position and coil type */

typedef fiffChPosRec fiff_ch_pos_t;

/*
 * Coil types
 */

/* \def FIFFV_COIL_NONE */

#define FIFFV_COIL_NONE                  0  /**< The location info contains no data */
#define FIFFV_COIL_EEG                   1  /**< EEG electrode position in r0 */
#define FIFFV_COIL_NM_122                2  /**< Neuromag 122 coils */
#define FIFFV_COIL_NM_24                 3  /**< Old 24 channel system in HUT */
#define FIFFV_COIL_NM_MCG_AXIAL          4  /**< The axial devices in the HUCS MCG system */
#define FIFFV_COIL_EEG_BIPOLAR           5  /**< Bipolar EEG lead */

#define FIFFV_COIL_DIPOLE              200  /**< Time-varying dipole definition
					     * The coil info contains dipole location (r0) and
					     * direction (ex) */
#define FIFFV_COIL_MCG_42             1000  /**< For testing the MCG software */

#define FIFFV_COIL_POINT_MAGNETOMETER 2000  /**< Simple point magnetometer */
#define FIFFV_COIL_AXIAL_GRAD_5CM     2001  /**< Generic axial gradiometer */

#define FIFFV_COIL_VV_PLANAR_W        3011  /**< VV prototype wirewound planar sensor */
#define FIFFV_COIL_VV_PLANAR_T1       3012  /**< Vectorview SQ20483N planar gradiometer */
#define FIFFV_COIL_VV_PLANAR_T2       3013  /**< Vectorview SQ20483N-A planar gradiometer */
#define FIFFV_COIL_VV_PLANAR_T3       3014  /**< Vectorview SQ20950N planar gradiometer */
#define FIFFV_COIL_VV_MAG_W           3021  /**< VV prototype wirewound magnetometer */
#define FIFFV_COIL_VV_MAG_T1          3022  /**< Vectorview SQ20483N magnetometer */
#define FIFFV_COIL_VV_MAG_T2          3023  /**< Vectorview SQ20483-A magnetometer */
#define FIFFV_COIL_VV_MAG_T3          3024  /**< Vectorview SQ20950N magnetometer */

#define FIFFV_COIL_MAGNES_MAG         4001  /**< Magnes WH magnetometer */
#define FIFFV_COIL_MAGNES_GRAD        4002  /**< Magnes WH gradiometer  */
#define FIFFV_COIL_CTF_GRAD           5001  /**< CTF axial gradiometer */

#define FIFFM_IS_VV_COIL(c) ((c)/1000 == 3)

/** Description of one channel */

//typedef struct _fiffChInfoRec {
// fiff_int_t    scanNo;		/**< Scanning order number */
// fiff_int_t    logNo;		/**< Logical channel # */
// fiff_int_t    kind;		/**< Kind of channel */
// fiff_float_t  range;		/**< Voltmeter range (-1 = auto ranging) */
// fiff_float_t  cal;		/**< Calibration from volts to units used */
// fiff_ch_pos_t chpos;		/**< Channel location */
// fiff_int_t    unit;		/**< Unit of measurement */
// fiff_int_t    unit_mul;	/**< Unit multiplier exponent */
// fiff_char_t   ch_name[16];	/**< Descriptive name for the channel */
//} fiffChInfoRec,*fiffChInfo;	/**< Description of one channel */

///** Alias for fiffChInfoRec */
//typedef fiffChInfoRec fiff_ch_info_t;

#define FIFFM_CHPOS(x) &((x)->chpos)

/*
 * Units of measurement
 */

#define FIFF_UNIT_NONE -1
/*
 * 1. SI base units
 */
#define FIFF_UNIT_M   1
#define FIFF_UNIT_KG  2
#define FIFF_UNIT_SEC 3
#define FIFF_UNIT_A   4
#define FIFF_UNIT_K   5
#define FIFF_UNIT_MOL 6
/*
 * 2. SI Supplementary units
 */
#define FIFF_UNIT_RAD 7
#define FIFF_UNIT_SR  8
/*
 * 1. SI base candela
 */
#define FIFF_UNIT_CD  9
/*
 * 3. SI derived units
 */
#define FIFF_UNIT_HZ  101
#define FIFF_UNIT_N   102
#define FIFF_UNIT_PA  103
#define FIFF_UNIT_J   104
#define FIFF_UNIT_W   105
#define FIFF_UNIT_C   106
#define FIFF_UNIT_V   107
#define FIFF_UNIT_F   108
#define FIFF_UNIT_OHM 109
#define FIFF_UNIT_MHO 110
#define FIFF_UNIT_WB  111
#define FIFF_UNIT_T   112
#define FIFF_UNIT_H   113
#define FIFF_UNIT_CEL 114
#define FIFF_UNIT_LM  115
#define FIFF_UNIT_LX  116
/*
 * 4. Others we need
 */
#define FIFF_UNIT_T_M 201	/* T/m */
#define FIFF_UNIT_AM  202	/* Am  */
/*
 * 5. Multipliers
 */
#define FIFF_UNITM_E   18
#define FIFF_UNITM_PET 15
#define FIFF_UNITM_T   12
#define FIFF_UNITM_MEG  6
#define FIFF_UNITM_K    3
#define FIFF_UNITM_H    2    
#define FIFF_UNITM_DA   1
#define FIFF_UNITM_NONE 0   
#define FIFF_UNITM_D   -1
#define FIFF_UNITM_C   -2
#define FIFF_UNITM_M   -3
#define FIFF_UNITM_MU  -6
#define FIFF_UNITM_N   -9
#define FIFF_UNITM_P  -12
#define FIFF_UNITM_F  -15
#define FIFF_UNITM_A  -18

/** Directories are composed of these structures. */

typedef struct _fiffDirEntryRec {
 fiff_int_t  kind;		/**< Tag number */
 fiff_int_t  type;		/**< Data type */
 fiff_int_t  size;		/**< How many bytes */
 fiff_int_t  pos;		/**< Location in file 
				 * Note: the data is located at pos + 
				 * FIFFC_DATA_OFFSET */
} fiffDirEntryRec,*fiffDirEntry;/**< Directory is composed of these */

/** Alias for fiffDirEntryRec */

typedef fiffDirEntryRec fiff_dir_entry_t;

/** Digitization point description */

typedef struct _fiffDigPointRec {
 fiff_int_t kind;		 /**< FIFFV_POINT_CARDINAL,
				  *   FIFFV_POINT_HPI, or
				  *   FIFFV_POINT_EEG */
 fiff_int_t ident;		 /**< Number identifying this point */
 fiff_float_t r[3];		 /**< Point location */
} *fiffDigPoint,fiffDigPointRec; /**< Digitization point description */

/** Structure representing digitized strings. */

typedef fiffDigPointRec  fiff_dig_point_t;
typedef fiffDigStringRec fiff_dig_string_t;

#define FIFFV_POINT_CARDINAL 1
#define FIFFV_POINT_HPI      2
#define FIFFV_POINT_EEG      3
#define FIFFV_POINT_ECG      FIFF_POINT_EEG
#define FIFFV_POINT_EXTRA    4

#define FIFFV_POINT_LPA      1
#define FIFFV_POINT_NASION   2
#define FIFFV_POINT_RPA      3
/*
 * These are the cardinal points for MCG data 
 * Left and right are supposed to be located below the sternum point
 */
#define FIFFV_POINT_CHEST_LEFT      1
#define FIFFV_POINT_CHEST_STERNUM   2
#define FIFFV_POINT_CHEST_RIGHT     3

/** Coordinate transformation descriptor */

typedef struct _fiffCoordTransRec {
 fiff_int_t   from;		      /**< Source coordinate system. */
 fiff_int_t   to;		      /**< Destination coordinate system. */
 fiff_float_t rot[3][3];	      /**< The forward transform (rotation part) */
 fiff_float_t move[3];		      /**< The forward transform (translation part) */
 fiff_float_t invrot[3][3];	      /**< The inverse transform (rotation part) */
 fiff_float_t invmove[3];            /**< The inverse transform (translation part) */
} *fiffCoordTrans, fiffCoordTransRec; /**< Coordinate transformation descriptor */

typedef fiffCoordTransRec fiff_coord_trans_t;

#define FIFFV_COORD_UNKNOWN     0
#define FIFFV_COORD_DEVICE      1
#define FIFFV_COORD_ISOTRAK     2
#define FIFFV_COORD_HPI         3
#define FIFFV_COORD_HEAD        4
#define FIFFV_COORD_MRI         5
#define FIFFV_COORD_MRI_SLICE   6
#define FIFFV_COORD_MRI_DISPLAY 7
#define FIFFV_COORD_XRAY        FIFF_COORD_MRI
#define FIFFV_COORD_XRAY_SLICE  FIFF_COORD_MRI_SLICE
#define FIFFV_COORD_CT          FIFF_COORD_MRI
#define FIFFV_COORD_CT_SLICE    FIFF_COORD_MRI_SLICE

#define FIFFV_COORD_DICOM_DEVICE     8
#define FIFFV_COORD_IMAGING_DEVICE   9

#define FIFFV_COORD_TORSO      100

#define FIFFV_MOVE         1
#define FIFFV_NO_MOVE      0
/*
 * Artefact removal parameters
 */
#define FIFFV_ARTEF_MAX   100
#define FIFFV_ARTEF_NONE   -1
#define FIFFV_ARTEF_KEEP   0
#define FIFFV_ARTEF_JUMP   1
#define FIFFV_ARTEF_NOJUMP 2

/*
 * The layered sphere model
 */

/** Layer descriptor for a layered sphere model */

//typedef struct _fiffLayerRec {
// fiff_int_t   id;		/**< Id # of this layer (see below) */
// fiff_float_t rad;		/**< Radius of this layer (m) */
//} *fiffLayer,fiffLayerRec;      /**< Layer descriptor for a layered sphere model */

#define FIFFV_LAYER_BRAIN   1
#define FIFFV_LAYER_CSF     2
#define FIFFV_LAYER_SKULL   3
#define FIFFV_LAYER_HEAD    4

#define FIFFV_XFIT_MAP_SURF_SENSORS 0  /**< Xfit contours on the sensor array */
#define FIFFV_XFIT_MAP_SURF_HEAD    1  /**< Xfit contours on a head surface */
#define FIFFV_XFIT_MAP_SURF_SPHERE  2  /**< Xfit contours on a spherical surface */

/*----------------------------------------------------------------------
 * Following types are used by the fiff library. They are not used
 * within the files.
 *---------------------------------------------------------------------*/

/** Directory tree structure used by the fiff library routines. */

//typedef struct _fiffDirNode {
// int                 type;	 /**< Block type for this directory */
// fiffId              id;        /**< Id of this block if any */
// fiffDirEntry        dir;	 /**< Directory of tags in this node */
// int                 nent;	 /**< Number of entries in this node */
// fiffDirEntry        dir_tree;	 /**< Directory of tags within this node
//				  * subtrees as well as FIFF_BLOCK_START and FIFF_BLOCK_END
//				  * included. NOTE: While dir is allocated separately
//				  * dir_tree is a pointer to the dirtree field
//				  * in the fiffFile structure. The dir_tree and nent_tree
//				  * fields are only used within the library to facilitate
//				  * certain operations. */
// int                 nent_tree; /**< Number of entries in the directory tree node */
// struct _fiffDirNode *parent;	 /**< Parent node */
// struct _fiffDirNode **children;/**< Child nodes */
// int                 nchild;	 /**< Number of child nodes */
//} fiffDirNodeRec,*fiffDirNode; 	 /**< Directory tree structure used by the fiff library routines. */

/** FIFF file handle returned by fiff_open(). */

//typedef struct _fiffFileRec {
//  char         *file_name;	/**< Name of the file */
//  FILE         *fd;		/**< The normal file descriptor */
//  fiffId       id;		/**< The file identifier */
//  fiffDirEntry dir;		/**< This is the directory.
//				 * If no directory exists, fiff_open
//				 * automatically scans the file to create one. */
//  int         nent;	        /**< How many entries? */
//  fiffDirNode dirtree;		/**< Directory compiled into a tree */
//  char        *ext_file_name;	/**< Name of the file holding the external data */
//  FILE        *ext_fd;		/**< The file descriptor of the above file if open  */
//} *fiffFile,fiffFileRec;	/**< FIFF file handle. fiff_open() returns this. */

/** Structure for sparse matrices */

typedef struct _fiff_sparse_matrix {
 fiff_int_t   coding;          /**< coding (storage) type of the sparse matrix */
 fiff_int_t   m;	        /**< m rows */
 fiff_int_t   n;               /**< n columns */
 fiff_int_t   nz;              /**< nz nonzeros */
 fiff_float_t *data;           /**< owns the data */
 fiff_int_t   *inds;           /**< index list, points into data, no dealloc! */
 fiff_int_t   *ptrs;           /**< pointer list, points into data, no dealloc! */
} *fiffSparseMatrix, fiffSparseMatrixRec;

typedef fiffSparseMatrixRec  fiff_sparse_matrix_t;

/** Structure for event bits */

/** Structure for hpi coil */

typedef struct _fiff_hpi_coil {
 char *event_channel;          /**< event channel */
 fiffEventBitsRec event_bits;  /**< event bits */
 char *signal_channel;         /**< signal channel */
} *fiffHpiCoil, fiffHpiCoilRec;

/** Structure for hpi subsystem */

typedef struct _fiff_hpi_subsys {
 fiff_int_t   ncoils;          /**< number of hpi coils */
 fiffHpiCoil  coils;           /**< hpi coils */
} *fiffHpiSubsys, fiffHpiSubsysRec;

typedef struct _fiff_data_ref {
    fiff_int_t      type;       /**< Type of the data */
    fiff_int_t      endian;     /**< Are the data in the little or big endian byte order */
    fiff_long_t     size;       /**< Size of the data, can be over 2 GB  */
    fiff_long_t     offset;     /**< Offset to the data in the external file  */
} *fiffDataRef,fiffDataRefRec;

} // NAMESPACE

#endif
