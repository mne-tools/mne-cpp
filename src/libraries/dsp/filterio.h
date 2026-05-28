//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     filterio.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Plain-text serialisation of @ref FilterKernel coefficient sets.
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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
// DEFINES
//=============================================================================================================

//=============================================================================================================
// DEFINE FORWARD DECLARATIONS
//=============================================================================================================

class FilterKernel;

//=============================================================================================================
/**
 * Processes txt files which hold filter coefficients.
 *
 * @brief Processes txt files which hold filter coefficients.
 */
class DSPSHARED_EXPORT FilterIO
{
public:
    typedef QSharedPointer<FilterIO> SPtr;            /**< Shared pointer type for FilterIO. */
    typedef QSharedPointer<const FilterIO> ConstSPtr; /**< Const shared pointer type for FilterIO. */

    //=========================================================================================================
    /**
     * Constructs a FilterIO object.
     */
    FilterIO();

    //=========================================================================================================
    /**
     * Reads a given txt file and scans it for filter coefficients. Pls see sample file for file syntax.
     *
     * @param[in] path holds the file path of the txt file which is to be read.
     * @param[in, out] filter holds the filter which the read parameters are to be saved to.
     *
     * @return true if reading was successful, false otherwise.
     */
    static bool readFilter(QString path, FilterKernel &filter);

    //=========================================================================================================
    /**
     * Writes a given filter to txt file .
     *
     * @param[in] path holds the file path of the txt file which is to be written to.
     * @param[in] filter holds the filter which is to be written to file.
     *
     * @return true if reading was successful, false otherwise.
     */
    static bool writeFilter(const QString &path, const FilterKernel &filter);

private:
};
} // NAMESPACE UTILSLIB

#endif // FILTERIO_H
