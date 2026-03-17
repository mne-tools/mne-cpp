//=============================================================================================================
/**
 * @file     inv_analyze_types.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2017
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
 * @brief    Legacy C-style type definitions for MEG/EEG analysis, visualization, and inverse modelling.
 *
 * @details  This header collects the original MNE C library "analyze" type definitions that are
 *           used across the entire inverse library — not only by dipole fitting, but also by
 *           field mapping, overlay display, HPI processing, and general MEG/EEG data handling.
 *
 *           The types are C-style typedef'd structs (pointer-to-struct idiom) preserved from the
 *           MNE C code base for backward compatibility.  New code should prefer modern C++ classes
 *           where possible; these definitions remain available for components that still rely on
 *           the legacy data layout.
 *
 */

#ifndef INV_ANALYZE_TYPES_H
#define INV_ANALYZE_TYPES_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_global.h"
#include <mne/mne_meas_data.h>

#include <fiff/fiff_types.h>
#include <fiff/fiff_digitizer_data.h>
#include <fiff/fiff_coord_trans_set.h>
#include <fwd/fwd_coil_set.h>
#include <mne/mne_surface_or_volume.h>
#include <mne/mne_cov_matrix.h>
#include <mne/mne_msh_color_scale_def.h>
#include <mne/mne_surface_patch.h>
#include <mne/mne_layout.h>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// DEFINE MACROS
//=============================================================================================================

/** @name Surface hemisphere identifiers */
///@{
#define SURF_UNKNOWN           -1

#define SURF_LEFT_HEMI          FIFFV_MNE_SURF_LEFT_HEMI
#define SURF_RIGHT_HEMI         FIFFV_MNE_SURF_RIGHT_HEMI
#define SURF_BOTH_HEMIS         103

#define SURF_LEFT_MORPH_HEMI   (1 << 16 | FIFFV_MNE_SURF_LEFT_HEMI)
#define SURF_RIGHT_MORPH_HEMI  (1 << 16 | FIFFV_MNE_SURF_RIGHT_HEMI)
///@}

/** @name Label extraction tasks */
///@{
#define LABEL_TASK_NONE        -1
#define LABEL_TASK_MAX          1
#define LABEL_TASK_AVE          2
#define LABEL_TASK_L2           3
#define LABEL_TASK_L1           4
#define LABEL_TASK_TIME_L2      5
///@}

/** @name Curvature display modes */
///@{
#define SHOW_CURVATURE_NONE     0
#define SHOW_CURVATURE_OVERLAY  1
///@}

/** @name Overlay colour modes */
///@{
#define SHOW_OVERLAY_NONE       0
#define SHOW_OVERLAY_HEAT       1
#define SHOW_OVERLAY_NEGPOS     2
///@}

/** @name Source estimate kind tags */
///@{
#define ESTIMATE_NONE           0
#define ESTIMATE_MNE            1
#define ESTIMATE_dSPM           2
#define ESTIMATE_sLORETA        3
#define ESTIMATE_fMRI           4
#define ESTIMATE_NOISE          5
#define ESTIMATE_OTHER          6
#define ESTIMATE_PSF            7
///@}

/** @name Field map type identifiers */
///@{
#define FIELD_MAP_UNKNOWN       0
#define FIELD_MAP_MEG           8
#define GRAD_MAP_MEG            9
#define FIELD_MAP_EEG          10
#define OVERLAY_MAP            11
///@}

/** @name Surface set indices */
///@{
#define MAIN_SURFACES           0
#define ALT_SURFACES            1
///@}

/** @name Overlay target surfaces */
///@{
#define OVERLAY_CORTEX          1
#define OVERLAY_SCALP           2
///@}

/** @name Timecourse data origins */
///@{
#define TIMECOURSE_SOURCE_UNKNOWN  -1
#define TIMECOURSE_SOURCE_DATA      1
#define TIMECOURSE_SOURCE_OVERLAY   2
///@}

/** @name MNE regularisation method tags */
///@{
#define MNEV_REG_NOISE          1
#define MNEV_REG_PEARSON        2
#define MNEV_REG_GCV            3
///@}

/**
 * @brief Check whether an estimate type is noise-normalised (dSPM or sLORETA).
 */
#define NOISE_NORMALIZED(e) ((e) == ESTIMATE_dSPM || (e) == ESTIMATE_sLORETA)

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
// TYPEDEFS — Legacy C-style structures
//=============================================================================================================

/**
 * @brief Display scale settings for MEG/EEG amplitude, time range, and baseline parameters.
 *
 * Stores vertical display range for gradiometers and EEG, the active time window,
 * baseline interval, and cursor stepping preferences used by the interactive viewer.
 */
typedef struct {
    float  megmin, megmax;          /**< MEG gradiometer vertical scale [T/m]. */
    float  megaxmult;               /**< Multiplier for the magnetometer scaling [m]. */
    float  eegmin, eegmax;          /**< EEG scale [V]. */
    float  tmin, tmax;              /**< Time limits [sec]. */
    float  full_range;              /**< Use the full time range available. */
    float  basemin, basemax;        /**< Baseline limits [sec]. */
    int    use_baseline;            /**< Baseline active. */
    int    show_stim;               /**< Show the digital stimulus channel in the sample display? */
    float  cursor_step;             /**< Time step for keyboard navigation [sec]. */
} *mshScales, mshScalesRec;

/**
 * @brief Preferences for MEG/EEG field mapping including interpolation grades, smoothing, and sphere model origins.
 *
 * Controls the icosahedron subdivision level, smoothing strength, integration radius,
 * and sphere-model origins (MEG and EEG) that drive the field-interpolation pipeline.
 */
typedef struct {
    int    meg_mapping_grade;       /**< Icosahedron grade for scalp MEG map (head surface only). */
    int    eeg_mapping_grade;       /**< Icosahedron grade for scalp EEG map. */
    float  miss_meg;                /**< Smoothing criterion for MEG. */
    float  miss_eeg;                /**< Smoothing criterion for EEG. */
    float  intrad;                  /**< Integration radius. */
    int    meg_nsmooth;             /**< Smoothing steps for MEG head surface maps. */
    int    eeg_nsmooth;             /**< Smoothing steps for EEG head surface maps. */
    float  meg_origin[3];           /**< MEG sphere model origin (defaults to MRI coordinates origin). */
    int    meg_origin_frame;        /**< Coordinate frame (FIFFV_COORD_UNKNOWN for automatic). */
    float  eeg_origin[3];           /**< EEG sphere model origin (defaults to best-fit sphere). */
    int    eeg_origin_frame;        /**< Coordinate frame (FIFFV_COORD_UNKNOWN for automatic). */
    int    head_surface_meg_map;    /**< Calculate the MEG map on the head surface? */
    float  meg_contour_step;        /**< Contour step for MEG. */
    float  eeg_contour_step;        /**< Contour step for EEG. */
    float  meg_maxval;              /**< Maximum absolute value when MEG contour step was last recalculated. */
    float  eeg_maxval;              /**< Maximum absolute value when EEG contour step was last recalculated. */
} *fieldMappingPref, fieldMappingPrefRec;

/**
 * @brief MNE computation preferences including regularisation, estimate type, colour scales, and display options.
 */
typedef struct {
    int                          reg_method;        /**< Regularisation method. */
    float                        reg_SNR;           /**< Virtual (power) SNR for regularisation. */
    int                          estimate;          /**< What to calculate (MNE, dSPM, sLORETA). */
    int                          do_sqrt;           /**< Apply square root? */
    int                          do_signed;         /**< Preserve sign (current orientation). */
    int                          do_normal_comp;    /**< Omit tangential components. */
    int                          do_alt_noise;      /**< Try an alternate noise normalisation. */
    MNELIB::MNEMshColorScaleDef  scale_MNE;        /**< MNE colour scale. */
    MNELIB::MNEMshColorScaleDef  scale_dSPM;       /**< dSPM colour scale. */
    MNELIB::MNEMshColorScaleDef  scale_sLORETA;    /**< sLORETA colour scale. */
    int                          nstep;             /**< Number of smooth steps. */
    float                        integ;             /**< Time integration to apply. */
    int                          show_scale_bar;    /**< Display the colour scale? */
    int                          show_comments;     /**< Show the comment text. */
    int                          sketch_mode;       /**< Use sketch mode? */
    int                          thresh_estimate;   /**< Statistic used for thresholding. */
    float                        thresh;            /**< Threshold value. */
    float                        alpha;             /**< Opacity 0..1. */
    int                          simulate;          /**< Simulate data on picking points from the surface. */
} *mnePref, mnePrefRec;

/**
 * @brief Cortical label storing vertex indices, hemisphere assignment, and display colour.
 */
typedef struct {
    char  *file_name;       /**< True file name. */
    char  *short_name;      /**< Nickname. */
    char  *comment;         /**< Comment read from the label file (first line). */
    int    hemi;            /**< Which hemisphere. */
    int   *sel;             /**< Selected vertices. */
    int    nsel;            /**< Number of selected vertices. */
    float  color[4];        /**< Display colour for this label. */
} *mshLabel, mshLabelRec;

/**
 * @brief Collection of cortical labels for region-of-interest analysis.
 */
typedef struct {
    mshLabel *labels;       /**< Array of labels. */
    int       nlabel;       /**< Number of labels. */
} *mshLabelSet, mshLabelSetRec;

/**
 * @brief Overlay data for cortical or scalp surface display.
 *
 * Contains colour-scale, contour, opacity, and smoothing settings together with the
 * actual overlay values for left and right hemispheres.
 */
typedef struct {
    char                         *short_name;       /**< Short name for this overlay. */
    int                           surf_type;        /**< OVERLAY_CORTEX or OVERLAY_SCALP. */
    int                           type;             /**< Type of the overlay data. */
    int                           is_signed;        /**< Are these data signed? */
    MNELIB::MNEMshColorScaleDef   scale;            /**< Colour scale. */
    int                           show_comments;    /**< Show the comment text. */
    int                           show_scale_bar;   /**< Show the scale bar. */
    int                           show_contours;    /**< Show the contour map for OVERLAY_SCALP. */
    float                         contour_step;     /**< Step between OVERLAY_SCALP contours. */
    float                         alpha;            /**< Opacity. */
    int                           nstep;            /**< Number of smooth steps. */
    float                         time;             /**< Timeslice to pick from stc data [sec]. */
    float                         integ;            /**< Time integration. */
    int                           lh_sparse;        /**< Is LH data sparse. */
    int                           rh_sparse;        /**< Is RH data sparse. */
    int                           lh_match_surf;    /**< LH surface match kind. */
    int                           rh_match_surf;    /**< RH surface match kind. */
    float                       **hist;             /**< Value histogram. */
    int                           nbin;             /**< Number of bins. */
} *mneOverlay, mneOverlayRec;

/**
 * @brief Overlay display preferences storing colour-scale definitions, smoothing, opacity, and contour settings per overlay type.
 */
typedef struct {
    int                          type;              /**< Overlay kind (fMRI, other). */
    int                          do_signed;         /**< Preserve sign (copied from current overlay). */
    MNELIB::MNEMshColorScaleDef  scale_MNE;        /**< MNE colour scale. */
    MNELIB::MNEMshColorScaleDef  scale_dSPM;       /**< dSPM colour scale. */
    MNELIB::MNEMshColorScaleDef  scale_sLORETA;    /**< sLORETA colour scale. */
    MNELIB::MNEMshColorScaleDef  scale_fMRI;       /**< fMRI colour scale. */
    MNELIB::MNEMshColorScaleDef  scale_other;      /**< Other overlay colour scale. */
    int                          nstep;             /**< Number of smooth steps. */
    float                        alpha;             /**< Opacity. */
    int                          show_comments;     /**< Show comment text. */
    int                          show_scale_bar;    /**< Show the scale bar. */
    int                          show_contours;     /**< Show the contour map. */
    float                        contour_step;      /**< Step between contours. */
    int                          surf_type;         /**< OVERLAY_SCALP or OVERLAY_CORTEX. */
} *overlayPref, overlayPrefRec;

/**
 * @brief Sphere-model field-mapping data.
 *
 * Holds coil definitions, dot-product matrices, and the interpolation matrix used to
 * project MEG or EEG sensor data onto a surface grid.
 */
typedef struct {
    int                    kind;                /**< FIELD_MAP_MEG or FIELD_MAP_EEG. */
    MNELIB::MNESurface    *surf;                /**< Surface on which we are mapping. */
    char                  *surfname;            /**< File that the surface was loaded from. */
    int                   *surface_sel;         /**< Subset of vertices for interpolation. */
    int                    nsurface_sel;        /**< Number of points in the subset. */
    int                    nsmooth;             /**< Number of smooth steps after interpolation. */
    float                **smooth_weights;      /**< Smoothing weights. */
    int                    nch;                 /**< Number of channels. */
    int                   *pick;                /**< Channel indices of this modality. */
    FWDLIB::FwdCoilSet    *coils;               /**< Coil definitions. */
    float                  origin[3];           /**< Sphere-model origin. */
    float                  miss;                /**< Unexplained variance. */
    float                **self_dots;           /**< Dot products between original leads. */
    float                **surface_dots;        /**< Dot products from original to virtual leads. */
    float                  intrad;              /**< Integration radius used. */
    MNELIB::MNECovMatrix  *noise;               /**< Noise-covariance matrix. */
    int                    nest;                /**< Number of singular values included. */
    float                **mapping_mat;         /**< The mapping matrix. */
} *fieldMappingData, fieldMappingDataRec;

/**
 * @brief Contour map values computed on a surface for MEG or EEG field visualisation.
 */
typedef struct {
    int    kind;        /**< FIELD_MAP_MEG or FIELD_MAP_EEG. */
    float *map;         /**< Contour map values at surface vertices. */
    int    nmap;        /**< Number of values. */
    int    show;        /**< Should this map be displayed. */
    float  step;        /**< Contour step. */
} *contourMap, contourMapRec;

/**
 * @brief HPI measurement result storing coil positions, goodness-of-fit, and head-to-device transformation.
 */
typedef struct {
    char                          *filename;        /**< Source file name. */
    FIFFLIB::FiffCoordTransSet    *meg_head_t;      /**< MEG device <-> head transformation. */
    FIFFLIB::FiffDigPoint         *hpi_coils;       /**< HPI coil locations in MEG device coordinates. */
    int                            ncoil;           /**< Number of coils. */
    int                           *dig_order;       /**< Digitised-to-physical coil mapping. */
    int                           *coils_used;      /**< Which coils were used. */
    int                            nused;           /**< How many were used. */
    float                         *g_values;        /**< Goodness-of-fit per coil. */
    float                          g_limit;         /**< Goodness-of-fit acceptance threshold. */
    float                          dist_limit;      /**< Distance acceptance threshold. */
    int                            fit_accept;      /**< Was the fit accepted. */
} *hpiResultData, hpiResultDataRec;

/**
 * @brief Single saved timecourse extracted from source data or overlay, with label, vertex selection, and waveform.
 */
typedef struct {
    char          *file_name;       /**< Associated file name. */
    char          *comment;         /**< Descriptive comment. */
    int            source;          /**< Data or overlay origin. */
    int            quantity;        /**< What quantity this represents. */
    int            fixed_ori;       /**< Fixed orientations? */
    int            active;          /**< Is this shown. */
    mshLabel       label;           /**< Label for this timecourse. */
    float        **timecourses;     /**< Timecourses for the points in label. */
    int           *sel;             /**< Vertex numbers in the complete triangulation. */
    float        **rr;              /**< Vertex locations. */
    int            coord_frame;     /**< Coordinate frame. */
    int            nsel;            /**< Number of selected vertices. */
    int            label_task;      /**< Reduction applied to the label. */
    float         *timecourse_task; /**< Reduced waveform. */
    int            is_signed;       /**< Does this contain positive and negative data. */
    int            np;              /**< Number of data points. */
    float          tmin;            /**< Starting time. */
    float          tstep;           /**< Time between points. */
    float          color[3];        /**< Drawing colour. */
} *timecourseData, timecourseDataRec;

/**
 * @brief Collection of saved timecourses for batch display and analysis.
 */
typedef struct {
    timecourseData *timecourses;    /**< Array of timecourse structures. */
    int             ncourse;        /**< Number of timecourses. */
} *timecourseDataSet, timecourseDataSetRec;

/**
 * @brief Persistent state across data set changes.
 *
 * Stores the stimulus-channel index, picked time, scale settings, and
 * MNE preferences that must survive when the active data set changes.
 */
typedef struct {
    int        sample;              /**< Sample channel index. */
    float      picked_time;         /**< Current time point. */
    int        have_picked;         /**< Has a time been picked. */
    mshScales  scales;              /**< Scale settings. */
    mnePref    mne_prefs;           /**< MNE preferences. */
} *dataSetData, dataSetDataRec;

/**
 * @brief Combined MEG/EEG measurement data with inverse operator, field mapping, dipole fitting, and display state.
 *
 * This is the central "workspace" structure that ties together raw/averaged
 * measurement data, the inverse operator, field-mapping engines, dipole-fit
 * state, digitiser points, display scales, and current viewer state.
 */
typedef struct mshMegEegDataRec {
    char                              *meas_file;           /**< Measurement file path. */
    char                              *inv_file;            /**< Inverse operator file path. */
    char                              *mri_trans_file;      /**< MRI transform source file. */
    int                                nave;                /**< nave < 0 => use nave from measurement. */
    MNELIB::MNEMeasData               *meas;                /**< Measurement data. */
    float                              raw_tmin, raw_tmax;  /**< Time range for raw data segments. */
    int                                sample;              /**< Sample channel index. */
    int                                firstp;              /**< First sample in current selection. */
    int                                np;                  /**< Number of data points in selection. */
    float                              tmin, tmax;          /**< Corresponding time values. */
    float                              picked_time;         /**< Current time point. */
    int                                have_picked;         /**< Has a time been picked. */
    double                             abs_picked_time;     /**< Absolute time from raw-data start. */
    int                               *bads;                /**< Bad-channel flags. */
    int                               *sels;                /**< Channels selected for dipole fitting. */
    char                              *selname;             /**< Current channel selection name. */
    std::unique_ptr<MNELIB::MNELayout> lout;                /**< Channel layout. */
    mshScales                          scales;              /**< Time and vertical scale and baseline. */

    float                             *custom_data;         /**< Custom data replacing picked responses. */
    char                              *custom_desc;         /**< Description of the custom data. */

    mnePref                            mne_prefs;           /**< MNE calculation preferences. */
    float                             *cur_vals;            /**< Current source values. */
    FIFFLIB::FiffSparseMatrix         *nn_vals;             /**< Noise normalisation values. */
    MNELIB::MNEMshColorScaleDef        scale;               /**< Active display colour scale. */

    FIFFLIB::FiffDigitizerData        *dig;                 /**< Polhemus digitiser data. */

    fieldMappingData                   meg_mapping;         /**< MEG field interpolation (helmet). */
    fieldMappingData                   meg_mapping_head;    /**< MEG field interpolation (scalp). */
    fieldMappingData                   eeg_mapping;         /**< EEG field interpolation (scalp). */
    fieldMappingPref                   mapping_pref;        /**< Field-mapping preferences. */

    void                              *dipole_fit_setup;    /**< Dipole fitting data (opaque). */
    MNELIB::mneUserFreeFunc            dipole_fit_setup_free;

    void                              *user_data;           /**< User-defined payload. */
    MNELIB::mneUserFreeFunc            user_data_free;      /**< Destructor for user_data. */

    /**
     * @brief Check whether a channel is selected in the measurement data.
     *
     * Refactored from: is_selected_in_data (dipole_fit_setup.c)
     *
     * @param[in] ch_name   Channel name to look up (case-insensitive).
     * @return true if the channel is found and selected.
     */
    bool isChannelSelected(const QString& ch_name) const
    {
        for (int k = 0; k < meas->nchan; k++)
            if (QString::compare(ch_name, meas->chs[k].ch_name, Qt::CaseInsensitive) == 0)
                return sels[k];
        return false;
    }
} *mshMegEegData, mshMegEegDataRec;

/**
 * Callback type for colour-editor completion.
 */
typedef void (*colorEditorDoneFunc)(float *color, void *user);

/**
 * @brief Single continuous HPI data point with quaternion orientation, translation, and goodness-of-fit.
 */
typedef struct {
    float                  time;        /**< Time point. */
    int                    samp;        /**< Corresponding sample number. */
    float                  quater[4];   /**< Unit quaternion orientation. */
    float                  move[3];     /**< Translation. */
    float                  good;        /**< Geometric mean of goodness-of-fit values. */
    FIFFLIB::FiffCoordTrans *t;         /**< The corresponding coordinate transformation. */
} *contHpiData, contHpiDataRec;

/**
 * @brief Time series of continuous HPI measurements with coil movement and angular velocity tracking.
 */
typedef struct {
    contHpiData *hpi;               /**< Data records. */
    int          nhpi;              /**< Number of records. */
    int          omit_samp;         /**< Samples omitted from the beginning (first_samp + initial skip). */
    float        sfreq;            /**< Sampling frequency of the data file. */
    int          current;           /**< Index of the current point of interest. */
    float        max_coil_move;     /**< Average coil movement scale. */
    float        max_velocity;      /**< Angular velocity scale. */
} *contHpiDataSet, contHpiDataSetRec;

} // NAMESPACE INVLIB

#endif // INV_ANALYZE_TYPES_H
