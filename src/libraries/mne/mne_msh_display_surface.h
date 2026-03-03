//=============================================================================================================
/**
 * @file     mne_msh_display_surface.h
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
 * @brief    MneMshDisplaySurface class declaration.
 *
 */

#ifndef MNEMSHDISPLAYSURFACE_H
#define MNEMSHDISPLAYSURFACE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_types.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class MneMorphMap;
class MneMshPicked;
class MneMshColorScaleDef;
class MneSurfaceOld;

//=============================================================================================================
/**
 * Replaces *mshDisplaySurface, mshDisplaySurfaceRec struct (analyze_types.c).
 *
 * @brief The MNE Msh Display Surface class holds information about a surface to be rendered.
 */
class MNESHARED_EXPORT MneMshDisplaySurface
{
public:
    typedef QSharedPointer<MneMshDisplaySurface> SPtr;              /**< Shared pointer type for MneMshDisplaySurface. */
    typedef QSharedPointer<const MneMshDisplaySurface> ConstSPtr;   /**< Const shared pointer type for MneMshDisplaySurface. */

    //=========================================================================================================
    /**
     * Constructs the MneMshDisplaySurface.
     */
    MneMshDisplaySurface();

    //=========================================================================================================
    /**
     * Destroys the MneMshDisplaySurface.
     */
    ~MneMshDisplaySurface();

public:
    QString        filename;	/**< Path to the file this surface was loaded from. */
    time_t         time_loaded = 0;	/**< Timestamp when the surface was loaded. */
    QString        subj;		/**< Subject name in SUBJECTS_DIR. */
    QString        surf_name;	/**< Name of the surface (e.g., "inflated", "white"). */
    std::unique_ptr<MNELIB::MneSurfaceOld> s;		/**< Underlying surface mesh data. */
    Eigen::Vector3f eye{1.0f, 0.0f, 0.0f};	/**< Eye position for 3D viewing. */
    Eigen::Vector3f up{0.0f, 0.0f, 1.0f};		/**< Up vector for 3D viewing. */
    Eigen::Vector3f rot = Eigen::Vector3f::Zero();        /**< Rotation angles of the MRI (in radians). */
    Eigen::Vector3f move = Eigen::Vector3f::Zero();	/**< Translation offset for the origin. */

    float          fov = 2.0f;		/**< Field of view (extent of the surface). */
    float          fov_scale = 1.0f;	/**< Scale factor for extra space around FOV. */
    Eigen::Vector3f minv = Eigen::Vector3f::Constant(-1.0f);	/**< Minimum values along the three coordinate axes. */
    Eigen::Vector3f maxv = Eigen::Vector3f::Constant(1.0f);	/**< Maximum values along the three coordinate axes. */
    Eigen::Matrix4f trans = Eigen::Matrix4f::Identity();	/**< Extra 4x4 transformation matrix for this surface. */
    int            sketch = 0;	/**< Non-zero to use sketch mode with decimated triangulation. */

    std::vector<std::unique_ptr<MNELIB::MneMorphMap>> maps;		/**< Morphing maps from other surfaces to this one. */

    int   overlay_type = 0;	        /**< Type identifier for the overlay values. */
    Eigen::VectorXf overlay_values;	/**< Per-vertex overlay value array. */
    int   alt_overlay_type = 0;	/**< Alternative overlay type identifier. */
    Eigen::VectorXf alt_overlay_values;  /**< Per-vertex alternative overlay values. */
    Eigen::VectorXf marker_values;		/**< Per-vertex marker values (shown in shades of marker color). */

    Eigen::VectorXf vertex_colors;		/**< Per-vertex RGBA color array. */
    std::unique_ptr<MNELIB::MneMshColorScaleDef> color_scale; /**< Color scale used to compute vertex colors. */
    int   nvertex_colors = 3;		/**< Number of color components per vertex. */
    Eigen::Vector4f even_vertex_color = Eigen::Vector4f::Zero();	/**< Uniform RGBA color for non-data-driven coloring. */

    Eigen::VectorXf marker_colors;		/**< Per-vertex marker color array. */
    int   nmarker_colors = 3;		/**< Number of color components per marker vertex. */
    Eigen::MatrixXi marker_tri;	        /**< Array of triangles containing markers. */
    Eigen::VectorXi marker_tri_no;		/**< Triangle indices of the marker triangles. */
    int   nmarker_tri = 0;		/**< Number of marker triangles. */
    Eigen::Vector4f marker_color = Eigen::Vector4f::Zero();	/**< RGBA color for markers. */
    int   curvature_color_mode = 0;	/**< How curvature is visualized. */

    int   overlay_color_mode = 0;	/**< How overlay data affects coloring. */
    int   transparent = 0;		/**< Non-zero if this surface is rendered transparently. */

    int   show_aux_data = 0;		/**< Non-zero to show auxiliary data related to this surface. */

    std::vector<MNELIB::MneMshPicked> picked;		/**< Picked locations in world coordinates. */

    void              *user_data = nullptr;       /**< Arbitrary user-defined data pointer. */
    mneUserFreeFunc    user_data_free = nullptr;   /**< Function to free user_data. */

// ### OLD STRUCT ###
//    typedef struct {		/* Display surface properties */
//      char           *filename;	/* Where did this surface come from? */
//      time_t         time_loaded;	/* When was the surface loaded */
//      char           *subj;		/* The name of the subject in SUBJECTS_DIR */
//      char           *surf_name;	/* The name of the surface */
//      MNELIB::MneSurfaceOld*     s;		/* This is the surface */
//      float          eye[3];	/* Eye position for viewing */
//      float          up[3];		/* Up vector for viewing */
//      float          rot[3];        /* Rotation angles of the MRI (in radians) */
//      float          move[3];	/* Possibly move the origin, too */

//      float          fov;		/* Field of view (extent of the surface) */
//      float          fov_scale;	/* How much space to leave */
//      float          minv[3];	/* Minimum values along the three coordinate axes */
//      float          maxv[3];	/* Maximum values along the three coordinate axes */
//      float          *trans;	/* Extra transformation for this surface */
//      int            sketch;	/* Use sketch mode if decimated triangulation is available? */

//      morphMap       *maps;		/* Morphing maps from other surfaces to this */
//      int            nmap;		/* Normally just one */

//      int   overlay_type;	        /* What are the overlay values? */
//      float *overlay_values;	/* Overlay value array */
//      int   alt_overlay_type;	/* A second choice for overlay */
//      float *alt_overlay_values;
//      float *marker_values;		/* Marker values (will be shown in shades of marker color) */

//      float *vertex_colors;		/* Vertex color array */
//      mshColorScaleDef* color_scale; /* Color scale used to define these colors */
//      int   nvertex_colors;		/* How many components? */
//      float even_vertex_color[4];	/* This is going to be employed in case of uniform coloring */

//      float *marker_colors;		/* Vertex color array (for the markers) */
//      int   nmarker_colors;		/* How many components? */
//      int   **marker_tri;	        /* Triangles containing markers */
//      int   *marker_tri_no;		/* Numbers of the marker triangles */
//      int   nmarker_tri;		/* How many */
//      float marker_color[4];	/* Marker color */
//      int   curvature_color_mode;	/* How to show curvature */

//      int   overlay_color_mode;	/* How to show overlay data */
//      int   transparent;		/* Is this surface going to be transparent? */

//      int   show_aux_data;		/* Show auxilliary data related to this surface */

//      mshPicked* picked;		/* Picked locations in world coordinates */
//      int       npicked;		/* How many */

//      void              *user_data;       /* Can be used to store whatever */
//      mneUserFreeFunc*   user_data_free;   /* Function to free the above */
//    } *mshDisplaySurface,mshDisplaySurfaceRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEMSHDISPLAYSURFACE_H
