//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     fiff_explain.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     December 2016
 * @brief    Human-readable names of FIFF tag kinds, block kinds, units, unit multipliers and channel kinds.
 *
 * Port of MNE-C's @c fiff_explain.c and @c mne_show_fiff explanation table: the names come from the
 * @c fiff_explanations.txt table compiled into the library, the one @c mne_show_fiff has always printed.
 */

#ifndef FIFF_EXPLAIN_H
#define FIFF_EXPLAIN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"

#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * @brief Names of FIFF tag kinds, block kinds, units, unit multipliers and channel kinds.
 *
 * Every lookup returns an empty string for a value the table does not know.
 *
 * @snippet ex_fiff_api/main.cpp fiff_explain_usage
 */
class FIFFSHARED_EXPORT FiffExplain
{
public:
    /**
     * Name of a tag kind (FIFF_*), e.g. "channel" for FIFF_CH_INFO.
     *
     * @param[in] kind   Tag kind.
     * @return The name, or an empty string.
     */
    static QString tag(int kind);

    /**
     * Name of a block kind (FIFFB_*), e.g. "meas. info" for FIFFB_MEAS_INFO.
     *
     * @param[in] kind   Block kind.
     * @return The name, or an empty string.
     */
    static QString block(int kind);

    /**
     * SI symbol of a unit (FIFF_UNIT_*), e.g. "T/m" for FIFF_UNIT_T_M; FIFF_UNIT_NONE has an empty symbol.
     *
     * @param[in] unit   Unit.
     * @return The symbol, or an empty string.
     */
    static QString unit(int unit);

    /**
     * SI prefix of a unit multiplier (FIFF_UNITM_*), e.g. "f" for FIFF_UNITM_F.
     *
     * @param[in] unitMul    Unit multiplier.
     * @return The prefix, or an empty string.
     */
    static QString unitMultiplier(int unitMul);

    /**
     * Name of a channel kind (FIFFV_*_CH), e.g. "EEG" for FIFFV_EEG_CH.
     *
     * @param[in] kind   Channel kind.
     * @return The name, or an empty string.
     */
    static QString channelKind(int kind);
};

} // namespace FIFFLIB

#endif // FIFF_EXPLAIN_H
