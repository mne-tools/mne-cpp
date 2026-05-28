//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_ecd_set.h
 * @since March 2026
 * @brief Ordered set of @ref INVLIB::InvEcd records — the result of a sequential dipole-fit run.
 *
 * @ref INVLIB::InvEcdSet collects one @ref InvEcd per fitted time bin
 * and exposes append, index access and stream-insertion as well as
 * read/write support for the canonical @c .dip text format (mrilab) and
 * the @c .bdip binary format (xfit). It is the value type returned by
 * @ref InvDipoleFit::calculateFit and serialised by the
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
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QList>
#include <QString>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB {

//=============================================================================================================
// FIFFLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Implements Electric Current Dipole Set (Replaces *ecdSet,ecdSetRec struct of MNE-C fit_types.h).
 *
 * @brief Holds a set of Electric Current Dipoles.
 */

class INVSHARED_EXPORT InvEcdSet
{

public:
    typedef QSharedPointer<InvEcdSet> SPtr;            /**< Shared pointer type for InvEcdSet. */
    typedef QSharedPointer<const InvEcdSet> ConstSPtr; /**< Const shared pointer type for InvEcdSet. */

    //=========================================================================================================
    /**
     * Constructs a Electric Current Dipole Set object.
     */
    InvEcdSet();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_ECDSet       Electric Current Dipole Set which should be copied.
     */
    InvEcdSet(const InvEcdSet &p_ECDSet);

    //=========================================================================================================
    /**
     * Destroys the Electric Current Dipole description
     */
    ~InvEcdSet();

    //=========================================================================================================
    /**
     * Appends an Electric Current Dipole to the set
     */
    void addEcd(const InvEcd& p_ecd);

    //=========================================================================================================
    /**
     * Read dipoles from the dip format compatible with mrilab
     *
     * @param[in] name   File name to read from.
     */
    static InvEcdSet read_dipoles_dip(const QString& fileName);

    //=========================================================================================================
    /**
     * Save dipoles in the bdip format employed by xfit
     *
     * @param[in] fileName   File name to save to.
     */
    bool save_dipoles_bdip(const QString& fileName);

    //=========================================================================================================
    /**
     * Save dipoles in the dip format suitable for mrilab
     *
     * @param[in] fileName   File name to save to.
     */
    bool save_dipoles_dip(const QString& fileName) const;

    //=========================================================================================================
    /**
     * Returns the number of stored ECDs
     *
     * @return number of stored ECDs.
     */
    inline qint32 size() const;

    //=========================================================================================================
    /**
     * Subscript operator [] to access InvEcd by index
     *
     * @param[in] idx    the InvEcd index.
     *
     * @return InvEcd related to the parameter index.
     */
    const InvEcd& operator[] (int idx) const;

    //=========================================================================================================
    /**
     * Subscript operator [] to access InvEcd by index
     *
     * @param[in] idx    the InvEcd index.
     *
     * @return InvEcd related to the parameter index.
     */
    InvEcd& operator[] (int idx);

    //=========================================================================================================
    /**
     * Subscript operator << to add a new InvEcd
     *
     * @param[in] p_ecd      InvEcd to be added.
     *
     * @return InvEcdSet.
     */
    InvEcdSet& operator<< (const InvEcd& p_ecd);

public:
    QString dataname;   /**< The associated data file. */

private:
    QList<InvEcd> m_qListDips;     /**< List of Electric Current Dipoles. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 InvEcdSet::size() const
{
    return m_qListDips.size();
}
} // NAMESPACE INVLIB

#ifndef metatype_ecdset
#define metatype_ecdset
Q_DECLARE_METATYPE(INVLIB::InvEcdSet);
#endif

#endif // INV_ECD_SET_H
