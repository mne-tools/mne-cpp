//=============================================================================================================
/**
 * @file     filterio.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    FilterIO class declaration.
 *
 */

#ifndef FILTERIO_H
#define FILTERIO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../rtprocessing_global.h"
#include "filterkernel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
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
class RTPROCESINGSHARED_EXPORT FilterIO
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
} // NAMESPACE RTPROCESSINGLIB

#endif // FILTERIO_H
