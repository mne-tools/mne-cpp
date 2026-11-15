//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_path.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Programmatic construction and matching of BIDS-compliant directories, filenames and sidecar paths.
 *
 * The BIDS specification fixes both the directory layout
 * (`root/sub-XX/[ses-YY/]<datatype>/`) and the `<entity>-<value>`
 * ordering of the basename (@c sub, @c ses, @c task, @c acq, @c run,
 * @c proc, @c space, @c rec, @c split, @c desc, then a `_<suffix>`
 * and a `.<ext>`). @ref mne::BIDSPath is the value object that encodes
 * those entities and emits canonical paths via @ref mne::BIDSPath::basename "BIDSPath::basename",
 * @ref mne::BIDSPath::directory "BIDSPath::directory" and @ref mne::BIDSPath::file_path "BIDSPath::file_path", mirroring the
 * API of @c mne_bids.BIDSPath in mne-python so call sites can be
 * ported with minimal cognitive overhead.
 *
 * Beyond serialisation, @ref mne::BIDSPath provides three orthogonal
 * conveniences: @ref mne::BIDSPath::with_suffix "BIDSPath::with_suffix" and the dedicated
 * @c channels_tsv_path / @c electrodes_tsv_path / @c coordsystem_json_path /
 * @c events_tsv_path / @c sidecar_json_path helpers so a single
 * @ref mne::BIDSPath instance can spawn every sidecar derived from a raw-data
 * BIDSPath; @ref mne::BIDSPath::mkdirs "BIDSPath::mkdirs" to materialise the directory
 * hierarchy lazily on write; and @ref mne::BIDSPath::match "BIDSPath::match" to enumerate
 * actual on-disk files whose entity values complete the unset slots of
 * the current path. Entity-value validation rejects the three
 * characters BIDS forbids inside a label (@c -, @c _, @c /).
 *
 * Spec: https://bids-specification.readthedocs.io/en/stable/common-principles.html#file-name-structure
 */

#ifndef BIDS_PATH_H
#define BIDS_PATH_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Manages BIDS-compliant file paths.
 *
 * A BIDSPath encodes the BIDS entities (subject, session, task, acquisition, run, etc.)
 * together with a datatype, suffix, extension, and root directory to produce
 * fully qualified file paths that conform to the BIDS specification.
 *
 * @brief BIDS-compliant path and filename construction.
 *
 * @snippet ex_bids/main.cpp bids_path_usage
 */
class BIDSSHARED_EXPORT BIDSPath
{
public:
    //=========================================================================================================
    /**
     * Constructs an empty BIDSPath.
     */
    BIDSPath() = default;

    //=========================================================================================================
    /**
     * Constructs a BIDSPath with the most common entities.
     *
     * @param[in] sRoot       Root directory of the BIDS dataset.
     * @param[in] sSubject    Subject label (without "sub-" prefix).
     * @param[in] sSession    Session label (without "ses-" prefix). Can be empty.
     * @param[in] sTask       Task label. Can be empty.
     * @param[in] sDatatype   BIDS datatype (e.g. "ieeg", "eeg", "meg", "anat").
     * @param[in] sSuffix     Filename suffix (e.g. "ieeg", "channels", "electrodes").
     * @param[in] sExtension  File extension including dot (e.g. ".vhdr", ".tsv", ".json").
     */
    BIDSPath(std::filesystem::path sRoot,
             std::string sSubject,
             std::string sSession = {},
             std::string sTask = {},
             std::string sDatatype = {},
             std::string sSuffix = {},
             std::string sExtension = {});

    //=========================================================================================================
    // Entity setters
    //=========================================================================================================

    /**
     * Set the BIDS dataset root directory.
     *
     * @param[in] sRoot Root directory of the BIDS dataset.
     */
    void set_root(std::filesystem::path sRoot);

    /**
     * Set the subject label (without "sub-" prefix).
     *
     * @param[in] sSubject Subject label without the "sub-" prefix.
     */
    void set_subject(std::string sSubject);

    /**
     * Set the session label (without "ses-" prefix).
     *
     * @param[in] sSession Session label without the "ses-" prefix; empty to omit the entity.
     */
    void set_session(std::string sSession);

    /**
     * Set the task label.
     *
     * @param[in] sTask Task label; empty to omit the entity.
     */
    void set_task(std::string sTask);

    /**
     * Set the acquisition label.
     *
     * @param[in] sAcquisition Acquisition label; empty to omit the entity.
     */
    void set_acquisition(std::string sAcquisition);

    /**
     * Set the run index (will be zero-padded to 2 digits).
     *
     * @param[in] sRun Run index as a numeric string (e.g. "1" becomes "01"); empty to omit the entity.
     */
    void set_run(std::string sRun);

    /**
     * Set the processing label.
     *
     * @param[in] sProcessing Processing label; empty to omit the entity.
     */
    void set_processing(std::string sProcessing);

    /**
     * Set the space label.
     *
     * @param[in] sSpace Space label (e.g. a coordinate system name); empty to omit the entity.
     */
    void set_space(std::string sSpace);

    /**
     * Set the recording label.
     *
     * @param[in] sRecording Recording label; empty to omit the entity.
     */
    void set_recording(std::string sRecording);

    /**
     * Set the split index.
     *
     * @param[in] sSplit Split index as a numeric string, zero-padded to 2 digits; empty to omit the entity.
     */
    void set_split(std::string sSplit);

    /**
     * Set the description label.
     *
     * @param[in] sDescription Description label; empty to omit the entity.
     */
    void set_description(std::string sDescription);

    /**
     * Set the BIDS datatype (e.g. "ieeg", "eeg", "meg", "anat").
     *
     * @param[in] sDatatype BIDS datatype, used as the innermost directory name.
     */
    void set_datatype(std::string sDatatype);

    /**
     * Set the filename suffix (e.g. "ieeg", "channels", "electrodes", "coordsystem").
     *
     * @param[in] sSuffix Filename suffix appended after the entities.
     */
    void set_suffix(std::string sSuffix);

    /**
     * Set the file extension including dot (e.g. ".vhdr", ".tsv", ".json").
     *
     * @param[in] sExtension File extension including the leading dot.
     */
    void set_extension(std::string sExtension);

    //=========================================================================================================
    // Entity getters
    //=========================================================================================================

    /** @return Root directory of the BIDS dataset, or an empty path if unset. */
    const std::filesystem::path& root() const;
    /** @return Subject label without the "sub-" prefix, or an empty string if unset. */
    const std::string& subject() const;
    /** @return Session label without the "ses-" prefix, or an empty string if unset. */
    const std::string& session() const;
    /** @return Task label, or an empty string if unset. */
    const std::string& task() const;
    /** @return Acquisition label, or an empty string if unset. */
    const std::string& acquisition() const;
    /** @return Zero-padded run index, or an empty string if unset. */
    const std::string& run() const;
    /** @return Processing label, or an empty string if unset. */
    const std::string& processing() const;
    /** @return Space label, or an empty string if unset. */
    const std::string& space() const;
    /** @return Recording label, or an empty string if unset. */
    const std::string& recording() const;
    /** @return Zero-padded split index, or an empty string if unset. */
    const std::string& split() const;
    /** @return Description label, or an empty string if unset. */
    const std::string& description() const;
    /** @return BIDS datatype (e.g. "ieeg"), or an empty string if unset. */
    const std::string& datatype() const;
    /** @return Filename suffix, or an empty string if unset. */
    const std::string& suffix() const;
    /** @return File extension including the leading dot, or an empty string if unset. */
    const std::string& extension() const;

    //=========================================================================================================
    // Path construction
    //=========================================================================================================

    /**
     * Constructs the BIDS-compliant filename (without directory).
     *
     * Format: `sub-<label>[_ses-<label>][_task-<label>][_acq-<label>][_run-<index>]`
     *         `[_proc-<label>][_space-<label>][_recording-<label>][_split-<index>]`
     *         `[_desc-<label>]_<suffix><extension>`
     *
     * @return The BIDS filename.
     */
    std::string basename() const;

    /**
     * Constructs the directory path for this entity combination.
     *
     * Format: `<root>/sub-<label>[/ses-<label>]/<datatype>/`
     *
     * @return The BIDS directory path.
     */
    std::filesystem::path directory() const;

    /**
     * Constructs the full file path: directory() / basename().
     *
     * @return The complete file path.
     */
    std::filesystem::path file_path() const;

    //=========================================================================================================
    // Convenience methods
    //=========================================================================================================

    /**
     * Returns a copy of this BIDSPath with updated suffix and extension.
     * Useful for deriving sidecar paths from a data path.
     *
     * @param[in] sSuffix     New suffix (e.g. "channels", "electrodes", "coordsystem").
     * @param[in] sExtension  New extension (e.g. ".tsv", ".json").
     * @return A new BIDSPath with the updated suffix and extension.
     */
    BIDSPath with_suffix(std::string sSuffix, std::string sExtension) const;

    /**
     * Returns the path for the channels.tsv sidecar.
     * @return BIDSPath pointing to the *_channels.tsv file.
     */
    BIDSPath channels_tsv_path() const;

    /**
     * Returns the path for the electrodes.tsv sidecar.
     * @return BIDSPath pointing to the *_electrodes.tsv file.
     */
    BIDSPath electrodes_tsv_path() const;

    /**
     * Returns the path for the coordsystem.json sidecar.
     * @return BIDSPath pointing to the *_coordsystem.json file.
     */
    BIDSPath coordsystem_json_path() const;

    /**
     * Returns the path for the events.tsv sidecar.
     * @return BIDSPath pointing to the *_events.tsv file.
     */
    BIDSPath events_tsv_path() const;

    /**
     * Returns the path for the sidecar JSON metadata file
     * (e.g. *_ieeg.json, *_eeg.json).
     * @return BIDSPath pointing to the sidecar JSON file.
     */
    BIDSPath sidecar_json_path() const;

    /**
     * Checks whether the file at file_path() exists on disk.
     * @return true if the file exists.
     */
    bool exists() const;

    /**
     * Creates the directory structure for this path (mkdir -p).
     * @return true if the directory exists or was created successfully.
     */
    bool mkdirs() const;

    /**
     * Searches the BIDS root (all subject, session and datatype folders) for files whose entities, datatype,
     * suffix and extension equal the ones set here; unset ones match anything, as in mne_bids.BIDSPath.match.
     *
     * @return Matching paths, sorted by file path.
     */
    std::vector<BIDSPath> match() const;

    //=========================================================================================================
    // Validation
    //=========================================================================================================

    /**
     * Validates entity values: BIDS labels are alphanumeric ASCII.
     * @param[in] sValue Entity value to validate.
     * @return true if the value is valid.
     */
    static bool is_valid_entity_value(std::string_view sValue);

    //=========================================================================================================
    // Operators
    //=========================================================================================================

    /**
     * @param[in] other   Another path.
     * @return Whether both paths have the same root, entities, datatype, suffix and extension.
     */
    bool operator==(const BIDSPath& other) const = default;

private:
    std::filesystem::path m_sRoot; /**< BIDS dataset root directory. */
    std::string m_sSubject;        /**< Subject label. */
    std::string m_sSession;        /**< Session label. */
    std::string m_sTask;           /**< Task label. */
    std::string m_sAcquisition;    /**< Acquisition label. */
    std::string m_sRun;            /**< Run index. */
    std::string m_sProcessing;     /**< Processing label. */
    std::string m_sSpace;          /**< Space label. */
    std::string m_sRecording;      /**< Recording label. */
    std::string m_sSplit;          /**< Split index. */
    std::string m_sDescription;    /**< Description label. */
    std::string m_sDatatype;       /**< BIDS datatype (ieeg, eeg, meg, anat, ...). */
    std::string m_sSuffix;         /**< Filename suffix. */
    std::string m_sExtension;      /**< File extension (with dot). */

    /**
     * Zero-pads a numeric string to at least 2 characters.
     *
     * @param[in] sValue   A label.
     * @return @p sValue, padded with leading zeros if it is a number.
     */
    static std::string zero_pad(std::string sValue);
};

} // namespace mne

#endif // BIDS_PATH_H
