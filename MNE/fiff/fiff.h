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
    //=========================================================================================================
    /**
    * fiff_pick_channels_evoked
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffEvokedDataSet pick_channels_evoked member function
    *
    * Pick desired channels from evoked-response data
    *
    * @param[in] orig       The original data
    * @param[in] include   - Channels to include (if empty, include all available)
    * @param[in] exclude   - Channels to exclude (if empty, do not exclude any)
    *
    * @return the desired fiff evoked data set
    */
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
        if (res->info)
            delete res->info;
        res->info = info;
        //
        //   Create the reduced data set
        //
        MatrixXd selBlock(1,1);
        qint32 k, l;
        for(k = 0; k < res->evoked.size(); ++k)
        {
            if(selBlock.rows() != sel.cols() || selBlock.cols() != res->evoked[k]->epochs->cols())
                selBlock.resize(sel.cols(), res->evoked[k]->epochs->cols());
            for(l = 0; l < sel.cols(); ++l)
            {
                selBlock.block(l,0,1,selBlock.cols()) = res->evoked[k]->epochs->block(sel(0,l),0,1,selBlock.cols());
            }
            res->evoked[k]->epochs->resize(sel.cols(), res->evoked[k]->epochs->cols());
            *res->evoked[k]->epochs = selBlock;
        }

        return res;
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

        return res;
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

    //=========================================================================================================
    /**
    * fiff_read_evoked
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffEvokedDataSet::read_evoked static function
    *
    * Read one evoked data set
    *
    * @param[in] p_sFileName    The name of the file to read from
    * @param[out] data          The read evoked data
    * @param[in] setno          the set to pick
    *
    * @return the CTF software compensation data
    */
    static inline bool read_evoked(QString& p_sFileName, FiffEvokedDataSet*& data, fiff_int_t setno = 0)
    {
        return FiffEvokedDataSet::read_evoked(p_sFileName, data, setno);
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
    inline static bool read_raw_segment(FiffRawData* raw, MatrixXd*& data, MatrixXd*& times, fiff_int_t from = -1, fiff_int_t to = -1, MatrixXi sel = defaultMatrixXi)
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
    inline static FiffFile* start_writing_raw(QString& p_sFileName, FiffInfo* info, MatrixXd*& cals, MatrixXi sel = defaultFileMatrixXi)
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
    inline static void write_float_matrix(FiffFile* p_pFile, fiff_int_t kind, MatrixXd* mat)
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
    inline static bool write_raw_buffer(FiffFile* p_pFile, MatrixXd* buf, MatrixXd* cals)
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
