//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_ecd_set.h
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Ordered set of @ref mne::InvEcd records — the result of a sequential dipole-fit run.
 *
 * @ref mne::InvEcdSet collects one @ref mne::InvEcd per fitted time bin
 * and exposes append, index access and stream-insertion as well as
 * read/write support for the canonical @c .dip text format (mrilab) and
 * the @c .bdip binary format (xfit). It is the value type returned by
 * @ref mne::InvDipoleFit::calculateFit "InvDipoleFit::calculateFit" and serialised by the
 * @c mne_dipole_fit driver.
 */

#ifndef INV_ECD_SET_H
#define INV_ECD_SET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"
#include "inv_ecd.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// mne FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Implements Electric Current Dipole Set (Replaces *ecdSet,ecdSetRec struct of MNE-C fit_types.h).
 *
 * @brief Holds a set of Electric Current Dipoles.
 *
 * @snippet ex_inv_dipole_fit/main.cpp inv_dipole_fit_usage
 */

class INVSHARED_EXPORT InvEcdSet
{
public:
    //=========================================================================================================
    /**
     * Read dipoles from the dip format compatible with mrilab, as written by save_dipoles_dip,
     * MNE-C and MNE-Python (whose khi^2 and free columns are read too).
     *
     * @param[in] path   File to read from.
     *
     * @return The dipole set read from the file; empty if the file could not be read or holds a malformed line.
     */
    static InvEcdSet read_dipoles_dip(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Save dipoles in the bdip format employed by xfit
     *
     * @param[in] path   File to save to.
     *
     * @return True on success or if there is nothing to save, false if the file could not be written.
     */
    [[nodiscard]] bool save_dipoles_bdip(const std::filesystem::path& path) const;

    //=========================================================================================================
    /**
     * Save dipoles in the dip format suitable for mrilab
     *
     * @param[in] path   File to save to.
     *
     * @return True on success or if there is nothing to save, false if the file could not be written.
     */
    [[nodiscard]] bool save_dipoles_dip(const std::filesystem::path& path) const;

    /**
     * Appends a dipole.
     *
     * @param[in] ecd   The dipole to append.
     */
    void push_back(const InvEcd& ecd)
    {
        m_dipoles.push_back(ecd);
    }

    /** @return The number of dipoles. */
    int size() const
    {
        return static_cast<int>(m_dipoles.size());
    }

    /** @return Whether the set holds no dipoles. */
    bool empty() const
    {
        return m_dipoles.empty();
    }

    /**
     * @param[in] idx   Index of the dipole, less than size().
     *
     * @return The dipole at @p idx.
     */
    const InvEcd& operator[](int idx) const
    {
        return m_dipoles[static_cast<std::size_t>(idx)];
    }

    /**
     * @param[in] idx   Index of the dipole, less than size().
     *
     * @return The dipole at @p idx.
     */
    InvEcd& operator[](int idx)
    {
        return m_dipoles[static_cast<std::size_t>(idx)];
    }

    auto begin() const
    {
        return m_dipoles.begin();
    }
    auto end() const
    {
        return m_dipoles.end();
    }
    auto begin()
    {
        return m_dipoles.begin();
    }
    auto end()
    {
        return m_dipoles.end();
    }

    std::string dataname; /**< The associated data file. */

private:
    std::vector<InvEcd> m_dipoles; /**< The fitted dipoles. */
};

} // NAMESPACE mne_inv

#endif // INV_ECD_SET_H
