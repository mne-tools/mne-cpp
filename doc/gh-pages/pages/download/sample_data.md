---
title: The Sample Data Set
parent: Download
nav_order: 3
---
# Setting Up the Sample Data Set

In order to run MNE-CPP examples out of the box it is necessary to install the MNE-Sample-Data set. You can download the data [here](https://osf.io/86qa2/download). After extracting the data to `mne-cpp/out/MNE-sample-data` the folder structure should look like this: 

```
mne-cpp/
├── out/
|   ├── resources/
|   |   └── MNE-sample-data/
|   |       ├── MEG/
|   |       ├── subjects/
|   |       └── README.md/
|   └── ...
└── ...
```

On MacOS and if you build with the `withAppBundles` flag, you need to also copy the data into the `.app/Contents/MacOS/MNE-sample-data` folder.