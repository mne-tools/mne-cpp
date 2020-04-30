//=============================================================================================================
/**
 * @file     fiff_explain.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Fiff block and dir tag explainations
 *
 */

#ifndef FIFF_EXPLAIN_H
#define FIFF_EXPLAIN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_types.h"
#include "fiff_file.h"

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
// Explaination Type
//=============================================================================================================

typedef struct {
  int kind;         /* What is this explanation good for? */
  const char *text;
} _fiffExp;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

static _fiffExp _fiff_explanations[] = {
  { FIFF_FILE_ID,         "file ID        " },
  { FIFF_DIR_POINTER,     "dir pointer    " },
  { FIFF_DIR,             "directory      " },
  { FIFF_BLOCK_ID,        "block ID       " },
  { FIFF_BLOCK_START,     "{              " },
  { FIFF_BLOCK_END,       "}              " },
  { FIFF_FREE_LIST,       "free list      " },
  { FIFF_FREE_BLOCK,      "free block     " },
  { FIFF_NOP,             "NOP            " },
  { FIFF_PARENT_FILE_ID,  "parent FID     " },
  { FIFF_PARENT_BLOCK_ID, "parent BID     " },
  { FIFF_BLOCK_NAME,      "block name     " },

  { FIFF_NCHAN,           "nchan          " },
  { FIFF_SFREQ,           "sfreq          " },
  { FIFF_LINE_FREQ,       "linefreq       " },
  { FIFF_DATA_PACK,       "packing        " },
  { FIFF_CH_INFO,         "channel        " },
  { FIFF_MEAS_DATE,       "date           " },
  { FIFF_SUBJECT,         "subject        " },
  { FIFF_COMMENT,         "comment        " },
  { FIFF_EVENT_COMMENT,   "event comment  " },
  { FIFF_NAVE,            "nave           " },
  { FIFF_FIRST_SAMPLE,    "firstsamp      " },
  { FIFF_LAST_SAMPLE,     "lastsamp       " },
  { FIFF_NO_SAMPLES,      "nsamp          " },
  { FIFF_FIRST_TIME,      "time minimum   " },
  { FIFF_ASPECT_KIND,     "aspect type    " },
  { FIFF_REF_EVENT,       "ref. event     " },
  { FIFF_EXPERIMENTER,    "scientist      " },
  { FIFF_DIG_POINT,       "dig. point     " },
  { FIFF_DIG_STRING,      "dig. string    " },
  { FIFF_CH_POS,          "channel pos    " },
  { FIFF_CH_CALS_VEC,     "channel cals   " },
  { FIFF_REQ_EVENT,       "req. event     " },
  { FIFF_REQ_LIMIT,       "req. limit     " },
  { FIFF_LOWPASS,         "lowpass        " },
  { FIFF_HIGHPASS,        "highpass       " },
  { FIFF_BAD_CHS,         "bad chs        " },
  { FIFF_ARTEF_REMOVAL,   "artefacts      " },
  { FIFF_COORD_TRANS,     "transform      " },
  { FIFF_SUBAVE_SIZE,     "subave size    " },
  { FIFF_SUBAVE_FIRST,    "subave first   " },
  { FIFF_NAME,            "block name     " },
  { FIFF_HPI_BAD_CHS,     "HPI bad chs    " },
  { FIFF_HPI_CORR_COEFF,  "HPI cor coef   " },

  { FIFF_HPI_SLOPES,        "HPI slopes     " },
  { FIFF_HPI_NCOIL,         "HPI # coils    " },
  { FIFF_HPI_COIL_MOMENTS,  "HPI moments    " },
  { FIFF_HPI_FIT_GOODNESS,  "HPI g-values   " },
  { FIFF_HPI_FIT_ACCEPT,    "HPI fit accept " },
  { FIFF_HPI_FIT_GOOD_LIMIT,"HPI g limit    " },
  { FIFF_HPI_FIT_DIST_LIMIT,"HPI dist limit " },
  { FIFF_HPI_COIL_NO,       "HPI coil no" },

  { FIFF_DATA_BUFFER,     "data buffer    " },
  { FIFF_DATA_SKIP,       "data skip      " },
  { FIFF_EPOCH,           "epoch          " },

  { FIFF_SUBJ_ID,         "subject id     " },
  { FIFF_SUBJ_FIRST_NAME, "first name     " },
  { FIFF_SUBJ_MIDDLE_NAME,"middle name    " },
  { FIFF_SUBJ_LAST_NAME,  "last name      " },
  { FIFF_SUBJ_BIRTH_DAY,  "birthday       " },
  { FIFF_SUBJ_SEX,        "sex            " },
  { FIFF_SUBJ_HAND,       "handedness     " },
  { FIFF_SUBJ_WEIGHT,     "weight         " },
  { FIFF_SUBJ_HEIGHT,     "height         " },
  { FIFF_SUBJ_COMMENT,    "comment        " },

  { FIFF_PROJ_ID,                  "project id     " },
  { FIFF_PROJ_NAME,                "proj. name     " },
  { FIFF_PROJ_AIM,                 "project aim    " },
  { FIFF_PROJ_PERSONS,             "proj. pers.    " },
  { FIFF_PROJ_COMMENT,             "proj. comm.    " },
  
  { FIFF_EVENT_CHANNELS,           "event ch #'s   " },
  { FIFF_EVENT_LIST,               "event list" },

  { FIFF_MRI_SOURCE_PATH,            "MRI source     " },
  { FIFF_MRI_SOURCE_FORMAT,          "MRI src fmt    " },
  { FIFF_MRI_PIXEL_ENCODING,         "pixel type     " },
  { FIFF_MRI_PIXEL_DATA_OFFSET,      "pixel offset   " },
  { FIFF_MRI_PIXEL_SCALE,            "pixel scale    " },
  { FIFF_MRI_PIXEL_DATA,             "pixel data     " },
  { FIFF_MRI_PIXEL_OVERLAY_DATA,     "overlay data   " },
  { FIFF_MRI_PIXEL_OVERLAY_ENCODING, "overlay type   " },
  { FIFF_MRI_ORIG_SOURCE_PATH,       "MRI orig source" },
  { FIFF_MRI_ORIG_SOURCE_FORMAT,     "MRI orig format" },
  { FIFF_MRI_ORIG_PIXEL_ENCODING,    "MRI opixel type" },
  { FIFF_MRI_ORIG_PIXEL_DATA_OFFSET, "MRI opixel offs" },

  { FIFF_MRI_WIDTH,                "pixel width    " },
  { FIFF_MRI_WIDTH_M,              "real width     " },
  { FIFF_MRI_HEIGHT,               "pixel height   " },
  { FIFF_MRI_HEIGHT_M,             "real height    " },
  { FIFF_MRI_DEPTH,                "pixel depth    " },
  { FIFF_MRI_DEPTH_M,              "real depth     " },
  { FIFF_MRI_THICKNESS,            "slice thickness" },

  { FIFF_SPHERE_ORIGIN,            "sphere orig.   " },
  { FIFF_SPHERE_COORD_FRAME,       "sphere coord fr" },
  { FIFF_SPHERE_LAYERS,            "sphere layers  " },

  { FIFF_BEM_SURF_ID,              "surface id     " },
  { FIFF_BEM_SURF_NAME,            "surface name   " },
  { FIFF_BEM_SURF_NNODE,           "surf.  nnode   " },
  { FIFF_BEM_SURF_NTRI,            "surface ntri   " },
  { FIFF_BEM_SURF_NODES,           "surf.  nodes   " },
  { FIFF_BEM_SURF_TRIANGLES,       "surf. triang   " },
  { FIFF_BEM_SURF_NORMALS,         "surf. normals  " },
  { FIFF_BEM_SURF_CURVS,           "surf. curv. vec" },
  { FIFF_BEM_SURF_CURV_VALUES,     "surf. curv. val" },
  { FIFF_BEM_APPROX,               "BEM method     " },
  { FIFF_BEM_POT_SOLUTION,         "BEM pot. sol.  " },

  { FIFF_SOURCE_DIPOLE,            "source dipole  " },
  { FIFF_XFIT_LEAD_PRODUCTS,       "xfit leadpro   " },
  { FIFF_XFIT_MAP_PRODUCTS,        "xfit mapprod   " },
  { FIFF_XFIT_GRAD_MAP_PRODUCTS,   "xfit gmappro   " },
  { FIFF_XFIT_VOL_INTEGRATION,     "xfit volint    " },
  { FIFF_XFIT_INTEGRATION_RADIUS,  "xfit intrad    " },

  { FIFF_PROJ_ITEM_KIND,           "proj item kind " },
  { FIFF_PROJ_ITEM_TIME,           "proj item time " },
  { FIFF_PROJ_ITEM_IGN_CHS,        "proj item ign  " },
  { FIFF_PROJ_ITEM_NVEC,           "proj nvec      " },
  { FIFF_PROJ_ITEM_VECTORS,        "proj item vect " },
  { FIFF_PROJ_ITEM_COMMENT,        "comment        " },
  { FIFF_PROJ_ITEM_CH_NAME_LIST,   "proj item chs  " },

  { FIFF_VOL_ID,                   "volume id      " },
  { FIFF_VOL_NAME,                 "volume name    " },
  { FIFF_VOL_OWNER_ID,             "volume uid     " },
  { FIFF_VOL_OWNER_NAME,           "volume uname   " },
  { FIFF_VOL_OWNER_REAL_NAME,      "vol.  urname   " },
  { FIFF_VOL_TYPE,                 "volume type    " },
  { FIFF_VOL_HOST,                 "volume host    " },
  { FIFF_VOL_MOUNT_POINT,          "volume mntpt   " },
  { FIFF_VOL_BLOCKS,               "vol.  blocks   " },
  { FIFF_VOL_AVAIL_BLOCKS,         "vol. ablocks   " },
  { FIFF_VOL_FREE_BLOCKS,          "vol. fblocks   " },
  { FIFF_VOL_BLOCK_SIZE,           "volume bsize   " }, 
  { FIFF_VOL_REAL_ROOT,            "volume rroot   " },
  { FIFF_VOL_SYMBOLIC_ROOT,        "volume sroot   " },
  { FIFF_VOL_DIRECTORY,            "volume dir     " },
  { FIFF_INDEX,                    "index          " },
  { FIFF_INDEX_KIND,               "index kind     " },
  { FIFF_DACQ_PARS,                "acq. pars.     " },
  { FIFF_DACQ_STIM,                "acq. stim seq. " },
  { -1,NULL} };

//=============================================================================================================

static _fiffExp _fiff_block_explanations[] = {
  { FIFFB_ROOT,           "root          " },
  { FIFFB_MEAS,           "measurement   " },
  { FIFFB_MEAS_INFO,      "meas. info    " },
  { FIFFB_RAW_DATA,       "raw data      " },
  { FIFFB_PROCESSED_DATA, "proc. data    " },
  { FIFFB_EVOKED,         "evoked data   " },
  { FIFFB_ASPECT,         "data aspect   " },
  { FIFFB_SUBJECT,        "subject       " },
  { FIFFB_ISOTRAK,        "isotrak       " },
  { FIFFB_HPI_MEAS,       "HPI meas      " },
  { FIFFB_HPI_RESULT,     "HPI result    " },
  { FIFFB_HPI_COIL,       "HPI coil      " },
  { FIFFB_PROJECT,        "project       " },
  { FIFFB_CONTINUOUS_DATA,"cont. data    " },
  { FIFFB_VOID,           "anything      " },
  { FIFFB_EVENTS,         "events        " },
  { FIFFB_DACQ_PARS,      "acq. pars.    " },
  { FIFFB_MRI,            "MRI data      " },
  { FIFFB_MRI_SET,        "MRI set       " },
  { FIFFB_MRI_SLICE,      "MRI slice     " },
  { FIFFB_MRI_SCENERY,	"MRI scenery   " },
  { FIFFB_MRI_SCENE,	"MRI scene     " },
  { FIFFB_SPHERE,         "Sphere mod.   " },
  { FIFFB_BEM,            "BEM data      " },
  { FIFFB_BEM_SURF,       "BEM surf      " },
  { FIFFB_XFIT_AUX,       "xfit aux      " },
  { FIFFB_XFIT_PROJ,      "projection    " },
  { FIFFB_XFIT_PROJ_ITEM, "proj. item    " },
  { FIFFB_VOL_INFO,       "volume info   " },
  { FIFFB_INDEX,          "index         " },
  { -1,                   NULL} };

//=============================================================================================================

static _fiffExp _fiff_unit_explanations[] = {
  /*
   * SI base units
   */
  { FIFF_UNIT_M, "m" },
  { FIFF_UNIT_KG,"kg" },
  { FIFF_UNIT_SEC,  "s" },
  { FIFF_UNIT_A, "A" },
  { FIFF_UNIT_K, "K" },
  { FIFF_UNIT_MOL,  "mol" },
  /*
   * SI Supplementary units
   */
  { FIFF_UNIT_RAD,  "rad" },
  { FIFF_UNIT_SR,"sr" },
  /*
   * SI derived units
   */
  { FIFF_UNIT_HZ,"Hz" },
  { FIFF_UNIT_N, "N" },
  { FIFF_UNIT_PA,"Pa" },
  { FIFF_UNIT_J, "J" },
  { FIFF_UNIT_W, "W" },
  { FIFF_UNIT_C, "C" },
  { FIFF_UNIT_V, "V" },
  { FIFF_UNIT_F, "F" },
  { FIFF_UNIT_OHM,  "ohm" },
  { FIFF_UNIT_MHO,  "S" },
  { FIFF_UNIT_WB,"Wb" },
  { FIFF_UNIT_T, "T" },
  { FIFF_UNIT_H, "H" },
  { FIFF_UNIT_CEL,  "C" },
  { FIFF_UNIT_LM,"lm" },
  { FIFF_UNIT_LX,"lx" },
  /*
   * Others we need
   */
  { FIFF_UNIT_T_M,"T/m" }, /* T/m */ 
  { -1, NULL }};

static _fiffExp _fiff_unit_mul_explanations[] = {
  { FIFF_UNITM_E, "E" },
  { FIFF_UNITM_PET,  "P" },
  { FIFF_UNITM_T, "T" },
  { FIFF_UNITM_MEG,  "M" },
  { FIFF_UNITM_K, "k" },
  { FIFF_UNITM_H, "h" },
  { FIFF_UNITM_DA,"da" },
  { FIFF_UNITM_NONE, "" },
  { FIFF_UNITM_D, "d" },
  { FIFF_UNITM_C, "c" },
  { FIFF_UNITM_M, "m" },
  { FIFF_UNITM_MU,"u" },
  { FIFF_UNITM_N, "n" },
  { FIFF_UNITM_P, "p" },
  { FIFF_UNITM_F, "f" },
  { FIFF_UNITM_A, "a" },
  {-1, NULL }};
} // NAMESPACE

#endif // FIFF_EXPLAIN_H
