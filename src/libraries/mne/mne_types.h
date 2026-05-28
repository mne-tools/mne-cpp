//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_types.h
 * @since 2026
 * @date  March 2026
 * @brief Legacy MNE-C constants and shared typedefs used across MNELIB structures.
 *
 * Mirrors a curated subset of the @c mne_types.h shipped with the
 * original MNE C tooling so ported code can continue to refer to the
 * familiar enumerations (point types, surface ids, BEM coordinate frames,
 * covariance kinds, ...) without dragging the full C header tree into
 * every consumer. Definitions here are pure typedefs / @c enum / @c
 * #define and carry no implementation, keeping the header safe to include
 * from any layer of MNELIB.
 */

#ifndef _mne_types_h
#define _mne_types_h

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
