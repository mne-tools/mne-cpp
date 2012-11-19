//=============================================================================================================
/**
* @file     fiff_raw_data.cpp
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
* @brief    Contains the implementation of the FiffRawData Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_raw_data.h"


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
, proj(NULL)
{
}


//*************************************************************************************************************

FiffRawData::~FiffRawData()
{
    if(file)
        delete file;
    if(info)
        delete info;
    if(proj)
        delete proj;
}

//*************************************************************************************************************

bool FiffRawData::read_raw_segment(MatrixXd*& data, MatrixXd*& times, fiff_int_t from, fiff_int_t to, MatrixXi sel)
{
    if(data)
        delete data;

    bool projAvailable = true;

    if (!this->proj)
        projAvailable = false;

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
//    MatrixXd cal(nchan,nchan);
    SparseMatrix<double> cal(nchan,nchan);
//    cal.setZero();
    cal.reserve(nchan);
    for(i = 0; i < nchan; ++i)
        cal.insert(i,i) = this->cals(0,i);
    cal.makeCompressed();

    MatrixXd mult_full;
    //
    if (sel.cols() == 0)
    {
        data = new MatrixXd(nchan, to-from+1);
//            data->setZero();
        if (projAvailable || this->comp.kind != -1)
        {
            if (!projAvailable)
                mult_full = (*this->comp.data->data)*cal;
            else if (this->comp.kind == -1)
                mult_full = (*this->proj)*cal;
            else
                mult_full = (*this->proj)*(*this->comp.data->data)*cal;
        }
    }
    else
    {
        data = new MatrixXd(sel.cols(),to-from+1);
//            data->setZero();

        MatrixXd selVect(sel.cols(), nchan);

        selVect.setZero();

        if (!projAvailable && this->comp.kind == -1)
        {
            cal.resize(sel.cols(),sel.cols());
//            cal.setZero();
            for( i = 0; i  < sel.cols(); ++i)
                cal.insert(i,i) = this->cals(0,sel(0,i));
            cal.makeCompressed();
        }
        else
        {
            if (!projAvailable)
            {
                qDebug() << "This has to be debugged! #1";
                for( i = 0; i  < sel.cols(); ++i)
                    selVect.row(i) = this->comp.data->data->block(sel(0,i),0,1,nchan);
                mult_full = selVect*cal;
            }
            else if (this->comp.kind == -1)
            {
                for( i = 0; i  < sel.cols(); ++i)
                    selVect.row(i) = this->proj->block(sel(0,i),0,1,nchan);

                mult_full = selVect*cal;
            }
            else
            {
                qDebug() << "This has to be debugged! #3";
                for( i = 0; i  < sel.cols(); ++i)
                    selVect.row(i) = this->proj->block(sel(0,i),0,1,nchan);

                mult_full = selVect*(*this->comp.data->data)*cal;
            }
        }
    }
    bool do_debug = false;

    //
    // Make mult sparse
    //
    SparseMatrix<double> mult(mult_full.rows(),mult_full.cols());
    for(i = 0; i < mult_full.rows(); ++i)
    {
        for(k = 0; k < mult_full.cols(); ++k)
        {
            if(mult_full(i,k) != 0)
                mult.insert(i,k) = mult_full(i,k);
        }
    }
    mult.makeCompressed();

    //

    FiffStream* fid = NULL;
    if (!this->file->device()->isOpen())
    {
        if (!this->file->device()->open(QIODevice::ReadOnly))
        {
            printf("Cannot open file %s",this->info->filename.toUtf8().constData());
        }
        fid = this->file;
    }
    else
    {
        fid = this->file;
    }

    MatrixXd one;
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
                            one = cal*(Map< MatrixDau16 >( t_pTag->toDauPack16(),nchan, thisRawDir.nsamp)).cast<double>();
                        else if(t_pTag->type == FIFFT_INT)
                            one = cal*(Map< MatrixXi >( t_pTag->toInt(),nchan, thisRawDir.nsamp)).cast<double>();
                        else if(t_pTag->type == FIFFT_FLOAT)
                            one = cal*(Map< MatrixXf >( t_pTag->toFloat(),nchan, thisRawDir.nsamp)).cast<double>();
                        else
                            printf("Data Storage Format not known jet [1]!! Type: %d\n", t_pTag->type);
                    }
                    else
                    {

                        //ToDo find a faster solution for this!! --> make cal and mul sparse like in MATLAB
                        MatrixXd newData(sel.cols(), thisRawDir.nsamp); //ToDo this can be done much faster, without newData

                        if (t_pTag->type == FIFFT_DAU_PACK16)
                        {
                            MatrixXd tmp_data = (Map< MatrixDau16 > ( t_pTag->toDauPack16(),nchan, thisRawDir.nsamp)).cast<double>();

                            for(r = 0; r < sel.cols(); ++r)
                                newData.block(r,0,1,thisRawDir.nsamp) = tmp_data.block(sel(0,r),0,1,thisRawDir.nsamp);
                        }
                        else if(t_pTag->type == FIFFT_INT)
                        {
                            MatrixXd tmp_data = (Map< MatrixXi >( t_pTag->toInt(),nchan, thisRawDir.nsamp)).cast<double>();

                            for(r = 0; r < sel.cols(); ++r)
                                newData.block(r,0,1,thisRawDir.nsamp) = tmp_data.block(sel(0,r),0,1,thisRawDir.nsamp);
                        }
                        else if(t_pTag->type == FIFFT_FLOAT)
                        {
                            MatrixXd tmp_data = (Map< MatrixXf > ( t_pTag->toFloat(),nchan, thisRawDir.nsamp)).cast<double>();

                            for(r = 0; r < sel.cols(); ++r)
                                newData.block(r,0,1,thisRawDir.nsamp) = tmp_data.block(sel(0,r),0,1,thisRawDir.nsamp);
                        }
                        else
                        {
                            printf("Data Storage Format not known jet [2]!! Type: %d\n", t_pTag->type);
                        }

                        one = cal*newData;
                    }
                }
                else
                {
                    if (t_pTag->type == FIFFT_DAU_PACK16)
                        one = mult*(Map< MatrixDau16 >( t_pTag->toDauPack16(),nchan, thisRawDir.nsamp)).cast<double>();
                    else if(t_pTag->type == FIFFT_INT)
                        one = mult*(Map< MatrixXi >( t_pTag->toInt(),nchan, thisRawDir.nsamp)).cast<double>();
                    else if(t_pTag->type == FIFFT_FLOAT)
                        one = mult*(Map< MatrixXf >( t_pTag->toFloat(),nchan, thisRawDir.nsamp)).cast<double>();
                    else
                        printf("Data Storage Format not known jet [3]!! Type: %d\n", t_pTag->type);
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
                last_pick  = thisRawDir.nsamp - 1;
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
//                    qDebug() << "This needs to be debugged!";
                    last_pick = thisRawDir.nsamp + to - thisRawDir.last - 1;//is this alright?
                    if (do_debug)
                        printf("M");
                }
                else
                {
                    //
                    //  From the middle to the end
                    //
                    last_pick = thisRawDir.nsamp - 1;
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
                last_pick  = to - thisRawDir.first;// + 1;
                if (do_debug)
                    printf("B");
            }
            //
            //  Now we are ready to pick
            //
            picksamp = last_pick - first_pick + 1;

            if(do_debug)
            {
                qDebug() << "first_pick: " << first_pick;
                qDebug() << "last_pick: " << last_pick;
                qDebug() << "picksamp: " << picksamp;
            }

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

    times = new MatrixXd(1, to-from+1);

    for (i = 0; i < times->cols(); ++i)
        (*times)(0, i) = ((float)(from+i)) / this->info->sfreq;

    return true;
}
