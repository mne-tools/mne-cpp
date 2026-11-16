//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_subject.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    One participant in an MNA project — owner of measurement sessions and link to the FreeSurfer anatomy.
 *
 * @ref mne::MnaSubject is the root of the per-participant subtree in an
 * MNA project. It binds together all sessions/recordings that
 * belong to one person and pins their structural MRI reconstruction
 * via @c freesurfer_dir, a project-relative path to the matching
 * @c $SUBJECTS_DIR entry. Keeping that pointer alongside the data
 * means forward solutions, source spaces and parcellations always
 * resolve to the correct anatomy without relying on environment
 * variables at analysis time.
 *
 * The @c id mirrors the BIDS @c sub-XX convention and is the key
 * downstream consumers (forward modelling, source estimation,
 * group statistics) use to look the subject up. @c sessions holds
 * the ordered @ref mne::MnaSession list, and @c extras preserves any
 * demographic, clinical or consent-tracking fields that newer
 * tooling may attach so older mne_mna builds can still read and
 * re-write the project without data loss.
 */

#ifndef MNA_SUBJECT_H
#define MNA_SUBJECT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_session.h"

#include <base/cbor.h>
#include <base/json.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief One participant in an MNA project with sessions and a link to the FreeSurfer anatomy.
 *
 * @snippet ex_mna/main.cpp mna_project_files
 */
struct MNASHARED_EXPORT MnaSubject
{
    std::string id;                   /**< Subject identifier. */
    std::string freesurfer_dir;       /**< Relative path to FreeSurfer SUBJECTS_DIR. */
    std::vector<MnaSession> sessions; /**< Sessions for this subject. */
    JsonObject extras;                /**< Unknown keys preserved for lossless round-trip. */

    /** @return The subject as a JSON object. */
    JsonObject to_json() const;

    /** @param[in] json   A subject as written by to_json(). @return The subject. */
    static MnaSubject from_json(const JsonObject& json);

    /** @return The subject as a CBOR map. */
    CborMap to_cbor() const;

    /** @param[in] cbor   A subject as written by to_cbor(). @return The subject. */
    static MnaSubject from_cbor(const CborMap& cbor);
};

} // namespace mne

#endif // MNA_SUBJECT_H
