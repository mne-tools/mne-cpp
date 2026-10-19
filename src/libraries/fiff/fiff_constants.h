//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_constants.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Limin Sun <liminsun@nmr.mgh.harvard.edu>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     August 2012
 * @brief    Symbolic FIFF tag, block, value, unit and channel-type constants shared across FIFFLIB.
 *
 * Mirrors the @c fiff_constants.h header shipped with the original MNE-C
 * toolchain and the matching tables in @c mne.io.constants in MNE-Python,
 * so a kind / block / unit number written by either implementation is
 * recognized verbatim here. The naming convention (also documented in
 * @ref fiff_file.h) is:
 *
 * - @c FIFF_   - tag kind (e.g. @c FIFF_NCHAN, @c FIFF_SFREQ).
 * - @c FIFFB_  - block type (e.g. @c FIFFB_MEAS, @c FIFFB_RAW_DATA).
 * - @c FIFFV_  - enumerated value with semantic meaning in some context
 *   (e.g. @c FIFFV_COIL_VV_PLANAR_T1, @c FIFFV_COORD_HEAD).
 * - @c FIFFC_  - implementation constant, never appears in a file.
 * - @c FIFFT_  - tag data type (scalar / matrix descriptor).
 *
 * The numeric values must never drift from the Elekta FIFF specification;
 * any new constant added here must match the value already used by the
 * neuromag acquisition stack and by MNE-Python, otherwise FIFF files
 * become non-portable.
 */

#ifndef FIFF_CONSTANTS_H
#define FIFF_CONSTANTS_H

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================


//=============================================================================================================
// FIFF constants
//=============================================================================================================

inline constexpr auto FIFFC_FAIL = -1;
inline constexpr auto FIFFC_OK = 0;

inline constexpr auto FIFFV_TRUE = 1;
inline constexpr auto FIFFV_FALSE = 0;

/* Compatibility definitions */

inline constexpr auto FIFF_FAIL = -1;
inline constexpr auto FIFF_OK = 0;


/*
 * Quaternion channels for head position monitoring
 */
inline constexpr auto FIFFV_QUAT_0 = 700;  /**< Quaternion parameter q0; obsolete for unit quaternion. */
inline constexpr auto FIFFV_QUAT_1 = 701;  /**< Quaternion parameter q1; rotation. */
inline constexpr auto FIFFV_QUAT_2 = 702;  /**< Quaternion parameter q2; rotation. */
inline constexpr auto FIFFV_QUAT_3 = 703;  /**< Quaternion parameter q3; rotation. */
inline constexpr auto FIFFV_QUAT_4 = 704;  /**< Quaternion parameter q4; translation. */
inline constexpr auto FIFFV_QUAT_5 = 705;  /**< Quaternion parameter q5; translation. */
inline constexpr auto FIFFV_QUAT_6 = 706;  /**< Quaternion parameter q6; translation. */
inline constexpr auto FIFFV_HPI_G = 707;   /**< Goodness-of-fit in continuous hpi. */
inline constexpr auto FIFFV_HPI_ERR = 708; /**< Estimation error in continuous hpi. */
inline constexpr auto FIFFV_HPI_MOV = 709; /**< Estimated head movement speed in continuous hpi. */

#define FIFFM_QUAT_CH(X) ((X >= FIFFV_QUAT_0) && (X <= FIFFV_HPI_MOV)) /* Check for a quaternion channel */

inline constexpr auto FIFFV_DIPOLE_WAVE_CH = 1000;  /* Dipole time curve */
inline constexpr auto FIFFV_GOODNESS_FIT_CH = 1001; /* Goodness of fit */

#define FIFFM_CHPOS(x) &((x)->chpos)

/*
 * Units of measurement
 */
inline constexpr auto FIFF_UNIT_NONE = -1;

/*
 * SI base units
 */
inline constexpr auto FIFF_UNIT_M = 1;
inline constexpr auto FIFF_UNIT_KG = 2;
inline constexpr auto FIFF_UNIT_SEC = 3;
inline constexpr auto FIFF_UNIT_A = 4;
inline constexpr auto FIFF_UNIT_K = 5;
inline constexpr auto FIFF_UNIT_MOL = 6;

/*
 * SI Supplementary units
 */
inline constexpr auto FIFF_UNIT_RAD = 7;
inline constexpr auto FIFF_UNIT_SR = 8;

/*
 * SI base candela
 */
inline constexpr auto FIFF_UNIT_CD = 9;

/*
 * SI derived units
 */
inline constexpr auto FIFF_UNIT_HZ = 101;
inline constexpr auto FIFF_UNIT_N = 102;
inline constexpr auto FIFF_UNIT_PA = 103;
inline constexpr auto FIFF_UNIT_J = 104;
inline constexpr auto FIFF_UNIT_W = 105;
inline constexpr auto FIFF_UNIT_C = 106;
inline constexpr auto FIFF_UNIT_V = 107;
inline constexpr auto FIFF_UNIT_F = 108;
inline constexpr auto FIFF_UNIT_OHM = 109;
inline constexpr auto FIFF_UNIT_MHO = 110;
inline constexpr auto FIFF_UNIT_WB = 111;
inline constexpr auto FIFF_UNIT_T = 112;
inline constexpr auto FIFF_UNIT_H = 113;
inline constexpr auto FIFF_UNIT_CEL = 114;
inline constexpr auto FIFF_UNIT_LM = 115;
inline constexpr auto FIFF_UNIT_LX = 116;

/*
 * Others we need
 */
inline constexpr auto FIFF_UNIT_T_M = 201;   /* T/m */
inline constexpr auto FIFF_UNIT_AM = 202;    /* Am  */
inline constexpr auto FIFF_UNIT_AM_M2 = 203; /* Am/m^2  */
inline constexpr auto FIFF_UNIT_AM_M3 = 204; /* Am/m^3  */

/*
 * Multipliers
 */
inline constexpr auto FIFF_UNITM_E = 18;
inline constexpr auto FIFF_UNITM_PET = 15;
inline constexpr auto FIFF_UNITM_T = 12;
inline constexpr auto FIFF_UNITM_MEG = 6;
inline constexpr auto FIFF_UNITM_K = 3;
inline constexpr auto FIFF_UNITM_H = 2;
inline constexpr auto FIFF_UNITM_DA = 1;
inline constexpr auto FIFF_UNITM_NONE = 0;
inline constexpr auto FIFF_UNITM_D = -1;
inline constexpr auto FIFF_UNITM_C = -2;
inline constexpr auto FIFF_UNITM_M = -3;
inline constexpr auto FIFF_UNITM_MU = -6;
inline constexpr auto FIFF_UNITM_N = -9;
inline constexpr auto FIFF_UNITM_P = -12;
inline constexpr auto FIFF_UNITM_F = -15;
inline constexpr auto FIFF_UNITM_A = -18;

/*
 * Coil types
 */
inline constexpr auto FIFFV_COIL_NONE = 0;                          /**< The location info contains no data. */
inline constexpr auto FIFFV_COIL_EEG = 1;                           /**< EEG electrode position in r0. */
inline constexpr auto FIFFV_COIL_NM_122 = 2;                        /**< Neuromag 122 coils. */
inline constexpr auto FIFFV_COIL_NM_24 = 3;                         /**< Old 24 channel system in HUT. */
inline constexpr auto FIFFV_COIL_NM_MCG_AXIAL = 4;                  /**< The axial devices in the HUCS MCG system. */
inline constexpr auto FIFFV_COIL_EEG_BIPOLAR = 5;                   /**< Bipolar EEG lead. */
inline constexpr auto FIFFV_COIL_DIPOLE = 200;                      /**< Time-varying dipole definition The coil info contains dipole location (r0) and direction (ex). */
inline constexpr auto FIFFV_COIL_MCG_42 = 1000;                     /**< For testing the MCG software. */
inline constexpr auto FIFFV_COIL_POINT_MAGNETOMETER = 2000;         /**< Simple point magnetometer. */
inline constexpr auto FIFFV_COIL_AXIAL_GRAD_5CM = 2001;             /**< Generic axial gradiometer. */
inline constexpr auto FIFFV_COIL_VV_PLANAR_W = 3011;                /**< VV prototype wirewound planar sensor. */
inline constexpr auto FIFFV_COIL_VV_PLANAR_T1 = 3012;               /**< Vectorview SQ20483N planar gradiometer. */
inline constexpr auto FIFFV_COIL_VV_PLANAR_T2 = 3013;               /**< Vectorview SQ20483N-A planar gradiometer. */
inline constexpr auto FIFFV_COIL_VV_PLANAR_T3 = 3014;               /**< Vectorview SQ20950N planar gradiometer. */
inline constexpr auto FIFFV_COIL_VV_MAG_W = 3021;                   /**< VV prototype wirewound magnetometer. */
inline constexpr auto FIFFV_COIL_VV_MAG_T1 = 3022;                  /**< Vectorview SQ20483N magnetometer. */
inline constexpr auto FIFFV_COIL_VV_MAG_T2 = 3023;                  /**< Vectorview SQ20483-A magnetometer. */
inline constexpr auto FIFFV_COIL_VV_MAG_T3 = 3024;                  /**< Vectorview SQ20950N magnetometer. */
inline constexpr auto FIFFV_COIL_MAGNES_MAG = 4001;                 /**< Magnes WH magnetometer. */
inline constexpr auto FIFFV_COIL_MAGNES_GRAD = 4002;                /**< Magnes WH gradiometer . */
inline constexpr auto FIFFV_COIL_CTF_GRAD = 5001;                   /**< CTF axial gradiometer. */
inline constexpr auto FIFFV_COIL_CTF_REF_MAG = 5002;                /**< CTF reference magnetometer. */
inline constexpr auto FIFFV_COIL_CTF_REF_GRAD = 5003;               /**< CTF reference gradiometer. */
inline constexpr auto FIFFV_COIL_CTF_REF_GOFF = 5004;               /**< CTF offdiagonal reference gradiometer. */
inline constexpr auto FIFFV_COIL_KIT_GRAD = 6001;                   /**< KIT MIT gradiometer. */
inline constexpr auto FIFFV_COIL_KIT_REF_GRAD = 6002;               /**< KIT MIT reference magnetometer. */
inline constexpr auto FIFFV_COIL_BABY_GRAD = 7001;                  /**< BabySQUID gradiometers. */
inline constexpr auto FIFFV_COIL_BABY_MAG = 7002;                   /**< BabyMEG inner layer magnetometers. */
inline constexpr auto FIFFV_COIL_BABY_REF_MAG = 7003;               /**< BabyMEG outer layer magnetometers. */
inline constexpr auto FIFFV_COIL_BABY_REF_MAG2 = 7004;              /**< BabyMEG reference magnetometer. */
inline constexpr auto FIFFV_COIL_ARTEMIS123_GRAD = 7501;            /**< Artemis123 gradiometer. */
inline constexpr auto FIFFV_COIL_ARTEMIS123_REF_MAG = 7502;         /**< Artemis123 reference magnetometer. */
inline constexpr auto FIFFV_COIL_ARTEMIS123_REF_GRAD = 7503;        /**< Artemis123 reference gradiometer. */
inline constexpr auto FIFFV_COIL_QUSPIN_ZFOPM_MAG = 8001;           /**< QuSpin ZFOPM magnetometer. */
inline constexpr auto FIFFV_COIL_KRISS_GRAD = 9001;                 /**< KRISS gradiometer. */
inline constexpr auto FIFFV_COIL_COMPUMEDICS_ADULT_GRAD = 9101;     /**< Compumedics adult gradiometer. */
inline constexpr auto FIFFV_COIL_COMPUMEDICS_PEDIATRIC_GRAD = 9102; /**< Compumedics pediatric gradiometer. */

#define FIFFM_IS_VV_COIL(c) ((c) / 1000 == 3)

/*
 * These are the cardinal points for MCG data
 * Left and right are supposed to be located below the sternum point
 */
inline constexpr auto FIFFV_POINT_CHEST_LEFT = 1;
inline constexpr auto FIFFV_POINT_CHEST_STERNUM = 2;
inline constexpr auto FIFFV_POINT_CHEST_RIGHT = 3;

/*
 * Coordinate transformation descriptor
 */
inline constexpr auto FIFFV_COORD_UNKNOWN = 0;
inline constexpr auto FIFFV_COORD_DEVICE = 1;
inline constexpr auto FIFFV_COORD_ISOTRAK = 2;
inline constexpr auto FIFFV_COORD_HPI = 3;
inline constexpr auto FIFFV_COORD_HEAD = 4;
inline constexpr auto FIFFV_COORD_MRI = 5;
inline constexpr auto FIFFV_COORD_MRI_SLICE = 6;
inline constexpr auto FIFFV_COORD_MRI_DISPLAY = 7;
#define FIFFV_COORD_XRAY FIFF_COORD_MRI
#define FIFFV_COORD_XRAY_SLICE FIFF_COORD_MRI_SLICE
#define FIFFV_COORD_CT FIFF_COORD_MRI
#define FIFFV_COORD_CT_SLICE FIFF_COORD_MRI_SLICE

inline constexpr auto FIFFV_COORD_DICOM_DEVICE = 8;
inline constexpr auto FIFFV_COORD_IMAGING_DEVICE = 9;

inline constexpr auto FIFFV_COORD_TORSO = 100;

inline constexpr auto FIFFV_MOVE = 1;
inline constexpr auto FIFFV_NO_MOVE = 0;

/*
 * Artefact removal parameters
 */
inline constexpr auto FIFFV_ARTEF_MAX = 100;
inline constexpr auto FIFFV_ARTEF_NONE = -1;
inline constexpr auto FIFFV_ARTEF_KEEP = 0;
inline constexpr auto FIFFV_ARTEF_JUMP = 1;
inline constexpr auto FIFFV_ARTEF_NOJUMP = 2;

/*
 * The layered sphere model
 */
inline constexpr auto FIFFV_LAYER_BRAIN = 1;
inline constexpr auto FIFFV_LAYER_CSF = 2;
inline constexpr auto FIFFV_LAYER_SKULL = 3;
inline constexpr auto FIFFV_LAYER_HEAD = 4;

inline constexpr auto FIFFV_XFIT_MAP_SURF_SENSORS = 0; /**< Xfit contours on the sensor array. */
inline constexpr auto FIFFV_XFIT_MAP_SURF_HEAD = 1;    /**< Xfit contours on a head surface. */
inline constexpr auto FIFFV_XFIT_MAP_SURF_SPHERE = 2;  /**< Xfit contours on a spherical surface. */

/*
 * Pointers
 */
inline constexpr auto FIFFV_NEXT_SEQ = 0;
inline constexpr auto FIFFV_NEXT_NONE = -1;

/*
 * Channel types
 */
inline constexpr auto FIFFV_MAGN_CH = 1;
inline constexpr auto FIFFV_EL_CH = 2;
#define FIFFV_MEG_CH FIFFV_MAGN_CH
inline constexpr auto FIFFV_MCG_CH = 201;
inline constexpr auto FIFFV_REF_MEG_CH = 301; /**< CTF coil and channel type. */
#define FIFFV_EEG_CH FIFFV_EL_CH
inline constexpr auto FIFFV_STIM_CH = 3;

inline constexpr auto FIFFV_EOG_CH = 202;
inline constexpr auto FIFFV_EMG_CH = 302;
inline constexpr auto FIFFV_ECG_CH = 402;
inline constexpr auto FIFFV_MISC_CH = 502;
inline constexpr auto FIFFV_RESP_CH = 602;         /**< Respiration monitoring*/
inline constexpr auto FIFFV_BIO_CH = 102;          /**< Biological signal (e.g. GSR recorded as bio). */
inline constexpr auto FIFFV_SYST_CH = 900;         /**< System status channel. */
inline constexpr auto FIFFV_IAS_CH = 910;          /**< Internal active shielding data (Triux). */
inline constexpr auto FIFFV_EXCI_CH = 920;         /**< Flux excitation channel. */
inline constexpr auto FIFFV_FNIRS_CH = 1100;       /**< Functional near-infrared spectroscopy. */
inline constexpr auto FIFFV_TEMPERATURE_CH = 1200; /**< Temperature. */
inline constexpr auto FIFFV_GALVANIC_CH = 1300;    /**< Galvanic skin response. */
inline constexpr auto FIFFV_EYETRACK_CH = 1400;    /**< Eye tracking. */

/*
 * Intracranial EEG channel types (FIFF standard, as written by mne-python); their coil type is FIFFV_COIL_EEG
 */
inline constexpr auto FIFFV_ECOG_CH = 902; /**< Electrocorticography (subdural grids/strips). */
inline constexpr auto FIFFV_SEEG_CH = 802; /**< Stereoelectroencephalography (depth electrodes). */
inline constexpr auto FIFFV_DBS_CH = 803;  /**< Deep brain stimulation electrodes. */

/*
 * Coil types that distinguish EEG, fNIRS and eye-tracking channel types
 */
inline constexpr auto FIFFV_COIL_EEG_CSD = 6;                      /**< Current source density (surface Laplacian). */
inline constexpr auto FIFFV_COIL_FNIRS_HBO = 300;                  /**< Oxyhaemoglobin. */
inline constexpr auto FIFFV_COIL_FNIRS_HBR = 301;                  /**< Deoxyhaemoglobin. */
inline constexpr auto FIFFV_COIL_FNIRS_CW_AMPLITUDE = 302;         /**< Continuous-wave amplitude. */
inline constexpr auto FIFFV_COIL_FNIRS_OD = 303;                   /**< Optical density. */
inline constexpr auto FIFFV_COIL_FNIRS_FD_AC_AMPLITUDE = 304;      /**< Frequency-domain AC amplitude. */
inline constexpr auto FIFFV_COIL_FNIRS_FD_PHASE = 305;             /**< Frequency-domain phase. */
inline constexpr auto FIFFV_COIL_FNIRS_TD_GATED_AMPLITUDE = 306;   /**< Time-domain gated amplitude. */
inline constexpr auto FIFFV_COIL_FNIRS_TD_MOMENTS_INTENSITY = 307; /**< Time-domain moments: intensity. */
inline constexpr auto FIFFV_COIL_FNIRS_TD_MOMENTS_MEAN = 308;      /**< Time-domain moments: mean. */
inline constexpr auto FIFFV_COIL_FNIRS_TD_MOMENTS_VARIANCE = 309;  /**< Time-domain moments: variance. */
inline constexpr auto FIFFV_COIL_EYETRACK_POS = 400;               /**< Eye-tracking gaze position. */
inline constexpr auto FIFFV_COIL_EYETRACK_PUPIL = 401;             /**< Eye-tracking pupil size. */

/*
 * More of those defined in MNE
 */
inline constexpr auto FIFFV_MNE_SURF_UNKNOWN = -1;
inline constexpr auto FIFFV_MNE_SURF_LEFT_HEMI = 101;
inline constexpr auto FIFFV_MNE_SURF_RIGHT_HEMI = 102;

/*
 * These relate to the Isotrak data
 */
inline constexpr auto FIFFV_POINT_CARDINAL = 1;
inline constexpr auto FIFFV_POINT_HPI = 2;
inline constexpr auto FIFFV_POINT_EEG = 3;
#define FIFFV_POINT_ECG FIFFV_POINT_EEG
inline constexpr auto FIFFV_POINT_EXTRA = 4;

inline constexpr auto FIFFV_POINT_LPA = 1;
inline constexpr auto FIFFV_POINT_NASION = 2;
inline constexpr auto FIFFV_POINT_RPA = 3;

/*
 * These are the MNE fiff definitions
 */
inline constexpr auto FIFFB_MNE = 350;
inline constexpr auto FIFFB_MNE_SOURCE_SPACE = 351;
inline constexpr auto FIFFB_MNE_FORWARD_SOLUTION = 352;
inline constexpr auto FIFFB_MNE_PARENT_MRI_FILE = 353;
inline constexpr auto FIFFB_MNE_PARENT_MEAS_FILE = 354;
inline constexpr auto FIFFB_MNE_COV = 355;
inline constexpr auto FIFFB_MNE_INVERSE_SOLUTION = 356;
inline constexpr auto FIFFB_MNE_NAMED_MATRIX = 357;
inline constexpr auto FIFFB_MNE_ENV = 358;
inline constexpr auto FIFFB_MNE_BAD_CHANNELS = 359;
inline constexpr auto FIFFB_MNE_VERTEX_MAP = 360;
inline constexpr auto FIFFB_MNE_EVENTS = 361;
inline constexpr auto FIFFB_MNE_MORPH_MAP = 362;

/*
 * CTF compensation data
 */
inline constexpr auto FIFFB_MNE_CTF_COMP = 370;
inline constexpr auto FIFFB_MNE_CTF_COMP_DATA = 371;
inline constexpr auto FIFFB_MNE_DERIVATIONS = 372;

/*
 * Fiff tags associated with MNE computations (3500...)
 * 3500... Bookkeeping
 */
inline constexpr auto FIFF_MNE_ROW_NAMES = 3502;
inline constexpr auto FIFF_MNE_COL_NAMES = 3503;
inline constexpr auto FIFF_MNE_NROW = 3504;
inline constexpr auto FIFF_MNE_NCOL = 3505;
inline constexpr auto FIFF_MNE_COORD_FRAME = 3506; /**< Coordinate frame employed. Defaults:*/
//FIFFB_MNE_SOURCE_SPACE   #define FIFFV_COORD_MRI
//FIFFB_MNE_FORWARD_SOLUTION   FIFFV_COORD_HEAD
//FIFFB_MNE_INVERSE_SOLUTION   FIFFV_COORD_HEAD
inline constexpr auto FIFF_MNE_CH_NAME_LIST = 3507;
inline constexpr auto FIFF_MNE_FILE_NAME = 3508; /**< This removes the collision with fiff_stream.h (used to be 3501)*/

/*
 * 3510... 3590... Source space or surface
 */
inline constexpr auto FIFF_MNE_SOURCE_SPACE_POINTS = 3510;       /**< The vertices*/
inline constexpr auto FIFF_MNE_SOURCE_SPACE_NORMALS = 3511;      /**< The vertex normals*/
inline constexpr auto FIFF_MNE_SOURCE_SPACE_NPOINTS = 3512;      /**< How many vertices*/
inline constexpr auto FIFF_MNE_SOURCE_SPACE_SELECTION = 3513;    /**< Which are selected to the source space*/
inline constexpr auto FIFF_MNE_SOURCE_SPACE_NUSE = 3514;         /**< How many are in use*/
inline constexpr auto FIFF_MNE_SOURCE_SPACE_NEAREST = 3515;      /**< Nearest source space vertex for all vertices*/
inline constexpr auto FIFF_MNE_SOURCE_SPACE_NEAREST_DIST = 3516; /**< Distance to the Nearest source space vertex for all vertices*/
inline constexpr auto FIFF_MNE_SOURCE_SPACE_ID = 3517;           /**< Identifier*/
inline constexpr auto FIFF_MNE_SOURCE_SPACE_TYPE = 3518;         /**< FsSurface or volume*/

inline constexpr auto FIFF_MNE_SOURCE_SPACE_NTRI = 3590;          /**< Number of triangles*/
inline constexpr auto FIFF_MNE_SOURCE_SPACE_TRIANGLES = 3591;     /**< The triangulation*/
inline constexpr auto FIFF_MNE_SOURCE_SPACE_NUSE_TRI = 3592;      /**< Number of triangles corresponding to the number of vertices in use*/
inline constexpr auto FIFF_MNE_SOURCE_SPACE_USE_TRIANGLES = 3593; /**< The triangulation of the used vertices in the source space*/

inline constexpr auto FIFF_MNE_SOURCE_SPACE_VOXEL_DIMS = 3596;   /**< Voxel space dimensions in a volume source space*/
inline constexpr auto FIFF_MNE_SOURCE_SPACE_INTERPOLATOR = 3597; /**< Matrix to interpolate a volume source space into a mri volume*/
inline constexpr auto FIFF_MNE_SOURCE_SPACE_MRI_FILE = 3598;     /**< MRI file used in the interpolation*/

inline constexpr auto FIFF_MNE_SOURCE_SPACE_DIST = 3599;       /**< Distances between vertices in use (along the surface)*/
inline constexpr auto FIFF_MNE_SOURCE_SPACE_DIST_LIMIT = 3600; /**< If distance is above this limit (in the volume) it has not been calculated*/

/*
 * 3520... Forward solution
 */
inline constexpr auto FIFF_MNE_FORWARD_SOLUTION = 3520;
inline constexpr auto FIFF_MNE_SOURCE_ORIENTATION = 3521; /**< Fixed or free*/
inline constexpr auto FIFF_MNE_INCLUDED_METHODS = 3522;
inline constexpr auto FIFF_MNE_FORWARD_SOLUTION_GRAD = 3523;

/*
 * 3530... Covariance matrix
 */
inline constexpr auto FIFF_MNE_COV_KIND = 3530;        /**< What kind of a covariance matrix*/
inline constexpr auto FIFF_MNE_COV_DIM = 3531;         /**< Matrix dimension*/
inline constexpr auto FIFF_MNE_COV = 3532;             /**< Full matrix in packed representation (lower triangle)*/
inline constexpr auto FIFF_MNE_COV_DIAG = 3533;        /**< Diagonal matrix*/
inline constexpr auto FIFF_MNE_COV_EIGENVALUES = 3534; /**< Eigenvalues and eigenvectors of the above*/
inline constexpr auto FIFF_MNE_COV_EIGENVECTORS = 3535;
inline constexpr auto FIFF_MNE_COV_NFREE = 3536; /**< Number of degrees of freedom*/

/*
 * 3540... Inverse operator
 * We store the inverse operator as the eigenleads  eigenfields and weights
 */
inline constexpr auto FIFF_MNE_INVERSE_LEADS = 3540;               /**< The eigenleads*/
inline constexpr auto FIFF_MNE_INVERSE_LEADS_WEIGHTED = 3546;      /**< The eigenleads (already weighted with R^0.5)*/
inline constexpr auto FIFF_MNE_INVERSE_FIELDS = 3541;              /**< The eigenfields*/
inline constexpr auto FIFF_MNE_INVERSE_SING = 3542;                /**< The singular values*/
inline constexpr auto FIFF_MNE_INVERSE_SOURCE_UNIT = 3547;         /**< Unit of the source estimates the operator produces, one of FIFF_UNIT_AM, FIFF_UNIT_AM_M2 or FIFF_UNIT_AM_M3*/
inline constexpr auto FIFF_MNE_PRIORS_USED = 3543;                 /**< Which kind of priors have been used for the source covariance matrix*/
inline constexpr auto FIFF_MNE_INVERSE_FULL = 3544;                /**< Inverse operator as one matrix*/
                                                                   // This matrix includes the whitening operator as well
                                                                   // The regularization is applied
inline constexpr auto FIFF_MNE_INVERSE_SOURCE_ORIENTATIONS = 3545; /**<  orientation of one source per row*/
                                                                   // The source orientations must be expressed in the coordinate system
                                                                   // given by FIFF_MNE_COORD_FRAME

/*
 * 3550... Saved environment info
 */
inline constexpr auto FIFF_MNE_ENV_WORKING_DIR = 3550;  /**< Working directory where the file was created*/
inline constexpr auto FIFF_MNE_ENV_COMMAND_LINE = 3551; /**< The command used to create the file*/

/*
 * 3560... Miscellaneous
 */
inline constexpr auto FIFF_MNE_PROJ_ITEM_ACTIVE = 3560;   /**< Is this projection item active?*/
inline constexpr auto FIFF_MNE_EVENT_LIST = 3561;         /**< An event list (for STI 014)*/
inline constexpr auto FIFF_MNE_EVENT_TRIGGER_MASK = 3565; /**< Mask applied to the trigger channel values*/
inline constexpr auto FIFF_MNE_EVENT_COMMENTS = 3566;     /**< Event comments, one NUL-terminated string per event*/
inline constexpr auto FIFF_MNE_HEMI = 3562;               /**< Hemisphere association for general purposes*/

/*
 * 3570... Morphing maps
 */
inline constexpr auto FIFF_MNE_MORPH_MAP = 3570;      /**< Mapping of closest vertices on the sphere*/
inline constexpr auto FIFF_MNE_MORPH_MAP_FROM = 3571; /**< Which subject is this map from*/
inline constexpr auto FIFF_MNE_MORPH_MAP_TO = 3572;   /**< Which subject is this map to*/

/*
 * 3580... CTF compensation data
 */
inline constexpr auto FIFF_MNE_CTF_COMP_KIND = 3580;       /**< What kind of compensation*/
inline constexpr auto FIFF_MNE_CTF_COMP_DATA = 3581;       /**< The compensation data itself*/
inline constexpr auto FIFF_MNE_CTF_COMP_CALIBRATED = 3582; /**< Are the coefficients calibrated?*/
inline constexpr auto FIFF_MNE_DERIVATION_DATA = 3585;     /**< Channel derivations (EEG montages and other virtual channels)*/

/*
 * 3700... Real-Time Communication
 */
inline constexpr auto FIFF_MNE_RT_COMMAND = 3700;   /**< Fiff Real-Time Command. */
inline constexpr auto FIFF_MNE_RT_CLIENT_ID = 3701; /**< Fiff Real-Time mne_t_server client id. */

/*
 * 3710... Real-Time Blocks
 */
inline constexpr auto FIFFB_MNE_RT_MEAS_INFO = 3710; /**< Fiff Real-Time Measurement Info. */

/*
 * Fiff values associated with MNE computations
 */
inline constexpr auto FIFFV_MNE_FIXED_ORI = 1;
inline constexpr auto FIFFV_MNE_FREE_ORI = 2;

inline constexpr auto FIFFV_MNE_MEG = 1;
inline constexpr auto FIFFV_MNE_EEG = 2;
inline constexpr auto FIFFV_MNE_MEG_EEG = 3;

inline constexpr auto FIFFV_MNE_UNKNOWN_COV = 0;
inline constexpr auto FIFFV_MNE_SENSOR_COV = 1;
inline constexpr auto FIFFV_MNE_NOISE_COV = 1; /**< This is what it should have been called*/
inline constexpr auto FIFFV_MNE_SOURCE_COV = 2;
inline constexpr auto FIFFV_MNE_FMRI_PRIOR_COV = 3;
inline constexpr auto FIFFV_MNE_SIGNAL_COV = 4;       /**< This will be potentially employed in beamformers*/
inline constexpr auto FIFFV_MNE_DEPTH_PRIOR_COV = 5;  /**< The depth weighting prior*/
inline constexpr auto FIFFV_MNE_ORIENT_PRIOR_COV = 6; /**< The orientation prior*/

/*
 * Source space types (values of FIFF_MNE_SOURCE_SPACE_TYPE)
 */
inline constexpr auto FIFFV_MNE_SPACE_UNKNOWN = -1;
inline constexpr auto FIFFV_MNE_SPACE_SURFACE = 1;
inline constexpr auto FIFFV_MNE_SPACE_VOLUME = 2;
inline constexpr auto FIFFV_MNE_SPACE_DISCRETE = 3;

/*
 * Covariance matrix channel classification
 */
inline constexpr auto FIFFV_MNE_COV_CH_UNKNOWN = -1; /**< No idea*/
inline constexpr auto FIFFV_MNE_COV_CH_MEG_MAG = 0;  /**< Axial gradiometer or magnetometer [T]*/
inline constexpr auto FIFFV_MNE_COV_CH_MEG_GRAD = 1; /**< Planar gradiometer [T/m]*/
inline constexpr auto FIFFV_MNE_COV_CH_EEG = 2;      /**< EEG [V]*/

/*
 * Projection item kinds
 */
inline constexpr auto FIFFV_MNE_PROJ_ITEM_EEG_AVREF = 10;

/*
 * Additional coordinate frames
 */
inline constexpr auto FIFFV_MNE_COORD_TUFTS_EEG = 300;   /**< For Tufts EEG data*/
inline constexpr auto FIFFV_MNE_COORD_CTF_DEVICE = 1001; /**< CTF device coordinates*/
inline constexpr auto FIFFV_MNE_COORD_CTF_HEAD = 1004;   /**< CTF head coordinates*/
inline constexpr auto FIFFV_MNE_COORD_MRI_VOXEL = 2001;  /**< The MRI voxel coordinates*/
inline constexpr auto FIFFV_MNE_COORD_RAS = 2002;        /**< FsSurface RAS coordinates with non-zero origin*/
inline constexpr auto FIFFV_MNE_COORD_MNI_TAL = 2003;    /**< MNI Talairach coordinates*/
inline constexpr auto FIFFV_MNE_COORD_FS_TAL_GTZ = 2004; /**< FreeSurfer Talairach coordinates (MNI z > 0)*/
inline constexpr auto FIFFV_MNE_COORD_FS_TAL_LTZ = 2005; /**< FreeSurfer Talairach coordinates (MNI z < 0)*/
inline constexpr auto FIFFV_MNE_COORD_FS_TAL = 2006;     /**< FreeSurfer Talairach coordinates*/

#endif // FIFF_CONSTANTS_H
