//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     mne_types.h
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     December 2016
 * @brief    Legacy MNE-C constants and shared typedefs used across mne_mne structures.
 *
 * Mirrors a curated subset of the @c mne_types.h shipped with the
 * original MNE C tooling so ported code can continue to refer to the
 * familiar constants (source-space types, channel-selection origins, CTF
 * compensation grades, the default trigger channel) without dragging the
 * full C header tree into every consumer. It holds constants only and
 * includes no other mne_mne header.
 */

#ifndef MNE_TYPES_H
#define MNE_TYPES_H

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// SOURCE-SPACE TYPE CONSTANTS
//=============================================================================================================

/**
 * @name Source-space types
 * Discriminators for the @c type field in MNESurfaceOrVolume / MNESourceSpace.
 * @{
 */
inline constexpr auto MNE_SOURCE_SPACE_UNKNOWN = -1; /**< Type not yet determined. */
inline constexpr auto MNE_SOURCE_SPACE_SURFACE = 1;  /**< FsSurface-based source space. */
inline constexpr auto MNE_SOURCE_SPACE_VOLUME = 2;   /**< Volumetric (3-D grid) source space. */
inline constexpr auto MNE_SOURCE_SPACE_DISCRETE = 3; /**< Discrete point set. */
/** @} */

//=============================================================================================================
// CHANNEL-SELECTION KIND CONSTANTS
//=============================================================================================================

/**
 * @name Channel-selection kinds
 * Origin tag for MNEChSelection records.
 * @{
 */
inline constexpr auto MNE_CH_SELECTION_UNKNOWN = 0; /**< Unknown origin. */
inline constexpr auto MNE_CH_SELECTION_FILE = 1;    /**< Loaded from file. */
inline constexpr auto MNE_CH_SELECTION_USER = 2;    /**< Created interactively. */
/** @} */

//=============================================================================================================
// CTF / 4D COMPENSATION CONSTANTS
//=============================================================================================================

/**
 * @name CTF compensation grades
 * Software gradient-compensation levels for CTF systems.
 * @{
 */
inline constexpr auto MNE_CTFV_NOGRAD = 0; /**< No gradient compensation. */
inline constexpr auto MNE_CTFV_GRAD1 = 1;  /**< 1st-order gradient compensation. */
inline constexpr auto MNE_CTFV_GRAD2 = 2;  /**< 2nd-order gradient compensation. */
inline constexpr auto MNE_CTFV_GRAD3 = 3;  /**< 3rd-order gradient compensation. */
/** @} */

/**
 * @name CTF compensation grades as stored in CTF channel info (ASCII "G1BR", "G2OI", ...)
 * @{
 */
inline constexpr auto MNE_CTFV_COMP_UNKNOWN = -1;      /**< Grade not recognized. */
inline constexpr auto MNE_CTFV_COMP_NONE = 0;          /**< No compensation. */
inline constexpr auto MNE_CTFV_COMP_G1BR = 0x47314252; /**< 1st-order gradient, balanced reference. */
inline constexpr auto MNE_CTFV_COMP_G2BR = 0x47324252; /**< 2nd-order gradient, balanced reference. */
inline constexpr auto MNE_CTFV_COMP_G3BR = 0x47334252; /**< 3rd-order gradient, balanced reference. */
inline constexpr auto MNE_CTFV_COMP_G2OI = 0x47324f49; /**< 2nd-order gradient, off-diagonal. */
inline constexpr auto MNE_CTFV_COMP_G3OI = 0x47334f49; /**< 3rd-order gradient, off-diagonal. */
/** @} */

/**
 * @name 4D Neuroimaging compensation
 * @{
 */
inline constexpr auto MNE_4DV_COMP1 = 101; /**< 4D Neuroimaging 1st-order compensation. */
/** @} */

//=============================================================================================================
// TRIGGER AND ENVIRONMENT DEFAULTS
//=============================================================================================================

/** @brief Default digital trigger channel name. */
inline constexpr std::string_view MNE_DEFAULT_TRIGGER_CH = "STI 014";

/** @brief Environment variable overriding the trigger channel name; a C string for getenv(). */
inline constexpr const char* MNE_ENV_TRIGGER_CH = "MNE_TRIGGER_CH_NAME";

} // namespace mne

#endif // MNE_TYPES_H
