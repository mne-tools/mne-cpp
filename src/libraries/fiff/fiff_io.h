//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *   Florian Schlembach <fschlembach@web.de>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fiff_io.h
 * @since November 2013
 * @brief High-level convenience reader/writer that loads a whole FIFF measurement file into FIFFLIB containers in one call.
 *
 * Most callers want to open a FIFF file and immediately get back a
 * @ref FiffRawData (for ``*-raw.fif''), a @ref FiffEvokedSet (for
 * ``*-ave.fif''), a @ref FiffCov (for ``*-cov.fif''), or the
 * appropriate combination without micromanaging @ref FiffStream and the
 * directory tree. @ref FiffIO provides exactly that one-call facade: it
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
#include "fiff_info.h"
#include "fiff_types.h"

#include "fiff_raw_data.h"
#include "fiff_evoked.h"

#include "fiff_stream.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QFile>
#include <QIODevice>
#include <QSharedPointer>
#include <QDebug>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * @brief One-call FIFF reader/writer: opens a file, dispatches to the right specialized loader and returns ready-to-use FIFFLIB containers.
 *
 * Sniffs the FIFF block tree, dispatches @ref FiffRawData /
 * @ref FiffEvokedSet / @ref FiffCov / @ref FiffInfo loaders as
 * appropriate, and returns shared pointers so the resulting objects can
 * be freely passed across the rest of the pipeline. Front-end parity
 * with @c mne.io.read_raw_fif and friends.
 */
class FIFFSHARED_EXPORT FiffIO : public QObject
{
public:
//    enum Type {
//        _RAW = FIFFB_RAW_DATA, //102
//        _EVOKED = FIFFB_EVOKED, //104
//        _PROJ = FIFFB_PROJ, //313
//        _FWD = FIFFB_MNE_FORWARD_SOLUTION, //352
//        _COV = FIFFB_MNE_COV, //355
//        _NAMED_MATRIX = FIFFB_MNE_NAMED_MATRIX //357
//    };

    //=========================================================================================================
    /**
     * Constructs a FiffIO
     *
     */
    FiffIO();

    //=========================================================================================================
    /**
     * Destroys the FiffIO.
     */
    ~FiffIO();

    //=========================================================================================================
    /**
     * Constructs a FiffIO object by reading from a I/O device pIODevice.
     *
     * @param[in] pIODevice    A fiff IO device like a fiff QFile or QTCPSocket.
     */
    FiffIO(QIODevice& pIODevice);

    //=========================================================================================================
    /**
     * Constructs a FiffIO object that uses the I/O device pIODevice.
     *
     * @param[in] p_qlistIODevices    A QList of fiff IO devices like a fiff QFile or QTCPSocket.
     */
    FiffIO(QList<QIODevice*>& p_qlistIODevices);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffIO    FiffIO, which should be copied.
     */
    FiffIO(const FiffIO& p_FiffIO);

    //=========================================================================================================
    /**
     * Setup a FiffStream
     *
     * @param[in] pIODevice      An fiff IO device like a fiff QFile or QTCPSocket.
     * @param[in] info           Overall info for fiff IO device.
     * @param[out] dirTree       Node directory structure.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool setup_read(QIODevice& pIODevice,
                           FiffInfo& info,
                           FiffDirNode::SPtr& dirTree);

    //=========================================================================================================
    /**
     * Read data from a pIODevice.
     *
     * @param[in] pIODevice    A fiff IO device like a fiff QFile or QTCPSocket.
     */
    bool read(QIODevice& pIODevice);

    //=========================================================================================================
    /**
     * Read data from a QList of pIODevices.
     *
     * @param[in] p_qlistIODevices    A QList of fiff IO devices like a fiff QFile or QTCPSocket.
     */
    bool read(QList<QIODevice>& p_qlistIODevices);

    //=========================================================================================================
    /**
     * Write data to a single pIODevice.
     *
     * @param[in] pIODevice   A fiff IO device like a fiff QFile or QTCPSocket.
     * @param[in] type        Type of data to write (fiff constants types, e.g. FIFFB_RAW_DATA).
     * @param[in] idx         Index of type, -1 for all entities of this type.
     *
     * @return true if succeeded, false otherwise.
     */
    bool write(QIODevice& pIODevice,
               const fiff_int_t type,
               const fiff_int_t idx) const;

    //=========================================================================================================
    /**
     * Write whole data of a type to a fiff file.
     *
     * @param[in] p_QFile   Output file (name extended with type and index, e.g. sample_audvis-type-1.fif).
     * @param[in] type      Type of data to write (fiff constants types, e.g. FIFFB_RAW_DATA).
     * @param[in] idx       Index of type, -1 for all entities of this type.
     *
     * @return true if succeeded, false otherwise.
     */
    bool write(QFile& p_QFile,
               const fiff_int_t type,
               const fiff_int_t idx) const;

    //=========================================================================================================
    /**
     * Write raw data to a pIODevice.
     *
     * @param[in] pIODevice   A fiff IO device like a fiff QFile or QTCPSocket.
     * @param[in] idx         Index of type, -1 for all entities of this type.
     *
     * @return true if succeeded, false otherwise.
     */
    bool write_raw(QIODevice& pIODevice,
                   const fiff_int_t idx) const;

    //=========================================================================================================
    /**
     * Overloading ostream for printing member infos.
     *
     * @param[in] out       The output stream.
     * @param[in] p_fiffIO  The FiffIO whose members shall be printed.
     *
     * @return The output stream with the FiffIO data appended.
     */

    friend std::ostream& operator<<(std::ostream& out,
                                    const FiffIO &p_fiffIO) {
        out << "\n\n---------------------- Fiff data read summary ---------------------- " << std::endl;
        out << "fiff data contains" << std::endl;
        out << p_fiffIO.m_qlistRaw.size() << " raw data sets" << std::endl;
        out << p_fiffIO.m_qlistEvoked.size() << " evoked sets" << std::endl;
//        out << p_fiffIO.m_qlistFwd.size() << " forward solutions" << std::endl;
        return out;
    }

public:
    QList<QSharedPointer<FiffRawData> >     m_qlistRaw;     /**< List of raw data sets. */
    QList<QSharedPointer<FiffEvoked> >      m_qlistEvoked;  /**< List of evoked data sets. */
//    QList<QSharedPointer<MNEForwardSolution> > m_qlistFwd;
    //FiffCov, MNEInverseOperator, FsAnnotationSet,
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // FIFF_IO_H
