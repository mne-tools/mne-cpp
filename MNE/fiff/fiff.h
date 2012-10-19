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
*           with mne matlab toolbox - Note: avoid using the wrappers, prefer the wrapped methods! Its
*           sufficient to include this header to have access to all Fiff classes.
*
*/

#ifndef FIFF_H
#define FIFF_H


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_constants.h"
#include "fiff_coord_trans.h"
#include "fiff_dir_tree.h"
#include "fiff_dir_entry.h"
#include "fiff_named_matrix.h"
#include "fiff_tag.h"
#include "fiff_types.h"
#include "fiff_proj.h"
#include "fiff_ctf_comp.h"
#include "fiff_info.h"
#include "fiff_raw_data.h"
#include "fiff_raw_dir.h"
#include "fiff_file.h"
#include "fiff_evoked_data_set.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include "../3rdParty/Eigen/Core"


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

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//=============================================================================================================
/**
* DECLARE CLASS Fiff
*
* @brief The Fiff class provides static wrapper functions to stay consistent with mne matlab toolbox
*        Note: avoid using the wrappers, prefer the wrapped methods!
*/
class FIFFSHARED_EXPORT Fiff
{
public:
    //=========================================================================================================
    /**
    * dtor
    */
    virtual ~Fiff(){ }

    //Alphabetic ordered MNE Toolbox fiff_function
    //=========================================================================================================
    /**
    * fiff_copy_tree
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the static FiffDirTree::copy_tree function
    *
    * Copies directory subtrees from fidin to fidout
    *
    * @param[in] fidin fiff file to copy from
    * @param[in] in_id file id description
    * @param[out] nodes subtree directories to be copied
    * @param[in] fidout fiff file to write to
    *
    * @return true if succeeded, false otherwise
    */
    static inline bool copy_tree(FiffFile* fidin, FiffId& in_id, QList<FiffDirTree*>& nodes, FiffFile* fidout)
    {
        return FiffDirTree::copy_tree(fidin, in_id, nodes, fidout);
    }

    //=========================================================================================================
    /**
    * fiff_end_block
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile end_block member function
    *
    * Writes a FIFF_BLOCK_END tag
    *
    * @param[in] p_pFile the opened fiff file
    * @param[in] kind The block kind to end
    */
    void end_block(FiffFile* p_pFile, fiff_int_t kind)
    {
        p_pFile->end_block(kind);
    }

    //=========================================================================================================
    /**
    * fiff_end_file
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile end_file member function
    *
    * Writes the closing tags to a fif file and closes the file
    *
    * @param[in] p_pFile the opened fiff file
    */
    void end_file(FiffFile* p_pFile)
    {
        p_pFile->end_file();
    }

    //=========================================================================================================
    /**
    * fiff_finish_writing_raw
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile finish_writing_raw member function
    *
    * Finishes a raw file by writing all necessary end tags.
    *
    * @param[in] p_pFile the opened fiff file
    */
    void finish_writing_raw(FiffFile* p_pFile)
    {
        p_pFile->finish_writing_raw();
    }

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
    * Wrapper for the static FiffCoordTrans::invert_transform function
    *
    * Invert a coordinate transformation
    *
    * @param[in] p_pTransform the transformation which should be inverted
    *
    * @return true if succeeded, false otherwise
    */
    static inline bool invert_transform(FiffCoordTrans* p_pTransform)
    {
        return p_pTransform->invert_transform();
    }

    //=========================================================================================================
    /**
    * fiff_make_dir_tree
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffCoordTrans::make_dir_tree static function
    *
    * @param[in] p_pFile the opened fiff file
    * @param[in] p_pDir the dir entries of which the tree should be constructed
    * @param[out] p_pTree the created dir tree
    * @param[in] start dir entry to start (optional, by default 0)
    *
    * @return index of the last read dir entry
    */
    static inline qint32 make_dir_tree(FiffFile* p_pFile, QList<FiffDirEntry>* p_pDir, FiffDirTree*& p_pTree, qint32 start = 0)
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
    static bool open(QString& p_sFileName, FiffFile*& p_pFile, FiffDirTree*& p_pTree, QList<FiffDirEntry>*& p_pDir)
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
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffInfo::pick_channels static function
    *
    * Make a selector to pick desired channels from data
    *
    * @param[in] ch_names  - The channel name list to consult
    * @param[in] include   - Channels to include (if empty, include all available)
    * @param[in] exclude   - Channels to exclude (if empty, do not exclude any)
    * @return the selector matrix (row Vector)
    */
    inline static MatrixXi pick_channels(QStringList& ch_names, QStringList& include = defaultQStringList, QStringList& exclude = defaultQStringList)
    {
        return FiffInfo::pick_channels(ch_names, include, exclude);
    }








//    function [res] = fiff_pick_channels_evoked(orig,include,exclude)
//    %
//    % [res] = fiff_pick_channels_evoked(orig,include,exclude)
//    %
//    % Pick desired channels from evoked-response data
//    %
//    % orig      - The original data
//    % include   - Channels to include (if empty, include all available)
//    % exclude   - Channels to exclude (if empty, do not exclude any)
//    %
//    %

    inline static FiffEvokedDataSet* pick_channels_evoked(const FiffEvokedDataSet* orig, QStringList& include = defaultQStringList, QStringList& exclude = defaultQStringList)
    {
        if(include.size() == 0 && exclude.size() == 0)
            return new FiffEvokedDataSet(orig);

        MatrixXi sel = FiffInfo::pick_channels(orig->info->ch_names, include, exclude);
        if (sel.cols() == 0)
        {
            printf("Warning : No channels match the selection.\n");
            return new FiffEvokedDataSet(orig);
        }

        FiffEvokedDataSet* res = new FiffEvokedDataSet(orig);
        //
        //   Modify the measurement info
        //
        FiffInfo* info = pick_info(res->info,&sel);
        //
        //   Create the reduced data set
        //
//        for (qint32 k = 0; k < res->evoked.size(); ++k)
//            res.evoked(k).epochs = res.evoked(k).epochs(sel,:);
//        end

//        return;

//        end





        return NULL;
    }





















//    function [res] = fiff_pick_info(info,sel)
//    %
//    % [res] = fiff_pick_info(info,sel)
//    %
//    % Pick desired channels from measurement info
//    %
//    % res       - Info modified according to sel
//    % info      - The original data
//    % sel       - List of channels to select
//    %
    static FiffInfo* pick_info(const FiffInfo* info, const MatrixXi* sel = NULL)
    {
        FiffInfo* res = new FiffInfo(info);
        if (sel == NULL)
            return res;

        //ToDo when pointer List do delation
        res->chs.clear();
        res->ch_names.clear();

        qint32 idx;
        for(qint32 i = 0; i < sel->cols(); ++i)
        {
            idx = (*sel)(0,i);
            res->chs.append(info->chs[idx]);
            res->ch_names.append(info->ch_names[idx]);
        }
        res->nchan  = sel->cols();

        return NULL;
    }










    //=========================================================================================================
    /**
    * fiff_pick_types
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffInfo pick_types member function
    *
    * Create a selector to pick desired channel types from data
    *
    * @param[in] info       The measurement info
    * @param[in] meg        Include MEG channels
    * @param[in] eeg        Include EEG channels
    * @param[in] stim       Include stimulus channels
    * @param[in] include    Additional channels to include (if empty, do not add any)
    * @param[in] exclude    Channels to exclude (if empty, do not exclude any)
    *
    * @return the selector matrix (row vector)
    */
    inline static MatrixXi pick_types(FiffInfo* info, bool meg, bool eeg = false, bool stim = false, QStringList& include = defaultQStringList, QStringList& exclude = defaultQStringList)
    {
        return info->pick_types(meg, eeg, stim, include, exclude);
    }

    //=========================================================================================================
    /**
    * fiff_read_bad_channels
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile read_bad_channels member function
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
        return p_pFile->read_bad_channels(p_pTree);
    }

    //=========================================================================================================
    /**
    * fiff_read_ctf_comp
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile read_ctf_comp member function
    *
    * Read the CTF software compensation data from the given node
    *
    * @param[in] p_pFile    The opened fif file to read from
    * @param[in] p_pTree    The node of interest
    * @param[in] chs        channels with the calibration info
    *
    * @return the CTF software compensation data
    */
    static inline QList<FiffCtfComp*> read_ctf_comp(FiffFile* p_pFile, FiffDirTree* p_pTree, QList<FiffChInfo>& chs)
    {
        return p_pFile->read_ctf_comp(p_pTree, chs);
    }


//    function [data] = fiff_read_evoked(fname,setno)
//    %
//    % [data] = fiff_read_evoked(fname,setno)
//    %
//    % Read one evoked data set
//    %
    static inline bool read_evoked(QString& p_sFileName, FiffEvokedDataSet*& data, fiff_int_t setno = 0)
    {
        if(data)
            delete data;
        data = NULL;

        if (setno < 0)
        {
            printf("Data set selector must be positive\n");
            return false;
        }
        //
        //   Open the file
        //
        printf("Reading %s ...\n",p_sFileName.toUtf8().constData());
        FiffFile* t_pFile = new FiffFile(p_sFileName);
        FiffDirTree* t_pTree = NULL;
        QList<FiffDirEntry>* t_pDir = NULL;

        if(!t_pFile->open(t_pTree, t_pDir))
        {
            if(t_pFile)
                delete t_pFile;
            if(t_pTree)
                delete t_pTree;
            if(t_pDir)
                delete t_pDir;

            return false;
        }
        //
        //   Read the measurement info
        //
        FiffInfo* info = NULL;
        FiffDirTree* meas = t_pFile->read_meas_info(t_pTree, info);
        info->filename = p_sFileName; //move fname storage to read_meas_info member function
        //
        //   Locate the data of interest
        //
        QList<FiffDirTree*> processed = meas->dir_tree_find(FIFFB_PROCESSED_DATA);
        if (processed.size() == 0)
        {
            if(t_pFile)
                delete t_pFile;
            if(t_pTree)
                delete t_pTree;
            if(t_pDir)
                delete t_pDir;
            if(info)
                delete info;

            printf("Could not find processed data\n");
            return false;
        }
        //
        QList<FiffDirTree*> evoked = meas->dir_tree_find(FIFFB_EVOKED);
        if (evoked.size() == 0)
        {
            if(t_pFile)
                delete t_pFile;
            if(t_pTree)
                delete t_pTree;
            if(t_pDir)
                delete t_pDir;
            if(info)
                delete info;
            printf("Could not find evoked data");
            return false;
        }
        //
        //   Identify the aspects
        //
        fiff_int_t naspect = 0;
        fiff_int_t nsaspects = 0;
        qint32 oldsize = 0;
        MatrixXi is_smsh(1,0);
        QList< QList<FiffDirTree*> > sets_aspects;
        QList< qint32 > sets_naspect;
        QList<FiffDirTree*> saspects;
        qint32 k;
        for (k = 0; k < evoked.size(); ++k)
        {
//            sets(k).aspects = fiff_dir_tree_find(evoked(k),FIFF.FIFFB_ASPECT);
//            sets(k).naspect = length(sets(k).aspects);

            sets_aspects.append(evoked[k]->dir_tree_find(FIFFB_ASPECT));
            sets_naspect.append(sets_aspects[k].size());

            if (sets_naspect[k] > 0)
            {
                oldsize = is_smsh.cols();
                is_smsh.conservativeResize(1, oldsize + sets_naspect[k]);
                is_smsh.block(0, oldsize, 1, sets_naspect[k]) = MatrixXi::Zero(1, sets_naspect[k]);
                naspect += sets_naspect[k];
            }
            saspects  = evoked[k]->dir_tree_find(FIFFB_SMSH_ASPECT);
            nsaspects = saspects.size();
            if (nsaspects > 0)
            {
                sets_naspect[k] += nsaspects;
                sets_aspects[k].append(saspects);

                oldsize = is_smsh.cols();
                is_smsh.conservativeResize(1, oldsize + sets_naspect[k]);
                is_smsh.block(0, oldsize, 1, sets_naspect[k]) = MatrixXi::Ones(1, sets_naspect[k]);
                naspect += nsaspects;
            }
        }
        printf("\t%d evoked data sets containing a total of %d data aspects in %s\n",evoked.size(),naspect,p_sFileName.toUtf8().constData());
        if (setno >= naspect || setno < 0)
        {
            if(t_pFile)
                delete t_pFile;
            if(t_pTree)
                delete t_pTree;
            if(t_pDir)
                delete t_pDir;
            if(info)
                delete info;
            printf("Data set selector out of range\n");
            return false;
        }
        //
        //   Next locate the evoked data set
        //
        qint32 p = 0;
        qint32 a = 0;
        bool goon = true;
        FiffDirTree* my_evoked = NULL;
        FiffDirTree* my_aspect = NULL;
        for(k = 0; k < evoked.size(); ++k)
        {
            for (a = 0; a < sets_naspect[k]; ++a)
            {
                if(p == setno)
                {
                    my_evoked = evoked[k];
                    my_aspect = sets_aspects[k][a];
                    goon = false;
                    break;
                }
                ++p;
            }
            if (!goon)
                break;
        }
        //
        //   The desired data should have been found but better to check
        //
        if (my_evoked == NULL || my_aspect == NULL)
        {
            if(t_pFile)
                delete t_pFile;
            if(t_pTree)
                delete t_pTree;
            if(t_pDir)
                delete t_pDir;
            if(info)
                delete info;
            printf("Desired data set not found\n");
            return false;
        }
        //
        //   Now find the data in the evoked block
        //
        fiff_int_t nchan = 0;
        float sfreq = -1.0f;
        QList<FiffChInfo> chs;
        fiff_int_t kind, pos, first, last;
        FiffTag* t_pTag = NULL;
        QString comment("");
        for (k = 0; k < my_evoked->nent; ++k)
        {
            kind = my_evoked->dir[k].kind;
            pos  = my_evoked->dir[k].pos;
            switch (kind)
            {
                case FIFF_COMMENT:
                    FiffTag::read_tag(t_pFile,t_pTag,pos);
                    comment = t_pTag->toString();
                    break;
                case FIFF_FIRST_SAMPLE:
                    FiffTag::read_tag(t_pFile,t_pTag,pos);
                    first = *t_pTag->toInt();
                    break;
                case FIFF_LAST_SAMPLE:
                    FiffTag::read_tag(t_pFile,t_pTag,pos);
                    last = *t_pTag->toInt();
                    break;
                case FIFF_NCHAN:
                    FiffTag::read_tag(t_pFile,t_pTag,pos);
                    nchan = *t_pTag->toInt();
                    break;
                case FIFF_SFREQ:
                    FiffTag::read_tag(t_pFile,t_pTag,pos);
                    sfreq = *t_pTag->toFloat();
                    break;
                case FIFF_CH_INFO:
                    FiffTag::read_tag(t_pFile, t_pTag, pos);
                    chs.append( t_pTag->toChInfo() );
                    break;
            }
        }
        if (comment.isEmpty())
            comment = QString("No comment");
        //
        //   Local channel information?
        //
        if (nchan > 0)
        {
            if (chs.size() == 0)
            {
                if(t_pFile)
                    delete t_pFile;
                if(t_pTree)
                    delete t_pTree;
                if(t_pDir)
                    delete t_pDir;
                if(info)
                    delete info;
                printf("Local channel information was not found when it was expected.\n");
                return false;
            }
            if (chs.size() != nchan)
            {
                if(t_pFile)
                    delete t_pFile;
                if(t_pTree)
                    delete t_pTree;
                if(t_pDir)
                    delete t_pDir;
                if(info)
                    delete info;
                printf("Number of channels and number of channel definitions are different\n");
                return false;
            }
            info->chs   = chs;
            info->nchan = nchan;
            printf("\tFound channel information in evoked data. nchan = %d\n",nchan);
            if (sfreq > 0.0f)
                info->sfreq = sfreq;
        }
        qint32 nsamp = last-first+1;
        printf("\tFound the data of interest:\n");
        printf("\t\tt = %10.2f ... %10.2f ms (%s)\n", 1000*(float)first/info->sfreq, 1000*(float)last/info->sfreq,comment.toUtf8().constData());
        if (info->comps.size() > 0)
            printf("\t\t%d CTF compensation matrices available\n", info->comps.size());
        //
        // Read the data in the aspect block
        //
        fiff_int_t nepoch = 0;
        fiff_int_t aspect_kind = -1;
        fiff_int_t nave = -1;
        QList<FiffTag*> epoch;
        for (k = 0; k < my_aspect->nent; ++k)
        {
            kind = my_aspect->dir[k].kind;
            pos  = my_aspect->dir[k].pos;

            switch (kind)
            {
                case FIFF_COMMENT:
                    FiffTag::read_tag(t_pFile, t_pTag, pos);
                    comment = t_pTag->toString();
                    break;
                case FIFF_ASPECT_KIND:
                    FiffTag::read_tag(t_pFile, t_pTag, pos);
                    aspect_kind = *t_pTag->toInt();
                    break;
                case FIFF_NAVE:
                    FiffTag::read_tag(t_pFile, t_pTag, pos);
                    nave = *t_pTag->toInt();
                    break;
                case FIFF_EPOCH:
                    FiffTag::read_tag(t_pFile, t_pTag, pos);
                    epoch.append(new FiffTag(t_pTag));
                    ++nepoch;
                    break;
            }
        }
        if (nave == -1)
            nave = 1;
        printf("\t\tnave = %d aspect type = %d\n", nave, aspect_kind);
        if (nepoch != 1 && nepoch != info->nchan)
        {
            if(t_pFile)
                delete t_pFile;
            if(t_pTree)
                delete t_pTree;
            if(t_pDir)
                delete t_pDir;
            if(info)
                delete info;
            printf("Number of epoch tags is unreasonable (nepoch = %d nchan = %d)\n", nepoch, info->nchan);
            return false;
        }
        //
        MatrixXf* all = NULL;
        if (nepoch == 1)
        {
            //
            //   Only one epoch
            //
            all = epoch[0]->toFloatMatrix();
            all->transposeInPlace();
            //
            //   May need a transpose if the number of channels is one
            //
            if (all->cols() == 1 && info->nchan == 1)
                all->transposeInPlace();
        }
        else
        {
            //
            //   Put the old style epochs together
            //
            all = epoch[0]->toFloatMatrix();
            all->transposeInPlace();

            for (k = 2; k < nepoch; ++k)
            {
                oldsize = all->rows();
                MatrixXf* tmp = epoch[k]->toFloatMatrix();
                tmp->transposeInPlace();
                all->conservativeResize(oldsize+tmp->rows(), all->cols());
                all->block(oldsize, 0, tmp->rows(), tmp->cols()) = *tmp;
                delete tmp;
            }
        }
        if (all->cols() != nsamp)
        {
            if(t_pFile)
                delete t_pFile;
            if(t_pTree)
                delete t_pTree;
            if(t_pDir)
                delete t_pDir;
            if(info)
                delete info;
            printf("Incorrect number of samples (%d instead of %d)", all->cols(), nsamp);
            return false;
        }
        //
        //   Calibrate
        //
        SparseMatrix<float> cals(info->nchan, info->nchan);
        for(k = 0; k < info->nchan; ++k)
            cals.insert(k, k) = info->chs[k].cal;
        *all = cals* *all;
        //
        //   Put it all together
        //
        data = new FiffEvokedDataSet();
        data->info = info;

//        if(data->evoked)
//            delete data->evoked;
        data->evoked.append(new FiffEvokedData());
        data->evoked[0]->aspect_kind = aspect_kind;
        data->evoked[0]->is_smsh     = is_smsh(0,setno);
        if (nave != -1)
            data->evoked[0]->nave = nave;
        else
            data->evoked[0]->nave  = 1;

        data->evoked[0]->first = first;
        data->evoked[0]->last  = last;
        if (!comment.isEmpty())
            data->evoked[0]->comment = comment;
        //
        //   Times for convenience and the actual epoch data
        //

        if(data->evoked[0]->times)
            delete data->evoked[0]->times;
        data->evoked[0]->times = new MatrixXf(1, last-first+1);

        for (k = 0; k < data->evoked[0]->times->cols(); ++k)
            (*data->evoked[0]->times)(0, k) = ((float)(first+k)) / info->sfreq;

        if(data->evoked[0]->epochs)
            delete data->evoked[0]->epochs;
        data->evoked[0]->epochs = all;

        if(t_pFile)
            delete t_pFile;
        if(t_pTree)
            delete t_pTree;
        if(t_pDir)
            delete t_pDir;

        return true;
    }

    //=========================================================================================================
    /**
    * fiff_read_meas_info
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile read_meas_info member function
    *
    * Read the measurement info
    * Source is assumed to be an open fiff file.
    *
    * @param[in] p_pFile The opened fif file to read from
    * @param[in] p_pTree The node of interest
    * @param[out] info the read measurement info
    *
    * @return the to measurement corresponding fiff_dir_tree.
    */
    static inline FiffDirTree* read_meas_info(FiffFile* p_pFile, FiffDirTree* p_pTree, FiffInfo*& info)
    {
        return p_pFile->read_meas_info(p_pTree, info);
    }

    //=========================================================================================================
    /**
    * fiff_read_named_matrix
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile read_named_matrix member function
    *
    * Reads a named matrix.
    *
    * @param[in] p_pFile    The opened fif file to read from
    * @param[in] node       The node of interest
    * @param[in] matkind    The matrix kind to look for
    * @param[out] mat       The named matrix
    *
    * @return true if succeeded, false otherwise
    */
    static inline bool read_named_matrix(FiffFile* p_pFile, FiffDirTree* node, fiff_int_t matkind, FiffNamedMatrix*& mat)
    {
        return p_pFile->read_named_matrix(node, matkind, mat);
    }

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###
    *
    * [ projdata ] = fiff_read_proj(fid,node)
    *
    * Read the SSP data under a given directory node
    *
    * Wrapper for the FiffFile read_proj member function
    *
    * @param[in] p_pFile    The opened fif file to read from
    * @param[in] node       The node of interest
    *
    * @return a list of SSP projectors
    */
    static inline QList<FiffProj*> read_proj(FiffFile* p_pFile, FiffDirTree* p_pNode)
    {
        return p_pFile->read_proj(p_pNode);
    }

    //=========================================================================================================
    /**
    * fiff_read_raw_segment
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffRawData read_raw_segment member function
    *
    * Read a specific raw data segment
    *
    * @param[in] raw        structure returned by fiff_setup_read_raw
    * @param[out] data      returns the data matrix (channels x samples)
    * @param[out] times     returns the time values corresponding to the samples
    * @param[in] from       first sample to include. If omitted, defaults to the first sample in data (optional)
    * @param[in] to         last sample to include. If omitted, defaults to the last sample in data (optional)
    * @param[in] sel        channel selection vector (optional)
    *
    * @return true if succeeded, false otherwise
    */
    inline static bool read_raw_segment(FiffRawData* raw, MatrixXf*& data, MatrixXf*& times, fiff_int_t from = -1, fiff_int_t to = -1, MatrixXi sel = defaultMatrixXi)
    {
        return raw->read_raw_segment(data, times, from, to, sel);
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
    inline static bool read_tag(FiffFile* p_pFile, FiffTag*& p_pTag, qint64 pos = -1)
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
    * Wrapper for the FiffFile::setup_read_raw static function
    *
    * Read information about raw data file
    *
    * @param[in] t_sFileName        Name of the file to read
    * @param[out] data              The raw data information - contains the opened fiff file
    * @param[in] allow_maxshield    Accept unprocessed MaxShield data
    *
    * @return true if succeeded, false otherwise
    */
    inline static bool setup_read_raw(QString& p_sFileName, FiffRawData*& data, bool allow_maxshield = false)
    {
        return FiffFile::setup_read_raw(p_sFileName, data, allow_maxshield);
    }

    //=========================================================================================================
    /**
    * fiff_split_name_list
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile::split_name_list static function
    *
    * Splits a string by looking for seperator ":"
    *
    * @param[in] p_sNameList    string to split
    *
    * @return the splitted string list
    */
    inline static QStringList split_name_list(QString p_sNameList)
    {
        return FiffFile::split_name_list(p_sNameList);
    }

    //=========================================================================================================
    /**
    * fiff_start_block
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile start_block member function
    *
    * Writes a FIFF_BLOCK_START tag
    *
    * @param[in] p_pFile    An open fif file to write to
    * @param[in] kind       The block kind to start
    */
    inline static void start_block(FiffFile* p_pFile, fiff_int_t kind)
    {
        p_pFile->start_block(kind);
    }

    //=========================================================================================================
    /**
    * fiff_start_file
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile::start_file static function
    *
    * Opens a fiff file for writing and writes the compulsory header tags
    *
    * @param[in] name   The name of the file to open. It is recommended that the name ends with .fif
    *
    * @return The opened file.
    */
    inline static FiffFile* start_file(QString& p_sFileName)
    {
        return FiffFile::start_file(p_sFileName);
    }

    //=========================================================================================================
    /**
    * fiff_start_writing_raw
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile::start_writing_raw static function
    *
    * function [fid,cals] = fiff_start_writing_raw(name,info,sel)
    *
    * @param[in] p_sFileName    filename
    * @param[in] info           The measurement info block of the source file
    * @param[out] cals          Thecalibration matrix
    * @param[in] sel            Which channels will be included in the output file (optional)
    *
    * @return the started fiff file
    */
    inline static FiffFile* start_writing_raw(QString& p_sFileName, FiffInfo* info, MatrixXf*& cals, MatrixXi sel = defaultFileMatrixXi)
    {
        return FiffFile::start_writing_raw(p_sFileName, info, cals, sel);
    }

    //=========================================================================================================
    /**
    * fiff_write_ch_info
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile write_ch_info member function
    *
    * Writes a channel information record to a fif file
    * The type, cal, unit, and pos members are explained in Table 9.5
    * of the MNE manual
    *
    * @param[in] p_pFile    An open fif file
    * @param[in] ch         The channel information structure to write
    */
    inline static void write_ch_info(FiffFile* p_pFile, FiffChInfo* ch)
    {
        p_pFile->write_ch_info(ch);
    }

    //=========================================================================================================
    /**
    * fiff_write_coord_trans
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile write_coord_trans member function
    *
    * Writes a coordinate transformation structure
    *
    * @param[in] p_pFile    An open fif file
    * @param[in] trans      The coordinate transfomation structure
    */
    inline static void write_coord_trans(FiffFile* p_pFile, FiffCoordTrans* trans)
    {
        p_pFile->write_coord_trans(trans);
    }

    //=========================================================================================================
    /**
    * fiff_write_ctf_comp
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile write_ctf_comp member function
    *
    * Writes the CTF compensation data into a fif file
    *
    * @param[in] p_pFile    An open fif file
    * @param[in] comps      The compensation data to write
    */
    inline static void write_ctf_comp(FiffFile* p_pFile, QList<FiffCtfComp*>& comps)
    {
        p_pFile->write_ctf_comp(comps);
    }

    //=========================================================================================================
    /**
    * fiff_write_dig_point
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile write_dig_point member function
    *
    * Writes a digitizer data point into a fif file
    *
    * @param[in] p_pFile    An open fif file
    * @param[in] dig        The point to write
    */
    inline static void write_dig_point(FiffFile* p_pFile, FiffDigPoint& dig)
    {
        p_pFile->write_dig_point(dig);
    }

    //=========================================================================================================
    /**
    * fiff_write_id
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile write_id member function
    *
    * Writes fiff id
    * If the id argument is missing it will be generated here
    *
    * @param[in] p_pFile    An open fif file
    * @param[in] kind       The tag kind
    * @param[in] id         The id to write
    */
    inline static void write_id(FiffFile* p_pFile, fiff_int_t kind, FiffId& id = defaultFiffId)
    {
        p_pFile->write_id(kind, id);
    }

    //=========================================================================================================
    /**
    * fiff_write_int
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile write_int member function
    *
    * Writes a 32-bit integer tag to a fif file
    *
    * @param[in] p_pFile    An open fif file
    * @param[in] kind       Tag kind
    * @param[in] data       The integer data pointer
    * @param[in] nel        Number of integers to write (default = 1)
    */
    inline static void write_int(FiffFile* p_pFile, fiff_int_t kind, fiff_int_t* data, fiff_int_t nel = 1)
    {
        p_pFile->write_int(kind, data, nel);
    }

    //=========================================================================================================
    /**
    * fiff_write_float
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile write_float member function
    *
    * Writes a single-precision floating point tag to a fif file
    *
    * @param[in] p_pFile    An open fif file
    * @param[in] kind       Tag kind
    * @param[in] data       The float data pointer
    * @param[in] nel        Number of floats to write (default = 1)
    */
    inline static void write_float(FiffFile* p_pFile, fiff_int_t kind, float* data, fiff_int_t nel = 1)
    {
        p_pFile->write_float(kind, data, nel);
    }

    //=========================================================================================================
    /**
    * fiff_write_float_matrix
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile write_float_matrix member function
    *
    * Writes a single-precision floating-point matrix tag
    *
    * @param[in] p_pFile    An open fif file
    * @param[in] kind       The tag kind
    * @param[in] mat        The data matrix
    */
    inline static void write_float_matrix(FiffFile* p_pFile, fiff_int_t kind, MatrixXf* mat)
    {
        p_pFile->write_float_matrix(kind, mat);
    }

    //=========================================================================================================
    /**
    * fiff_write_name_list
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile write_name_list member function
    *
    * Writes a colon-separated list of names
    *
    * @param[in] p_pFile    An open fif file
    * @param[in] kind       The tag kind
    * @param[in] data       An array of names to create the list from
    */
    inline static void write_name_list(FiffFile* p_pFile, fiff_int_t kind, QStringList& data)
    {
        p_pFile->write_name_list(kind, data);
    }

    //=========================================================================================================
    /**
    * fiff_write_named_matrix
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile write_named_matrix member function
    *
    * Writes a named single-precision floating-point matrix
    *
    * @param[in] p_pFile    An open fif file
    * @param[in] kind       The tag kind to use for the data
    * @param[in] data       The data matrix
    */
    inline static void write_named_matrix(FiffFile* p_pFile, fiff_int_t kind,FiffNamedMatrix* mat)
    {
        p_pFile->write_named_matrix(kind, mat);
    }

    //=========================================================================================================
    /**
    * fiff_write_proj
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile write_proj member function
    *
    * Writes the projection data into a fif file
    *
    * @param[in] p_pFile    An open fif file
    * @param[in] projs      The compensation data to write
    */
    inline static void write_proj(FiffFile* p_pFile, QList<FiffProj*>& projs)
    {
        p_pFile->write_proj(projs);
    }

    //=========================================================================================================
    /**
    * fiff_write_raw_buffer
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile write_raw_buffer member function
    *
    * Writes a raw buffer.
    *
    * @param[in] p_pFile    An open fif file
    * @param[in] buf        the buffer to write
    * @param[in] cals       calibration factors
    *
    * @return true if succeeded, false otherwise
    */
    inline static bool write_raw_buffer(FiffFile* p_pFile, MatrixXf* buf, MatrixXf* cals)
    {
        return p_pFile->write_raw_buffer(buf, cals);
    }

    //=========================================================================================================
    /**
    * fiff_write_string
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffFile write_string member function
    *
    * Writes a string tag
    *
    * @param[in] p_pFile    An open fif file
    * @param[in] kind       The tag kind
    * @param[in] data       The string data to write
    */
    inline static void write_string(FiffFile* p_pFile, fiff_int_t kind, QString& data)
    {
        p_pFile->write_string(kind, data);
    }

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE

#endif // FIFF_H
