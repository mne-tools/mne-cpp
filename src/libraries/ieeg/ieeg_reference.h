//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ieeg_reference.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Bipolar re-referencing of sEEG / ECoG contacts along their leads.
 *
 * A bipolar derivation subtracts the next contact on the same lead from each contact, which
 * removes signal common to neighbouring contacts (reference and volume conduction) and leaves
 * the activity between them. The pairs follow the lead: along a depth lead or strip, and
 * along the rows and columns of a grid.
 */

#ifndef IEEG_REFERENCE_H
#define IEEG_REFERENCE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ieeg_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE IEEGLIB
//=============================================================================================================

namespace IEEGLIB
{

class IeegImplantation;

//=============================================================================================================
/**
 * @brief One bipolar derivation: anode minus cathode.
 *
 * @snippet ex_ieeg/main.cpp ieeg_reference_usage
 */
struct IEEGSHARED_EXPORT IeegBipolarPair
{
    int anode = -1;                                     /**< Contact index of the anode. */
    int cathode = -1;                                   /**< Contact index of the cathode. */
    QString name;                                       /**< Derivation name, "anode-cathode" as in MNE-Python. */
    Eigen::Vector3f position = Eigen::Vector3f::Zero(); /**< Midpoint of the two contacts. */
};

//=============================================================================================================
/**
 * @brief Bipolar montages along sEEG / ECoG leads.
 *
 * @snippet ex_ieeg/main.cpp ieeg_reference_usage
 */
class IEEGSHARED_EXPORT IeegReference
{
public:
    /** Contacts further apart than this are not paired: they are not neighbours on one lead. */
    static constexpr float kMaxNeighbourDistance = 0.015f;

    //=========================================================================================================
    /**
     * Pair each contact with its next neighbour on the same lead: along a depth lead or strip,
     * and along the rows and then the columns of a grid with a known shape. The contact earlier
     * along the lead is the anode, so a depth lead gives LA1-LA2, LA2-LA3, …
     *
     * @param[in] implantation  Contacts and leads.
     * @return The derivations, lead by lead.
     */
    static QVector<IeegBipolarPair> bipolarPairs(const IeegImplantation& implantation);

    //=========================================================================================================
    /**
     * Montage matrix of the derivations: bipolar data = M * contact data.
     *
     * @param[in] pairs         Derivations.
     * @param[in] contactCount  Number of contacts, the columns of M.
     * @return Sparse pairs x contacts matrix, +1 at the anode and -1 at the cathode of each row.
     */
    static Eigen::SparseMatrix<double> bipolarMatrix(const QVector<IeegBipolarPair>& pairs, int contactCount);
};

} // namespace IEEGLIB

#endif // IEEG_REFERENCE_H
