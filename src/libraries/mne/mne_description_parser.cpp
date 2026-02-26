//=============================================================================================================
/**
 * @file     mne_description_parser.cpp
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
 * @brief    MNEDescriptionParser implementation.
 *           Ported from interpret.c by Matti Hamalainen.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_description_parser.h"

#include <QFile>
#include <QDebug>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

void MNEDescriptionParser::skipComments(QTextStream &in)
{
    while (!in.atEnd()) {
        QChar c;
        in >> c;
        if (c == '#') {
            in.readLine(); // consume rest of comment line
        } else {
            // push back by seeking
            in.seek(in.pos() - 1);
            return;
        }
    }
}

//=============================================================================================================

QString MNEDescriptionParser::nextWord(QTextStream &in)
{
    skipComments(in);

    // Skip whitespace
    while (!in.atEnd()) {
        QChar c;
        in >> c;
        if (!c.isSpace()) {
            // Check if quoted string
            if (c == '"') {
                QString word;
                while (!in.atEnd()) {
                    in >> c;
                    if (c == '"') break;
                    word.append(c);
                }
                return word;
            }
            // Regular word
            QString word;
            word.append(c);
            while (!in.atEnd()) {
                in >> c;
                if (c.isSpace()) break;
                if (c == '#') {
                    in.readLine();
                    break;
                }
                word.append(c);
            }
            return word;
        }
    }
    return QString(); // EOF
}

//=============================================================================================================

bool MNEDescriptionParser::getInt(QTextStream &in, int &val)
{
    QString word = nextWord(in);
    if (word.isEmpty()) {
        qWarning() << "MNEDescriptionParser: expected integer, got EOF";
        return false;
    }
    bool ok;
    val = word.toInt(&ok);
    if (!ok) {
        qWarning() << "MNEDescriptionParser: bad integer:" << word;
        return false;
    }
    return true;
}

//=============================================================================================================

bool MNEDescriptionParser::getFloat(QTextStream &in, float &val)
{
    QString word = nextWord(in);
    if (word.isEmpty()) {
        qWarning() << "MNEDescriptionParser: expected float, got EOF";
        return false;
    }
    bool ok;
    val = word.toFloat(&ok);
    if (!ok) {
        qWarning() << "MNEDescriptionParser: bad float:" << word;
        return false;
    }
    return true;
}

//=============================================================================================================

bool MNEDescriptionParser::parseRejectionParam(const QString &keyword, QTextStream &in,
                                               RejectionParams &rej, bool &ok)
{
    ok = true;
    float fval;

    if (keyword.compare("gradReject", Qt::CaseInsensitive) == 0) {
        ok = getFloat(in, fval); if (ok) rej.megGradReject = fval;
    } else if (keyword.compare("magReject", Qt::CaseInsensitive) == 0) {
        ok = getFloat(in, fval); if (ok) rej.megMagReject = fval;
    } else if (keyword.compare("eegReject", Qt::CaseInsensitive) == 0) {
        ok = getFloat(in, fval); if (ok) rej.eegReject = fval;
    } else if (keyword.compare("eogReject", Qt::CaseInsensitive) == 0) {
        ok = getFloat(in, fval); if (ok) rej.eogReject = fval;
    } else if (keyword.compare("ecgReject", Qt::CaseInsensitive) == 0) {
        ok = getFloat(in, fval); if (ok) rej.ecgReject = fval;
    } else if (keyword.compare("gradFlat", Qt::CaseInsensitive) == 0) {
        ok = getFloat(in, fval); if (ok) rej.megGradFlat = fval;
    } else if (keyword.compare("magFlat", Qt::CaseInsensitive) == 0) {
        ok = getFloat(in, fval); if (ok) rej.megMagFlat = fval;
    } else if (keyword.compare("eegFlat", Qt::CaseInsensitive) == 0) {
        ok = getFloat(in, fval); if (ok) rej.eegFlat = fval;
    } else if (keyword.compare("eogFlat", Qt::CaseInsensitive) == 0) {
        ok = getFloat(in, fval); if (ok) rej.eogFlat = fval;
    } else if (keyword.compare("ecgFlat", Qt::CaseInsensitive) == 0) {
        ok = getFloat(in, fval); if (ok) rej.ecgFlat = fval;
    } else if (keyword.compare("stimIgnore", Qt::CaseInsensitive) == 0) {
        ok = getFloat(in, fval); if (ok) rej.stimIgnore = std::fabs(fval);
    } else {
        return false; // not a rejection keyword
    }
    return true; // was a rejection keyword (ok indicates parse success)
}

//=============================================================================================================
// AVERAGE DESCRIPTION PARSER
//=============================================================================================================

bool MNEDescriptionParser::parseAverageFile(const QString &fileName, AverageDescription &desc)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "MNEDescriptionParser: cannot open" << fileName;
        return false;
    }

    QTextStream in(&file);
    desc = AverageDescription();

    bool expectBrace = false;
    bool bminSet = false, bmaxSet = false;
    bool inAverage = false;
    bool inCategory = false;
    AverageCategory currentCat;

    QString word;
    while (!(word = nextWord(in)).isEmpty()) {
        if (expectBrace) {
            if (word == "{") {
                expectBrace = false;
            } else {
                qWarning() << "MNEDescriptionParser: expected '{', got" << word;
                return false;
            }
            continue;
        }

        if (word == "{") {
            qWarning() << "MNEDescriptionParser: unexpected '{'";
            return false;
        }

        if (word == "}") {
            if (inCategory) {
                currentCat.doBaseline = bminSet && bmaxSet;
                // Validate category
                if (currentCat.comment.isEmpty()) {
                    qWarning() << "MNEDescriptionParser: category name missing";
                    return false;
                }
                if (currentCat.tmin >= currentCat.tmax) {
                    qWarning() << "MNEDescriptionParser: illegal time range for" << currentCat.comment;
                    return false;
                }
                if (currentCat.events.isEmpty()) {
                    qWarning() << "MNEDescriptionParser: no events for" << currentCat.comment;
                    return false;
                }
                if (!currentCat.prevIgnore) currentCat.prevIgnore = currentCat.ignore;
                if (!currentCat.nextIgnore) currentCat.nextIgnore = currentCat.ignore;
                desc.categories.append(currentCat);
                currentCat = AverageCategory();
                inCategory = false;
                bminSet = bmaxSet = false;
            } else if (inAverage) {
                if (desc.comment.isEmpty()) desc.comment = "Average";
                inAverage = false;
                break; // done
            }
            continue;
        }

        // Top-level: "average" keyword
        if (word.compare("average", Qt::CaseInsensitive) == 0) {
            if (inAverage || inCategory) {
                qWarning() << "MNEDescriptionParser: nested average";
                return false;
            }
            inAverage = true;
            expectBrace = true;
            continue;
        }

        // Category start
        if (word.compare("category", Qt::CaseInsensitive) == 0 ||
            word.compare("condition", Qt::CaseInsensitive) == 0) {
            if (!inAverage || inCategory) {
                qWarning() << "MNEDescriptionParser: misplaced category";
                return false;
            }
            inCategory = true;
            expectBrace = true;
            bminSet = bmaxSet = false;
            continue;
        }

        // "name" keyword
        if (word.compare("name", Qt::CaseInsensitive) == 0) {
            QString val = nextWord(in);
            if (val.isEmpty()) { qWarning() << "MNEDescriptionParser: name requires a value"; return false; }
            if (inCategory) currentCat.comment = val;
            else if (inAverage) desc.comment = val;
            else { qWarning() << "MNEDescriptionParser: misplaced name"; return false; }
            continue;
        }

        // "outfile" keyword
        if (word.compare("outfile", Qt::CaseInsensitive) == 0) {
            QString val = nextWord(in);
            if (val.isEmpty()) { qWarning() << "MNEDescriptionParser: outfile requires a value"; return false; }
            if (inAverage && !inCategory) desc.filename = val;
            else { qWarning() << "MNEDescriptionParser: misplaced outfile"; return false; }
            continue;
        }

        // "eventfile" keyword
        if (word.compare("eventfile", Qt::CaseInsensitive) == 0) {
            QString val = nextWord(in);
            if (val.isEmpty()) { qWarning() << "MNEDescriptionParser: eventfile requires a value"; return false; }
            if (inAverage && !inCategory) desc.eventFile = val;
            else { qWarning() << "MNEDescriptionParser: misplaced eventfile"; return false; }
            continue;
        }

        // "logfile" keyword
        if (word.compare("logfile", Qt::CaseInsensitive) == 0) {
            QString val = nextWord(in);
            if (val.isEmpty()) { qWarning() << "MNEDescriptionParser: logfile requires a value"; return false; }
            if (inAverage && !inCategory) desc.logFile = val;
            else { qWarning() << "MNEDescriptionParser: misplaced logfile"; return false; }
            continue;
        }

        // "fixskew" keyword
        if (word.compare("fixskew", Qt::CaseInsensitive) == 0) {
            if (inAverage && !inCategory) desc.fixSkew = true;
            else { qWarning() << "MNEDescriptionParser: misplaced fixskew"; return false; }
            continue;
        }

        // --- Category-level keywords ---
        if (word.compare("tmin", Qt::CaseInsensitive) == 0) {
            float fval; if (!getFloat(in, fval)) return false;
            if (inCategory) currentCat.tmin = fval;
            else { qWarning() << "MNEDescriptionParser: misplaced tmin"; return false; }
            continue;
        }
        if (word.compare("tmax", Qt::CaseInsensitive) == 0) {
            float fval; if (!getFloat(in, fval)) return false;
            if (inCategory) currentCat.tmax = fval;
            else { qWarning() << "MNEDescriptionParser: misplaced tmax"; return false; }
            continue;
        }
        if (word.compare("basemin", Qt::CaseInsensitive) == 0 ||
            word.compare("bmin", Qt::CaseInsensitive) == 0) {
            float fval; if (!getFloat(in, fval)) return false;
            if (inCategory) { currentCat.bmin = fval; bminSet = true; }
            else { qWarning() << "MNEDescriptionParser: misplaced basemin"; return false; }
            continue;
        }
        if (word.compare("basemax", Qt::CaseInsensitive) == 0 ||
            word.compare("bmax", Qt::CaseInsensitive) == 0) {
            float fval; if (!getFloat(in, fval)) return false;
            if (inCategory) { currentCat.bmax = fval; bmaxSet = true; }
            else { qWarning() << "MNEDescriptionParser: misplaced basemax"; return false; }
            continue;
        }
        if (word.compare("event", Qt::CaseInsensitive) == 0) {
            int ival; if (!getInt(in, ival)) return false;
            if (ival <= 0) { qWarning() << "MNEDescriptionParser: event numbers must be positive"; return false; }
            if (inCategory) currentCat.events.append(static_cast<unsigned int>(ival));
            else { qWarning() << "MNEDescriptionParser: misplaced event"; return false; }
            continue;
        }
        if (word.compare("nextevent", Qt::CaseInsensitive) == 0) {
            int ival; if (!getInt(in, ival)) return false;
            if (ival <= 0) { qWarning() << "MNEDescriptionParser: nextevent must be positive"; return false; }
            if (inCategory) currentCat.nextEvent = static_cast<unsigned int>(ival);
            else { qWarning() << "MNEDescriptionParser: misplaced nextevent"; return false; }
            continue;
        }
        if (word.compare("prevevent", Qt::CaseInsensitive) == 0) {
            int ival; if (!getInt(in, ival)) return false;
            if (ival <= 0) { qWarning() << "MNEDescriptionParser: prevevent must be positive"; return false; }
            if (inCategory) currentCat.prevEvent = static_cast<unsigned int>(ival);
            else { qWarning() << "MNEDescriptionParser: misplaced prevevent"; return false; }
            continue;
        }
        if (word.compare("ignore", Qt::CaseInsensitive) == 0) {
            int ival; if (!getInt(in, ival)) return false;
            if (inCategory) currentCat.ignore = static_cast<unsigned int>(ival);
            else { qWarning() << "MNEDescriptionParser: misplaced ignore"; return false; }
            continue;
        }
        if (word.compare("prevignore", Qt::CaseInsensitive) == 0) {
            int ival; if (!getInt(in, ival)) return false;
            if (inCategory) currentCat.prevIgnore = static_cast<unsigned int>(ival);
            else { qWarning() << "MNEDescriptionParser: misplaced prevignore"; return false; }
            continue;
        }
        if (word.compare("nextignore", Qt::CaseInsensitive) == 0) {
            int ival; if (!getInt(in, ival)) return false;
            if (inCategory) currentCat.nextIgnore = static_cast<unsigned int>(ival);
            else { qWarning() << "MNEDescriptionParser: misplaced nextignore"; return false; }
            continue;
        }
        if (word.compare("mask", Qt::CaseInsensitive) == 0) {
            int ival; if (!getInt(in, ival)) return false;
            if (ival <= 0) { qWarning() << "MNEDescriptionParser: mask must be positive"; return false; }
            if (inCategory) { currentCat.ignore = static_cast<unsigned int>(ival); currentCat.ignore = ~currentCat.ignore; }
            else { qWarning() << "MNEDescriptionParser: misplaced mask"; return false; }
            continue;
        }
        if (word.compare("prevmask", Qt::CaseInsensitive) == 0) {
            int ival; if (!getInt(in, ival)) return false;
            if (inCategory) { currentCat.prevIgnore = static_cast<unsigned int>(ival); currentCat.prevIgnore = ~currentCat.prevIgnore; }
            else { qWarning() << "MNEDescriptionParser: misplaced prevmask"; return false; }
            continue;
        }
        if (word.compare("nextmask", Qt::CaseInsensitive) == 0) {
            int ival; if (!getInt(in, ival)) return false;
            if (inCategory) { currentCat.nextIgnore = static_cast<unsigned int>(ival); currentCat.nextIgnore = ~currentCat.nextIgnore; }
            else { qWarning() << "MNEDescriptionParser: misplaced nextmask"; return false; }
            continue;
        }
        if (word.compare("delay", Qt::CaseInsensitive) == 0) {
            float fval; if (!getFloat(in, fval)) return false;
            if (inCategory) currentCat.delay = fval;
            else { qWarning() << "MNEDescriptionParser: misplaced delay"; return false; }
            continue;
        }
        if (word.compare("stderr", Qt::CaseInsensitive) == 0) {
            if (inCategory) currentCat.doStdErr = true;
            else { qWarning() << "MNEDescriptionParser: misplaced stderr"; return false; }
            continue;
        }
        if (word.compare("abs", Qt::CaseInsensitive) == 0) {
            if (inCategory) currentCat.doAbs = true;
            else { qWarning() << "MNEDescriptionParser: misplaced abs"; return false; }
            continue;
        }
        if (word.compare("color", Qt::CaseInsensitive) == 0) {
            float r, g, b;
            if (!getFloat(in, r) || !getFloat(in, g) || !getFloat(in, b)) return false;
            if (inCategory) { currentCat.color[0] = r; currentCat.color[1] = g; currentCat.color[2] = b; }
            else { qWarning() << "MNEDescriptionParser: misplaced color"; return false; }
            continue;
        }

        // Try rejection parameters (average-level only)
        if (inAverage && !inCategory) {
            bool parseOk;
            if (parseRejectionParam(word, in, desc.rej, parseOk)) {
                if (!parseOk) return false;
                continue;
            }
        }

        qWarning() << "MNEDescriptionParser: unknown keyword" << word << "in" << fileName;
    }

    if (desc.categories.isEmpty()) {
        qWarning() << "MNEDescriptionParser: no categories found in" << fileName;
        return false;
    }

    return true;
}

//=============================================================================================================
// COVARIANCE DESCRIPTION PARSER
//=============================================================================================================

bool MNEDescriptionParser::parseCovarianceFile(const QString &fileName, CovDescription &desc)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "MNEDescriptionParser: cannot open" << fileName;
        return false;
    }

    QTextStream in(&file);
    desc = CovDescription();

    bool expectBrace = false;
    bool bminSet = false, bmaxSet = false;
    bool inCov = false;
    bool inDef = false;
    CovDefinition currentDef;

    QString word;
    while (!(word = nextWord(in)).isEmpty()) {
        if (expectBrace) {
            if (word == "{") {
                expectBrace = false;
            } else {
                qWarning() << "MNEDescriptionParser: expected '{', got" << word;
                return false;
            }
            continue;
        }

        if (word == "{") {
            qWarning() << "MNEDescriptionParser: unexpected '{'";
            return false;
        }

        if (word == "}") {
            if (inDef) {
                currentDef.doBaseline = bminSet && bmaxSet;
                if (currentDef.tmin >= currentDef.tmax) {
                    qWarning() << "MNEDescriptionParser: illegal time range in def";
                    return false;
                }
                desc.defs.append(currentDef);
                currentDef = CovDefinition();
                inDef = false;
                bminSet = bmaxSet = false;
            } else if (inCov) {
                inCov = false;
                break;
            }
            continue;
        }

        if (word.compare("cov", Qt::CaseInsensitive) == 0) {
            if (inCov || inDef) {
                qWarning() << "MNEDescriptionParser: nested cov";
                return false;
            }
            inCov = true;
            expectBrace = true;
            continue;
        }

        if (word.compare("def", Qt::CaseInsensitive) == 0) {
            if (!inCov || inDef) {
                qWarning() << "MNEDescriptionParser: misplaced def";
                return false;
            }
            inDef = true;
            expectBrace = true;
            bminSet = bmaxSet = false;
            continue;
        }

        // Cov-level keywords
        if (word.compare("outfile", Qt::CaseInsensitive) == 0) {
            QString val = nextWord(in);
            if (inCov && !inDef) desc.filename = val;
            else { qWarning() << "MNEDescriptionParser: misplaced outfile"; return false; }
            continue;
        }
        if (word.compare("eventfile", Qt::CaseInsensitive) == 0) {
            QString val = nextWord(in);
            if (inCov && !inDef) desc.eventFile = val;
            else { qWarning() << "MNEDescriptionParser: misplaced eventfile"; return false; }
            continue;
        }
        if (word.compare("logfile", Qt::CaseInsensitive) == 0) {
            QString val = nextWord(in);
            if (inCov && !inDef) desc.logFile = val;
            else { qWarning() << "MNEDescriptionParser: misplaced logfile"; return false; }
            continue;
        }
        if (word.compare("keepsamplemean", Qt::CaseInsensitive) == 0) {
            if (inCov && !inDef) desc.removeSampleMean = false;
            else { qWarning() << "MNEDescriptionParser: misplaced keepsamplemean"; return false; }
            continue;
        }
        if (word.compare("fixskew", Qt::CaseInsensitive) == 0) {
            if (inCov && !inDef) desc.fixSkew = true;
            else { qWarning() << "MNEDescriptionParser: misplaced fixskew"; return false; }
            continue;
        }

        // Def-level keywords
        if (word.compare("tmin", Qt::CaseInsensitive) == 0) {
            float fval; if (!getFloat(in, fval)) return false;
            if (inDef) currentDef.tmin = fval;
            else { qWarning() << "MNEDescriptionParser: misplaced tmin"; return false; }
            continue;
        }
        if (word.compare("tmax", Qt::CaseInsensitive) == 0) {
            float fval; if (!getFloat(in, fval)) return false;
            if (inDef) currentDef.tmax = fval;
            else { qWarning() << "MNEDescriptionParser: misplaced tmax"; return false; }
            continue;
        }
        if (word.compare("basemin", Qt::CaseInsensitive) == 0 ||
            word.compare("bmin", Qt::CaseInsensitive) == 0) {
            float fval; if (!getFloat(in, fval)) return false;
            if (inDef) { currentDef.bmin = fval; bminSet = true; }
            else { qWarning() << "MNEDescriptionParser: misplaced basemin"; return false; }
            continue;
        }
        if (word.compare("basemax", Qt::CaseInsensitive) == 0 ||
            word.compare("bmax", Qt::CaseInsensitive) == 0) {
            float fval; if (!getFloat(in, fval)) return false;
            if (inDef) { currentDef.bmax = fval; bmaxSet = true; }
            else { qWarning() << "MNEDescriptionParser: misplaced basemax"; return false; }
            continue;
        }
        if (word.compare("event", Qt::CaseInsensitive) == 0) {
            int ival; if (!getInt(in, ival)) return false;
            if (ival <= 0) { qWarning() << "MNEDescriptionParser: event must be positive"; return false; }
            if (inDef) currentDef.events.append(static_cast<unsigned int>(ival));
            else { qWarning() << "MNEDescriptionParser: misplaced event"; return false; }
            continue;
        }
        if (word.compare("ignore", Qt::CaseInsensitive) == 0) {
            int ival; if (!getInt(in, ival)) return false;
            if (inDef) currentDef.ignore = static_cast<unsigned int>(ival);
            else { qWarning() << "MNEDescriptionParser: misplaced ignore"; return false; }
            continue;
        }
        if (word.compare("mask", Qt::CaseInsensitive) == 0) {
            int ival; if (!getInt(in, ival)) return false;
            if (inDef) { currentDef.ignore = static_cast<unsigned int>(ival); currentDef.ignore = ~currentDef.ignore; }
            else { qWarning() << "MNEDescriptionParser: misplaced mask"; return false; }
            continue;
        }
        if (word.compare("delay", Qt::CaseInsensitive) == 0) {
            float fval; if (!getFloat(in, fval)) return false;
            if (inDef) currentDef.delay = fval;
            else { qWarning() << "MNEDescriptionParser: misplaced delay"; return false; }
            continue;
        }

        // Rejection parameters (cov-level only)
        if (inCov && !inDef) {
            bool parseOk;
            if (parseRejectionParam(word, in, desc.rej, parseOk)) {
                if (!parseOk) return false;
                continue;
            }
        }

        qWarning() << "MNEDescriptionParser: unknown keyword" << word << "in" << fileName;
    }

    return true;
}
