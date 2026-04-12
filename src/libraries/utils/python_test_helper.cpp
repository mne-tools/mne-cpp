//=============================================================================================================
/**
 * @file     python_test_helper.cpp
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
 *
 * @brief    PythonTestHelper class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "python_test_helper.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QTextStream>
#include <QRegularExpression>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PythonTestHelper::PythonTestHelper()
{
}

//=============================================================================================================

bool PythonTestHelper::isAvailable() const
{
    return isPythonAvailable() && hasPackage("mne");
}

//=============================================================================================================

bool PythonTestHelper::isPythonAvailable() const
{
    return m_runner.isPythonAvailable();
}

//=============================================================================================================

bool PythonTestHelper::hasPackage(const QString& packageName) const
{
    return m_runner.isPackageAvailable(packageName);
}

//=============================================================================================================

PythonRunnerResult PythonTestHelper::eval(const QString& code, int timeoutMs) const
{
    Q_UNUSED(timeoutMs);
    return m_runner.runCode(code);
}

//=============================================================================================================

double PythonTestHelper::evalDouble(const QString& code, bool* ok, int timeoutMs) const
{
    PythonRunnerResult result = eval(code, timeoutMs);
    if (!result.success) {
        if (ok) *ok = false;
        return 0.0;
    }

    bool parseOk = false;
    double value = result.stdOut.trimmed().toDouble(&parseOk);
    if (ok) *ok = parseOk;
    return parseOk ? value : 0.0;
}

//=============================================================================================================

VectorXd PythonTestHelper::evalVector(const QString& code, bool* ok, int timeoutMs) const
{
    PythonRunnerResult result = eval(code, timeoutMs);
    if (!result.success) {
        if (ok) *ok = false;
        return VectorXd();
    }

    // Parse stdout: one value per line, or space-separated on one line
    QString output = result.stdOut.trimmed();
    if (output.isEmpty()) {
        if (ok) *ok = false;
        return VectorXd();
    }

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    // If single line, split by whitespace
    QList<double> values;
    for (const QString& line : lines) {
        QStringList tokens = line.trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        for (const QString& token : tokens) {
            bool parseOk = false;
            double val = token.toDouble(&parseOk);
            if (!parseOk) {
                if (ok) *ok = false;
                return VectorXd();
            }
            values.append(val);
        }
    }

    VectorXd vec(values.size());
    for (int i = 0; i < values.size(); ++i) {
        vec(i) = values[i];
    }

    if (ok) *ok = true;
    return vec;
}

//=============================================================================================================

MatrixXd PythonTestHelper::evalMatrix(const QString& code, bool* ok, int timeoutMs) const
{
    PythonRunnerResult result = eval(code, timeoutMs);
    if (!result.success) {
        if (ok) *ok = false;
        return MatrixXd();
    }

    QString output = result.stdOut.trimmed();
    if (output.isEmpty()) {
        if (ok) *ok = false;
        return MatrixXd();
    }

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    int nRows = lines.size();
    int nCols = -1;

    QList<QList<double>> rowData;
    for (const QString& line : lines) {
        QStringList tokens = line.trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (nCols < 0) {
            nCols = tokens.size();
        } else if (tokens.size() != nCols) {
            if (ok) *ok = false;
            return MatrixXd();
        }

        QList<double> row;
        for (const QString& token : tokens) {
            bool parseOk = false;
            double val = token.toDouble(&parseOk);
            if (!parseOk) {
                if (ok) *ok = false;
                return MatrixXd();
            }
            row.append(val);
        }
        rowData.append(row);
    }

    if (nCols <= 0) {
        if (ok) *ok = false;
        return MatrixXd();
    }

    MatrixXd mat(nRows, nCols);
    for (int r = 0; r < nRows; ++r) {
        for (int c = 0; c < nCols; ++c) {
            mat(r, c) = rowData[r][c];
        }
    }

    if (ok) *ok = true;
    return mat;
}

//=============================================================================================================

PythonRunnerResult PythonTestHelper::runScript(const QString& scriptPath,
                                                const QStringList& args,
                                                int timeoutMs) const
{
    Q_UNUSED(timeoutMs);
    return m_runner.run(scriptPath, args);
}

//=============================================================================================================

QString PythonTestHelper::testDataPath()
{
    return QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/";
}

//=============================================================================================================

bool PythonTestHelper::isPythonRequired()
{
    const QString val = QProcessEnvironment::systemEnvironment().value("MNE_REQUIRE_PYTHON").toLower();
    return val == "true" || val == "1";
}

//=============================================================================================================

bool PythonTestHelper::writeMatrix(const QString& filePath, const MatrixXd& mat)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberNotation(QTextStream::ScientificNotation);
    out.setRealNumberPrecision(17);

    for (int r = 0; r < mat.rows(); ++r) {
        for (int c = 0; c < mat.cols(); ++c) {
            if (c > 0) out << ' ';
            out << mat(r, c);
        }
        out << '\n';
    }

    file.close();
    return true;
}

//=============================================================================================================

MatrixXd PythonTestHelper::readMatrix(const QString& filePath, bool* ok)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (ok) *ok = false;
        return MatrixXd();
    }

    QTextStream in(&file);
    QList<QList<double>> rowData;
    int nCols = -1;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (nCols < 0) {
            nCols = tokens.size();
        } else if (tokens.size() != nCols) {
            if (ok) *ok = false;
            return MatrixXd();
        }

        QList<double> row;
        for (const QString& token : tokens) {
            bool parseOk = false;
            double val = token.toDouble(&parseOk);
            if (!parseOk) {
                if (ok) *ok = false;
                return MatrixXd();
            }
            row.append(val);
        }
        rowData.append(row);
    }

    file.close();

    if (rowData.isEmpty() || nCols <= 0) {
        if (ok) *ok = false;
        return MatrixXd();
    }

    MatrixXd mat(rowData.size(), nCols);
    for (int r = 0; r < rowData.size(); ++r) {
        for (int c = 0; c < nCols; ++c) {
            mat(r, c) = rowData[r][c];
        }
    }

    if (ok) *ok = true;
    return mat;
}

//=============================================================================================================

MatrixXd PythonTestHelper::evalMatrixViaFile(const QString& code,
                                              const QString& outputFilePath,
                                              bool* ok,
                                              int timeoutMs) const
{
    PythonRunnerResult result = eval(code, timeoutMs);
    if (!result.success) {
        if (ok) *ok = false;
        return MatrixXd();
    }

    return readMatrix(outputFilePath, ok);
}
