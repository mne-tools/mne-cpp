//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
 * @brief    Convert LISP-format covariance matrix to FIFF format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_raw_data.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QTextStream>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// LISP S-expression parser
//=============================================================================================================

struct LspToken {
    enum Type { LeftParen, RightParen, Number, Symbol, EndOfFile };
    Type type;
    double numVal;
    QString strVal;
};

//=============================================================================================================

class LspParser {
public:
    explicit LspParser(QTextStream& stream) : m_stream(stream), m_pos(0) {}

    LspToken next()
    {
        skipWhitespace();
        if (m_pos >= m_buffer.size() && !readMore()) {
            return {LspToken::EndOfFile, 0.0, {}};
        }

        QChar c = m_buffer[m_pos];
        if (c == '(') { m_pos++; return {LspToken::LeftParen, 0.0, {"("}}; }
        if (c == ')') { m_pos++; return {LspToken::RightParen, 0.0, {")"}}; }

        // Read a token (number or symbol)
        QString tok;
        while (m_pos < m_buffer.size()) {
            QChar ch = m_buffer[m_pos];
            if (ch.isSpace() || ch == '(' || ch == ')') break;
            tok += ch;
            m_pos++;
        }

        bool ok = false;
        double val = tok.toDouble(&ok);
        if (ok) return {LspToken::Number, val, tok};
        return {LspToken::Symbol, 0.0, tok};
    }

private:
    void skipWhitespace()
    {
        while (true) {
            while (m_pos < m_buffer.size() && m_buffer[m_pos].isSpace()) m_pos++;
            if (m_pos < m_buffer.size()) return;
            if (!readMore()) return;
        }
    }

    bool readMore()
    {
        if (m_stream.atEnd()) return false;
        m_buffer = m_stream.readLine();
        // Strip comments (lines starting with ; or #)
        if (m_buffer.startsWith(';') || m_buffer.startsWith('#')) {
            m_buffer.clear();
        }
        m_pos = 0;
        return !m_buffer.isEmpty() || !m_stream.atEnd();
    }

    QTextStream& m_stream;
    QString m_buffer;
    int m_pos;
};

//=============================================================================================================

static bool parseLspMatrix(QTextStream& stream, MatrixXd& mat)
{
    LspParser parser(stream);
    QList<QList<double>> rows;

    // Skip tokens until we find the opening parenthesis of the matrix
    LspToken tok;
    bool foundStart = false;
    while (true) {
        tok = parser.next();
        if (tok.type == LspToken::EndOfFile) break;
        if (tok.type == LspToken::LeftParen) {
            foundStart = true;
            break;
        }
    }
    if (!foundStart) return false;

    // Parse rows: each row is ( num num num ... )
    while (true) {
        tok = parser.next();
        if (tok.type == LspToken::EndOfFile || tok.type == LspToken::RightParen) break;

        if (tok.type == LspToken::LeftParen) {
            QList<double> row;
            while (true) {
                tok = parser.next();
                if (tok.type == LspToken::RightParen || tok.type == LspToken::EndOfFile) break;
                if (tok.type == LspToken::Number) row.append(tok.numVal);
            }
            rows.append(row);
        }
    }

    if (rows.isEmpty()) return false;

    int n = rows.size();
    mat.resize(n, n);
    for (int i = 0; i < n; ++i) {
        if (rows[i].size() != n) {
            qCritical("Row %d has %lld values, expected %d (matrix must be square).",
                      i, static_cast<long long>(rows[i].size()), n);
            return false;
        }
        for (int j = 0; j < n; ++j) {
            mat(i, j) = rows[i][j];
        }
    }

    return true;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_convert_lspcov");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Convert a LISP-format covariance matrix to FIFF format.\n\n"
        "The LISP format stores symmetric matrices as nested S-expressions\n"
        "produced by external graph tools. The tool reads the matrix, matches\n"
        "it against the channel information from a FIFF measurement file,\n"
        "and writes a standard FIFF covariance file.\n\n"
        "Port of the MNE-C mne_convert_lspcov utility.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption lspcovOpt("lspcov", "Input LISP-format covariance file.", "file");
    QCommandLineOption measOpt("meas", "Measurement FIFF file (for channel names).", "file");
    QCommandLineOption outOpt("out", "Output FIFF covariance file.", "file");
    QCommandLineOption outascOpt("outasc", "Output text (ASCII) covariance file.", "file");

    parser.addOption(lspcovOpt);
    parser.addOption(measOpt);
    parser.addOption(outOpt);
    parser.addOption(outascOpt);

    parser.process(app);

    QString lspcovFile = parser.value(lspcovOpt);
    QString measFile = parser.value(measOpt);
    QString outFile = parser.value(outOpt);
    QString outascFile = parser.value(outascOpt);

    if (lspcovFile.isEmpty()) { qCritical("--lspcov is required."); return 1; }
    if (measFile.isEmpty()) { qCritical("--meas is required."); return 1; }
    if (outFile.isEmpty() && outascFile.isEmpty()) {
        qCritical("At least one of --out or --outasc is required.");
        return 1;
    }

    //=========================================================================
    // Read measurement info for channel names
    //=========================================================================
    QFile fMeas(measFile);
    FiffRawData raw(fMeas);
    if (raw.info.isEmpty()) {
        qCritical("Cannot read measurement info from: %s", qPrintable(measFile));
        return 1;
    }
    const FiffInfo& info = raw.info;

    int nChan = info.nchan;
    printf("Measurement file: %d channels\n", nChan);

    //=========================================================================
    // Parse LISP covariance matrix
    //=========================================================================
    QFile lspFile(lspcovFile);
    if (!lspFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("Cannot open LISP covariance file: %s", qPrintable(lspcovFile));
        return 1;
    }
    QTextStream lspStream(&lspFile);

    MatrixXd covMat;
    if (!parseLspMatrix(lspStream, covMat)) {
        qCritical("Failed to parse LISP covariance matrix from: %s", qPrintable(lspcovFile));
        return 1;
    }
    lspFile.close();

    printf("Parsed %dx%d matrix from LISP file\n",
           static_cast<int>(covMat.rows()), static_cast<int>(covMat.cols()));

    if (covMat.rows() != nChan) {
        qCritical("Matrix dimension (%d) does not match channel count (%d).",
                  static_cast<int>(covMat.rows()), nChan);
        return 1;
    }

    // Symmetrize
    covMat = (covMat + covMat.transpose()) / 2.0;

    //=========================================================================
    // Write ASCII output
    //=========================================================================
    if (!outascFile.isEmpty()) {
        QFile ascFile(outascFile);
        if (!ascFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qCritical("Cannot open output file: %s", qPrintable(outascFile));
            return 1;
        }
        QTextStream out(&ascFile);
        out << "# Covariance matrix converted from LISP format\n";
        out << "# Channels: " << nChan << "\n";
        for (int i = 0; i < nChan; ++i) {
            for (int j = 0; j < nChan; ++j) {
                out << QString::number(covMat(i, j), 'e', 8);
                if (j < nChan - 1) out << "\t";
            }
            out << "\n";
        }
        ascFile.close();
        printf("Written ASCII covariance to: %s\n", qPrintable(outascFile));
    }

    //=========================================================================
    // Write FIFF covariance
    //=========================================================================
    if (!outFile.isEmpty()) {
        // Build channel name list
        QStringList chNames;
        for (int i = 0; i < nChan; ++i) {
            chNames << info.chs[i].ch_name;
        }

        // Store as lower triangle (packed)
        int nTri = nChan * (nChan + 1) / 2;
        VectorXd packedCov(nTri);
        int idx = 0;
        for (int i = 0; i < nChan; ++i) {
            for (int j = 0; j <= i; ++j) {
                packedCov(idx++) = covMat(i, j);
            }
        }

        FiffCov cov;
        cov.kind = FIFFV_MNE_NOISE_COV;
        cov.diag = false;
        cov.dim = nChan;
        cov.names = chNames;
        cov.data = covMat;
        cov.nfree = 1;

        if (!cov.save(outFile)) {
            qCritical("Cannot write output file: %s", qPrintable(outFile));
            return 1;
        }

        printf("Written FIFF covariance to: %s\n", qPrintable(outFile));
    }

    return 0;
}
