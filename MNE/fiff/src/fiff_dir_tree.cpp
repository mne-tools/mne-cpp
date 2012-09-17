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
* @brief    ToDo Documentation...
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../include/fiff_dir_tree.h"


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
{
}


//*************************************************************************************************************

FiffDirTree::~FiffDirTree()
{
    qDebug() << "Destructor dir tree";
    QList<FiffDirTree*>::iterator i;
    for (i = this->children.begin(); i != this->children.end(); ++i)
        if (*i != NULL)
            delete *i;
}


//*************************************************************************************************************

qint32 FiffDirTree::make_dir_tree(QFile* p_pFile, QList<fiff_dir_entry_t>* p_pDir, FiffDirTree*& p_pTree, qint32 start)
{
//    qDebug() << "Size Dir: " << p_pDir->size();

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

    int current = start;

    p_pTree->block = block;
    p_pTree->nent = 0;
    p_pTree->nchild = 0;
//        p_pTree->dir.append(p_pDir->at(current));

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

bool FiffDirTree::find_tag(QFile* p_pFile, fiff_int_t findkind, FiffTag*& p_pTag)
{
    for (int p = 0; p < this->nent; ++p)
    {
       if (this->dir.at(p).kind == findkind)
       {
          FiffTag::read_tag(p_pFile,p_pTag,this->dir.at(p).pos);
          return true;
       }
    }
    return false;
}


//*************************************************************************************************************

bool FiffDirTree::has_tag(fiff_int_t findkind)
{
    for(int p = 0; p < this->nent; ++p)
        if(this->dir.at(p).kind == findkind)
            return true;
   return false;
}


//*************************************************************************************************************

QStringList FiffDirTree::read_bad_channels(QFile* p_pFile)
{
    QList<FiffDirTree*> node = this->dir_tree_find(FIFFB_MNE_BAD_CHANNELS);
    FIFFLIB::FiffTag* t_pTag = NULL;

    QStringList bads;

    if (node.size() > 0)
        if(node.at(0)->find_tag(p_pFile, FIFF_MNE_CH_NAME_LIST, t_pTag))
            bads = FiffDirTree::split_name_list(t_pTag->toString());

    return bads;
}


//*************************************************************************************************************

QStringList FiffDirTree::split_name_list(QString p_sNameList)
{
    return p_sNameList.split(":");
}
