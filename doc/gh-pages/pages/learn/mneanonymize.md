---
title: MNE Anonymize CLI
has_children: true
parent: Learn
nav_order: 3
---
# MNE Anonymize CLI

This application substitutes Personal Health Information and Personal Identifiable information from a FIFF file with some default information.

## Introduction 

Fiff files may include Personal Health Information and Personal Identifyable information. The consequences of openly distributing this kind of protected information can be dire. Typically, the regulatory bodies in charge of these issues in each state or country will describe methods for deidentify and anonymize data. In the United States of America the law related to this problem is the well-known HIPAA, issued by the US Department of Health and Human Services (HHS). This law mentions two main ways (see this [link](https://www.hhs.gov/hipaa/for-professionals/privacy/special-topics/de-identification/index.html) for more information) to know when it is ok to distribute a file related to a patient. This kind of easy(-er) method is called the "safe harbor", by which if the data is stripped from the following info, it is then considered "safe". 

(A) Names
(B) All geographic subdivisions smaller than a state, including street address, city, county, precinct, ZIP code, and their equivalent geocodes, except for the initial three digits of the ZIP code if, according to the current publicly available data from the Bureau of the Census:
(C) All elements of dates (except year) for dates that are directly related to an individual, including birth date, admission date, discharge date, death date, and all ages over 89 and all elements of dates (including year) indicative of such age, except that such ages and elements may be aggregated into a single category of age 90 or older
(D) Telephone numbers
(L) Vehicle identifiers and serial numbers, including license plate numbers
(E) Fax numbers
(M) Device identifiers and serial numbers
(F) Email addresses
(N) Web Universal Resource Locators (URLs)
(G) Social security numbers
(O) Internet Protocol (IP) addresses
(H) Medical record numbers
(P) Biometric identifiers, including finger and voice prints
(I) Health plan beneficiary numbers
(Q) Full-face photographs and any comparable images
(J) Account numbers
(R) Any other unique identifying number, characteristic, or code, except as permitted by paragraph (c) of this section [Paragraph (c) is presented below in the section “Re-identification”]; and
(K) Certificate/license numbers

Depending on the settings during acquisition the data files may contain few or many of the previous fields, stored in plain text, in unencrypted form. The application `mne_anonymize` was written to solve this problem. 

It is not a good idea to directly erase the problematic information contained in a fiff file, because some of these fields like Subject Name or Measuremenet date, are needed and expected by other software. Therefore `mne_anonymize` reads data from the input file, modifyies the information by inserting a set of default values instead. These new values may slighlty modify the resulting size of the output file. `mne_anonymize` does not modify the input file. It can read from write-protected folders without problem. All the new information will be stored in a separated output file.

As the fiff file consists of a linked list of tags, `mne_anonymize` will follow the list and modify the protected information. Hidden or 'unlinked' tags in the file will not be copied to the output. The so-called "free list" of tags, will not be copied to the output anonymized file either. The tag directory will not be copied to the output file either. Therefore, after `mne_anonymize` has processed a data file there is no way to recover the removed information. Use this utility with caution. |

## Command-line options 

`mne_anonymize` recognizes the following command-line options:

| Option | Description | 
|--------|-------------|
|`-h --help`| Displays help on the command line.|
|`--help-all`| Displays help, including Qt specific options on the command line.|
|`--gui`| GUI version of the application.|
|`--version`| Show the version of this appliation.|
|`-i --in <infile>`| File to anonymize.|
|`-o --out <outfile>` *optional*| Output file `<outfile>`. As default '_anonymized.fif' is attached to the file name.|
|`--verbose` *optional*| Prints out all the information about each specific anonymized field. Default: false |
|`-s --silent` *optional*| Prints no output to the terminal, other than interaction with the user or execution errors. |
|`-d --delete_input_file_after` *optional*| Delete input fiff file after anonymization. A confirmation message will be prompted to the user. Default: false |
|`-f --avoid_delete_confirmation` *optional*| Avoid confirming the deletion of the input fiff file. Default: false|
|`-b --brute` *optional*| Also anonymize subject’s weight, height, sex and handedness, and project’s ID, name, aim and comment. Default: false |
|`--md --measurement_date <value>` *optional*| Specify the measurement date. Only when anonymizing a single file. Format: DDMMYYYY Default: 01012000. |
|`--mdo --measurement_date_offset <value>` *optional*| Specify number of days to subtract to the measurement <date>. Only allowed when anonymizing a single file. Default: 0 |
|`--sb --subject_birthday <value>` *optional*| Specify the subject's birthday <date>. Only allowed when anonymizing a single file. Format: DDMMYYYY. Default: 01012000 |
|`--sbo --subject_birthday_offset <value>` *optional*| Specify number of <days> to subtract to the subject's birthday. Only allowed when anonymizing a single file. Default: 0 |
|`--his <value>` *optional*| Specify the Subject's ID within the Hospital system. Only allowed when anonymizing a single file. Default: 'mne_anonymize' |


Specifically, this utility modifies the following tags from the fiff file:

| Tag | Description | Default Anonymization Value |
|-----|-------------|-----------------------------|
|`FIFF_FILE_ID`, `FIFF_BLOCK_ID`, `FIFF_PARENT_FILE_ID`, `FIFF_PARENT_BLOCK_ID`, `FIFF_REF_FILE_ID`, `FIFF_REF_BLOCK_ID`| The ID tag includes a measurement date and unique machine ID. The machine ID usually contains the hardware address of the primary LAN card. | 2000/01/01 and 00:00:00:00:00:00:00:00 |
|`FIFF_MEAS_DATE`| The date of the measurement. | 2000/01/01 |
|`FIFF_COMMENT` in the measurement block | Holds a (textual) description of the acquisition system. | 'mne_anonymize' |
|`FIFF_EXPERIMENTER`| The experimenter's name. | 'mne_anonymize' |
|`FIFF_SUBJ_ID`| The Subject ID. | 0 |
|`FIFF_SUBJ_FIRST_NAME`| The first name of the subject. | 'mne_anonymize' |
|`FIFF_SUBJ_MIDDLE_NAME`| The middle name of the subject. | 'mne' |
|`FIFF_SUBJ_LAST_NAME`| The last name of the subject. | 'mne_anonymize' |
|`FIFF_SUBJ_BIRTH_DAY`| The birthday of the subject. | 2000/01/01 |
|`FIFF_SUBJ_SEX`| The sex of the subject. | 0 *brute mode only*|
|`FIFF_SUBJ_HAND`| The handnes of the subject. | 0 *brute mode only*|
|`FIFF_SUBJ_WEIGHT`| The weight of the subject. | 0 *brute mode only* |
|`FIFF_SUBJ_HEIGHT`| The height of the subject. | 0 *brute mode only* |
|`FIFF_SUBJ_COMMENT`| Comment about the subject. | 2000/01/01 |
|`FIFF_SUBJ_HIS_ID`| The subject's ID used in the Hospital Information System.| 'mne_anonymize' |
|`FIFF_PROJ_ID`| The project ID. | 0 *brute mode only* |
|`FIFF_PROJ_NAME`| The project name. | 'mne_anonymize' *brute mode only* |
|`FIFF_PROJ_AIM`| The project aim. | 'mne_anonymize' *brute mode only* |
|`FIFF_PROJ_PERSONS`| Persons participating in the project. | 'mne_anonymize' |
|`FIFF_PROJ_COMMENT`| Comment about the project | 'mne_anonymize' *brute mode only* |

| **Please note:** `mne_anonymize` can also alter the measurement date or the subject's birthday date, by some number of days before or after the date which is stored in the input file. |

| **Please note:** `mne_anonymize` substitutes the information in the `FIFF_SUBJ_HIS_ID` tag because some laboratories use that field to store other subject specific information. If the `--his` option is used on the command line, followed by some text, the `FIFF_SUBJ_HIS_ID` tag will be substituted with the text specified. |

| **Please note:** In case the input fiff file contains MRI data, beware that a subject's face can be reconstructed from it. The current implementation of `mne_anonymize` can not anonymize MRI data. |

## Examples

The easiest way to anonymize one single file is (will result in the output file `sample_audvis_raw_anonymized.fif`):

    $ mne_anonymize --in ./MNE-sample-data/MEG/sample/sample_audvis_raw.fif

Inplace anonymization can be performed via (`--avoid_delete_confirmation` will result in an automatic overwriting/deletion of the input file):

    $ mne_anonymize --in ./MNE-sample-data/MEG/sample/sample_audvis_raw.fif --out ./MNE-sample-data/MEG/sample/sample_audvis_raw.fif --delete_input_file --avoid_delete_confirmation

In order to substract 35 days from all measurement dates, both in the ID and `FIFF_MEAS_DATE` tags, use:

    $ mne_anonymize --in ./MNE-sample-data/MEG/sample/sample_audvis_raw.fif --measurement_date_offset 35

Typical use with abbreviated options. This line will call `mne_anonymize`, specify the input file, set verbose mode and brute mode on. Set `delete_input_file` on, avoiding the deletion confirmation, and finally set the measurement date to be 35 days before the date registered in the file.
    $ mne_anonymize -i ./MNE-sample-data/MEG/sample/sample_audvis_raw.fif -vbdf --mdo 35
