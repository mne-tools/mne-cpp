//=============================================================================================================
/**
* @file     fiff.cpp
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

#include "fiff.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Fiff::Fiff()
{
}


//*************************************************************************************************************

bool Fiff::open(QString& p_sFileName, QFile*& p_pFile, FiffDirTree*& p_pTree, QList<fiff_dir_entry_t>*& p_pDir)
{
    if (p_pFile)
    {
        p_pFile->close();
        delete p_pFile;
    }
    p_pFile = new QFile(p_sFileName);

    if (!p_pFile->open(QIODevice::ReadOnly))
    {
        printf("Cannot open file %s\n", p_pFile->fileName().toUtf8().constData());//consider throw
        return false;
    }

    FIFFLIB::FiffTag* t_pTag = NULL;
    FiffTag::read_tag_info(p_pFile, t_pTag);

    if (t_pTag->kind != FIFF_FILE_ID)
    {
        printf("Fiff::open: file does not start with a file id tag");//consider throw
        return false;
    }

    if (t_pTag->type != FIFFT_ID_STRUCT)
    {
        printf("Fiff::open: file does not start with a file id tag");//consider throw
        return false;
    }
    if (t_pTag->size != 20)
    {
        printf("Fiff::open: file does not start with a file id tag");//consider throw
        return false;
    }

    FiffTag::read_tag(p_pFile, t_pTag);

    if (t_pTag->kind != FIFF_DIR_POINTER)
    {
        printf("Fiff::open: file does have a directory pointer");//consider throw
        return false;
    }

    //
    //   Read or create the directory tree
    //
//    qDebug() << "\nCreating tag directory for "<< p_sFileName << "...";


    if (p_pDir)
        delete p_pDir;
    p_pDir = new QList<fiff_dir_entry_t>;

    qint32 dirpos = *t_pTag->toInt();
    if (dirpos > 0)
    {
        FiffTag::read_tag(p_pFile, t_pTag, dirpos);
        *p_pDir = t_pTag->toDirEntry();
    }
    else
    {
        int k = 0;
        p_pFile->seek(0);//fseek(fid,0,'bof');
        //dir = struct('kind',{},'type',{},'size',{},'pos',{});
        fiff_dir_entry_t t_fiffDirEntry;
        while (t_pTag->next >= 0)
        {
            t_fiffDirEntry.pos = p_pFile->pos();//pos = ftell(fid);
            FiffTag::read_tag_info(p_pFile, t_pTag);
            ++k;
            t_fiffDirEntry.kind = t_pTag->kind;
            t_fiffDirEntry.type = t_pTag->type;
            t_fiffDirEntry.size = t_pTag->size;
            p_pDir->append(t_fiffDirEntry);
        }
    }
    delete t_pTag;
    //
    //   Create the directory tree structure
    //

    FiffDirTree::make_dir_tree(p_pFile, p_pDir, p_pTree);

//    qDebug() << "[done]\n";

    //
    //   Back to the beginning
    //
    p_pFile->seek(0); //fseek(fid,0,'bof');
    return true;
}


//*************************************************************************************************************

bool Fiff::read_named_matrix(QFile* p_pFile, FiffDirTree* node, fiff_int_t matkind, FiffSolution*& mat)
{
    if (mat != NULL)
        delete mat;
    mat = new FiffSolution();
    //
    //   Descend one level if necessary
    //
    bool found_it = false;
    if (node->block != FIFFB_MNE_NAMED_MATRIX)
    {
        for (int k = 0; k < node->nchild; ++k)
        {
            if (node->children.at(k)->block == FIFFB_MNE_NAMED_MATRIX)
            {
                if(node->children.at(k)->has_tag(matkind))
                {
                    node = node->children.at(k);
                    found_it = true;
                    break;
                }
            }
       }
       if (!found_it)
       {
          printf("Fiff::read_named_matrix: Desired named matrix (kind = %d) not available\n",matkind);
          return false;
       }
    }
    else
    {
        if (!node->has_tag(matkind))
        {
            printf("Desired named matrix (kind = %d) not available",matkind);
            return false;
        }
    }

    FIFFLIB::FiffTag* t_pTag = NULL;
    //
    //   Read everything we need
    //
    if(!node->find_tag(p_pFile, matkind, t_pTag))
    {
        printf("Matrix data missing.\n");
        return false;
    }
    else
    {
        //qDebug() << "Is Matrix" << t_pTag->isMatrix() << "Special Type:" << t_pTag->getType();
        mat->data = t_pTag->toFloatMatrix().transpose();
    }

    mat->nrow = mat->data.rows();
    mat->ncol = mat->data.cols();

    if(node->find_tag(p_pFile, FIFF_MNE_NROW, t_pTag))
        if (*t_pTag->toInt() != mat->nrow)
        {
            printf("Number of rows in matrix data and FIFF_MNE_NROW tag do not match");
            return false;
        }
    if(node->find_tag(p_pFile, FIFF_MNE_NCOL, t_pTag))
        if (*t_pTag->toInt() != mat->ncol)
        {
            printf("Number of columns in matrix data and FIFF_MNE_NCOL tag do not match");
            return false;
        }

    QString row_names;
    if(node->find_tag(p_pFile, FIFF_MNE_ROW_NAMES, t_pTag))
        row_names = t_pTag->toString();

    QString col_names;
    if(node->find_tag(p_pFile, FIFF_MNE_COL_NAMES, t_pTag))
        col_names = t_pTag->toString();

    //
    //   Put it together
    //
    if (!row_names.isEmpty())
        mat->row_names = Fiff::split_name_list(row_names);

    if (!col_names.isEmpty())
        mat->col_names = Fiff::split_name_list(col_names);

    return true;
}
