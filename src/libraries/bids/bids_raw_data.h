//=============================================================================================================
/**
 * @file     bids_raw_data.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    BidsRawData class declaration — central container for a BIDS raw dataset
 *           with integrated read/write capabilities and sidecar metadata.
 *
 */

#ifndef BIDS_RAW_DATA_H
#define BIDS_RAW_DATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_global.h"
#include "bids_event.h"
#include "bids_electrode.h"
#include "bids_coordinate_system.h"
#include "bids_path.h"
#include "readers/bids_abstract_format_reader.h"

#include <fiff/fiff_raw_data.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>
#include <QList>
#include <QMap>

//=============================================================================================================
// DEFINE NAMESPACE BIDSLIB
//=============================================================================================================

namespace BIDSLIB
{

//=============================================================================================================
/**
 * @brief Central container for a BIDS raw dataset, bundling electrophysiological
 *        data with all associated sidecar metadata.
 *
 * BidsRawData is the primary data object of the BIDS library, analogous to
 * FiffRawData for the FIFF library.  It owns both the data and the I/O logic:
 *
 * - **read()** — static factory that loads a BIDS dataset from disk.
 * - **write()** — member that writes the current dataset to a BIDS directory.
 *
 * Sidecar and coordinate-system fields that are derivable from FiffInfo
 * (sampling frequency, channel counts) are computed on write and applied to
 * FiffInfo on read.  Only fields carrying independent information are stored
 * as explicit members.
 *
 * Example:
 * @code
 *   BIDSPath path("/data/bids", "01", "implant01", "rest", "ieeg", "ieeg", ".vhdr");
 *   BidsRawData data = BidsRawData::read(path);
 *   if(data.isValid()) {
 *       qDebug() << "Channels:" << data.raw.info.nchan;
 *       qDebug() << "Events:"   << data.events.size();
 *
 *       // Round-trip: write to a new BIDS root
 *       BIDSPath out("/data/bids_out", "01", "implant01", "rest", "ieeg", "ieeg", ".vhdr");
 *       data.write(out, path.filePath());
 *   }
 * @endcode
 */
class BIDSSHARED_EXPORT BidsRawData
{
public:
    using SPtr = QSharedPointer<BidsRawData>;
    using ConstSPtr = QSharedPointer<const BidsRawData>;

    //=========================================================================================================
    /**
     * @brief Options controlling how write() operates.
     */
    struct WriteOptions
    {
        bool    overwrite{false};   /**< Overwrite existing files if true. */
        bool    copyData{true};     /**< Copy the raw data file into the BIDS directory.
                                         If false, only sidecars are written. */
        QString datasetName;        /**< Name for dataset_description.json (defaults to "[Unspecified]"). */
    };

    //=========================================================================================================
    /**
     * Default constructor. Creates an empty, invalid BidsRawData.
     */
    BidsRawData() = default;

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~BidsRawData() = default;

    // Move-only: the reader holds exclusive ownership of the open file.
    BidsRawData(BidsRawData&& other) noexcept;
    BidsRawData& operator=(BidsRawData&& other) noexcept;
    BidsRawData(const BidsRawData&) = delete;
    BidsRawData& operator=(const BidsRawData&) = delete;

    //=========================================================================================================
    // I/O — read
    //=========================================================================================================

    /**
     * @brief Read a BIDS dataset from disk.
     *
     * Static factory that orchestrates reading of:
     * 1. Raw data file via the appropriate format reader (EDF, BrainVision, …)
     * 2. *_channels.tsv  → channel types, units, bad-channel marking
     * 3. *_electrodes.tsv + *_coordsystem.json → digitization points
     * 4. *_events.tsv  → event annotations
     * 5. *_{datatype}.json → line frequency and recording metadata
     *
     * The BIDSPath must have at minimum: root, subject, datatype, suffix, extension.
     *
     * @param[in] bidsPath  Fully specified BIDSPath pointing to the raw data file.
     *
     * @return Populated BidsRawData.  Check isValid() to determine success.
     */
    static BidsRawData read(const BIDSPath& bidsPath);

    //=========================================================================================================
    // I/O — write
    //=========================================================================================================

    /**
     * @brief Write this dataset to a BIDS-compliant directory.
     *
     * Orchestrates writing of:
     * 1. Directory structure: root/sub-XX/[ses-YY/]&lt;datatype&gt;/
     * 2. Raw data file — copies the source file into the BIDS directory
     * 3. *_channels.tsv
     * 4. *_electrodes.tsv + *_coordsystem.json
     * 5. *_events.tsv
     * 6. *_{datatype}.json  (iEEG sidecar)
     * 7. dataset_description.json
     *
     * @param[in] bidsPath   Target BIDSPath (root, subject, task, datatype, suffix, extension).
     * @param[in] sourcePath Path to the original raw data file to copy.
     *                       If empty, the raw data file is not copied.
     * @param[in] options    Write options (overwrite, dataset name, etc.).
     *
     * @return BIDSPath of the written data file, or empty BIDSPath on failure.
     */
    BIDSPath write(const BIDSPath& bidsPath,
                   const QString& sourcePath,
                   const WriteOptions& options) const;

    /**
     * @brief Convenience overload — write with default options.
     */
    inline BIDSPath write(const BIDSPath& bidsPath,
                          const QString& sourcePath = QString()) const
    {
        return write(bidsPath, sourcePath, WriteOptions());
    }

    //=========================================================================================================
    // Query
    //=========================================================================================================

    /**
     * @brief Returns true if the data was loaded successfully.
     */
    bool isValid() const { return m_bIsValid; }

    /**
     * @brief Marks the data as valid or invalid.
     */
    void setValid(bool bValid) { m_bIsValid = bValid; }

    /**
     * @brief Clears all data members and resets to invalid state.
     */
    void clear();

    //=========================================================================================================
    // Static helpers
    //=========================================================================================================

    /**
     * @brief Create the appropriate format reader for a given file extension.
     *
     * @param[in] sExtension  File extension including dot (e.g. ".vhdr", ".edf").
     *
     * @return Shared pointer to the reader, or nullptr if unsupported.
     */
    static AbstractFormatReader::UPtr createReader(const QString& sExtension);

    //=========================================================================================================
    // Core data members (public, following mne-cpp convention)
    //=========================================================================================================

    FIFFLIB::FiffRawData                raw;            /**< Raw data with fully populated FiffInfo. */
    QList<BidsEvent>                    events;         /**< Events from *_events.tsv. */
    QMap<QString, int>                  eventIdMap;     /**< trial_type string → numeric value mapping. */
    QList<BidsElectrode>                electrodes;     /**< Electrode positions from *_electrodes.tsv. */
    BidsCoordinateSystem                coordinateSystem; /**< Coordinate system from *_coordsystem.json. */
    AbstractFormatReader::UPtr          reader;         /**< The underlying format reader (keeps file open). */

    //=========================================================================================================
    // iEEG sidecar metadata (integrated from *_{datatype}.json)
    // Fields derivable from FiffInfo (sfreq, channel counts) are omitted —
    // they are computed on write and applied on read.
    //=========================================================================================================

    QString ieegReference;              /**< Reference electrode name (RECOMMENDED). */
    QString taskDescription;            /**< Task description (OPTIONAL). */
    QString manufacturer;               /**< Amplifier manufacturer (RECOMMENDED). */
    QString manufacturerModelName;      /**< Amplifier model (RECOMMENDED). */
    QString softwareVersions;           /**< Recording software versions (RECOMMENDED). */
    QString recordingType;              /**< "continuous", "epoched", or "discontinuous" (RECOMMENDED). */

private:
    bool m_bIsValid{false};     /**< True if the data was loaded successfully. */
};

} // namespace BIDSLIB

#endif // BIDS_RAW_DATA_H
