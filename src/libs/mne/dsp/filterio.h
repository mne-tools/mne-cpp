//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     filterio.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     April 2015
 * @brief    Plain-text serialisation of @ref mne::FilterKernel coefficient sets.
 *
 * FilterIO reads and writes the simple human-readable text format used by
 * mne-cpp tooling to persist designed FIR / IIR coefficient sets: header
 * lines carry the design parameters (filter type, design method, cutoff
 * frequencies, transition bandwidth, sampling frequency, tap count), and
 * the body holds the numerator (and, for IIR, denominator) coefficients
 * one per line. This makes it trivial to inspect, version-control or hand-
 * edit a filter without having to rerun the design step.
 *
 * Only the kernel coefficients and their design metadata are stored — no
 * runtime state, channel selection or overlap-add buffer is serialised.
 */

#ifndef FILTERIO_H
#define FILTERIO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"
#include "filterkernel.h"
#include <mne/base/error.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Reads and writes FIR filters as text: "#key value" lines (sFreq, name, type, order, HPFreq, LPFreq,
 * CenterFreq, DesignMethod, ParksWidth) followed by one coefficient per line.
 *
 * @brief Processes txt files which hold filter coefficients.
 *
 * @snippet ex_dsp_filter/main.cpp filter_io_round_trip
 */
class DSPSHARED_EXPORT FilterIO
{
public:
    //=========================================================================================================
    /**
     * @param[in] path   The filter file.
     *
     * @return The filter, with as many taps as the file has coefficients, or an error if the file cannot be read
     *         or holds a value that is no number.
     */
    static Expected<FilterKernel> read_filter(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Writes the coefficients with all the digits they need to be read back exactly.
     *
     * @param[in] path     The filter file to write.
     * @param[in] filter   The filter.
     *
     * @return Nothing, or an error if the file cannot be written.
     */
    static Expected<void> write_filter(const std::filesystem::path& path, const FilterKernel& filter);
};

} // namespace mne

#endif // FILTERIO_H
