//=============================================================================================================
/**
* @file     fiff_raw_data.cpp
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
* @brief    Contains the implementation of the FiffRawData Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../include/fiff_raw_data.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffRawData::FiffRawData()
: file(NULL)
, info(NULL)
{

}


//*************************************************************************************************************

FiffRawData::~FiffRawData()
{
    if(file)
        delete file;
    if(info)
        delete info;
}


//*************************************************************************************************************

bool FiffRawData::read_raw_segment(MatrixXf*& data, MatrixXf*& times, fiff_int_t from, fiff_int_t to, MatrixXi sel)
{
    if(from == -1)
        from = this->first_samp;
    if(to == -1)
        to = this->last_samp;
    //
    //  Initial checks
    //
    if(from < this->first_samp)
        from = this->first_samp;
    if(to > this->last_samp)
        to = this->last_samp;
    //
    if(from > to)
    {
        printf("No data in this range\n");
        return false;
    }
    printf("Reading %d ... %d  =  %9.3f ... %9.3f secs...", from, to, ((float)from)/this->info->sfreq, ((float)to)/this->info->sfreq);
    //
    //  Initialize the data and calibration vector
    //
    qint32 nchan = this->info->nchan;
    qint32 dest  = 0;//1;
    qint32 i, k, r;
    MatrixXf cal(nchan,nchan);
    cal.setZero();
    for(i = 0; i < nchan; ++i)
        cal(i,i) = this->cals(0,i);

    MatrixXf mult;
    //
    if (sel.cols() == 0)
    {
        if(data)
            delete data;
        data = new MatrixXf(nchan, to-from+1);
//            data->setZero();
        if (this->proj.kind != -1 || this->comp.kind != -1)
        {
            if (this->proj.kind == -1)
                mult = this->comp.data->data*cal;
            else if (this->comp.kind == -1)
                mult = this->proj.data->data*cal;
            else
                mult = this->proj.data->data*this->comp.data->data*cal;
        }
    }
    else
    {
        if(data)
            delete data;
        data = new MatrixXf(sel.cols(),to-from+1);
//            data->setZero();

        MatrixXf selVect(sel.cols(), sel.cols());

        selVect.setZero();

        if (this->proj.kind == -1 && this->comp.kind == -1)
        {
            for( i = 0; i  < sel.cols(); ++i)
                selVect(i,i) = this->cals(0,sel(0,i));

            cal  = selVect;
        }
        else
        {
            if (this->proj.kind == -1)
            {
                qDebug() << "This has to be debugged!";
                for( i = 0; i  < sel.cols(); ++i)
                    selVect(i,i) = this->comp.data->data(0,sel(0,i));
                mult = selVect*cal;
            }
            else if (this->comp.kind == -1)
            {
                qDebug() << "This has to be debugged!";
                for( i = 0; i  < sel.cols(); ++i)
                    selVect(i,i) = this->proj.data->data(0,sel(0,i));
                mult = selVect*cal;
            }
            else
            {
                qDebug() << "This has to be debugged!";
                for( i = 0; i  < sel.cols(); ++i)
                    selVect(i,i) = this->proj.data->data(0,sel(0,i));
                mult = selVect*this->comp.data->data*cal;
            }
        }
    }
    bool do_debug = false;
//        if ~isempty(cal)
//            cal = sparse(cal);
//        end
//        if ~isempty(mult)
//            mult = sparse(mult);
//        end

    FiffFile* fid = NULL;
    if (!this->file->isOpen())
    {
        if (!this->file->open(QIODevice::ReadOnly))
        {
            printf("Cannot open file %s",this->info->filename.toUtf8().constData());
        }
        fid = this->file;
    }
    else
    {
        fid = this->file;
    }

    MatrixXf one;
    bool doing_whole;
    fiff_int_t first_pick, last_pick, picksamp;
    for(k = 0; k < this->rawdir.size(); ++k)
    {
        FiffRawDir thisRawDir = this->rawdir[k];
        //
        //  Do we need this buffer
        //
        if (thisRawDir.last > from)
        {
            if (thisRawDir.ent.kind == -1)
            {
                //
                //  Take the easy route: skip is translated to zeros
                //
                if(do_debug)
                    printf("S");
                doing_whole = false;
                if (sel.cols() <= 0)
                    one.resize(nchan,thisRawDir.nsamp);
                else
                    one.resize(sel.cols(),thisRawDir.nsamp);

                one.setZero();
            }
            else
            {
                FIFFLIB::FiffTag* t_pTag = NULL;
                FiffTag::read_tag(fid, t_pTag, thisRawDir.ent.pos);
                //
                //   Depending on the state of the projection and selection
                //   we proceed a little bit differently
                //
                if (mult.cols() == 0)
                {
                    if (sel.cols() == 0)
                    {
                        if (t_pTag->type == FIFFT_DAU_PACK16)
                        {
                            MatrixDau16 tmp_data = (Map< MatrixDau16 >( t_pTag->toDauPack16(),nchan, thisRawDir.nsamp));
                            one = cal*tmp_data.cast<float>();
                        }
                        else
                        {
                            printf("Data Storage Format not known jet!!\n");
                        }
                    }
                    else
                    {
                        MatrixXf newData(sel.cols(), thisRawDir.nsamp);

                        if (t_pTag->type == FIFFT_DAU_PACK16)
                        {
                            MatrixXf tmp_data = (Map< MatrixDau16 >( t_pTag->toDauPack16(),nchan, thisRawDir.nsamp)).cast<float>();

                            for(r = 0; r < sel.cols(); ++r)
                                newData.block(r,0,1,thisRawDir.nsamp) = tmp_data.block(sel(0,r),0,1,thisRawDir.nsamp);
                        }
                        else
                        {
                            printf("Data Storage Format not known jet!!\n");
                        }

                        one = cal*newData;
                    }
                }
                else
                {
                    if (t_pTag->type == FIFFT_DAU_PACK16)
                    {
                        MatrixDau16 data = (Map< MatrixDau16 >( t_pTag->toDauPack16(),nchan, thisRawDir.nsamp));
                        one = mult*data.cast<float>();
                    }
                    else
                    {
                        printf("Data Storage Format not known jet!!\n");
                    }


                }
            }
            //
            //  The picking logic is a bit complicated
            //
            if (to >= thisRawDir.last && from <= thisRawDir.first)
            {
                //
                //  We need the whole buffer
                //
                first_pick = 0;//1;
                last_pick  = thisRawDir.nsamp;
                if (do_debug)
                    printf("W");
            }
            else if (from > thisRawDir.first)
            {
                first_pick = from - thisRawDir.first;// + 1;
                if(to < thisRawDir.last)
                {
                    //
                    //  Something from the middle
                    //
                    qDebug() << "This needs to be debugged!";
                    last_pick = thisRawDir.nsamp + to - thisRawDir.last;//is this alright?
                    if (do_debug)
                        printf("M");
                }
                else
                {
                    //
                    //  From the middle to the end
                    //
                    last_pick = thisRawDir.nsamp;
                    if (do_debug)
                        printf("E");
                }
            }
            else
            {
                //
                //  From the beginning to the middle
                //
                first_pick = 0;//1;
                last_pick  = to - thisRawDir.first + 1;
                if (do_debug)
                    printf("B");
            }
            //
            //  Now we are ready to pick
            //
            picksamp = last_pick - first_pick;// + 1;
            if (picksamp > 0)
            {
//                    for(r = 0; r < data->rows(); ++r)
//                        for(c = 0; c < picksamp; ++c)
//                            (*data)(r,dest + c) = one(r,first_pick + c);
                data->block(0,dest,data->rows(),picksamp) = one.block(0, first_pick, data->rows(), picksamp);

                dest += picksamp;
            }
        }
        //
        //  Done?
        //
        if (thisRawDir.last >= to)
        {
            printf(" [done]\n");
            break;
        }
    }

//        fclose(fid);

    if(times)
        delete times;

    times = new MatrixXf(1, to-from+1);

    for (i = 0; i < times->cols(); ++i)
        (*times)(0, i) = ((float)(from+i)) / this->info->sfreq;

    return true;
}
