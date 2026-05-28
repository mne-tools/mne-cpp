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
 * (@c root/sub-XX/[ses-YY/]<datatype>/) and the @c <entity>-<value>
 * ordering of the basename (@c sub, @c ses, @c task, @c acq, @c run,
 * @c proc, @c space, @c rec, @c split, @c desc, then a @c _<suffix>
 * and a @c .<ext>). @ref BIDSPath is the value object that encodes
 * those entities and emits canonical paths via @ref BIDSPath::basename,
 * @ref BIDSPath::directory and @ref BIDSPath::filePath, mirroring the
 * API of @c mne_bids.BIDSPath in mne-python so call sites can be
 * ported with minimal cognitive overhead.
 *
 * Beyond serialisation, @ref BIDSPath provides three orthogonal
 * conveniences: @ref BIDSPath::withSuffix and the dedicated
 * @c channelsTsvPath / @c electrodesTsvPath / @c coordsystemJsonPath /
 * @c eventsTsvPath / @c sidecarJsonPath helpers so a single
 * @ref BIDSPath instance can spawn every sidecar derived from a raw-data
 * BIDSPath; @ref BIDSPath::mkdirs to materialise the directory
 * hierarchy lazily on write; and @ref BIDSPath::match to enumerate
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
 * Example usage:
 * @code
 *   BIDSPath path;
 *   path.setRoot("/data/bids_dataset");
 *   path.setSubject("01");
 *   path.setSession("implant01");
 *   path.setTask("rest");
 *   path.setDatatype("ieeg");
 *   path.setSuffix("ieeg");
 *   path.setExtension(".vhdr");
 *
 *   // Produces: sub-01_ses-implant01_task-rest_ieeg.vhdr
 *   QString filename = path.basename();
 *
 *   // Produces: /data/bids_dataset/sub-01/ses-implant01/ieeg/
 *   QString dir = path.directory();
 *
 *   // Produces: /data/bids_dataset/sub-01/ses-implant01/ieeg/sub-01_ses-implant01_task-rest_ieeg.vhdr
 *   QString full = path.filePath();
 * @endcode
 *
 * @brief BIDS-compliant path and filename construction.
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

    /** Set the BIDS dataset root directory. */
    void setRoot(const QString& sRoot);

    /** Set the subject label (without "sub-" prefix). */
    void setSubject(const QString& sSubject);

    /** Set the session label (without "ses-" prefix). */
    void setSession(const QString& sSession);

    /** Set the task label. */
    void setTask(const QString& sTask);

    /** Set the acquisition label. */
    void setAcquisition(const QString& sAcquisition);

    /** Set the run index (will be zero-padded to 2 digits). */
    void setRun(const QString& sRun);

    /** Set the processing label. */
    void setProcessing(const QString& sProcessing);

    /** Set the space label. */
    void setSpace(const QString& sSpace);

    /** Set the recording label. */
    void setRecording(const QString& sRecording);

    /** Set the split index. */
    void setSplit(const QString& sSplit);

    /** Set the description label. */
    void setDescription(const QString& sDescription);

    /** Set the BIDS datatype (e.g. "ieeg", "eeg", "meg", "anat"). */
    void setDatatype(const QString& sDatatype);

    /** Set the filename suffix (e.g. "ieeg", "channels", "electrodes", "coordsystem"). */
    void setSuffix(const QString& sSuffix);

    /** Set the file extension including dot (e.g. ".vhdr", ".tsv", ".json"). */
    void setExtension(const QString& sExtension);

    //=========================================================================================================
    // Entity getters
    //=========================================================================================================

    QString root() const;               /**< BIDS dataset root path. */
    QString subject() const;            /**< Subject label (without "sub-"). */
    QString session() const;            /**< Session label (without "ses-"). */
    QString task() const;               /**< Task label. */
    QString acquisition() const;        /**< Acquisition label. */
    QString run() const;                /**< Run index. */
    QString processing() const;         /**< Processing label. */
    QString space() const;              /**< Space label. */
    QString recording() const;          /**< Recording label. */
    QString split() const;              /**< Split index. */
    QString description() const;        /**< Description label. */
    QString datatype() const;           /**< BIDS datatype string. */
    QString suffix() const;             /**< Filename suffix. */
    QString extension() const;          /**< File extension. */

    //=========================================================================================================
    // Path construction
    //=========================================================================================================

    /**
     * Constructs the BIDS-compliant filename (without directory).
     *
     * Format: sub-<label>[_ses-<label>][_task-<label>][_acq-<label>][_run-<index>]
     *         [_proc-<label>][_space-<label>][_rec-<label>][_split-<index>]
     *         [_desc-<label>]_<suffix><extension>
     *
     * @return The BIDS filename.
     */
    QString basename() const;

    /**
     * Constructs the directory path for this entity combination.
     *
     * Format: <root>/sub-<label>[/ses-<label>]/<datatype>/
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
     * Searches the BIDS root for all files matching the current entities.
     * Wildcards are used for unset entities.
     *
     * @return List of matching BIDSPath objects.
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
    QString m_sRoot;            /**< BIDS dataset root directory. */
    QString m_sSubject;         /**< Subject label. */
    QString m_sSession;         /**< Session label. */
    QString m_sTask;            /**< Task label. */
    QString m_sAcquisition;     /**< Acquisition label. */
    QString m_sRun;             /**< Run index. */
    QString m_sProcessing;      /**< Processing label. */
    QString m_sSpace;           /**< Space label. */
    QString m_sRecording;       /**< Recording label. */
    QString m_sSplit;           /**< Split index. */
    QString m_sDescription;     /**< Description label. */
    QString m_sDatatype;        /**< BIDS datatype (ieeg, eeg, meg, anat, ...). */
    QString m_sSuffix;          /**< Filename suffix. */
    QString m_sExtension;       /**< File extension (with dot). */

    /**
     * Zero-pads a numeric string to at least 2 characters.
     */
    static QString zeroPad(const QString& sValue);
};

} // namespace BIDSLIB

#endif // BIDS_PATH_H
