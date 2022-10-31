//=============================================================================================================
/**
 * @file     fiff_dir_node.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the FiffDirNode Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_dir_node.h"
#include "fiff_stream.h"
#include "fiff_tag.h"
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
, parent(NULL)
{
}

//=============================================================================================================

FiffDirNode::FiffDirNode(const FiffDirNode* p_FiffDirTree)
: type(p_FiffDirTree->type)
, id(p_FiffDirTree->id)
, parent_id(p_FiffDirTree->parent_id)
, dir(p_FiffDirTree->dir)
, nent_tree(p_FiffDirTree->nent_tree)
, parent(p_FiffDirTree->parent)
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
    if(p_Nodes.size() <= 0)
        return false;

    qint32 k, p;

    for(k = 0; k < p_Nodes.size(); ++k)
    {
        p_pStreamOut->start_block(p_Nodes[k]->type);//8
        if (p_Nodes[k]->id.version != -1)
        {
            if (in_id.version != -1)
                p_pStreamOut->write_id(FIFF_PARENT_FILE_ID, in_id);//9

            p_pStreamOut->write_id(FIFF_BLOCK_ID);//10
            p_pStreamOut->write_id(FIFF_PARENT_BLOCK_ID, p_Nodes[k]->id);//11
        }
        for (p = 0; p < p_Nodes[k]->nent(); ++p)
        {
            //
            //   Do not copy these tags
            //
            if(p_Nodes[k]->dir[p]->kind == FIFF_BLOCK_ID || p_Nodes[k]->dir[p]->kind == FIFF_PARENT_BLOCK_ID || p_Nodes[k]->dir[p]->kind == FIFF_PARENT_FILE_ID)
                continue;

            //
            //   Read and write tags, pass data through transparently
            //
            if (!p_pStreamIn->device()->seek(p_Nodes[k]->dir[p]->pos)) //fseek(fidin, nodes(k).dir(p).pos, 'bof') == -1
            {
                printf("Could not seek to the tag\n");
                return false;
            }

            //ToDo this is the same like read_tag
            FiffTag::SPtr tag(new FiffTag());
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
            if (tag->size() > 0)
            {
                in->readRawData(tag->data(), tag->size());
                // Don't do this conversion because it breaks the writing on
                // little endian systems (i.e., OSX, Linux, Windows...)
                // FiffTag::convert_tag_data(tag,FIFFV_BIG_ENDIAN,FIFFV_NATIVE_ENDIAN);
            }

            //QDataStream out(p_pStreamOut);
            FiffStream::SPtr out = p_pStreamOut;
            out->setByteOrder(QDataStream::BigEndian);

            *out << (qint32)tag->kind;
            *out << (qint32)tag->type;
            *out << (qint32)tag->size();
            *out << (qint32)FIFFV_NEXT_SEQ;

            out->writeRawData(tag->data(),tag->size());
        }
        for(p = 0; p < p_Nodes[k]->nchild(); ++p)
        {
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
    if(this->type == p_kind)
        nodes.append(FiffDirNode::SPtr(new FiffDirNode(this)));

    QList<FiffDirNode::SPtr>::const_iterator i;
    for (i = this->children.begin(); i != this->children.end(); ++i)
        nodes.append((*i)->dir_tree_find(p_kind));

    return nodes;
}

//=============================================================================================================

bool FiffDirNode::find_tag(FiffStream* p_pStream, fiff_int_t findkind, FiffTag::SPtr& p_pTag) const
{
    for (qint32 p = 0; p < this->nent(); ++p)
    {
        if (this->dir[p]->kind == findkind)
        {
            p_pStream->read_tag(p_pTag,this->dir[p]->pos);
            return true;
        }
    }
    if (p_pTag)
        p_pTag.clear();

    return false;
}

//=============================================================================================================

bool FiffDirNode::has_tag(fiff_int_t findkind)
{
    for(qint32 p = 0; p < this->nent(); ++p)
        if(this->dir.at(p)->kind == findkind)
            return true;
    return false;
}

//=============================================================================================================

bool FiffDirNode::has_kind(fiff_int_t p_kind) const
{
    if(this->type == p_kind)
        return true;

    QList<FiffDirNode::SPtr>::const_iterator i;
    for(i = this->children.begin(); i != this->children.end(); ++i)
        if((*i)->has_kind(p_kind))
            return true;

    return false;
}

//=============================================================================================================

void FiffDirNode::print(int indent) const
{
    int j, prev_kind,count;
    QList<FiffDirEntry::SPtr> dentry = this->dir;

    for (int k = 0; k < indent; k++)
        putchar(' ');
    explain_block (this->type);
    printf (" { ");
    if (!this->id.isEmpty())
        this->id.print();
    printf ("\n");

    for (j = 0, prev_kind = -1, count = 0; j < this->nent(); j++) {
        if (dentry[j]->kind != prev_kind) {
            if (count > 1)
                printf (" [%d]\n",count);
            else if (j > 0)
                putchar('\n');
            for (int k = 0; k < indent+2; k++)
                putchar(' ');
            explain (dentry[j]->kind);
            prev_kind = dentry[j]->kind;
            count = 1;
        }
        else
            count++;
        prev_kind = dentry[j]->kind;
    }
    if (count > 1)
        printf (" [%d]\n",count);
    else if (j > 0)
        putchar ('\n');
    for (j = 0; j < this->nchild(); j++)
        this->children[j]->print(indent+5);
    for (int k = 0; k < indent; k++)
        putchar(' ');
    printf ("}\n");
}

//=============================================================================================================

void FiffDirNode::explain_block(int kind)
{
    for (int k = 0; _fiff_block_explanations[k].kind >= 0; k++) {
        if (_fiff_block_explanations[k].kind == kind) {
            printf ("%d = %s",kind,_fiff_block_explanations[k].text);
            return;
        }
    }
    printf ("Cannot explain: %d",kind);
}

//=============================================================================================================

void FiffDirNode::explain(int kind)
{
    int k;
    for (k = 0; _fiff_explanations[k].kind >= 0; k++) {
        if (_fiff_explanations[k].kind == kind) {
            printf ("%d = %s",kind,_fiff_explanations[k].text);
            return;
        }
    }
    printf ("Cannot explain: %d",kind);
}

//=============================================================================================================

const char *FiffDirNode::get_tag_explanation(int kind)
{
    int k;
    for (k = 0; _fiff_explanations[k].kind >= 0; k++) {
        if (_fiff_explanations[k].kind == kind)
            return _fiff_explanations[k].text;
    }
    return "unknown";
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
