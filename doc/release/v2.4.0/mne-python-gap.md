# MNE-CPP ↔ MNE-Python Gap Analysis (machine-rendered)

> **Do not hand-edit.** This is rendered from the qualitative parity data in `doc/api_registry.json` (`classes` + `parity`). To change a verdict, edit the registry and rerun `python3 tools/parity/gap_analysis.py`.

- MNE-Python reference: **1.11.0** (pinned 1.11.x)
- Source of truth: `doc/api_registry.json`

## Summary

- **Total public MNE-Python APIs inventoried:** 443
- Implemented: **191**
- Partial: **84**
- Missing: **160**
- Not-applicable: **8**
- **Parity** (implemented + ½·partial, excluding not-applicable): **53.6%** of 435 in-scope APIs

Every parity figure exposes its denominator (in-scope = implemented + partial + missing; not-applicable excluded) and the pinned reference version.

## Per-domain status

| Domain | Implemented | Partial | Missing | N/A | Total |
|---|---:|---:|---:|---:|---:|
| I/O & Readers | 18 | 2 | 39 | 0 | 59 |
| Core Data Containers | 40 | 13 | 5 | 3 | 61 |
| Preprocessing & Artifacts | 10 | 8 | 24 | 1 | 43 |
| Channels & Montages | 16 | 9 | 16 | 1 | 42 |
| Epochs & Evoked | 12 | 2 | 2 | 0 | 16 |
| Covariance & Whitening | 6 | 1 | 0 | 0 | 7 |
| Forward Modelling | 27 | 5 | 3 | 0 | 35 |
| Inverse & Source Estimation | 29 | 12 | 19 | 0 | 60 |
| Source Space & Morphing | 15 | 5 | 7 | 0 | 27 |
| Time-Frequency | 6 | 20 | 15 | 0 | 41 |
| Decoding & Machine Learning | 4 | 6 | 12 | 2 | 24 |
| Statistics | 8 | 1 | 8 | 0 | 17 |
| Simulation | 0 | 0 | 10 | 0 | 10 |
| Visualisation | 0 | 0 | 0 | 1 | 1 |

## Gaps (missing / partial), grouped by domain


### I/O & Readers

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.io.get_channel_type_constants` | partial | FIFFV_MEG_CH | macros only; no name→constants map |
| `mne.io.read_raw` | partial | BidsRawData::createReader | dispatch for BrainVision/EDF/BDF only |
| `mne.export.export_epochs` | missing | — | only mne_epochs2mat (.mat output) |
| `mne.export.export_evokeds` | missing | — |  |
| `mne.export.export_evokeds_mff` | missing | — | EGI-MFF evoked export not ported |
| `mne.export.export_raw` | missing | — | no EDF/BrainVision/EEGLAB writer |
| `mne.io.match_channel_orders` | missing | — |  |
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
| `mne.match_channel_orders` | missing | — |  |
| `mne.read_epochs_eeglab` | missing | — | EEGLAB epochs reader not ported |
| `mne.read_epochs_fieldtrip` | missing | — | FieldTrip epochs reader not ported |
| `mne.read_epochs_kit` | missing | — | KIT epochs reader not ported |
| `mne.read_evoked_besa` | missing | — | BESA evoked reader not ported |
| `mne.read_evoked_fieldtrip` | missing | — | FieldTrip evoked reader not ported |
| `mne.read_evokeds_mff` | missing | — | EGI-MFF evoked reader not ported |

### Core Data Containers

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.compute_proj_raw` | partial | FIFFLIB::FiffProj::compute_from_raw | event-locked epochs only; no continuous segmenting |
| `mne.compute_rank` | partial | MATHLIB::Linalg::rank | matrix rank; no per-type/info/SSP handling |
| `mne.get_volume_labels_from_aseg` | partial | FSLIB::FsAtlasLookup::load | loads aseg; no listing of present labels |
| `mne.grow_labels` | partial | FsLabelUtils::growLabel | edge-hop count; no mm extent or multiple seeds |
| `mne.head_to_mni` | partial | FSLIB (partial) | MNI transform limited |
| `mne.label_sign_flip` | partial | InvLabelTimeCourse::computeSignFlip | signs from data SVD, not source normals |
| `mne.labels_to_stc` | partial | FsLabelUtils::labelsToStc | binary mask only; no per-label values |
| `mne.pick_types` | partial | FiffInfoBase::pick_types | only meg/eeg/stim flags; no eog/ecg/seeg… |
| `mne.read_freesurfer_lut` | partial | FsAtlasLookup::initLookupTable | hard-coded subset; no LUT file parser |
| `mne.sensitivity_map` | partial | mne_sensitivity_map (CLI) | only norm/svd modes |
| `mne.split_label` | partial | FSLIB (partial) | split limited |
| `mne.vertex_to_mni` | partial | FiffCoordTransSet::headToMni | head points only; no vertex/surface-RAS input |
| `mne.write_label` | partial | writeLabel | tool-private writer; FsLabel has no write |
| `mne.BiHemiLabel` | missing | — | FsLabel is deliberately single-hemisphere (src/libraries/fs/fs_label.h:170) |
| `mne.compute_proj_evoked` | missing | — | mne_cov2proj works on covariance, not evoked |
| `mne.random_parcellation` | missing | — | random parcellation not ported |
| `mne.read_lta` | missing | — | FreeSurfer LTA transform reader not ported |
| `mne.write_labels_to_annot` | missing | — | no .annot writer |

### Preprocessing & Artifacts

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.preprocessing.ICA` | partial | UTILSLIB::ICA::run/excludeComponents, ExtendedInfomax | stateless functions; no info-aware object or save |
| `mne.preprocessing.compute_proj_ecg` | partial | ArtifactDetect::detectEcg, FiffProj::compute_from_raw | detection plus SSP exist; no combined helper |
| `mne.preprocessing.compute_proj_eog` | partial | ArtifactDetect::detectEog, compute_from_raw | detection plus SSP exist; no combined helper |
| `mne.preprocessing.create_ecg_epochs` | partial | detectEcg, EpochExtractor::extract | no combined detect+epoch helper |
| `mne.preprocessing.create_eog_epochs` | partial | detectEog, EpochExtractor::extract | no combined detect+epoch helper |
| `mne.preprocessing.ica_find_ecg_events` | partial | detectEcg | needs a FiffInfo ECG channel; no ICA-source input |
| `mne.preprocessing.ica_find_eog_events` | partial | detectEog | needs a FiffInfo EOG channel; no ICA-source input |
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
| `mne.preprocessing.equalize_bads` | missing | — |  |
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
| `mne.channels.DigMontage` | partial | FIFFLIB::FiffDigPointSet | no channel-name→position montage API |
| `mne.channels.combine_channels` | partial | UTILSLIB::ChannelDerivation::apply | weighted sums only; no median/std |
| `mne.channels.compute_dev_head_t` | partial | MNELIB::fitMatchedPoints | rigid point fit; no montage-HPI extraction |
| `mne.channels.generate_2d_layout` | partial | UTILSLIB::LayoutMaker::makeLayout | projects 3-D points; no direct 2-D input |
| `mne.channels.make_dig_montage` | partial | FiffDigPointSet(QList<FiffDigPoint>) | no name→position mapping |
| `mne.channels.read_ch_adjacency` | partial | StatsAdjacency (partial) |  |
| `mne.channels.read_custom_montage` | partial | UTILSLIB::LayoutLoader::readAsaElcFile | .elc only; no .sfp/.loc/.bvef |
| `mne.channels.read_vectorview_selection` | partial | UTILSLIB::SelectionIO::readMNESelFile | reads .sel; no name filter or space fixing |
| `mne.read_vectorview_selection` | partial | SelectionIO::readMNESelFile | reads .sel; no name filter or space fixing |
| `mne.channels.compute_native_head_t` | missing | — | native->head transform not ported |
| `mne.channels.equalize_channels` | missing | — |  |
| `mne.channels.find_layout` | missing | — | no automatic layout selection |
| `mne.channels.get_builtin_ch_adjacencies` | missing | — | builtin adjacency database not ported |
| `mne.channels.get_builtin_montages` | missing | — | builtin montage database not ported |
| `mne.channels.make_1020_channel_selections` | missing | — | 10-20 channel selection groups not ported |
| `mne.channels.make_grid_layout` | missing | — |  |
| `mne.channels.make_standard_montage` | missing | StandardMontage | standard 10-20/10-10/10-05 montages |
| `mne.channels.read_dig_curry` | missing | — | Curry dig reader not ported |
| `mne.channels.unify_bad_channels` | missing | — |  |
| `mne.equalize_channels` | missing | — |  |
| `mne.find_layout` | missing | — |  |
| `mne.scale_bem` | missing | — | BEM scaling not ported |
| `mne.scale_labels` | missing | — | label scaling not ported |
| `mne.scale_mri` | missing | — | MRI scaling coregistration not ported |
| `mne.scale_source_space` | missing | — | source space scaling not ported |

### Epochs & Evoked

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.BaseEpochs` | partial | MNELIB::MNEEpochDataList | no crop/resample/save/metadata |
| `mne.EpochsArray` | partial | MNELIB::MNEEpochData, FIFFLIB::FiffEpochs::makeFixedLengthEpochs | no info/events-bearing array constructor |
| `mne.AcqParserFIF` | missing | — | Elekta acquisition-parameter parser not ported |
| `mne.make_fixed_length_epochs` | missing | — | fixed-length epochs helper |

### Covariance & Whitening

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.whiten_evoked` | partial | INVLIB::computeWhitener | builds whitener; not applied to evoked |

### Forward Modelling

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.forward.make_forward_dipole` | partial | ComputeFwd (partial) | dipole forward limited |
| `mne.forward.restrict_forward_to_label` | partial | MNEForwardSolution (partial) | label restriction limited |
| `mne.forward.use_coil_def` | partial | FwdCoilSet::read_coil_defs | custom coil file readable; ComputeFwd path hard-coded |
| `mne.make_forward_dipole` | partial | ComputeFwd (partial) | dipole forward limited |
| `mne.use_coil_def` | partial | FwdCoilSet::read_coil_defs | reader exists; ComputeFwd path hard-coded |
| `mne.forward.make_field_map` | missing | — | field map interpolation (viz overlay planned) |
| `mne.forward.restrict_forward_to_stc` | missing | — | stc restriction not ported |
| `mne.make_field_map` | missing | — | field map interpolation (viz overlay planned) |

### Inverse & Source Estimation

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.VolSourceEstimate` | partial | INVLIB::InvVolumeSourceEstimate | no NIfTI export |
| `mne.beamformer.Beamformer` | partial | INVLIB::InvBeamformer | no save/read |
| `mne.beamformer.apply_lcmv_cov` | partial | Lcmv (partial) | covariance source power |
| `mne.grade_to_tris` | partial | generateIcoVertices | ico vertices only (file-private); no triangles returned |
| `mne.inverse_sparse.gamma_map` | partial | InvGammaMap::compute | solver only; no evoked/forward/whitening wrapper |
| `mne.inverse_sparse.mixed_norm` | partial | InvMxne::compute | IRLS solver only; no whitening/depth/debias |
| `mne.minimum_norm.compute_source_psd` | partial | INVLIB::computeSourcePsd | takes an STC; no raw→inverse→PSD pipeline |
| `mne.read_dipole` | partial | InvEcdSet::read_dipoles_dip | .dip only; .bdip is write-only |
| `mne.read_source_estimate` | partial | InvSourceEstimate::read, read_w | .stc/.w only; no .h5 |
| `mne.spatial_dist_adjacency` | partial | StatsAdjacency (partial) | distance-based adjacency limited |
| `mne.spatial_inter_hemi_adjacency` | partial | StatsAdjacency (partial) | inter-hemi adjacency limited |
| `mne.spatio_temporal_dist_adjacency` | partial | StatsAdjacency (partial) | distance adjacency limited |
| `mne.DipoleFixed` | missing | — |  |
| `mne.MixedSourceEstimate` | missing | — | mixed STC not ported |
| `mne.MixedVectorSourceEstimate` | missing | — | mixed vector STC not ported |
| `mne.VolVectorSourceEstimate` | missing | — | volume vector STC not ported |
| `mne.beamformer.apply_dics_tfr_epochs` | missing | — | TFR-epochs DICS not ported |
| `mne.beamformer.make_lcmv_resolution_matrix` | missing | — | LCMV resolution matrix not ported |
| `mne.beamformer.read_beamformer` | missing | — | InvBeamformer has no I/O |
| `mne.inverse_sparse.make_stc_from_dipoles` | missing | — | no dipole-list→STC conversion |
| `mne.inverse_sparse.tf_mixed_norm` | missing | InvTfMxne | TF-MxNE not yet ported |
| `mne.minimum_norm.apply_inverse_cov` | missing | — | covariance source power convenience |
| `mne.minimum_norm.apply_inverse_tfr_epochs` | missing | — | TFR-epochs inverse convenience not ported |
| `mne.minimum_norm.compute_source_psd_epochs` | missing | — | epochs source PSD convenience |
| `mne.minimum_norm.estimate_snr` | missing | MNEMneData | SNR estimation not ported |
| `mne.minimum_norm.get_cross_talk` | missing | — | CTF resolution metric not ported |
| `mne.minimum_norm.get_point_spread` | missing | — | PSF resolution metric not ported |
| `mne.minimum_norm.resolution_metrics` | missing | — | resolution metrics not ported |
| `mne.minimum_norm.source_band_induced_power` | missing | — | banded source power convenience |
| `mne.minimum_norm.source_induced_power` | missing | — | source induced power convenience |
| `mne.stc_near_sensors` | missing | — | sensor-space stc projection not ported |

### Source Space & Morphing

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.compute_source_morph` | partial | INVLIB::SourceMorph::compute, MNEMorphMap::compute | nearest-neighbour surface only; no smoothing/volume |
| `mne.decimate_surface` | partial | MNEBemSurface::makeScalpSurfaces | MNEBemSurface input only; vertex (not triangle) target |
| `mne.get_head_surf` | partial | MNEBem(QIODevice&) | readable via MNEBem; no subject/path lookup |
| `mne.source_space.get_decimated_surfaces` | partial | MNESurfaceOrVolume::use_itris | use_itris stored; no extraction helper |
| `mne.write_surface` | partial | mne_flash_bem, mne_setup_forward_model (tool-private FreeSurfer writers) | tool-private FreeSurfer writers; no FsSurface write |
| `mne.add_source_space_distances` | missing | — | geodesic src distances not ported |
| `mne.dig_mri_distances` | missing | — | dig<->MRI distance QA not ported |
| `mne.get_volume_labels_from_src` | missing | — |  |
| `mne.morph_source_spaces` | missing | — |  |
| `mne.read_source_morph` | missing | — | no .h5 morph reader (only FIF morph maps: MNEMorphMap::read src/libraries/mne/mne_morph_map.h:120) |
| `mne.source_space.add_source_space_distances` | missing | — | geodesic src distances not ported |
| `mne.source_space.compute_distance_to_sensors` | missing | — | src-sensor distance not ported |

### Time-Frequency

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.time_frequency.AverageTFR` | partial | MorletTfrResult, MultitaperTfrResult | result struct only; no info/average/IO |
| `mne.time_frequency.AverageTFRArray` | partial | MorletTfrResult | result struct only |
| `mne.time_frequency.BaseTFR` | partial | MultitaperTfrResult | no common TFR base class |
| `mne.time_frequency.CrossSpectralDensity` | partial | UTILSLIB::CsdResult | no channel names/pick/IO |
| `mne.time_frequency.EpochsSpectrum` | partial | Spectral (partial) | epochs spectrum container |
| `mne.time_frequency.EpochsSpectrumArray` | partial | Spectral (partial) |  |
| `mne.time_frequency.RawTFR` | partial | MorletTfr::computeMultiChannel | result struct only |
| `mne.time_frequency.RawTFRArray` | partial | MorletTfrResult | result struct only |
| `mne.time_frequency.combine_spectrum` | partial | Spectral (partial) |  |
| `mne.time_frequency.csd_array_fourier` | partial | Csd::computeFourier | 2-D input; no averaging over epochs |
| `mne.time_frequency.csd_array_morlet` | partial | Csd::computeMorlet | 2-D input; no averaging over epochs |
| `mne.time_frequency.csd_array_multitaper` | partial | Csd::computeMultitaper | 2-D input; no averaging over epochs |
| `mne.time_frequency.csd_fourier` | partial | Csd::computeFourier | matrix input; no Epochs object |
| `mne.time_frequency.csd_morlet` | partial | Csd::computeMorlet | matrix input; no Epochs object |
| `mne.time_frequency.csd_multitaper` | partial | Csd::computeMultitaper | matrix input; no Epochs object |
| `mne.time_frequency.morlet` | partial | MorletTfr::buildWavelet | wavelet builder is private |
| `mne.time_frequency.read_spectrum` | partial | Spectral (partial) | spectrum I/O limited |
| `mne.time_frequency.stft` | partial | Spectrogram::makeSpectrogram | Gaussian-window magnitude only; no complex output/istft |
| `mne.time_frequency.tfr_array_morlet` | partial | MorletTfr::computeMultiChannel | power only; single epoch; no complex/ITC |
| `mne.time_frequency.tfr_morlet` | partial | MorletTfr::compute | no Epochs input, averaging or ITC |
| `mne.time_frequency.EpochsTFR` | missing | — | no per-epoch TFR container |
| `mne.time_frequency.EpochsTFRArray` | missing | — |  |
| `mne.time_frequency.combine_tfr` | missing | — |  |
| `mne.time_frequency.csd_tfr` | missing | — |  |
| `mne.time_frequency.fit_iir_model_raw` | missing | — | AR/IIR model PSD not ported |
| `mne.time_frequency.fwhm` | missing | — | wavelet FWHM helper not ported |
| `mne.time_frequency.istft` | missing | — | inverse STFT not ported |
| `mne.time_frequency.pick_channels_csd` | missing | — |  |
| `mne.time_frequency.read_csd` | missing | — |  |
| `mne.time_frequency.read_tfrs` | missing | — |  |
| `mne.time_frequency.tfr_array_multitaper` | missing | — | multitaper TFR not ported |
| `mne.time_frequency.tfr_array_stockwell` | missing | — | Stockwell TFR not ported |
| `mne.time_frequency.tfr_multitaper` | missing | MultitaperTfr | multitaper TFR (PSD multitaper exists) |
| `mne.time_frequency.tfr_stockwell` | missing | — | Stockwell TFR not ported |
| `mne.time_frequency.write_tfrs` | missing | — |  |

### Decoding & Machine Learning

| Python API | Status | MNE-CPP | Notes |
|---|---|---|---|
| `mne.decoding.FilterEstimator` | partial | FirFilter::filterData | filters exist; no transformer wrapper |
| `mne.decoding.PSDEstimator` | partial | Spectral (partial) | no sklearn wrapper |
| `mne.decoding.Scaler` | partial | StandardScaler | generic external scaler; no per-channel-type scalings |
| `mne.decoding.SpatialFilter` | partial | DecodingCsp::filters/patterns | no shared base class; each decoder has its own filters/patterns |
| `mne.decoding.TemporalFilter` | partial | FirFilter::filterData | no transformer wrapper |
| `mne.decoding.TimeFrequency` | partial | MorletTfr::computeMultiChannel | no transformer; power only |
| `mne.decoding.EMS` | missing | — | eigenvector method |
| `mne.decoding.GeneralizingEstimator` | missing | — | temporal generalization wrapper |
| `mne.decoding.LinearModel` | missing | — | no coef→patterns (Haufe) wrapper |
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

191 MNE-Python APIs already have an MNE-CPP equivalent. See `mne-python-gap.json` (`status == "implemented"`) for the full mapping. TASK 8 candidates must not target any API listed there (AC-T8.0-3).

