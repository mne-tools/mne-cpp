---
title: Feature Catalogue
parent: Home
nav_order: 6
---
# Feature Catalogue

## Applications

### MNE Scan

* Connect to the following hardware devices and receive data in real-time: MEGIN MEG (via Fieldtrip Buffer), BabyMEG, Natus, BrainAmp, EegoSports, gUSBAmp, TMSI EEG Refa, BrainFlow, LSL.

* Stream pre-recorded FIFF data and pipe the data to subsequent connected processing steps.

* Visualize incoming data stream in real-time. The visualization provides controls to scale channels, show incoming events as color coded vertical lines, change the amount of data being visualized (max. 30 secs), signal as well as display color, the number of channels and time spacers. The signal visualization features a channel selection mode where groups of channels can be defined and selected.

* Event/Trigger based averaging and visualiazion of the results as butterfly and 2D layout plot. The average visualization features a channel selection mode where groups of channels can be defined and selected.

* Covariance estimation. The user can control the amount of data used to compute the covariance matrix. The covariance matrix is visualized as 2D image based on a colormap.

* Temporal filtering of the data. A FIR filter can be designed via setting the lower and higer cut-off frequency of the filter. The designed filter response in the frequency domain is visualized. 

* Signal Space Projection (SSP) components can be individually activated or deactivated and applied to the data.

* In case of the BabyMEG synthetic gradiometers (compensators) can be applied on the incoming data. 

* Raw data stream coming from acquisition or the filter plugins. Currently writing to FIFF file format is supported.

* Estimate HPIs and head movements based on real-time MEG data. The head position can be estimated in real-time via continous HPI fitting and is visualized relative to the sensor helmet in 3D. Estimating if a large movement occured.

* Recomputing the forward solution in real-time based on continous HPI fitting. Is only triggered when a large head movement occurs.

* Real-time MNE source localization based on evoked responses (averaged data) or raw (single-trial) data blocks. The result is visualized on a cortical mantle in 3D. 

* Real-time functional connectivity estimation. The resulting network can be visualized in 3D. Incoming changes to the network activity are reflected on the fly.

### MNE Analyze

* Load Fiff data.

* Browse through the loaded data. The visualization provides controls to scale channels, show incoming events as color coded vertical lines, change the amount of data being visualized, signal as well as display color, the number of channels and time spacers. The signal visualization features a channel selection mode where groups of channels can be defined and selected.

* Mark, load and save events.

* Average data based on marked or loaded events. Visualize the data as butterfly and 2D lyout plot.

* Perform Co-Registration between the digitized/tracked HPI points and the Freesurfer reconstructed surfaces.

### MNE Anonymize

* Substitute/Anonymize different Personal Health Information (PHI) and Personal Identifiable Information (PII) fields from a FIFF (Functional Imaging File Format) file (.fif) with other values.

## Libraries

### Utils

* Basic mathematical routines (MNEMath).

* Compute the tapered spectral representations (FFT, CSD, PSD) of a signal (Spectral).

* Compute a spectogram of a signal (Spectogram).

* Compute a Thin Palte Spline warp of two 3D surfaces (Warp). This can be used to warp tempalte BEM models to, e.g., digitizer points.

* Fit a sphere to a given data set (Sphere).

* Perform fits via the simplex solver (SimplexAlgorithm).

* Load and save MNE .sel selection group files (SelectionIO).

* Create 2D layout from 3D layouts (LayoutMaker).

* Load a 2D layout from ANT .elc and MNE .lout files (LayoutLoader).

* K-Means clustering for a given data set (KMeans).

* Input and output operations for binary data streams. Read and write Eigen matrixes to .txt files (IOUtils).

* Provide an implementaion of a circular buffer able to any kind of data (CircularBuffer).

* Basic design patterns such as the command pattern and the observer pattern.

* Application logger based on the Qt QDebug class (ApplicationLogger).

### Fs

* Load and save Freesurfer generated surfaces and annotations.

### Fiff

* Load and save Fiff files.

### Mne

* Data structures conform with MNE-C structs: BemSurface, Bem, EpochData, EpochDataList, ForwardSolution, Hemisphere, InverseOperator, ProjectToSurface,SourceEstimate, SourceSpace, Surface.

### Fwd

* Routines to generate a forward solution based on Freesurfer reconstructions for MEG/EEG source localization.

### Inverse 

* Methods for single dipole fitting.

* Methods for HPI fitting.

* Methods for MNE source localization.

* Methods for RAP-MUSIC source localization. A GPU CUDA implementation is available as well.

### Communication

* Client and command specifications to communicate with a running mne_rt_server.

### RtProcessing

* Tools for general processing of electrophysiological data: averaging, filtering, iterative closest point, trigger detection, SPHARA

* Classes to guarantee asynchronous processing of averaging, functional connectivity, HPI fitting, inverse operator creation, noise estimation. These classes provide means to outsource the actual computation, which are implemented elsewhere (and in different libraries), to different threads. Communication of requests and repsonses is handled via Qt's signal/slot system.

### Connectivity

* Network data storage capable of holding nodes and edges. The container ca also be used to generate basic graph measures (indegree, cluster, thresholding, etc.) and generate a connectivity matrix.

* Compute functional connectivity via the following methods: Correlation, Cross Correlation, Coherence, Imaginary Coherence, Phase Locking Value, Phase Lag Index, Weighted Phase Lag Index, Unbiased Squared Phase Lag Index, Debiased Squared Weighted Phase Lag Index.

### Disp

* Following plots are provided: Bar, graph, imagesc, lineplot, spline, tfplot.

* A number of viewers which come with a graphical user interface and are ready to be used in a QWidget based GUI application. 

### Disp3D

* Provides a model, delegate and view to visualize data in 3D.

* Supported data are: Source estimate, functional connectivity networks, digitzers, BEM models, Freesurfer surfaces, 3D topoplots for MEG/EEG, sensor information, source space information