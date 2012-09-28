//=============================================================================================================
/**
* @file     fiff.h
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
* @brief    Contains the FIFF class declaration, which provides static wrapper functions to stay consistent
*           with mne matlab toolbox
*
*/

#ifndef FIFF_H
#define FIFF_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "include/fiff_constants.h"
#include "include/fiff_coord_trans.h"
#include "include/fiff_dir_tree.h"
#include "include/fiff_named_matrix.h"
#include "include/fiff_tag.h"
#include "include/fiff_types.h"
#include "include/fiff_proj.h"
#include "include/fiff_ctf_comp.h"
#include "include/fiff_info.h"
#include "include/fiff_raw_data.h"
#include "include/fiff_raw_dir.h"
#include "include/fiff_file.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include "../../include/3rdParty/Eigen/Core"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDataStream>
#include <QList>
#include <QStringList>

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

static QStringList defaultQStringList = QStringList();
static MatrixXi defaultMatrixXi(0,0);


typedef Matrix<qint16, Dynamic, Dynamic> MatrixDau16;


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//=============================================================================================================
/**
* DECLARE CLASS Fiff
*
* @brief The Fiff class provides...
*/
class FIFFSHARED_EXPORT Fiff
{
public:
    //=========================================================================================================
    /**
    * ctor
    */
    Fiff();

    //=========================================================================================================
    /**
    * dtor
    */
    ~Fiff()
    { }

    //Alphabetic ordered MNE Toolbox fiff_function

    //=========================================================================================================
    /**
    * fiff_dir_tree_find
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffDirTree dir_tree_find member function
    *
    * Find nodes of the given kind from a directory tree structure
    *
    * @param[in] tree the directory tree structure
    * @param[in] kind the given kind
    *
    * @return the found nodes
    */
    static inline QList<FiffDirTree*> dir_tree_find(FiffDirTree* tree, fiff_int_t kind)
    {
        return tree->dir_tree_find(kind);
    }

    //=========================================================================================================
    /**
    * fiff_invert_transform
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffCoordTrans::invert_transform static function
    *
    * Invert a coordinate transformation
    *
    * @param[in] p_pTransform the transformation which should be inverted
    *
    * @return true if succeeded, false otherwise
    */
    static inline bool invert_transform(FiffCoordTrans* p_pTransform)
    {
        return FiffCoordTrans::invert_transform(p_pTransform);
    }

    //=========================================================================================================
    /**
    * fiff_make_dir_tree
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffCoordTrans::invert_transform static function
    */
    static inline qint32 make_dir_tree(FiffFile* p_pFile, QList<fiff_dir_entry_t>* p_pDir, FiffDirTree*& p_pTree, qint32 start = 0)
    {
        return FiffDirTree::make_dir_tree(p_pFile, p_pDir, p_pTree, start);
    }

    //=========================================================================================================
    /**
    * fiff_open
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile open member function
    *
    * Opens a fif file and provides the directory of tags
    *
    * @param[in] p_sFileName file name of the file to open
    * @param[out] p_pFile file which is openened
    * @param[out] p_pTree tag directory organized into a tree
    * @param[out] p_pDir the sequential tag directory
    *
    * @return true if succeeded, false otherwise
    */
    static bool open(QString& p_sFileName, FiffFile*& p_pFile, FiffDirTree*& p_pTree, QList<fiff_dir_entry_t>*& p_pDir)
    {
        if(p_pFile)
            delete p_pFile;

        p_pFile = new FiffFile(p_sFileName);

        return p_pFile->open(p_pTree, p_pDir);
    }


    //=========================================================================================================
    /**
    * fiff_pick_channels
    *
    * function [sel] = fiff_pick_channels(ch_names,include,exclude)
    *
    * [sel] = fiff_pick_channels(ch_names,include,exclude)
    *
    * Make a selector to pick desired channels from data
    *
    * ch_names  - The channel name list to consult
    * include   - Channels to include (if empty, include all available)
    * exclude   - Channels to exclude (if empty, do not exclude any)
    *
    */
    static MatrixXi pick_channels(QStringList& ch_names, QStringList& include = defaultQStringList, QStringList& exclude = defaultQStringList);


    //=========================================================================================================
    /**
    * [sel] = fiff_pick_types(info,meg,eeg,stim,exclude)
    *
    * Create a selector to pick desired channel types from data
    *
    * info      - The measurement info
    * meg       - Include MEG channels
    * eeg       - Include EEG channels
    * stim      - Include stimulus channels
    * include   - Additional channels to include (if empty, do not add any)
    * exclude   - Channels to exclude (if empty, do not exclude any)
    *
    */
    //fiff_pick_types(raw.info,want_meg,want_eeg,want_stim,include,raw.info.bads)
    static MatrixXi pick_types(FiffInfo* info, bool meg, bool eeg = false, bool stim = false, QStringList& include = defaultQStringList, QStringList& exclude = defaultQStringList);


    //=========================================================================================================
    /**
    * fiff_read_bad_channels
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffDirTree read_bad_channels member function
    *
    * Reads the bad channel list from a node if it exists
    *
    * @param[in] p_pFile The opened fif file to read from
    * @param[in] p_pTree The node of interest
    *
    * @return the bad channel list
    */
    static inline QStringList read_bad_channels(FiffFile* p_pFile, FiffDirTree* p_pTree)
    {
        return p_pTree->read_bad_channels(p_pFile);
    }

    //=========================================================================================================
    /**
    * fiff_read_ctf_comp
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffDirTree read_ctf_comp member function
    *
    * [ compdata ] = fiff_read_ctf_comp(fid,node,chs)
    *
    * Read the CTF software compensation data from the given node
    *
    */
    static inline QList<FiffCtfComp*> read_ctf_comp(FiffFile* p_pFile, FiffDirTree* p_pNode, QList<FiffChInfo>& chs)
    {
        return p_pNode->read_ctf_comp(p_pFile, chs);
    }

    //=========================================================================================================
    /**
    * fiff_read_meas_info
    *
    * ### MNE toolbox root function ###
    *
    * [info,meas] = fiff_read_meas_info(source,tree)
    *
    * Read the measurement info
    *
    * If tree is specified, source is assumed to be an open file id,
    * otherwise a the name of the file to read. If tree is missing, the
    * meas output argument should not be specified.
    *
    */
    static inline FiffDirTree* read_meas_info(FiffFile* p_pFile, FiffDirTree* p_pTree, FiffInfo*& info)
    {
        return p_pTree->read_meas_info(p_pFile, info);
    }

    //=========================================================================================================
    /**
    * fiff_read_named_matrix
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffDirTree read_named_matrix member function
    *
    * ToDo
    */
    static inline bool read_named_matrix(FiffFile* p_pFile, FiffDirTree* node, fiff_int_t matkind, FiffNamedMatrix*& mat)
    {
        return node->read_named_matrix(p_pFile, matkind, mat);
    }

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###
    *
    * [ projdata ] = fiff_read_proj(fid,node)
    *
    * Read the SSP data under a given directory node
    *
    * Wrapper for the FiffDirTree read_proj member function
    *
    * ToDo
    */
    static inline QList<FiffProj*> read_proj(FiffFile* p_pFile, FiffDirTree* p_pNode)
    {
        return p_pNode-> read_proj(p_pFile);
    }




    //=========================================================================================================
    /**
    * ToDo make this part of FiffRawData
    *
    * fiff_read_raw_segment
    *
    * [data,times] = fiff_read_raw_segment(raw,from,to,sel)
    *
    * Read a specific raw data segment
    *
    * raw    - structure returned by fiff_setup_read_raw
    * from   - first sample to include. If omitted, defaults to the
    *          first sample in data
    * to     - last sample to include. If omitted, defaults to the last
    *          sample in data
    * sel    - optional channel selection vector
    *
    * data   - returns the data matrix (channels x samples)
    * times  - returns the time values corresponding to the samples (optional)
    *
    */
    //function [data,times] =
    static bool read_raw_segment(FiffRawData* raw, MatrixXf*& data, MatrixXf*& times, fiff_int_t from = -1, fiff_int_t to = -1, MatrixXi sel = defaultMatrixXi)
    {
        if(from == -1)
            from = raw->first_samp;
        if(to == -1)
            to = raw->last_samp;
        //
        //  Initial checks
        //
        if(from < raw->first_samp)
            from = raw->first_samp;
        if(to > raw->last_samp)
            to = raw->last_samp;
        //
        if(from > to)
        {
            printf("No data in this range\n");
            return false;
        }
        printf("Reading %d ... %d  =  %9.3f ... %9.3f secs...", from, to, ((float)from)/raw->info->sfreq, ((float)to)/raw->info->sfreq);
        //
        //  Initialize the data and calibration vector
        //
        qint32 nchan = raw->info->nchan;
        qint32 dest  = 0;//1;
        qint32 i, k, r, c;
        MatrixXf cal(nchan,nchan);
        cal.setZero();
        for(i = 0; i < nchan; ++i)
            cal(i,i) = raw->cals(0,i);

        MatrixXf mult;
        //
        if (sel.cols() == 0)
        {
            if(data)
                delete data;
            data = new MatrixXf(nchan, to-from+1);
//            data->setZero();
            if (raw->proj.kind != -1 || raw->comp.kind != -1)
            {
                if (raw->proj.kind == -1)
                    mult = raw->comp.data->data*cal;
                else if (raw->comp.kind == -1)
                    mult = raw->proj.data->data*cal;
                else
                    mult = raw->proj.data->data*raw->comp.data->data*cal;
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

            if (raw->proj.kind == -1 && raw->comp.kind == -1)
            {
                for( i = 0; i  < sel.cols(); ++i)
                    selVect(i,i) = raw->cals(0,sel(0,i));

                cal  = selVect;
            }
            else
            {
                if (raw->proj.kind == -1)
                {
                    qDebug() << "This has to be debugged!";
                    for( i = 0; i  < sel.cols(); ++i)
                        selVect(i,i) = raw->comp.data->data(0,sel(0,i));
                    mult = selVect*cal;
                }
                else if (raw->comp.kind == -1)
                {
                    qDebug() << "This has to be debugged!";
                    for( i = 0; i  < sel.cols(); ++i)
                        selVect(i,i) = raw->proj.data->data(0,sel(0,i));
                    mult = selVect*cal;
                }
                else
                {
                    qDebug() << "This has to be debugged!";
                    for( i = 0; i  < sel.cols(); ++i)
                        selVect(i,i) = raw->proj.data->data(0,sel(0,i));
                    mult = selVect*raw->comp.data->data*cal;
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
        if (!raw->m_pFile->isOpen())
        {
            if (!raw->m_pFile->open(QIODevice::ReadOnly))
            {
                printf("Cannot open file %s",raw->info->filename.toUtf8().constData());
            }
            fid = raw->m_pFile;
        }
        else
        {
            fid = raw->m_pFile;
        }

        MatrixXf one;
        bool doing_whole;
        fiff_int_t first_pick, last_pick, picksamp;
        for(k = 0; k < raw->rawdir.size(); ++k)
        {
            FiffRawDir thisRawDir = raw->rawdir[k];
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
            (*times)(0, i) = ((float)(from+i)) / raw->info->sfreq;

        return true;
    }


    //=========================================================================================================
    /**
    * fiff_read_tag
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffTag::read_tag function
    *
    * Read one tag from a fif file.
    * if pos is not provided, reading starts from the current file position
    *
    * @param[in] p_pFile opened fif file
    * @param[out] p_pTag the read tag
    * @param[in] pos position of the tag inside the fif file
    *
    * @return true if succeeded, false otherwise
    */
    static inline bool read_tag(FiffFile* p_pFile, FiffTag*& p_pTag, qint64 pos = -1)
    {
        return FiffTag::read_tag(p_pFile, p_pTag, pos);
    }

    //=========================================================================================================
    /**
    * fiff_read_tag_info
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffTag::read_tag_info function
    *
    * Read tag information of one tag from a fif file.
    * if pos is not provided, reading starts from the current file position
    *
    * @param[in] p_pFile opened fif file
    * @param[out] p_pTag the read tag info
    *
    * @return true if succeeded, false otherwise
    */
    static inline bool read_tag_info(FiffFile* p_pFile, FiffTag*& p_pTag)
    {
        return FiffTag::read_tag_info(p_pFile, p_pTag);
    }

    //=========================================================================================================
    /**
    * fiff_setup_read_raw
    *
    * ### MNE toolbox root function ###
    *
    * [data] = fiff_setup_read_raw(fname,allow_maxshield)
    *
    * Read information about raw data file
    *
    * fname               Name of the file to read
    * allow_maxshield     Accept unprocessed MaxShield data
    */
    static bool setup_read_raw(QString t_sFileName, FiffRawData*& data, bool allow_maxshield = false);

    //=========================================================================================================
    /**
    * fiff_split_name_list
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffDirTree::split_name_list static function
    */
    static inline QStringList split_name_list(QString p_sNameList)
    {
        return FiffDirTree::split_name_list(p_sNameList);
    }

    //=========================================================================================================
    /**
    * fiff_start_block
    *
    * fiff_start_block(fid,kind)
    *
    * Writes a FIFF_BLOCK_START tag
    *
    *     fid           An open fif file descriptor
    *     kind          The block kind to start
    *
    */
    static void start_block(FiffFile* p_pFile, fiff_int_t kind)
    {
        p_pFile->write_int(FIFF_BLOCK_START,&kind);
    }

    //=========================================================================================================
    /**
    * ToDo make this part of the FiffFile classs
    *
    * fiff_start_file
    *
    * ### MNE toolbox root function ###
    *
    * [fid] = fiff_start_file(name)
    *
    * Opens a fiff file for writing and writes the compulsory header tags
    *
    *     name           The name of the file to open. It is recommended
    *                    that the name ends with .fif
    *
    */
    static FiffFile* start_file(QString& p_sFileName)
    {
        return FiffFile::start_file(p_sFileName);
    }


    //=========================================================================================================
    /**
    * fiff_start_writing_raw
    *
    * ### MNE toolbox root function ###
    *
    * function [fid,cals] = fiff_start_writing_raw(name,info,sel)
    *
    * name       filename
    * info       The measurement info block of the source file
    * sel        Which channels will be included in the output file (optional)
    *
    */
    static FiffFile* start_writing_raw(QString& p_sFileName, FiffInfo* info, MatrixXf*& cals, MatrixXi sel = defaultMatrixXi)
    {
        //
        //   We will always write floats
        //
        fiff_int_t data_type = 4;
        qint32 k;

        if(sel.cols() == 0)
        {
            sel.resize(1,info->nchan);
            for (k = 0; k < info->nchan; ++k)
                sel(0, k) = k; //+1 when MATLAB notation
        }

        QList<FiffChInfo> chs;

        for(k = 0; k < sel.cols(); ++k)
            chs << info->chs.at(sel(0,k));

        fiff_int_t nchan = chs.size();
        //
        //  Create the file and save the essentials
        //

        FiffFile* t_pFile = Fiff::start_file(p_sFileName);
        t_pFile->start_block(FIFFB_MEAS);
        t_pFile->write_id(FIFF_BLOCK_ID);
        if(info->meas_id.version != -1)
        {
            t_pFile->write_id(FIFF_PARENT_BLOCK_ID,info->meas_id);
        }
        //
        //
        //    Measurement info
        //
        t_pFile->start_block(FIFFB_MEAS_INFO);
        //
        //    Blocks from the original
        //
        QList<fiff_int_t> blocks;
        blocks << FIFFB_SUBJECT << FIFFB_HPI_MEAS << FIFFB_HPI_RESULT << FIFFB_ISOTRAK << FIFFB_PROCESSING_HISTORY;
        bool have_hpi_result = false;
        bool have_isotrak    = false;
        if (blocks.size() > 0 && !info->filename.isEmpty())
        {
            FiffFile* t_pFile2 = new FiffFile(info->filename);

            FiffDirTree* t_pTree = NULL;
            QList<fiff_dir_entry_t>* t_pDir = NULL;
            t_pFile2->open(t_pTree, t_pDir);

            for(qint32 k = 0; k < blocks.size(); ++k)
            {
                QList<FiffDirTree*> nodes = t_pTree->dir_tree_find(blocks.at(k));
                FiffDirTree::copy_tree(t_pFile2,t_pTree->id,nodes,t_pFile);
                if(blocks[k] == FIFFB_HPI_RESULT && nodes.size() > 0)
                    have_hpi_result = true;

                if(blocks[k] == FIFFB_ISOTRAK && nodes.size() > 0)
                    have_isotrak = true;
            }

            delete t_pDir;
            delete t_pTree;
            delete t_pFile2;
            t_pFile2 = NULL;
        }
        //
        //    megacq parameters
        //
        if (!info->acq_pars.isEmpty() || !info->acq_stim.isEmpty())
        {
            t_pFile->start_block(FIFFB_DACQ_PARS);
            if (!info->acq_pars.isEmpty())
                t_pFile->write_string(FIFF_DACQ_PARS, info->acq_pars);

            if (!info->acq_stim.isEmpty())
                t_pFile->write_string(FIFF_DACQ_STIM, info->acq_stim);

            t_pFile->end_block(FIFFB_DACQ_PARS);
        }
        //
        //    Coordinate transformations if the HPI result block was not there
        //
        if (!have_hpi_result)
        {
            if (info->dev_head_t.from != -1)
                t_pFile->write_coord_trans(info->dev_head_t);

            if (info->ctf_head_t.from != -1)
                t_pFile->write_coord_trans(info->ctf_head_t);
        }
        //
        //    Polhemus data
        //
        if (info->dig.size() > 0 && !have_isotrak)
        {
            t_pFile->start_block(FIFFB_ISOTRAK);
            for (qint32 k = 0; k < info->dig.size(); ++k)
                t_pFile->write_dig_point(info->dig[k]);

            t_pFile->end_block(FIFFB_ISOTRAK);
        }
        //
        //    Projectors
        //
        t_pFile->write_proj(info->projs);
        //
        //    CTF compensation info
        //
        t_pFile->write_ctf_comp(info->comps);
        //
        //    Bad channels
        //
        if (info->bads.size() > 0)
        {
            t_pFile->start_block(FIFFB_MNE_BAD_CHANNELS);
            t_pFile->write_name_list(FIFF_MNE_CH_NAME_LIST,info->bads);
            t_pFile->end_block(FIFFB_MNE_BAD_CHANNELS);
        }
        //
        //    General
        //
        t_pFile->write_float(FIFF_SFREQ,&info->sfreq);
        t_pFile->write_float(FIFF_HIGHPASS,&info->highpass);
        t_pFile->write_float(FIFF_LOWPASS,&info->lowpass);
        t_pFile->write_int(FIFF_NCHAN,&nchan);
        t_pFile->write_int(FIFF_DATA_PACK,&data_type);
        if (info->meas_date[0] != -1)
            t_pFile->write_int(FIFF_MEAS_DATE,info->meas_date, 2);
        //
        //    Channel info
        //
        if (cals)
            delete cals;
        cals = new MatrixXf(1,nchan);

        for(k = 0; k < nchan; ++k)
        {
            //
            //    Scan numbers may have been messed up
            //
            chs[k].scanno = k+1;//+1 because
            chs[k].range  = 1.0f;
            (*cals)(0,k) = chs[k].cal;
            t_pFile->write_ch_info(&chs[k]);
        }
        //
        //
        t_pFile->end_block(FIFFB_MEAS_INFO);
        //
        // Start the raw data
        //
        t_pFile->start_block(FIFFB_RAW_DATA);

        return t_pFile;
    }

    //=========================================================================================================
    /**
    * fiff_write_id
    *
    * ### MNE toolbox root function ###
    *
    * fiff_write_id(fid,kind,id)
    *
    * Writes fiff id
    *
    *     fid           An open fif file descriptor
    *     kind          The tag kind
    *     id            The id to write
    *
    * If the id argument is missing it will be generated here
    *
    */
    static void write_id(FiffFile* p_pFile, fiff_int_t kind, FiffId& id = defaultFiffId)
    {
        p_pFile->write_id(kind, id);
    }


    //=========================================================================================================
    /**
    * fiff_write_int
    *
    * ### MNE toolbox root function ###
    *
    * fiff_write_int(fid,kind,data)
    *
    * Writes a 32-bit integer tag to a fif file
    *
    *     fid           An open fif file descriptor
    *     kind          Tag kind
    *     data          The integers to use as data
    *     nel           Zahl an Elementen in der data size
    */
    static void write_int(FiffFile* p_pFile, fiff_int_t kind, fiff_int_t* data, fiff_int_t nel = 1)
    {
        p_pFile->write_int(kind, data, nel);
    }


};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE

#endif // FIFF_H
