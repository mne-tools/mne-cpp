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
#include "fwd_coil_set.h"



typedef struct {
  float  megmin,megmax;				     /* MEG gradiometer vertical scale [T/m] */
  float  megaxmult;				     /* Multiplier for the magnetometer scaling [m] */
  float  eegmin,eegmax;				     /* EEG scale [V] */
  float  tmin,tmax;				     /* Time limits [sec] */
  float  full_range;		                     /* Use the full time range available */
  float  basemin,basemax;			     /* Baseline limits [sec] */
  int    use_baseline;		                     /* Baseline active */
  int    show_stim;		                     /* Show the digital stimulus channel in the sample display? */
  float  cursor_step;				     /* How much to step in time when using the keyboard to go back and forth [sec] */
} *mshScales,mshScalesRec;

typedef struct {		                     /* The celebrated tksurfer-style values */
  int   type;		                             /* What is this scale setting good for? */
  float mult;			                     /* Convenience multiplier from internal units to displayed numbers */
  float fthresh;				     /* Threshold */
  float fmid;					     /* This is in the middle */
  float fslope;					     /* We still use the slope internally (sigh) */
  float tc_mult;			             /* Multiply the scales by this value for timecourses */
  int   relative;		                     /* Are fthresh and fmid relative to the maximum value over the surface? */
} *mshColorScaleDef,mshColorScaleDefRec;

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
  mshColorScaleDefRec  scale_MNE;                    /* MNE scale */
  mshColorScaleDefRec  scale_dSPM;                   /* SPM scale */
  mshColorScaleDefRec  scale_sLORETA;                /* sLORETA scale */
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

typedef struct {		                     /* This is used for field mapping with help of the sphere-model MNE */
  int          kind;				     /* Either FIELD_MAP_MEG or FIELD_MAP_EEG */
  mneSurface   surf;		                     /* The surface on which we are mapping */
  char         *surfname;	                     /* The name of the file where the above surface came from */
  int          *surface_sel;			     /* We may calculate the interpolation only in a subset of vertices */
  int          nsurface_sel;			     /* How many points in the above */
  int          nsmooth;				     /* How many smoothsteps to take after the extrapolation/interpolation */
  float        **smooth_weights;                     /* The smoothing weights */
  int          nch;			             /* How many channels */
  int          *pick;		                     /* Which channels are of this modality in the original data */
  FwdCoilSet*   coils;		                     /* Coils */
  float        origin[3];		             /* Origin */
  float        miss;		                     /* Amount of unexplained variance */
  float        **self_dots;	                     /* Dot products between the original leads */
  float        **surface_dots;			     /* Dot products from the original leads to the virtual leads */
  float        intrad;		                     /* The integration radius used */
  mneCovMatrix noise;				     /* Noise-covariance matrix to use */
  int          nest;	                             /* How many singular values to include? */
  float        **mapping_mat;		             /* The mapping matrix */
} *fieldMappingData, fieldMappingDataRec;

typedef struct {		                     /* The digitizer data will be loaded from the measurement file or elsewhere */
  char           *filename;			     /* Where did these come from */
  FIFFLIB::fiffCoordTrans head_mri_t;			     /* This is relevant for us */
  FIFFLIB::fiffCoordTrans head_mri_t_adj;                     /* This is the adjusted transformation */
  FIFFLIB::fiffDigPoint   points;			     /* The points */
  int            coord_frame;	                     /* The coordinate frame of the above points */
  int            *active;	                     /* Which are active */
  int            *discard;	                     /* Which should be discarded? */
  int            npoint;			     /* How many? */
  FIFFLIB::fiffDigPoint   mri_fids;	                     /* MRI coordinate system fiducials picked here */
  int            nfids;		                     /* How many? */
  int            show;		                     /* Should the digitizer data be shown */
  int            show_minimal;                       /* Show fiducials and coils only? */
  float          *dist;		                     /* Distance of each point from the head surface */
  int            *closest;			     /* Closest vertex # on the head surface */
  float          **closest_point;		     /* Closest vertex locations on the head surface */
  int            dist_valid;			     /* Are the above data valid at this point? */
} *digitizerData,digitizerDataRec;

typedef struct {
  char          *meas_file;	                     /* The measurement file */
  char          *inv_file;			     /* Inverse operator file */
  char          *mri_trans_file;		     /* Where does the MRI transform come from */
  int           nave;			             /* If nave < 0 use nave from the measurement data? */
  mneMeasData   meas;		                     /* The measurement */
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
  mneLayout     lout;				     /* This is the layout */
  mshScales     scales;				     /* Time and vertical scale and baseline */

  float         *custom_data;	                     /* Custom data to use instead of data picked from the responses */
  char          *custom_desc;	                     /* Description of the custom data */

  mnePref             mne_prefs;		     /* MNE calculation preferences */
  float               *cur_vals;                     /* Current values */
  mneSparseMatrix     nn_vals;			     /* Noise normalization values */
  mshColorScaleDefRec scale;	                     /* Scale presently used for display */

  digitizerData    dig;                              /* These are the Polhemus data */

  fieldMappingData meg_mapping;			     /* Data for field interpolations (MEG on helmet) */
  fieldMappingData meg_mapping_head;		     /* Data for field interpolations (MEG on scalp)  */
  fieldMappingData eeg_mapping;			     /* Data for field interpolations (EEG on scalp)  */
  fieldMappingPref mapping_pref;                     /* Desired settings */

  void             *dipole_fit_setup;                /* Dipole fitting data (opaque for us) */
  mneUserFreeFunc  dipole_fit_setup_free;

  void             *user_data;                       /* Can be used to store whatever */
  mneUserFreeFunc  user_data_free;                   /* Called to free the above object */
} *mshMegEegData,mshMegEegDataRec;

#endif
