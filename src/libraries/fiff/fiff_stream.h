//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_stream.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     October 2012
 * @brief    FIFF binary tag-stream layer: reads and writes FIFF tags, directories, blocks and the structured records nested inside them.
 *
 * @ref mne::FiffStream is the workhorse of mne_fiff. It owns a
 * @ref mne::ByteStream "byte stream" (a file, or bytes in memory), reads and
 * writes the 16-byte FIFF tag header, decodes payloads through the
 * endian-swap helpers in @ref fiff_byte_swap.h, and assembles the
 * directory and block tree on the way in. The high-level methods then
 * trade tag streams for typed objects: @ref mne::FiffStream::read_meas_info "read_meas_info" returns a
 * @ref mne::FiffInfo, @c read_raw_data returns a @ref mne::FiffRawData,
 * @ref mne::FiffEvoked::read "FiffEvoked::read" returns a @ref mne::FiffEvoked, @ref mne::FiffStream::read_cov "read_cov" returns a
 * @ref mne::FiffCov, etc. On the writing side @ref mne::FiffStream::start_writing_raw "start_writing_raw",
 * @ref mne::FiffStream::write_tag "write_tag", @ref mne::FiffStream::write_int "write_int" / @ref mne::FiffStream::write_float "write_float" /
 * @ref mne::FiffStream::write_float_matrix "write_float_matrix", @ref mne::FiffStream::start_block "start_block" / @ref mne::FiffStream::end_block "end_block" and the
 * record-level writers (@ref mne::FiffStream::write_id "write_id", @ref mne::FiffStream::write_ch_info "write_ch_info",
 * @ref mne::FiffStream::write_coord_trans "write_coord_trans", ...) emit a fully spec-compliant FIFF file
 * that round-trips through MNE-Python and MNE-C unchanged.
 */

#ifndef FIFF_STREAM_H
#define FIFF_STREAM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_file.h"
#include "fiff_types.h"
#include "fiff_id.h"

#include "fiff_dir_node.h"
#include "fiff_dir_entry.h"

#include <base/byte_stream.h>
#include <base/error.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

#include <expected>
#include <filesystem>
#include <memory>
#include <string>
#include <optional>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
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
class FiffEvokedSet;
class FiffNamedMatrix;
class FiffDigPoint;
class FiffChInfo;
class FiffChPos;
class FiffCoordTrans;
class FiffDigitizerData;
class FiffAnnotations;
struct FiffDeviceInfo;

//=============================================================================================================
/**
 * @brief FIFF tag-stream reader/writer: exposes typed read_* / write_* methods for every FIFF block and record kind.
 *
 * Reads or writes one byte stream. Builds the FIFF directory and
 * @ref FiffDirNode block tree on @ref open and exposes typed loaders /
 * emitters for every structured record (id, channel info, coord trans,
 * named matrix, dig point, projector, CTF comp, raw segment, evoked,
 * covariance). The on-disk format is the Elekta/Neuromag FIFF
 * specification — files emitted here round-trip through @c mne.io.fiff
 * in MNE-Python without modification.
 *
 * @snippet ex_fiff_api/main.cpp fiff_stream_usage
 */

class FIFFSHARED_EXPORT FiffStream
{
public:
    //=========================================================================================================
    /**
     * Constructs a fiff stream that reads a file; open() opens it and reads its directory.
     *
     * @param[in] path   The file; if it cannot be opened, open() fails.
     */
    explicit FiffStream(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Constructs a fiff stream on a byte stream, e.g. bytes in memory.
     *
     * @param[in] bytes  The byte stream, already open; the fiff stream owns it.
     * @param[in] name   Name reported by stream_name(), e.g. the file path.
     */
    explicit FiffStream(std::unique_ptr<mne::ByteStream> bytes, std::string name = {});

    //=========================================================================================================
    /**
     * Opens a FIFF file for reading and reads its tag directory.
     *
     * @param[in] path   The file.
     *
     * @return The open stream, or why the file cannot be opened or is not a FIFF file.
     */
    [[nodiscard]] static mne::Expected<FiffStream> open(const std::filesystem::path& path);

    ~FiffStream();

    FiffStream(const FiffStream&) = delete;
    FiffStream& operator=(const FiffStream&) = delete;
    FiffStream(FiffStream&&) noexcept;
    FiffStream& operator=(FiffStream&&) noexcept;

    //=========================================================================================================
    /**
     * @return The bytes the stream reads and writes, e.g. to rewind them or to take what was written to memory;
     *         a stream constructed from a path has them only once it is open.
     */
    mne::ByteStream& byte_stream() const;

    //=========================================================================================================
    /**
     * @return Whether the underlying device or byte stream is open.
     */
    bool is_open() const;

    //=========================================================================================================
    /**
     * Opens a closed stream again without truncating it; the directory read before stays valid.
     *
     * @return Whether the stream is open.
     */
    bool reopen();

    //=========================================================================================================
    /**
     * Sets the byte order of the stream; FIFF files are big-endian, some raw data files little-endian.
     *
     * @param[in] littleEndian  True to read and write little-endian.
     */
    void set_little_endian(bool littleEndian);

    //=========================================================================================================
    /**
     * @return True when the stream reads and writes little-endian.
     */
    bool is_little_endian() const;

    //=========================================================================================================
    /**
     * Get the stream name
     *
     * @return the name of the current stream.
     */
    std::string stream_name();

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
    std::vector<FiffDirEntry>& dir();

    //=========================================================================================================
    /**
     * Returns the directory compiled into a tree
     * dir is set when open() was called.
     *
     * @return the directory.
     */
    const std::vector<FiffDirEntry>& dir() const;

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
    const std::shared_ptr<FiffDirNode>& dirtree() const;

    //=========================================================================================================
    /**
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
     * Writes the tag that ends a FIFF file; the file is complete once the stream is closed or destroyed.
     */
    void end_file();

    //=========================================================================================================
    /**
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
     * @return true if information is available, false otherwise.
     */
    bool get_evoked_entries(const std::vector<std::shared_ptr<FiffDirNode>>& evoked_node, StringList& comments, std::vector<fiff_int_t>& aspect_kinds, std::string& t);

    //=========================================================================================================
    /**
     * Reads the file id and the directory of tags, scanning the file if it has no directory.
     *
     * @return true if succeeded, false otherwise.
     */
    bool open();

    //=========================================================================================================
    /**
     * Close stream
     *
     *    *
     * @return true if succeeded, false otherwise.
     */
    bool close();

    //=========================================================================================================
    /**
     * Builds the block tree of a directory from the block that starts at an entry.
     *
     * @param[in] dentry    The directory.
     * @param[in] current   Index of the entry that starts the block.
     *
     * @return The block, or null if a tag cannot be read.
     */
    std::shared_ptr<FiffDirNode> make_subtree(const std::vector<FiffDirEntry>& dentry, qsizetype current);

    //=========================================================================================================
    /**
     * fiff_read_bad_channels
     *
     * Reads the bad channel list from a node if it exists
     * Note: In difference to mne-matlab this is not a static function. This is a method of the FiffDirNode
     *       class, that's why a tree object doesn't need to be handed to the function.
     *
     * @param[in] p_Node The node of interest.
     *
     * @return the bad channel list.
     */
    StringList read_bad_channels(const std::shared_ptr<FiffDirNode>& p_Node);

    //=========================================================================================================
    /**
     * Write a bad channel list as a FIFFB_MNE_BAD_CHANNELS block.
     *
     * @param[in] bads The list of bad channel names.
     */
    void write_bad_channels(const StringList& bads);

    //=========================================================================================================
    /**
     * Reads the first MNE-Python annotations block (FIFFB_MNE_ANNOTATIONS) below @p p_Node.
     *
     * @param[in] p_Node         The node to search, e.g. dirtree().
     * @param[out] annotations   Onsets in seconds since @p origTime, or since the first sample without it.
     * @param[out] origTime      orig_time as seconds and microseconds; origTime[0] is -1 when the block has none.
     *
     * @return false if there is no annotations block.
     */
    bool read_annotations(const std::shared_ptr<FiffDirNode>& p_Node, FiffAnnotations& annotations, fiff_int_t origTime[2]);

    //=========================================================================================================
    /**
     * Writes @p annotations as an MNE-Python annotations block (FIFFB_MNE_ANNOTATIONS): onsets and ends as
     * float, descriptions as a colon-separated list, channel names and extras as JSON.
     *
     * @param[in] annotations   Onsets in seconds since @p origTime, or since the first sample without it.
     * @param[in] origTime      orig_time as seconds and microseconds (e.g. FiffInfo::meas_date), or nullptr.
     *
     * @return The position of the block start.
     */
    fiff_long_t write_annotations(const FiffAnnotations& annotations, const fiff_int_t* origTime = nullptr);

    //=========================================================================================================
    /**
     * Writes a FIFFB_DEVICE block; nothing for an empty @p device.
     *
     * @param[in] device   The acquisition device.
     *
     * @return The position of the block start, or -1 if nothing was written.
     */
    fiff_long_t write_device_info(const FiffDeviceInfo& device);

    //=========================================================================================================
    /**
     * Attach environment metadata (working directory, command line) to this stream.
     * Formerly mne_attach_env (SVN MNE).
     *
     * The stream must already be opened for update (via open_update).
     * Finds the first known top-level block (MNE, MEAS, MRI, or BEM)
     * and inserts a FIFFB_MNE_ENV block with working directory and
     * command-line string.
     *
     * @param[in] workingDir  The working directory to record.
     * @param[in] command     The command-line string to record.
     * @return true on success, false on error.
     */
    bool attach_env(const std::string& workingDir, const std::string& command);

    //=========================================================================================================
    /**
     * mne_read_cov - also for mne_read_noise_cov
     *
     * Reads a covariance matrix from a fiff file
     *
     * @param[in] p_Node          look for the matrix in here.
     * @param[in] cov_kind      what kind of a covariance matrix do we want?.
     * @param[in, out] p_covData    the read covariance matrix.
     *
     * @return true if succeeded, false otherwise.
     */
    bool read_cov(const std::shared_ptr<FiffDirNode>& p_Node, fiff_int_t cov_kind, FiffCov& p_covData);

    //=========================================================================================================
    /**
     * fiff_read_ctf_comp
     *
     * Read the CTF software compensation data from the given node
     *
     * @param[in] p_Node The node of interest.
     * @param[in] p_Chs  channels with the calibration info.
     *
     * @return the CTF software compensation data, or none if a compensator is incomplete or names a
     *         channel that is missing from, or ambiguous in, p_Chs.
     */
    std::vector<FiffCtfComp> read_ctf_comp(const std::shared_ptr<FiffDirNode>& p_Node, const std::vector<FiffChInfo>& p_Chs);

    //=========================================================================================================
    /**
     * Reads the digitizer data below a node, as MNE-C's load_digitizer_data (digitizer.c): the points
     * of its isotrak block in head coordinates, or of its MRI set in MRI coordinates.
     *
     * @param[in] node   The node.
     *
     * @return The data, all points active, or NotFound if the node holds no digitizer points.
     */
    [[nodiscard]] mne::Expected<FiffDigitizerData> read_digitizer_data(const std::shared_ptr<FiffDirNode>& node);

    //=========================================================================================================
    /**
     * fiff_read_meas_info
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
    bool read_meas_info(const std::shared_ptr<FiffDirNode>& p_Node, FiffInfo& p_Info, std::shared_ptr<FiffDirNode>& p_NodeInfo);

    //=========================================================================================================
    /**
     * python read_forward_meas_info
     *
     * Read light measurement info from forward operator.
     *
     * @param[in] p_Node         The node of interest.
     * @param[out] p_InfoForward The read light measurement info.
     *
     * @return true when successful.
     */
    bool read_meas_info_base(const std::shared_ptr<FiffDirNode>& p_Node, FiffInfoBase& p_InfoForward);

    //=========================================================================================================
    /**
     * fiff_read_named_matrix
     *
     * Reads a named matrix.
     *
     * @param[in] p_Node     The node of interest.
     * @param[in] matkind    The matrix kind to look for.
     * @param[out] mat       The named matrix.
     *
     * @return true if succeeded, false otherwise.
     */
    bool read_named_matrix(const std::shared_ptr<FiffDirNode>& p_Node, fiff_int_t matkind, FiffNamedMatrix& mat);

    //=========================================================================================================
    /**
     * Read the SSP data under a given directory node
     *
     * @param[in] p_Node    The node of interest.
     *
     * @return a list of SSP projectors.
     */
    std::vector<FiffProj> read_proj(const std::shared_ptr<FiffDirNode>& p_Node);

    //=========================================================================================================
    /**
     * Reads the payload of a tag whose header read_tag_info() has read.
     *
     * @param[in, out] tag   The tag; its payload is filled in, in host byte order.
     * @param[in] pos        Where the payload starts, or -1 for the current position.
     *
     * @return Whether the payload could be read.
     */
    bool read_tag_data(FiffTag& tag, fiff_long_t pos = -1);

    //=========================================================================================================
    /**
     * Reads the header of the tag at the current position.
     *
     * @param[in] skip   Move on to the next tag instead of stopping at the payload.
     *
     * @return The tag with its size but without payload, or nothing if the stream ends.
     */
    [[nodiscard]] std::optional<FiffTag> read_tag_info(bool skip = true);

    //=========================================================================================================
    /**
     * Reads one tag, with its payload in host byte order.
     *
     * @param[in] pos   Where the tag starts, or -1 for the current position.
     *
     * @return The tag, or nothing if it cannot be read.
     */
    [[nodiscard]] std::optional<FiffTag> read_tag(fiff_long_t pos = -1);

    //=========================================================================================================
    /**
     * fiff_split_name_list
     *
     * Splits a FIFF name list at its separator ":".
     *
     * @param[in] p_sNameList    The list, e.g. "MEG 0113:MEG 0112".
     *
     * @return The names as stored, spaces included; empty names are skipped.
     */
    static StringList split_name_list(std::string_view p_sNameList);

    //=========================================================================================================
    /**
     * Writes a FIFF_BLOCK_START tag
     *
     * @param[in] kind       The block kind to start.
     *
     * @return the position where the start block struct was written to.
     */
    fiff_long_t start_block(fiff_int_t kind);

    //=========================================================================================================
    /**
     * Writes the compulsory header tags to bytes that are not a file, e.g. in memory.
     *
     * @param[in] bytes   Where to write; the returned stream owns them.
     *
     * @return The stream, or nullptr if @p bytes is null.
     */
    static std::unique_ptr<FiffStream> start_file(std::unique_ptr<mne::ByteStream> bytes);

    //=========================================================================================================
    /**
     * Creates or truncates a FIFF file and writes the compulsory header tags.
     *
     * @param[in] path   The file.
     *
     * @return The stream, or nullptr if the file cannot be created.
     */
    static std::unique_ptr<FiffStream> start_file(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Opens an existing FIFF file for update, as MNE-C's fiff_open_update: tags can be rewritten in place, and
     * new tags go to the end, where its directory, which is dropped, used to be.
     *
     * @param[in] path   The file.
     *
     * @return The stream, or nullptr if the file does not exist or is not a FIFF file.
     */
    static std::unique_ptr<FiffStream> open_update(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * fiff_start_writing_raw
     *
     * function [fid,cals] = fiff_start_writing_raw(name,info,sel)
     *
     * @param[in] path           The file to create.
     * @param[in] info           The measurement info block of the source file.
     * @param[out] cals          A copy of the calibration values.
     * @param[in] sel            Which channels will be included in the output file (optional).
     * @param[in] bResetRange    Flag whether to reset the channel range to 1.0. Default is true.
     * @param[in] dataType       FIFF_DATA_PACK of the buffers that follow: FIFFT_FLOAT (write_raw_buffer), or
     *                           FIFFT_DAU_PACK16, FIFFT_INT, FIFFT_DOUBLE for writers of native samples.
     *
     * @return the started fiff file, or null if the file cannot be created.
     */
    static std::unique_ptr<FiffStream> start_writing_raw(const std::filesystem::path& path,
                                                         const FiffInfo& info,
                                                         Eigen::RowVectorXd& cals,
                                                         Eigen::MatrixXi sel = defaultMatrixXi,
                                                         bool bResetRange = true,
                                                         fiff_int_t dataType = FIFFT_FLOAT);

    //=========================================================================================================
    /**
     * Starts raw data in bytes that are not a file, e.g. in memory.
     *
     * @param[in] bytes          Where to write; the returned stream owns them.
     * @param[in] info           The measurement info block of the source file.
     * @param[out] cals          A copy of the calibration values.
     * @param[in] sel            Which channels will be included in the output (optional).
     * @param[in] bResetRange    Flag whether to reset the channel range to 1.0. Default is true.
     * @param[in] dataType       FIFF_DATA_PACK of the buffers that follow, as for the path overload.
     *
     * @return the started stream, or null if @p bytes is null.
     */
    static std::unique_ptr<FiffStream> start_writing_raw(std::unique_ptr<mne::ByteStream> bytes,
                                                         const FiffInfo& info,
                                                         Eigen::RowVectorXd& cals,
                                                         Eigen::MatrixXi sel = defaultMatrixXi,
                                                         bool bResetRange = true,
                                                         fiff_int_t dataType = FIFFT_FLOAT);

    //=========================================================================================================
    /**
     * Writes one tag; its payload is taken in host byte order, as read_tag() returns it.
     *
     * @param[in] tag   The tag.
     * @param[in] pos   Where to write it, or -1 for the end of the file.
     *
     * @return The position the tag was written to.
     */
    fiff_long_t write_tag(const FiffTag& tag, fiff_long_t pos = -1);

    //=========================================================================================================
    /**
     * Writes a channel information record to a fif file
     * The type, cal, unit, and pos members are explained in Table 9.5
     * of the MNE manual
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
     *
     * @param[in] trans  The coordinate transfomation structure.
     *
     * @return the position where the coordinate transformation struct was written to.
     */
    fiff_long_t write_coord_trans(const FiffCoordTrans& trans);

    //=========================================================================================================
    /**
     * Write a noise covariance matrix
     *
     * @param[in] p_FiffCov      The noise covariance matrix to write.
     *
     * @return the position where the covaraince was written to.
     */
    fiff_long_t write_cov(const FiffCov& p_FiffCov);

    //=========================================================================================================
    /**
     * fiff_write_ctf_comp
     *
     * Writes the CTF compensation data into a fif file
     *
     * @param[in] comps  The compensation data to write.
     *
     * @return the position where the ctf compensators struct was written to.
     */
    fiff_long_t write_ctf_comp(const std::vector<FiffCtfComp>& comps);

    //=========================================================================================================
    /**
     * fiff_write_dig_point
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
    fiff_long_t write_dir_entries(const std::vector<FiffDirEntry>& dir, fiff_long_t pos = -1);

    //=========================================================================================================
    /**
     * Writes a double-precision floating point tag to a fif file
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
    fiff_long_t write_info_base(const FiffInfoBase& p_FiffInfoBase);

    //=========================================================================================================
    /**
     * Writes a 32-bit integer tag to a fif file
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
     * Writes a colon-separated list of names
     *
     * @param[in] kind       The tag kind.
     * @param[in] data       An array of names to create the list from.
     *
     * @return the position where the name list struct was written to.
     */
    fiff_long_t write_name_list(fiff_int_t kind, const StringList& data);

    //=========================================================================================================
    /**
     * fiff_write_named_matrix
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
     * fiff_write_evoked_set
     *
     * Writes a complete evoked data set to a fif stream (file).
     * This writes FIFFB_MEAS containing measurement info and one FIFFB_EVOKED/FIFFB_ASPECT
     * block per evoked response.
     *
     * @param[in] p_FiffEvokedSet   The evoked data set to write.
     *
     * @return the position where the evoked set was written to, or -1 on failure.
     */
    fiff_long_t write_evoked_set(const FiffEvokedSet& p_FiffEvokedSet);

    //=========================================================================================================
    /**
     * fiff_write_proj
     *
     * Writes the projection data into a fif stream (file)
     *
     * @param[in] projs      The compensation data to write.
     *
     * @return the position where the projector struct was written to.
     */
    fiff_long_t write_proj(const std::vector<FiffProj>& projs);

    //=========================================================================================================
    /**
     * fiff_write_raw_buffer
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
     *
     * @param[in] kind       The tag kind.
     * @param[in] data       The string data to write.
     *
     * @return the position where the string struct was written to.
     */
    fiff_long_t write_string(fiff_int_t kind, std::string_view data);

    //=========================================================================================================
    /**
     * Writes a real-time command
     *
     * @param[in] command    The real time command.
     * @param[in] data       The string data to write.
     */
    void write_rt_command(fiff_int_t command, const std::string& data);

    //=========================================================================================================
    /**
     * Copy the FIFF processing history block from one file to another.
     *
     * Opens the source file, locates the FIFFB_PROCESSING_HISTORY block,
     * reads all tags within it, then appends them to the destination file.
     *
     * @param[in] fromPath   Path to the source FIFF file.
     * @param[in] toPath     Path to the destination FIFF file.
     *
     * @return true on success, false on error.
     *
     * @since 2.2.0
     */
    static bool copy_processing_history(const std::filesystem::path& fromPath, const std::filesystem::path& toPath);

private:
    /** Writes the file id, directory pointer and free list that open every FIFF file. */
    static std::unique_ptr<FiffStream> start_file(std::unique_ptr<FiffStream> stream);


    /** Writes the measurement info and starts the raw data block of a started file. */
    static std::unique_ptr<FiffStream> start_writing_raw(std::unique_ptr<FiffStream> stream,
                                                         const FiffInfo& info,
                                                         Eigen::RowVectorXd& cals,
                                                         Eigen::MatrixXi sel,
                                                         bool bResetRange,
                                                         fiff_int_t dataType);

    //=========================================================================================================
    /**
     * Reads the file id tag a FIFF file starts with.
     *
     * @return The tag, or nothing if the file does not start with one.
     */
    [[nodiscard]] std::optional<FiffTag> check_beginning();

    //=========================================================================================================
    /**
     * Scan the tag list to create a directory
     *
     * @param[out] ok    If a conversion error occurs, *ok is set to false; otherwise *ok is set to true.
     *
     * @return The created directory.
     */
    std::vector<FiffDirEntry> make_dir(bool* ok = nullptr);

    fiff_long_t tell() const;
    bool seek(fiff_long_t pos);
    void seek_to_end();
    void read_raw(void* data, fiff_long_t bytes);
    void write_raw(const void* data, fiff_long_t bytes);

    // Reads one value in the stream's byte order; a short read yields 0 and marks the stream failed
    template<mne::Swappable T>
    void read(T& value)
    {
        if (m_bytes->read(&value, sizeof(T)) != sizeof(T)) {
            value = T{};
            m_readFailed = true;
            return;
        }
        if (m_littleEndian != (std::endian::native == std::endian::little))
            value = mne::byteSwap(value);
    }

    template<mne::Swappable T>
    void write(T value)
    {
        if (m_littleEndian != (std::endian::native == std::endian::little))
            value = mne::byteSwap(value);
        m_bytes->write(&value, sizeof(T));
    }

private:
    std::filesystem::path m_path;             /**< File of the path constructor, opened by open(). */
    std::unique_ptr<mne::ByteStream> m_bytes; /**< Null until a path is opened, or if it cannot be. */
    std::string m_name;                       /**< File path, or empty for bytes in memory. */
    bool m_littleEndian = false;
    bool m_readFailed = false;
    FiffId m_id;                            /**< The file identifier. */
    std::vector<FiffDirEntry> m_dir;        /**< This is the directory. If no directory exists, open automatically scans the file to create one. */
    std::shared_ptr<FiffDirNode> m_dirtree; /**< Directory compiled into a tree. */
};
} // NAMESPACE

#endif // FIFF_STREAM_H
