//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_io.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Florian Schlembach <fschlembach@web.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     July 2012
 * @brief    High-level convenience reader/writer that loads a whole FIFF measurement file into mne_fiff containers in one call.
 *
 * Most callers want to open a FIFF file and immediately get back a
 * @ref mne::FiffRawData (for "*-raw.fif"), a @ref mne::FiffEvokedSet (for
 * "*-ave.fif"), a @ref mne::FiffCov (for "*-cov.fif"), or the
 * appropriate combination without micromanaging @ref mne::FiffStream and the
 * directory tree. @ref mne::FiffIO provides exactly that one-call facade: it
 * sniffs the top-level FIFF blocks (@c FIFFB_RAW_DATA, @c FIFFB_EVOKED,
 * @c FIFFB_MNE_COV, ...), invokes the matching specialized reader and
 * exposes the results as ready-to-use shared pointers, with parity to
 * the @c mne.io.read_raw_fif / @c mne.read_evokeds / @c mne.read_cov
 * front-end of MNE-Python.
 */

#ifndef FIFF_IO_H
#define FIFF_IO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"
#include "fiff_raw_data.h"
#include "fiff_evoked.h"
#include "fiff_dir_node.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <filesystem>
#include <memory>
#include <ostream>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief One-call FIFF reader/writer: opens a file, dispatches to the right specialized loader and returns ready-to-use mne_fiff containers.
 *
 * Sniffs the FIFF block tree, dispatches @ref FiffRawData /
 * @ref FiffEvokedSet loaders as appropriate, and returns shared pointers so the
 * resulting objects can be freely passed across the rest of the pipeline. Front-end
 * parity with @c mne.io.read_raw_fif and @c mne.read_evokeds.
 *
 * @snippet ex_fiff_api/main.cpp fiff_io_usage
 */
class FIFFSHARED_EXPORT FiffIO
{
public:
    FiffIO() = default;

    //=========================================================================================================
    /**
     * Reads the raw and evoked data of a file.
     *
     * @param[in] path   The file.
     */
    explicit FiffIO(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads the raw and evoked data of several files.
     *
     * @param[in] paths   The files.
     */
    explicit FiffIO(const std::vector<std::filesystem::path>& paths);

    //=========================================================================================================
    /**
     * Reads the measurement info of a file.
     *
     * @param[in] path       The file.
     * @param[out] info      The measurement info.
     * @param[out] dirTree   The node of the measurement info.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool setup_read(const std::filesystem::path& path, FiffInfo& info, std::shared_ptr<FiffDirNode>& dirTree);

    //=========================================================================================================
    /**
     * Appends the raw and evoked data of a file.
     *
     * @param[in] path   The file.
     *
     * @return false if the file has no directory tree, true otherwise.
     */
    bool read(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Appends the raw data of a FIFF file held in memory; evoked data can only be read from a file.
     *
     * @param[in] bytes   The file contents.
     *
     * @return false if the bytes hold no FIFF directory tree, true otherwise.
     */
    bool read(std::vector<std::byte> bytes);

    //=========================================================================================================
    /**
     * Writes data of one type to a FIFF file.
     *
     * @param[in] path   The file. Raw data sets written all at once go to one file each, named after it with
     *                   "_raw-<index>" before ".fif"; evoked data sets go to one file together.
     * @param[in] type   FIFFB_RAW_DATA or FIFFB_EVOKED.
     * @param[in] idx    Index of the data set, -1 for all.
     *
     * @return true if succeeded, false otherwise.
     */
    bool write(const std::filesystem::path& path, fiff_int_t type, fiff_int_t idx = -1) const;

    //=========================================================================================================
    /**
     * Writes one raw data set to a FIFF file.
     *
     * @param[in] path   The file.
     * @param[in] idx    Index of the raw data set.
     *
     * @return true if succeeded, false otherwise.
     */
    bool write_raw(const std::filesystem::path& path, fiff_int_t idx) const;

    friend std::ostream& operator<<(std::ostream& out, const FiffIO& io)
    {
        out << "\n\n---------------------- Fiff data read summary ---------------------- " << std::endl;
        out << "fiff data contains" << std::endl;
        out << io.raw.size() << " raw data sets" << std::endl;
        out << io.evoked.size() << " evoked sets" << std::endl;
        return out;
    }

    std::vector<std::shared_ptr<FiffRawData>> raw;   /**< Raw data sets. */
    std::vector<std::shared_ptr<FiffEvoked>> evoked; /**< Evoked data sets. */

private:
    /** Reads the info of @p stream and its raw data from @p rawBytes; @return the directory tree of the info, or nullptr. */
    std::shared_ptr<FiffDirNode> readRaw(FiffStream& stream, std::unique_ptr<mne::ByteStream> rawBytes);
};

} // NAMESPACE

#endif // FIFF_IO_H
