---
title: MNE Anonymize
has_children: true
parent: Learn
nav_order: 2
---
# MNE Anonymize

## Introduction 

Depending on the settings during acquisition the data files may contain subject identifying information in unencrypted form. The utility `mne_anonymize` was written to clear tags containing such information from a fiff file. Specifically, this utility removes the following tags from the fif file:

| Tag | Description | 
|-----|-------------|
|`FIFF_FILE_ID`, `FIFF_BLOCK_ID`, `FIFF_PARENT_FILE_ID`, `FIFF_PARENT_BLOCK_ID`, `FIFF_REF_FILE_ID`, `FIFF_REF_BLOCK_ID`| The ID tag includes a measurement date and unique machine ID. The machine ID usually contains the hardware address of the primary LAN card. |
|`FIFF_MEAS_DATE`| The date of the measurement. |
|`FIFF_COMMENT` in the FiffInfo block | Holds a (textual) description of the measurement. |
|`FIFF_EXPERIMENTER`| The experimenter's name. |
|`FIFF_SUBJ_ID`| The Subject ID. |
|`FIFF_SUBJ_FIRST_NAME`| The first name of the subject. |
|`FIFF_SUBJ_MIDDLE_NAME`| The middle name of the subject. |
|`FIFF_SUBJ_LAST_NAME`| The last name of the subject. |
|`FIFF_SUBJ_BIRTH_DAY`| The birthday of the subject. |
|`FIFF_SUBJ_SEX`| The sex of the subject. |
|`FIFF_SUBJ_WEIGHT`| The weight of the subject. |
|`FIFF_SUBJ_HEIGHT`| The height of the subject. |
|`FIFF_SUBJ_COMMENT`| Comment about the subject. |
|`FIFF_SUBJ_HIS_ID`| The subject's ID used in the Hospital Information System.|
|`FIFF_PROJ_ID`| The project ID. |
|`FIFF_PROJ_NAME`| The project name. |
|`FIFF_PROJ_AIM`| The project aim. |
|`FIFF_PROJ_PERSONS`| Persons participating in the project. |
|`FIFF_PROJ_COMMENT`| Comment about the project |

| **Please note:** `mne_anonymize` normally keeps the `FIFF_SUBJ_HIS_ID` tag which can be used to identify the subjects uniquely after the information listed in the table above have been removed. If the `--his` option is specified on the command line, the `FIFF_SUBJ_HIS_ID` tag will be removed as well. Therefore, after `mne_anonymize` has processed a data file there is no way to recover the removed information. Use this utility with caution. |

| **Please note:** In case the input fif file contains MRI data, beware that a subject's face can be reconstructed from it. The current implementation of `mne_anonymize` can not anonymize MRI data. |

## Command-line options 

`mne_anonymize` recognizes the following command-line options:

| Option | Description | 
|--------|-------------|
|`--in <infile>`| File to anonymize. Multiple `--in <infile>` statements can be present (files will be processed in parallel).|
|`--out <outfile>` *optional*| Output file `<outfile>`. Only allowed when anonymizing one single file. As default '_anonymized.fif' is attached to the file name. |
|`--verbose` *optional*| Prints out more information, about each specific anonymized field. Only allowed when anonymizing one single file. Default: false |
|`--quiet` *optional*| Show no output. Default: false |
|`--delete_input_file_after` *optional*| Delete input fiff file after anonymization. A confirmation message will be prompted to the user. Default: false |
|`--avoid_delete_confirmation` *optional*| Avoid confirming the deletion of the input fiff file. Default: false|
|`--brute` *optional*| Apart from anonymizing other more usual fields in the fiff file, if present in the input fiff file, anonymize also Subject's weight and height, and Project's ID, Name, Aim and Comment. Default: false |
|`--measurement_date <value>` *optional*| Specify the measurement date. Only when anonymizing a single file. Format: YYYMMDD Default: 20000101. |
|`--measurement_date_offset <value>` *optional*| Specify number of days to subtract to the measurement <date>. Only allowed when anonymizing a single file. Default: 0 |
|`--subject_birthday <value>` *optional*| Specify the subject's birthday <date>. Only allowed when anonymizing a single file. Format: YYYMMDD. Default: 20000101 |
|`--subject_birthday_offset <value>` *optional*| Specify number of <days> to subtract to the subject's birthday. Only allowed when anonymizing a single file. Default: 0 |
|`--his <value>` *optional*| Specify the Subject's ID within the Hospital system. Only allowed when anonymizing a single file. Default: 'mne_anonymize' |

## Examples

The easiest way to anonymize one single file is (results in the output file `sample_audvis_raw_anonymized.fif`):

    $ mne_anonymize --in ./MNE-sample-data/mne/MEG/sample/sample_audvis_raw.fif

Multiple files can be anonymized by typing (results in the output files `sample_audvis_raw_anonymized.fif` and `ernoise_raw_anonymized.fif`):

    $ mne_anonymize --in ./MNE-sample-data/mne/MEG/sample/sample_audvis_raw.fif --in ./MNE-sample-data/mne/MEG/sample/ernoise_raw.fif

Inplace anonymization can be performed via (`--avoid_delete_confirmation` will result in an automatic overwriting/deletion of the input file):

    $ mne_anonymize --in ./MNE-sample-data/mne/MEG/sample/sample_audvis_raw.fif --out ./MNE-sample-data/mne/MEG/sample/sample_audvis_raw.fif --avoid_delete_confirmation

In order to substract 35 days from all measurement dates, both in the ID and `FIFF_MEAS_DATE` tags, use:

    $ mne_anonymize --in ./MNE-sample-data/mne/MEG/sample/sample_audvis_raw.fif --measurement_date_offset 35