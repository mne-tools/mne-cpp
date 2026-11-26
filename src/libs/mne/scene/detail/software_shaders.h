//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     software_shaders.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Software implementations of the shaders of the views in mne_scene.
 */

#ifndef SCENE_DETAIL_SOFTWARE_SHADERS_H
#define SCENE_DETAIL_SOFTWARE_SHADERS_H

//=============================================================================================================
// DEFINE NAMESPACE mne::scene::detail
//=============================================================================================================

namespace mne::scene
{

class SoftwareRenderer;

namespace detail
{

/**
 * Registers the C++ counterparts of the GLSL shaders the views in mne_scene use.
 *
 * @param[in, out] renderer   Renderer to register them with.
 */
void register_builtin_software_shaders(SoftwareRenderer& renderer);

} // namespace detail
} // namespace mne::scene

#endif // SCENE_DETAIL_SOFTWARE_SHADERS_H
