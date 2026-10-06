//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_deriv_set.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Set of channel derivations (montages): MNE-C derivation files, text definitions and matching to recordings.
 *
 * @ref MNELIB::MNEDerivSet is the C++ counterpart of MNE-C's @c mneDerivSet
 * (@c mne_derivations.c, @c mne_make_derivations). A derivation defines a
 * virtual channel as a weighted sum of recorded channels, e.g. a bipolar
 * pair or an average reference. Sets are read and written as
 * @c FIFFB_MNE_DERIVATIONS blocks and from MNE-C's text syntax, and are
 * matched to the channels of a recording before raw data are read through them.
 */

#ifndef MNEDERIVSET_H
#define MNEDERIVSET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_deriv.h"

#include <fiff/fiff_ch_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <optional>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * @brief Collection of channel derivations.
 *
 * @snippet ex_mne_api/main.cpp mne_deriv_set_usage
 */
class MNESHARED_EXPORT MNEDerivSet
{
public:
    /** Derived channel name and the weight of every input channel. */
    using Definition = QPair<QString, QMap<QString, double>>;

    MNEDerivSet() = default;
    MNEDerivSet(MNEDerivSet&&) = default;
    MNEDerivSet& operator=(MNEDerivSet&&) = default;
    ~MNEDerivSet() = default;

    //=========================================================================================================
    /**
     * Copies the set including every derivation.
     *
     * @param[in] other   The set to copy.
     */
    MNEDerivSet(const MNEDerivSet& other)
    {
        append(other);
    }

    //=========================================================================================================
    /**
     * Replaces the derivations by copies of those of another set.
     *
     * @param[in] other   The set to copy.
     *
     * @return This set.
     */
    MNEDerivSet& operator=(const MNEDerivSet& other)
    {
        if (this != &other) {
            derivs.clear();
            append(other);
        }
        return *this;
    }

    //=========================================================================================================
    /**
     * Reads every derivation stored in the @c FIFFB_MNE_DERIVATIONS blocks of a file (MNE-C @c mne_read_deriv).
     *
     * @param[in] path   A derivation file written by write() or MNE-C @c mne_make_derivations.
     *
     * @return The set (empty if the file holds no derivations), or no value if the file cannot be read.
     */
    static std::optional<MNEDerivSet> read(const QString& path);

    //=========================================================================================================
    /**
     * Reads MNE-C's text syntax (@c mne_make_derivations --in).
     *
     * Each derivation is @c name = followed by terms joined by + and -; a term is a
     * channel name optionally preceded by a weight and *, e.g.
     * @c "EEG 001-EEG 002" = "EEG 001" - "EEG 002". Names with spaces are quoted,
     * tokens are separated by white space and @c # starts a comment line. Weights of
     * the same input channel add up; derivations whose weights are all zero are dropped.
     *
     * @param[in] path   The text file.
     *
     * @return One derivation holding all definitions, or no value on a syntax error.
     */
    static std::optional<MNEDerivSet> readText(const QString& path);

    //=========================================================================================================
    /**
     * Builds one derivation from definitions such as those of dsp::ChannelDerivation.
     *
     * @param[in] definitions   Derived channel names with the weights of their inputs.
     * @param[in] name          Short name of the derivation.
     *
     * @return A set with one derivation; input channels appear in order of first use.
     */
    static MNEDerivSet fromDefinitions(const QList<Definition>& definitions, const QString& name = QString());

    //=========================================================================================================
    /**
     * Writes the set as a @c FIFFB_MNE_DERIVATIONS block (MNE-C @c mne_write_deriv_file).
     *
     * @param[in] path   The output file.
     *
     * @return True if the file was written; an empty set is not written.
     */
    bool write(const QString& path) const;

    //=========================================================================================================
    /**
     * Returns every derivation of the set as definitions, in file order.
     *
     * @return Derived channel names with the weights of their inputs.
     */
    QList<Definition> definitions() const;

    //=========================================================================================================
    /**
     * Appends copies of the derivations of another set (MNE-C @c mne_merge_deriv_sets).
     *
     * @param[in] other   The derivations to append.
     */
    void append(const MNEDerivSet& other);

    //=========================================================================================================
    /**
     * Keeps the derivations whose inputs are all among @p chNames and merges them into one
     * derivation whose columns are exactly @p chNames (MNE-C @c mne_match_merge_deriv).
     *
     * @param[in] chNames   Channel names of a recording, in data order.
     *
     * @return The merged derivation with MNEDeriv::in_use set, or nullptr if nothing matched.
     */
    std::unique_ptr<MNEDeriv> match(const QStringList& chNames) const;

    //=========================================================================================================
    /**
     * Returns the number of derived channels over all derivations (MNE-C @c mne_count_deriv).
     *
     * @return The total number of rows.
     */
    int count() const;

    std::vector<std::unique_ptr<MNEDeriv>> derivs; /**< The derivations, owned. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEDERIVSET_H
