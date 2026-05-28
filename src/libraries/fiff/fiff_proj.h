//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_proj.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @author   Christof Pieloth <pieloth@labp.htwk-leipzig.de>
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2012
 * @brief    SSP projection item: a named projection vector set with active/desired flags, parsed from FIFFB_PROJ_ITEM.
 *
 * Signal-Space Projection (SSP) removes a low-rank subspace from MEG/EEG
 * data; the projector itself is stored under @c FIFFB_PROJ /
 * @c FIFFB_PROJ_ITEM tags. @ref FiffProj is the C++ wrapper for one such
 * item: a description, the kind (@c FIFFV_PROJ_ITEM_FIELD,
 * @c FIFFV_PROJ_ITEM_EEG_AVREF, ...), the active flag, the desired flag
 * and the named matrix carrying the projection vectors keyed by channel
 * name. The list of @ref FiffProj inside @ref FiffInfo::projs is what
 * @ref FiffRawData and @ref FiffEvoked apply (or de-apply) via
 * @c make_projector during raw / evoked processing, with field-for-field
 * parity to @c mne.Projection / @c mne.compute_proj_* in MNE-Python.
 */

#ifndef FIFF_PROJ_H
#define FIFF_PROJ_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"
#include "fiff_named_matrix.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QMap>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FiffRawData;

//=============================================================================================================
/**
 * @brief Single SSP projection item: kind, active flag, desired flag and the named projection vector matrix.
 *
 * Maps one @c FIFFB_PROJ_ITEM block to a C++ value: description, kind,
 * @c nvec, @c active, plus a @ref FiffNamedMatrix whose row names are the
 * channels the projector acts on. Multiple @ref FiffProj instances form
 * the @c info['projs'] list and are combined into one projection
 * operator by @c make_projector when raw / evoked data is loaded.
 */
class FIFFSHARED_EXPORT FiffProj {

public:
    using SPtr = QSharedPointer<FiffProj>;            /**< Shared pointer type for FiffProj. */
    using ConstSPtr = QSharedPointer<const FiffProj>; /**< Const shared pointer type for FiffProj. */
    using UPtr = std::unique_ptr<FiffProj>;             /**< Unique pointer type for FiffProj. */
    using ConstUPtr = std::unique_ptr<const FiffProj>;  /**< Const unique pointer type for FiffProj. */

    //=========================================================================================================
    /**
     * Default constructor.
     */
    FiffProj();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffProj  SSP projector data which should be copied.
     */
    FiffProj(const FiffProj& p_FiffProj);

    //=========================================================================================================
    /**
     * Constructor
     */
    explicit FiffProj(fiff_int_t p_kind,
                      bool p_active,
                      QString p_desc,
                      FiffNamedMatrix& p_data);

    //=========================================================================================================
    /**
     * Destroys the FiffProj.
     */
    ~FiffProj();

    //=========================================================================================================
    /**
     * Set all projections to active
     *
     * @param[in, out] p_qListFiffProj  activates projectors in place.
     */
    static void activate_projs(QList<FiffProj> &p_qListFiffProj);

    //=========================================================================================================
    /**
     * mne_make_projector
     *
     * ToDo move this to fiff_proj; Before: check if info is needed and if make_projector_info should be also moved.
     *
     * Make an SSP operator
     *
     * @param[in] projs      A set of projection vectors.
     * @param[in] ch_names   A cell array of channel names.
     * @param[out] proj      The projection operator to apply to the data.
     * @param[in] bads       Bad channels to exclude.
     * @param[out] U         The orthogonal basis of the projection vectors (optional).
     *
     * @return nproj - How many items in the projector.
     */
    static fiff_int_t make_projector(const QList<FiffProj>& projs,
                                     const QStringList& ch_names,
                                     Eigen::MatrixXd& proj,
                                     const QStringList& bads = defaultQStringList,
                                     Eigen::MatrixXd& U = defaultMatrixXd);

    //=========================================================================================================
    /**
     * compute_from_raw
     *
     * Create SSP (Signal-Space Projection) operators from raw data via SVD.
     * Ported from make_ssp.c (MNE-C).
     *
     * @param[in] raw           The raw data.
     * @param[in] events        Event matrix (nEvents x 3).
     * @param[in] eventCode     Which event code to select epochs for.
     * @param[in] tmin          Start of epoch relative to event (seconds).
     * @param[in] tmax          End of epoch relative to event (seconds).
     * @param[in] nGrad         Number of gradiometer projection vectors.
     * @param[in] nMag          Number of magnetometer projection vectors.
     * @param[in] nEeg          Number of EEG projection vectors.
     * @param[in] mapReject     Rejection thresholds (key = channel type string, value = threshold).
     *
     * @return List of FiffProj items, or empty list on failure.
     */
    static QList<FiffProj> compute_from_raw(const FiffRawData &raw,
                                            const Eigen::MatrixXi &events,
                                            int eventCode,
                                            float tmin,
                                            float tmax,
                                            int nGrad,
                                            int nMag,
                                            int nEeg,
                                            const QMap<QString,double> &mapReject = QMap<QString,double>());

    //=========================================================================================================
    /**
     * overloading the stream out operator<<
     *
     * @param[in] out           The stream to which the fiff projector should be assigned to.
     * @param[in] p_FiffProj    Fiff projector which should be assigned to the stream.
     *
     * @return the stream with the attached fiff projector.
     */
    friend std::ostream& operator<<(std::ostream& out, const FIFFLIB::FiffProj &p_FiffProj);

public:
    fiff_int_t kind;                /**< Fiff kind. */
    bool active;                    /**< If fiff projector active. */
    QString desc;                   /**< Projector description. */

    FiffNamedMatrix::SDPtr data;    /**< Projector data, rows are equal to the length of channels. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline std::ostream& operator<<(std::ostream& out, const FIFFLIB::FiffProj &p_FiffProj)
{
    out << "#### Fiff Projector ####\n";
    out << "\tKind: " << p_FiffProj.kind << std::endl;
    out << "\tactive: " << p_FiffProj.active << std::endl;
    out << "\tdesc: " << p_FiffProj.desc.toUtf8().constData() << std::endl;
    out << "\tdata:\n\t" << *p_FiffProj.data.data() << std::endl;
    return out;
}
} // NAMESPACE

#endif // FIFF_PROJ_H
