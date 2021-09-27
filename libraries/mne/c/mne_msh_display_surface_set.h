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
 * @brief    MneMshDisplaySurfaceSet class declaration.
 *
 */

#ifndef MNEMSHDISPLAYSURFACESET_H
#define MNEMSHDISPLAYSURFACESET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_global.h"

typedef void (*mneUserFreeFunc)(void *);  /* General purpose */

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffCoordTransSet;
}

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

class MneMshDisplaySurface;
class MneMshLightSet;
class MneSurfacePatch;

//=============================================================================================================
/**
 * Replaces  *mshDisplaySurfaceSet, mshDisplaySurfaceSetRec struct (analyze_types.c). Note that another implementation can be found in movie_types.h
 *
 * @brief The MNE Msh Display Surface Set class holds information about a set of surfaces to be rendered.
 */
class MNESHARED_EXPORT MneMshDisplaySurfaceSet
{
public:
    typedef QSharedPointer<MneMshDisplaySurfaceSet> SPtr;              /**< Shared pointer type for MneMshDisplaySurfaceSet. */
    typedef QSharedPointer<const MneMshDisplaySurfaceSet> ConstSPtr;   /**< Const shared pointer type for MneMshDisplaySurfaceSet. */

    //=========================================================================================================
    /**
     * Constructs the MneMshDisplaySurfaceSet.
     */
    MneMshDisplaySurfaceSet(int nsurf = 0);

    //=========================================================================================================
    /**
     * Destroys the MneMshDisplaySurfaceSet.
     */
    ~MneMshDisplaySurfaceSet();

    //============================= display_surfaces.c =============================

    static MneMshDisplaySurfaceSet* load_new_surface(const QString &subject_id, const QString &surf, const QString &subjects_dir);

    static void decide_surface_extent(MneMshDisplaySurface* surf,
                                                        const char *tag);

    static void decide_curv_display(const char *name,
                    MneMshDisplaySurface* s);

    static int add_bem_surface(MneMshDisplaySurfaceSet* surfs,
                        QString              filepath,
                        int                  kind,
                        QString              bemname,
                        int                  full_geom,
                        int                  check);

    static void add_replace_display_surface(MneMshDisplaySurfaceSet* surfs,
                                            MneMshDisplaySurface*    newSurf,
                                            bool                  replace,
                                            bool                  drawable);

    //============================= vertex_colors.c =============================

    static void setup_curvature_colors(MneMshDisplaySurface* surf);

    //============================= eyes.c =============================

    static void apply_left_right_eyes(MneMshDisplaySurfaceSet* surfs);

    static void apply_left_eyes(MneMshDisplaySurfaceSet* surfs);

    //============================= lights.c =============================

    static void setup_current_surface_lights(MneMshDisplaySurfaceSet* surfs);

    static void initialize_custom_lights();

    static MneMshLightSet* dup_light_set(MneMshLightSet* s);

    static void setup_these_surface_lights(MneMshDisplaySurfaceSet* surfs, MneMshLightSet* set);

public:
    char              *subj;	       /* The name of the subject */
    char              *morph_subj;       /* The subject we are morphing to */
    FIFFLIB::FiffCoordTransSet     *main_t;            /* Coordinate transformations for the main surfaces */
    FIFFLIB::FiffCoordTransSet     *morph_t;           /* Coordinate transformations for the morph surfaces */
    MneMshDisplaySurface **surfs;	       /* These are the surfaces */
    MneSurfacePatch   **patches;	       /* Optional patches for display */
    float             *patch_rot;	       /* Rotation angles for the (flat) patches */
    int               nsurf;	       /* How many? */
    int               use_patches;       /* Use patches for display? */
    int               *active;	       /* Which surfaces are currently active */
    int               *drawable;	       /* Which surfaces could be drawn? */
    MneMshLightSet*       lights;            /* Lighting */
    float             rot[3];            /* Rotation angles of the MRI (in radians) */
    float             move[3];	       /* Possibly move the origin, too */
    float             fov;	       /* Field of view (extent of the surface) */
    float             fov_scale;	       /* How much space to leave */
    float             eye[3];	       /* Eye position for viewing (used in composite views) */
    float             up[3];	       /* Up vector for viewing */
    float             bg_color[3];       /* Background color */
    float             text_color[3];     /* Text color */
    void              *user_data;        /* Can be used to store whatever */
    mneUserFreeFunc   user_data_free;

// ### OLD STRUCT ###
//    typedef struct {		       /* Set of display surfaces */
//      char              *subj;	       /* The name of the subject */
//      char              *morph_subj;       /* The subject we are morphing to */
//      coordTransSet     main_t;            /* Coordinate transformations for the main surfaces */
//      coordTransSet     morph_t;           /* Coordinate transformations for the morph surfaces */
//      mshDisplaySurface *surfs;	       /* These are the surfaces */
//      mneSurfacePatch   *patches;	       /* Optional patches for display */
//      float             *patch_rot;	       /* Rotation angles for the (flat) patches */
//      int               nsurf;	       /* How many? */
//      int               use_patches;       /* Use patches for display? */
//      int               *active;	       /* Which surfaces are currently active */
//      int               *drawable;	       /* Which surfaces could be drawn? */
//      mshLightSet       lights;            /* Lighting */
//      float             rot[3];            /* Rotation angles of the MRI (in radians) */
//      float             move[3];	       /* Possibly move the origin, too */
//      float             fov;	       /* Field of view (extent of the surface) */
//      float             fov_scale;	       /* How much space to leave */
//      float             eye[3];	       /* Eye position for viewing (used in composite views) */
//      float             up[3];	       /* Up vector for viewing */
//      float             bg_color[3];       /* Background color */
//      float             text_color[3];     /* Text color */
//      void              *user_data;        /* Can be used to store whatever */
//      mneUserFreeFunc   user_data_free;
//    } *mshDisplaySurfaceSet,mshDisplaySurfaceSetRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEMSHDISPLAYSURFACESET_H
