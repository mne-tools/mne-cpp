#ifndef _mne_types_h
#define _mne_types_h
/*
 *    Type definitions for the MNE library
 *
 *    Copyright 2001 - 2009
 *
 *    Matti Hamalainen
 *    Athinoula A. Martinos Center for Biomedical Imaging
 *    Massachusetts General Hospital
 *    Charlestown, MA, USA
 *
 *    $Id: mne_types.h 3477 2016-01-14 12:48:00Z msh $
 *
 *    Revision 1.76  2009/03/14 12:37:30  msh
 *    Quite a bit of work for C++ compatibility
 *
 *    Revision 1.75  2009/02/28 17:53:50  msh
 *    Added extra fields to mneWdata and mneStcData with future extensions in mind
 *
 *    Revision 1.74  2009/02/18 02:45:40  msh
 *    Moved to the mneEventList structure, which contains comments per event
 *
 *    Revision 1.73  2009/02/05 04:54:14  msh
 *    Added first_sample_val member to mneRawData
 *
 *    Revision 1.72  2009/01/30 15:20:29  msh
 *    Added code to mask the contents of the digital trigger channel if appropriate
 *
 *    Revision 1.71  2009/01/19 13:08:05  msh
 *    Initialize the MRI structure more completely
 *
 *    Revision 1.70  2008/11/16 20:19:59  msh
 *    Added Talairach transforms
 *    Fixed bugs in MRI reading
 *
 *    Revision 1.69  2008/09/30 11:47:01  msh
 *    Added option to load unprocessed MaxShield data
 *
 *    Revision 1.68  2008/08/15 12:56:49  msh
 *    Added option for discrete source spaces
 *
 *    Revision 1.67  2008/06/16 16:07:09  msh
 *    Added compatibility for the old event file format
 *
 *    Revision 1.66  2008/05/28 10:14:05  msh
 *    Added possibility for a sparse covariance matrix
 *
 *    Revision 1.65  2008/05/26 10:50:10  msh
 *    Changes to accommodate the preweighted lead field basis
 *
 *    Revision 1.64  2008/03/17 18:29:31  msh
 *    Possibility to define the trigger channel with an environment variable
 *    Added mne_compare_filters routine
 *
 *    Revision 1.63  2008/03/15 12:54:46  msh
 *    Handle FIFF_MEAS_DATE appropriately
 *    Copy MNE_PROCESSING_HISTORY to output files as needed
 *    Possibility to add new epochs to raw data epochs previously loaded
 *
 *    Revision 1.62  2008/03/09 01:19:35  msh
 *    Added mne_raw_formatted_time routine
 *    Include first sample to number of omitted samples
 *    Read the FIFF_MEAS_DATE tag
 *
 *    Revision 1.61  2007/04/06 19:07:58  msh
 *    Implemented a new event structure in raw data which will eventually
 *    allow for multiple event lists and comments for the events
 *
 *    Revision 1.60  2007/03/01 04:32:19  msh
 *    Reworked the MRI data structure to be more flexible
 *
 *    Revision 1.59  2007/02/27 17:40:14  msh
 *    Fixed empty row or column handling in mne_create_sparse_rcs and mne_create_sparse_ccs
 *    Fixed memory leak in mne_inverse_io.c
 *    Added new fields to volume source spaces
 *
 *    Revision 1.58  2007/02/26 05:01:29  msh
 *    Added the voxel-RAS coordinate transformation to mneMRIdata
 *    Added routine to set voxels at given RAS coordinate location to given value
 *
 *    Revision 1.57  2007/02/25 14:29:44  msh
 *    Added routines to read mgh and mgz MRI data files.
 *
 *    Revision 1.56  2007/02/23 14:24:47  msh
 *    Made more provisions for volume source spaces.
 *
 *    Revision 1.55  2007/02/20 20:40:40  msh
 *    Match channel names without spaces as well for compatibility with
 *    the new Neuromag channel naming scheme.
 *
 *    Revision 1.54  2007/01/29 21:02:44  msh
 *    Added the additional prior weightings to the inverse operator structure
 *
 *    Revision 1.53  2007/01/09 12:43:08  msh
 *    Added total area member to the mneSourceSpace structure
 *    Compute the total area in mne_add_triangle_data
 *
 *    Revision 1.52  2006/12/08 16:36:36  msh
 *    Added source file name to layout.
 *
 *    Revision 1.51  2006/12/05 21:27:55  msh
 *    Added the SSS stuff to raw data and the covariance matrix
 *
 *    Revision 1.50  2006/11/16 22:46:13  msh
 *    Changed sample number in mneTrigEvent to be a signed integer.
 *
 *    Revision 1.49  2006/10/16 13:39:17  msh
 *    Changed field names in mneMeasDataSet structure
 *
 */
#include <fiff/c/fiff_types.h>

#if defined(__cplusplus) 
extern "C" {
#endif

/*
 * Complex data
 */
typedef struct {
  float re;
  float im;
} *mneComplex,**mneComplexMatrix,mneComplexRec;

typedef struct {
  double re;
  double im;
} *mneDoubleComplex,**mneDoubleComplexMatrix,mneDoubleComplexRec;

typedef void (*mneUserFreeFunc)(void *);  /* General purpose */
typedef fiffSparseMatrix mneSparseMatrix;
typedef fiffSparseMatrixRec mneSparseMatrixRec;

typedef struct {
  int   vert;			/* Which vertex does this apply to */
  int   *memb_vert;		/* Which vertices constitute the patch? */
  int   nmemb;			/* How many? */
  float area;			/* Area of the patch */
  float ave_nn[3];		/* Average normal */
  float dev_nn;			/* Average deviation of the patch normals from the average normal */
} *mnePatchInfo,mnePatchInfoRec;

typedef struct {		/* This is used in the patch definitions */
  int   vert;			/* Number of this vertex (to enable sorting) */
  int   nearest;		/* Nearest 'inuse' vertex */
  float dist;	                /* Distance to the nearest 'inuse' vertex */
  mnePatchInfo patch;           /* The patch information record for the patch this vertex belongs to */
} *mneNearest,mneNearestRec;

typedef struct {
  int   *vert;           	/* Triangle vertices (pointers to the itris member of the associated mneSurface) */
  float *r1,*r2,*r3;           	/* Triangle vertex locations (pointers to the rr member of the associated mneSurface) */
  float r12[3],r13[3];		/* Vectors along the sides */
  float nn[3];			/* Normal vector */
  float area;			/* Area */
  float cent[3];		/* Centroid */
  float ex[3],ey[3];		/* Other unit vectors (used by BEM calculations) */
} *mneTriangle,mneTriangleRec;	/* Triangle data */

typedef struct {
  int            valid;		             /* Is the information below valid */
  int            width,height,depth;	     /* Size of the stack */
  float          xsize,ysize,zsize;	     /* Increments in the three voxel directions */
  float          x_ras[3],y_ras[3],z_ras[3]; /* Directions of the coordinate axes */
  float          c_ras[3];                   /* Center of the RAS coordinates */
  char           *filename;		     /* Name of the MRI data file */
} *mneVolGeom,mneVolGeomRec;		     /* MRI data volume geometry information like FreeSurfer keeps it */
  
/*
 * Values of the type field in the mneVolumeOrSurface structure
 */
#define MNE_SOURCE_SPACE_UNKNOWN -1
#define MNE_SOURCE_SPACE_SURFACE  1 /* Surface-based source space */
#define MNE_SOURCE_SPACE_VOLUME   2 /* 3D volume source space */
#define MNE_SOURCE_SPACE_DISCRETE 3 /* Discrete points */

typedef struct {		    /* This defines a source space or a surface */
  int              type;	    /* Is this a volume or a surface */
  char             *subject;	    /* Name (id) of the subject */
  int              id;		    /* Surface id */
  int              coord_frame;     /* Which coordinate system are the data in now */
  /*
   * These relate to the FreeSurfer way
   */
  mneVolGeom       vol_geom;	    /* MRI volume geometry information as FreeSurfer likes it */
  void             *mgh_tags;	    /* Tags listed in the file */
  /*
   * These are meaningful for both surfaces and volumes
   */
  int              np;		    /* Number of vertices */
  float            **rr;	    /* The vertex locations */
  float            **nn;	    /* Surface normals at these points */
  float            cm[3];	    /* Center of mass */

  int              *inuse;	    /* Is this point in use in the source space */
  int              *vertno;	    /* Vertex numbers of the used vertices in the full source space */
  int              nuse;	    /* Number of points in use */

  int              **neighbor_vert; /* Vertices neighboring each vertex */
  int              *nneighbor_vert; /* Number of vertices neighboring each vertex */
  float            **vert_dist;     /* Distances between neigboring vertices */
  /*
   * These are for surfaces only
   */
  int              ntri;	    /* Number of triangles */
  mneTriangle      tris;	    /* The triangulation information */
  int              **itris;	    /* The vertex numbers */
  float            tot_area;	    /* Total area of the surface, computed from the triangles */

  int              nuse_tri;	    /* The triangulation corresponding to the vertices in use */
  mneTriangle      use_tris;	    /* The triangulation information for the vertices in use */
  int              **use_itris;	    /* The vertex numbers for the 'use' triangulation */

  int              **neighbor_tri;  /* Neighboring triangles for each vertex 
				     * Note: number of entries varies for vertex to vertex */
  int              *nneighbor_tri;  /* Number of neighboring triangles for each vertex */

  mneNearest       nearest;	    /* Nearest inuse vertex info (number of these is the same as the number vertices) */
  mnePatchInfo     *patches;        /* Patch information (number of these is the same as the number of points in use) */
  int              npatch;	    /* How many (should be same as nuse) */

  mneSparseMatrix  dist;	    /* Distances between the (used) vertices (along the surface). */
  float            dist_limit;	    /* Distances above this (in volume) have not been calculated. 
				     * If negative, only used vertices have been considered */

  float            *curv;	    /* The FreeSurfer curvature values */
  float            *val;	    /* Some other values associated with the vertices */
  /*
   * These are for volumes only
   */
  fiffCoordTrans   voxel_surf_RAS_t;/* Transform from voxel coordinate to the surface RAS (MRI) coordinates */
  int              vol_dims[3];     /* Dimensions of the volume grid (width x height x depth) 
				     * NOTE: This will be present only if the source space is a complete 
				     * rectangular grid with unused vertices included */
  float            voxel_size[3];   /* Derived from the above */
  mneSparseMatrix  interpolator;    /* Matrix to interpolate into an MRI volume */
  char             *MRI_volume;     /* The name of the file the above interpolator is based on */
  fiffCoordTrans   MRI_voxel_surf_RAS_t;
  fiffCoordTrans   MRI_surf_RAS_RAS_t;  /* Transform from surface RAS to RAS coordinates in the associated MRI volume */
  int              MRI_vol_dims[3];     /* Dimensions of the MRI volume (width x height x depth) */
  /*
   * Possibility to add user-defined data
   */
  void             *user_data;      /* Anything else we want */
  mneUserFreeFunc  user_data_free;  /* Function to set the above free */
} *mneSurfaceOrVolume,mneSurfaceOrVolumeRec;
/*
 * These are the aliases
 */
typedef mneSurfaceOrVolume mneSourceSpace;
typedef mneSurfaceOrVolume mneSourceVolume;
typedef mneSurfaceOrVolume mneSurface;
typedef mneSurfaceOrVolume mneVolume;

typedef mneSurfaceOrVolumeRec mneSourceSpaceRec;
typedef mneSurfaceOrVolumeRec mneSourceVolumeRecRec;
typedef mneSurfaceOrVolumeRec mneSurfaceRec;
typedef mneSurfaceOrVolumeRec mneVolumeRec;

//typedef struct {		    /* FreeSurfer patches */
//  mneSurface       s;		    /* Patch represented as a surface */
//  int              *vert;	    /* Vertex numbers in the complete surface*/
//  int              *surf_vert;	    /* Which vertex corresponds to each complete surface vertex here? */
//  int              np_surf;	    /* How many points on the complete surface? */
//  int              *tri;	    /* Which triangles in the complete surface correspond to our triangles? */
//  int              *surf_tri;	    /* Which of our triangles corresponds to each triangle on the complete surface? */
//  int              ntri_surf;	    /* How many triangles on the complete surface */
//  int              *border;	    /* Is this vertex on the border? */
//  int              flat;	    /* Is this a flat patch? */
//  void             *user_data;      /* Anything else we want */
//  mneUserFreeFunc  user_data_free;  /* Function to set the above free */
//} *mneSurfacePatch,mneSurfacePatchRec;

typedef struct {		/* Matrix specification with a channel list */
  int   nrow;			/* Number of rows */
  int   ncol;			/* Number of columns */
  char  **rowlist;		/* Name list for the rows (may be NULL) */
  char  **collist;		/* Name list for the columns (may be NULL) */
  float **data;                  /* The data itself (dense) */
} *mneNamedMatrix,mneNamedMatrixRec;

typedef struct {		/* Matrix specification with a channel list */
  int   nrow;			/* Number of rows (same as in data) */
  int   ncol;			/* Number of columns (same as in data) */
  char  **rowlist;		/* Name list for the rows (may be NULL) */
  char  **collist;		/* Name list for the columns (may be NULL) */
  mneSparseMatrix data;		/* The data itself (sparse) */
} *mneSparseNamedMatrix,mneSparseNamedMatrixRec;

typedef struct {		/* Vector specification with a channel list */
  int    nvec;			/* Number of elements */
  char   **names;		/* Name list for the elements */
  float  *data;			/* The data itself */
} *mneNamedVector,mneNamedVectorRec;

typedef struct {		/* One linear projection item */
  mneNamedMatrix vecs;          /* The original projection vectors */
  int            nvec;          /* Number of vectors = vecs->nrow */
  char           *desc;	        /* Projection item description */
  int            kind;          /* Projection item kind */
  int            active;	/* Is this item active now? */
  int            active_file;	/* Was this item active when loaded from file? */
  int            has_meg;	/* Does it have MEG channels? */
  int            has_eeg;	/* Does it have EEG channels? */
} *mneProjItem,mneProjItemRec;

typedef struct {		/* Collection of projection items and the projector itself */
  mneProjItem    *items;        /* The projection items */
  int            nitems;        /* Number of items */
  char           **names;	/* Names of the channels in the final projector */
  int            nch;	        /* Number of channels in the final projector */
  int            nvec;          /* Number of vectors in the final projector */
  float          **proj_data;	/* The orthogonalized projection vectors picked and orthogonalized from the original data */
} *mneProjOp,mneProjOpRec;

typedef struct {
  int   job;			/* Value of FIFF_SSS_JOB tag */
  int   coord_frame;		/* Coordinate frame */
  float origin[3];		/* The expansion origin */
  int   nchan;			/* How many channels */
  int   out_order;		/* Order of the outside expansion */
  int   in_order;		/* Order of the inside expansion */
  int   *comp_info;		/* Which components are included */
  int   ncomp;			/* How many entries in the above */
  int   in_nuse;		/* How many components included in the inside expansion */
  int   out_nuse;		/* How many components included in the outside expansion */
} *mneSssData,mneSssDataRec;	/* Essential information about SSS */

/*
 * The class field in mneCovMatrix can have these values
 */
#define MNE_COV_CH_UNKNOWN  -1	/* No idea */
#define MNE_COV_CH_MEG_MAG   0  /* Axial gradiometer or magnetometer [T] */
#define MNE_COV_CH_MEG_GRAD  1  /* Planar gradiometer [T/m] */
#define MNE_COV_CH_EEG       2  /* EEG [V] */

//typedef struct {		/* Covariance matrix storage */
//  int        kind;		/* Sensor or source covariance */
//  int        ncov;		/* Dimension */
//  int        nfree;		/* Number of degrees of freedom */
//  int        nproj;		/* Number of dimensions projected out */
//  int        nzero;		/* Number of zero or small eigenvalues */
//  char       **names;		/* Names of the entries (optional) */
//  double     *cov;		/* Covariance matrix in packed representation (lower triangle) */
//  double     *cov_diag;		/* Diagonal covariance matrix */
//  mneSparseMatrix cov_sparse;   /* A sparse covariance matrix
//				 * (Note: data are floats in this which is an inconsistency) */
//  double     *lambda;		/* Eigenvalues of cov */
//  double     *inv_lambda;	/* Inverses of the square roots of the eigenvalues of cov */
//  float      **eigen;		/* Eigenvectors of cov */
//  double     *chol;		/* Cholesky decomposition */
//  mneProjOp  proj;		/* The projection which was active when this matrix was computed */
//  mneSssData sss;		/* The SSS data present in the associated raw data file */
//  int        *ch_class;		/* This will allow grouping of channels for regularization (MEG [T/m], MEG [T], EEG [V] */
//  char       **bads;		/* Which channels were designated bad when this noise covariance matrix was computed? */
//  int        nbad;		/* How many of them */
//} *mneCovMatrix,mneCovMatrixRec;

//typedef struct {		/* A forward solution */
//  char           *fwdname;	/* Name of the file this was loaded from */
//  fiffId         meas_id;       /* The assosiated measurement ID */
//  mneSourceSpace *spaces;	/* The source spaces */
//  int            nspace;	/* Number of source spaces */
//  fiffCoordTrans mri_head_t;	/* MRI <-> head coordinate transformation */
//  fiffCoordTrans meg_head_t;    /* MEG <-> head coordinate transformation */
//  int            methods;	/* EEG, MEG or EEG+MEG (see mne_fiff.h) */
//  int            coord_frame;	/* The coordinate frame employed in the forward calculations */
//  int            source_ori;	/* Fixed or free source orientations */
//  float          **rr_source;	/* The active source points */
//  float          **nn_source;	/* The source orientations
//				 * (These are equal to the cortex normals
//				 * in the fixed orientation case) */
//  int            nsource;	/* Number of source (recalculated for convenience) */
//  fiffChInfo     chs;		/* The channel list */
//  int            nch;		/* Number of channels */
//  mneNamedMatrix fwd;	        /* The forward solution (may be whitened) */
//  mneNamedMatrix fwd_proc;	/* This is an alternate matrix for a processed forward matrix (linear projection
//				 * and whitening) As a rule, this field is not used but rater the operations are
//				 * applied to the field fwd itself */
//  float          *patch_areas;  /* Contains the patch areas if the CSD transformation has been applied */
//  int            fwd_whitened;	/* Has the noise covariance been applied to the field fwd? */
//  mneCovMatrix   noise_cov;	/* The noise covariance matrix employed in whitening */
//  mneProjOp      proj;		/* Associated projection operator */
//} *mneForwardSolution,mneForwardSolutionRec;

//typedef struct {		          /* An inverse operator */
//  fiffId         meas_id;                 /* The assosiated measurement ID */
//  mneSourceSpace *spaces;	          /* The source spaces */
//  int            nspace;	          /* Number of source spaces */
//  fiffCoordTrans meg_head_t;              /* MEG device <-> head coordinate transformation */
//  fiffCoordTrans mri_head_t;	          /* MRI device <-> head coordinate transformation */
//  int            methods;	          /* EEG, MEG or EEG+MEG (see mne_fiff.h) */
//  int            nchan;		          /* Number of measurement channels */
//  int            nsource;	          /* Number of source points */
//  int            fixed_ori;	          /* Fixed source orientations? */
//  float          **rr_source;	          /* The active source points */
//  float          **nn_source;	          /* The source orientations
//					   * (These are equal to the cortex normals
//					   * in the fixed orientation case) */
//  int            coord_frame;             /* Which coordinates are the locations and orientations given in? */
//  mneCovMatrix   sensor_cov;	          /* Sensor covariance matrix */
//  int            nave;		          /* Number of averaged responses (affects scaling of the noise covariance) */
//  int            current_unit;		  /* This can be FIFF_UNIT_AM, FIFF_UNIT_AM_M2, FIFF_UNIT_AM_M3 */
//  mneCovMatrix   source_cov;	          /* Source covariance matrix */
//  mneCovMatrix   orient_prior;	          /* Orientation prior applied */
//  mneCovMatrix   depth_prior;	          /* Depth-weighting prior applied */
//  mneCovMatrix   fMRI_prior;	          /* fMRI prior applied */
//  float          *sing;		          /* Singular values of the inverse operator */
//  mneNamedMatrix eigen_leads;	          /* The eigen leadfields */
//  int            eigen_leads_weighted;    /* Have the above been already weighted with R^0.5? */
//  mneNamedMatrix eigen_fields;	          /* Associated field patterns */
//  float          trace_ratio;	          /* tr(GRG^T)/tr(C) */
//  mneProjOp      proj;		          /* The associated projection operator */
//} *mneInverseOperator,mneInverseOperatorRec;

typedef struct {		/* For storing the wdata */
  int   id;			/* Surface id these data belong to */
  int   kind;			/* What kind of data */
  float lat;			/* Latency */
  int   nvert;			/* Number of vertices */
  int   *vertno;		/* Vertex numbers */
  float *vals;			/* The values */
} *mneWdata,mneWdataRec;

typedef struct {
  int   id;			/* Surface id these data belong to */
  int   kind;			/* What kind of data */
  float tmin;			/* First time */
  float tstep;			/* Step between times */
  int   ntime;			/* Number of times */
  int   nvert;			/* Number of vertices */
  int   *vertno;		/* The vertex numbers */
  float **data;			/* The data, time by time */
} *mneStcData,mneStcDataRec;

//typedef struct {		/* Information about raw data in fiff file */
//  char          *filename;	/* The name of the file this comes from */
//  fiffId        id;		/* Measurement id from the file */
//  int           nchan;		/* Number of channels */
//  fiffChInfo    chInfo;		/* Channel info data  */
//  int           coord_frame;	/*
//				 * Which coordinate frame are the
//				 * positions defined in?
//				 */
//  fiffCoordTrans trans;	        /* This is the coordinate transformation
//				 * FIFF_COORD_HEAD <--> FIFF_COORD_DEVICE
//				 */
//  float         sfreq;		/* Sampling frequency */
//  float         lowpass;	/* Lowpass filter setting */
//  float         highpass;       /* Highpass filter setting */
//  fiffTimeRec   start_time;	/* Starting time of the acquisition
//				 * taken from the meas date
//				 * or the meas block id
//				 * whence it may be inaccurate. */
//  int           buf_size;	/* Buffer size in samples */
//  int           maxshield_data; /* Are these unprocessed MaxShield data */
//  fiffDirEntry  rawDir;		/* Directory of raw data tags
//				 * These may be of type
//				 *       FIFF_DATA_BUFFER
//				 *       FIFF_DATA_SKIP
//				 *       FIFF_DATA_SKIP_SAMP
//				 *       FIFF_NOP
//				 */
//  int           ndir;		/* Number of tags in the above
//				 * directory */
//} mneRawInfoRec, *mneRawInfo;

typedef struct {		/* Spatiotemporal map */
  int kind;			/* What kind of data */
  int coord_frame;		/* Coordinate frame for vector values */
  int nvert;			/* Number of vertex values */
  int ntime;			/* Number of time points */
  int nval_vert;		/* Number of values per vertex */
  float tmin;			/* First time point */
  float tstep;			/* Step between the time points */
  int   *vertno;		/* Vertex numbers in the full triangulation (starting with zero) */
  float **data;			/* The data values (time by time) */
} mneMapRec, *mneMap;

typedef struct {				     /* Plotter layout port definition */
  int   portno;					     /* Running number of this viewport */
  int   invert;					     /* Invert the signal coming to this port */
  float xmin,xmax,ymin,ymax;			     /* Limits */
  char  *names;					     /* Channels to go into this port (one line, separated by colons) */
  int   match;					     /* Does this port match with our present channel? */
} mneLayoutPortRec,*mneLayoutPort;

typedef struct {				     /* Plotter layout */
  char          *name;				     /* File where this came from */
  float         xmin,xmax,ymin,ymax;		     /* The VDC limits */
  float         cxmin,cxmax,cymin,cymax;	     /* The confined VDC limits */
  int           nport;				     /* Number of viewports */
  mneLayoutPort ports;				     /* Array of viewports */
  int           **match;			     /* Matching matrix */
  int           nmatch;				     /* How many channels in matching matrix */
} mneLayoutRec,*mneLayout;

/*
 * The following relate to high-level handling of raw data
 */
#define MNE_CH_SELECTION_UNKNOWN 0
#define MNE_CH_SELECTION_FILE    1
#define MNE_CH_SELECTION_USER    2

typedef struct {
  char *name;			/* Name of this selection */
  char **chdef;			/* Channel definitions (may contain regular expressions) */
  int  ndef;			/* How many of them */
  char **chspick;		/* Translated into channel names using the present data */
  char **chspick_nospace;	/* The same without spaces */
  int  *pick;			/* These are the corresponding channels in the raw data 
				   (< 0 indicates missing) */
  int  *pick_deriv;		/* These are the corresponding derivations in the raw data */
  int  nderiv;			/* How many derivations in the above */
  int  *ch_kind;		/* Kinds of the channels corresponding to picks (< 0 indicates missing) */
  int  nchan;			/* How many picked channels? */
  int  kind;			/* Loaded from file or created here? */
} *mneChSelection,mneChSelectionRec;

typedef struct {
  char        *name;		/* Name of this set */
  mneChSelection *sels;		/* These are the selections */
  int         nsel;		/* How many */
  int         current;		/* Which is active now? */
} *mneChSelectionSet, mneChSelectionSetRec;

typedef struct {
  unsigned int from;		/* Source transition value */
  unsigned int to;		/* Destination transition value */
  int          sample;		/* Sample number */
  int          show;		/* Can be used as desired */
  int          created_here;	/* Was this event created in the program */
  char         *comment;	/* Event comment */
} *mneEvent,mneEventRec;

typedef struct {		/* List of the above. */
  mneEvent     *events;
  int          nevent;
} *mneEventList,mneEventListRec;

typedef struct {
  int   filter_on;		/* Is it on? */
  int   size;			/* Length in samples (must be a power of 2) */
  int   taper_size;		/* How long a taper in the beginning and end */
  float highpass;		/* Highpass in Hz */
  float highpass_width;		/* Highpass transition width in Hz */
  float lowpass;		/* Lowpass in Hz */
  float lowpass_width;		/* Lowpass transition width in Hz */
  float eog_highpass;		/* EOG highpass in Hz */
  float eog_highpass_width;	/* EOG highpass transition width in Hz */
  float eog_lowpass;		/* EOG lowpass in Hz */
  float eog_lowpass_width;	/* EOG lowpass transition width in Hz */
} *mneFilterDef,mneFilterDefRec;

//typedef struct {
//  fiffDirEntry ent;		/* Where is this in the file (file bufs only, pointer to info) */
//  int   firsts,lasts;		/* First and last sample */
//  int   ntaper;			/* For filtered buffers: taper length */
//  int   ns;			/* Number of samples (last - first + 1) */
//  int   nchan;			/* Number of channels */
//  int   is_skip;		/* Is this a skip? */
//  float **vals;			/* Values (null if not in memory) */
//  int   valid;			/* Are the data meaningful? */
//  int   *ch_filtered;		/* For filtered buffers: has this channel filtered already */
//  int   comp_status;		/* For raw buffers: compensation status */
//} *mneRawBufDef,mneRawBufDefRec;

/*
 * CTF compensation stuff
 */
#define MNE_CTFV_NOGRAD           0
#define MNE_CTFV_GRAD1            1
#define MNE_CTFV_GRAD2            2
#define MNE_CTFV_GRAD3            3
/*
 * 4D compensation stuff
 */
#define MNE_4DV_COMP1           101

typedef struct {
  int             kind;		     /* The compensation kind (CTF) */
  int             mne_kind;	     /* Our kind */
  int             calibrated;	     /* Are the coefficients in the file calibrated already? */
  mneNamedMatrix  data;	             /* The compensation data */
  mneSparseMatrix presel;            /* Apply this selector prior to compensation */
  mneSparseMatrix postsel;	     /* Apply this selector after compensation */
  float           *presel_data;	     /* These are used for the intermediate results in the calculations */
  float           *comp_data;
  float           *postsel_data;
} *mneCTFcompData,mneCTFcompDataRec;

typedef struct {
  mneCTFcompData *comps;	/* All available compensation data sets */
  int            ncomp;		/* How many? */
  fiffChInfo     chs;		/* Channel information */
  int            nch;		/* How many of the above */
  mneCTFcompData undo;		/* Compensation data to undo the current compensation before applying current */
  mneCTFcompData current;	/* The current compensation data composed from the above 
				 * taking into account channels presently available */
} *mneCTFcompDataSet,mneCTFcompDataSetRec;

typedef struct {		        /* One item in a derivation data set */
  char                 *filename;       /* Source file name */
  char                 *shortname;      /* Short nickname for this derivation */
  mneSparseNamedMatrix deriv_data;	/* The derivation data itself */
  int                  *in_use;		/* How many non-zero elements on each column of the derivation data 
					 * (This field is not always used) */
  int                  *valid;		/* Which of the derivations are valid considering the units of the input 
					 * channels (This field is not always used) */
  fiffChInfo           chs;		/* First matching channel info in each derivation */
} *mneDeriv,mneDerivRec;

typedef struct {		        /* A collection of derivations */
  int      nderiv;		        /* How many? */
  mneDeriv *derivs;			/* Pointers to the items */
} *mneDerivSet,mneDerivSetRec;

typedef struct {			/* A comprehensive raw data structure */
  char             *filename;           /* This is our file */
  fiffFile         file;	        
  mneRawInfo       info;	        /* Loaded using the mne routines */
  char             **ch_names;		/* Useful to have the channel names as a single list */
  char             **badlist;		/* Bad channel names */
  int              nbad;		/* How many? */
  int              *bad;		/* Which channels are bad? */
  mneRawBufDef     bufs;		/* These are the data */
  int              nbuf;		/* How many? */
  mneRawBufDef     filt_bufs;	        /* These are the filtered ones */
  int              nfilt_buf;
  int              first_samp;          /* First sample? */
  int              omit_samp;		/* How many samples of skip omitted in the beginning */
  int              omit_samp_old;       /* This is the value omit_samp would have in the old versions */
  int              nsamp;	        /* How many samples in total? */
  float            *first_sample_val;	/* Values at the first sample (for dc offset correction before filtering) */
  mneProjOp        proj;		/* Projection operator */
  mneSssData       sss;			/* SSS data found in this file */
  mneCTFcompDataSet comp;		/* Compensation data */
  int              comp_file;           /* Compensation status of these raw data in file */
  int              comp_now;            /* Compensation status of these raw data in file */
  mneFilterDef     filter;		/* Filter definition */
  void             *filter_data;        /* This can be whatever the filter needs */
  mneUserFreeFunc  filter_data_free;    /* Function to free the above */
  mneEventList     event_list;		/* Trigger events */
  unsigned int     max_event;	        /* Maximum event number in usenest */
  char             *dig_trigger;        /* Name of the digital trigger channel */
  unsigned int     dig_trigger_mask;    /* Mask applied to digital trigger channel before considering it */
  float            *offsets;		/* Dc offset corrections for display */
  void             *ring;	        /* The ringbuffer (structure is of no
					 * interest to us) */
  void             *filt_ring;          /* Separate ring buffer for filtered data */
  mneDerivSet      deriv;	        /* Derivation data */
  mneDeriv         deriv_matched;	/* Derivation data matched to this raw data and 
					 * collected into a single item */
  float            *deriv_offsets;	/* Dc offset corrections for display of the derived channels */
  void             *user;	        /* Whatever */
  mneUserFreeFunc  user_free;		/* How this is freed */
} *mneRawData,mneRawDataRec;		

typedef struct {		 /* Data associated with MNE computations for each mneMeasDataSet */
  float **datap;		 /* Projection of the whitened data onto the field eigenvectors */
  float **predicted;		 /* The predicted data */
  float *SNR;			 /* Estimated power SNR as a function of time */
  float *lambda2_est;		 /* Regularization parameter estimated from available data */
  float *lambda2;                /* Regularization parameter to be used (as a function of time) */      
} *mneMneData,mneMneDataRec;

typedef struct {		 /* One data set, used in mneMeasData */
  /*
   * These are unique to each data set
   */ 
  char            *comment;	 /* Comment associated with these data */
  float           **data;	 /* The measured data */
  float           **data_proj;	 /* Some programs maybe interested in keeping the data after SSP separately */
  float           **data_filt;	 /* Some programs maybe interested in putting a filtered version here */
  float           **data_white;	 /* The whitened data */
  float           *stim14;	 /* Data from the digital stimulus channel */
  int             first;	 /* First sample index for raw data processing */
  int             np;		 /* Number of times */
  int             nave;		 /* Number of averaged responses */
  int             kind;		 /* Which aspect of data */
  float           tmin;		 /* Starting time */
  float           tstep;	 /* Time step */
  float           *baselines;	 /* Baseline values currently applied to the data */
  mneMneData      mne;		 /* These are the data associated with MNE computations */
  void            *user_data;    /* Anything else we want */
  mneUserFreeFunc user_data_free;/* Function to set the above free */
} *mneMeasDataSet,mneMeasDataSetRec;

typedef struct {		 /* Measurement data representation in MNE calculations */
  /*
   * These are common to all data sets
   */
  char               *filename;	 /* The source file name */
  fiffId             meas_id;	 /* The id from the measurement file */
  fiffTimeRec        meas_date;	 /* The measurement date from the file */
  fiffChInfo         chs;	 /* The channel information */
  fiffCoordTrans     meg_head_t; /* MEG device <-> head coordinate transformation */
  fiffCoordTrans     mri_head_t; /* MRI device <-> head coordinate transformation 
				    (duplicated from the inverse operator or loaded separately) */
  float              sfreq;	 /* Sampling frequency */
  int                nchan;	 /* Number of channels */
  float              highpass;	 /* Highpass filter setting */ 
  float              lowpass;	 /* Lowpass filter setting */ 
  mneProjOp          proj;	 /* Associated projection operator (useful if inverse operator is not included) */
  mneCTFcompDataSet  comp;	 /* The software gradient compensation data */
  mneInverseOperator op;	 /* Associated inverse operator */
  mneNamedMatrix     fwd;	 /* Forward operator for dipole fitting */
  mneRawData         raw;	 /* This will be non-null if the data stems from a raw data file */
  mneChSelection     chsel;	 /* Channel selection for raw data */
  char               **badlist;	 /* Bad channel names */
  int                nbad;	 /* How many? */
  int                *bad;	 /* Which channels are bad? */
  /*
   * These are the data sets loaded
   */
  int                ch_major;	 /* Rows are channels rather than times */
  mneMeasDataSet     *sets;	 /* All loaded data sets */
  int                nset;	 /* How many */
  mneMeasDataSet     current;	 /* Which is the current one */
} *mneMeasData,mneMeasDataRec;

typedef struct {		/* Identifier for the automatic parcellation */
  char  *name;			/* Name of this area */
  int   index;			/* Index in the parcellation */
  int   flag;			/* Flag read from  the color table (whatever it means) */
  float r,g,b,alpha;		/* The color values */
  int   *vert;		        /* The vertices belonging to this one */
  int   nvert;
} *mneParc,mneParcRec;

typedef struct {		/* A set of the above */
  int       *vert;		/* The assignment of each vertex */
  int       nvert;		/* How many */
  mneParc   *parcs;		/* The 'parcels' */
  int       nparc;
} *mneParcSet,mneParcSetRec;

typedef struct {
  mneParcSet   lh_parc;
  mneParcSet   rh_parc;
} *mneParcData,mneParcDataRec;	/* This is the complete parcellation data */

/*
 * The following are included for reading and writing MRI data in the mgh and mgz formats
 */
#define MNE_MRI_VERSION       1
#define MNE_MRI_UCHAR         0
#define MNE_MRI_INT           1
#define MNE_MRI_LONG          2
#define MNE_MRI_FLOAT         3
#define MNE_MRI_SHORT         4
#define MNE_MRI_BITMAP        5
#define MNE_MRI_TENSOR        6

#define MNE_MRI_ALL_FRAMES    -1	     /* Load all frames */
#define MNE_MRI_NO_FRAMES     -2	     /* Do not load data at all */

typedef union {
  unsigned char uval;
  short         sval;
  int           ival;
  float         fval;
} mneMRIvoxelVal;

typedef union {
  unsigned char   ***uslices;	             /* Different types of voxels */
  short           ***sslices;
  int             ***islices;
  float           ***fslices;
} mneMRIvoxels;

typedef struct {
  int             type;			     /* Data type duplicated from mneMRIdata */
  int             width;		     /* Size of the stack duplicated from mneMRIdata*/
  int             height;
  int             depth;
  void            *data;		     /* These are the actual voxels stored sequentially */
  int             nbytes;		     /* How many bytes in data (just a convenience) */
  mneMRIvoxels    voxels;		     /* Different types of voxels */
  int             frame;		     /* Which frame is this in the original data */
  void            *user_data;                /* Anything else we want */
  mneUserFreeFunc user_data_free;            /* Function to set the above free */
} *mneMRIvolume,mneMRIvolumeRec;	     /* One volume of data */

typedef struct {
  char           *filename;                  /* Name of the source file */
  int            version;	             /* Version number of the data file */
  int            width;		             /* Size of the stack */
  int            height;
  int            depth;
  int            nframes;		     /* Number of frames in the original */
  int            dof;
  int            ras_good;                   /* Indicates that the values below are good */
  float          xsize,ysize,zsize;	     /* Increments in the three voxel directions */
  float          x_ras[3],y_ras[3],z_ras[3]; /* these are the RAS distances across the whole volume */
  float          c_ras[3];                   /* Center of the RAS coordinates 
					      * This is irrelevant to the MNE software because we work in
					      * surface RAS coordinates */
  fiffCoordTrans voxel_surf_RAS_t;           /* Transform from voxel coordinate to the surface RAS (MRI) coordinates */
  fiffCoordTrans surf_RAS_RAS_t;             /* Transform from surface RAS to RAS (nonzero origin) coordinates */
  fiffCoordTrans head_surf_RAS_t;	     /* Transform from MEG head coordinates to surface RAS */
  fiffCoordTrans RAS_MNI_tal_t;              /* Transform from RAS (nonzero origin) to MNI Talairach coordinates */
  fiffCoordTrans MNI_tal_tal_gtz_t;          /* Transform MNI Talairach to FreeSurfer Talairach coordinates (z > 0) */
  fiffCoordTrans MNI_tal_tal_ltz_t;          /* Transform MNI Talairach to FreeSurfer Talairach coordinates (z < 0) */
  int            type;                       /* Data type for voxels */
  mneMRIvolume   *volumes;	             /* The volumes loaded */
  int            nvol;		             /* How many loaded volumes */
  int            current_vol;		     /* Which volume is the current destination for modifications */
  float          TR;			     /* Recovery time */
  float          TE;			     /* Echo time time */
  float          TI;			     /* Inversion time */
  float          flip_angle;		     /* Flip angle */
  float          fov;			     /* Field of view */
  void            *tags;		     /* The format of the tag information is kept private */
  void            *user_data;                /* Anything else we want */
  mneUserFreeFunc user_data_free;            /* Function to set the above free */
} *mneMRIdata,mneMRIdataRec;		     /* MRI data description suitable for handling mgh and mgz data files */

/*
 * Wavelet transform structures
 */
typedef struct {
  float sfreq;			/* The sampling frequency */
  int   nfreq;		        /* How many frequencies */
  float **W;			/* The corresponding wavelets */
  float **W_power;              /* Power spectra of the above */
  int   *np;                    /* Number of points in each */
  float *freqs;                 /* The corresponding frequencies */
  float **W_fft;		/* Fourier transforms of the wavelets for convolution */
  int   *np_fft;                /* Number of points in each the above */
  /*
   * Possibility to add user-defined data
   */
  void             *user_data;      /* Anything else we want */
  mneUserFreeFunc  user_data_free;  /* Function to set the above free */
} *mneWaveletSet,mneWaveletSetRec;

typedef struct {                /* Used to store the wavelet transform */
  int   type;                   /* What is stored here? */
  int   nchan;			/* Number of channels */
  int   nsamp;			/* Number of time samples */
  float sfreq;                  /* Sampling frequency */
  float *freqs;			/* Wavelet frequencies */
  int   nfreq;                  /* Number of frequencies */
  float tmin;                   /* Time scale minimum */
  float ***trans;               /* The transforms */
} *mneWaveletTransform,mneWaveletTransformRec;

#define MNE_WAVELET_COMPLEX 1
#define MNE_WAVELET_POWER   2

/*
 * Environment variables and others
 */
#define MNE_DEFAULT_TRIGGER_CH      "STI 014"
#define MNE_DEFAULT_FILTER_SIZE     4096
#define MNE_ENV_TRIGGER_CH          "MNE_TRIGGER_CH_NAME"
#define MNE_ENV_TRIGGER_CH_MASK     "MNE_TRIGGER_CH_MASK"
#define MNE_ENV_ROOT                "MNE_ROOT"
#define MNE_ENV_FILTER_SIZE         "MNE_FILTER_SIZE"

#if defined(__cplusplus) 
}
#endif
#endif
