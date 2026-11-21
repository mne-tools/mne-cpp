//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fs_annotation.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Felix Arndt <Felix.Arndt@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July 2012
 * @brief    Implementation of @ref mne::FsAnnotation: parses FreeSurfer .annot per-vertex parcellation files and their embedded colortable.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_annotation.h"

#include "fs_label.h"
#include "fs_surface.h"
#include <base/log.h>
#include <base/byte_stream.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>
#include <algorithm>
#include <numeric>
#include <format>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FsAnnotation::FsAnnotation()
: m_iHemi(-1)
{
}

//=============================================================================================================

FsAnnotation::FsAnnotation(const std::filesystem::path& p_sFileName)
: m_iHemi(-1)
{
    FsAnnotation::read(p_sFileName, *this);
}

//=============================================================================================================

FsAnnotation::FsAnnotation(const std::string& subject_id, std::int32_t hemi, const std::string& atlas, const std::filesystem::path& subjects_dir)
: m_iHemi(-1)
{
    FsAnnotation::read(subject_id, hemi, atlas, subjects_dir, *this);
}

//=============================================================================================================

FsAnnotation::FsAnnotation(const std::filesystem::path& path, std::int32_t hemi, const std::string& atlas)
: m_iHemi(-1)
{
    FsAnnotation::read(path, hemi, atlas, *this);
}

//=============================================================================================================

FsAnnotation::~FsAnnotation()
{
}

//=============================================================================================================

void FsAnnotation::clear()
{
    m_sFileName.clear();
    m_sFilePath.clear();
    m_iHemi = -1;
    m_Vertices = VectorXi::Zero(0);
    m_LabelIds = VectorXi::Zero(0);
    m_Colortable.clear();
}

//=============================================================================================================

bool FsAnnotation::read(const std::string& subject_id, std::int32_t hemi, const std::string& atlas, const std::filesystem::path& subjects_dir, FsAnnotation& p_Annotation)
{
    if (hemi != 0 && hemi != 1)
        return false;

    return read(subjects_dir / mne::path_from_utf8(subject_id) / "label", hemi, atlas, p_Annotation);
}

//=============================================================================================================

bool FsAnnotation::read(const std::filesystem::path& path, std::int32_t hemi, const std::string& atlas, FsAnnotation& p_Annotation)
{
    if (hemi != 0 && hemi != 1)
        return false;

    return read(path / mne::path_from_utf8(std::format("{}.{}.annot", hemi == 0 ? "lh" : "rh", atlas)), p_Annotation);
}

//=============================================================================================================

bool FsAnnotation::read(const std::filesystem::path& p_sFileName, FsAnnotation& p_Annotation)
{
    p_Annotation.clear();

    mne::Log::info("Reading annotation...\n");
    auto bytes = mne::read_file_bytes(p_sFileName);
    if (!bytes) {
        mne::Log::warning("\tError: Couldn't open the file");
        return false;
    }
    mne::MemoryByteStream stream(std::move(*bytes));
    const auto readInt = [&stream]() {
        std::int32_t value = 0;
        stream.read_big_endian(value);
        return value;
    };
    // A length-prefixed string; FreeSurfer counts the terminating NUL
    const auto readString = [&stream, &readInt]() {
        const std::int32_t len = readInt();
        std::string text(static_cast<std::size_t>(std::clamp<std::int64_t>(len, 0, static_cast<std::int64_t>(stream.size() - stream.pos()))), '\0');
        stream.read(text.data(), text.size());
        if (!text.empty() && text.back() == '\0')
            text.pop_back();
        return text;
    };
    const auto readColor = [&stream](FsColortable& ct, std::int32_t row) {
        std::int32_t rgba[4] = {};
        stream.read_big_endian(rgba, 4);
        for (std::int32_t j = 0; j < 4; ++j)
            ct.table(row, j) = rgba[j];
        ct.table(row, 4) = rgba[0] + rgba[1] * 256 + rgba[2] * 65536 + rgba[3] * 16777216;
    };

    const std::int32_t numEl = readInt();
    if (numEl < 0 || static_cast<std::uint64_t>(numEl) * 8 > stream.size() - stream.pos()) {
        mne::Log::warning("\tError: Truncated annotation file");
        return false;
    }
    // Vertex number and label id interleaved
    std::vector<std::int32_t> pairs(2 * static_cast<std::size_t>(numEl));
    stream.read_big_endian(pairs.data(), pairs.size());
    p_Annotation.m_Vertices.resize(numEl);
    p_Annotation.m_LabelIds.resize(numEl);
    for (std::int32_t i = 0; i < numEl; ++i) {
        p_Annotation.m_Vertices[i] = pairs[2 * static_cast<std::size_t>(i)];
        p_Annotation.m_LabelIds[i] = pairs[2 * static_cast<std::size_t>(i) + 1];
    }

    FsColortable& ct = p_Annotation.m_Colortable;
    if (readInt()) {
        ct.clear();
        std::int32_t numEntries = readInt();
        if (numEntries > 0) {
            mne::Log::info("\tReading from Original Version\n");
            ct.numEntries = numEntries;
            ct.orig_tab = readString();
            ct.struct_names.assign(static_cast<std::size_t>(numEntries), std::string());
            ct.table = MatrixXi(numEntries, 5);
            for (std::int32_t i = 0; i < numEntries; ++i) {
                ct.struct_names[static_cast<std::size_t>(i)] = readString();
                readColor(ct, i);
            }
        } else {
            const std::int32_t version = -numEntries;
            if (version != 2)
                mne::Log::warning("\tError! Does not handle version {}", version);
            else
                mne::Log::info("\tReading from version {}\n", version);

            numEntries = readInt();
            if (numEntries < 0) {
                mne::Log::warning("\tError: Invalid colortable size {}", numEntries);
                return false;
            }
            ct.numEntries = numEntries;
            ct.orig_tab = readString();
            ct.struct_names.assign(static_cast<std::size_t>(numEntries), std::string());
            ct.table = MatrixXi(numEntries, 5);

            const std::int32_t numEntriesToRead = readInt();
            for (std::int32_t i = 0; i < numEntriesToRead; ++i) {
                const std::int32_t structure = readInt();
                if (structure < 0 || structure >= numEntries) {
                    mne::Log::warning("\tError! Read entry, index {}", structure);
                    return false;
                }
                std::string& name = ct.struct_names[static_cast<std::size_t>(structure)];
                if (!name.empty())
                    mne::Log::warning("Error! Duplicate Structure {}", structure);
                name = readString();
                readColor(ct, structure);
            }
        }
        mne::Log::info("\tcolortable with {} entries read\n\t(originally {})\n", ct.numEntries, ct.orig_tab);
    } else {
        mne::Log::warning("\tError! No colortable stored");
    }

    p_Annotation.m_sFileName = mne::utf8_from_path(p_sFileName.filename());
    p_Annotation.m_sFilePath = mne::utf8_from_path(p_sFileName);
    p_Annotation.m_iHemi = p_Annotation.m_sFileName.starts_with("lh.") ? 0 : 1;

    mne::Log::info("[done]\n");

    return true;
}

//=============================================================================================================

bool FsAnnotation::toLabels(const FsSurface& p_surf,
                            std::vector<FsLabel>& p_qListLabels,
                            std::vector<RowVector4i>& p_qListLabelRGBAs,
                            const mne::StringList& lLabelPicks) const
{
    if (this->m_iHemi != p_surf.hemi()) {
        mne::Log::warning("FsAnnotation and surface hemisphere (annot = {}; surf = {}) do not match!\n", this->m_iHemi, p_surf.hemi());
        return false;
    }

    if (m_LabelIds.size() == 0) {
        mne::Log::warning("FsAnnotation doesn't' contain data!\n");
        return false;
    }

    mne::Log::info("Converting labels from annotation...");

    //n_read = 0
    //labels = list()
    //label_colors = list()

    VectorXi label_ids = m_Colortable.getLabelIds();
    const mne::StringList& label_names = m_Colortable.getNames();
    MatrixX4i label_rgbas = m_Colortable.getRGBAs();

    // load the vertex positions from surface
    MatrixX3f vert_pos = p_surf.rr();


    //    std::cout << label_ids;

    const std::size_t firstNew = p_qListLabels.size();
    std::int32_t label_id, count;
    RowVector4i label_rgba;
    VectorXi vertices;
    VectorXd values;
    MatrixX3f pos;
    std::string name;
    for (std::int32_t i = 0; i < label_rgbas.rows(); ++i) {
        label_id = label_ids[i];
        label_rgba = label_rgbas.row(i);
        count = 0;
        vertices.resize(m_LabelIds.size());
        //Where
        for (std::int32_t j = 0; j < m_LabelIds.size(); ++j) {
            if (m_LabelIds[j] == label_id) {
                vertices[count] = j;
                ++count;
            }
        }
        // check if label is part of cortical surface
        if (count == 0)
            continue;
        vertices.conservativeResize(count);

        pos.resize(count, 3);
        for (std::int32_t j = 0; j < count; ++j)
            pos.row(j) = vert_pos.row(vertices[j]);

        values = VectorXd::Ones(count);
        name = std::format("{}-{}", label_names[static_cast<std::size_t>(i)], this->m_iHemi == 0 ? "lh" : "rh");

        // put it all together
        if (lLabelPicks.empty()) {
            //t_tris
            p_qListLabels.emplace_back(vertices, pos, values, this->m_iHemi, name, label_id);
            // store the color
            p_qListLabelRGBAs.push_back(label_rgba);
        } else if (std::ranges::contains(lLabelPicks, name)) {
            //t_tris
            p_qListLabels.emplace_back(vertices, pos, values, this->m_iHemi, name, label_id);
            // store the color
            p_qListLabelRGBAs.push_back(label_rgba);
        }
    }

    //    for label_id, label_name, label_rgba in
    //            zip(label_ids, label_names, label_rgbas):
    //        vertices = np.where(annot == label_id)[0]
    //        if len(vertices) == 0:
    //            # label is not part of cortical surface
    //            continue
    //        pos = vert_pos[vertices, :]
    //        values = np.zeros(len(vertices))
    //        name = label_name + '-' + hemi
    //        label = FsLabel(vertices, pos, values, hemi, name=name)
    //        labels.append(label)

    //        # store the color
    //        label_rgba = tuple(label_rgba / 255.)
    //        label_colors.append(label_rgba)

    //    n_read = len(labels) - n_read
    //    logger.info('   read %d labels from %s' % (n_read, fname))

    //# sort the labels and colors by label name
    //names = [label.name for label in labels]
    //labels, label_colors = zip(*((label, color) for (name, label, color)
    //                           in sorted(zip(names, labels, label_colors))))
    //# convert tuples to lists
    //labels = list(labels)
    //label_colors = list(label_colors)

    // mne.read_labels_from_annot sorts the labels by name; sort an index, then apply it by moving
    std::vector<std::size_t> order(p_qListLabels.size() - firstNew);
    std::iota(order.begin(), order.end(), firstNew);
    std::ranges::stable_sort(order, {}, [&p_qListLabels](std::size_t k) -> const std::string& { return p_qListLabels[k].name; });
    std::vector<FsLabel> labels;
    std::vector<RowVector4i> rgbas;
    labels.reserve(order.size());
    rgbas.reserve(order.size());
    for (const std::size_t k : order) {
        labels.push_back(std::move(p_qListLabels[k]));
        rgbas.push_back(p_qListLabelRGBAs[k]);
    }
    std::ranges::move(labels, p_qListLabels.begin() + static_cast<std::ptrdiff_t>(firstNew));
    std::ranges::copy(rgbas, p_qListLabelRGBAs.begin() + static_cast<std::ptrdiff_t>(firstNew));

    mne::Log::info("[done]\n");

    return true;
}
