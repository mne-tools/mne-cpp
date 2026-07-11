# MNE-CPP ↔ MNE-Python Gap Analysis (machine-rendered)

> **Do not hand-edit.** This is rendered from the qualitative parity data in `doc/api_registry.json` (`classes` + `parity`). To change a verdict, edit the registry and rerun `python3 tools/parity/gap_analysis.py`.

- Generated: **2026-07-11**
- MNE-Python reference: **1.11.0** (pinned 1.11.x)
- Source of truth: `doc/api_registry.json`

## Summary

- **Total public MNE-Python APIs inventoried:** 443
- Implemented: **251**
- Partial: **47**
- Missing: **137**
- Not-applicable: **8**
- **Parity** (implemented + ½·partial, excluding not-applicable): **63.1%** of 435 in-scope APIs

Every parity figure exposes its denominator (in-scope = implemented + partial + missing; not-applicable excluded) and the pinned reference version.

## Per-domain status

| Domain | Implemented | Partial | Missing | N/A | Total |
|---|---:|---:|---:|---:|---:|
| I/O & Readers | 22 | 3 | 34 | 0 | 59 |
| Core Data Containers | 48 | 8 | 2 | 3 | 61 |
| Preprocessing & Artifacts | 18 | 1 | 23 | 1 | 43 |
| Channels & Montages | 23 | 4 | 14 | 1 | 42 |
| Epochs & Evoked | 13 | 0 | 3 | 0 | 16 |
| Covariance & Whitening | 7 | 0 | 0 | 0 | 7 |
| Forward Modelling | 27 | 3 | 5 | 0 | 35 |
| Inverse & Source Estimation | 35 | 9 | 16 | 0 | 60 |
| Source Space & Morphing | 19 | 4 | 4 | 0 | 27 |
| Time-Frequency | 25 | 9 | 7 | 0 | 41 |
| Decoding & Machine Learning | 6 | 5 | 11 | 2 | 24 |
| Statistics | 8 | 1 | 8 | 0 | 17 |
| Simulation | 0 | 0 | 10 | 0 | 10 |
| Visualisation | 0 | 0 | 0 | 1 | 1 |

## Gaps (missing / partial), grouped by domain


### I/O & Readers

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.export.export_epochs` | partial | EDF/BrainVision export (partial) | epochs export limited |
| `mne.export.export_evokeds` | partial | EDF/BrainVision export (partial) | evoked export limited |
| `mne.export.export_raw` | partial | EDF/BrainVision export (partial) | not all formats |
| `mne.export.export_evokeds_mff` | missing | — | EGI-MFF evoked export not ported |
| `mne.io.read_epochs_eeglab` | missing | — | EEGLAB epochs reader not ported |
| `mne.io.read_epochs_fieldtrip` | missing | — | FieldTrip epochs reader not ported |
| `mne.io.read_epochs_kit` | missing | — | KIT epochs reader not ported |
| `mne.io.read_evoked_besa` | missing | — | BESA evoked reader not ported |
| `mne.io.read_evoked_fieldtrip` | missing | — | FieldTrip evoked reader not ported |
| `mne.io.read_evokeds_mff` | missing | — | EGI-MFF evoked reader not ported |
| `mne.io.read_raw_ant` | missing | — | ANT Neuro reader not ported |
| `mne.io.read_raw_artemis123` | missing | — | Artemis123 reader not ported |
| `mne.io.read_raw_boxy` | missing | — | BOXY fNIRS reader not ported |
| `mne.io.read_raw_bti` | missing | — | BTi/4D reader not ported (mne_insert_4D_comp exists) |
| `mne.io.read_raw_cnt` | missing | — | Neuroscan CNT reader not ported |
| `mne.io.read_raw_curry` | missing | — | Curry reader not ported |
| `mne.io.read_raw_eeglab` | missing | — | EEGLAB .set reader not ported |
| `mne.io.read_raw_egi` | missing | — | EGI-MFF reader not ported |
| `mne.io.read_raw_eyelink` | missing | — | EyeLink eye-tracking reader not ported |
| `mne.io.read_raw_fieldtrip` | missing | — | FieldTrip reader not ported |
| `mne.io.read_raw_fil` | missing | — | FIL OPM reader not ported |
| `mne.io.read_raw_gdf` | missing | — | GDF reader not ported |
| `mne.io.read_raw_hitachi` | missing | — | Hitachi fNIRS reader not ported |
| `mne.io.read_raw_nedf` | missing | — | NEDF reader not ported |
| `mne.io.read_raw_neuralynx` | missing | — | Neuralynx reader not ported |
| `mne.io.read_raw_nicolet` | missing | — | Nicolet reader not ported |
| `mne.io.read_raw_nihon` | missing | — | Nihon Kohden reader not ported |
| `mne.io.read_raw_nirx` | missing | — | NIRx fNIRS reader not ported |
| `mne.io.read_raw_nsx` | missing | — | Blackrock NSx reader not ported |
| `mne.io.read_raw_persyst` | missing | — | Persyst reader not ported |
| `mne.io.read_raw_snirf` | missing | — | SNIRF fNIRS reader not ported |
| `mne.read_epochs_eeglab` | missing | — | EEGLAB epochs reader not ported |
| `mne.read_epochs_fieldtrip` | missing | — | FieldTrip epochs reader not ported |
| `mne.read_epochs_kit` | missing | — | KIT epochs reader not ported |
| `mne.read_evoked_besa` | missing | — | BESA evoked reader not ported |
| `mne.read_evoked_fieldtrip` | missing | — | FieldTrip evoked reader not ported |
| `mne.read_evokeds_mff` | missing | — | EGI-MFF evoked reader not ported |

### Core Data Containers

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.get_volume_labels_from_aseg` | partial | MRILIB aseg (partial) |  |
| `mne.grow_labels` | partial | FSLIB (partial) | label grow present, split limited |
| `mne.head_to_mni` | partial | FSLIB (partial) | MNI transform limited |
| `mne.labels_to_stc` | partial | FSLIB (partial) | label->stc limited |
| `mne.read_talxfm` | partial | FSLIB (partial) | Talairach xfm limited |
| `mne.split_label` | partial | FSLIB (partial) | split limited |
| `mne.stc_to_label` | partial | FSLIB (partial) | stc->label limited |
| `mne.vertex_to_mni` | partial | FSLIB (partial) | MNI mapping limited |
| `mne.random_parcellation` | missing | — | random parcellation not ported |
| `mne.read_lta` | missing | — | FreeSurfer LTA transform reader not ported |

### Preprocessing & Artifacts

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.preprocessing.maxwell_filter` | partial | SSS/tSSS (DSPLIB) | no movement compensation |
| `mne.preprocessing.EOGRegression` | missing | — | EOG regression not ported |
| `mne.preprocessing.annotate_amplitude` | missing | — | amplitude annotation not ported |
| `mne.preprocessing.annotate_break` | missing | — | break annotation not ported |
| `mne.preprocessing.annotate_movement` | missing | — | movement annotation not ported |
| `mne.preprocessing.annotate_muscle_zscore` | missing | — | muscle annotation not ported |
| `mne.preprocessing.annotate_nan` | missing | — | NaN annotation not ported |
| `mne.preprocessing.apply_pca_obs` | missing | — | PCA-OBS cardiac removal not ported |
| `mne.preprocessing.compute_average_dev_head_t` | missing | — | average device->head transform not ported |
| `mne.preprocessing.compute_current_source_density` | missing | SurfaceLaplacian | surface Laplacian/CSD not ported |
| `mne.preprocessing.compute_fine_calibration` | missing | — | fine calibration not ported |
| `mne.preprocessing.compute_proj_hfc` | missing | — | homogeneous field correction not ported |
| `mne.preprocessing.corrmap` | missing | — | ICA corrmap not ported |
| `mne.preprocessing.cortical_signal_suppression` | missing | — | CSS not ported |
| `mne.preprocessing.find_bad_channels_maxwell` | missing | BadChannelsMaxwell | Maxwell-basis bad-ch detection |
| `mne.preprocessing.interpolate_bridged_electrodes` | missing | — | bridged electrode repair not ported |
| `mne.preprocessing.maxwell_filter_prepare_emptyroom` | missing | — | empty-room Maxwell prep not ported |
| `mne.preprocessing.oversampled_temporal_projection` | missing | — | OTP not ported |
| `mne.preprocessing.read_eog_regression` | missing | — | EOG regression I/O not ported |
| `mne.preprocessing.read_ica` | missing | — | ICA solution I/O not ported |
| `mne.preprocessing.read_ica_eeglab` | missing | — | EEGLAB ICA reader not ported |
| `mne.preprocessing.realign_raw` | missing | — | raw realignment not ported |
| `mne.preprocessing.regress_artifact` | missing | EogRegression | EOG regression not ported |
| `mne.preprocessing.write_fine_calibration` | missing | — | fine-calibration I/O not ported |

### Channels & Montages

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.channels.DigMontage` | partial | FIFFLIB dig (partial) | montage container limited |
| `mne.channels.make_dig_montage` | partial | FIFFLIB dig (partial) | montage builder limited |
| `mne.channels.read_ch_adjacency` | partial | StatsAdjacency (partial) |  |
| `mne.channels.read_custom_montage` | partial | FIFFLIB dig (partial) | custom montage limited |
| `mne.channels.compute_native_head_t` | missing | — | native->head transform not ported |
| `mne.channels.get_builtin_ch_adjacencies` | missing | — | builtin adjacency database not ported |
| `mne.channels.get_builtin_montages` | missing | — | builtin montage database not ported |
| `mne.channels.make_1020_channel_selections` | missing | — | 10-20 channel selection groups not ported |
| `mne.channels.make_standard_montage` | missing | StandardMontage | standard 10-20/10-10/10-05 montages |
| `mne.channels.read_dig_captrak` | missing | — | CapTrak dig reader not ported |
| `mne.channels.read_dig_curry` | missing | — | Curry dig reader not ported |
| `mne.channels.read_dig_dat` | missing | — | Neuroscan .dat dig reader not ported |
| `mne.channels.read_dig_egi` | missing | — | EGI dig reader not ported |
| `mne.channels.read_dig_localite` | missing | — | Localite dig reader not ported |
| `mne.scale_bem` | missing | — | BEM scaling not ported |
| `mne.scale_labels` | missing | — | label scaling not ported |
| `mne.scale_mri` | missing | — | MRI scaling coregistration not ported |
| `mne.scale_source_space` | missing | — | source space scaling not ported |

### Epochs & Evoked

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.AcqParserFIF` | missing | — | Elekta acquisition-parameter parser not ported |
| `mne.make_fixed_length_epochs` | missing | — | fixed-length epochs helper |
| `mne.make_fixed_length_events` | missing | — | fixed-length events helper |

### Forward Modelling

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.forward.make_forward_dipole` | partial | ComputeFwd (partial) | dipole forward limited |
| `mne.forward.restrict_forward_to_label` | partial | MNEForwardSolution (partial) | label restriction limited |
| `mne.make_forward_dipole` | partial | ComputeFwd (partial) | dipole forward limited |
| `mne.forward.compute_depth_prior` | missing | — | depth weighting not ported |
| `mne.forward.compute_orient_prior` | missing | — | orientation prior not ported |
| `mne.forward.make_field_map` | missing | — | field map interpolation (viz overlay planned) |
| `mne.forward.restrict_forward_to_stc` | missing | — | stc restriction not ported |
| `mne.make_field_map` | missing | — | field map interpolation (viz overlay planned) |

### Inverse & Source Estimation

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.DipoleFixed` | partial | ECD (partial) | fixed-dipole time course limited |
| `mne.VolSourceEstimate` | partial | MNESourceEstimate (partial) | volume STC limited |
| `mne.beamformer.apply_dics_epochs` | partial | Dics (partial) | epochs DICS |
| `mne.beamformer.apply_lcmv_cov` | partial | Lcmv (partial) | covariance source power |
| `mne.beamformer.read_beamformer` | partial | Lcmv/Dics (partial) | beamformer I/O limited |
| `mne.minimum_norm.compute_source_psd` | partial | Welch PSD + inverse (manual) | no single convenience API |
| `mne.spatial_dist_adjacency` | partial | StatsAdjacency (partial) | distance-based adjacency limited |
| `mne.spatial_inter_hemi_adjacency` | partial | StatsAdjacency (partial) | inter-hemi adjacency limited |
| `mne.spatio_temporal_dist_adjacency` | partial | StatsAdjacency (partial) | distance adjacency limited |
| `mne.MixedSourceEstimate` | missing | — | mixed STC not ported |
| `mne.MixedVectorSourceEstimate` | missing | — | mixed vector STC not ported |
| `mne.VolVectorSourceEstimate` | missing | — | volume vector STC not ported |
| `mne.beamformer.apply_dics_tfr_epochs` | missing | — | TFR-epochs DICS not ported |
| `mne.beamformer.make_lcmv_resolution_matrix` | missing | — | LCMV resolution matrix not ported |
| `mne.inverse_sparse.tf_mixed_norm` | missing | InvTfMxne | TF-MxNE not yet ported |
| `mne.minimum_norm.apply_inverse_cov` | missing | — | covariance source power convenience |
| `mne.minimum_norm.apply_inverse_tfr_epochs` | missing | — | TFR-epochs inverse convenience not ported |
| `mne.minimum_norm.compute_source_psd_epochs` | missing | — | epochs source PSD convenience |
| `mne.minimum_norm.estimate_snr` | missing | — | SNR estimation not ported |
| `mne.minimum_norm.get_cross_talk` | missing | — | CTF resolution metric not ported |
| `mne.minimum_norm.get_point_spread` | missing | — | PSF resolution metric not ported |
| `mne.minimum_norm.resolution_metrics` | missing | — | resolution metrics not ported |
| `mne.minimum_norm.source_band_induced_power` | missing | — | banded source power convenience |
| `mne.minimum_norm.source_induced_power` | missing | — | source induced power convenience |
| `mne.stc_near_sensors` | missing | — | sensor-space stc projection not ported |

### Source Space & Morphing

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.compute_source_morph` | partial | MNEMorphMap (partial) | surface morph; no unified vol morph |
| `mne.get_volume_labels_from_src` | partial | MRILIB aseg (partial) |  |
| `mne.morph_source_spaces` | partial | MNEMorphMap (partial) |  |
| `mne.read_source_morph` | partial | MNEMorphMap (partial) | surface morph only |
| `mne.add_source_space_distances` | missing | — | geodesic src distances not ported |
| `mne.dig_mri_distances` | missing | — | dig<->MRI distance QA not ported |
| `mne.source_space.add_source_space_distances` | missing | — | geodesic src distances not ported |
| `mne.source_space.compute_distance_to_sensors` | missing | — | src-sensor distance not ported |

### Time-Frequency

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.time_frequency.EpochsSpectrum` | partial | Spectral (partial) | epochs spectrum container |
| `mne.time_frequency.EpochsSpectrumArray` | partial | Spectral (partial) |  |
| `mne.time_frequency.combine_spectrum` | partial | Spectral (partial) |  |
| `mne.time_frequency.combine_tfr` | partial | TFR (partial) |  |
| `mne.time_frequency.csd_tfr` | partial | CrossSpectralDensity (partial) |  |
| `mne.time_frequency.read_csd` | partial | CrossSpectralDensity (partial) | CSD I/O limited |
| `mne.time_frequency.read_spectrum` | partial | Spectral (partial) | spectrum I/O limited |
| `mne.time_frequency.read_tfrs` | partial | TFR (partial) | TFR I/O limited |
| `mne.time_frequency.write_tfrs` | partial | TFR (partial) | TFR I/O limited |
| `mne.time_frequency.fit_iir_model_raw` | missing | — | AR/IIR model PSD not ported |
| `mne.time_frequency.fwhm` | missing | — | wavelet FWHM helper not ported |
| `mne.time_frequency.istft` | missing | — | inverse STFT not ported |
| `mne.time_frequency.tfr_array_multitaper` | missing | — | multitaper TFR not ported |
| `mne.time_frequency.tfr_array_stockwell` | missing | — | Stockwell TFR not ported |
| `mne.time_frequency.tfr_multitaper` | missing | MultitaperTfr | multitaper TFR (PSD multitaper exists) |
| `mne.time_frequency.tfr_stockwell` | missing | — | Stockwell TFR not ported |

### Decoding & Machine Learning

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.decoding.FilterEstimator` | partial | DSPLIB filters (partial) | no sklearn wrapper |
| `mne.decoding.LinearModel` | partial | MlLinearModel (partial) | predict-only container |
| `mne.decoding.PSDEstimator` | partial | Spectral (partial) | no sklearn wrapper |
| `mne.decoding.TemporalFilter` | partial | DSPLIB filters (partial) | no sklearn wrapper |
| `mne.decoding.TimeFrequency` | partial | DSPLIB TFR (partial) | no sklearn wrapper |
| `mne.decoding.EMS` | missing | — | eigenvector method |
| `mne.decoding.GeneralizingEstimator` | missing | — | temporal generalization wrapper |
| `mne.decoding.ReceptiveField` | missing | — | encoding/decoding TRF |
| `mne.decoding.SlidingEstimator` | missing | — | time-resolved decoding wrapper |
| `mne.decoding.TimeDelayingRidge` | missing | — | time-delaying ridge (TRF) not ported |
| `mne.decoding.UnsupervisedSpatialFilter` | missing | — | sklearn PCA/ICA wrapper |
| `mne.decoding.Vectorizer` | missing | — | flatten wrapper not ported |
| `mne.decoding.compute_ems` | missing | — | EMS not ported |
| `mne.decoding.cross_val_multiscore` | missing | — | cross-validation helper |
| `mne.decoding.get_coef` | missing | — | coefficient extraction helper |
| `mne.decoding.get_spatial_filter_from_estimator` | missing | — | sklearn helper not ported |

### Statistics

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.stats.combine_adjacency` | partial | StatsAdjacency (partial) |  |
| `mne.stats.bonferroni_correction` | missing | — | Bonferroni correction not ported |
| `mne.stats.bootstrap_confidence_interval` | missing | — | bootstrap CI not ported |
| `mne.stats.f_mway_rm` | missing | — | RM-ANOVA not ported |
| `mne.stats.f_threshold_mway_rm` | missing | — | RM-ANOVA threshold not ported |
| `mne.stats.fdr_correction` | missing | — | FDR correction not ported |
| `mne.stats.linear_regression` | missing | — | channel-wise regression not ported |
| `mne.stats.linear_regression_raw` | missing | — | raw linear regression (rERP) not ported |
| `mne.stats.summarize_clusters_stc` | missing | — | cluster summary stc not ported |

### Simulation

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.simulation.SourceSimulator` | missing | — | source simulator not ported |
| `mne.simulation.add_chpi` | missing | — | cHPI injection not ported |
| `mne.simulation.add_ecg` | missing | — | ECG injection not ported |
| `mne.simulation.add_eog` | missing | — | EOG injection not ported |
| `mne.simulation.add_noise` | missing | — | noise injection not ported |
| `mne.simulation.select_source_in_label` | missing | — | source selection in label not ported |
| `mne.simulation.simulate_evoked` | missing | — | evoked simulation not ported |
| `mne.simulation.simulate_raw` | missing | — | raw simulation not ported |
| `mne.simulation.simulate_sparse_stc` | missing | — | sparse STC simulation not ported |
| `mne.simulation.simulate_stc` | missing | — | STC simulation not ported |

## Already implemented (do not re-implement)

251 MNE-Python APIs already have an MNE-CPP equivalent. See `mne-python-gap.json` (`status == "implemented"`) for the full mapping. TASK 8 candidates must not target any API listed there (AC-T8.0-3).

