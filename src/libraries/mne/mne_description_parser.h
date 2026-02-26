//=============================================================================================================
/**
 * @file     mne_description_parser.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    MNEDescriptionParser class declaration.
 *           Parses MNE-C style .ave and .cov description files.
 *           Ported from interpret.c by Matti Hamalainen.
 *
 */

#ifndef MNE_DESCRIPTION_PARSER_H
#define MNE_DESCRIPTION_PARSER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_process_description.h"

#include <QString>
#include <QTextStream>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * Parses MNE-C style averaging (.ave) and covariance (.cov) description files.
 *
 * File format is a keyword-value text format with nested blocks:
 *   average { ... category { ... } ... }
 *   cov { ... def { ... } ... }
 *
 * Lines starting with '#' are comments.
 */
class MNESHARED_EXPORT MNEDescriptionParser
{
public:
    /**
     * Parse an averaging description file.
     *
     * @param[in] fileName  Path to the .ave file.
     * @param[out] desc     Parsed description.
     * @return true if successful, false on error.
     */
    static bool parseAverageFile(const QString &fileName, AverageDescription &desc);

    /**
     * Parse a covariance description file.
     *
     * @param[in] fileName  Path to the .cov file.
     * @param[out] desc     Parsed description.
     * @return true if successful, false on error.
     */
    static bool parseCovarianceFile(const QString &fileName, CovDescription &desc);

private:
    /**
     * Skip comment lines (starting with '#') and whitespace.
     */
    static void skipComments(QTextStream &in);

    /**
     * Read the next whitespace-delimited word, handling quoted strings.
     * Returns empty string on EOF.
     */
    static QString nextWord(QTextStream &in);

    /**
     * Read and parse an integer value.
     */
    static bool getInt(QTextStream &in, int &val);

    /**
     * Read and parse a float value.
     */
    static bool getFloat(QTextStream &in, float &val);

    /**
     * Parse rejection parameters common to both .ave and .cov files.
     * Returns true if the keyword was handled, false otherwise (unrecognized keyword).
     */
    static bool parseRejectionParam(const QString &keyword, QTextStream &in,
                                    RejectionParams &rej, bool &ok);
};

} // namespace MNELIB

#endif // MNE_DESCRIPTION_PARSER_H
