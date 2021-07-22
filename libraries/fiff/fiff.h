//=============================================================================================================
/**
 * @file     fiff.h
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
 * @brief    FIFF class declaration, which provides static wrapper functions to stay consistent
 *           with mne matlab toolbox - Note: avoid using the wrappers, prefer the wrapped methods! Its
 *           sufficient to include this header to have access to all Fiff classes.
 *
 */

#ifndef FIFF_H
#define FIFF_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_constants.h"
#include "fiff_coord_trans.h"
#include "fiff_dir_node.h"
#include "fiff_dir_entry.h"
#include "fiff_named_matrix.h"
#include "fiff_tag.h"
#include "fiff_types.h"
#include "fiff_proj.h"
#include "fiff_ctf_comp.h"
#include "fiff_info.h"
#include "fiff_raw_data.h"
#include "fiff_raw_dir.h"
#include "fiff_stream.h"
#include "fiff_evoked_set.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QIODevice>
#include <QList>
#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

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
     * Wrapper for the static FiffDirNode::copy_tree function
     *
     * Copies directory subtrees from fidin to fidout
     *
     * @param[in] p_pStreamIn    fiff file to copy from.
     * @param[in] in_id          file id description.
     * @param[out] p_Nodes       subtree directories to be copied.
     * @param[in] p_pStreamOut   fiff file to write to.
     *
     * @return true if succeeded, false otherwise.
     */
    inline static bool copy_tree(FiffStream::SPtr p_pStreamIn, const FiffId& in_id, const QList<FiffDirNode::SPtr>& p_Nodes, FiffStream::SPtr& p_pStreamOut)
    {
        return FiffDirNode::copy_tree(p_pStreamIn, in_id, p_Nodes, p_pStreamOut);
    }

    //=========================================================================================================
    /**
     * fiff_end_block
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream end_block member function
     *
     * Writes a FIFF_BLOCK_END tag
     *
     * @param[in] p_pStream the opened fiff file.
     * @param[in] kind The block kind to end.
     */
    inline static void end_block(FiffStream* p_pStream, fiff_int_t kind)
    {
        p_pStream->end_block(kind);
    }

    //=========================================================================================================
    /**
     * fiff_end_file
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream end_file member function
     *
     * Writes the closing tags to a fif file and closes the file
     *
     * @param[in] p_pStream the opened fiff file.
     */
    inline static void end_file(FiffStream* p_pStream)
    {
        p_pStream->end_file();
    }

    //=========================================================================================================
    /**
     * fiff_finish_writing_raw
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream finish_writing_raw member function
     *
     * Finishes a raw file by writing all necessary end tags.
     *
     * @param[in] p_pStream the opened fiff file.
     */
    inline static void finish_writing_raw(FiffStream* p_pStream)
    {
        p_pStream->finish_writing_raw();
    }

    //=========================================================================================================
    /**
     * fiff_dir_tree_find
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffDirNode dir_tree_find member function
     *
     * Find nodes of the given kind from a directory tree structure
     *
     * @param[in] p_Node     The directory tree structure.
     * @param[in] p_kind     The given kind.
     *
     * @return the found nodes.
     */
    inline static QList<FiffDirNode::SPtr> dir_tree_find(const FiffDirNode::SPtr& p_Node, fiff_int_t p_kind)
    {
        return p_Node->dir_tree_find(p_kind);
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
     * @param[in] p_Transform    The transformation which should be inverted.
     *
     * @return true if succeeded, false otherwise.
     */
    inline static bool invert_transform(FiffCoordTrans& p_Transform)
    {
        return p_Transform.invert_transform();
    }

    //=========================================================================================================
    /**
     * fiff_open
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream open member function
     *
     * Opens a fif file and provides the directory of tags
     *
     * @param[in] p_IODevice    A fiff IO device like a fiff QFile or QTCPSocket.
     * @param[out] p_pStream    file which is openened.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool open(QIODevice& p_IODevice, FiffStream::SPtr& p_pStream)
    {
        p_pStream = FiffStream::SPtr(new FiffStream(&p_IODevice));

        return p_pStream->open();
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
     * @param[in] ch_names  - The channel name list to consult.
     * @param[in] include   - Channels to include (if empty, include all available).
     * @param[in] exclude   - Channels to exclude (if empty, do not exclude any).
     * @return the selector matrix (row Vector).
     */
    inline static Eigen::RowVectorXi pick_channels(QStringList& ch_names,
                                                   const QStringList& include = defaultQStringList,
                                                   const QStringList& exclude = defaultQStringList)
    {
        return FiffInfo::pick_channels(ch_names, include, exclude);
    }

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
     * @param[in] orig       The original data.
     * @param[in] include   - Channels to include (if empty, include all available).
     * @param[in] exclude   - Channels to exclude (if empty, do not exclude any).
     *
     * @return the desired fiff evoked data set.
     */
    inline static FiffEvokedSet pick_channels(FiffEvokedSet& orig,
                                              const QStringList& include = defaultQStringList,
                                              const QStringList& exclude = defaultQStringList)
    {
        return orig.pick_channels(include, exclude);
    }

    //=========================================================================================================
    /**
     * fiff_find_evoked
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffEvokedDataSet find_evoked member function
     *
     * Find evoked data sets
     *
     * @param[out] orig   The read evoked data set.
     *
     * @return true when any set was found, false otherwise.
     */
    inline static bool find_evoked(FiffEvokedSet& orig)
    {
        return orig.find_evoked(orig);
    }

    //=========================================================================================================
    /**
     * fiff_pick_info
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffInfo pick_info member function
     *
     * Pick desired channels from measurement info
     *
     * @param[in] info   The original data.
     * @param[in] sel    List of channels to select.
     *
     * @return Info modified according to sel.
     */
    inline static FiffInfo pick_info(const FiffInfo& info,
                                     const Eigen::RowVectorXi &sel = defaultVectorXi)
    {
        return info.pick_info(sel);
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
     * @param[in] info       The measurement info.
     * @param[in] meg        Include MEG channels.
     * @param[in] eeg        Include EEG channels.
     * @param[in] stim       Include stimulus channels.
     * @param[in] include    Additional channels to include (if empty, do not add any).
     * @param[in] exclude    Channels to exclude (if empty, do not exclude any).
     *
     * @return the selector matrix (row vector).
     */
    inline static Eigen::RowVectorXi pick_types(FiffInfo &info,
                                                bool meg,
                                                bool eeg = false,
                                                bool stim = false,
                                                const QStringList& include = defaultQStringList,
                                                const QStringList& exclude = defaultQStringList)
    {
        return info.pick_types(meg, eeg, stim, include, exclude);
    }

    //=========================================================================================================
    /**
     * fiff_read_bad_channels
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream read_bad_channels member function
     *
     * Reads the bad channel list from a node if it exists
     *
     * @param[in] p_pStream  The opened fif file to read from.
     * @param[in] p_Node    The node of interest.
     *
     * @return the bad channel list.
     */
    static inline QStringList read_bad_channels(FiffStream::SPtr& p_pStream,
                                                FiffDirNode::SPtr& p_Node)
    {
        return p_pStream->read_bad_channels(p_Node);
    }

    //=========================================================================================================
    /**
     * fiff_read_ctf_comp
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream read_ctf_comp member function
     *
     * Read the CTF software compensation data from the given node
     *
     * @param[in] p_pStream    The opened fif file to read from.
     * @param[in] p_Node    The node of interest.
     * @param[in] p_Chs        channels with the calibration info.
     *
     * @return the CTF software compensation data.
     */
    static inline QList<FiffCtfComp> read_ctf_comp(FiffStream::SPtr& p_pStream,
                                                   const FiffDirNode::SPtr& p_Node,
                                                   const QList<FiffChInfo>& p_Chs)
    {
        return p_pStream->read_ctf_comp(p_Node, p_Chs);
    }

    //=========================================================================================================
    /**
     * fiff_read_evoked
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffEvoked::read static function
     *
     * Read one evoked data set
     *
     * @param[in] p_IODevice     A fiff IO device like a fiff QFile or QTCPSocket.
     * @param[out] data          The read evoked data.
     * @param[in] setno          the set to pick.
     * @param[in] baseline       The time interval to apply rescaling / baseline correction. If None do not apply it. If baseline is (a, b).
     *                           the interval is between "a (s)" and "b (s)". If a is None the beginning of the data is used and if b is
     *                           None then b is set to the end of the interval. If baseline is equal ot (None, None) all the time interval is used.
     *                           If None, no correction is applied.
     * @param[in] proj           Apply SSP projection vectors (optional, default = true).
     * @param[in] p_aspect_kind  Either "FIFFV_ASPECT_AVERAGE" or "FIFFV_ASPECT_STD_ERR". The type of data to read. Only used if "setno" is a str.
     *
     * @return true if successful, false otherwise.
     */
    static inline bool read_evoked(QIODevice& p_IODevice,
                                   FiffEvoked& data,
                                   QVariant setno = 0,
                                   QPair<float,float> baseline = defaultFloatPair,
                                   bool proj = true,
                                   fiff_int_t p_aspect_kind = FIFFV_ASPECT_AVERAGE)
    {
        return FiffEvoked::read(p_IODevice, data, setno, baseline, proj, p_aspect_kind);
    }

    //=========================================================================================================
    /**
     * Read all evoked data from one file
     *
     * @param[in] p_IODevice     A fiff IO device like a fiff QFile or QTCPSocket.
     * @param[out] data          The read evoked data.
     *
     * @return true if successful, false otherwise.
     */
    static inline bool read_evoked_set(QIODevice& p_IODevice, FiffEvokedSet& data)
    {
        return FiffEvokedSet::read(p_IODevice, data);
    }

    //=========================================================================================================
    /**
     * fiff_read_meas_info
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream read_meas_info member function
     *
     * Read the measurement info
     * Source is assumed to be an open fiff stream.
     *
     * @param[in] p_pStream      The opened fiff stream to read from.
     * @param[in] p_Node         The node of interest.
     * @param[out] p_Info        The read measurement info.
     * @param[out] p_NodeInfo    The to measurement corresponding fiff_dir_tree.
     *
     * @return true if succeeded, false otherwise.
     */
    static inline bool read_meas_info(FiffStream::SPtr& p_pStream, const FiffDirNode::SPtr& p_Node, FiffInfo& p_Info, FiffDirNode::SPtr& p_NodeInfo)
    {
        return p_pStream->read_meas_info(p_Node, p_Info, p_NodeInfo);
    }

    //=========================================================================================================
    /**
     * fiff_read_named_matrix
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream read_named_matrix member function
     *
     * Reads a named matrix.
     *
     * @param[in] p_pStream      The opened fif file to read from.
     * @param[in] p_Node         The node of interest.
     * @param[in] matkind        The matrix kind to look for.
     * @param[out] mat           The named matrix.
     *
     * @return true if succeeded, false otherwise.
     */
    static inline bool read_named_matrix(FiffStream::SPtr& p_pStream, const FiffDirNode::SPtr& p_Node, fiff_int_t matkind, FiffNamedMatrix& mat)
    {
        return p_pStream->read_named_matrix(p_Node, matkind, mat);
    }

    //=========================================================================================================
    /**
     * ### MNE toolbox root function ###
     *
     * [ projdata ] = fiff_read_proj(fid,node)
     *
     * Read the SSP data under a given directory node
     *
     * Wrapper for the FiffStream read_proj member function
     *
     * @param[in] p_pStream    The opened fif file to read from.
     * @param[in] p_Node       The node of interest.
     *
     * @return a list of SSP projectors.
     */
    static inline QList<FiffProj> read_proj(FiffStream::SPtr& p_pStream, const FiffDirNode::SPtr& p_Node)
    {
        return p_pStream->read_proj(p_Node);
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
     * @param[in] raw        structure returned by fiff_setup_read_raw.
     * @param[out] data      returns the data matrix (channels x samples).
     * @param[out] times     returns the time values corresponding to the samples.
     * @param[in] from       first sample to include. If omitted, defaults to the first sample in data (optional).
     * @param[in] to         last sample to include. If omitted, defaults to the last sample in data (optional).
     * @param[in] sel        channel selection vector (optional).
     *
     * @return true if succeeded, false otherwise.
     */
    inline static bool read_raw_segment(FiffRawData& raw,
                                        Eigen::MatrixXd& data,
                                        Eigen::MatrixXd& times,
                                        fiff_int_t from = -1,
                                        fiff_int_t to = -1,
                                        Eigen::MatrixXi sel = defaultMatrixXi)
    {
        return raw.read_raw_segment(data, times, from, to, sel);
    }

    /**
     * fiff_read_raw_segment
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffRawData read_raw_segment member function
     *
     * Read a specific raw data segment
     *
     * @param[in] raw        structure returned by fiff_setup_read_raw.
     * @param[out] data      returns the data matrix (channels x samples).
     * @param[out] times     returns the time values corresponding to the samples.
     * @param[out] mult      matrix to store used multiplication matrix (compensator,projection,calibration).
     * @param[in] from       first sample to include. If omitted, defaults to the first sample in data (optional).
     * @param[in] to         last sample to include. If omitted, defaults to the last sample in data (optional).
     * @param[in] sel        channel selection vector (optional).
     *
     * @return true if succeeded, false otherwise.
     */
    inline static bool read_raw_segment(FiffRawData& raw,
                                        Eigen::MatrixXd& data,
                                        Eigen::MatrixXd& times,
                                        Eigen::SparseMatrix<double>& mult,
                                        fiff_int_t from = -1,
                                        fiff_int_t to = -1,
                                        Eigen::MatrixXi sel = defaultMatrixXi)
    {
        return raw.read_raw_segment(data, times, mult, from, to, sel);
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
     * @param[in] p_pStream opened fif file.
     * @param[out] p_pTag the read tag.
     * @param[in] pos position of the tag inside the fif file.
     *
     * @return true if succeeded, false otherwise.
     */
    inline static bool read_tag(FiffStream::SPtr& p_pStream, FiffTag::SPtr& p_pTag, qint64 pos = -1)
    {
        return p_pStream->read_tag(p_pTag, pos);
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
     * @param[in] p_pStream opened fif file.
     * @param[out] p_pTag the read tag info.
     *
     * @return true if succeeded, false otherwise.
     */
    static inline bool read_tag_info(FiffStream::SPtr& p_pStream, FiffTag::SPtr& p_pTag)
    {
        return p_pStream->read_tag_info(p_pTag);
    }

    //=========================================================================================================
    /**
     * fiff_setup_read_raw
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream::setup_read_raw static function
     *
     * Read information about raw data file
     *
     * @param[in] p_IODevice         A fiff IO device like a fiff QFile or QTCPSocket.
     * @param[out] data              The raw data information - contains the opened fiff file.
     * @param[in] allow_maxshield    Accept unprocessed MaxShield data.
     *
     * @return true if succeeded, false otherwise.
     */
    inline static bool setup_read_raw(QIODevice &p_IODevice, FiffRawData& data, bool allow_maxshield = false)
    {
        return FiffStream::setup_read_raw(p_IODevice, data, allow_maxshield);
    }

    //=========================================================================================================
    /**
     * fiff_split_name_list
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream::split_name_list static function
     *
     * Splits a string by looking for seperator ":"
     *
     * @param[in] p_sNameList    string to split.
     *
     * @return the splitted string list.
     */
    inline static QStringList split_name_list(QString p_sNameList)
    {
        return FiffStream::split_name_list(p_sNameList);
    }

    //=========================================================================================================
    /**
     * fiff_start_block
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream start_block member function
     *
     * Writes a FIFF_BLOCK_START tag
     *
     * @param[in] p_pStream  An open fif file to write to.
     * @param[in] kind       The block kind to start.
     */
    inline static void start_block(FiffStream* p_pStream, fiff_int_t kind)
    {
        p_pStream->start_block(kind);
    }

    //=========================================================================================================
    /**
     * fiff_start_file
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream::start_file static function
     *
     * Opens a fiff file for writing and writes the compulsory header tags
     *
     * @param[in] p_IODevice   A fiff IO device like a fiff QFile or QTCPSocket.
     *
     * @return The opened file.
     */
    inline static FiffStream::SPtr start_file(QIODevice& p_IODevice)
    {
        return FiffStream::start_file(p_IODevice);
    }

    //=========================================================================================================
    /**
     * fiff_start_writing_raw
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream::start_writing_raw static function
     *
     * function [fid,cals] = fiff_start_writing_raw(name,info,sel)
     *
     * @param[in] p_IODevice     A fiff IO device like a fiff QFile or QTCPSocket.
     * @param[in] info           The measurement info block of the source file.
     * @param[out] cals          Thecalibration matrix.
     * @param[in] sel            Which channels will be included in the output file (optional).
     *
     * @return the started fiff file.
     */
    inline static FiffStream::SPtr start_writing_raw(QIODevice &p_IODevice,
                                                     const FiffInfo& info,
                                                     Eigen::RowVectorXd& cals,
                                                     Eigen::MatrixXi sel = defaultMatrixXi)
    {
        return FiffStream::start_writing_raw(p_IODevice, info, cals, sel);
    }

    //=========================================================================================================
    /**
     * fiff_write_ch_info
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream write_ch_info member function
     *
     * Writes a channel information record to a fif file
     * The type, cal, unit, and pos members are explained in Table 9.5
     * of the MNE manual
     *
     * @param[in] p_pStream  An open fif file.
     * @param[in] ch         The channel information structure to write.
     */
    inline static void write_ch_info(FiffStream* p_pStream, const FiffChInfo& ch)
    {
        p_pStream->write_ch_info(ch);
    }

    //=========================================================================================================
    /**
     * fiff_write_coord_trans
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream write_coord_trans member function
     *
     * Writes a coordinate transformation structure
     *
     * @param[in] p_pStream  An open fif file.
     * @param[in] trans      The coordinate transfomation structure.
     */
    inline static void write_coord_trans(FiffStream* p_pStream, FiffCoordTrans& trans)
    {
        p_pStream->write_coord_trans(trans);
    }

    //=========================================================================================================
    /**
     * fiff_write_ctf_comp
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream write_ctf_comp member function
     *
     * Writes the CTF compensation data into a fif file
     *
     * @param[in] p_pStream  An open fif file.
     * @param[in] comps      The compensation data to write.
     */
    inline static void write_ctf_comp(FiffStream* p_pStream, QList<FiffCtfComp>& comps)
    {
        p_pStream->write_ctf_comp(comps);
    }

    //=========================================================================================================
    /**
     * fiff_write_dig_point
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream write_dig_point member function
     *
     * Writes a digitizer data point into a fif file
     *
     * @param[in] p_pStream  An open fif file.
     * @param[in] dig        The point to write.
     */
    inline static void write_dig_point(FiffStream* p_pStream, FiffDigPoint& dig)
    {
        p_pStream->write_dig_point(dig);
    }

    //=========================================================================================================
    /**
     * fiff_write_id
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream write_id member function
     *
     * Writes fiff id
     * If the id argument is missing it will be generated here
     *
     * @param[in] p_pStream  An open fif file.
     * @param[in] kind       The tag kind.
     * @param[in] id         The id to write.
     */
    inline static void write_id(FiffStream* p_pStream, fiff_int_t kind, FiffId& id = FiffId::getDefault())
    {
        p_pStream->write_id(kind, id);
    }

    //=========================================================================================================
    /**
     * fiff_write_int
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream write_int member function
     *
     * Writes a 32-bit integer tag to a fif file
     *
     * @param[in] p_pStream  An open fif file.
     * @param[in] kind       Tag kind.
     * @param[in] data       The integer data pointer.
     * @param[in] nel        Number of integers to write (default = 1).
     */
    inline static void write_int(FiffStream* p_pStream, fiff_int_t kind, const fiff_int_t* data, fiff_int_t nel = 1)
    {
        p_pStream->write_int(kind, data, nel);
    }

    //=========================================================================================================
    /**
     * fiff_write_float
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream write_float member function
     *
     * Writes a single-precision floating point tag to a fif file
     *
     * @param[in] p_pStream  An open fif file.
     * @param[in] kind       Tag kind.
     * @param[in] data       The float data pointer.
     * @param[in] nel        Number of floats to write (default = 1).
     */
    inline static void write_float(FiffStream* p_pStream, fiff_int_t kind, float* data, fiff_int_t nel = 1)
    {
        p_pStream->write_float(kind, data, nel);
    }

    //=========================================================================================================
    /**
     * fiff_write_float_matrix
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream write_float_matrix member function
     *
     * Writes a single-precision floating-point matrix tag
     *
     * @param[in] p_pStream    An open fif file.
     * @param[in] kind       The tag kind.
     * @param[in] mat        The data matrix.
     */
    inline static void write_float_matrix(FiffStream* p_pStream, fiff_int_t kind, Eigen::MatrixXf& mat)
    {
        p_pStream->write_float_matrix(kind, mat);
    }

    //=========================================================================================================
    /**
     * fiff_write_name_list
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream write_name_list member function
     *
     * Writes a colon-separated list of names
     *
     * @param[in] p_pStream    An open fif file.
     * @param[in] kind       The tag kind.
     * @param[in] data       An array of names to create the list from.
     */
    inline static void write_name_list(FiffStream* p_pStream, fiff_int_t kind, QStringList& data)
    {
        p_pStream->write_name_list(kind, data);
    }

    //=========================================================================================================
    /**
     * fiff_write_named_matrix
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream write_named_matrix member function
     *
     * Writes a named single-precision floating-point matrix
     *
     * @param[in] p_pStream  An open fif file.
     * @param[in] kind       The tag kind to use for the data.
     * @param[in] mat        The data matrix.
     */
    inline static void write_named_matrix(FiffStream* p_pStream, fiff_int_t kind,FiffNamedMatrix& mat)
    {
        p_pStream->write_named_matrix(kind, mat);
    }

    //=========================================================================================================
    /**
     * fiff_write_proj
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream write_proj member function
     *
     * Writes the projection data into a fif file
     *
     * @param[in] p_pStream    An open fif file.
     * @param[in] projs      The compensation data to write.
     */
    inline static void write_proj(FiffStream* p_pStream, QList<FiffProj>& projs)
    {
        p_pStream->write_proj(projs);
    }

    //=========================================================================================================
    /**
     * fiff_write_raw_buffer
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream write_raw_buffer member function
     *
     * Writes a raw buffer.
     *
     * @param[in] p_pStream    An open fif file.
     * @param[in] buf        the buffer to write.
     * @param[in] cals       calibration factors.
     *
     * @return true if succeeded, false otherwise.
     */
    inline static bool write_raw_buffer(FiffStream* p_pStream, const Eigen::MatrixXd& buf, const Eigen::MatrixXd& cals)
    {
        return p_pStream->write_raw_buffer(buf, cals);
    }

    //=========================================================================================================
    /**
     * fiff_write_string
     *
     * ### MNE toolbox root function ###
     *
     * Wrapper for the FiffStream write_string member function
     *
     * Writes a string tag
     *
     * @param[in] p_pStream    An open fif file.
     * @param[in] kind       The tag kind.
     * @param[in] data       The string data to write.
     */
    inline static void write_string(FiffStream* p_pStream, fiff_int_t kind, QString& data)
    {
        p_pStream->write_string(kind, data);
    }
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#endif // FIFF_H
