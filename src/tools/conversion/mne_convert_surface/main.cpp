//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     2001
 * @brief    Implements mne_convert_surface: convert between surface file formats.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/mne/mne.h>
#include <mne/fs/fs_surface.h>
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
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
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
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

enum SurfaceFormat
{
    FORMAT_UNKNOWN = 0,
    FORMAT_FIF,
    FORMAT_TRI,
    FORMAT_FREESURFER,
    FORMAT_SMF
};

static SurfaceFormat detectFormat(const std::string& filename);
static bool readTriFile(const std::string& filename, MatrixX3f& rr, MatrixX3i& tris);
static bool writeTriFile(const std::string& filename, const MatrixX3f& rr, const MatrixX3i& tris);
static bool readSmfFile(const std::string& filename, MatrixX3f& rr, MatrixX3i& tris);
static bool writeSmfFile(const std::string& filename, const MatrixX3f& rr, const MatrixX3i& tris);
static bool readFifSurface(const std::string& filename, MatrixX3f& rr, MatrixX3i& tris);
static bool writeFifSurface(const std::string& filename, const MatrixX3f& rr, const MatrixX3i& tris, int surfId);

//=============================================================================================================

static SurfaceFormat detectFormat(const std::string& filename)
{
    if (filename.ends_with(".fif"))
        return FORMAT_FIF;
    if (filename.ends_with(".tri"))
        return FORMAT_TRI;
    if (filename.ends_with(".smf"))
        return FORMAT_SMF;
    // FreeSurfer surfaces typically have no extension or are named like lh.white, rh.pial
    const std::string base = mne::utf8_from_path(mne::path_from_utf8(filename).filename());
    if (base.starts_with("lh.") || base.starts_with("rh."))
        return FORMAT_FREESURFER;
    return FORMAT_UNKNOWN;
}

//=============================================================================================================

/**
 * Reads whitespace-separated numbers from a text file; a missing or malformed number reads as 0.
 */
class SurfNumberReader
{
public:
    explicit SurfNumberReader(std::istream& in)
    : m_text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>())
    {
    }

    SurfNumberReader& operator>>(int& value)
    {
        value = mne::to_int(nextToken()).value_or(0);
        return *this;
    }

    SurfNumberReader& operator>>(float& value)
    {
        value = static_cast<float>(mne::to_double(nextToken()).value_or(0.0));
        return *this;
    }

private:
    static bool isSpace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
    }

    std::string_view nextToken()
    {
        while (m_pos < m_text.size() && isSpace(m_text[m_pos]))
            ++m_pos;
        const std::size_t start = m_pos;
        while (m_pos < m_text.size() && !isSpace(m_text[m_pos]))
            ++m_pos;
        return std::string_view(m_text).substr(start, m_pos - start);
    }

    std::string m_text;
    std::size_t m_pos = 0;
};

//=============================================================================================================

/**
 * @param[in] value       Any value.
 * @param[in] format      'f', 'e' or 'g' as in QString::number.
 * @param[in] precision   Decimals for 'f' and 'e', significant digits for 'g'.
 *
 * @return @p value formatted like QString::number(value, format, precision), which rounds ties away from zero
 *         where std::format rounds them to even.
 */
static std::string surfQtNumber(double value, char format, int precision)
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

/** Six significant digits, the format the tool has always written. */
static std::string surfFormatReal(float value)
{
    return surfQtNumber(static_cast<double>(value), 'g', 6);
}

//=============================================================================================================

static bool readTriFile(const std::string& filename, MatrixX3f& rr, MatrixX3i& tris)
{
    std::ifstream file(mne::path_from_utf8(filename));
    if (!file) {
        Log::critical("Cannot open tri file: {}", filename);
        return false;
    }
    SurfNumberReader in(file);
    int nvert = 0;
    in >> nvert;
    if (nvert <= 0) {
        Log::critical("Invalid vertex count in tri file");
        return false;
    }
    rr.resize(nvert, 3);
    for (int i = 0; i < nvert; i++) {
        int idx;
        float x, y, z;
        in >> idx >> x >> y >> z;
        rr(i, 0) = x;
        rr(i, 1) = y;
        rr(i, 2) = z;
    }
    int ntri = 0;
    in >> ntri;
    if (ntri <= 0) {
        Log::critical("Invalid triangle count in tri file");
        return false;
    }
    tris.resize(ntri, 3);
    for (int i = 0; i < ntri; i++) {
        int idx, v0, v1, v2;
        in >> idx >> v0 >> v1 >> v2;
        tris(i, 0) = v0 - 1; // tri format is 1-based
        tris(i, 1) = v1 - 1;
        tris(i, 2) = v2 - 1;
    }
    return true;
}

//=============================================================================================================

static bool writeTriFile(const std::string& filename, const MatrixX3f& rr, const MatrixX3i& tris)
{
    std::ofstream out(mne::path_from_utf8(filename));
    if (!out) {
        Log::critical("Cannot write tri file: {}", filename);
        return false;
    }
    int nvert = static_cast<int>(rr.rows());
    int ntri = static_cast<int>(tris.rows());
    std::print(out, "{}\n", nvert);
    for (int i = 0; i < nvert; i++) {
        std::print(out, "{} {} {} {}\n", i + 1, surfFormatReal(rr(i, 0)), surfFormatReal(rr(i, 1)), surfFormatReal(rr(i, 2)));
    }
    std::print(out, "{}\n", ntri);
    for (int i = 0; i < ntri; i++) {
        std::print(out, "{} {} {} {}\n", i + 1, tris(i, 0) + 1, tris(i, 1) + 1, tris(i, 2) + 1);
    }
    return true;
}

//=============================================================================================================

static bool readSmfFile(const std::string& filename, MatrixX3f& rr, MatrixX3i& tris)
{
    std::ifstream in(mne::path_from_utf8(filename));
    if (!in) {
        Log::critical("Cannot open smf file: {}", filename);
        return false;
    }
    std::vector<Vector3f> verts;
    std::vector<Vector3i> faces;
    const auto toFloat = [](std::string_view text) {
        return static_cast<float>(mne::to_double(text).value_or(0.0));
    };
    const auto toInt = [](std::string_view text) {
        return mne::to_int(text).value_or(0);
    };
    std::string rawLine;
    while (std::getline(in, rawLine)) {
        const std::string_view line = mne::trimmed(rawLine);
        if (line.empty() || line.starts_with('#'))
            continue;
        const std::vector<std::string_view> parts = mne::split_whitespace(line);
        if (parts.size() < 4)
            continue;
        if (parts[0] == "v") {
            verts.emplace_back(toFloat(parts[1]), toFloat(parts[2]), toFloat(parts[3]));
        } else if (parts[0] == "f") {
            faces.emplace_back(toInt(parts[1]) - 1, toInt(parts[2]) - 1, toInt(parts[3]) - 1);
        }
    }
    rr.resize(std::ssize(verts), 3);
    for (int i = 0; i < std::ssize(verts); i++)
        rr.row(i) = verts[i].transpose();
    tris.resize(std::ssize(faces), 3);
    for (int i = 0; i < std::ssize(faces); i++)
        tris.row(i) = faces[i].transpose();
    return true;
}

//=============================================================================================================

static bool writeSmfFile(const std::string& filename, const MatrixX3f& rr, const MatrixX3i& tris)
{
    std::ofstream out(mne::path_from_utf8(filename));
    if (!out) {
        Log::critical("Cannot write smf file: {}", filename);
        return false;
    }
    std::print(out, "# SMF surface file\n");
    for (int i = 0; i < rr.rows(); i++) {
        std::print(out, "v {} {} {}\n", surfFormatReal(rr(i, 0)), surfFormatReal(rr(i, 1)), surfFormatReal(rr(i, 2)));
    }
    for (int i = 0; i < tris.rows(); i++) {
        std::print(out, "f {} {} {}\n", tris(i, 0) + 1, tris(i, 1) + 1, tris(i, 2) + 1);
    }
    return true;
}

//=============================================================================================================

static bool readFifSurface(const std::string& filename, MatrixX3f& rr, MatrixX3i& tris)
{
    auto read = MNEBem::read(mne::path_from_utf8(filename));
    if (!read) {
        Log::critical("{}", read.error().what());
        return false;
    }
    const MNEBem& bem = *read;
    // Take the first surface
    rr = bem[0].rr;
    tris = bem[0].itris;
    return true;
}

//=============================================================================================================

static bool writeFifSurface(const std::string& filename, const MatrixX3f& rr, const MatrixX3i& tris, int surfId)
{
    auto stream = FiffStream::start_file(mne::path_from_utf8(filename));
    if (!stream) {
        Log::critical("Cannot create FIF file: {}", filename);
        return false;
    }
    stream->start_block(FIFFB_BEM);
    stream->start_block(FIFFB_BEM_SURF);

    fiff_int_t val;

    // Surface ID
    val = surfId;
    stream->write_int(FIFF_BEM_SURF_ID, &val);

    // Coordinate frame: MRI
    val = FIFFV_COORD_MRI;
    stream->write_int(FIFF_BEM_COORD_FRAME, &val);

    // Number of vertices
    val = static_cast<fiff_int_t>(rr.rows());
    stream->write_int(FIFF_BEM_SURF_NNODE, &val);

    // Number of triangles
    val = static_cast<fiff_int_t>(tris.rows());
    stream->write_int(FIFF_BEM_SURF_NTRI, &val);

    // Vertices
    stream->write_float_matrix(FIFF_BEM_SURF_NODES, rr);

    // Triangles (1-based in FIFF)
    MatrixXi tris1 = tris.array() + 1;
    stream->write_int_matrix(FIFF_BEM_SURF_TRIANGLES, tris1);

    // Normals
    MatrixX3f nn = FsSurface::compute_normals(rr, tris);
    stream->write_float_matrix(FIFF_BEM_SURF_NORMALS, nn);

    stream->end_block(FIFFB_BEM_SURF);
    stream->end_block(FIFFB_BEM);
    stream->end_file();
    return true;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    CommandLineParser parser("Convert between surface file formats (FreeSurfer, FIFF, tri, SMF).");
    parser.add_help_option();
    parser.add_version_option();
    parser.add_option({{"surf"}, "Input FreeSurfer surface file.", "file", {}});
    parser.add_option({{"fif"}, "Input FIF surface file.", "file", {}});
    parser.add_option({{"tri"}, "Input ASCII tri format file.", "file", {}});
    parser.add_option({{"smf"}, "Input SMF format file.", "file", {}});
    parser.add_option({{"out"}, "Output file (format detected from extension).", "file", {}});
    parser.add_option({{"outtri"}, "Output as ASCII tri format.", "file", {}});
    parser.add_option({{"outfif"}, "Output as FIF format.", "file", {}});
    parser.add_option({{"outsurf"}, "Output as FreeSurfer surface format.", "file", {}});
    parser.add_option({{"outsmf"}, "Output as SMF format.", "file", {}});
    parser.add_option({{"surfid"}, "Surface ID for FIF output (4=head).", "id", "4"});
    parser.add_option({{"meters"}, "Input coordinates are in meters.", {}, {}});
    parser.add_option({{"millimeters"}, "Input coordinates are in millimeters.", {}, {}});
    parser.process(argc, argv, "mne_convert_surface", PROGRAM_VERSION);

    std::string inputFile;
    SurfaceFormat inputFormat = FORMAT_UNKNOWN;
    std::string outputFile;
    SurfaceFormat outputFormat = FORMAT_UNKNOWN;
    int surfId = mne::to_int(parser.value("surfid")).value_or(0);
    float scaleFactor = 1.0f;
    bool scaleSet = false;

    if (parser.is_set("surf")) {
        inputFile = parser.value("surf");
        inputFormat = FORMAT_FREESURFER;
    } else if (parser.is_set("fif")) {
        inputFile = parser.value("fif");
        inputFormat = FORMAT_FIF;
    } else if (parser.is_set("tri")) {
        inputFile = parser.value("tri");
        inputFormat = FORMAT_TRI;
    } else if (parser.is_set("smf")) {
        inputFile = parser.value("smf");
        inputFormat = FORMAT_SMF;
    }

    if (parser.is_set("out")) {
        outputFile = parser.value("out");
        outputFormat = detectFormat(outputFile);
    } else if (parser.is_set("outtri")) {
        outputFile = parser.value("outtri");
        outputFormat = FORMAT_TRI;
    } else if (parser.is_set("outfif")) {
        outputFile = parser.value("outfif");
        outputFormat = FORMAT_FIF;
    } else if (parser.is_set("outsurf")) {
        outputFile = parser.value("outsurf");
        outputFormat = FORMAT_FREESURFER;
    } else if (parser.is_set("outsmf")) {
        outputFile = parser.value("outsmf");
        outputFormat = FORMAT_SMF;
    }

    if (parser.is_set("meters")) {
        scaleFactor = 1.0f;
        scaleSet = true;
    }
    if (parser.is_set("millimeters")) {
        scaleFactor = 0.001f;
        scaleSet = true;
    }

    if (inputFile.empty()) {
        Log::info("No input file specified.");
        return 1;
    }
    if (outputFile.empty()) {
        Log::info("No output file specified.");
        return 1;
    }
    if (outputFormat == FORMAT_UNKNOWN) {
        Log::info("Cannot determine output format. Use --outtri, --outfif, --outsurf, or --outsmf.");
        return 1;
    }

    // Auto-detect input format if not specified
    if (inputFormat == FORMAT_UNKNOWN)
        inputFormat = detectFormat(inputFile);

    //
    // Read input surface
    //
    MatrixX3f rr;
    MatrixX3i tris;
    bool ok = false;

    switch (inputFormat) {
        case FORMAT_FIF:
            ok = readFifSurface(inputFile, rr, tris);
            break;
        case FORMAT_TRI:
            ok = readTriFile(inputFile, rr, tris);
            if (ok && !scaleSet)
                scaleFactor = 0.001f; // tri files are typically in mm
            break;
        case FORMAT_FREESURFER: {
            FsSurface surface;
            ok = FsSurface::read(mne::path_from_utf8(inputFile), surface);
            if (ok) {
                rr = surface.rr();
                tris = surface.tris();
            }
            break;
        }
        case FORMAT_SMF:
            ok = readSmfFile(inputFile, rr, tris);
            if (ok && !scaleSet)
                scaleFactor = 0.001f; // smf files are typically in mm
            break;
        default:
            Log::info("Cannot determine input format. Use --surf, --fif, --tri, or --smf.");
            return 1;
    }

    if (!ok) {
        Log::critical("Failed to read input surface.");
        return 1;
    }

    Log::info("Read surface: {} vertices, {} triangles", static_cast<long>(rr.rows()), static_cast<long>(tris.rows()));

    // Apply scale factor to convert to meters (internal representation)
    if (scaleFactor != 1.0f) {
        rr *= scaleFactor;
    }

    //
    // Write output surface
    //
    switch (outputFormat) {
        case FORMAT_FIF:
            ok = writeFifSurface(outputFile, rr, tris, surfId);
            break;
        case FORMAT_TRI:
            // tri format uses mm
            ok = writeTriFile(outputFile, rr * 1000.0f, tris);
            break;
        case FORMAT_FREESURFER:
            Log::info("FreeSurfer surface writing not yet supported. Use --outtri or --outfif.");
            return 1;
        case FORMAT_SMF:
            // smf format uses mm
            ok = writeSmfFile(outputFile, rr * 1000.0f, tris);
            break;
        default:
            Log::critical("Unknown output format.");
            return 1;
    }

    if (!ok) {
        Log::critical("Failed to write output surface.");
        return 1;
    }

    Log::info("Wrote surface to {}", outputFile);
    return 0;
}
