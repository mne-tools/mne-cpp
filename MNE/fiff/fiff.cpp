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
* @brief    Contains the implementation of the FIFF Wrapper Class.
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
    printf("\nCreating tag directory for %s...\n", p_sFileName.toUtf8().constData());

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

bool Fiff::setup_read_raw(QString t_sFileName, FiffRawData*& data, bool allow_maxshield)
{
    if(data)
        delete data;
    data = NULL;

    //
    //   Open the file
    //
    printf("Opening raw data file %s...\n",t_sFileName.toUtf8().constData());

    QFile* p_pFile = NULL;
    FiffDirTree* t_pTree = NULL;
    QList<fiff_dir_entry_t>* t_pDir = NULL;

    if(!Fiff::open(t_sFileName, p_pFile, t_pTree, t_pDir))
        return false;
    //
    //   Read the measurement info
    //
//        [ info, meas ] = fiff_read_meas_info(fid,tree);
    FiffInfo* info = NULL;
    FiffDirTree* meas = t_pTree->read_meas_info(p_pFile, info);

    if (!meas)
        return false; //ToDo garbage collecting

    //
    //   Locate the data of interest
    //
    QList<FiffDirTree*> raw = meas->dir_tree_find(FIFFB_RAW_DATA);
    if (raw.size() == 0)
    {
        raw = meas->dir_tree_find(FIFFB_CONTINUOUS_DATA);
        if(allow_maxshield)
        {
            for (qint32 i = 0; i < raw.size(); ++i)
                if(raw[i])
                    delete raw[i];
            raw = meas->dir_tree_find(FIFFB_SMSH_RAW_DATA);
            if (raw.size() == 0)
            {
                printf("No raw data in %s\n", t_sFileName.toUtf8().constData());
                return false;
            }
        }
        else
        {
            if (raw.size() == 0)
            {
                printf("No raw data in %s\n", t_sFileName.toUtf8().constData());
                return false;
            }
        }
    }

    //
    //   Set up the output structure
    //
    info->filename   = t_sFileName;

    data = new FiffRawData();
    data->m_pFile = p_pFile;// fid;
    data->info       = info;
    data->first_samp = 0;
    data->last_samp  = 0;
    //
    //   Process the directory
    //
    QList<fiff_dir_entry_t> dir = raw.at(0)->dir;
    fiff_int_t nent = raw.at(0)->nent;
    fiff_int_t nchan = info->nchan;
    fiff_int_t first = 0;
    fiff_int_t first_samp = 0;
    fiff_int_t first_skip   = 0;
    //
    //  Get first sample tag if it is there
    //
    FiffTag* t_pTag = NULL;
    if (dir.at(first).kind == FIFF_FIRST_SAMPLE)
    {
        FiffTag::read_tag(p_pFile, t_pTag, dir.at(first).pos);
        first_samp = *t_pTag->toInt();
        ++first;
    }
    //
    //  Omit initial skip
    //
    if (dir.at(first).kind == FIFF_DATA_SKIP)
    {
        //
        //  This first skip can be applied only after we know the buffer size
        //
        FiffTag::read_tag(p_pFile, t_pTag, dir.at(first).pos);
        first_skip = *t_pTag->toInt();
        ++first;
    }
    data->first_samp = first_samp;
    //
    //   Go through the remaining tags in the directory
    //
    QList<FiffRawDir> rawdir;
//        rawdir = struct('ent',{},'first',{},'last',{},'nsamp',{});
    fiff_int_t nskip = 0;
    fiff_int_t ndir  = 0;
    fiff_int_t nsamp = 0;
    for (qint32 k = first; k < nent; ++k)
    {
        fiff_dir_entry_t ent = dir.at(k);
        if (ent.kind == FIFF_DATA_SKIP)
        {
            FiffTag::read_tag(p_pFile, t_pTag, ent.pos);
            nskip = *t_pTag->toInt();
        }
        else if(ent.kind == FIFF_DATA_BUFFER)
        {
            //
            //   Figure out the number of samples in this buffer
            //
            switch(ent.type)
            {
                case FIFFT_DAU_PACK16:
                    nsamp = ent.size/(2*nchan);
                    break;
                case FIFFT_SHORT:
                    nsamp = ent.size/(2*nchan);
                    break;
                case FIFFT_FLOAT:
                    nsamp = ent.size/(4*nchan);
                    break;
                case FIFFT_INT:
                    nsamp = ent.size/(4*nchan);
                    break;
                default:
                    printf("Cannot handle data buffers of type %d\n",ent.type);
                    return false;
            }
            //
            //  Do we have an initial skip pending?
            //
            if (first_skip > 0)
            {
                first_samp += nsamp*first_skip;
                data->first_samp = first_samp;
                first_skip = 0;
            }
            //
            //  Do we have a skip pending?
            //
            if (nskip > 0)
            {
                FiffRawDir t_RawDir;
                t_RawDir.first = first_samp;
                t_RawDir.last  = first_samp + nskip*nsamp - 1;//ToDo -1 right or is that MATLAB syntax
                t_RawDir.nsamp = nskip*nsamp;
                rawdir.append(t_RawDir);
                first_samp = first_samp + nskip*nsamp;
                nskip = 0;
                ++ndir;
            }
            //
            //  Add a data buffer
            //
            FiffRawDir t_RawDir;
            t_RawDir.ent   = ent;
            t_RawDir.first = first_samp;
            t_RawDir.last  = first_samp + nsamp - 1;//ToDo -1 right or is that MATLAB syntax
            t_RawDir.nsamp = nsamp;
            rawdir.append(t_RawDir);
            first_samp += nsamp;
            ++ndir;
        }
    }
    data->last_samp  = first_samp - 1;//ToDo -1 right or is that MATLAB syntax
    //
    //   Add the calibration factors
    //
    MatrixXf cals(1,data->info->nchan);
    cals.setZero();
    for (int k = 0; k < data->info->nchan; ++k)
        cals(0,k) = data->info->chs.at(k).range*data->info->chs.at(k).cal;
    //
    data->cals       = cals;
    data->rawdir     = rawdir;
    //data->proj       = [];
    //data.comp       = [];
    //
    printf("\tRange : %d ... %d  =  %9.3f ... %9.3f secs\n",
           data->first_samp,data->last_samp,
           (double)data->first_samp/data->info->sfreq,
           (double)data->last_samp/data->info->sfreq);
    printf("Ready.\n");
    data->m_pFile->close();
    return true;
}
