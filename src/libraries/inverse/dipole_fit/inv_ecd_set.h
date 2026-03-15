//=============================================================================================================
/**
 * @file     inv_ecd_set.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief     FiffDigPointSet class declaration.
 *
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
