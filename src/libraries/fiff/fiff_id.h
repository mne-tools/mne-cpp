//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_id.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2012
 * @brief    128-bit FIFF identifier record (machine ID + creation time) used to stamp files, blocks and parent references.
 *
 * Every FIFF file carries a @c FIFF_FILE_ID tag at its head and every
 * significant block (measurement, raw data, evoked, epochs, projection,
 * covariance, ...) carries a @c FIFF_BLOCK_ID tag; in addition each child
 * file can carry @c FIFF_PARENT_FILE_ID / @c FIFF_PARENT_BLOCK_ID
 * back-references so that derived data sets remain provenance-traceable
 * to their source recording. All four are physically the same record:
 * the legacy @c fiffIdRec.
 *
 * @ref FiffId is the C++ wrapper for that record. It exposes the version
 * word, the 2x32-bit @c machid hardware identifier and a @ref FiffTime
 * creation time, plus convenience constructors that synthesize a fresh
 * ID for a newly written file (mirroring @c mne.io.write.write_id in
 * MNE-Python).
 */

#ifndef FIFF_ID_H
#define FIFF_ID_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * @brief 128-bit FIFF identifier: hardware machine ID plus creation time, stamped on every file and block.
 *
 * On-disk layout is a 20-byte record (version + 2x4-byte machine ID +
 * 8-byte @ref FiffTime). The @c machid is filled from the host MAC
 * address by @c FiffStream::write_id when generating new identifiers,
 * matching the Neuromag acquisition stack so file lineage stays
 * reconstructible.
 */

class FIFFSHARED_EXPORT FiffId {

public:
    using SPtr = QSharedPointer<FiffId>;            /**< Shared pointer type for FiffId. */
    using ConstSPtr = QSharedPointer<const FiffId>; /**< Const shared pointer type for FiffId. */
    using UPtr = std::unique_ptr<FiffId>;             /**< Unique pointer type for FiffId. */
    using ConstUPtr = std::unique_ptr<const FiffId>;  /**< Const unique pointer type for FiffId. */

    //=========================================================================================================
    /**
     * Default Constructor
     */
    FiffId();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffId   Universially unique identifier which should be copied.
     */
    FiffId(const FiffId& p_FiffId);

    //=========================================================================================================
    /**
     * Destroys the universially unique identifier.
     */
    ~FiffId();

    //=========================================================================================================
    /**
     * Constructs a (hopefully) unique file id.
     *
     * @return A new unique FiffId.
     */
    static FiffId new_file_id();

    //=========================================================================================================
    /**
     * Resets the File identifier.
     */
    void clear();

    //=========================================================================================================
    /**
     * Returns the machine ID consisting of a two integer number.
     *
     * @param[out] fixed_id   Pointer to an array of at least 2 ints to receive the machine ID.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool get_machid(int *fixed_id);

    //=========================================================================================================
    /**
     * Returns a default FiffId object to be used as a mutable placeholder for a default instance of the class.
     *
     * @return Reference to the default FiffId instance.
     */
    static FiffId& getDefault();

    //=========================================================================================================
    /**
     * Prints the id.
     */
    void print() const;

    //=========================================================================================================
    /**
     * True if FIFF id is empty.
     *
     * @return true if FIFF id is empty.
     */
    inline bool isEmpty() const;

    //=========================================================================================================
    /**
     * Size of the old struct (fiffIdRec) 5*int = 5*4 = 20
     *
     * @return the size of the old struct fiffIdRec.
     */
    inline static qint32 storageSize();

    //=========================================================================================================
    /**
     * Returns the machine ID as a human-readable string.
     *
     * @return The machine ID formatted as a string.
     */
    QString toMachidString() const;

    //=========================================================================================================
    /**
     * Returns a human-readable string representation of this ID.
     *
     * Format: "major.minor 0xMACHID1MACHID2 timestamp"
     *
     * @return Formatted string describing the ID.
     */
    QString toString() const;

    /**
     * Compares two FiffId instances for equality.
     *
     * @param[in] f1   First FiffId.
     * @param[in] f2   Second FiffId.
     *
     * @return true if both IDs are equal, false otherwise.
     */
    friend bool operator== (const FiffId &f1, const FiffId &f2);

public:
    fiff_int_t version;     /**< File version. */
    fiff_int_t machid[2];   /**< Unique machine ID. */
    FiffTime time;           /**< Time of the ID creation. */

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool FiffId::isEmpty() const
{
    return this->version <= 0;
}

//=============================================================================================================

inline qint32 FiffId::storageSize()
{
    return sizeof(FiffId::version) + sizeof(FiffId::machid) + FiffTime::storageSize();
}

//=============================================================================================================

inline bool operator== (const FiffId &a, const FiffId &b)
{
    return (a.version == b.version &&
            a.machid[0] == b.machid[0] &&
            a.machid[1] == b.machid[1] &&
            a.time.secs == b.time.secs &&
            a.time.usecs == b.time.usecs);
}
} // NAMESPACE

#endif // FIFF_ID_H
