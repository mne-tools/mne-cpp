//=============================================================================================================
/**
 * @file     fiff_io.h
 * @author   Florian Schlembach <Florian.Schlembach@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Florian Schlembach, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Definition of a generic Fiff IO interface
 *
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
// FORWARD DECLARATIONS
//=============================================================================================================

namespace RTPROCESSINGLIB
{
    class FilterKernel;
}

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

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
     * @param[in] pIODevice               A fiff IO device like a fiff QFile or QTCPSocket.
     * @param[in] type of data to write  fiff constants types, e.g. FIFFB_RAW_DATA.
     * @param[in] idx                    index of type, -1 for all entities of this type.
     *
     */
    bool write(QIODevice& pIODevice,
               const fiff_int_t type,
               const fiff_int_t idx) const;

    //=========================================================================================================
    /**
     * Write whole data of a type to a fiff file.
     *
     * @param[in] p_QFile                   filename including the path but not the type, e.g. ./sample_date/sample_audvis.fif -> will be extended to ./sample_date/sample_audvis-type-1.fif.
     * @param[in] type of data to write     fiff constants types, e.g. FIFFB_RAW_DATA.
     * @param[in] idx                       index of type, -1 for all entities of this type.
     */
    bool write(QFile& p_QFile,
               const fiff_int_t type,
               const fiff_int_t idx) const;

    //=========================================================================================================
    /**
     * Write raw data to a pIODevice.
     *
     * @param[in] pIODevice               A fiff IO device like a fiff QFile or QTCPSocket.
     * @param[in] idx                    index of type, -1 for all entities of this type.
     *
     */
    bool write_raw(QIODevice& pIODevice,
                   const fiff_int_t idx) const;

    //=========================================================================================================
    /**
     * Overloading ostream for printing member infos
     *
     * @param[in] p_fiffIO    the fiffIO, whose members shall be printed.
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
    QList<QSharedPointer<FiffRawData> >     m_qlistRaw;
    QList<QSharedPointer<FiffEvoked> >      m_qlistEvoked;
//    QList<QSharedPointer<MNEForwardSolution> > m_qlistFwd;
    //FiffCov, MNEInverseOperator, AnnotationSet,
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // FIFF_IO_H
