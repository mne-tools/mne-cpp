#ifndef _mne_types_h
#define _mne_types_h

//=============================================================================================================
/**
 * @file     mne_types.h
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Legacy MNE-C constants and common typedefs.
 *
 * Provides the constants originally defined in the MNE-C @c mne_types.h
 * (source-space discriminators, CTF/4D-compensation grades,
 * channel-selection origin tags, trigger-channel defaults) together with
 * the @c mneUserFreeFunc callback typedef.
 *
 * This header also re-exports the individual MNE class headers that
 * replaced the original C struct definitions, so that existing code
 * relying on the umbrella include continues to compile.  New code
 * should include the specific class headers directly.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_types.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_sparse_matrix.h>

#include "mne_sss_data.h"
#include "mne_named_matrix.h"
#include "mne_sparse_named_matrix.h"
#include "mne_named_vector.h"
#include "mne_deriv_set.h"
#include "mne_proj_item.h"
#include "mne_proj_op.h"
#include "mne_cov_matrix.h"
#include "mne_ctf_comp_data.h"
#include "mne_ctf_comp_data_set.h"
#include "mne_layout_port.h"
#include "mne_layout.h"
#include "mne_ch_selection.h"
#include "mne_event.h"
#include "mne_event_list.h"
#include "mne_filter_def.h"

#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB {

//=============================================================================================================
// TYPEDEFS
//=============================================================================================================

/**
 * @brief Generic destructor callback for user-attached data.
 *
 * Several legacy structures carry a @c void* user_data slot together
 * with a matching @c mneUserFreeFunc that is called when the owning
 * object is destroyed.
 */
typedef void (*mneUserFreeFunc)(void *);

//=============================================================================================================
// SOURCE-SPACE TYPE CONSTANTS
//=============================================================================================================

/**
 * @name Source-space types
 * Discriminators for the @c type field in MNESurfaceOrVolume / MNESourceSpace.
 * @{
 */
#define MNE_SOURCE_SPACE_UNKNOWN  -1  /**< Type not yet determined. */
#define MNE_SOURCE_SPACE_SURFACE   1  /**< FsSurface-based source space. */
#define MNE_SOURCE_SPACE_VOLUME    2  /**< Volumetric (3-D grid) source space. */
#define MNE_SOURCE_SPACE_DISCRETE  3  /**< Discrete point set. */
/** @} */

//=============================================================================================================
// CHANNEL-SELECTION KIND CONSTANTS
//=============================================================================================================

/**
 * @name Channel-selection kinds
 * Origin tag for MNEChSelection records.
 * @{
 */
#define MNE_CH_SELECTION_UNKNOWN  0  /**< Unknown origin. */
#define MNE_CH_SELECTION_FILE     1  /**< Loaded from file. */
#define MNE_CH_SELECTION_USER     2  /**< Created interactively. */
/** @} */

//=============================================================================================================
// CTF / 4D COMPENSATION CONSTANTS
//=============================================================================================================

/**
 * @name CTF compensation grades
 * Software gradient-compensation levels for CTF systems.
 * @{
 */
#define MNE_CTFV_NOGRAD  0  /**< No gradient compensation. */
#define MNE_CTFV_GRAD1   1  /**< 1st-order gradient compensation. */
#define MNE_CTFV_GRAD2   2  /**< 2nd-order gradient compensation. */
#define MNE_CTFV_GRAD3   3  /**< 3rd-order gradient compensation. */
/** @} */

/**
 * @name 4D Neuroimaging compensation
 * @{
 */
#define MNE_4DV_COMP1  101  /**< 4D Neuroimaging 1st-order compensation. */
/** @} */

//=============================================================================================================
// TRIGGER AND ENVIRONMENT DEFAULTS
//=============================================================================================================

/** @brief Default digital trigger channel name. */
#define MNE_DEFAULT_TRIGGER_CH  "STI 014"

/** @brief Environment variable overriding the trigger channel name. */
#define MNE_ENV_TRIGGER_CH      "MNE_TRIGGER_CH_NAME"

/** @brief Environment variable pointing to the MNE installation root. */
#define MNE_ENV_ROOT            "MNE_ROOT"

} // namespace MNELIB

#endif // _mne_types_h
