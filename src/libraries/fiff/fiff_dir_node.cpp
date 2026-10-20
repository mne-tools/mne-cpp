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
#include "fiff_stream.h"
#include "fiff_tag.h"
#include "fiff_explain.h"
#include <QDebug>
//#include "fiff_ctf_comp.h"
//#include "fiff_proj.h"
//#include "fiff_info.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffDirNode::FiffDirNode()
: type(-1)
, nent_tree(-1)
, parent(nullptr)
{
}

//=============================================================================================================

FiffDirNode::FiffDirNode(const FiffDirNode* p_FiffDirTree)
: type(p_FiffDirTree->type)
, id(p_FiffDirTree->id)
, dir(p_FiffDirTree->dir)
, nent_tree(p_FiffDirTree->nent_tree)
, parent(p_FiffDirTree->parent)
, parent_id(p_FiffDirTree->parent_id)
, children(p_FiffDirTree->children)
{
}

//=============================================================================================================

FiffDirNode::~FiffDirNode()
{
    //    QList<FiffDirNode*>::iterator i;
    //    for (i = this->children.begin(); i != this->children.end(); ++i)
    //        if (*i)
    //            delete *i;
}

//=============================================================================================================

bool FiffDirNode::copy_tree(FiffStream::SPtr& p_pStreamIn, const FiffId& in_id, const QList<FiffDirNode::SPtr>& p_Nodes, FiffStream::SPtr& p_pStreamOut)
{
    if (p_Nodes.size() <= 0)
        return false;

    qint32 k, p;

    for (k = 0; k < p_Nodes.size(); ++k) {
        p_pStreamOut->start_block(p_Nodes[k]->type); //8
        if (p_Nodes[k]->id.version != -1) {
            if (in_id.version != -1)
                p_pStreamOut->write_id(FIFF_PARENT_FILE_ID, in_id); //9

            p_pStreamOut->write_id(FIFF_BLOCK_ID);                        //10
            p_pStreamOut->write_id(FIFF_PARENT_BLOCK_ID, p_Nodes[k]->id); //11
        }
        for (p = 0; p < p_Nodes[k]->nent(); ++p) {
            //
            //   Do not copy these tags
            //
            if (p_Nodes[k]->dir[p]->kind == FIFF_BLOCK_ID || p_Nodes[k]->dir[p]->kind == FIFF_PARENT_BLOCK_ID || p_Nodes[k]->dir[p]->kind == FIFF_PARENT_FILE_ID)
                continue;

            //
            //   Read and write tags, pass data through transparently
            //
            if (!p_pStreamIn->device()->seek(p_Nodes[k]->dir[p]->pos)) //fseek(fidin, nodes(k).dir(p).pos, 'bof') == -1
            {
                qWarning("Could not seek to the tag\n");
                return false;
            }

            //ToDo this is the same like read_tag
            auto tag = std::make_unique<FiffTag>();
            //QDataStream in(fidin);
            FiffStream::SPtr in = p_pStreamIn;
            in->setByteOrder(QDataStream::BigEndian);

            //
            // Read fiff tag header from stream
            //
            *in >> tag->kind;
            *in >> tag->type;
            qint32 size;
            *in >> size;
            tag->resize(size);
            *in >> tag->next;

            //
            // Read data when available
            //
            if (tag->size() > 0) {
                in->readRawData(tag->data(), tag->size());
                // Don't do this conversion because it breaks the writing on
                // little endian systems (i.e., OSX, Linux, Windows...)
                // FiffTag::convert_tag_data(tag.get(),FIFFV_BIG_ENDIAN,FIFFV_NATIVE_ENDIAN);
            }

            //QDataStream out(p_pStreamOut);
            FiffStream::SPtr out = p_pStreamOut;
            out->setByteOrder(QDataStream::BigEndian);

            *out << static_cast<qint32>(tag->kind);
            *out << static_cast<qint32>(tag->type);
            *out << static_cast<qint32>(tag->size());
            *out << static_cast<qint32>(FIFFV_NEXT_SEQ);

            out->writeRawData(tag->data(), tag->size());
        }
        for (p = 0; p < p_Nodes[k]->nchild(); ++p) {
            QList<FiffDirNode::SPtr> childList;
            childList << p_Nodes[k]->children[p];
            FiffDirNode::copy_tree(p_pStreamIn, in_id, childList, p_pStreamOut);
        }
        p_pStreamOut->end_block(p_Nodes[k]->type);
    }
    return true;
}

//=============================================================================================================

QList<FiffDirNode::SPtr> FiffDirNode::dir_tree_find(fiff_int_t p_kind) const
{
    QList<FiffDirNode::SPtr> nodes;
    if (this->type == p_kind)
        nodes.append(FiffDirNode::SPtr(new FiffDirNode(this)));

    QList<FiffDirNode::SPtr>::const_iterator i;
    for (i = this->children.begin(); i != this->children.end(); ++i)
        nodes.append((*i)->dir_tree_find(p_kind));

    return nodes;
}

//=============================================================================================================

bool FiffDirNode::find_tag(FiffStream* p_pStream, fiff_int_t findkind, FiffTag::UPtr& p_pTag) const
{
    for (qint32 p = 0; p < this->nent(); ++p) {
        if (this->dir[p]->kind == findkind) {
            p_pStream->read_tag(p_pTag, this->dir[p]->pos);
            return true;
        }
    }
    if (p_pTag)
        p_pTag.reset();

    return false;
}

//=============================================================================================================

bool FiffDirNode::has_tag(fiff_int_t findkind)
{
    for (qint32 p = 0; p < this->nent(); ++p)
        if (this->dir.at(p)->kind == findkind)
            return true;
    return false;
}

//=============================================================================================================

bool FiffDirNode::has_kind(fiff_int_t p_kind) const
{
    if (this->type == p_kind)
        return true;

    QList<FiffDirNode::SPtr>::const_iterator i;
    for (i = this->children.begin(); i != this->children.end(); ++i)
        if ((*i)->has_kind(p_kind))
            return true;

    return false;
}

//=============================================================================================================

// MNE-C fiff_explain / fiff_explain_block: names padded to a fixed column
static QString explained(int kind, const QString& name, int width)
{
    return name.isEmpty() ? QStringLiteral("Cannot explain: %1").arg(kind)
                          : QStringLiteral("%1 = %2").arg(kind).arg(name, -width);
}

//=============================================================================================================

void FiffDirNode::print(QTextStream& out, int indent) const
{
    const QString pad(indent, QLatin1Char(' '));
    out << pad << explained(type, FiffExplain::block(type), 14) << " { ";
    if (!id.isEmpty()) {
        out << QString::asprintf("\t%d.%d 0x%x%x %d %d ", id.version >> 16, id.version & 0xFFFF, id.machid[0], id.machid[1], id.time.secs, id.time.usecs);
    }
    out << '\n';

    for (int j = 0; j < nent();) {
        const int kind = dir[j]->kind;
        int count = 1;
        while (j + count < nent() && dir[j + count]->kind == kind) {
            ++count;
        }
        out << pad << "  " << explained(kind, FiffExplain::tag(kind), 15);
        if (count > 1) {
            out << " [" << count << ']';
        }
        out << '\n';
        j += count;
    }
    for (const FiffDirNode::SPtr& child : children) {
        child->print(out, indent + 5);
    }
    out << pad << "}\n";
}

//=============================================================================================================

fiff_int_t FiffDirNode::nent() const
{
    return dir.size();
}

//=============================================================================================================

fiff_int_t FiffDirNode::nchild() const
{
    return children.size();
}
