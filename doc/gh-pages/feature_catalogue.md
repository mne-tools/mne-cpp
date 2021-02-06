---
title: Feature Catalogue
parent: Home
nav_order: 7
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

### MNE Analyze

### MNE Anonymize

## Libraries

### Utils

### Fs

### Fiff

### Mne

### Fwd

### Inverse 

### Communication

### RtProcessing

### Connectivity

### Disp

### Disp3D
