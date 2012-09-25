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

MatrixXi Fiff::pick_channels(QStringList& ch_names, QStringList& include, QStringList& exclude)
{
    qint32 nchan = ch_names.size();
    MatrixXi sel(1, nchan);
    qint32 i, k, p, c, count, nzero;
    if (include.size() == 0 && exclude.size() == 0)
    {
        sel.setOnes();
        for(k = 0; k < nchan; ++k)
            sel(0, k) = k;//+1 using MATLAB notation
        return sel;
    }

    if (include.size() == 0)
    {
        //
        //   Include all initially
        //
        sel.setZero();
        for (k = 0; k < nchan; ++k)
            sel(0, k) = k;//+1 using MATLAB notation

        qint32 nzero = 0;
        for(k = 0; k < exclude.size(); ++k)
        {
            count = 0;
            for (i = ch_names.size()-1; i >= 0 ; --i)
            {
                if (QString::compare(exclude.at(k),ch_names.at(i)) == 0)
                {
                    count += 1;
                    c = i;
                }
            }
            nzero = 0;//does this make sense? - its here because its in the MATLAB file
            if(count > 0)
            {
                sel(0, c) = 0;
                ++nzero;
            }
        }
        //
        //  Check for exclusions
        //
        if(nzero > 0)
        {
            MatrixXi newsel(1,nchan-nzero);
            newsel.setZero();
            p = 0;
            for(k = 0; k < nchan; ++k)
            {
                if (sel(0, k) > 0)
                {
                    newsel(0, p) = sel(0, k);
                    ++p;
                }
            }
            sel = newsel;
        }
    }
    else
    {
        //
        //   First do the channels to be included
        //
        sel.resize(1,include.size());
        sel.setZero();
        nzero = 0;
        for(k = 0; k < include.size(); ++k)
        {
            count = 0;
            for (i = ch_names.size()-1; i >= 0 ; --i)
            {
                if (QString::compare(include.at(k),ch_names.at(i)) == 0)
                {
                    count += 1;
                    c = i;
                }
            }
            if (count == 0)
                printf("Missing channel %s\n",include.at(k).toUtf8().constData());
            else if (count > 1)
                printf("Ambiguous channel, taking first occurence: %s",include.at(k).toUtf8().constData());

            //
            //  Is this channel in the exclusion list?
            //
            sel(0,k) = c;//+1 using MATLAB notation
            if (exclude.size() > 0)
            {

                count = 0;
                for (i = exclude.size()-1; i >= 0 ; --i)
                {
                    if (QString::compare(include.at(k),exclude.at(i)) == 0)
                        count += 1;
                }
                if (count > 0)
                {
                    sel(0, k) = 0;
                    ++nzero;
                }
            }
        }
        //
        //    Check whether some channels were excluded
        //
        if (nzero > 0)
        {
            MatrixXi newsel(1,include.size()-nzero);
            newsel.setZero();

            p = 0;
            for(k = 0; k < include.size(); ++k)
            {
                if (sel(0,k) > 0)
                {
                    newsel(0,p) = sel(0,k);
                    ++p;
                }
            }
            sel.resize(newsel.rows(), newsel.cols());
            sel = newsel;
        }
    }
    return sel;
}



//*************************************************************************************************************

MatrixXi Fiff::pick_types(FiffInfo* info, bool meg, bool eeg, bool stim, QStringList& include, QStringList& exclude)
{
    MatrixXi pick(1, info->nchan);
    pick.setZero();

    fiff_int_t kind;
    qint32 k;
    for(k = 0; k < info->nchan; ++k)
    {
        kind = info->chs.at(k).kind;
        if ((kind == FIFFV_MEG_CH || kind == FIFFV_REF_MEG_CH) && meg)
            pick(0, k) = true;
        else if (kind == FIFFV_EEG_CH && eeg)
            pick(0, k) = true;
        else if ((kind == FIFFV_STIM_CH) && stim)
            pick(0, k) = true;
    }

    qint32 p = 0;
    QStringList myinclude;
    for(k = 0; k < info->nchan; ++k)
    {
        if (pick(0, k))
        {
            myinclude << info->ch_names.at(k);
            ++p;
        }
    }

//        qDebug() << "Size: " << myinclude.size();
//        qDebug() << myinclude;

    if (include.size() > 0)
    {
        for (k = 0; k < include.size(); ++k)
        {
            myinclude << include.at(k);
            ++p;
        }
    }

//        qDebug() << "Size: " << myinclude.size();
//        qDebug() << myinclude;

    MatrixXi sel;
    if (p != 0)
        sel = Fiff::pick_channels(info->ch_names, myinclude, exclude);

    return sel;
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

    FiffFile* p_pFile = new FiffFile(t_sFileName);
    FiffDirTree* t_pTree = NULL;
    QList<fiff_dir_entry_t>* t_pDir = NULL;

    if(!p_pFile->open(t_pTree, t_pDir))
    {
        if(t_pTree)
            delete t_pTree;

        if(t_pDir)
            delete t_pDir;

        return false;
    }

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
