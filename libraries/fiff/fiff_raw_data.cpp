//=============================================================================================================
/**
 * @file     fiff_raw_data.cpp
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
 * @brief    Definition of the FiffRawData Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_raw_data.h"
#include "fiff_tag.h"
#include "fiff_stream.h"
#include "cstdlib"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffRawData::FiffRawData()
: first_samp(-1)
, last_samp(-1)
{
}

//=============================================================================================================

FiffRawData::FiffRawData(QIODevice &p_IODevice)
: first_samp(-1)
, last_samp(-1)
{
    //setup FiffRawData object
    if(!FiffStream::setup_read_raw(p_IODevice, *this))
    {
        printf("\tError during fiff setup raw read.\n");
        //exit(EXIT_FAILURE); //ToDo Throw here, e.g.: throw std::runtime_error("IO Error! File not found");
        return;
    }
}

//=============================================================================================================

FiffRawData::FiffRawData(QIODevice &p_IODevice, bool b_littleEndian)
: first_samp(-1)
, last_samp(-1)
{
    //setup FiffRawData object
    if(!FiffStream::setup_read_raw(p_IODevice, *this, false, b_littleEndian))
    {
        printf("\tError during fiff setup raw read.\n");
        //exit(EXIT_FAILURE); //ToDo Throw here, e.g.: throw std::runtime_error("IO Error! File not found");
        return;
    }
}

//=============================================================================================================

FiffRawData::FiffRawData(const FiffRawData &p_FiffRawData)
: file(p_FiffRawData.file)
, info(p_FiffRawData.info)
, first_samp(p_FiffRawData.first_samp)
, last_samp(p_FiffRawData.last_samp)
, cals(p_FiffRawData.cals)
, rawdir(p_FiffRawData.rawdir)
, proj(p_FiffRawData.proj)
, comp(p_FiffRawData.comp)
{
}

//=============================================================================================================

FiffRawData::~FiffRawData()
{
}

//=============================================================================================================

void FiffRawData::clear()
{
    info.clear();
    first_samp = -1;
    last_samp = -1;
    cals = RowVectorXd();
    rawdir.clear();
    proj = MatrixXd();
    comp.clear();
}

//=============================================================================================================
#include <QElapsedTimer>
#include <QDebug>
bool FiffRawData::read_raw_segment(MatrixXd& data,
                                   MatrixXd& times,
                                   fiff_int_t from,
                                   fiff_int_t to,
                                   const RowVectorXi& sel,
                                   bool do_debug) const
{
    bool projAvailable = true;

    if (this->proj.size() == 0) {
        //qDebug() << "FiffRawData::read_raw_segment - No projectors setup. Consider calling MNE::setup_compensators.";
        projAvailable = false;
    }

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
        printf("No data in this range %d ... %d  =  %9.3f ... %9.3f secs...", from, to, ((float)from)/this->info.sfreq, ((float)to)/this->info.sfreq);
        return false;
    }
    //printf("Reading %d ... %d  =  %9.3f ... %9.3f secs...", from, to, ((float)from)/this->info.sfreq, ((float)to)/this->info.sfreq);
    //
    //  Initialize the data and calibration vector
    //
    qint32 nchan = this->info.nchan;
    qint32 dest  = 0;//1;
    qint32 i, k, r;

    typedef Eigen::Triplet<double> T;
    std::vector<T> tripletList;
    tripletList.reserve(nchan);
    for(i = 0; i < nchan; ++i)
        tripletList.push_back(T(i, i, this->cals[i]));

    SparseMatrix<double> cal(nchan, nchan);
    cal.setFromTriplets(tripletList.begin(), tripletList.end());
//    cal.makeCompressed();

    MatrixXd mult_full;
    //
    if (sel.size() == 0)
    {
        data = MatrixXd(nchan, to-from+1);
//            data->setZero();
        if (projAvailable || this->comp.kind != -1)
        {
            if (!projAvailable)
                mult_full = this->comp.data->data*cal;
            else if (this->comp.kind == -1)
                mult_full = this->proj*cal;
            else
                mult_full = this->proj*this->comp.data->data*cal;
        }
    }
    else
    {
        data = MatrixXd(sel.size(),to-from+1);
//            data->setZero();

        MatrixXd selVect(sel.size(), nchan);

        selVect.setZero();

        if (!projAvailable && this->comp.kind == -1)
        {
            tripletList.clear();
            tripletList.reserve(sel.size());
            for(i = 0; i < sel.size(); ++i)
                tripletList.push_back(T(i, i, this->cals[sel[i]]));
            cal = SparseMatrix<double>(sel.size(), sel.size());
            cal.setFromTriplets(tripletList.begin(), tripletList.end());
        }
        else
        {
            if (!projAvailable)
            {
                qDebug() << "This has to be debugged! #1";
                for( i = 0; i  < sel.size(); ++i)
                    selVect.row(i) = this->comp.data->data.block(sel[i],0,1,nchan);
                mult_full = selVect*cal;
            }
            else if (this->comp.kind == -1)
            {
                for( i = 0; i  < sel.size(); ++i)
                    selVect.row(i) = this->proj.block(sel[i],0,1,nchan);

                mult_full = selVect*cal;
            }
            else
            {
                qDebug() << "This has to be debugged! #3";
                for( i = 0; i  < sel.size(); ++i)
                    selVect.row(i) = this->proj.block(sel[i],0,1,nchan);

                mult_full = selVect*this->comp.data->data*cal;
            }
        }
    }

    //
    // Make mult sparse
    //
    tripletList.clear();
    tripletList.reserve(mult_full.rows()*mult_full.cols());
    for(i = 0; i < mult_full.rows(); ++i)
        for(k = 0; k < mult_full.cols(); ++k)
            if(mult_full(i,k) != 0)
                tripletList.push_back(T(i, k, mult_full(i,k)));

    SparseMatrix<double> mult(mult_full.rows(),mult_full.cols());
    if(tripletList.size() > 0)
        mult.setFromTriplets(tripletList.begin(), tripletList.end());
//    mult.makeCompressed();

    FiffStream::SPtr fid;
    if (!this->file->device()->isOpen())
    {
        if (!this->file->device()->open(QIODevice::ReadOnly))
        {
            printf("Cannot open file %s",this->info.filename.toUtf8().constData());
        }
        fid = this->file;
    }
    else
    {
        fid = this->file;
    }

    MatrixXd one, newData, tmp_data;
    FiffRawDir thisRawDir;
    FiffTag::SPtr t_pTag;
    fiff_int_t first_pick, last_pick, picksamp;
    for(k = 0; k < this->rawdir.size(); ++k)
    {
        thisRawDir = this->rawdir[k];
        //
        //  Do we need this buffer
        //
        if (thisRawDir.last > from)
        {
            if (thisRawDir.ent->kind == -1)
            {
                //
                //  Take the easy route: skip is translated to zeros
                //
                if(do_debug)
                    printf("S");
                if (sel.cols() <= 0)
                    one.resize(nchan,thisRawDir.nsamp);
                else
                    one.resize(sel.cols(),thisRawDir.nsamp);

                one.setZero();
            }
            else
            {
                fid->read_tag(t_pTag, thisRawDir.ent->pos);
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
                        else if(t_pTag->type == FIFFT_SHORT)
                            one = cal*(Map< MatrixShort >( t_pTag->toShort(),nchan, thisRawDir.nsamp)).cast<double>();
                        else
                            printf("Data Storage Format not known yet [1]!! Type: %d\n", t_pTag->type);
                    }
                    else
                    {
                        //ToDo find a faster solution for this!! --> make cal and mul sparse like in MATLAB
                        newData.resize(sel.cols(), thisRawDir.nsamp); //ToDo this can be done much faster, without newData

                        if (t_pTag->type == FIFFT_DAU_PACK16)
                        {
                            tmp_data = (Map< MatrixDau16 > ( t_pTag->toDauPack16(),nchan, thisRawDir.nsamp)).cast<double>();

                            for(r = 0; r < sel.size(); ++r)
                                newData.block(r,0,1,thisRawDir.nsamp) = tmp_data.block(sel[r],0,1,thisRawDir.nsamp);
                        }
                        else if(t_pTag->type == FIFFT_INT)
                        {
                            tmp_data = (Map< MatrixXi >( t_pTag->toInt(),nchan, thisRawDir.nsamp)).cast<double>();

                            for(r = 0; r < sel.size(); ++r)
                                newData.block(r,0,1,thisRawDir.nsamp) = tmp_data.block(sel[r],0,1,thisRawDir.nsamp);
                        }
                        else if(t_pTag->type == FIFFT_FLOAT)
                        {
                            tmp_data = (Map< MatrixXf > ( t_pTag->toFloat(),nchan, thisRawDir.nsamp)).cast<double>();

                            for(r = 0; r < sel.size(); ++r)
                                newData.block(r,0,1,thisRawDir.nsamp) = tmp_data.block(sel[r],0,1,thisRawDir.nsamp);
                        }
                        else if(t_pTag->type == FIFFT_SHORT)
                        {
                            tmp_data = (Map< MatrixShort > ( t_pTag->toShort(),nchan, thisRawDir.nsamp)).cast<double>();

                            for(r = 0; r < sel.size(); ++r)
                                newData.block(r,0,1,thisRawDir.nsamp) = tmp_data.block(sel[r],0,1,thisRawDir.nsamp);
                        }
                        else
                        {
                            printf("Data Storage Format not known yet [2]!! Type: %d\n", t_pTag->type);
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
                        printf("Data Storage Format not known yet [3]!! Type: %d\n", t_pTag->type);
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
                data.block(0,dest,data.rows(),picksamp) = one.block(0, first_pick, data.rows(), picksamp);

                dest += picksamp;
            }
        }
        //
        //  Done?
        //
        if (thisRawDir.last >= to)
        {
            //printf(" [done]\n");
            break;
        }
    }

    if (!this->file->device()->isOpen()) {
        this->file->device()->close();
    }

    times = MatrixXd(1, to-from+1);

    for (i = 0; i < times.cols(); ++i)
        times(0, i) = ((float)(from+i)) / this->info.sfreq;

    return true;
}

//=============================================================================================================

bool FiffRawData::read_raw_segment(MatrixXd& data,
                                   MatrixXd& times,
                                   SparseMatrix<double>& multSegment,
                                   fiff_int_t from,
                                   fiff_int_t to,
                                   const RowVectorXi& sel,
                                   bool do_debug) const
{
    bool projAvailable = true;

    if (this->proj.size() == 0) {
        //qInfo() << "FiffRawData::read_raw_segment - No projectors setup. Consider calling MNE::setup_compensators.";
        projAvailable = false;
    }

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
    //printf("Reading %d ... %d  =  %9.3f ... %9.3f secs...", from, to, ((float)from)/this->info.sfreq, ((float)to)/this->info.sfreq);
    //
    //  Initialize the data and calibration vector
    //
    qint32 nchan = this->info.nchan;
    qint32 dest  = 0;//1;
    qint32 i, k, r;

    typedef Eigen::Triplet<double> T;
    std::vector<T> tripletList;
    tripletList.reserve(nchan);
    for(i = 0; i < nchan; ++i)
        tripletList.push_back(T(i, i, this->cals[i]));

    SparseMatrix<double> cal(nchan, nchan);
    cal.setFromTriplets(tripletList.begin(), tripletList.end());
//    cal.makeCompressed();

    MatrixXd mult_full;
    //
    if (sel.size() == 0)
    {
        data = MatrixXd(nchan, to-from+1);
//            data->setZero();
        if (projAvailable || this->comp.kind != -1)
        {
            if (!projAvailable)
                mult_full = this->comp.data->data*cal;
            else if (this->comp.kind == -1)
                mult_full = this->proj*cal;
            else
                mult_full = this->proj*this->comp.data->data*cal;
        }
    }
    else
    {
        data = MatrixXd(sel.size(),to-from+1);
//            data->setZero();

        MatrixXd selVect(sel.size(), nchan);

        selVect.setZero();

        if (!projAvailable && this->comp.kind == -1)
        {
            tripletList.clear();
            tripletList.reserve(sel.size());
            for(i = 0; i < sel.size(); ++i)
                tripletList.push_back(T(i, i, this->cals[sel[i]]));
            cal = SparseMatrix<double>(sel.size(), sel.size());
            cal.setFromTriplets(tripletList.begin(), tripletList.end());
        }
        else
        {
            if (!projAvailable)
            {
                qDebug() << "This has to be debugged! #1";
                for( i = 0; i  < sel.size(); ++i)
                    selVect.row(i) = this->comp.data->data.block(sel[i],0,1,nchan);
                mult_full = selVect*cal;
            }
            else if (this->comp.kind == -1)
            {
                for( i = 0; i  < sel.size(); ++i)
                    selVect.row(i) = this->proj.block(sel[i],0,1,nchan);

                mult_full = selVect*cal;
            }
            else
            {
                qDebug() << "This has to be debugged! #3";
                for( i = 0; i  < sel.size(); ++i)
                    selVect.row(i) = this->proj.block(sel[i],0,1,nchan);

                mult_full = selVect*this->comp.data->data*cal;
            }
        }
    }

    //
    // Make mult sparse
    //
    tripletList.clear();
    tripletList.reserve(mult_full.rows()*mult_full.cols());
    for(i = 0; i < mult_full.rows(); ++i)
        for(k = 0; k < mult_full.cols(); ++k)
            if(mult_full(i,k) != 0)
                tripletList.push_back(T(i, k, mult_full(i,k)));

    SparseMatrix<double> mult(mult_full.rows(),mult_full.cols());
    if(tripletList.size() > 0)
        mult.setFromTriplets(tripletList.begin(), tripletList.end());
//    mult.makeCompressed();

    //

    FiffStream::SPtr fid;
    if (!this->file->device()->isOpen())
    {
        if (!this->file->device()->open(QIODevice::ReadOnly))
        {
            printf("Cannot open file %s",this->info.filename.toUtf8().constData());
        }
        fid = this->file;
    }
    else
    {
        fid = this->file;
    }

    MatrixXd one;
    fiff_int_t first_pick, last_pick, picksamp;
    for(k = 0; k < this->rawdir.size(); ++k)
    {
        FiffRawDir thisRawDir = this->rawdir[k];
        //
        //  Do we need this buffer
        //
        if (thisRawDir.last > from)
        {
            if (thisRawDir.ent->kind == -1)
            {
                //
                //  Take the easy route: skip is translated to zeros
                //
                if(do_debug)
                    printf("S");
                if (sel.cols() <= 0)
                    one.resize(nchan,thisRawDir.nsamp);
                else
                    one.resize(sel.cols(),thisRawDir.nsamp);

                one.setZero();
            }
            else
            {
                FiffTag::SPtr t_pTag;
                fid->read_tag(t_pTag, thisRawDir.ent->pos);
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
                        else if(t_pTag->type == FIFFT_SHORT)
                            one = cal*(Map< MatrixShort >( t_pTag->toShort(),nchan, thisRawDir.nsamp)).cast<double>();
                        else
                            printf("Data Storage Format not known yet [1]!! Type: %d\n", t_pTag->type);
                    }
                    else
                    {

                        //ToDo find a faster solution for this!! --> make cal and mul sparse like in MATLAB
                        MatrixXd newData(sel.cols(), thisRawDir.nsamp); //ToDo this can be done much faster, without newData

                        if (t_pTag->type == FIFFT_DAU_PACK16)
                        {
                            MatrixXd tmp_data = (Map< MatrixDau16 > ( t_pTag->toDauPack16(),nchan, thisRawDir.nsamp)).cast<double>();

                            for(r = 0; r < sel.size(); ++r)
                                newData.block(r,0,1,thisRawDir.nsamp) = tmp_data.block(sel[r],0,1,thisRawDir.nsamp);
                        }
                        else if(t_pTag->type == FIFFT_INT)
                        {
                            MatrixXd tmp_data = (Map< MatrixXi >( t_pTag->toInt(),nchan, thisRawDir.nsamp)).cast<double>();

                            for(r = 0; r < sel.size(); ++r)
                                newData.block(r,0,1,thisRawDir.nsamp) = tmp_data.block(sel[r],0,1,thisRawDir.nsamp);
                        }
                        else if(t_pTag->type == FIFFT_FLOAT)
                        {
                            MatrixXd tmp_data = (Map< MatrixXf > ( t_pTag->toFloat(),nchan, thisRawDir.nsamp)).cast<double>();

                            for(r = 0; r < sel.size(); ++r)
                                newData.block(r,0,1,thisRawDir.nsamp) = tmp_data.block(sel[r],0,1,thisRawDir.nsamp);
                        }
                        else if(t_pTag->type == FIFFT_SHORT)
                        {
                            MatrixXd tmp_data = (Map< MatrixShort > ( t_pTag->toShort(),nchan, thisRawDir.nsamp)).cast<double>();

                            for(r = 0; r < sel.size(); ++r)
                                newData.block(r,0,1,thisRawDir.nsamp) = tmp_data.block(sel[r],0,1,thisRawDir.nsamp);
                        }
                        else
                        {
                            printf("Data Storage Format not known yet [2]!! Type: %d\n", t_pTag->type);
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
                        printf("Data Storage Format not known yet [3]!! Type: %d\n", t_pTag->type);
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
                data.block(0,dest,data.rows(),picksamp) = one.block(0, first_pick, data.rows(), picksamp);

                dest += picksamp;
            }
        }
        //
        //  Done?
        //
        if (thisRawDir.last >= to)
        {
            //printf(" [done]\n");
            break;
        }
    }

    if(mult.cols()==0)
        multSegment = cal;
    else
        multSegment = mult;

    if (!this->file->device()->isOpen()) {
        this->file->device()->close();
    }

    times = MatrixXd(1, to-from+1);

    for (i = 0; i < times.cols(); ++i)
        times(0, i) = ((float)(from+i)) / this->info.sfreq;

    return true;
}

//=============================================================================================================

bool FiffRawData::read_raw_segment_times(MatrixXd& data,
                                         MatrixXd& times,
                                         float from,
                                         float to,
                                         const RowVectorXi& sel) const
{
    //
    //   Convert to samples
    //
    from = floor(from*this->info.sfreq);
    to   = ceil(to*this->info.sfreq);
    //
    //   Read it
    //
    return this->read_raw_segment(data, times, (qint32)from, (qint32)to, sel);
}
