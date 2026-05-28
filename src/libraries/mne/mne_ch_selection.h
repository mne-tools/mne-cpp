//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_ch_selection.h
 * @since March 2026
 * @brief Named subset of channels (e.g. @c "Left-temporal") loaded from an MNE @c .sel file.
 *
 * @ref MNELIB::MNEChSelection represents one entry of the channel
 * selection files shipped with the MNE C tools (per-region groupings of
 * MEG / EEG channels). Used by browsers and analyzers to drive
 * topographic layouts and quick channel subsets.
 */

#ifndef MNE_CH_SELECTION_H
#define MNE_CH_SELECTION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <Eigen/Core>

#include <QString>
#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

/**
 * Channel selection.
 */
class MNESHARED_EXPORT MNEChSelection
{
public:
    MNEChSelection() = default;
    ~MNEChSelection() = default;

    QString     name;               /**< Name of this selection. */
    QStringList chdef;              /**< Channel definitions (may contain regular expressions). */
    int         ndef = 0;           /**< How many of them. */
    QStringList chspick;            /**< Translated into channel names using the present data. */
    QStringList chspick_nospace;    /**< The same without spaces. */
    Eigen::VectorXi pick;           /**< Corresponding channels in raw data (< 0 indicates missing). */
    Eigen::VectorXi pick_deriv;     /**< Corresponding derivations in raw data. */
    int  nderiv = 0;                /**< How many derivations in the above. */
    Eigen::VectorXi ch_kind;        /**< Kinds of the channels corresponding to picks. */
    int  nchan = 0;                 /**< How many picked channels? */
    int  kind = 0;                  /**< Loaded from file or created here? */
};

/** Backward-compatible typedef aliases. */
typedef MNEChSelection  mneChSelectionRec;
typedef MNEChSelection* mneChSelection;

} // namespace MNELIB

#endif // MNE_CH_SELECTION_H
