//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_msh_eyes.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Mesh-viewer camera state (position, focal point, up vector, field of view).
 *
 * @ref mne::MNEMshEyes captures the camera parameters that the
 * @c tksurfer / @c mneAnalyze viewers serialised next to a scene; used
 * by mne-cpp's 3D viewers to reproduce a saved viewpoint exactly.
 */

#ifndef MNEMSHEYES_H
#define MNEMSHEYES_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Eye/camera position and gaze direction for 3-D surface rendering.
 *
 * Stores left and right hemisphere viewpoints and up-vectors.
 */
class MNESHARED_EXPORT MNEMshEyes
{
public:
    //=========================================================================================================
    /**
     * Constructs an empty MNEMshEyes.
     */
    MNEMshEyes() = default;

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~MNEMshEyes() = default;

public:
    std::string name;       /**< Name of this viewpoint definition. */
    float left[3] = {};     /**< Left hemisphere viewpoint (x, y, z). */
    float right[3] = {};    /**< Right hemisphere viewpoint (x, y, z). */
    float left_up[3] = {};  /**< Left hemisphere up-vector. */
    float right_up[3] = {}; /**< Right hemisphere up-vector. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE mne_mne

#endif // MNEMSHEYES_H
