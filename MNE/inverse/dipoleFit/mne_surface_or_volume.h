//=============================================================================================================
/**
* @file     mne_mne_data.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    MNE Surface or Volume (MneSurfaceOrVolume) class declaration.
*
*/

#ifndef MNESURFACEORVOLUME_H
#define MNESURFACEORVOLUME_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inverse_global.h"
#include "mne_types.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QStringList>
#include <QDebug>


//============================= mne_fiff.h =============================

#define FIFF_MNE_SOURCE_SPACE_NNEIGHBORS    3594    /* Number of neighbors for each source space point (used for volume source spaces) */
#define FIFF_MNE_SOURCE_SPACE_NEIGHBORS     3595    /* Neighbors for each source space point (used for volume source spaces) */

#define FIFFV_MNE_COORD_SURFACE_RAS   FIFFV_COORD_MRI    /* The surface RAS coordinates */


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

//=============================================================================================================
/**
* Implements MNE Surface or Volume (Replaces *mneSurfaceOrVolume,mneSurfaceOrVolumeRec; struct of MNE-C mne_types.h).
*
* @brief This defines a source space or a surface
*/
class INVERSESHARED_EXPORT MneSurfaceOrVolume
{
public:
    typedef QSharedPointer<MneSurfaceOrVolume> SPtr;              /**< Shared pointer type for MneSurfaceOrVolume. */
    typedef QSharedPointer<const MneSurfaceOrVolume> ConstSPtr;   /**< Const shared pointer type for MneSurfaceOrVolume. */

    /*
     * These are the aliases
     */
    typedef MneSurfaceOrVolume MneCSourceSpace; //TODO create a derived class
    //typedef mneSurfaceOrVolume mneSourceVolume;
    typedef MneSurfaceOrVolume MneCSurface; //TODO create a derived class
    //typedef mneSurfaceOrVolume mneVolume;


    //=========================================================================================================
    /**
    * Constructs the MNE Surface or Volume
    */
    MneSurfaceOrVolume();

    //=========================================================================================================
    /**
    * Destroys the MNE Surface or Volume description
    * Refactored: mne_free_source_space (mne_source_space.c)
    */
    ~MneSurfaceOrVolume();




    //============================= make_filter_source_sapces.c =============================

    static double solid_angle (float       *from,	/* From this point... */
                               mneTriangle tri);

    static double sum_solids(float *from, MneSurfaceOrVolume::MneCSurface* surf);


    static int mne_filter_source_spaces(MneSurfaceOrVolume::MneCSurface* surf,  /* The bounding surface must be provided */
                                        float limit,                                   /* Minimum allowed distance from the surface */
                                        INVERSELIB::FiffCoordTransOld* mri_head_t,     /* Coordinate transformation (may not be needed) */
                                        MneSurfaceOrVolume::MneCSourceSpace* *spaces,  /* The source spaces  */
                                        int nspace,
                                        FILE *filtered);




    //============================= make_volume_source_space.c =============================



    static MneSurfaceOrVolume::MneCSourceSpace* make_volume_source_space(MneSurfaceOrVolume::MneCSurface* surf,
                                                                         float grid,
                                                                         float exclude,
                                                                         float mindist);


    //============================= mne_source_space.c =============================
    static MneCSourceSpace* mne_new_source_space(int np);




    //============================= dipole_fit_guesses.c =============================

    static MneCSurface* make_guesses(MneSurfaceOrVolume::MneCSurface* guess_surf, /* Predefined boundary for the guesses */
                                     float guessrad,     /* Radius for the spherical boundary if the above is missing */
                                     float *guess_r0,    /* Origin for the spherical boundary */
                                     float grid,         /* Spacing between guess points */
                                     float exclude,      /* Exclude points closer than this to the CM of the guess boundary surface */
                                     float mindist);




    //============================= mne_bem_surface_io.c =============================

    //Refactored: mne_read_bem_surface (mne_bem_surface_io.c)
    static MneCSurface* read_bem_surface(const QString&  name,   /* Filename */
                                         int  which,             /* Which surface are we looking for (-1 loads the first one)*/
                                         int  add_geometry,      /* Add the geometry information */
                                         float *sigmap);


    //Refactored: read_bem_surface (mne_bem_surface_io.c)
    static MneCSurface* read_bem_surface( const QString& name,    /* Filename */
                                          int  which,             /* Which surface are we looking for (-1 loads the first one)*/
                                          int  add_geometry,      /* Add the geometry information */
                                          float *sigmap,          /* Conductivity? */
                                          bool   check_too_many_neighbors);



    //============================= mne_project_to_surface.c =============================

    typedef struct {
        float *a;
        float *b;
        float *c;
        int   *act;
        int   nactive;
    } *projData,projDataRec;


    static void mne_triangle_coords(float       *r,       /* Location of a point */
                                    MneSurfaceOrVolume::MneCSurface*  s,	       /* The surface */
                                    int         tri,      /* Which triangle */
                                    float       *x,       /* Coordinates of the point on the triangle */
                                    float       *y,
                                    float       *z);




    static int nearest_triangle_point(float       *r,    /* Location of a point */
                                      MneSurfaceOrVolume::MneCSurface*  s,     /* The surface */
                                      void        *user, /* Something precomputed */
                                      int         tri,   /* Which triangle */
                                      float       *x,    /* Coordinates of the point on the triangle */
                                      float       *y,
                                      float       *z);

    static void project_to_triangle(MneSurfaceOrVolume::MneCSurface* s,
                                    int        tri,
                                    float      p,
                                    float      q,
                                    float      *r);

    static int mne_nearest_triangle_point(float       *r,    /* Location of a point */
                                          MneSurfaceOrVolume::MneCSurface*  s,     /* The surface */
                                          int         tri,   /* Which triangle */
                                          float       *x,    /* Coordinates of the point on the triangle */
                                          float       *y,
                                          float       *z);

    static int mne_project_to_surface(MneSurfaceOrVolume::MneCSurface* s, void *proj_data, float *r, int project_it, float *distp);


    //============================= mne_source_space.c =============================

    static int mne_read_source_spaces(const QString& name,               /* Read from here */
                                      MneSurfaceOrVolume::MneCSourceSpace* **spacesp, /* These are the results */
                                      int            *nspacep);


    static void mne_source_space_update_inuse(MneSurfaceOrVolume::MneCSourceSpace* s,
                                              int *new_inuse);


    static int mne_is_left_hemi_source_space(MneSurfaceOrVolume::MneCSourceSpace* s);







    //============================= mne_forward_solution.c =============================

    static void enable_all_sources(MneSurfaceOrVolume::MneCSourceSpace* s);


    //============================= resrict_sources.c =============================

    static int restrict_sources_to_labels(MneSurfaceOrVolume::MneCSourceSpace* *spaces,
                                          int            nspace,
                                          const QStringList& labels,
                                          int            nlabel);


    //============================= mne_labels.c =============================
    //TODO Move to separate label class
    static int mne_find_sources_in_label(char *label,          /* The label file */
                                  MneSurfaceOrVolume::MneCSourceSpace* s,	    /* The corresponding source space hemisphere */
                                  int  off,		    /* Offset to the complete source space */
                                  int  **selp,	    /* Sources selected */
                                  int  *nselp);

    static int mne_read_label(const QString& label,	    /* The label file */
                       char **commentp, /* The comment in the file */
                       int  **selp,	    /* Points in label */
                       int  *nselp);

    static int mne_write_label(char *label,    /* The label file */
                        char *comment,
                        int  *sel,	    /* Points in label */
                        int  nsel,	    /* How many? */
                        float **rr);

    static int mne_label_area(char *label,      /* The label file */
                       MneSurfaceOrVolume::MneCSourceSpace* s, /* The associated source space */
                       float *areap);


public:
    int              type;          /* Is this a volume or a surface */
    char             *subject;      /* Name (id) of the subject */
    int              id;            /* Surface id */
    int              coord_frame;   /* Which coordinate system are the data in now */
    /*
    * These relate to the FreeSurfer way
    */
    mneVolGeom       vol_geom;      /* MRI volume geometry information as FreeSurfer likes it */
    void             *mgh_tags;     /* Tags listed in the file */
    /*
    * These are meaningful for both surfaces and volumes
    */
    int              np;        /* Number of vertices */
    float            **rr;      /* The vertex locations */
    float            **nn;      /* Surface normals at these points */
    float            cm[3];     /* Center of mass */

    int              *inuse;    /* Is this point in use in the source space */
    int              *vertno;   /* Vertex numbers of the used vertices in the full source space */
    int              nuse;      /* Number of points in use */

    int              **neighbor_vert; /* Vertices neighboring each vertex */
    int              *nneighbor_vert; /* Number of vertices neighboring each vertex */
    float            **vert_dist;     /* Distances between neigboring vertices */
    /*
    * These are for surfaces only
    */
    int              ntri;      /* Number of triangles */
    mneTriangle      tris;      /* The triangulation information */
    int              **itris;   /* The vertex numbers */
    float            tot_area;  /* Total area of the surface, computed from the triangles */

    int              nuse_tri;      /* The triangulation corresponding to the vertices in use */
    mneTriangle      use_tris;      /* The triangulation information for the vertices in use */
    int              **use_itris;   /* The vertex numbers for the 'use' triangulation */

    int              **neighbor_tri;    /* Neighboring triangles for each vertex Note: number of entries varies for vertex to vertex */
    int              *nneighbor_tri;    /* Number of neighboring triangles for each vertex */

    mneNearest       nearest;   /* Nearest inuse vertex info (number of these is the same as the number vertices) */
    mnePatchInfo     *patches;  /* Patch information (number of these is the same as the number of points in use) */
    int              npatch;    /* How many (should be same as nuse) */

    INVERSELIB::FiffSparseMatrix*  dist;          /* Distances between the (used) vertices (along the surface). */
    float            dist_limit;    /* Distances above this (in volume) have not been calculated. If negative, only used vertices have been considered */

    float            *curv; /* The FreeSurfer curvature values */
    float            *val;  /* Some other values associated with the vertices */
    /*
    * These are for volumes only
    */
    INVERSELIB::FiffCoordTransOld*  voxel_surf_RAS_t;   /* Transform from voxel coordinate to the surface RAS (MRI) coordinates */
    int              vol_dims[3];                       /* Dimensions of the volume grid (width x height x depth) NOTE: This will be present only if the source space is a complete rectangular grid with unused vertices included */
    float            voxel_size[3];                     /* Derived from the above */
    INVERSELIB::FiffSparseMatrix*  interpolator;        /* Matrix to interpolate into an MRI volume */
    char             *MRI_volume;                       /* The name of the file the above interpolator is based on */
    INVERSELIB::FiffCoordTransOld*   MRI_voxel_surf_RAS_t;
    INVERSELIB::FiffCoordTransOld*   MRI_surf_RAS_RAS_t;/* Transform from surface RAS to RAS coordinates in the associated MRI volume */
    int              MRI_vol_dims[3];                   /* Dimensions of the MRI volume (width x height x depth) */
    /*
    * Possibility to add user-defined data
    */
    void             *user_data;        /* Anything else we want */
    mneUserFreeFunc  user_data_free;    /* Function to set the above free */

    // ### OLD STRUCT ###
    //typedef struct {                /* This defines a source space or a surface */
    //    int              type;          /* Is this a volume or a surface */
    //    char             *subject;      /* Name (id) of the subject */
    //    int              id;            /* Surface id */
    //    int              coord_frame;   /* Which coordinate system are the data in now */
    //    /*
    //    * These relate to the FreeSurfer way
    //    */
    //    mneVolGeom       vol_geom;      /* MRI volume geometry information as FreeSurfer likes it */
    //    void             *mgh_tags;     /* Tags listed in the file */
    //    /*
    //    * These are meaningful for both surfaces and volumes
    //    */
    //    int              np;        /* Number of vertices */
    //    float            **rr;      /* The vertex locations */
    //    float            **nn;      /* Surface normals at these points */
    //    float            cm[3];     /* Center of mass */

    //    int              *inuse;    /* Is this point in use in the source space */
    //    int              *vertno;   /* Vertex numbers of the used vertices in the full source space */
    //    int              nuse;      /* Number of points in use */

    //    int              **neighbor_vert; /* Vertices neighboring each vertex */
    //    int              *nneighbor_vert; /* Number of vertices neighboring each vertex */
    //    float            **vert_dist;     /* Distances between neigboring vertices */
    //    /*
    //    * These are for surfaces only
    //    */
    //    int              ntri;      /* Number of triangles */
    //    mneTriangle      tris;      /* The triangulation information */
    //    int              **itris;   /* The vertex numbers */
    //    float            tot_area;  /* Total area of the surface, computed from the triangles */

    //    int              nuse_tri;      /* The triangulation corresponding to the vertices in use */
    //    mneTriangle      use_tris;      /* The triangulation information for the vertices in use */
    //    int              **use_itris;   /* The vertex numbers for the 'use' triangulation */

    //    int              **neighbor_tri;    /* Neighboring triangles for each vertex Note: number of entries varies for vertex to vertex */
    //    int              *nneighbor_tri;    /* Number of neighboring triangles for each vertex */

    //    mneNearest       nearest;   /* Nearest inuse vertex info (number of these is the same as the number vertices) */
    //    mnePatchInfo     *patches;  /* Patch information (number of these is the same as the number of points in use) */
    //    int              npatch;    /* How many (should be same as nuse) */

    //    mneSparseMatrix  dist;          /* Distances between the (used) vertices (along the surface). */
    //    float            dist_limit;    /* Distances above this (in volume) have not been calculated. If negative, only used vertices have been considered */

    //    float            *curv; /* The FreeSurfer curvature values */
    //    float            *val;  /* Some other values associated with the vertices */
    //    /*
    //    * These are for volumes only
    //    */
    //    INVERSELIB::FiffCoordTransOld*   voxel_surf_RAS_t; /* Transform from voxel coordinate to the surface RAS (MRI) coordinates */
    //    int              vol_dims[3];   /* Dimensions of the volume grid (width x height x depth) NOTE: This will be present only if the source space is a complete rectangular grid with unused vertices included */
    //    float            voxel_size[3]; /* Derived from the above */
    //    mneSparseMatrix  interpolator;  /* Matrix to interpolate into an MRI volume */
    //    char             *MRI_volume;   /* The name of the file the above interpolator is based on */
    //    INVERSELIB::FiffCoordTransOld*   MRI_voxel_surf_RAS_t;
    //    INVERSELIB::FiffCoordTransOld*   MRI_surf_RAS_RAS_t;   /* Transform from surface RAS to RAS coordinates in the associated MRI volume */
    //    int              MRI_vol_dims[3];               /* Dimensions of the MRI volume (width x height x depth) */
    //    /*
    //    * Possibility to add user-defined data
    //    */
    //    void             *user_data;        /* Anything else we want */
    //    mneUserFreeFunc  user_data_free;    /* Function to set the above free */
    //} *mneSurfaceOrVolume,mneSurfaceOrVolumeRec;
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE INVERSELIB

#endif // MNESURFACEORVOLUME_H
