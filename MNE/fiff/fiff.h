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
        FiffFile* p_pFile = new FiffFile(p_sFileName);
        p_pFile->start_file();
        return p_pFile;
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
    static bool start_writing_raw(QString& p_sFileName, FiffInfo* info, MatrixXi sel = defaultMatrixXi)
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

        FiffFile* fid = Fiff::start_file(p_sFileName);
//        fiff_start_block(fid,FIFF.FIFFB_MEAS);
//        fiff_write_id(fid,FIFF.FIFF_BLOCK_ID);
//        if ~isempty(info.meas_id)
//            fiff_write_id(fid,FIFF.FIFF_PARENT_BLOCK_ID,info.meas_id);
//        end
//        %
//        %
//        %    Measurement info
//        %
//        fiff_start_block(fid,FIFF.FIFFB_MEAS_INFO);
//        %
//        %    Blocks from the original
//        %
//        blocks = [ FIFF.FIFFB_SUBJECT FIFF.FIFFB_HPI_MEAS FIFF.FIFFB_HPI_RESULT FIFF.FIFFB_ISOTRAK FIFF.FIFFB_PROCESSING_HISTORY ];
//        have_hpi_result = false;
//        have_isotrak    = false;
//        if length(blocks) > 0 && isfield(info,'filename') && ~isempty(info.filename)
//            [ fid2, tree ] = fiff_open(info.filename);
//            for k = 1:length(blocks)
//                nodes = fiff_dir_tree_find(tree,blocks(k));
//                fiff_copy_tree(fid2,tree.id,nodes,fid);
//                if blocks(k) == FIFF.FIFFB_HPI_RESULT && length(nodes) > 0
//                    have_hpi_result = true;
//                end
//                if blocks(k) == FIFF.FIFFB_ISOTRAK && length(nodes) > 0
//                    have_isotrak = true;
//                end
//            end
//            fclose(fid2);
//        end
//        %
//        %    megacq parameters
//        %
//        if ~isempty(info.acq_pars) || ~isempty(info.acq_stim)
//            fiff_start_block(fid,FIFF.FIFFB_DACQ_PARS);
//            if ~isempty(info.acq_pars)
//                fiff_write_string(fid,FIFF.FIFF_DACQ_PARS, ...
//                    info.acq_pars);
//            end
//            if ~isempty(info.acq_stim)
//                fiff_write_string(fid,FIFF.FIFF_DACQ_STIM, ...
//                    info.acq_stim);
//            end
//            fiff_end_block(fid,FIFF.FIFFB_DACQ_PARS);
//        end
//        %
//        %    Coordinate transformations if the HPI result block was not there
//        %
//        if ~have_hpi_result
//            if ~isempty(info.dev_head_t)
//                fiff_write_coord_trans(fid,info.dev_head_t);
//            end
//            if ~isempty(info.ctf_head_t)
//                fiff_write_coord_trans(fid,info.ctf_head_t);
//            end
//        end
//        %
//        %    Polhemus data
//        %
//        if ~isempty(info.dig) && ~have_isotrak
//            fiff_start_block(fid,FIFF.FIFFB_ISOTRAK);
//            for k = 1:length(info.dig)
//                fiff_write_dig_point(fid,info.dig(k))
//            end
//            fiff_end_block(fid,FIFF.FIFFB_ISOTRAK);
//        end
//        %
//        %    Projectors
//        %
//        fiff_write_proj(fid,info.projs);
//        %
//        %    CTF compensation info
//        %
//        fiff_write_ctf_comp(fid,info.comps);
//        %
//        %    Bad channels
//        %
//        if length(info.bads) > 0
//            fiff_start_block(fid,FIFF.FIFFB_MNE_BAD_CHANNELS);
//            fiff_write_name_list(fid,FIFF.FIFF_MNE_CH_NAME_LIST,info.bads);
//            fiff_end_block(fid,FIFF.FIFFB_MNE_BAD_CHANNELS);
//        end
//        %
//        %    General
//        %
//        fiff_write_float(fid,FIFF.FIFF_SFREQ,info.sfreq);
//        fiff_write_float(fid,FIFF.FIFF_HIGHPASS,info.highpass);
//        fiff_write_float(fid,FIFF.FIFF_LOWPASS,info.lowpass);
//        fiff_write_int(fid,FIFF.FIFF_NCHAN,nchan);
//        fiff_write_int(fid,FIFF.FIFF_DATA_PACK,data_type);
//        if [ ~isempty(info.meas_date) ]
//            fiff_write_int(fid,FIFF.FIFF_MEAS_DATE,info.meas_date);
//        end
//        %
//        %    Channel info
//        %
//        for k = 1:nchan
//            %
//            %   Scan numbers may have been messed up
//            %
//            chs(k).scanno = k;
//            chs(k).range  = 1.0;
//            cals(k) = chs(k).cal;
//            fiff_write_ch_info(fid,chs(k));
//        end
//        %
//        %
//        fiff_end_block(fid,FIFF.FIFFB_MEAS_INFO);
//        %
//        % Start the raw data
//        %
//        fiff_start_block(fid,FIFF.FIFFB_RAW_DATA);

        return true;
    }




};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE

#endif // FIFF_H
