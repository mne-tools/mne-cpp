//=============================================================================================================
/**
 * @file     mne_msh_display_surface_set.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief    MNEMshDisplaySurfaceSet class declaration.
 *
 */

#ifndef MNEMSHDISPLAYSURFACESET_H
#define MNEMSHDISPLAYSURFACESET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

#include <memory>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================



//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

class MNEMshDisplaySurface;
class MNEMshLightSet;
class MNESurfacePatch;

//=============================================================================================================
/**
 * Replaces  *mshDisplaySurfaceSet, mshDisplaySurfaceSetRec struct (analyze_types.c). Note that another implementation can be found in movie_types.h
 *
 * @brief The MNE Msh Display FsSurface Set class holds information about a set of surfaces to be rendered.
 */
class MNESHARED_EXPORT MNEMshDisplaySurfaceSet
{
public:
    typedef QSharedPointer<MNEMshDisplaySurfaceSet> SPtr;              /**< Shared pointer type for MNEMshDisplaySurfaceSet. */
    typedef QSharedPointer<const MNEMshDisplaySurfaceSet> ConstSPtr;   /**< Const shared pointer type for MNEMshDisplaySurfaceSet. */

    //=========================================================================================================
    /**
     * Constructs the MNEMshDisplaySurfaceSet.
     */
    MNEMshDisplaySurfaceSet(int nsurf = 0);

    //=========================================================================================================
    /**
     * Destroys the MNEMshDisplaySurfaceSet.
     */
    ~MNEMshDisplaySurfaceSet();

    /**
     * Load left and right hemisphere FreeSurfer surfaces (with curvature) from
     * the subjects directory, configuring eye positions and lighting.
     *
     * @param[in] subject_id    Subject name.
     * @param[in] surf          FsSurface type name (e.g. "inflated", "white").
     * @param[in] subjects_dir  Path to the FreeSurfer subjects directory.
     *
     * @return A new display surface set, or NULL on failure. Caller takes ownership.
     */
    static std::unique_ptr<MNEMshDisplaySurfaceSet> load(const QString &subject_id, const QString &surf, const QString &subjects_dir);



    /**
     * Read a BEM surface from a FIFF file, optionally validate that it is
     * topologically closed (via solid-angle check), and add it to this set.
     *
     * @param[in] filepath    Path to the BEM FIFF file.
     * @param[in] kind        BEM surface kind constant.
     * @param[in] bemname     Display name for the surface.
     * @param[in] full_geom   If non-zero, compute full geometry information.
     * @param[in] check       If non-zero, verify the surface is closed.
     *
     * @return OK on success, FAIL on error.
     */
    int add_bem_surface(const QString&       filepath,
                        int                  kind,
                        const QString&       bemname,
                        int                  full_geom,
                        int                  check);

    /**
     * Add a new surface to the set, or replace an existing surface with the
     * same ID if @p replace is true.
     *
     * @param[in] newSurf    The surface to add or use as replacement.
     * @param[in] replace    If true, replace an existing surface with matching ID.
     * @param[in] drawable   Whether the surface should be marked as drawable.
     */
    void add_replace_surface(std::unique_ptr<MNEMshDisplaySurface> newSurf,
                             bool                  replace,
                             bool                  drawable);



    /**
     * Assign hemisphere-appropriate eye positions and up-vectors to each
     * surface (left eye for left hemisphere, right eye for right).
     */
    void apply_left_right_eyes();

    /**
     * Apply the left-eye position and up-vector uniformly to all surfaces
     * regardless of hemisphere.
     */
    void apply_left_eyes();

    /**
     * Initialize the custom light set (if needed) and apply it as the
     * active lighting configuration.
     */
    void setup_current_lights();

    /**
     * Create a default set of 8 directional lights if custom lights have
     * not been initialized yet.
     */
    static void initialize_custom_lights();

    /**
     * Deep-copy a light set including all individual light entries.
     *
     * @param[in] s   The light set to duplicate.
     *
     * @return A new deep copy. Caller takes ownership.
     */
    static std::unique_ptr<MNEMshLightSet> dup_light_set(const MNEMshLightSet& s);

    /**
     * Replace the current active lighting with a deep copy of the provided
     * light set.
     *
     * @param[in] set   The light set to apply.
     */
    void setup_lights(const MNEMshLightSet& set);

public:
    QString           subj;	       /**< The name of the subject. */
    QString           morph_subj;       /**< The subject we are morphing to. */
    std::vector<std::unique_ptr<MNEMshDisplaySurface>> surfs; /**< Array of display surfaces (owned). */
    std::vector<std::unique_ptr<MNESurfacePatch>>  patches;    /**< Optional flat patches for display (owned). */
    std::vector<float>            patch_rot;            /**< Rotation angles for the (flat) patches. */
    int               nsurf;	       /**< Number of surfaces. */
    bool              use_patches;       /**< Whether to use patches for display. */
    Eigen::VectorXi   active;	       /**< Boolean array indicating which surfaces are currently active. */
    Eigen::VectorXi   drawable;	       /**< Boolean array indicating which surfaces could be drawn. */
    std::unique_ptr<MNEMshLightSet> lights;  /**< Current active lighting configuration. */
    float             rot[3];            /**< Rotation angles of the MRI (in radians). */
    float             move[3];	       /**< Translation offset for the origin. */
    float             fov;	       /**< Field of view (extent of the surface). */
    float             fov_scale;	       /**< Scale factor for extra space around FOV. */
    float             eye[3];	       /**< Eye position for viewing (used in composite views). */
    float             up[3];	       /**< Up vector for viewing. */
    float             bg_color[3];       /**< Background color (RGB). */
    float             text_color[3];     /**< Text color (RGB). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEMSHDISPLAYSURFACESET_H
