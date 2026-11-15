//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     fiff_dir_node.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Eric Larson <larson.eric.d@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     January 2017
 * @brief    Implementation of @ref FiffDirNode: builds the FIFFB_* block tree from the parsed directory and walks it for typed lookups.
 *
 * @ref FiffDirNode::make_dir_tree assembles the tree once after the
 * directory tag is parsed; @ref dir_tree_find drives every higher-level
 * reader (@ref FiffInfo, @ref FiffRawData, @ref FiffEvoked,
 * @ref FiffCov, ...) to its block of interest.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_dir_node.h"
#include <memory>
#include <base/log.h>
#include "fiff_stream.h"
#include "fiff_tag.h"
#include "fiff_explain.h"
#include <cstdint>

#include <QString>
//#include "fiff_ctf_comp.h"
//#include "fiff_proj.h"
//#include "fiff_info.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffDirNode::FiffDirNode()
: type(-1)
{
}

//=============================================================================================================

//=============================================================================================================

FiffDirNode::~FiffDirNode()
{
}

//=============================================================================================================

bool FiffDirNode::copy_tree(FiffStream& source, const FiffId& in_id, std::span<const std::shared_ptr<FiffDirNode>> p_Nodes, FiffStream& target)
{
    if (p_Nodes.size() <= 0)
        return false;

    std::int32_t k, p;

    for (k = 0; k < std::ssize(p_Nodes); ++k) {
        target.start_block(p_Nodes[k]->type); //8
        if (p_Nodes[k]->id.version != -1) {
            if (in_id.version != -1)
                target.write_id(FIFF_PARENT_FILE_ID, in_id); //9

            target.write_id(FIFF_BLOCK_ID);                        //10
            target.write_id(FIFF_PARENT_BLOCK_ID, p_Nodes[k]->id); //11
        }
        for (p = 0; p < p_Nodes[k]->nent(); ++p) {
            //
            //   Do not copy these tags
            //
            if (p_Nodes[k]->dir[p].kind == FIFF_BLOCK_ID || p_Nodes[k]->dir[p].kind == FIFF_PARENT_BLOCK_ID || p_Nodes[k]->dir[p].kind == FIFF_PARENT_FILE_ID)
                continue;

            // Copy the tag verbatim; read_tag and write_tag reproduce its bytes
            source.set_little_endian(false);
            std::optional<FiffTag> tag;
            if (tag = source.read_tag(p_Nodes[k]->dir[p].pos); !tag)
                return false;
            tag->next = FIFFV_NEXT_SEQ;
            target.set_little_endian(false);
            target.write_tag(*tag);
        }
        for (p = 0; p < p_Nodes[k]->nchild(); ++p) {
            FiffDirNode::copy_tree(source, in_id, std::span(&p_Nodes[k]->children[p], 1), target);
        }
        target.end_block(p_Nodes[k]->type);
    }
    return true;
}

//=============================================================================================================

std::vector<std::shared_ptr<FiffDirNode>> FiffDirNode::dir_tree_find(fiff_int_t p_kind)
{
    std::vector<std::shared_ptr<FiffDirNode>> found;
    collect(p_kind, found);
    return found;
}

//=============================================================================================================

void FiffDirNode::collect(fiff_int_t kind, std::vector<std::shared_ptr<FiffDirNode>>& found)
{
    if (type == kind) {
        found.push_back(shared_from_this());
    }
    for (const std::shared_ptr<FiffDirNode>& child : children) {
        child->collect(kind, found);
    }
}

//=============================================================================================================

std::optional<FiffTag> FiffDirNode::find_tag(FiffStream& stream, fiff_int_t findkind) const
{
    const auto entry = std::ranges::find(dir, findkind, &FiffDirEntry::kind);
    return entry != dir.end() ? stream.read_tag(entry->pos) : std::nullopt;
}

//=============================================================================================================

bool FiffDirNode::has_tag(fiff_int_t findkind)
{
    for (std::int32_t p = 0; p < this->nent(); ++p)
        if (this->dir.at(p).kind == findkind)
            return true;
    return false;
}

//=============================================================================================================

bool FiffDirNode::has_kind(fiff_int_t p_kind) const
{
    if (this->type == p_kind)
        return true;

    std::vector<std::shared_ptr<FiffDirNode>>::const_iterator i;
    for (i = this->children.begin(); i != this->children.end(); ++i)
        if ((*i)->has_kind(p_kind))
            return true;

    return false;
}

//=============================================================================================================

// MNE-C fiff_explain / fiff_explain_block: names padded to a fixed column
static QString explained(int kind, const std::string& name, int width)
{
    return name.empty() ? QStringLiteral("Cannot explain: %1").arg(kind)
                        : QStringLiteral("%1 = %2").arg(kind).arg(QString::fromStdString(name), -width);
}

//=============================================================================================================

void FiffDirNode::print(QTextStream& out, int indent) const
{
    const QString pad(indent, QLatin1Char(' '));
    out << pad << explained(type, FiffExplain::block(type), 14) << " { ";
    if (!id.empty()) {
        out << QString::asprintf("\t%d.%d 0x%x%x %d %d ", id.version >> 16, id.version & 0xFFFF, id.machid[0], id.machid[1], id.time.secs, id.time.usecs);
    }
    out << '\n';

    for (int j = 0; j < nent();) {
        const int kind = dir[j].kind;
        int count = 1;
        while (j + count < nent() && dir[j + count].kind == kind) {
            ++count;
        }
        out << pad << "  " << explained(kind, FiffExplain::tag(kind), 15);
        if (count > 1) {
            out << " [" << count << ']';
        }
        out << '\n';
        j += count;
    }
    for (const std::shared_ptr<FiffDirNode>& child : children) {
        child->print(out, indent + 5);
    }
    out << pad << "}\n";
}

//=============================================================================================================

fiff_int_t FiffDirNode::nent() const
{
    return static_cast<fiff_int_t>(dir.size());
}

//=============================================================================================================

fiff_int_t FiffDirNode::nchild() const
{
    return static_cast<fiff_int_t>(children.size());
}
