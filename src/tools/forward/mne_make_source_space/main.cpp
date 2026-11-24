//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     February, 2026
 * @brief    Implements mne_make_source_space application.
 *           Port of the original MNE-C mne_make_source_space by Matti Hamalainen.
 *
 *           Creates cortical source spaces by subsampling FreeSurfer surfaces
 *           using icosahedron/octahedron-based or spacing-based decimation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_coord_trans.h>

#include <mne/mne/mne.h>
#include <mne/mne/mne_hemisphere.h>
#include <mne/mne/mne_source_spaces.h>

#include <mne/fs/fs_surface.h>
#include <mne/fs/fs_surfaceset.h>

#include <mne/base/log.h>
#include <mne/base/command_line.h>
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
#include <filesystem>
#include <format>
#include <fstream>
#include <limits>
#include <map>
#include <print>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace mne;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
/**
 * @brief Build an icosahedron and subdivide it to the given grade.
 *
 * Grade 0 = 12 vertices (base icosahedron)
 * Grade n = each triangle subdivided 4^n times
 *
 * For negative grade, use octahedron subdivision instead (oct grade = -ico_grade).
 *
 * @param[in]  grade   Subdivision grade (1-7 for ico, negative for oct).
 * @param[out] verts   Vertex positions on the unit sphere.
 * @return true on success.
 */
static bool makeIcosahedron(int grade, MatrixX3f& verts)
{
    // Golden ratio
    const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;

    // Base icosahedron vertices (12 vertices)
    std::vector<Vector3f> icoVerts = {
        Vector3f(-1, phi, 0), Vector3f(1, phi, 0),
        Vector3f(-1, -phi, 0), Vector3f(1, -phi, 0),
        Vector3f(0, -1, phi), Vector3f(0, 1, phi),
        Vector3f(0, -1, -phi), Vector3f(0, 1, -phi),
        Vector3f(phi, 0, -1), Vector3f(phi, 0, 1),
        Vector3f(-phi, 0, -1), Vector3f(-phi, 0, 1)};

    // Normalize to unit sphere
    for (auto& v : icoVerts)
        v.normalize();

    // Base icosahedron triangles (20 faces)
    std::vector<Vector3i> icoFaces = {
        Vector3i(0, 11, 5), Vector3i(0, 5, 1), Vector3i(0, 1, 7),
        Vector3i(0, 7, 10), Vector3i(0, 10, 11), Vector3i(1, 5, 9),
        Vector3i(5, 11, 4), Vector3i(11, 10, 2), Vector3i(10, 7, 6),
        Vector3i(7, 1, 8), Vector3i(3, 9, 4), Vector3i(3, 4, 2),
        Vector3i(3, 2, 6), Vector3i(3, 6, 8), Vector3i(3, 8, 9),
        Vector3i(4, 9, 5), Vector3i(2, 4, 11), Vector3i(6, 2, 10),
        Vector3i(8, 6, 7), Vector3i(9, 8, 1)};

    // Subdivide
    for (int g = 0; g < std::abs(grade); ++g) {
        std::vector<Vector3f> newVerts = icoVerts;
        std::vector<Vector3i> newFaces;
        std::map<std::pair<int, int>, int> edgeMidpoint;

        auto getMidpoint = [&](int v0, int v1) -> int {
            const auto key = std::make_pair(std::min(v0, v1), std::max(v0, v1));
            if (const auto it = edgeMidpoint.find(key); it != edgeMidpoint.end())
                return it->second;
            Vector3f mid = (newVerts[static_cast<std::size_t>(v0)] + newVerts[static_cast<std::size_t>(v1)]).normalized();
            const auto idx = static_cast<int>(newVerts.size());
            newVerts.push_back(mid);
            edgeMidpoint[key] = idx;
            return idx;
        };

        for (const auto& f : icoFaces) {
            int a = getMidpoint(f(0), f(1));
            int b = getMidpoint(f(1), f(2));
            int c = getMidpoint(f(2), f(0));

            newFaces.emplace_back(f(0), a, c);
            newFaces.emplace_back(a, f(1), b);
            newFaces.emplace_back(c, b, f(2));
            newFaces.emplace_back(a, b, c);
        }

        icoVerts = newVerts;
        icoFaces = newFaces;
    }

    // Copy to output
    verts.resize(std::ssize(icoVerts), 3);
    for (std::size_t i = 0; i < icoVerts.size(); ++i) {
        verts.row(static_cast<Eigen::Index>(i)) = icoVerts[i].transpose();
    }

    Log::info("  Icosahedron grade {}: {} vertices", std::abs(grade), static_cast<int>(verts.rows()));
    return true;
}

//=============================================================================================================
/**
 * @brief Select source vertices on a surface by finding the nearest surface
 *        vertex to each icosahedron point.
 *
 * @param[in]  surfVerts   The high-resolution surface vertices (np x 3).
 * @param[in]  icoVerts    Icosahedron vertices on unit sphere (nico x 3).
 * @param[out] inuse       Binary vector (np): 1 = selected, 0 = not.
 * @param[out] vertno      Indices of the selected vertices.
 * @return Number of selected vertices.
 */
static int selectVerticesIco(const MatrixX3f& surfVerts,
                             const MatrixX3f& icoVerts,
                             VectorXi& inuse,
                             VectorXi& vertno)
{
    const auto np = static_cast<int>(surfVerts.rows());
    const auto nico = static_cast<int>(icoVerts.rows());

    // Normalize surface vertices to the unit sphere
    MatrixX3f surfNorm(np, 3);
    for (int i = 0; i < np; ++i) {
        surfNorm.row(i) = surfVerts.row(i).normalized();
    }

    inuse = VectorXi::Zero(np);

    // For each icosahedron point, find the closest surface vertex
    for (int k = 0; k < nico; ++k) {
        float minDist = std::numeric_limits<float>::max();
        int minIdx = 0;

        for (int i = 0; i < np; ++i) {
            float dist = (surfNorm.row(i) - icoVerts.row(k)).squaredNorm();
            if (dist < minDist) {
                minDist = dist;
                minIdx = i;
            }
        }
        inuse(minIdx) = 1;
    }

    // Collect selected vertex indices
    int nuse = inuse.sum();
    vertno.resize(nuse);
    int idx = 0;
    for (int i = 0; i < np; ++i) {
        if (inuse(i)) {
            vertno(idx++) = i;
        }
    }

    return nuse;
}

//=============================================================================================================
/**
 * @brief Select source vertices by approximate spacing in mm.
 *
 * Selects a subset of surface vertices such that the minimum distance
 * between any two selected vertices is approximately the given spacing.
 *
 * @param[in]  surfVerts   FsSurface vertex positions (np x 3).
 * @param[in]  spacing     Desired spacing in mm.
 * @param[out] inuse       Binary vector (np): 1 = selected, 0 = not.
 * @param[out] vertno      Indices of the selected vertices.
 * @return Number of selected vertices.
 */
static int selectVerticesSpacing(const MatrixX3f& surfVerts,
                                 float spacing,
                                 VectorXi& inuse,
                                 VectorXi& vertno)
{
    const auto np = static_cast<int>(surfVerts.rows());
    float spacingM = spacing / 1000.0f; // mm to m
    float spacingSq = spacingM * spacingM;

    inuse = VectorXi::Zero(np);

    // Greedy selection: iterate through vertices and select if far enough
    // from all previously selected vertices
    std::vector<int> selected;
    selected.reserve(static_cast<std::size_t>(np / 10));

    for (int i = 0; i < np; ++i) {
        bool tooClose = false;
        for (std::size_t j = 0; j < selected.size(); ++j) {
            float dist = (surfVerts.row(i) - surfVerts.row(selected[j])).squaredNorm();
            if (dist < spacingSq) {
                tooClose = true;
                break;
            }
        }
        if (!tooClose) {
            selected.push_back(i);
            inuse(i) = 1;
        }
    }

    const auto nuse = static_cast<int>(selected.size());
    vertno.resize(nuse);
    for (int i = 0; i < nuse; ++i) {
        vertno(i) = selected[static_cast<std::size_t>(i)];
    }

    return nuse;
}

//=============================================================================================================
/**
 * @brief Build a MNEHemisphere from a FreeSurfer surface with vertex selection.
 *
 * @param[in]  surf     The FreeSurfer surface.
 * @param[in]  inuse    Binary vector indicating selected vertices.
 * @param[in]  vertno   Indices of selected vertices.
 * @param[in]  hemiId   Hemisphere ID (FIFFV_MNE_SURF_LEFT_HEMI or FIFFV_MNE_SURF_RIGHT_HEMI).
 * @return The constructed MNEHemisphere.
 */
static MNEHemisphere buildHemisphere(const FsSurface& surf,
                                     const VectorXi& inuse,
                                     const VectorXi& vertno,
                                     int hemiId)
{
    MNEHemisphere hemi;

    hemi.id = hemiId;
    hemi.np = static_cast<int>(surf.rr().rows());
    hemi.ntri = static_cast<int>(surf.tris().rows());
    hemi.coord_frame = FIFFV_COORD_MRI;
    hemi.type = 1; // FsSurface type

    // Copy vertex positions and normals
    hemi.rr = surf.rr();
    hemi.nn = surf.nn();
    hemi.itris = surf.tris();

    // Selection info
    hemi.nuse = static_cast<int>(vertno.size());
    hemi.inuse = inuse;
    hemi.vertno = vertno;

    // Build use_tris (triangles that use only selected vertices)
    std::vector<int> useTrisIdx;
    for (int t = 0; t < hemi.ntri; ++t) {
        if (inuse(hemi.itris(t, 0)) &&
            inuse(hemi.itris(t, 1)) &&
            inuse(hemi.itris(t, 2))) {
            useTrisIdx.push_back(t);
        }
    }
    hemi.nuse_tri = static_cast<int>(useTrisIdx.size());
    hemi.use_itris.resize(hemi.nuse_tri, 3);
    for (int i = 0; i < hemi.nuse_tri; ++i) {
        hemi.use_itris.row(i) = hemi.itris.row(useTrisIdx[static_cast<std::size_t>(i)]);
    }

    // Add geometry information (normals, triangle centers, etc.)
    hemi.add_geometry_info();

    return hemi;
}

//=============================================================================================================
// MAIN
//=============================================================================================================

/**
 * The function main marks the entry point of the mne_make_source_space application.
 * This is a port of the original MNE-C mne_make_source_space by Matti Hamalainen.
 *
 * It reads FreeSurfer surface files for left and right hemispheres, subsamples
 * them using icosahedron-based or spacing-based decimation, and writes the
 * resulting source space to a FIFF file.
 *
 * @param[in] argc  (argument count)
 * @param[in] argv  (argument vector)
 * @return exit code (0 on success).
 */
int main(int argc, char* argv[])
{
    //=========================================================================================================
    // Command line parser
    //=========================================================================================================

    CommandLineParser parser(
        "Create cortical source spaces from FreeSurfer surfaces.\n"
        "Port of the original MNE-C mne_make_source_space by Matti Hamalainen.\n\n"
        "Reads FreeSurfer surface meshes for left and right hemispheres,\n"
        "subsamples them using icosahedron or spacing-based decimation,\n"
        "and writes the resulting source space to a FIFF file.\n\n"
        "Requires --surf and a decimation method (--ico, --oct, --spacing, or --all).\n"
        "Subject defaults to $SUBJECT environment variable if --subject is not given.");
    parser.add_help_option();
    parser.add_version_option();

    // --subject: Subject name
    parser.add_option({{"subject"}, "Name of the FreeSurfer subject.", "name", ""});

    // --subjects_dir: Subjects directory (default: $SUBJECTS_DIR)
    parser.add_option({{"subjects_dir"}, "FreeSurfer subjects directory (default: $SUBJECTS_DIR).", "dir", ""});

    // --surf: FsSurface name(s) (required, matching SVN MNE-C)
    parser.add_option({{"surf"},
                       "FsSurface name to use (e.g., white, pial). "
                       "For multiple surfaces, separate with colons (e.g., white:pial).",
                       "names",
                       ""});

    // --ico: Icosahedron subdivision grade
    parser.add_option({{"ico"},
                       "Icosahedron subdivision grade for downsampling (1-7).\n"
                       "Grade 1 = 42 vertices, 2 = 162, 3 = 642, 4 = 2562, 5 = 10242, 6 = 40962, 7 = 163842.",
                       "grade",
                       ""});

    // --oct: Octahedron subdivision grade
    parser.add_option({{"oct"}, "Octahedron subdivision grade (stored as negative ico grade internally).", "grade", ""});

    // --spacing: Approximate spacing in mm
    parser.add_option({{"spacing"}, "Approximate source space spacing in mm (alternative to --ico/--oct).", "dist", ""});

    // --all: Use all surface vertices
    parser.add_option({{"all"}, "Use all surface vertices (no decimation).", "", ""});

    // --src: Output file name
    parser.add_option({{"src"}, "Output source space file (default: <subject>-<ico/spacing>-src.fif).", "file", ""});

    parser.process(argc, argv, "mne_make_source_space", PROGRAM_VERSION);

    //=========================================================================================================
    // Validate arguments
    //=========================================================================================================

    const auto envValue = [](const char* name) {
        const char* value = std::getenv(name);
        return std::string(value ? value : "");
    };

    if (!parser.is_set("subject")) {
        // Fall back to $SUBJECT environment variable (matching SVN MNE-C)
        const std::string envSubject = envValue("SUBJECT");
        if (envSubject.empty()) {
            Log::critical("Error: --subject option is required (or set $SUBJECT).");
            std::print("{}", parser.help_text("mne_make_source_space"));
            return 1;
        }
        Log::info("Using subject from $SUBJECT: {}", envSubject);
    }

    const std::string subject = parser.is_set("subject") ? parser.value("subject")
                                                         : envValue("SUBJECT");
    std::string subjectsDir;
    if (parser.is_set("subjects_dir")) {
        subjectsDir = parser.value("subjects_dir");
    } else {
        subjectsDir = envValue("SUBJECTS_DIR");
        if (subjectsDir.empty()) {
            Log::critical("Error: SUBJECTS_DIR not set. Use --subjects_dir.");
            return 1;
        }
    }

    // --surf is required (matching SVN MNE-C)
    if (!parser.is_set("surf")) {
        Log::critical("Error: --surf option is required.");
        std::print("{}", parser.help_text("mne_make_source_space"));
        return 1;
    }
    const std::string surfName = parser.value("surf");

    // Determine decimation method (must be explicitly specified)
    enum DecimMethod
    {
        ICO,
        SPACING,
        ALL
    } decimMethod;
    int icoGrade = 0;
    float spacing = 0.0f;

    if (parser.is_set("all")) {
        decimMethod = ALL;
    } else if (parser.is_set("spacing")) {
        decimMethod = SPACING;
        spacing = static_cast<float>(to_double(parser.value("spacing")).value_or(0.0));
        if (spacing <= 0.0f) {
            Log::critical("Error: --spacing must be a positive number.");
            return 1;
        }
    } else if (parser.is_set("ico") || parser.is_set("oct")) {
        decimMethod = ICO;
        if (parser.is_set("ico")) {
            icoGrade = to_int(parser.value("ico")).value_or(0);
        } else {
            icoGrade = to_int(parser.value("oct")).value_or(0);
        }
        if (icoGrade < 1 || icoGrade > 7) {
            Log::critical("Error: --ico/--oct grade should be between 1 and 7.");
            return 1;
        }
    } else {
        Log::critical("Error: A decimation method is required (--ico, --oct, --spacing, or --all).");
        std::print("{}", parser.help_text("mne_make_source_space"));
        return 1;
    }

    // Determine output file name
    std::string srcName;
    if (parser.is_set("src")) {
        srcName = parser.value("src");
    } else {
        std::string tag;
        if (decimMethod == ICO)
            tag = std::format("ico-{}", icoGrade);
        else if (decimMethod == SPACING)
            tag = std::format("spacing-{}", static_cast<int>(spacing));
        else
            tag = "all";

        srcName = subjectsDir + "/" + subject + "/bem/" + subject + "-" + tag + "-src.fif";
    }

    //=========================================================================================================
    // Build icosahedron for decimation (if needed)
    //=========================================================================================================

    MatrixX3f icoVerts;
    if (decimMethod == ICO) {
        Log::info("\nBuilding icosahedron with grade {} for decimation...", icoGrade);
        if (!makeIcosahedron(icoGrade, icoVerts)) {
            Log::critical("Error: Failed to build icosahedron.");
            return 1;
        }
    }

    //=========================================================================================================
    // Process each hemisphere
    //=========================================================================================================

    const StringList hemiNames = {"lh", "rh"};
    int hemiIds[2] = {FIFFV_MNE_SURF_LEFT_HEMI, FIFFV_MNE_SURF_RIGHT_HEMI};

    std::vector<MNEHemisphere> hemispheres;

    for (int h = 0; h < 2; ++h) {
        Log::info("\n========================================");
        Log::info("Processing {} hemisphere...", hemiNames[static_cast<std::size_t>(h)]);

        // Read FreeSurfer surface
        const std::string surfPath = std::format("{}/{}/surf/{}.{}", subjectsDir, subject, hemiNames[static_cast<std::size_t>(h)], surfName);

        Log::info("Reading surface from {}...", surfPath);

        FsSurface surf;
        if (!FsSurface::read(path_from_utf8(surfPath), surf)) {
            Log::critical("Error: Could not read surface {}", surfPath);
            return 1;
        }

        Log::info("  FsSurface: {} vertices, {} triangles", static_cast<int>(surf.rr().rows()), static_cast<int>(surf.tris().rows()));

        // Select source vertices
        VectorXi inuse;
        VectorXi vertno;
        int nuse;

        if (decimMethod == ALL) {
            Log::info("  Using all vertices.");
            const auto np = static_cast<int>(surf.rr().rows());
            inuse = VectorXi::Ones(np);
            vertno.resize(np);
            for (int i = 0; i < np; ++i)
                vertno(i) = i;
            nuse = np;
        } else if (decimMethod == ICO) {
            Log::info("  Decimating with icosahedron grade {}...", icoGrade);
            nuse = selectVerticesIco(surf.rr(), icoVerts, inuse, vertno);
        } else {
            Log::info("  Decimating with spacing {:.1f} mm...", spacing);
            nuse = selectVerticesSpacing(surf.rr(), spacing, inuse, vertno);
        }

        Log::info("  Selected {} source locations.", nuse);

        if (nuse == 0) {
            Log::critical("Error: No vertices selected for {}", hemiNames[static_cast<std::size_t>(h)]);
            return 1;
        }

        // Build hemisphere
        MNEHemisphere hemi = buildHemisphere(surf, inuse, vertno, hemiIds[h]);
        hemispheres.push_back(hemi);
    }

    //=========================================================================================================
    // Write source space
    //=========================================================================================================

    Log::info("\n========================================");
    Log::info("Writing source space to {}...", srcName);

    // Ensure output directory exists
    const std::filesystem::path srcPath = path_from_utf8(srcName);
    std::error_code ec;
    if (srcPath.has_parent_path())
        std::filesystem::create_directories(srcPath.parent_path(), ec);

    if (!std::ofstream(srcPath, std::ios::binary)) {
        Log::critical("Error: Could not open output file {}", srcName);
        return 1;
    }

    auto pStream = FiffStream::start_file(srcPath);
    pStream->start_block(FIFFB_MNE);
    for (MNEHemisphere& hemisphere : hemispheres) {
        hemisphere.writeToStream(*pStream);
    }
    pStream->end_block(FIFFB_MNE);
    pStream->end_file();

    // Summary
    Log::info("\nSource space created successfully:");
    for (std::size_t h = 0; h < hemispheres.size(); ++h) {
        if (hemispheres[h].nuse_tri > 0) {
            Log::info("  {} hemisphere: {} of {} vertices selected, {} use triangles", h == 0 ? "Left" : "Right", hemispheres[h].nuse, hemispheres[h].np, hemispheres[h].nuse_tri);
        } else {
            Log::info("  {} hemisphere: {} of {} vertices selected", h == 0 ? "Left" : "Right", hemispheres[h].nuse, hemispheres[h].np);
        }
    }
    Log::info("\nOutput: {}", srcName);
    Log::info("\nYou can now use mne_forward_solution to compute forward solutions");
    Log::info("using this source space.\n");

    return 0;
}
