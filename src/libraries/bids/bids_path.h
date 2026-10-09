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
 * and a `.<ext>`). @ref BIDSLIB::BIDSPath is the value object that encodes
 * those entities and emits canonical paths via @ref BIDSLIB::BIDSPath::basename "BIDSPath::basename",
 * @ref BIDSLIB::BIDSPath::directory "BIDSPath::directory" and @ref BIDSLIB::BIDSPath::filePath "BIDSPath::filePath", mirroring the
 * API of @c mne_bids.BIDSPath in mne-python so call sites can be
 * ported with minimal cognitive overhead.
 *
 * Beyond serialisation, @ref BIDSLIB::BIDSPath provides three orthogonal
 * conveniences: @ref BIDSLIB::BIDSPath::withSuffix "BIDSPath::withSuffix" and the dedicated
 * @c channelsTsvPath / @c electrodesTsvPath / @c coordsystemJsonPath /
 * @c eventsTsvPath / @c sidecarJsonPath helpers so a single
 * @ref BIDSLIB::BIDSPath instance can spawn every sidecar derived from a raw-data
 * BIDSPath; @ref BIDSLIB::BIDSPath::mkdirs "BIDSPath::mkdirs" to materialise the directory
 * hierarchy lazily on write; and @ref BIDSLIB::BIDSPath::match "BIDSPath::match" to enumerate
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
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>
#include <QMap>
#include <QDir>

//=============================================================================================================
// DEFINE NAMESPACE BIDSLIB
//=============================================================================================================

namespace BIDSLIB
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
    using SPtr = QSharedPointer<BIDSPath>;            /**< Shared pointer type for BIDSPath. */
    using ConstSPtr = QSharedPointer<const BIDSPath>; /**< Const shared pointer type for BIDSPath. */

    //=========================================================================================================
    /**
     * Constructs an empty BIDSPath.
     */
    BIDSPath();

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
    BIDSPath(const QString& sRoot,
             const QString& sSubject,
             const QString& sSession = QString(),
             const QString& sTask = QString(),
             const QString& sDatatype = QString(),
             const QString& sSuffix = QString(),
             const QString& sExtension = QString());

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] other BIDSPath to copy.
     */
    BIDSPath(const BIDSPath& other);

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~BIDSPath();

    //=========================================================================================================
    // Entity setters
    //=========================================================================================================

    /**
     * Set the BIDS dataset root directory.
     *
     * @param[in] sRoot Root directory of the BIDS dataset.
     */
    void setRoot(const QString& sRoot);

    /**
     * Set the subject label (without "sub-" prefix).
     *
     * @param[in] sSubject Subject label without the "sub-" prefix.
     */
    void setSubject(const QString& sSubject);

    /**
     * Set the session label (without "ses-" prefix).
     *
     * @param[in] sSession Session label without the "ses-" prefix; empty to omit the entity.
     */
    void setSession(const QString& sSession);

    /**
     * Set the task label.
     *
     * @param[in] sTask Task label; empty to omit the entity.
     */
    void setTask(const QString& sTask);

    /**
     * Set the acquisition label.
     *
     * @param[in] sAcquisition Acquisition label; empty to omit the entity.
     */
    void setAcquisition(const QString& sAcquisition);

    /**
     * Set the run index (will be zero-padded to 2 digits).
     *
     * @param[in] sRun Run index as a numeric string (e.g. "1" becomes "01"); empty to omit the entity.
     */
    void setRun(const QString& sRun);

    /**
     * Set the processing label.
     *
     * @param[in] sProcessing Processing label; empty to omit the entity.
     */
    void setProcessing(const QString& sProcessing);

    /**
     * Set the space label.
     *
     * @param[in] sSpace Space label (e.g. a coordinate system name); empty to omit the entity.
     */
    void setSpace(const QString& sSpace);

    /**
     * Set the recording label.
     *
     * @param[in] sRecording Recording label; empty to omit the entity.
     */
    void setRecording(const QString& sRecording);

    /**
     * Set the split index.
     *
     * @param[in] sSplit Split index as a numeric string, zero-padded to 2 digits; empty to omit the entity.
     */
    void setSplit(const QString& sSplit);

    /**
     * Set the description label.
     *
     * @param[in] sDescription Description label; empty to omit the entity.
     */
    void setDescription(const QString& sDescription);

    /**
     * Set the BIDS datatype (e.g. "ieeg", "eeg", "meg", "anat").
     *
     * @param[in] sDatatype BIDS datatype, used as the innermost directory name.
     */
    void setDatatype(const QString& sDatatype);

    /**
     * Set the filename suffix (e.g. "ieeg", "channels", "electrodes", "coordsystem").
     *
     * @param[in] sSuffix Filename suffix appended after the entities.
     */
    void setSuffix(const QString& sSuffix);

    /**
     * Set the file extension including dot (e.g. ".vhdr", ".tsv", ".json").
     *
     * @param[in] sExtension File extension including the leading dot.
     */
    void setExtension(const QString& sExtension);

    //=========================================================================================================
    // Entity getters
    //=========================================================================================================

    /** @return Root directory of the BIDS dataset, or an empty string if unset. */
    QString root() const; /**< BIDS dataset root path. */
    /** @return Subject label without the "sub-" prefix, or an empty string if unset. */
    QString subject() const; /**< Subject label (without "sub-"). */
    /** @return Session label without the "ses-" prefix, or an empty string if unset. */
    QString session() const; /**< Session label (without "ses-"). */
    /** @return Task label, or an empty string if unset. */
    QString task() const; /**< Task label. */
    /** @return Acquisition label, or an empty string if unset. */
    QString acquisition() const; /**< Acquisition label. */
    /** @return Zero-padded run index, or an empty string if unset. */
    QString run() const; /**< Run index. */
    /** @return Processing label, or an empty string if unset. */
    QString processing() const; /**< Processing label. */
    /** @return Space label, or an empty string if unset. */
    QString space() const; /**< Space label. */
    /** @return Recording label, or an empty string if unset. */
    QString recording() const; /**< Recording label. */
    /** @return Zero-padded split index, or an empty string if unset. */
    QString split() const; /**< Split index. */
    /** @return Description label, or an empty string if unset. */
    QString description() const; /**< Description label. */
    /** @return BIDS datatype (e.g. "ieeg"), or an empty string if unset. */
    QString datatype() const; /**< BIDS datatype string. */
    /** @return Filename suffix, or an empty string if unset. */
    QString suffix() const; /**< Filename suffix. */
    /** @return File extension including the leading dot, or an empty string if unset. */
    QString extension() const; /**< File extension. */

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
    QString basename() const;

    /**
     * Constructs the directory path for this entity combination.
     *
     * Format: `<root>/sub-<label>[/ses-<label>]/<datatype>/`
     *
     * @return The BIDS directory path (with trailing separator).
     */
    QString directory() const;

    /**
     * Constructs the full file path: directory() + basename().
     *
     * @return The complete file path.
     */
    QString filePath() const;

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
    BIDSPath withSuffix(const QString& sSuffix, const QString& sExtension) const;

    /**
     * Returns the path for the channels.tsv sidecar.
     * @return BIDSPath pointing to the *_channels.tsv file.
     */
    BIDSPath channelsTsvPath() const;

    /**
     * Returns the path for the electrodes.tsv sidecar.
     * @return BIDSPath pointing to the *_electrodes.tsv file.
     */
    BIDSPath electrodesTsvPath() const;

    /**
     * Returns the path for the coordsystem.json sidecar.
     * @return BIDSPath pointing to the *_coordsystem.json file.
     */
    BIDSPath coordsystemJsonPath() const;

    /**
     * Returns the path for the events.tsv sidecar.
     * @return BIDSPath pointing to the *_events.tsv file.
     */
    BIDSPath eventsTsvPath() const;

    /**
     * Returns the path for the sidecar JSON metadata file
     * (e.g. *_ieeg.json, *_eeg.json).
     * @return BIDSPath pointing to the sidecar JSON file.
     */
    BIDSPath sidecarJsonPath() const;

    /**
     * Checks whether the file at filePath() exists on disk.
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
    QList<BIDSPath> match() const;

    //=========================================================================================================
    // Validation
    //=========================================================================================================

    /**
     * Validates entity values (no forbidden characters: -, _, /).
     * @param[in] sValue Entity value to validate.
     * @return true if the value is valid.
     */
    static bool isValidEntityValue(const QString& sValue);

    //=========================================================================================================
    // Operators
    //=========================================================================================================

    BIDSPath& operator=(const BIDSPath& other);
    friend bool operator==(const BIDSPath& a, const BIDSPath& b);

private:
    QString m_sRoot;        /**< BIDS dataset root directory. */
    QString m_sSubject;     /**< Subject label. */
    QString m_sSession;     /**< Session label. */
    QString m_sTask;        /**< Task label. */
    QString m_sAcquisition; /**< Acquisition label. */
    QString m_sRun;         /**< Run index. */
    QString m_sProcessing;  /**< Processing label. */
    QString m_sSpace;       /**< Space label. */
    QString m_sRecording;   /**< Recording label. */
    QString m_sSplit;       /**< Split index. */
    QString m_sDescription; /**< Description label. */
    QString m_sDatatype;    /**< BIDS datatype (ieeg, eeg, meg, anat, ...). */
    QString m_sSuffix;      /**< Filename suffix. */
    QString m_sExtension;   /**< File extension (with dot). */

    /**
     * Zero-pads a numeric string to at least 2 characters.
     */
    static QString zeroPad(const QString& sValue);
};

//=============================================================================================================

inline bool operator==(const BIDSPath& a, const BIDSPath& b)
{
    return a.root() == b.root() &&
        a.subject() == b.subject() &&
        a.session() == b.session() &&
        a.task() == b.task() &&
        a.acquisition() == b.acquisition() &&
        a.run() == b.run() &&
        a.processing() == b.processing() &&
        a.space() == b.space() &&
        a.recording() == b.recording() &&
        a.split() == b.split() &&
        a.description() == b.description() &&
        a.datatype() == b.datatype() &&
        a.suffix() == b.suffix() &&
        a.extension() == b.extension();
}

} // namespace BIDSLIB

#endif // BIDS_PATH_H
