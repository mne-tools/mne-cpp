<!-- ---
title: MNE Forward Solution
parent: Learn
nav_order: 5
---
# MNE Forward Solution

## Command line options 

`mne_forward_solution` recognizes the following command line options:

Input data:
```
--meg             to compute the MEG forward solution
--eeg             to compute the EEG forward solution
--grad            compute the gradient of the field with respect to the dipole coordinates as well
--fixed           to calculate only for the source orientation given by the surface normals
--mricoord        do calculations in MRI coordinates instead of head coordinates
--accurate        use more accurate coil definitions in MEG forward computation
--src name        specify the source space
--label name      label file to select the sources (can have multiple of these)
--mri name        take head/MRI coordinate transform from here (Neuromag MRI description file)
--trans name      take head/MRI coordinate transform from here (text file)
--notrans         head and MRI coordinate systems are identical.
--meas name       take MEG sensor and EEG electrode locations from here
--bem  name       BEM model name
--origin x:y:z/mm use a sphere model with this origin (head coordinates/mm)
--eegscalp        scale the electrode locations to the surface of the scalp when using a sphere model
--eegmodels name  read EEG sphere model specifications from here.
--eegmodel  name  name of the EEG sphere model to use (default : Default)
--eegrad rad/mm   radius of the scalp surface to use in EEG sphere model (default :    90.0 mm)
--mindist dist/mm minimum allowable distance of the sources from the inner skull surface.
--mindistout name Output the omitted source space points here.
--includeall      Omit all source space checks
--all             calculate forward solution in all nodes instead the selected ones only.
--fwd  name       save the solution here
--help            print this info.
--version         print version info.
``` -->
