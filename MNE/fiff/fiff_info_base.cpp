//=============================================================================================================
/**
* @file     fiff_info_base.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
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
* @brief    Implementation of the FiffInfoBase Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_info_base.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffInfoBase::FiffInfoBase()
: nchan(-1)
{
}


//*************************************************************************************************************

FiffInfoBase::FiffInfoBase(const FiffInfoBase& p_FiffInfoBase)
: nchan(p_FiffInfoBase.nchan)
, dev_head_t(p_FiffInfoBase.dev_head_t)
, ctf_head_t(p_FiffInfoBase.ctf_head_t)
, ch_names(p_FiffInfoBase.ch_names)
, bads(p_FiffInfoBase.bads)
{
    qint32 i;
    for(i = 0; i < p_FiffInfoBase.chs.size(); ++i)
        chs.append(p_FiffInfoBase.chs[i]);
}


//*************************************************************************************************************

FiffInfoBase::~FiffInfoBase()
{

}


//*************************************************************************************************************

void FiffInfoBase::clear()
{
    nchan = -1;
    chs.clear();
    ch_names.clear();
    dev_head_t.clear();
    ctf_head_t.clear();
    bads.clear();
}


//*************************************************************************************************************

RowVectorXi FiffInfoBase::pick_types(bool meg, bool eeg, bool stim, const QStringList& include, const QStringList& exclude) const
{
    RowVectorXi pick = RowVectorXi::Zero(this->nchan);

    fiff_int_t kind;
    qint32 k;
    for(k = 0; k < this->nchan; ++k)
    {
        kind = this->chs[k].kind;
        if ((kind == FIFFV_MEG_CH || kind == FIFFV_REF_MEG_CH) && meg)
            pick(0, k) = true;
        else if (kind == FIFFV_EEG_CH && eeg)
            pick(0, k) = true;
        else if ((kind == FIFFV_STIM_CH) && stim)
            pick(0, k) = true;
    }

    qint32 p = 0;
    QStringList myinclude;
    for(k = 0; k < this->nchan; ++k)
    {
        if (pick(0, k))
        {
            myinclude << this->ch_names[k];
            ++p;
        }
    }

//        qDebug() << "Size: " << myinclude.size();
//        qDebug() << myinclude;

    if (include.size() > 0)
    {
        for (k = 0; k < include.size(); ++k)
        {
            myinclude << include[k];
            ++p;
        }
    }

//        qDebug() << "Size: " << myinclude.size();
//        qDebug() << myinclude;

    RowVectorXi sel;
    if (p != 0)
        sel = FiffInfoBase::pick_channels(this->ch_names, myinclude, exclude);

    return sel;
}


//*************************************************************************************************************

RowVectorXi FiffInfoBase::pick_channels(const QStringList& ch_names, const QStringList& include, const QStringList& exclude)
{
    qint32 nchan = ch_names.size();
    RowVectorXi sel = RowVectorXi::Zero(nchan);
    qint32 i, k, p, c, count, nzero;
    if (include.size() == 0 && exclude.size() == 0)
    {
        sel.setOnes();
        for(k = 0; k < nchan; ++k)
            sel[k] = k;//+1 using MATLAB notation
        return sel;
    }

    if (include.size() == 0)
    {
        //
        //   Include all initially
        //
        sel.setZero();
        for (k = 0; k < nchan; ++k)
            sel[k] = k; //+1 using MATLAB notation

        qint32 nzero = 0;
        for(k = 0; k < exclude.size(); ++k)
        {
            count = 0;
            for (i = ch_names.size()-1; i >= 0 ; --i)
            {
                if (QString::compare(exclude[k],ch_names[i]) == 0)
                {
                    ++count;
                    c = i;
                }
            }
            nzero = 0;//does this make sense? - its here because its in the MATLAB file
            if(count > 0)
            {
                sel[c] = -1;//to elimnate channels use -1 instead of 0 - cause its zero based indexed
                ++nzero;
            }
        }
        //
        //  Check for exclusions
        //
        if(nzero > 0)
        {
            RowVectorXi newsel = RowVectorXi::Zero(nchan-nzero);
            p = 0;
            for(k = 0; k < nchan; ++k)
            {
                if (sel[k] >= 0)
                {
                    newsel[p] = sel[k];
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
        sel.resize(include.size());
        sel.setZero();
        nzero = 0;
        for(k = 0; k < include.size(); ++k)
        {
            count = 0;
            for (i = ch_names.size()-1; i >= 0 ; --i)
            {
                if (QString::compare(include[k],ch_names[i]) == 0)
                {
                    count += 1;
                    c = i;
                }
            }
            if (count == 0)
                printf("Missing channel %s\n",include[k].toUtf8().constData());
            else if (count > 1)
                printf("Ambiguous channel, taking first occurence: %s",include[k].toUtf8().constData());

            //
            //  Is this channel in the exclusion list?
            //
            sel[k] = c;//+1 using MATLAB notation
            if (exclude.size() > 0)
            {
                count = 0;
                for (i = 0; i < exclude.size(); ++i)
                {
                    if (QString::compare(include[k],exclude[i]) == 0)
                        ++count;
                }
                if (count > 0)
                {
                    sel[k] = -1;//to elimnate channels use -1 instead of 0 - cause its zero based indexed
                    ++nzero;
                }
            }
        }
        //
        //    Check whether some channels were excluded
        //
        if (nzero > 0)
        {
            RowVectorXi newsel = RowVectorXi::Zero(include.size()-nzero);

            p = 0;
            for(k = 0; k < include.size(); ++k)
            {
                if (sel(0,k) >= 0) // equal also cause of zero based picking
                {
                    newsel[p] = sel[k];
                    ++p;
                }
            }
            sel.resize(newsel.cols());
            sel = newsel;
        }
    }
    return sel;
}


//*************************************************************************************************************

FiffInfoBase FiffInfoBase::pick_info(const MatrixXi* sel) const
{
    FiffInfoBase res = *this;//new FiffInfo(this);
    if (sel == NULL)
        return res;

    //ToDo when pointer List do delation
    res.chs.clear();
    res.ch_names.clear();

    qint32 idx;
    for(qint32 i = 0; i < sel->cols(); ++i)
    {
        idx = (*sel)(0,i);
        res.chs.append(this->chs[idx]);
        res.ch_names.append(this->ch_names[idx]);
    }
    res.nchan  = sel->cols();

    return res;
}
