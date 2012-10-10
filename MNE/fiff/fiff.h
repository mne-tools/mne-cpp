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
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "include/fiff_constants.h"
#include "include/fiff_coord_trans.h"
#include "include/fiff_dir_tree.h"
#include "include/fiff_dir_entry.h"
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
        return FiffCoordTrans::invert_transform(p_pTransform);
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
    static inline QList<FiffProj*>read_proj(FiffFile* p_pFile, FiffDirTree* p_pNode)
    {
        return p_pNode->read_proj(p_pFile);
    }

    //=========================================================================================================
    /**
    * fiff_read_raw_segment
    *
    * ### MNE toolbox root function ###: Implementation of the fiff_read_raw_segment function
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
    * [data] = fiff_setup_read_raw(fname,allow_maxshield)
    *
    * Read information about raw data file
    *
    * fname               Name of the file to read
    * allow_maxshield     Accept unprocessed MaxShield data
    */
    inline static bool setup_read_raw(QString t_sFileName, FiffRawData*& data, bool allow_maxshield = false)
    {
        return FiffFile::setup_read_raw(t_sFileName, data, allow_maxshield);
    }

    //=========================================================================================================
    /**
    * fiff_split_name_list
    *
    * ### MNE toolbox root function ###
    *
    * Wrapper for the FiffDirTree::split_name_list static function
    */
    inline static QStringList split_name_list(QString p_sNameList)
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
    static FiffFile* start_writing_raw(QString& p_sFileName, FiffInfo* info, MatrixXf*& cals, MatrixXi sel = defaultFileMatrixXi)
    {
        return FiffFile::start_writing_raw(p_sFileName, info, cals, sel);
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
