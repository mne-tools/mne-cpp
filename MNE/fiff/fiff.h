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
    static inline qint32 make_dir_tree(QFile* p_pFile, QList<fiff_dir_entry_t>* p_pDir, FiffDirTree*& p_pTree, qint32 start = 0)
    {
        return FiffDirTree::make_dir_tree(p_pFile, p_pDir, p_pTree, start);
    }


    //=========================================================================================================
    /**
    * fiff_open
    *
    * ### MNE toolbox root function ###
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
    static bool open(QString& p_sFileName, QFile*& p_pFile, FiffDirTree*& p_pTree, QList<fiff_dir_entry_t>*& p_pDir);


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
    static QStringList read_bad_channels(QFile* p_pFile, FiffDirTree* p_pTree)
    {
        return p_pTree->read_bad_channels(p_pFile);
    }


    /*
    * [info,meas] = fiff_read_meas_info(source,tree)
    *
    * Read the measurement info
    *
    * If tree is specified, source is assumed to be an open file id,
    * otherwise a the name of the file to read. If tree is missing, the
    * meas output argument should not be specified.
    *
    */
    static bool read_meas_info(QFile* p_pFile, FiffDirTree* p_pTree)
    {
        //
        //   Find the desired blocks
        //

//        meas = fiff_dir_tree_find(tree,FIFF.FIFFB_MEAS);

        QList<FiffDirTree*> meas = p_pTree->dir_tree_find(FIFFB_MEAS);

        if (meas.size() == 0)
        {
            printf("Could not find measurement data\n");
            return false;
        }
        //
        QList<FiffDirTree*> meas_info = meas.at(0)->dir_tree_find(FIFFB_MEAS_INFO);
        if (meas_info.count() == 0)
        {
            printf("Could not find measurement info\n");
            return false;
        }
        //
        //   Read measurement info
        //
        FIFFLIB::FiffTag* t_pTag = NULL;

        fiff_int_t nchan = -1;
        float sfreq = -1.0f;
        QList<FiffChInfo> chs;
        float lowpass = 0.0f;
        float highpass = 0.0f;

        FiffChInfo t_chInfo;

        FiffCoordTrans cand;
        FiffCoordTrans dev_head_t;
        FiffCoordTrans ctf_head_t;

        fiff_int_t meas_date_first = -1;
        fiff_int_t meas_date_second = -1;

        fiff_int_t kind = -1;
        fiff_int_t pos = -1;

        for (qint32 k = 0; k < meas_info.at(0)->nent; ++k)
        {
            kind = meas_info.at(0)->dir.at(k).kind;
            pos  = meas_info.at(0)->dir.at(k).pos;
            switch (kind)
            {
                case FIFF_NCHAN:
                    FiffTag::read_tag(p_pFile, t_pTag, pos);
                    //qDebug() << "FIFF_NCHAN" << t_pTag->getType();
                    nchan = *t_pTag->toInt();
                    break;
                case FIFF_SFREQ:
                    FiffTag::read_tag(p_pFile, t_pTag, pos);
                    //qDebug() << "FIFF_SFREQ" << t_pTag->getType();
                    sfreq = *t_pTag->toFloat();
                    break;
                case FIFF_CH_INFO:
                    FiffTag::read_tag(p_pFile, t_pTag, pos);
                    //qDebug() << "FIFF_CH_INFO" << t_pTag->getType();
                    chs.append( t_pTag->toChInfo() );
                    break;
                case FIFF_LOWPASS:
                    FiffTag::read_tag(p_pFile, t_pTag, pos);
                    //qDebug() << "FIFF_LOWPASS" << t_pTag->getType();
                    lowpass = *t_pTag->toFloat();
                    break;
                case FIFF_HIGHPASS:
                    FiffTag::read_tag(p_pFile, t_pTag, pos);
                    //qDebug() << "FIFF_HIGHPASS" << t_pTag->getType();
                    highpass = *t_pTag->toFloat();
                    break;
                case FIFF_MEAS_DATE:
                    FiffTag::read_tag(p_pFile, t_pTag, pos);
                    //qDebug() << "FIFF_MEAS_DATE" << t_pTag->getType();
                    meas_date_first = t_pTag->toInt()[0];
                    meas_date_second = t_pTag->toInt()[1];
                    break;
                case FIFF_COORD_TRANS:
                    //This has to be debugged!!
                    FiffTag::read_tag(p_pFile, t_pTag, pos);
                    //qDebug() << "FIFF_COORD_TRANS" << t_pTag->getType();
                    cand = t_pTag->toCoordTrans();
                    if(cand.from == FIFFV_COORD_DEVICE && cand.to == FIFFV_COORD_HEAD)
                        dev_head_t = cand;
                    else if (cand.from == FIFFV_MNE_COORD_CTF_HEAD && cand.to == FIFFV_COORD_HEAD)
                        ctf_head_t = cand;
                    break;
            }
        }
        //
        //   Check that we have everything we need
        //
        if (nchan < 0)
        {
            printf("Number of channels in not defined\n");
            return false;
        }
        if (sfreq < 0)
        {
            printf("Sampling frequency is not defined\n");
            return false;
        }
        if (chs.size() == 0)
        {
            printf("Channel information not defined\n");
            return false;
        }
        if (chs.size() != nchan)
        {
            printf("Incorrect number of channel definitions found\n");
            return false;
        }


        if ((dev_head_t.from == -1) || (ctf_head_t.from == -1)) //if isempty(dev_head_t) || isempty(ctf_head_t)
        {
            QList<FiffDirTree*> hpi_result = meas_info.at(0)->dir_tree_find(FIFFB_HPI_RESULT);
            if (hpi_result.size() == 1)
            {
                for( qint32 k = 0; k < hpi_result.at(0)->nent; ++k)
                {
                    kind = hpi_result.at(0)->dir.at(k).kind;
                    pos  = hpi_result.at(0)->dir.at(k).pos;
                    if (kind == FIFF_COORD_TRANS)
                    {
                        FiffTag::read_tag(p_pFile, t_pTag, pos);
                        cand = t_pTag->toCoordTrans();
                        if (cand.from == FIFFV_COORD_DEVICE && cand.to == FIFFV_COORD_HEAD)
                            dev_head_t = cand;
                        else if (cand.from == FIFFV_MNE_COORD_CTF_HEAD && cand.to == FIFFV_COORD_HEAD)
                            ctf_head_t = cand;
                    }
                }
            }
        }
        //
        //   Locate the Polhemus data
        //
        QList<FiffDirTree*> isotrak = meas_info.at(0)->dir_tree_find(FIFFB_ISOTRAK);

        QList<fiff_dig_point_t> dig;// = struct('kind',{},'ident',{},'r',{},'coord_frame',{});
        fiff_int_t coord_frame = FIFFV_COORD_HEAD;
        FiffCoordTrans dig_trans;
        qint32 k = 0;

        if (isotrak.size() == 1)
        {
            for (k = 0; k < isotrak.at(0)->nent; ++k)
            {
                kind = isotrak.at(0)->dir.at(k).kind;
                pos  = isotrak.at(0)->dir.at(k).pos;
                if (kind == FIFF_DIG_POINT)
                {
                    FiffTag::read_tag(p_pFile, t_pTag, pos);
                    dig.append(t_pTag->toDigPoint());
                }
                else
                {
                    if (kind == FIFF_MNE_COORD_FRAME)
                    {
                        FiffTag::read_tag(p_pFile, t_pTag, pos);
                        qDebug() << "NEEDS To BE DEBBUGED: FIFF_MNE_COORD_FRAME" << t_pTag->getType();
                        coord_frame = *t_pTag->toInt();
                    }
                    else if (kind == FIFF_COORD_TRANS)
                    {
                        FiffTag::read_tag(p_pFile, t_pTag, pos);
                        qDebug() << "NEEDS To BE DEBBUGED: FIFF_COORD_TRANS" << t_pTag->getType();
                        dig_trans = t_pTag->toCoordTrans();
                    }
                }
            }
        }
        for(k = 0; k < dig.size(); ++k)
            dig[k].coord_frame = coord_frame;

        if (dig_trans.from != -1) //if exist('dig_trans','var')
        {
            if (dig_trans.from != coord_frame && dig_trans.to != coord_frame)
            {
                FiffCoordTrans tmpEmptyTrans;
                dig_trans = tmpEmptyTrans; //clear('dig_trans');

            }
        }

        //
        //   Locate the acquisition information
        //
        QList<FiffDirTree*> acqpars = meas_info.at(0)->dir_tree_find(FIFFB_DACQ_PARS);
        QString acq_pars;
        QString acq_stim;
        if (acqpars.size() == 1)
        {
            for( k = 0; k < acqpars.at(0)->nent; ++k)
            {
                kind = acqpars.at(0)->dir.at(k).kind;
                pos  = acqpars.at(0)->dir.at(k).pos;
                if (kind == FIFF_DACQ_PARS)
                {
                    FiffTag::read_tag(p_pFile, t_pTag, pos);
                    acq_pars = t_pTag->toString();
                }
                else if (kind == FIFF_DACQ_STIM)
                {
                    FiffTag::read_tag(p_pFile, t_pTag, pos);
                    acq_stim = t_pTag->toString();
                }
            }
        }
        //
        //   Load the SSP data
        //
        QList<FiffProj*> projs = Fiff::read_proj(p_pFile,meas_info.at(0));
//        %
//        %   Load the CTF compensation data
//        %
//        comps = fiff_read_ctf_comp(fid,meas_info,chs);
//        %
//        %   Load the bad channel list
//        %
//        bads = fiff_read_bad_channels(fid,meas_info);
//        %
//        %   Put the data together
//        %
//        if ~isempty(tree.id)
//            info.file_id = tree.id;
//        else
//            info.file_id = [];
//        end
//        %
//        %  Make the most appropriate selection for the measurement id
//        %
//        if isempty(meas_info.parent_id)
//            if isempty(meas_info.id)
//                if isempty(meas.id)
//                    if isempty(meas.parent_id)
//                        info.meas_id = info.file_id;
//                    else
//                        info.meas_id = meas.parent_id;
//                    end
//                else
//                    info.meas_id = meas.id;
//                end
//            else
//                info.meas_id = meas_info.id;
//            end
//        else
//            info.meas_id = meas_info.parent_id;
//        end
//        if isempty(meas_date)
//            info.meas_date = [ info.meas_id.secs info.meas_id.usecs ];
//        else
//            info.meas_date = meas_date;
//        end
//        info.nchan     = nchan;
//        info.sfreq     = sfreq;
//        if exist('highpass','var')
//            info.highpass = highpass;
//        else
//            info.highpass = 0;
//        end
//        if exist('lowpass','var')
//            info.lowpass = lowpass;
//        else
//            info.lowpass = info.sfreq/2.0;
//        end
//        %
//        %   Add the channel information and make a list of channel names
//        %   for convenience
//        %
//        info.chs = chs;
//        for c = 1:info.nchan
//            info.ch_names{c} = info.chs(c).ch_name;
//        end
//        %
//        %  Add the coordinate transformations
//        %
//        info.dev_head_t = dev_head_t;
//        info.ctf_head_t = ctf_head_t;
//        if ~isempty(info.dev_head_t) && ~isempty(info.ctf_head_t)
//            info.dev_ctf_t    = info.dev_head_t;
//            info.dev_ctf_t.to = info.ctf_head_t.from;
//            info.dev_ctf_t.trans = inv(ctf_head_t.trans)*info.dev_ctf_t.trans;
//        else
//            info.dev_ctf_t = [];
//        end
//        %
//        %   All kinds of auxliary stuff
//        %
//        info.dig   = dig;
//        if exist('dig_trans','var')
//            info.dig_trans = dig_trans;
//        end
//        info.bads  = bads;
//        info.projs = projs;
//        info.comps = comps;
//        info.acq_pars = acq_pars;
//        info.acq_stim = acq_stim;

//        if open_here
//            fclose(fid);
//        end


        return true;
    }

    //=========================================================================================================
    /**
    * fiff_read_named_matrix
    *
    * ### MNE toolbox root function ###
    */
    static bool read_named_matrix(QFile* p_pFile, FiffDirTree* node, fiff_int_t matkind, FiffNamedMatrix*& mat);


    //=========================================================================================================
    /**
    *
    * ToDo make this member of FiffDirTree
    * ### MNE toolbox root function ###
    *
    * [ projdata ] = fiff_read_proj(fid,node)
    *
    * Read the SSP data under a given directory node
    *
    */
    static inline QList<FiffProj*> read_proj(QFile* p_pFile, FiffDirTree* p_pListNode)
    {
        QList<FiffProj*> projdata;// = struct('kind',{},'active',{},'desc',{},'data',{});
        //
        //   Locate the projection data
        //
        QList<FiffDirTree*> t_qListNodes = p_pListNode->dir_tree_find(FIFFB_PROJ);
        if ( t_qListNodes.size() == 0 )
            return projdata;


        FIFFLIB::FiffTag* t_pTag = NULL;
        t_qListNodes.at(0)->find_tag(p_pFile, FIFF_NCHAN, t_pTag);
        fiff_int_t global_nchan;
        if (t_pTag)
            global_nchan = *t_pTag->toInt();


        fiff_int_t nchan;
        QList<FiffDirTree*> t_qListItems = t_qListNodes.at(0)->dir_tree_find(FIFFB_PROJ_ITEM);
        for ( qint32 i = 0; i < t_qListItems.size(); ++i)
        {
            //
            //   Find all desired tags in one item
            //
            FiffDirTree* t_pFiffDirTreeItem = t_qListItems[i];
            t_pFiffDirTreeItem->find_tag(p_pFile, FIFF_NCHAN, t_pTag);
            if (t_pTag)
                nchan = *t_pTag->toInt();
            else
                nchan = global_nchan;

            t_pFiffDirTreeItem->find_tag(p_pFile, FIFF_DESCRIPTION, t_pTag);
            QString desc; // maybe, in some cases this has to be a struct.
            if (t_pTag)
            {
                qDebug() << "read_proj: this has to be debugged";
                desc = t_pTag->toString();
            }
            else
            {
                t_pFiffDirTreeItem->find_tag(p_pFile, FIFF_NAME, t_pTag);
                if (t_pTag)
                    desc = t_pTag->toString();
                else
                {
                    printf("Projection item description missing\n");
                    return projdata;
                }
            }
//            t_pFiffDirTreeItem->find_tag(p_pFile, FIFF_PROJ_ITEM_CH_NAME_LIST, t_pTag);
//            QString namelist;
//            if (t_pTag)
//            {
//                namelist = t_pTag->toString();
//            }
//            else
//            {
//                printf("Projection item channel list missing\n");
//                return projdata;
//            }
            t_pFiffDirTreeItem->find_tag(p_pFile, FIFF_PROJ_ITEM_KIND, t_pTag);
            fiff_int_t kind;
            if (t_pTag)
            {
                kind = *t_pTag->toInt();
            }
            else
            {
                printf("Projection item kind missing");
                return projdata;
            }
            t_pFiffDirTreeItem->find_tag(p_pFile, FIFF_PROJ_ITEM_NVEC, t_pTag);
            fiff_int_t nvec;
            if (t_pTag)
            {
                nvec = *t_pTag->toInt();
            }
            else
            {
                printf("Number of projection vectors not specified\n");
                return projdata;
            }
            t_pFiffDirTreeItem->find_tag(p_pFile, FIFF_PROJ_ITEM_CH_NAME_LIST, t_pTag);
            QStringList names;
            if (t_pTag)
            {
                names = split_name_list(t_pTag->toString());
            }
            else
            {
                printf("Projection item channel list missing\n");
                return projdata;
            }
            t_pFiffDirTreeItem->find_tag(p_pFile, FIFF_PROJ_ITEM_VECTORS, t_pTag);
            MatrixXf data;
            if (t_pTag)
            {
                data = t_pTag->toFloatMatrix().transpose();
            }
            else
            {
                printf("Projection item data missing\n");
                return projdata;
            }
            t_pFiffDirTreeItem->find_tag(p_pFile, FIFF_MNE_PROJ_ITEM_ACTIVE, t_pTag);
            bool active;
            if (t_pTag)
                active = *t_pTag->toByte(); //this has to be debugged.
            else
                active = false;

            if (data.cols() != names.size())
            {
                printf("Number of channel names does not match the size of data matrix\n");
                return projdata;
            }



            //
            //   create a named matrix for the data
            //
            QStringList defaultList;
            FiffNamedMatrix* t_fiffNamedMatrix = new FiffNamedMatrix(nvec, nchan, defaultList, names, data);

            FiffProj* one = new FiffProj(kind, active, desc, t_fiffNamedMatrix);
            //
            projdata.append(one);
        }

        if (projdata.size() > 0)
        {
            printf("\tRead a total of %d projection items:\n", projdata.size());
            for(qint32 k = 0; k < projdata.size(); ++k)
            {
                printf("\t\t%s (%d x %d)",projdata.at(k)->desc.toUtf8().constData(), projdata.at(k)->data->nrow, projdata.at(k)->data->ncol);
                if (projdata.at(k)->active)
                    printf(" active\n");
                else
                    printf(" idle\n");
            }
        }


        return projdata;
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
    static inline bool read_tag(QFile* p_pFile, FiffTag*& p_pTag, qint64 pos = -1)
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
    static inline bool read_tag_info(QFile* p_pFile, FiffTag*& p_pTag)
    {
        return FiffTag::read_tag_info(p_pFile, p_pTag);
    }



    /*
    * [data] = fiff_setup_read_raw(fname,allow_maxshield)
    *
    * Read information about raw data file
    *
    * fname               Name of the file to read
    * allow_maxshield     Accept unprocessed MaxShield data
    */

    static bool setup_read_raw(QString t_sFileName, bool allow_maxshield = false)
    {
        //
        //   Open the file
        //
        printf("Opening raw data file %s...\n",t_sFileName.toUtf8().constData());

        QFile* t_pFile = NULL;
        FiffDirTree* t_pTree = NULL;
        QList<fiff_dir_entry_t>* t_pDir = NULL;

        if(!Fiff::open(t_sFileName, t_pFile, t_pTree, t_pDir))
            return false;
        //
        //   Read the measurement info
        //
//        [ info, meas ] = fiff_read_meas_info(fid,tree);

        read_meas_info(t_pFile,t_pTree);


//        //
//        //   Locate the data of interest
//        //
//        raw = fiff_dir_tree_find(meas,FIFF.FIFFB_RAW_DATA);
//        if isempty(raw)
//            raw = fiff_dir_tree_find(meas,FIFF.FIFFB_CONTINUOUS_DATA);
//            if allow_maxshield
//                raw = fiff_dir_tree_find(meas,FIFF.FIFFB_SMSH_RAW_DATA);
//                if isempty(raw)
//                    error(me,'No raw data in %s',fname);
//                end
//            else
//                if isempty(raw)
//                    error(me,'No raw data in %s',fname);
//                end
//            end
//        end
//        %
//        %   Set up the output structure
//        %
//        info.filename   = fname;
//        data.fid        = fid;
//        data.info       = info;
//        data.first_samp = 0;
//        data.last_samp  = 0;
//        %
//        %   Process the directory
//        %
//        dir          = raw.dir;
//        nent         = raw.nent;
//        nchan        = info.nchan;
//        first        = 1;
//        first_samp   = 0;
//        first_skip   = 0;
//        %
//        %  Get first sample tag if it is there
//        %
//        if dir(first).kind == FIFF.FIFF_FIRST_SAMPLE
//            tag = fiff_read_tag(fid,dir(first).pos);
//            first_samp = tag.data;
//            first = first + 1;
//        end
//        %
//        %  Omit initial skip
//        %
//        if dir(first).kind == FIFF.FIFF_DATA_SKIP
//            %
//            %  This first skip can be applied only after we know the buffer size
//            %
//            tag = fiff_read_tag(fid,dir(first).pos);
//            first_skip = tag.data;
//            first = first + 1;
//        end
//        data.first_samp = first_samp;
//        %
//        %   Go through the remaining tags in the directory
//        %
//        rawdir = struct('ent',{},'first',{},'last',{},'nsamp',{});
//        nskip = 0;
//        ndir  = 0;
//        for k = first:nent
//            ent = dir(k);
//            if ent.kind == FIFF.FIFF_DATA_SKIP
//                tag = fiff_read_tag(fid,ent.pos);
//                nskip = tag.data;
//            elseif ent.kind == FIFF.FIFF_DATA_BUFFER
//                %
//                %   Figure out the number of samples in this buffer
//                %
//                switch ent.type
//                    case FIFF.FIFFT_DAU_PACK16
//                        nsamp = ent.size/(2*nchan);
//                    case FIFF.FIFFT_SHORT
//                        nsamp = ent.size/(2*nchan);
//                    case FIFF.FIFFT_FLOAT
//                        nsamp = ent.size/(4*nchan);
//                    case FIFF.FIFFT_INT
//                        nsamp = ent.size/(4*nchan);
//                    otherwise
//                        fclose(fid);
//                        error(me,'Cannot handle data buffers of type %d',ent.type);
//                end
//                %
//                %  Do we have an initial skip pending?
//                %
//                if first_skip > 0
//                    first_samp = first_samp + nsamp*first_skip;
//                    data.first_samp = first_samp;
//                    first_skip = 0;
//                end
//                %
//                %  Do we have a skip pending?
//                %
//                if nskip > 0
//                    ndir        = ndir+1;
//                    rawdir(ndir).ent   = [];
//                    rawdir(ndir).first = first_samp;
//                    rawdir(ndir).last  = first_samp + nskip*nsamp - 1;
//                    rawdir(ndir).nsamp = nskip*nsamp;
//                    first_samp = first_samp + nskip*nsamp;
//                    nskip = 0;
//                end
//                %
//                %  Add a data buffer
//                %
//                ndir               = ndir+1;
//                rawdir(ndir).ent   = ent;
//                rawdir(ndir).first = first_samp;
//                rawdir(ndir).last  = first_samp + nsamp - 1;
//                rawdir(ndir).nsamp = nsamp;
//                first_samp = first_samp + nsamp;
//            end
//        end
//        data.last_samp  = first_samp - 1;
//        %
//        %   Add the calibration factors
//        %
//        cals = zeros(1,data.info.nchan);
//        for k = 1:data.info.nchan
//            cals(k) = data.info.chs(k).range*data.info.chs(k).cal;
//        end
//        %
//        data.cals       = cals;
//        data.rawdir     = rawdir;
//        data.proj       = [];
//        data.comp       = [];
//        %
//        fprintf(1,'\tRange : %d ... %d  =  %9.3f ... %9.3f secs\n',...
//            data.first_samp,data.last_samp,...
//            double(data.first_samp)/data.info.sfreq,...
//            double(data.last_samp)/data.info.sfreq);
//        fprintf(1,'Ready.\n');
//        fclose(data.fid);
//        data.fid = -1;


        return true;
    }


    //=========================================================================================================
    /**
    * fiff_split_name_list
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffDirTree::split_name_list static function
    */
    static QStringList split_name_list(QString p_sNameList)
    {
        return FiffDirTree::split_name_list(p_sNameList);
    }
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE

#endif // FIFF_H
