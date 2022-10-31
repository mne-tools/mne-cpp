//=============================================================================================================
/**
 * @file     fiff_stream.h
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
 * @brief    FiffStream class declaration
 *
 */

#ifndef FIFF_STREAM_H
#define FIFF_STREAM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"
#include "fiff_id.h"

#include "fiff_dir_node.h"
#include "fiff_dir_entry.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QList>
#include <QSharedPointer>
#include <QString>
#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FiffStream;
class FiffTag;
class FiffCtfComp;
class FiffRawData;
class FiffInfo;
class FiffInfoBase;
class FiffCov;
class FiffProj;
class FiffNamedMatrix;
class FiffDigPoint;
class FiffChInfo;
class FiffChPos;
class FiffCoordTrans;
class FiffDigitizerData;

//=============================================================================================================
/**
 * FiffStream provides an interface for reading from and writing to fiff files
 * Comparable to: fiffFile (struct *fiffFile,fiffFileRec)
 *
 * @brief FIFF File I/O routines.
 **/

class FIFFSHARED_EXPORT FiffStream : public QDataStream
{
public:
    typedef QSharedPointer<FiffStream> SPtr;            /**< Shared pointer type for FiffStream. */
    typedef QSharedPointer<const FiffStream> ConstSPtr; /**< Const shared pointer type for FiffStream. */

    //=========================================================================================================
    /**
     * Constructs a fiff stream that uses the I/O device p_pIODevice.
     *
     * @param[in] p_pIODevice    A fiff IO device like a fiff QFile or QTCPSocket.
     */
    explicit FiffStream(QIODevice *p_pIODevice);

    //=========================================================================================================
    /**
     * Constructs a fiff stream that operates on a byte array, a. The mode describes how the device is to be used.
     *
     * @param[in] a      The byte array.
     * @param[in] mode   The open mode.
     */
    explicit FiffStream(QByteArray * a, QIODevice::OpenMode mode);

    //=========================================================================================================
    /**
     * Get the stream name
     *
     * @return the name of the current stream.
     */
    QString streamName();

    //=========================================================================================================
    /**
     * Returns the file identifier
     *
     * @return The file identifier.
     */
    FiffId id() const;

    //=========================================================================================================
    /**
     * Returns the directory compiled into a tree
     * dir is set when open() was called.
     *
     * @return the directory.
     */
    QList<FiffDirEntry::SPtr>& dir();

    //=========================================================================================================
    /**
     * Returns the directory compiled into a tree
     * dir is set when open() was called.
     *
     * @return the directory.
     */
    const QList<FiffDirEntry::SPtr>& dir() const;

    //=========================================================================================================
    /**
     * How many entries?
     *
     * @return The number of directory entries.
     */
    int nent() const;

    //=========================================================================================================
    /**
     * Returns the directory compiled into a tree
     * tree is set when open() was called.
     *
     * @return the compiled directory.
     */
    const FiffDirNode::SPtr& dirtree() const;

    //=========================================================================================================
    /**
     * ### MNE toolbox root function ###: Definition of the fiff_end_block function
     *
     * Writes a FIFF_BLOCK_END tag
     *
     * @param[in] kind   The block kind to end.
     * @param[in] next   Position of the next tag (default = FIFFV_NEXT_SEQ).
     *
     * @return the position where the end block struct was written to.
     */
    fiff_long_t end_block(fiff_int_t kind, fiff_int_t next = FIFFV_NEXT_SEQ);

    //=========================================================================================================
    /**
     * Writes the closing tags to a fif file and closes the file
     * Refactored: fiff_end_file (MNE-C); fiff_end_file (MNE-MATLAB)
     */
    void end_file();

    //=========================================================================================================
    /**
     * ### MNE toolbox root function ###: Definition of the fiff_finish_writing_raw function
     *
     * Finishes a raw file by writing all necessary end tags.
     *
     */
    void finish_writing_raw();

    //=========================================================================================================
    /**
     * Helper to get all evoked entries
     *
     * @param[in] evoked_node    evoked tree nodes.
     * @param[out] comments      found comments.
     * @param[out] aspect_kinds  found aspect_kinds.
     * @param[out] t             text formatted found information.
     *
     * @return true if information is available, fasle otherwise.
     */
    bool get_evoked_entries(const QList<FiffDirNode::SPtr> &evoked_node, QStringList &comments, QList<fiff_int_t> &aspect_kinds, QString &t);

    //=========================================================================================================
    /**
     * QFile::open
     *
     * unmask base class open function
     */
//    using QFile::open;

    //=========================================================================================================
    /**
     * Opens a fif file and provides the directory of tags
     * Refactored: open_file (fiff_open.c)
     *
     * @param[in] mode      The open mode (Default = ReadOnly).
     *
     * @return true if succeeded, false otherwise.
     */
    bool open(QIODevice::OpenModeFlag mode = QIODevice::ReadOnly);

    //=========================================================================================================
    /**
     * Close stream
     *
     * Refactored: fiff_close (fiff_open.c)
     *    *
     * @return true if succeeded, false otherwise.
     */
    bool close();

    //=========================================================================================================
    /**
     * Create the directory tree structure
     * Refactored: make_subtree (fiff_dir_tree.c), fiff_make_dir_tree (MATLAB)
     *
     * @param[in] dentry     The dir entries of which the tree should be constructed.
     *
     * @return The created dir tree.
     */
    FiffDirNode::SPtr make_subtree(QList<FiffDirEntry::SPtr>& dentry);

    //=========================================================================================================
    /**
     * fiff_read_bad_channels
     *
     * ### MNE toolbox root function ###
     *
     * Reads the bad channel list from a node if it exists
     * Note: In difference to mne-matlab this is not a static function. This is a method of the FiffDirNode
     *       class, that's why a tree object doesn't need to be handed to the function.
     *
     * @param[in] p_Node The node of interest.
     *
     * @return the bad channel list.
     */
    QStringList read_bad_channels(const FiffDirNode::SPtr& p_Node);

    //=========================================================================================================
    /**
     * mne_read_cov - also for mne_read_noise_cov
     *
     * ### MNE toolbox root function ###
     *
     * Reads a covariance matrix from a fiff file
     *
     * @param[in] p_Node          look for the matrix in here.
     * @param[in] cov_kind      what kind of a covariance matrix do we want?.
     * @param[in, out] p_covData    the read covariance matrix.
     *
     * @return true if succeeded, false otherwise.
     */
    bool read_cov(const FiffDirNode::SPtr& p_Node, fiff_int_t cov_kind, FiffCov& p_covData);

    //=========================================================================================================
    /**
     * fiff_read_ctf_comp
     *
     * ### MNE toolbox root function ###
     *
     * Read the CTF software compensation data from the given node
     *
     * @param[in] p_Node The node of interest.
     * @param[in] p_Chs  channels with the calibration info.
     *
     * @return the CTF software compensation data.
     */
    QList<FiffCtfComp> read_ctf_comp(const FiffDirNode::SPtr& p_Node, const QList<FiffChInfo>& p_Chs);

    //=========================================================================================================
    /**
     * Reimplemntation of load_digitizer_data (digitizer.c)
     *
     * ### MNE-C root function ###
     *
     * Read the digitizer data from the given node.
     *
     * @param[in] p_Node     The node of interest.
     * @param[out] p_digData  The read digitizer data.
     *
     * @return rue if succeeded, false otherwise.
     */
    bool read_digitizer_data(const FiffDirNode::SPtr& p_Node, FiffDigitizerData& p_digData);

    //=========================================================================================================
    /**
     * fiff_read_meas_info
     *
     * ### MNE toolbox root function ###
     *
     * Read the measurement info
     * Source is assumed to be an open fiff file.
     *
     * @param[in] p_Node       The node of interest.
     * @param[out] p_Info      The read measurement info.
     * @param[out] p_NodeInfo  The to measurement corresponding fiff_dir_node.
     *
     * @return true if successful.
     */
    bool read_meas_info(const FiffDirNode::SPtr& p_Node, FiffInfo& p_Info, FiffDirNode::SPtr& p_NodeInfo);

    //=========================================================================================================
    /**
     * python read_forward_meas_info
     *
     * Read light measurement info from forward operator -> ToDo base class of FiffInfo
     *
     * @param[in] p_Node         The node of interest.
     * @param[out] p_InfoForward The read light measurement info.
     *
     * @return true when successful.
     */
    bool read_meas_info_base(const FiffDirNode::SPtr& p_Node, FiffInfoBase& p_InfoForward);

    //=========================================================================================================
    /**
     * fiff_read_named_matrix
     *
     * ### MNE toolbox root function ###
     *
     * Reads a named matrix.
     *
     * @param[in] p_Node     The node of interest.
     * @param[in] matkind    The matrix kind to look for.
     * @param[out] mat       The named matrix.
     *
     * @return true if succeeded, false otherwise.
     */
    bool read_named_matrix(const FiffDirNode::SPtr& p_Node, fiff_int_t matkind, FiffNamedMatrix& mat);

    //=========================================================================================================
    /**
     * Read the SSP data under a given directory node
     * Refactored: fiff_read_proj (MNE-MATLAB)
     *
     * @param[in] p_Node    The node of interest.
     *
     * @return a list of SSP projectors.
     */
    QList<FiffProj> read_proj(const FiffDirNode::SPtr& p_Node);

    //=========================================================================================================
    /**
     * Read tag data from a fif file.
     * if pos is not provided, reading starts from the current file position
     * Refactored: fiff_read_tag (MNE-MATLAB)
     *
     * @param[out] p_pTag the read tag.
     * @param[in] pos position of the tag inside the fif file.
     *
     * @return true if succeeded, false otherwise.
     */
    bool read_tag_data(QSharedPointer<FiffTag>& p_pTag, fiff_long_t pos = -1);

    //=========================================================================================================
    /**
     * Read tag information of one tag from a fif file.
     * if pos is not provided, reading starts from the current file position
     * Refactored: fiff_read_tag_info (MNE-MATLAB)
     *
     * @param[out] p_pTag the read tag info.
     * @param[in] p_bDoSkip if true it skips the data of the tag (optional, default = true).
     *
     * @return the position where the tag info was read from.
     */
    fiff_long_t read_tag_info(QSharedPointer<FiffTag>& p_pTag, bool p_bDoSkip = true);

    //=========================================================================================================
    /**
     * Read one tag from a fif real-time stream.
     * difference to the other read tag functions is: that this function has blocking behaviour (waitForReadyRead)
     *
     * @param[out] p_pTag the read tag.
     *
     * @return true if succeeded, false otherwise.
     */
    bool read_rt_tag(QSharedPointer<FiffTag>& p_pTag);

    //=========================================================================================================
    /**
     * Read one tag from a fif file.
     * if pos is not provided, reading starts from the current file position
     * Refactored: fiff_read_tag (MNE-MATLAB)
     *
     * @param[out] p_pTag the read tag.
     * @param[in] pos position of the tag inside the fif file.
     *
     * @return true if succeeded, false otherwise.
     */
    bool read_tag(QSharedPointer<FiffTag>& p_pTag,
                  fiff_long_t pos = -1);

    //=========================================================================================================
    /**
     * fiff_setup_read_raw
     *
     * ### MNE toolbox root function ###
     *
     * Read information about raw data file
     *
     * @param[in] p_IODevice        An fiff IO device like a fiff QFile or QTCPSocket.
     * @param[out] data              The raw data information - contains the opened fiff file.
     * @param[in] allow_maxshield    Accept unprocessed MaxShield data.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool setup_read_raw(QIODevice &p_IODevice,
                               FiffRawData& data,
                               bool allow_maxshield = true,
                               bool is_littleEndian = false);

    //=========================================================================================================
    /**
     * fiff_split_name_list
     *
     * ### MNE toolbox root function ###
     *
     * Splits a string by looking for seperator ":"
     *
     * @param[in] p_sNameList    string to split.
     *
     * @return the splitted string list.
     */
    static QStringList split_name_list(QString p_sNameList);

    //=========================================================================================================
    /**
     * Writes a FIFF_BLOCK_START tag
     * Refactored: fiff_start_block (MNE-C); fiff_start_block (MNE-MATLAB)
     *
     * @param[in] kind       The block kind to start.
     *
     * @return the position where the start block struct was written to.
     */
    fiff_long_t start_block(fiff_int_t kind);

    //=========================================================================================================
    /**
     * Opens a fiff file for writing and writes the compulsory header tags
     * Refactored: fiff_start_files (MNE-C); fiff_start_file (MNE-MATLAB)
     *
     * @param[in] p_IODevice    The IODevice (like QFile or QTCPSocket) to open. It is recommended that the name ends with .fif.
     *
     * @return The opened file.
     */
    static FiffStream::SPtr start_file(QIODevice& p_IODevice);

    //=========================================================================================================
    /**
     * Open fiff file for update
     * Refactored: fiff_open_update (MNE-C)
     *
     * @param[in] p_IODevice    The IODevice (like QFile or QTCPSocket) to open. It is recommended that the name ends with .fif.
     *
     * @return The opened file stream.
     */
    static FiffStream::SPtr open_update(QIODevice& p_IODevice);

    //=========================================================================================================
    /**
     * fiff_start_writing_raw
     *
     * ### MNE toolbox root function ###
     *
     * function [fid,cals] = fiff_start_writing_raw(name,info,sel)
     *
     * @param[in] p_IODevice     A fiff IO device like a fiff QFile or QTCPSocket.
     * @param[in] info           The measurement info block of the source file.
     * @param[out] cals          A copy of the calibration values.
     * @param[in] sel            Which channels will be included in the output file (optional).
     * @param[in] bResetRange    Flag whether to reset the channel range to 1.0. Default is true.
     *
     * @return the started fiff file.
     */
    static FiffStream::SPtr start_writing_raw(QIODevice &p_IODevice,
                                              const FiffInfo& info,
                                              Eigen::RowVectorXd& cals,
                                              Eigen::MatrixXi sel = defaultMatrixXi,
                                              bool bResetRange = true);

    //=========================================================================================================
    /**
     * Write one tag to file including its data
     * Data is not written if it is NULL
     * Refactored: fiff_write_tag, fiff_write_this_tag (MNE-C)
     *
     * @param[in] p_pTag     Tag to write;.
     * @param[in] pos        the position where the entires should be written to (default -1, i.e. end of the file).
     *
     * @return the position where the tag struct was written to.
     */
    fiff_long_t write_tag(const QSharedPointer<FiffTag>& p_pTag, fiff_long_t pos = -1);

    //=========================================================================================================
    /**
     * Writes a channel information record to a fif file
     * The type, cal, unit, and pos members are explained in Table 9.5
     * of the MNE manual
     * Refactored: fiff_write_ch_info (MNE-C); fiff_write_ch_info (MNE-MATLAB)
     *
     * @param[in] ch     The channel information structure to write.
     *
     * @return the position where the channel info struct was written to.
     */
    fiff_long_t write_ch_info(const FiffChInfo& ch);

    //=========================================================================================================
    /**
     * Writes a channel position to a fif file
     *
     * @param[in] chpos      Channel position structure to write.
     *
     * @return the position where the channel position struct was written to.
     */
    fiff_long_t write_ch_pos(const FiffChPos& chpos);

    //=========================================================================================================
    /**
     * Writes a coordinate transformation structure
     * Refactored: fiff_write_coord_trans (MNE-C); fiff_write_coord_trans (MNE-MATLAB)
     *
     * @param[in] trans  The coordinate transfomation structure.
     *
     * @return the position where the coordinate transformation struct was written to.
     */
    fiff_long_t write_coord_trans(const FiffCoordTrans &trans);

    //=========================================================================================================
    /**
     * Write a noise covariance matrix
     *
     * ### MNE toolbox root function ###
     *
     * @param[in] p_FiffCov      The noise covariance matrix to write.
     *
     * @return the position where the covaraince was written to.
     */
    fiff_long_t write_cov(const FiffCov &p_FiffCov);

    //=========================================================================================================
    /**
     * fiff_write_ctf_comp
     *
     * ### MNE toolbox root function ###
     *
     * Writes the CTF compensation data into a fif file
     *
     * @param[in] comps  The compensation data to write.
     *
     * @return the position where the ctf compensators struct was written to.
     */
    fiff_long_t write_ctf_comp(const QList<FiffCtfComp>& comps);

    //=========================================================================================================
    /**
     * fiff_write_dig_point
     *
     * ### MNE toolbox root function ###
     *
     * Writes a digitizer data point into a fif file
     *
     * @param[in] dig        The point to write.
     *
     * @return the position where the digitizer points struct was written to.
     */
    fiff_long_t write_dig_point(const FiffDigPoint& dig);

    //=========================================================================================================
    /**
     * Writes directory position pointer FIFF_DIR_POINTER
     * Returns the postion where the structure was written to.
     *
     * @param[in] dirpos     The directory position pointer.
     * @param[in] pos        the position where the directory pointer should be written to (default -1, i.e. end of the file).
     * @param[in] next       Position of the next tag (default = FIFFV_NEXT_SEQ).
     *
     * @return the position where the directory position pointer was written to.
     */
    fiff_long_t write_dir_pointer(fiff_int_t dirpos, fiff_long_t pos = -1, fiff_int_t next = FIFFV_NEXT_SEQ);

    //=========================================================================================================
    /**
     * Writes a list of dir entries to a fif file, as a FIFFT_DIR_ENTRY_STRUCT
     * Returns the postion where the structure was written to.
     *
     * @param[in] dir        The dir entries to write.
     * @param[in] pos        the position where the entires should be written to (default -1, i.e. end of the file).
     *
     * @return the position where the directory entries struct was written to.
     */
    fiff_long_t write_dir_entries(const QList<FiffDirEntry::SPtr>& dir, fiff_long_t pos = -1);

    //=========================================================================================================
    /**
     * Writes a double-precision floating point tag to a fif file
     * Refactored: fiff_write_double (MNE-MATLAB)
     *
     * @param[in] kind       Tag kind.
     * @param[in] data       The float data pointer.
     * @param[in] nel        Number of doubles to write (default = 1).
     *
     * @return the position where the double struct was written to.
     */
    fiff_long_t write_double(fiff_int_t kind, const double* data, fiff_int_t nel = 1);

    //=========================================================================================================
    /**
     * Writes fiff id
     * If the id argument is missing it will be generated here
     * Refactored: fiff_write_this_id (MNE-C); fiff_write_id (MNE-MATLAB)
     *
     * @param[in] kind       The tag kind.
     * @param[in] id         The id to write.
     *
     * @return the position where the file id struct was written to.
     */
    fiff_long_t write_id(fiff_int_t kind, const FiffId& id = FiffId::getDefault());

    //=========================================================================================================
    /**
     * Write measurement info stored in forward solution
     *
     * @param[in] p_FiffInfoBase     The measurement info.
     *
     * @return the position where the info base struct was written to.
     */
    fiff_long_t write_info_base(const FiffInfoBase & p_FiffInfoBase);

    //=========================================================================================================
    /**
     * Writes a 32-bit integer tag to a fif file
     * Refactored: fiff_write_int_tag (MNE-C); fiff_write_int (MNE-MATLAB)
     *
     * @param[in] kind       Tag kind.
     * @param[in] data       The integer data pointer.
     * @param[in] nel        Number of integers to write (default = 1).
     * @param[in] next       Position of the next tag (default = FIFFV_NEXT_SEQ).
     *
     * @return the position where the int was written to.
     */
    fiff_long_t write_int(fiff_int_t kind, const fiff_int_t* data, fiff_int_t nel = 1, fiff_int_t next = FIFFV_NEXT_SEQ);

    //=========================================================================================================
    /**
     * fiff_write_int_matrix
     *
     * ### MNE toolbox root function ###
     *
     * Writes a integer matrix tag
     *
     * @param[in] kind       The tag kind.
     * @param[in] mat        The data matrix.
     *
     * @return the position where the write_int_matrix was written to.
     */
    fiff_long_t write_int_matrix(fiff_int_t kind, const Eigen::MatrixXi& mat);

    //=========================================================================================================
    /**
     * Writes a single-precision floating point tag to a fif file
     * Refactored: fiff_write_float_tag (MNE-C); fiff_write_float (MNE-MATLAB)
     *
     * @param[in] kind       Tag kind.
     * @param[in] data       The float data pointer.
     * @param[in] nel        Number of floats to write (default = 1).
     *
     * @return the position where the float struct was written to.
     */
    fiff_long_t write_float(fiff_int_t kind, const float* data, fiff_int_t nel = 1);

    //=========================================================================================================
    /**
     * Writes a single-precision floating-point matrix tag
     * Refactored: fiff_write_float_matrix (MNE-C); fiff_write_float_matrix (MNE-MATLAB)
     *
     * @param[in] kind       The tag kind.
     * @param[in] mat        The data matrix.
     *
     * @return the position where the float matrix struct was written to.
     */
    fiff_long_t write_float_matrix(fiff_int_t kind, const Eigen::MatrixXf& mat);

    //=========================================================================================================
    /**
     * fiff_write_float_sparse_ccs
     *
     * ### MNE toolbox root function ###
     *
     * Writes a single-precision sparse (ccs) floating-point matrix tag
     *
     * @param[in] kind       The tag kind.
     * @param[in] mat        The data matrix.
     *
     * @return the position where the float sparse ccs matrix struct was written to.
     */
    fiff_long_t write_float_sparse_ccs(fiff_int_t kind, const Eigen::SparseMatrix<float>& mat);

    //=========================================================================================================
    /**
     * fiff_write_float_sparse_rcs
     *
     * ### MNE toolbox root function ###
     *
     * Writes a single-precision sparse (RCS) floating-point matrix tag
     *
     * @param[in] kind       The tag kind.
     * @param[in] mat        The data matrix.
     *
     * @return the position where the float sparse rcs matrix struct was written to.
     */
    fiff_long_t write_float_sparse_rcs(fiff_int_t kind, const Eigen::SparseMatrix<float>& mat);

    //=========================================================================================================
    /**
     * fiff_write_name_list
     *
     * ### MNE toolbox root function ###
     *
     * Writes a colon-separated list of names
     *
     * @param[in] kind       The tag kind.
     * @param[in] data       An array of names to create the list from.
     *
     * @return the position where the name list struct was written to.
     */
    fiff_long_t write_name_list(fiff_int_t kind, const QStringList& data);

    //=========================================================================================================
    /**
     * fiff_write_named_matrix
     *
     * ### MNE toolbox root function ###
     *
     * Writes a named single-precision floating-point matrix
     *
     * @param[in] kind       The tag kind to use for the data.
     * @param[in] mat        The data matrix.
     *
     * @return the position where the named matrix struct was written to.
     */
    fiff_long_t write_named_matrix(fiff_int_t kind, const FiffNamedMatrix& mat);

    //=========================================================================================================
    /**
     * fiff_write_proj
     *
     * ### MNE toolbox root function ###
     *
     * Writes the projection data into a fif stream (file)
     *
     * @param[in] projs      The compensation data to write.
     *
     * @return the position where the projector struct was written to.
     */
    fiff_long_t write_proj(const QList<FiffProj>& projs);

    //=========================================================================================================
    /**
     * fiff_write_raw_buffer
     *
     * ### MNE toolbox root function ###
     *
     * Writes a raw buffer.
     *
     * @param[in] buf        the buffer to write.
     * @param[in] cals       calibration factors.
     *
     * @return true if succeeded, false otherwise.
     */
    bool write_raw_buffer(const Eigen::MatrixXd& buf, const Eigen::RowVectorXd& cals);

    //=========================================================================================================
    /**
     * fiff_write_raw_buffer
     *
     * ### MNE toolbox root function ###
     *
     * Writes a raw buffer.
     *
     * @param[in] buf        the buffer to write.
     * @param[in] mult       the used multiplication matrix consisting out of projection,calibration and compensation.
     *
     * @return true if succeeded, false otherwise.
     */
    bool write_raw_buffer(const Eigen::MatrixXd& buf, const Eigen::SparseMatrix<double>& mult);

    //=========================================================================================================
    /**
     * fiff_write_raw_buffer
     *
     *
     * Writes a raw buffer without calibrations.
     *
     * @param[in] buf        the buffer to write.
     *
     * @return true if succeeded, false otherwise.
     */
    bool write_raw_buffer(const Eigen::MatrixXd& buf);

    //=========================================================================================================
    /**
     * Writes a string tag
     * Refactored: fiff_write_string_tag (MNE-C); fiff_write_string (MNE-MATLAB)
     *
     * @param[in] kind       The tag kind.
     * @param[in] data       The string data to write.
     *
     * @return the position where the string struct was written to.
     */
    fiff_long_t write_string(fiff_int_t kind, const QString& data);

    //=========================================================================================================
    /**
     * Writes a real-time command
     *
     * @param[in] command    The real time command.
     * @param[in] data       The string data to write.
     */
    void write_rt_command(fiff_int_t command, const QString& data);

private:
    //=========================================================================================================
    /**
     * Check that the file starts properly.
     * Refactored: check_beginning (fiff_open.c)
     *
     * @param[out] p_pTag     The tag containing the beginning.
     *
     * @return true if beginning is correct, false otherwise.
     */
    bool check_beginning(QSharedPointer<FiffTag>& p_pTag);

    //=========================================================================================================
    /**
     * Scan the tag list to create a directory
     * Refactored: fiff_make_dir (fiff_dir.c)
     *
     * @param[out] ok    If a conversion error occurs, *ok is set to false; otherwise *ok is set to true.
     *
     * @return The created directory.
     */
    QList<FiffDirEntry::SPtr> make_dir(bool *ok=Q_NULLPTR);

private:

//    char         *file_name;    /**< Name of the file. */ -> Use streamName() instead
//    FILE         *fd;           /**< The normal file descriptor. */ -> file descitpion is part of the stream: stream->device()
    FiffId                      m_id;   /**< The file identifier. */
    QList<FiffDirEntry::SPtr>   m_dir;  /**< This is the directory. If no directory exists, open automatically scans the file to create one. */
//    int         nent;           /**< How many entries?. */ -> Use nent() instead
    FiffDirNode::SPtr           m_dirtree; /**< Directory compiled into a tree. */
//    char        *ext_file_name; /**< Name of the file holding the external data. */
//    FILE        *ext_fd;        /**< The file descriptor of the above file if open . */

// ### OLD STRUCT ###
// /** FIFF file handle returned by fiff_open(). */
//typedef struct _fiffFileRec {
//    char         *file_name;    /**< Name of the file. */ -> part of the Parent class of the QIODevice, wrapped by streamName function
//    FILE         *fd;           /**< The normal file descriptor. */
//    fiffId       id;            /**< The file identifier. */
//    fiffDirEntry dir;           /**< This is the directory.
//                                   * If no directory exists, fiff_open
//                                   * automatically scans the file to create one. */
//    int         nent;           /**< How many entries?. */
//    fiffDirNode dirtree;        /**< Directory compiled into a tree. */
//    char        *ext_file_name; /**< Name of the file holding the external data. */
//    FILE        *ext_fd;        /**< The file descriptor of the above file if open . */
//} *fiffFile,fiffFileRec;        /**< FIFF file handle. fiff_open() returns this. */
};
} // NAMESPACE

#endif // FIFF_STREAM_H
