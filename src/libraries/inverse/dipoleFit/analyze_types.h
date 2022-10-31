#ifndef _analyze_types_h
#define _analyze_types_h

#define SURF_UNKNOWN -1

#define SURF_LEFT_HEMI        FIFFV_MNE_SURF_LEFT_HEMI
#define SURF_RIGHT_HEMI       FIFFV_MNE_SURF_RIGHT_HEMI

#define SURF_BOTH_HEMIS       103

#define SURF_LEFT_MORPH_HEMI  (1 << 16 | FIFFV_MNE_SURF_LEFT_HEMI) 
#define SURF_RIGHT_MORPH_HEMI (1 << 16 | FIFFV_MNE_SURF_RIGHT_HEMI)

#define LABEL_TASK_NONE   -1
#define LABEL_TASK_MAX     1
#define LABEL_TASK_AVE     2
#define LABEL_TASK_L2      3
#define LABEL_TASK_L1      4
#define LABEL_TASK_TIME_L2 5

#define SHOW_CURVATURE_NONE    0
#define SHOW_CURVATURE_OVERLAY 1

#define SHOW_OVERLAY_NONE      0
#define SHOW_OVERLAY_HEAT      1
#define SHOW_OVERLAY_NEGPOS    2

#define ESTIMATE_NONE           0
#define ESTIMATE_MNE            1
#define ESTIMATE_dSPM           2
#define ESTIMATE_sLORETA        3
#define ESTIMATE_fMRI           4
#define ESTIMATE_NOISE          5
#define ESTIMATE_OTHER          6
#define ESTIMATE_PSF            7

#define FIELD_MAP_UNKNOWN       0
#define FIELD_MAP_MEG           8
#define GRAD_MAP_MEG            9
#define FIELD_MAP_EEG          10
#define OVERLAY_MAP            11

#define MAIN_SURFACES           0
#define ALT_SURFACES            1

#define OVERLAY_CORTEX          1
#define OVERLAY_SCALP           2

#define TIMECOURSE_SOURCE_UNKNOWN -1
#define TIMECOURSE_SOURCE_DATA     1
#define TIMECOURSE_SOURCE_OVERLAY  2

#define MNEV_REG_NOISE   1
#define MNEV_REG_PEARSON 2
#define MNEV_REG_GCV     3
       
#define NOISE_NORMALIZED(e) ((e) == ESTIMATE_dSPM || (e) == ESTIMATE_sLORETA)

#include <fiff/fiff_types.h>
#include <fiff/c/fiff_coord_trans_old.h>
#include <fwd/fwd_coil_set.h>
#include "../c/mne_meas_data.h"
#include <mne/c/mne_surface_or_volume.h>
#include <mne/c/mne_cov_matrix.h>
#include <fiff/c/fiff_digitizer_data.h>
#include <fiff/c/fiff_coord_trans_set.h>
#include <mne/c/mne_msh_color_scale_def.h>
#include <mne/c/mne_surface_patch.h>

//=============================================================================================================
// DEFINE NAMESPACE INVERSELIB
//=============================================================================================================

namespace INVERSELIB
{

typedef struct {
  float  megmin,megmax;             /* MEG gradiometer vertical scale [T/m] */
  float  megaxmult;                 /* Multiplier for the magnetometer scaling [m] */
  float  eegmin,eegmax;             /* EEG scale [V] */
  float  tmin,tmax;                 /* Time limits [sec] */
  float  full_range;                /* Use the full time range available */
  float  basemin,basemax;           /* Baseline limits [sec] */
  int    use_baseline;              /* Baseline active */
  int    show_stim;                 /* Show the digital stimulus channel in the sample display? */
  float  cursor_step;               /* How much to step in time when using the keyboard to go back and forth [sec] */
} *mshScales,mshScalesRec;

//typedef struct {		                     /* The celebrated tksurfer-style values */
//  int   type;		                             /* What is this scale setting good for? */
//  float mult;			                     /* Convenience multiplier from internal units to displayed numbers */
//  float fthresh;				     /* Threshold */
//  float fmid;					     /* This is in the middle */
//  float fslope;					     /* We still use the slope internally (sigh) */
//  float tc_mult;			             /* Multiply the scales by this value for timecourses */
//  int   relative;		                     /* Are fthresh and fmid relative to the maximum value over the surface? */
//} *mshColorScaleDef,mshColorScaleDefRec;

typedef struct {
  int                  meg_mapping_grade;            /* Icosahedron downsampling grade for the scalp MEG map (only applies to the head surface map) */
  int                  eeg_mapping_grade;            /* Icosahedron downsampling grade for the scalp EEG map*/
  float                miss_meg;	             /* Smoothing criterion for MEG */
  float                miss_eeg;	             /* Smoothing criterion for EEG */
  float                intrad;	                     /* Integration radius */
  int                  meg_nsmooth;	             /* Smoothing for the MEG head surface maps (only applies to the head surface map) */
  int                  eeg_nsmooth;                  /* Smoothing for the EEG head surface maps */
  float                meg_origin[3];                /* MEG sphere model origin (default to origin of MRI coordinates) */
  int                  meg_origin_frame;	     /* Which coordinate frame? (set to FIFFV_COORD_UNKNOWN for automatic setting) */
  float                eeg_origin[3];                /* EEG sphere model origin (Default to sphere fitting the electrodes best) */
  int                  eeg_origin_frame;	     /* Which coordinate frame? (set to FIFFV_COORD_UNKNOWN for automatic setting) */
  int                  head_surface_meg_map;         /* Calculate the MEG map on the head surface? */
  float                meg_contour_step;	     /* Primary source of steps */
  float                eeg_contour_step;	     /* Primary source of steps */
  float                meg_maxval;		     /* Maximum absolute value at the time the MEG contour step was recalculated */
  float                eeg_maxval;		     /* Maximum absolute value at the time the EEG contour step was recalculated */
} *fieldMappingPref,fieldMappingPrefRec;

typedef struct {
  int                  reg_method;                   /* The method used to compute regularization estimates */
  float                reg_SNR;	                     /* The virtual (power) SNR employed in regularization */
  int                  estimate;                     /* What to calculate (MNE, dSPM, sLORETA) */
  int                  do_sqrt;                      /* do square root? */
  int                  do_signed;                    /* Preserve sign (current orientation) */
  int                  do_normal_comp;               /* Omit the components tangential to the cortex */
  int                  do_alt_noise;		     /* Try an alternate noise normalization */
  MNELIB::MneMshColorScaleDef  scale_MNE;                    /* MNE scale */
  MNELIB::MneMshColorScaleDef  scale_dSPM;                   /* SPM scale */
  MNELIB::MneMshColorScaleDef  scale_sLORETA;                /* sLORETA scale */
  int                  nstep;	                     /* Desired number of smoothsteps */
  float                integ;	                     /* Time integration to apply */
  int                  show_scale_bar;               /* Display the color scale? */
  int                  show_comments;		     /* Show the comment text */
  int                  sketch_mode;		     /* Use sketch mode? */
  int                  thresh_estimate;		     /* Use this statistic to threshold */
  float                thresh;                       /* Threshold the data at this value */
  float                alpha;	                     /* Opacity 0...1 */
  int                  simulate;                     /* Simulate data on picking points from the surface */
  /*
   * More to come
   */
} *mnePref,mnePrefRec;

typedef struct {		                     /* Stores label information */
  char  *file_name;		                     /* True file name */
  char  *short_name;		                     /* Nickname */
  char  *comment;				     /* Comment read from the label file (first line) */
  int   hemi;			                     /* Which hemisphere */
  int   *sel;			                     /* Vertices */
  int   nsel;			                     /* How many? */
  float color[4];				     /* Color for this label */
} *mshLabel,mshLabelRec;

typedef struct {
  mshLabel *labels;
  int      nlabel;
} *mshLabelSet,mshLabelSetRec;

typedef struct {
  char                *short_name;		     /* Short name for this overlay */
  int                 surf_type;		     /* OVERLAY_CORTEX or OVERLAY_SCALP */
  int                 type;	                     /* Type of the overlay data */
  int                 is_signed;		     /* Are these data signed? */
  MNELIB::MneMshColorScaleDef scale;			     /* Scale to use */
  int                 show_comments;		     /* Show the comment text */
  int                 show_scale_bar;		     /* Show the scale bar */
  int                 show_contours;		     /* Show the contour map for OVERLAY_SCALP */
  float               contour_step;		     /* Step between the OVERLAY_SCALP contours */
  float               alpha;	                     /* Determines the opacity */
  int                 nstep;			     /* Number of smoothsteps to take */
  float               time;	                     /* Timeslice to pick from stc data (in seconds) */
  float               integ;			     /* Time integration */
//  mneWdata            lh_data;			     /* LH wdata loaded from an overlay file */
//  mneWdata            rh_data;			     /* RH wdata loaded from an overlay file */
//  mneStcData          lh_stc_data;	             /* LH stc data loaded from an overlay file */
//  mneStcData          rh_stc_data;	             /* RH stc data loaded from an overlay file */
  int                 lh_sparse;	             /* Is LH data sparse */
  int                 rh_sparse;		     /* Is RH data sparse */
  int                 lh_match_surf;                 /* What kind of surface does the data match to */
  int                 rh_match_surf;                 /* What kind of surface does the data match to */
  float               **hist;			     /* The value histogram */
  int                 nbin;			     /* How many bins */
} *mneOverlay,mneOverlayRec;

typedef struct {		                     /* Overlay preferences. This structure can be kept private */
  int                  type;                         /* What kind of overlay? (fMRI, other) */
  int                  do_signed;                    /* Preserve sign (current orientation)
                              * This is never changed, just copied from the current overlay */
  MNELIB::MneMshColorScaleDef  scale_MNE;                    /* MNE scale */
  MNELIB::MneMshColorScaleDef  scale_dSPM;                   /* SPM scale */
  MNELIB::MneMshColorScaleDef  scale_sLORETA;                /* sLORETA scale */
  MNELIB::MneMshColorScaleDef  scale_fMRI;                   /* Scale */
  MNELIB::MneMshColorScaleDef  scale_other;                  /* Scale */
  int                  nstep;	                     /* Desired number of smoothsteps */
  float                alpha;	                     /* opacity */
  int                  show_comments;		     /* Show comment text */
  int                  show_scale_bar;               /* Show the scale bar */
  int                  show_contours;		     /* Show the contour map */
  float                contour_step;                 /* Step between contours */
  int                  surf_type;                    /* OVERLAY_SCALP or OVERLAY_CORTEX */
} *overlayPref,overlayPrefRec;

typedef struct {		                     /* This is used for field mapping with help of the sphere-model MNE */
  int          kind;				     /* Either FIELD_MAP_MEG or FIELD_MAP_EEG */
  MNELIB::MneSurfaceOld*   surf;		                     /* The surface on which we are mapping */
  char         *surfname;	                     /* The name of the file where the above surface came from */
  int          *surface_sel;			     /* We may calculate the interpolation only in a subset of vertices */
  int          nsurface_sel;			     /* How many points in the above */
  int          nsmooth;				     /* How many smoothsteps to take after the extrapolation/interpolation */
  float        **smooth_weights;                     /* The smoothing weights */
  int          nch;			             /* How many channels */
  int          *pick;		                     /* Which channels are of this modality in the original data */
  FWDLIB::FwdCoilSet*   coils;		                     /* Coils */
  float        origin[3];		             /* Origin */
  float        miss;		                     /* Amount of unexplained variance */
  float        **self_dots;	                     /* Dot products between the original leads */
  float        **surface_dots;			     /* Dot products from the original leads to the virtual leads */
  float        intrad;		                     /* The integration radius used */
  MNELIB::MneCovMatrix* noise;				     /* Noise-covariance matrix to use */
  int          nest;	                             /* How many singular values to include? */
  float        **mapping_mat;		             /* The mapping matrix */
} *fieldMappingData, fieldMappingDataRec;

typedef struct {		                     /* This is used for the calculated contours */
  int        kind;				     /* Either FIELD_MAP_MEG or FIELD_MAP_EEG */
  float      *map;				     /* The contour map values at the vertices of the surface (could go to the overlay data as well) */
  int        nmap;				     /* How many values */
  int        show;				     /* Should we really show this map */
  float      step;				     /* Contour step */
} *contourMap,contourMapRec;

//typedef struct {                            /* The digitizer data will be loaded from the measurement file or elsewhere */
//  char           *filename;                 /* Where did these come from */
//  FIFFLIB::FiffCoordTransOld* head_mri_t;            /* This is relevant for us */
//  FIFFLIB::FiffCoordTransOld* head_mri_t_adj;        /* This is the adjusted transformation */
//  FIFFLIB::fiffDigPoint   points;           /* The points */
//  int            coord_frame;               /* The coordinate frame of the above points */
//  int            *active;                   /* Which are active */
//  int            *discard;                  /* Which should be discarded? */
//  int            npoint;                    /* How many? */
//  FIFFLIB::fiffDigPoint   mri_fids;         /* MRI coordinate system fiducials picked here */
//  int            nfids;                     /* How many? */
//  int            show;                      /* Should the digitizer data be shown */
//  int            show_minimal;              /* Show fiducials and coils only? */
//  float          *dist;                     /* Distance of each point from the head surface */
//  int            *closest;                  /* Closest vertex # on the head surface */
//  float          **closest_point;           /* Closest vertex locations on the head surface */
//  int            dist_valid;                /* Are the above data valid at this point? */
//} *digitizerData,digitizerDataRec;

typedef struct {                                     /* These are the data from the HPI result block */
  char           *filename;			     /* Where did these come from */
  FIFFLIB::FiffCoordTransSet* meg_head_t;			     /* The MEG device <-> head coordinate system transformation */
  FIFFLIB::FiffDigPoint   *hpi_coils;			     /* Locations of the HPI coils in MEG device coordinates */
  int            ncoil;			             /* How many of them? */
  int            *dig_order;			     /* Which digitized HPI coil corresponds to each of the above coils */
  int            *coils_used;			     /* Which coils were used? */
  int            nused;				     /* How many were used? */
  float          *g_values;			     /* Goodness of fit for each of the coils */
  float          g_limit;			     /* Goodness-of-fit acceptance requirement */
  float          dist_limit;			     /* Distance acceptance requirement */
  int            fit_accept;			     /* Was the fit accepted */
} *hpiResultData,hpiResultDataRec;

typedef struct {				     /* One saved timecourse */
  char          *file_name;			     /* The associated file name */
  char          *comment;	                     /* Descriptive comment */
  int           source;		                     /* Does this come from the data or from an overlay */
  int           quantity;			     /* What is this all about */
  int           fixed_ori;			     /* Fixed orientations? */
  int           active;				     /* Is this to be shown */
  mshLabel      label;	                             /* Label for this timecourse */
  float         **timecourses;			     /* Timecourses for the points in label */
  int           *sel;				     /* Vertex numbers in the complete triangulation for each time course */
  float         **rr;				     /* Locations of these points */
  int           coord_frame;			     /* Which coordinate frame? */
  int           nsel;				     /* How many? */
  int           label_task;                          /* What has been done with it */
  float         *timecourse_task;                    /* The corresponding waveform */
  int           is_signed;	                     /* Does this timecourse contain positive and negative data */
  int           np;				     /* How many data points? */
  float         tmin;				     /* Starting time */
  float         tstep;				     /* Time between points */
  float         color[3];	                     /* Color for drawing */
} *timecourseData,timecourseDataRec;

typedef struct {				     /* A collection of the above */
  timecourseData *timecourses;			     /* Pointers to the above structures */
  int            ncourse;			     /* How many */
} *timecourseDataSet,timecourseDataSetRec;

typedef struct {
  int           sample;				     /* Which channel is the sample */
  float         picked_time;	                     /* Current time point */
  int           have_picked;			     /* Have we picked a time */
  mshScales     scales;
  mnePref       mne_prefs;
} *dataSetData,dataSetDataRec;			     /* These have to be kept between data set changes */

typedef struct {
  char          *meas_file;                     /* The measurement file */
  char          *inv_file;                      /* Inverse operator file */
  char          *mri_trans_file;                /* Where does the MRI transform come from */
  int           nave;                           /* If nave < 0 use nave from the measurement data? */
  INVERSELIB::MneMeasData*  meas;               /* The measurement */
  float         raw_tmin,raw_tmax;		     /* Time range for raw data segments */
  int           sample;				     /* Which channel is the sample */
  int           firstp;		                     /* First data point in the current time range selection */
  int           np;		                     /* Number of data points in the current time range selection */
  float         tmin,tmax;	                     /* Corresponding time values (precalculated for convenience) */
  float         picked_time;	                     /* Current time point */
  int           have_picked;			     /* Have we picked a time */
  double        abs_picked_time;                     /* Absolute time picked from the start of the raw data
                              * (good to use double because of the possibly long time span) */
  int           *bads;		                     /* Which channels are bad */
  int           *sels;		                     /* Which channels have been selected for dipole fitting? */
  char          *selname;	                     /* Name of the current channel selection (if any) */
  MNELIB::mneLayout     lout;				     /* This is the layout */
  mshScales     scales;				     /* Time and vertical scale and baseline */

  float         *custom_data;	                     /* Custom data to use instead of data picked from the responses */
  char          *custom_desc;	                     /* Description of the custom data */

  mnePref             mne_prefs;		     /* MNE calculation preferences */
  float               *cur_vals;                     /* Current values */
  FIFFLIB::FiffSparseMatrix* nn_vals;			     /* Noise normalization values */
  MNELIB::MneMshColorScaleDef scale;	                     /* Scale presently used for display */

  FIFFLIB::FiffDigitizerData    *dig;                              /* These are the Polhemus data */

  fieldMappingData meg_mapping;			     /* Data for field interpolations (MEG on helmet) */
  fieldMappingData meg_mapping_head;		     /* Data for field interpolations (MEG on scalp)  */
  fieldMappingData eeg_mapping;			     /* Data for field interpolations (EEG on scalp)  */
  fieldMappingPref mapping_pref;                     /* Desired settings */

  void             *dipole_fit_setup;                /* Dipole fitting data (opaque for us) */
  mneUserFreeFunc  dipole_fit_setup_free;

  void             *user_data;                       /* Can be used to store whatever */
  mneUserFreeFunc  user_data_free;                   /* Called to free the above object */
} *mshMegEegData,mshMegEegDataRec;

//typedef struct {		/* Definition of lighting */
//  int   state;			/* On or off? */
//  float pos[3];			/* Where is the light? */
//  float diff[3];		/* Diffuse intensity */
//} *mshLight,mshLightRec;	/* We are only using diffuse lights here */

//typedef struct {		/* Light set */
//  char     *name;		/* Name of this set */
//  mshLight lights;		/* Which lights */
//  int      nlight;		/* How many */
//} *mshLightSet,mshLightSetRec;

//typedef struct {
//  int   vert;			/* Vertex # */
//  int   sparse;			/* Is this a isolated point? */
//} *mshPicked,mshPickedRec;

//typedef struct {
//  FIFFLIB::FiffSparseMatrix* map;		/* Multiply the data in the from surface with this to get to
//                 * 'this' surface from the 'from' surface */
//  int *best;			/* For each point on 'this' surface, the closest point on 'from' surface */
//  int from_kind;		/* The kind field of the other surface */
//  char *from_subj;		/* Name of the subject of the other surface */
//} *morphMap,morphMapRec;

//typedef struct {		/* Display surface properties */
//  char           *filename;	/* Where did this surface come from? */
//  time_t         time_loaded;	/* When was the surface loaded */
//  char           *subj;		/* The name of the subject in SUBJECTS_DIR */
//  char           *surf_name;	/* The name of the surface */
//  MNELIB::MneSurfaceOld*     s;		/* This is the surface */
//  float          eye[3];	/* Eye position for viewing */
//  float          up[3];		/* Up vector for viewing */
//  float          rot[3];        /* Rotation angles of the MRI (in radians) */
//  float          move[3];	/* Possibly move the origin, too */

//  float          fov;		/* Field of view (extent of the surface) */
//  float          fov_scale;	/* How much space to leave */
//  float          minv[3];	/* Minimum values along the three coordinate axes */
//  float          maxv[3];	/* Maximum values along the three coordinate axes */
//  float          *trans;	/* Extra transformation for this surface */
//  int            sketch;	/* Use sketch mode if decimated triangulation is available? */

//  morphMap       *maps;		/* Morphing maps from other surfaces to this */
//  int            nmap;		/* Normally just one */

//  int   overlay_type;	        /* What are the overlay values? */
//  float *overlay_values;	/* Overlay value array */
//  int   alt_overlay_type;	/* A second choice for overlay */
//  float *alt_overlay_values;
//  float *marker_values;		/* Marker values (will be shown in shades of marker color) */

//  float *vertex_colors;		/* Vertex color array */
//  mshColorScaleDef* color_scale; /* Color scale used to define these colors */
//  int   nvertex_colors;		/* How many components? */
//  float even_vertex_color[4];	/* This is going to be employed in case of uniform coloring */

//  float *marker_colors;		/* Vertex color array (for the markers) */
//  int   nmarker_colors;		/* How many components? */
//  int   **marker_tri;	        /* Triangles containing markers */
//  int   *marker_tri_no;		/* Numbers of the marker triangles */
//  int   nmarker_tri;		/* How many */
//  float marker_color[4];	/* Marker color */
//  int   curvature_color_mode;	/* How to show curvature */

//  int   overlay_color_mode;	/* How to show overlay data */
//  int   transparent;		/* Is this surface going to be transparent? */

//  int   show_aux_data;		/* Show auxilliary data related to this surface */

//  mshPicked* picked;		/* Picked locations in world coordinates */
//  int       npicked;		/* How many */

//  void              *user_data;       /* Can be used to store whatever */
//  mneUserFreeFunc*   user_data_free;   /* Function to free the above */
//} *mshDisplaySurface,mshDisplaySurfaceRec;

//typedef struct {
//  FIFFLIB::FiffCoordTransOld*    head_surf_RAS_t;   /* Transform from MEG head coordinates to surface RAS */
//  FIFFLIB::FiffCoordTransOld*    surf_RAS_RAS_t;    /* Transform from surface RAS to RAS (nonzero origin) coordinates */
//  FIFFLIB::FiffCoordTransOld*    RAS_MNI_tal_t;     /* Transform from RAS (nonzero origin) to MNI Talairach coordinates */
//  FIFFLIB::FiffCoordTransOld*    MNI_tal_tal_gtz_t; /* Transform MNI Talairach to FreeSurfer Talairach coordinates (z > 0) */
//  FIFFLIB::FiffCoordTransOld*    MNI_tal_tal_ltz_t; /* Transform MNI Talairach to FreeSurfer Talairach coordinates (z < 0) */
//} *coordTransSet,coordTransSetRec;

//typedef struct {		       /* Set of display surfaces */
//  char              *subj;	       /* The name of the subject */
//  char              *morph_subj;       /* The subject we are morphing to */
//  FIFFLIB::FiffCoordTransSet*     main_t;            /* Coordinate transformations for the main surfaces */
//  FIFFLIB::FiffCoordTransSet*     morph_t;           /* Coordinate transformations for the morph surfaces */
//  MNELIB::MneMshDisplaySurface **surfs;	       /* These are the surfaces */
//  MNELIB::MneSurfacePatch   **patches;	       /* Optional patches for display */
//  float             *patch_rot;	       /* Rotation angles for the (flat) patches */
//  int               nsurf;	       /* How many? */
//  int               use_patches;       /* Use patches for display? */
//  int               *active;	       /* Which surfaces are currently active */
//  int               *drawable;	       /* Which surfaces could be drawn? */
//  mshLightSet       lights;            /* Lighting */
//  float             rot[3];            /* Rotation angles of the MRI (in radians) */
//  float             move[3];	       /* Possibly move the origin, too */
//  float             fov;	       /* Field of view (extent of the surface) */
//  float             fov_scale;	       /* How much space to leave */
//  float             eye[3];	       /* Eye position for viewing (used in composite views) */
//  float             up[3];	       /* Up vector for viewing */
//  float             bg_color[3];       /* Background color */
//  float             text_color[3];     /* Text color */
//  void              *user_data;        /* Can be used to store whatever */
//  mneUserFreeFunc   user_data_free;
//} *mshDisplaySurfaceSet,mshDisplaySurfaceSetRec;

//typedef struct {		/* Where to look at the surfaces from */
//  char  *name;			/* Name of this definition */
//  float left[3];		/* Left hemisphere viewpoint */
//  float right[3];		/* Right hemisphere viewpoint */
//  float left_up[3];		/* The up vectors */
//  float right_up[3];		/* The up vectors */
//} *mshEyes,mshEyesRec;

typedef void (*colorEditorDoneFunc)(float *color, void *user);

typedef struct {
  float time;			/* Time point */
  int   samp;			/* Corresponding sample number */
  float quater[4];		/* The unit quaternion */
  float move[3];		/* Translation */
  float good;			/* Geometric mean of the goodness of fits */
  FIFFLIB::FiffCoordTransOld *t;		/* The corresponding fiff coordinate transformation */
} *contHpiData,contHpiDataRec;

typedef struct {
  contHpiData *hpi;		/* The data records */
  int         nhpi;		/* How many of them */
  int         omit_samp;	/* How many samples were omitted from the beginning (first_samp + initial skip) */
  float       sfreq;		/* Sampling frequency of the data file */
  /*
   * The following are needed in the the viewing
   */
  int         current;		/* Which point is the interesting one now? */
  float       max_coil_move;	/* Average coil movement scale */
  float       max_velocity;	/* Angular velocity scale */
} *contHpiDataSet, contHpiDataSetRec;

} // Namespace

#endif
