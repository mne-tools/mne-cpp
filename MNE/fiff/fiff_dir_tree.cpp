//=============================================================================================================
/**
* @file     fiff_dir_tree.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the implementation of the FiffDirTree Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_dir_tree.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffDirTree::FiffDirTree()
: block(-1)
, nent(-1)
, nent_tree(-1)
, nchild(-1)
{
}


//*************************************************************************************************************

FiffDirTree::~FiffDirTree()
{
    QList<FiffDirTree*>::iterator i;
    for (i = this->children.begin(); i != this->children.end(); ++i)
        if (*i)
            delete *i;
}


//*************************************************************************************************************

bool FiffDirTree::copy_tree(FiffFile* fidin, FiffId& in_id, QList<FiffDirTree*>& nodes, FiffFile* fidout)
{
    if(nodes.size() <= 0)
        return false;

    qint32 k, p;

    for(k = 0; k < nodes.size(); ++k)
    {
        fidout->start_block(nodes[k]->block);//8
        if (nodes[k]->id.version != -1)
        {
            if (in_id.version != -1)
                fidout->write_id(FIFF_PARENT_FILE_ID, in_id);//9

            fidout->write_id(FIFF_BLOCK_ID);//10
            fidout->write_id(FIFF_PARENT_BLOCK_ID, nodes[k]->id);//11
        }
        for (p = 0; p < nodes[k]->nent; ++p)
        {
            //
            //   Do not copy these tags
            //
            if(nodes[k]->dir[p].kind == FIFF_BLOCK_ID || nodes[k]->dir[p].kind == FIFF_PARENT_BLOCK_ID || nodes[k]->dir[p].kind == FIFF_PARENT_FILE_ID)
                continue;

            //
            //   Read and write tags, pass data through transparently
            //
            if (!fidin->seek(nodes[k]->dir[p].pos)) //fseek(fidin, nodes(k).dir(p).pos, 'bof') == -1
            {
                printf("Could not seek to the tag\n");
                return false;
            }

            FiffTag tag;

            QDataStream in(fidin);
            in.setByteOrder(QDataStream::BigEndian);

            in >> tag.kind;// = fread(fidin, 1, 'int32');
            in >> tag.type;// = fread(fidin, 1, 'uint32');
            in >> tag.size;// = fread(fidin, 1, 'int32');
            in >> tag.next;// = fread(fidin, 1, 'int32');


            if (tag.size > 0)
            {
                if (tag.data == NULL)
                    tag.data = malloc(tag.size + ((tag.type == FIFFT_STRING) ? 1 : 0));
                else
                    tag.data = realloc(tag.data,tag.size + ((tag.type == FIFFT_STRING) ? 1 : 0));

                if (tag.data == NULL) {
                    printf("fiff_read_tag: memory allocation failed.\n");//consider throw
                    return false;
                }
                char *t_pCharData = static_cast< char* >(tag.data);
                in.readRawData(t_pCharData, tag.size);
                if (tag.type == FIFFT_STRING)
                    t_pCharData[tag.size] = NULL;//make sure that char ends with NULL
                FiffTag::convert_tag_data(&tag,FIFFV_BIG_ENDIAN,FIFFV_NATIVE_ENDIAN);
            } //tag.data = fread(fidin, tag.size, 'uchar');


            QDataStream out(fidout);
            out.setByteOrder(QDataStream::BigEndian);

            out << (qint32)tag.kind;
            out << (qint32)tag.type;
            out << (qint32)tag.size;
            out << (qint32)FIFFV_NEXT_SEQ;

            out.writeRawData(static_cast< const char* >(tag.data),tag.size);
        }
        for(p = 0; p < nodes[k]->nchild; ++p)
        {
            QList<FiffDirTree*> childList;
            childList << nodes[k]->children[p];
            FiffDirTree::copy_tree(fidin, in_id, childList, fidout);
        }
        fidout->end_block(nodes[k]->block);
    }
    return true;
}


//*************************************************************************************************************

qint32 FiffDirTree::make_dir_tree(FiffFile* p_pFile, QList<FiffDirEntry>* p_pDir, FiffDirTree*& p_pTree, qint32 start)
{
    if (p_pTree != NULL)
        delete p_pTree;
    p_pTree = new FiffDirTree();

    FIFFLIB::FiffTag* t_pTag = NULL;

    qint32 block;
    if(p_pDir->at(start).kind == FIFF_BLOCK_START)
    {
        FiffTag::read_tag(p_pFile, t_pTag, p_pDir->at(start).pos);
        block = *t_pTag->toInt();
    }
    else
    {
        block = 0;
    }

//    qDebug() << "start { " << p_pTree->block;

    qint32 current = start;

    p_pTree->block = block;
    p_pTree->nent = 0;
    p_pTree->nchild = 0;

    while (current < p_pDir->size())
    {
        if (p_pDir->at(current).kind == FIFF_BLOCK_START)
        {
            if (current != start)
            {
                FiffDirTree* p_pChildTree = NULL;
                current = FiffDirTree::make_dir_tree(p_pFile,p_pDir,p_pChildTree, current);
                ++p_pTree->nchild;
                p_pTree->children.append(p_pChildTree);
            }
        }
        else if(p_pDir->at(current).kind == FIFF_BLOCK_END)
        {
            FiffTag::read_tag(p_pFile, t_pTag, p_pDir->at(start).pos);
            if (*t_pTag->toInt() == p_pTree->block)
                break;
        }
        else
        {
            ++p_pTree->nent;
            p_pTree->dir.append(p_pDir->at(current));

            //
            //  Add the id information if available
            //
            if (block == 0)
            {
                if (p_pDir->at(current).kind == FIFF_FILE_ID)
                {
                    FiffTag::read_tag(p_pFile, t_pTag, p_pDir->at(current).pos);
                    p_pTree->id = t_pTag->toFiffID();
                }
            }
            else
            {
                if (p_pDir->at(current).kind == FIFF_BLOCK_ID)
                {
                    FiffTag::read_tag(p_pFile, t_pTag, p_pDir->at(current).pos);
                    p_pTree->id = t_pTag->toFiffID();
                }
                else if (p_pDir->at(current).kind == FIFF_PARENT_BLOCK_ID)
                {
                    FiffTag::read_tag(p_pFile, t_pTag, p_pDir->at(current).pos);
                    p_pTree->parent_id = t_pTag->toFiffID();
                }
            }
        }
        ++current;
    }

    //
    // Eliminate the empty directory
    //
    if(p_pTree->nent == 0)
        p_pTree->dir.clear();

//    qDebug() << "block =" << p_pTree->block << "nent =" << p_pTree->nent << "nchild =" << p_pTree->nchild;
//    qDebug() << "end } " << block;

    delete t_pTag;

    return current;
}


//*************************************************************************************************************

QList<FiffDirTree*> FiffDirTree::dir_tree_find(fiff_int_t kind)
{
    QList<FiffDirTree*> nodes;
    if(this->block == kind)
        nodes.append(this);

    QList<FiffDirTree*>::iterator i;
    for (i = this->children.begin(); i != this->children.end(); ++i)
        nodes.append((*i)->dir_tree_find(kind));

    return nodes;
}


//*************************************************************************************************************

bool FiffDirTree::find_tag(FiffFile* p_pFile, fiff_int_t findkind, FiffTag*& p_pTag)
{
    for (qint32 p = 0; p < this->nent; ++p)
    {
       if (this->dir.at(p).kind == findkind)
       {
          FiffTag::read_tag(p_pFile,p_pTag,this->dir.at(p).pos);
          return true;
       }
    }
    if (p_pTag != NULL)
    {
        delete p_pTag;
        p_pTag = NULL;
    }
    return false;
}


//*************************************************************************************************************

bool FiffDirTree::has_tag(fiff_int_t findkind)
{
    for(qint32 p = 0; p < this->nent; ++p)
        if(this->dir.at(p).kind == findkind)
            return true;
   return false;
}

