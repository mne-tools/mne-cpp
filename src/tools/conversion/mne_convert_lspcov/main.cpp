//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.2.0
 * @date     2001
 * @brief    Convert LISP-format covariance matrix to FIFF format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/fiff/fiff_cov.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <format>
#include <fstream>
#include <istream>
#include <optional>
#include <ostream>
#include <print>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// LISP S-expression parser
//=============================================================================================================

struct LspToken
{
    enum Type
    {
        LeftParen,
        RightParen,
        Number,
        Symbol,
        EndOfFile
    };
    Type type;
    double numVal;
    std::string strVal;
};

//=============================================================================================================

class LspParser
{
public:
    explicit LspParser(std::istream& stream)
    : m_stream(stream)
    , m_pos(0)
    {
    }

    LspToken next()
    {
        skipWhitespace();
        if (m_pos >= m_buffer.size() && !readMore()) {
            return {LspToken::EndOfFile, 0.0, {}};
        }

        char c = m_buffer[m_pos];
        if (c == '(') {
            m_pos++;
            return {LspToken::LeftParen, 0.0, {"("}};
        }
        if (c == ')') {
            m_pos++;
            return {LspToken::RightParen, 0.0, {")"}};
        }

        // Read a token (number or symbol)
        std::string tok;
        while (m_pos < m_buffer.size()) {
            char ch = m_buffer[m_pos];
            if (isSpace(ch) || ch == '(' || ch == ')')
                break;
            tok += ch;
            m_pos++;
        }

        const std::optional<double> val = mne::to_double(tok);
        if (val)
            return {LspToken::Number, *val, tok};
        return {LspToken::Symbol, 0.0, tok};
    }

private:
    static bool isSpace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
    }

    bool atEnd()
    {
        return m_stream.peek() == std::istream::traits_type::eof();
    }

    void skipWhitespace()
    {
        while (true) {
            while (m_pos < m_buffer.size() && isSpace(m_buffer[m_pos]))
                m_pos++;
            if (m_pos < m_buffer.size())
                return;
            if (!readMore())
                return;
        }
    }

    bool readMore()
    {
        if (atEnd())
            return false;
        std::getline(m_stream, m_buffer);
        // Strip comments (lines starting with ; or #)
        if (m_buffer.starts_with(';') || m_buffer.starts_with('#')) {
            m_buffer.clear();
        }
        m_pos = 0;
        return !m_buffer.empty() || !atEnd();
    }

    std::istream& m_stream;
    std::string m_buffer;
    std::size_t m_pos;
};

//=============================================================================================================

static bool parseLspMatrix(std::istream& stream, MatrixXd& mat)
{
    LspParser parser(stream);
    std::vector<std::vector<double>> rows;

    // Skip tokens until we find the opening parenthesis of the matrix
    LspToken tok;
    bool foundStart = false;
    while (true) {
        tok = parser.next();
        if (tok.type == LspToken::EndOfFile)
            break;
        if (tok.type == LspToken::LeftParen) {
            foundStart = true;
            break;
        }
    }
    if (!foundStart)
        return false;

    // Parse rows: each row is ( num num num ... )
    while (true) {
        tok = parser.next();
        if (tok.type == LspToken::EndOfFile || tok.type == LspToken::RightParen)
            break;

        if (tok.type == LspToken::LeftParen) {
            std::vector<double> row;
            while (true) {
                tok = parser.next();
                if (tok.type == LspToken::RightParen || tok.type == LspToken::EndOfFile)
                    break;
                if (tok.type == LspToken::Number)
                    row.push_back(tok.numVal);
            }
            rows.push_back(row);
        }
    }

    if (rows.empty())
        return false;

    const int n = static_cast<int>(rows.size());
    mat.resize(n, n);
    for (int i = 0; i < n; ++i) {
        if (std::ssize(rows[i]) != n) {
            Log::critical("Row {} has {} values, expected {} (matrix must be square).", i, static_cast<long long>(rows[i].size()), n);
            return false;
        }
        for (int j = 0; j < n; ++j) {
            mat(i, j) = rows[i][j];
        }
    }

    return true;
}

//=============================================================================================================

/**
 * @param[in] value       Any value.
 * @param[in] format      'f', 'e' or 'g' as in QString::number.
 * @param[in] precision   Decimals for 'f' and 'e', significant digits for 'g'.
 *
 * @return @p value formatted like QString::number(value, format, precision), which rounds ties away from zero
 *         where std::format rounds them to even.
 */
static std::string lspQtNumber(double value, char format, int precision)
{
    if (std::isnan(value))
        return "nan";
    if (std::isinf(value))
        return value < 0 ? "-inf" : "inf";
    const double magnitude = std::abs(value);
    // Decimal digits d0 d1 d2 ... of magnitude = d0.d1d2... * 10^exponent
    const auto expand = [magnitude](int decimals, std::string& digits, int& exponent) {
        const std::string text = std::format("{:.{}e}", magnitude, decimals);
        const std::size_t ePos = text.find('e');
        digits = text.substr(0, 1) + text.substr(2, ePos - 2);
        exponent = std::stoi(text.substr(ePos + 1));
    };
    std::string digits;
    int exponent = 0;
    expand(17, digits, exponent);
    const int significant = format == 'e' ? precision + 1 : format == 'g' ? std::max(precision, 1)
                                                                          : exponent + 1 + precision;
    expand(std::max(significant, 0) + 40, digits, exponent);
    const std::string_view tail = std::string_view(digits).substr(static_cast<std::size_t>(std::max(significant, 0)) + 1);
    if (tail.find_first_not_of('0') == std::string_view::npos || tail.find_first_not_of('9') == std::string_view::npos)
        expand(std::max(significant, 0) + 1100, digits, exponent); // exact for every double

    std::string rounded;
    if (significant >= 0) {
        rounded = digits.substr(0, static_cast<std::size_t>(significant));
        if (digits[static_cast<std::size_t>(significant)] >= '5') {
            int i = significant - 1;
            while (i >= 0 && rounded[static_cast<std::size_t>(i)] == '9')
                rounded[static_cast<std::size_t>(i--)] = '0';
            if (i >= 0) {
                ++rounded[static_cast<std::size_t>(i)];
            } else {
                rounded.insert(rounded.begin(), '1');
                ++exponent;
                if (format != 'f')
                    rounded.pop_back();
            }
        }
    }
    const bool zero = rounded.find_first_not_of('0') == std::string::npos;
    if (zero)
        exponent = 0;

    const auto fixed = [&](int decimals) {
        std::string intPart;
        std::string fraction;
        if (zero) {
            intPart = "0";
            fraction.assign(static_cast<std::size_t>(decimals), '0');
        } else if (exponent + 1 <= 0) {
            intPart = "0";
            fraction = std::string(static_cast<std::size_t>(-(exponent + 1)), '0') + rounded;
        } else {
            const std::size_t intDigits = static_cast<std::size_t>(exponent) + 1;
            intPart = rounded.substr(0, intDigits);
            fraction = rounded.substr(intDigits);
        }
        fraction.resize(static_cast<std::size_t>(decimals), '0');
        return decimals > 0 ? intPart + "." + fraction : intPart;
    };
    const auto scientific = [&](std::string mantissaDigits) {
        if (mantissaDigits.empty())
            mantissaDigits = "0";
        std::string text(1, mantissaDigits[0]);
        if (mantissaDigits.size() > 1)
            text += "." + mantissaDigits.substr(1);
        return text + std::format("e{}{:02}", exponent < 0 ? '-' : '+', std::abs(exponent));
    };

    std::string text;
    if (format == 'f') {
        text = fixed(precision);
    } else if (format == 'e') {
        text = scientific(zero ? std::string(static_cast<std::size_t>(precision + 1), '0') : rounded);
    } else {
        const int digitsWanted = std::max(precision, 1);
        if (!zero && (exponent < -4 || exponent >= digitsWanted)) {
            std::string mantissa = rounded;
            mantissa.erase(mantissa.find_last_not_of('0') + 1);
            text = scientific(mantissa);
        } else {
            text = fixed(std::max(digitsWanted - 1 - exponent, 0));
            if (text.find('.') != std::string::npos) {
                text.erase(text.find_last_not_of('0') + 1);
                if (text.back() == '.')
                    text.pop_back();
            }
        }
    }
    return value < 0 ? "-" + text : text;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser(
        "Convert a LISP-format covariance matrix to FIFF format.\n\n"
        "The LISP format stores symmetric matrices as nested S-expressions\n"
        "produced by external graph tools. The tool reads the matrix, matches\n"
        "it against the channel information from a FIFF measurement file,\n"
        "and writes a standard FIFF covariance file.\n\n"
        "Port of the MNE-C mne_convert_lspcov utility.");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"lspcov"}, "Input LISP-format covariance file.", "file", {}});
    parser.add_option({{"meas"}, "Measurement FIFF file (for channel names).", "file", {}});
    parser.add_option({{"out"}, "Output FIFF covariance file.", "file", {}});
    parser.add_option({{"outasc"}, "Output text (ASCII) covariance file.", "file", {}});
    parser.process(argc, argv, "mne_convert_lspcov", PROGRAM_VERSION);

    const std::string lspcovFile = parser.value("lspcov");
    const std::string measFile = parser.value("meas");
    const std::string outFile = parser.value("out");
    const std::string outascFile = parser.value("outasc");

    if (lspcovFile.empty()) {
        Log::critical("--lspcov is required.");
        return 1;
    }
    if (measFile.empty()) {
        Log::critical("--meas is required.");
        return 1;
    }
    if (outFile.empty() && outascFile.empty()) {
        Log::critical("At least one of --out or --outasc is required.");
        return 1;
    }

    //=========================================================================
    // Read measurement info for channel names
    //=========================================================================
    FiffRawData raw(mne::path_from_utf8(measFile));
    if (raw.info.empty()) {
        Log::critical("Cannot read measurement info from: {}", measFile);
        return 1;
    }
    const FiffInfo& info = raw.info;

    int nChan = info.nchan;
    Log::info("Measurement file: {} channels", nChan);

    //=========================================================================
    // Parse LISP covariance matrix
    //=========================================================================
    std::ifstream lspStream(mne::path_from_utf8(lspcovFile));
    if (!lspStream) {
        Log::critical("Cannot open LISP covariance file: {}", lspcovFile);
        return 1;
    }

    MatrixXd covMat;
    if (!parseLspMatrix(lspStream, covMat)) {
        Log::critical("Failed to parse LISP covariance matrix from: {}", lspcovFile);
        return 1;
    }
    lspStream.close();

    Log::info("Parsed {}x{} matrix from LISP file", static_cast<int>(covMat.rows()), static_cast<int>(covMat.cols()));

    if (covMat.rows() != nChan) {
        Log::critical("Matrix dimension ({}) does not match channel count ({}).", static_cast<int>(covMat.rows()), nChan);
        return 1;
    }

    // Symmetrize
    covMat = (covMat + covMat.transpose()) / 2.0;

    //=========================================================================
    // Write ASCII output
    //=========================================================================
    if (!outascFile.empty()) {
        std::ofstream out(mne::path_from_utf8(outascFile));
        if (!out) {
            Log::critical("Cannot open output file: {}", outascFile);
            return 1;
        }
        std::print(out, "# Covariance matrix converted from LISP format\n");
        std::print(out, "# Channels: {}\n", nChan);
        for (int i = 0; i < nChan; ++i) {
            for (int j = 0; j < nChan; ++j) {
                std::print(out, "{}", lspQtNumber(covMat(i, j), 'e', 8));
                if (j < nChan - 1)
                    std::print(out, "\t");
            }
            std::print(out, "\n");
        }
        out.close();
        Log::info("Written ASCII covariance to: {}", outascFile);
    }

    //=========================================================================
    // Write FIFF covariance
    //=========================================================================
    if (!outFile.empty()) {
        // Build channel name list
        StringList chNames;
        for (int i = 0; i < nChan; ++i) {
            chNames.push_back(info.chs[i].ch_name);
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

        if (!cov.save(mne::path_from_utf8(outFile))) {
            Log::critical("Cannot write output file: {}", outFile);
            return 1;
        }

        Log::info("Written FIFF covariance to: {}", outFile);
    }

    return 0;
}
