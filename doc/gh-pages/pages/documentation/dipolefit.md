<!-- ---
title: MNE Dipole Fit
parent: Documentation
nav_order: 4
---
# MNE Dipole Fit

## Command line options 

`mne_dipole_fit` recognizes the following command line options:

Input data:
```
--meas name       specify an evoked-response data file
--set   no        evoked data set number to use (default: 1)
--bad name        take bad channel list from here
```

Modality selection:
```
--meg             employ MEG data in fitting
--eeg             employ EEG data in fitting
```

Time scale selection:
```
--tmin  time/ms   specify the starting analysis time
--tmax  time/ms   specify the ending analysis time
--tstep time/ms   specify the time step between frames (default 1/(sampling frequency))
--integ time/ms   specify the time integration for each frame (default 0)
```

Preprocessing:
```
--bmin  time/ms   specify the baseline starting time (evoked data only)
--bmax  time/ms   specify the baseline ending time (evoked data only)
--proj name       Load the linear projection from here
                  Multiple projections can be specified.
                  The data file will be automatically included, unless --noproj is present.
--noproj          Do not load the projection from the data file, just those given with the --proj option.
```

Filtering (raw data only):
```
--filtersize size desired filter length (default = 4096)
--highpass val/Hz highpass corner (default =    0.0 Hz)
--lowpass  val/Hz lowpass  corner (default =   40.0 Hz)
--lowpassw val/Hz lowpass transition width (default =    5.0 Hz)
--filteroff       do not filter the data
```

Noise specification:
```
--noise name      take the noise-covariance matrix from here
--gradnoise val   specify a gradiometer noise value in fT/cm
--magnoise val    specify a gradiometer noise value in fT
--eegnoise val    specify an EEG value in uV
                  NOTE: The above will be used only if --noise is missing
--diagnoise       omit off-diagonal terms from the noise-covariance matrix
--reg amount      Apply regularization to the noise-covariance matrix (same fraction for all channels).
--gradreg amount  Apply regularization to the MEG noise-covariance matrix (planar gradiometers, default =   0.10).
--magreg amount   Apply regularization to the EEG noise-covariance matrix (axial gradiometers and magnetometers, default =   0.10).
--eegreg amount   Apply regularization to the EEG noise-covariance matrix (default =   0.10).
```

Forward model:
```
--mri name        take head/MRI coordinate transform from here (Neuromag MRI description file)
--bem  name       BEM model name
--origin x:y:z/mm use a sphere model with this origin (head coordinates/mm)
--eegscalp        scale the electrode locations to the surface of the scalp when using a sphere model
--eegmodels name  read EEG sphere model specifications from here.
--eegmodel  name  name of the EEG sphere model to use (default : Default)
--eegrad val      radius of the scalp surface to use in EEG sphere model (default :    90.0 mm)
--accurate        use accurate coil definitions in MEG forward computation
```

Fitting parameters:
```
--guess name      The source space of initial guesses.
                  If not present, the values below are used to generate the guess grid.
--guesssurf name  Read the inner skull surface from this fif file to generate the guesses.
--guessrad value  Radius of a spherical guess volume if neither of the above is present (default : 80.0 mm)
--exclude dist/mm Exclude points which are closer than this distance from the CM of the inner skull surface (default =    20.0 mm).
--mindist dist/mm Exclude points which are closer than this distance from the inner skull surface  (default =   10.0 mm).
--grid    dist/mm Source space grid size (default =   10.0 mm).
--magdip          Fit magnetic dipoles instead of current dipoles.
```

Output:
```
--dip     name    xfit dip format output file name
--bdip    name    xfit bdip format output file name
```

General:
```
--gui             Enables the gui.
--help            print this info.
--version         print version info.
```

## Examples
```
mne_dipole_fit --meas ../resources/data/MNE-sample-data/MEG/sample/sample_audvis-ave.fif --noise ../resources/data/MNE-sample-data/MEG/sample/sample_audvis-cov.fif --bem ../resources/data/MNE-sample-data/subjects/sample/bem/sample-5120-bem.fif --mri ../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw-trans.fif --set 1 --meg --tmin 32 --tmax 148 --bmin -100 --bmax 0 --gui --dip ../resources/data/MNE-sample-data/Result/dip-result.dat
``` -->