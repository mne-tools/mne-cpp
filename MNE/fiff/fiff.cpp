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
            sel(0, k) = k; //+1 using MATLAB notation

        qint32 nzero = 0;
        for(k = 0; k < exclude.size(); ++k)
        {
            count = 0;
            for (i = ch_names.size()-1; i >= 0 ; --i)
            {
                if (QString::compare(exclude.at(k),ch_names.at(i)) == 0)
                {
                    ++count;
                    c = i;
                }
            }
            nzero = 0;//does this make sense? - its here because its in the MATLAB file
            if(count > 0)
            {
                sel(0, c) = -1;//to elimnate channels use -1 instead of 0 - cause its zero based indexed
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
                if (sel(0, k) >= 0)
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
                for (i = 0; i < exclude.size(); ++i)
                {
                    if (QString::compare(include.at(k),exclude.at(i)) == 0)
                        ++count;
                }
                if (count > 0)
                {
                    sel(0, k) = -1;//to elimnate channels use -1 instead of 0 - cause its zero based indexed
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
                if (sel(0,k) >= 0) // equal also cause of zero based picking
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
