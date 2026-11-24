//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_stream.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Florian Schlembach <fschlembach@web.de>;
 *           Limin Sun <liminsun@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Eric Larson <larson.eric.d@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Implementation of @ref FiffStream: FIFF tag I/O, directory and block-tree construction, and the typed read_* / write_* methods.
 *
 * Owns one byte stream. Builds the directory and @ref FiffDirNode tree
 * on open; trades tag streams for typed mne_fiff objects on read; emits
 * spec-compliant FIFF on write. Files emitted here round-trip through
 * @c mne.io.fiff unchanged.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_stream.h"

#include "fiff_annotations.h"
#include "fiff_annotations_json.h"
#include "fiff_tag.h"
#include "fiff_dir_node.h"
#include "fiff_ctf_comp.h"
#include "fiff_info.h"
#include "fiff_info_base.h"
#include "fiff_raw_data.h"
#include "fiff_cov.h"
#include "fiff_evoked_set.h"
#include "fiff_coord_trans.h"
#include "fiff_ch_info.h"
#include "fiff_ch_pos.h"
#include "fiff_dig_point.h"
#include "fiff_id.h"
#include "fiff_digitizer_data.h"
#include <mne/base/log.h>
#include <mne/math/linalg.h>
#include <mne/math/numerics.h>
#include <mne/base/string_utils.h>
#include <time.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/LU>
#include <Eigen/Dense>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <format>
#include <vector>
#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>
#include <cstdint>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using mne::ErrorCode;
using mne::fail;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffStream::FiffStream(const std::filesystem::path& path)
: m_path(path)
, m_name(mne::utf8_from_path(path))
{
}

//=============================================================================================================

FiffStream::FiffStream(std::unique_ptr<mne::ByteStream> bytes, std::string name)
: m_bytes(std::move(bytes))
, m_name(std::move(name))
{
}

//=============================================================================================================

mne::Expected<FiffStream> FiffStream::open(const std::filesystem::path& path)
{
    const std::string name = mne::utf8_from_path(path);
    auto bytes = mne::FileByteStream::open(path, mne::FileByteStream::Mode::Read);
    if (!bytes) {
        return fail(ErrorCode::Io, "Cannot open {}", name);
    }
    FiffStream stream(std::move(bytes), name);
    if (!stream.open()) {
        return fail(ErrorCode::Format, "{} is not a FIFF file", name);
    }
    return stream;
}

//=============================================================================================================

std::unique_ptr<FiffStream> FiffStream::start_file(const std::filesystem::path& path)
{
    auto bytes = mne::FileByteStream::open(path, mne::FileByteStream::Mode::Write);
    if (!bytes) {
        Log::warning("Cannot write to {}", mne::utf8_from_path(path));
        return {};
    }
    return start_file(std::make_unique<FiffStream>(std::move(bytes), mne::utf8_from_path(path)));
}

//=============================================================================================================

std::unique_ptr<FiffStream> FiffStream::start_file(std::unique_ptr<FiffStream> stream)
{
    stream->write_id(FIFF_FILE_ID);
    int null_pointer = FIFFV_NEXT_NONE;
    stream->write_int(FIFF_DIR_POINTER, &null_pointer);
    stream->write_int(FIFF_FREE_LIST, &null_pointer);
    return stream;
}

//=============================================================================================================

FiffStream::~FiffStream() = default;

FiffStream::FiffStream(FiffStream&&) noexcept = default;

FiffStream& FiffStream::operator=(FiffStream&&) noexcept = default;

//=============================================================================================================

mne::ByteStream& FiffStream::byte_stream() const
{
    return *m_bytes;
}

//=============================================================================================================

bool FiffStream::is_open() const
{
    return m_bytes && m_bytes->is_open();
}

//=============================================================================================================

bool FiffStream::reopen()
{
    if (is_open())
        return true;
    if (!m_bytes && !m_path.empty()) {
        m_bytes = mne::FileByteStream::open(m_path, mne::FileByteStream::Mode::Read);
        return m_bytes != nullptr;
    }
    return m_bytes && m_bytes->reopen();
}

//=============================================================================================================

void FiffStream::set_little_endian(bool littleEndian)
{
    m_littleEndian = littleEndian;
}

//=============================================================================================================

bool FiffStream::is_little_endian() const
{
    return m_littleEndian;
}

//=============================================================================================================

fiff_long_t FiffStream::tell() const
{
    return static_cast<fiff_long_t>(m_bytes->pos());
}

//=============================================================================================================

bool FiffStream::seek(fiff_long_t pos)
{
    return pos >= 0 && m_bytes->seek(static_cast<std::uint64_t>(pos));
}

//=============================================================================================================

void FiffStream::seek_to_end()
{
    m_bytes->seek(m_bytes->size());
}

//=============================================================================================================

void FiffStream::read_raw(void* data, fiff_long_t bytes)
{
    if (bytes > 0 && m_bytes->read(data, static_cast<std::size_t>(bytes)) != static_cast<std::size_t>(bytes))
        m_readFailed = true;
}

//=============================================================================================================

void FiffStream::write_raw(const void* data, fiff_long_t bytes)
{
    if (bytes > 0)
        m_bytes->write(data, static_cast<std::size_t>(bytes));
}

//=============================================================================================================

std::string FiffStream::stream_name()
{
    return m_name.empty() ? std::string("memory") : m_name;
}

//=============================================================================================================

FiffId FiffStream::id() const
{
    return m_id;
}

//=============================================================================================================

std::vector<FiffDirEntry>& FiffStream::dir()
{
    return m_dir;
}

//=============================================================================================================

const std::vector<FiffDirEntry>& FiffStream::dir() const
{
    return m_dir;
}

//=============================================================================================================

int FiffStream::nent() const
{
    return static_cast<int>(m_dir.size());
}

//=============================================================================================================

const std::shared_ptr<FiffDirNode>& FiffStream::dirtree() const
{
    return m_dirtree;
}

//=============================================================================================================

fiff_long_t FiffStream::end_block(fiff_int_t kind, fiff_int_t next)
{
    return this->write_int(FIFF_BLOCK_END, &kind, 1, next);
}

//=============================================================================================================

void FiffStream::end_file()
{
    fiff_int_t datasize = 0;

    write(static_cast<std::int32_t>(FIFF_NOP));
    write(static_cast<std::int32_t>(FIFFT_VOID));
    write(static_cast<std::int32_t>(datasize));
    write(static_cast<std::int32_t>(FIFFV_NEXT_NONE));
}

//=============================================================================================================

void FiffStream::finish_writing_raw()
{
    this->end_block(FIFFB_RAW_DATA);
    this->end_block(FIFFB_MEAS);
    this->end_file();
    this->close();
}

//=============================================================================================================

bool FiffStream::get_evoked_entries(const std::vector<std::shared_ptr<FiffDirNode>>& evoked_node, StringList& comments, std::vector<fiff_int_t>& aspect_kinds, std::string& t)
{
    comments.clear();
    aspect_kinds.clear();
    std::optional<FiffTag> t_pTag;
    std::int32_t k;

    for (const std::shared_ptr<FiffDirNode>& ev : evoked_node) {
        for (const FiffDirEntry& entry : ev->dir) {
            if (entry.kind == FIFF_COMMENT) {
                t_pTag = this->read_tag(entry.pos);
                comments.push_back(t_pTag->to_string());
            }
        }
        const std::vector<std::shared_ptr<FiffDirNode>> aspects = ev->dir_tree_find(FIFFB_ASPECT);
        if (aspects.empty()) {
            continue;
        }
        for (const FiffDirEntry& entry : aspects.front()->dir) {
            if (entry.kind == FIFF_ASPECT_KIND) {
                t_pTag = this->read_tag(entry.pos);
                aspect_kinds.push_back(*t_pTag->to_int());
            }
        }
    }

    if (comments.size() != aspect_kinds.size() || comments.empty()) {
        Log::warning("Dataset names in FIF file could not be found.");
        return false;
    }

    t.clear();
    for (k = 0; k < std::ssize(aspect_kinds); ++k) {
        t += std::format("{} - \"{}\" (", k, comments[static_cast<std::size_t>(k)]);
        if (aspect_kinds[static_cast<std::size_t>(k)] == FIFFV_ASPECT_AVERAGE)
            t += "FIFFV_ASPECT_AVERAGE)\n";
        else if (aspect_kinds[static_cast<std::size_t>(k)] == FIFFV_ASPECT_STD_ERR)
            t += "FIFFV_ASPECT_STD_ERR)\n";
        else
            t += "unknown)\n";
    }

    return true;
}

//=============================================================================================================

bool FiffStream::open()
{
    const std::string t_sFileName = this->stream_name();
    std::optional<FiffTag> t_pTag;

    /*
     * Try to open...
     */
    if (!reopen()) {
        Log::critical("Cannot open {}\n", t_sFileName); //consider throw
        return false;
    }

    if (!(t_pTag = check_beginning())) // Supposed to get the id already in the beginning - read once approach - for TCP/IP support
        return false;

    /*
     * Read id and directory pointer
     */
    if (t_pTag->kind != FIFF_FILE_ID) {
        Log::critical("FIFF file should start with FIFF_FILE_ID!"); //consider throw
        close();
        return false;
    }
    m_id = t_pTag->to_fiff_id();

    t_pTag = this->read_tag();
    if (t_pTag->kind != FIFF_DIR_POINTER) {
        Log::warning("Fiff::open: file does have a directory pointer"); //consider throw
        close();
        return false;
    }

    //
    //   Read or create the directory tree
    //
    Log::info("Creating tag directory for {}...", t_sFileName);
    m_dir.clear();
    std::int32_t dirpos = *t_pTag->to_int();
    /*
     * Do we have a directory or not?
     */
    if (dirpos <= 0) { /* Must do it in the hard way... */
        bool ok = false;
        m_dir = this->make_dir(&ok);
        if (!ok) {
            Log::critical("Could not create tag directory!");
            return false;
        }
    } else { /* Just read the directory */
        if (t_pTag = this->read_tag(dirpos); !t_pTag) {
            Log::critical("Could not read the tag directory (file probably damaged)!");
            return false;
        }
        m_dir = t_pTag->to_dir_entry();
    }

    /*
     * Check for a mistake
     */
    if (m_dir[m_dir.size() - 2].kind == FIFF_DIR) {
        m_dir.pop_back();
        m_dir.back() = FiffDirEntry{};
    }

    //
    //   Create the directory tree structure
    //
    if ((this->m_dirtree = this->make_subtree(m_dir, 0)) == nullptr)
        return false;
    else
        this->m_dirtree->parent.reset();

    //
    //   Back to the beginning
    //
    seek(SEEK_SET); //fseek(fid,0,'bof');
    return true;
}

//=============================================================================================================

bool FiffStream::close()
{
    if (m_bytes && m_bytes->is_open())
        m_bytes->close();
    return true;
}

//=============================================================================================================

std::shared_ptr<FiffDirNode> FiffStream::make_subtree(const std::vector<FiffDirEntry>& dentry, std::ptrdiff_t current)
{
    std::shared_ptr<FiffDirNode> defaultNode;
    auto node = std::make_shared<FiffDirNode>();
    std::shared_ptr<FiffDirNode> child;
    std::optional<FiffTag> t_pTag;
    std::vector<FiffDirEntry> dir;

    node->type = FIFFB_ROOT;

    if (dentry[current].kind == FIFF_BLOCK_START) {
        if (t_pTag = this->read_tag(dentry[current].pos); !t_pTag)
            return defaultNode;
        else
            node->type = *t_pTag->to_int();
    } else {
        node->id = this->id();
    }

    ++current;
    int level = 0;
    for (; current < std::ssize(dentry); ++current) {
        if (dentry[current].kind == FIFF_BLOCK_START) {
            level++;
            if (level == 1) {
                if (child = this->make_subtree(dentry, current); !child)
                    return defaultNode;
                child->parent = node;
                node->children.push_back(child);
            }
        } else if (dentry[current].kind == FIFF_BLOCK_END) {
            level--;
            if (level < 0)
                break;
        } else if (dentry[current].kind == -1)
            break;
        else if (level == 0) {
            /*
            * Take the node id from the parent block id,
            * block id, or file id. Let the block id
            * take precedence over parent block id and file id
            */
            if (((dentry[current].kind == FIFF_PARENT_BLOCK_ID || dentry[current].kind == FIFF_FILE_ID) && node->id.empty()) || dentry[current].kind == FIFF_BLOCK_ID) {
                if (t_pTag = this->read_tag(dentry[current].pos); !t_pTag)
                    return defaultNode;
                node->id = t_pTag->to_fiff_id();
            }
            dir.push_back(dentry[current]);
        }
    }
    /*
     * Strip unused entries
     */
    node->dir = dir;
    return node;
}

//=============================================================================================================

StringList FiffStream::read_bad_channels(const std::shared_ptr<FiffDirNode>& p_Node)
{
    std::vector<std::shared_ptr<FiffDirNode>> node = p_Node->dir_tree_find(FIFFB_MNE_BAD_CHANNELS);
    std::optional<FiffTag> t_pTag;

    StringList bads;

    if (node.size() > 0)
        if (t_pTag = node[0]->find_tag(*this, FIFF_MNE_CH_NAME_LIST); t_pTag)
            bads = split_name_list(t_pTag->to_string());

    return bads;
}

//=============================================================================================================

void FiffStream::write_bad_channels(const StringList& bads)
{
    start_block(FIFFB_MNE_BAD_CHANNELS);
    write_name_list(FIFF_MNE_CH_NAME_LIST, bads);
    end_block(FIFFB_MNE_BAD_CHANNELS);
}

//=============================================================================================================

bool FiffStream::read_annotations(const std::shared_ptr<FiffDirNode>& p_Node, FiffAnnotations& annotations, fiff_int_t origTime[2])
{
    annotations.clear();
    origTime[0] = -1;
    origTime[1] = 0;
    const std::vector<std::shared_ptr<FiffDirNode>> blocks = p_Node->dir_tree_find(FIFFB_MNE_ANNOTATIONS);
    if (blocks.empty()) {
        return false;
    }
    std::vector<float> onsets;
    std::vector<float> ends;
    std::vector<std::string> descriptions;
    mne::JsonDocument channelNames;
    mne::JsonDocument extras;
    std::optional<FiffTag> tag;
    for (const FiffDirEntry& entry : blocks.front()->dir) {
        if (tag = this->read_tag(entry.pos); !tag) {
            continue;
        }
        switch (entry.kind) {
            case FIFF_MNE_BASELINE_MIN:
                onsets.assign(tag->to_float(), tag->to_float() + tag->size() / 4);
                break;
            case FIFF_MNE_BASELINE_MAX:
                ends.assign(tag->to_float(), tag->to_float() + tag->size() / 4);
                break;
            case FIFF_COMMENT:
                // Unlike channel name lists, descriptions keep spaces; colons inside them are stored as {COLON}.
                if (!tag->to_string().empty()) {
                    for (std::string& name : mne::split(tag->to_string(), ':')) {
                        for (std::size_t at = name.find("{COLON}"); at != std::string::npos; at = name.find("{COLON}", at + 1)) {
                            name.replace(at, 7, ":");
                        }
                        descriptions.push_back(std::move(name));
                    }
                }
                break;
            case FIFF_MEAS_DATE:
                if (tag->data_type() == FIFFT_DOUBLE && tag->size() >= 16) {
                    origTime[0] = static_cast<fiff_int_t>(tag->to_double()[0]);
                    origTime[1] = static_cast<fiff_int_t>(tag->to_double()[1]);
                } else if (tag->data_type() == FIFFT_DOUBLE && tag->size() == 8) {
                    const double stamp = tag->to_double()[0];
                    origTime[0] = static_cast<fiff_int_t>(std::floor(stamp));
                    origTime[1] = static_cast<fiff_int_t>(std::lround((stamp - std::floor(stamp)) * 1e6));
                }
                break;
            case FIFF_MNE_EPOCHS_DROP_LOG:
                channelNames = mne::JsonDocument::from_json(tag->to_string());
                break;
            case FIFF_FREE_LIST:
                extras = mne::JsonDocument::from_json(tag->to_string());
                break;
            default:
                break;
        }
    }
    const std::size_t count = std::min({onsets.size(), ends.size(), descriptions.size()});
    for (std::size_t i = 0; i < count; ++i) {
        annotations.push_back({static_cast<double>(onsets[i]),
                               static_cast<double>(ends[i] - onsets[i]), // float32, as MNE-Python
                               std::move(descriptions[i]),
                               names_from_json(channelNames.array().at(i).to_array()),
                               {},
                               extras_from_json(extras.array().at(i).to_object())});
    }
    return true;
}

//=============================================================================================================

fiff_long_t FiffStream::write_annotations(const FiffAnnotations& annotations, const fiff_int_t* origTime)
{
    const fiff_long_t pos = start_block(FIFFB_MNE_ANNOTATIONS);
    const auto count = static_cast<fiff_int_t>(annotations.size());
    std::vector<float> onsets;
    std::vector<float> ends;
    std::string descriptions;
    mne::JsonArray channelNames;
    mne::JsonArray extras;
    channelNames.reserve(annotations.size());
    extras.reserve(annotations.size());
    bool anyChannels = false;
    bool anyExtras = false;
    for (const FiffAnnotation& annotation : annotations) {
        if (!onsets.empty()) {
            descriptions += ':';
        }
        onsets.push_back(static_cast<float>(annotation.onset));
        ends.push_back(static_cast<float>(annotation.onset + annotation.duration));
        for (const char c : annotation.description) {
            descriptions += c == ':' ? std::string_view("{COLON}") : std::string_view(&c, 1);
        }
        channelNames.push_back(names_to_json(annotation.ch_names));
        extras.push_back(extras_to_json(annotation.extras));
        anyChannels = anyChannels || !annotation.ch_names.empty();
        anyExtras = anyExtras || !annotation.extras.empty();
    }
    write_float(FIFF_MNE_BASELINE_MIN, onsets.data(), count);
    write_float(FIFF_MNE_BASELINE_MAX, ends.data(), count);
    write_string(FIFF_COMMENT, descriptions);
    if (origTime != nullptr) {
        const double stamp[2] = {static_cast<double>(origTime[0]), static_cast<double>(origTime[1])};
        write_double(FIFF_MEAS_DATE, stamp, 2);
    }
    if (anyChannels) {
        write_string(FIFF_MNE_EPOCHS_DROP_LOG, mne::JsonDocument(std::move(channelNames)).to_json(mne::JsonDocument::Compact));
    }
    if (anyExtras) {
        write_string(FIFF_FREE_LIST, mne::JsonDocument(std::move(extras)).to_json(mne::JsonDocument::Compact));
    }
    end_block(FIFFB_MNE_ANNOTATIONS);
    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_device_info(const FiffDeviceInfo& device)
{
    if (device.empty()) {
        return -1;
    }
    const fiff_long_t pos = start_block(FIFFB_DEVICE);
    const std::pair<fiff_int_t, const std::string*> fields[] = {{FIFF_DEVICE_TYPE, &device.type},
                                                                {FIFF_DEVICE_MODEL, &device.model},
                                                                {FIFF_DEVICE_SERIAL, &device.serial},
                                                                {FIFF_DEVICE_SITE, &device.site}};
    for (const auto& [kind, value] : fields) {
        if (!value->empty()) {
            write_string(kind, *value);
        }
    }
    end_block(FIFFB_DEVICE);
    return pos;
}

//=============================================================================================================

bool FiffStream::attach_env(const std::string& workingDir, const std::string& command)
{
    int insert_blocks[] = {FIFFB_MNE, FIFFB_MEAS, FIFFB_MRI, FIFFB_BEM, -1};

    int b, k, insert;
    std::optional<FiffTag> t_pTag;

    FiffId id(FiffId::new_file_id());

    /*
     * Find an appropriate position to insert
     */
    for (insert = -1, b = 0; insert_blocks[b] >= 0; b++) {
        for (k = 0; k < nent(); k++) {
            if (dir()[k].kind == FIFF_BLOCK_START) {
                if (t_pTag = read_tag(dir()[k].pos); !t_pTag)
                    return false;
                if (*(t_pTag->to_int()) == insert_blocks[b]) {
                    insert = k;
                    break;
                }
            }
        }
        if (insert >= 0)
            break;
    }
    if (insert < 0) {
        Log::critical("Suitable place for environment insertion not found.");
        return false;
    }

    /*
     * Inline fiff_insert_after logic
     */
    int where = insert;
    if (where < 0 || where >= nent() - 1) {
        Log::critical("illegal insertion point in fiff_insert_after!");
        return false;
    }

    std::optional<FiffTag> t_pTagNext;
    const std::vector<FiffDirEntry> old_dir = dir();
    const FiffDirEntry this_ent = old_dir[static_cast<std::size_t>(where)];

    if (t_pTagNext = read_tag(this_ent.pos); !t_pTagNext)
        return false;
    /*
     * Update next info to be sequential
     */
    const fiff_long_t next_tmp = tell();
    /*
     * Go to the end of the file
     */
    seek_to_end();
    /*
     * Copy the beginning of old directory
     */
    std::vector<FiffDirEntry> new_dir(old_dir.begin(), old_dir.begin() + where + 1);

    const fiff_long_t old_end = tell();
    /*
     * Write the MNE_ENV block tags
     */
    new_dir.push_back({FIFF_BLOCK_START, FIFFT_INT, 1 * 4, static_cast<fiff_int_t>(start_block(FIFFB_MNE_ENV))});
    new_dir.push_back({FIFF_BLOCK_ID, FIFFT_ID_STRUCT, 5 * 4, static_cast<fiff_int_t>(write_id(FIFF_BLOCK_ID, id))});
    new_dir.push_back({FIFF_MNE_ENV_WORKING_DIR, FIFFT_STRING, static_cast<fiff_int_t>(workingDir.size()),
                       static_cast<fiff_int_t>(write_string(FIFF_MNE_ENV_WORKING_DIR, workingDir))});
    new_dir.push_back({FIFF_MNE_ENV_COMMAND_LINE, FIFFT_STRING, static_cast<fiff_int_t>(command.size()),
                       static_cast<fiff_int_t>(write_string(FIFF_MNE_ENV_COMMAND_LINE, command))});
    new_dir.push_back({FIFF_BLOCK_END, FIFFT_INT, 1 * 4, static_cast<fiff_int_t>(end_block(FIFFB_MNE_ENV, next_tmp))});
    new_dir.insert(new_dir.end(), old_dir.begin() + where + 1, old_dir.end());
    /*
     * Update the branching tag
     */
    t_pTagNext->next = static_cast<std::int32_t>(old_end);
    write_tag(*t_pTagNext, this_ent.pos);

    dir() = new_dir;

    return true;
}

//=============================================================================================================

bool FiffStream::read_cov(const std::shared_ptr<FiffDirNode>& p_Node, fiff_int_t cov_kind, FiffCov& p_covData)
{
    p_covData.clear();

    //
    //   Find all covariance matrices
    //
    std::vector<std::shared_ptr<FiffDirNode>> covs = p_Node->dir_tree_find(FIFFB_MNE_COV);
    if (covs.size() == 0) {
        Log::warning("No covariance matrices found");
        return false;
    }
    //
    //   Is any of the covariance matrices a noise covariance
    //
    std::int32_t p = 0;
    std::optional<FiffTag> tag;
    bool success = false;
    fiff_int_t dim, nfree, nn;
    StringList names;
    bool diagmat = false;
    VectorXd eig;
    MatrixXd eigvec;
    VectorXd cov_diag;
    MatrixXd cov;
    VectorXd cov_sparse;
    StringList bads;
    for (p = 0; p < std::ssize(covs); ++p) {
        tag = covs[p]->find_tag(*this, FIFF_MNE_COV_KIND);
        success = tag.has_value();
        if (success && *tag->to_int() == cov_kind) {
            const std::shared_ptr<FiffDirNode>& current = covs[p];
            //
            //   Find all the necessary data
            //
            if (tag = current->find_tag(*this, FIFF_MNE_COV_DIM); !tag) {
                Log::warning("Covariance matrix dimension not found.\n");
                return false;
            }
            dim = *tag->to_int();
            if (tag = current->find_tag(*this, FIFF_MNE_COV_NFREE); !tag)
                nfree = -1;
            else
                nfree = *tag->to_int();

            if (tag = current->find_tag(*this, FIFF_MNE_ROW_NAMES); tag) {
                names = FiffStream::split_name_list(tag->to_string());
                if (static_cast<int>(names.size()) != dim) {
                    Log::warning("Number of names does not match covariance matrix dimension\n");
                    return false;
                }
            }
            if (tag = current->find_tag(*this, FIFF_MNE_COV); !tag) {
                if (tag = current->find_tag(*this, FIFF_MNE_COV_DIAG); !tag) {
                    Log::warning("No covariance matrix data found\n");
                    return false;
                } else {
                    //
                    //   Diagonal is stored
                    //
                    if (tag->type == FIFFT_DOUBLE) {
                        cov_diag = Map<const VectorXd>(tag->to_double(), dim);
                    } else if (tag->type == FIFFT_FLOAT) {
                        cov_diag = Map<const VectorXf>(tag->to_float(), dim).cast<double>();
                    } else {
                        Log::critical("Illegal data type for covariance matrix\n");
                        return false;
                    }

                    diagmat = true;
                    Log::info("\t{} x {} diagonal covariance (kind = {}) found.\n", dim, dim, cov_kind);
                }
            } else {
                VectorXd vals;
                nn = dim * (dim + 1) / 2;
                if (tag->type == FIFFT_DOUBLE) {
                    vals = Map<const VectorXd>(tag->to_double(), nn);
                } else if (tag->type == FIFFT_FLOAT) {
                    vals = Map<const VectorXf>(tag->to_float(), nn).cast<double>();
                } else {
                    Log::debug("{}", tag->info());
                    return false;
                }

                if (!Numerics::issparse(vals)) {
                    //
                    //   Lower diagonal is stored
                    //
                    cov = MatrixXd::Zero(dim, dim);

                    // XXX : should remove for loops
                    std::int32_t q = 0;
                    for (std::int32_t j = 0; j < dim; ++j) {
                        for (std::int32_t k = 0; k <= j; ++k) {
                            cov(j, k) = vals(q);
                            ++q;
                        }
                    }
                    for (std::int32_t j = 0; j < dim; ++j)
                        for (std::int32_t k = j + 1; k < dim; ++k)
                            cov(j, k) = cov(k, j);

                    diagmat = false;
                    Log::info("\t{} x {} full covariance (kind = {}) found.\n", dim, dim, cov_kind);

                } else {
                    diagmat = false;
                    Log::debug("ToDo: FiffStream::read_cov - this needs to be debugged.\n");
                    cov = vals;
                    Log::info("\t{} x {} sparse covariance (kind = {}) found.\n", dim, dim, cov_kind);
                }
                //MATLAB
                //                    if ~issparse(tag.data)
                //                        //
                //                        //   Lower diagonal is stored
                //                        //
                //                        vals = tag.data;
                //                        data = zeros(dim,dim);
                //                        % XXX : should remove for loops
                //                        q = 1;
                //                        for j = 1:dim
                //                            for k = 1:j
                //                                data(j,k) = vals(q);
                //                                q = q + 1;
                //                            end
                //                        end
                //                        for j = 1:dim
                //                            for k = j+1:dim
                //                                data(j,k) = data(k,j);
                //                            end
                //                        end
                //                        diagmat = false;
                //                        fprintf('\t%d x %d full covariance (kind = %d) found.\n',dim,dim,cov_kind);
                //                    else
                //                        diagmat = false;
                //                        data = tag.data;
                //                        fprintf('\t%d x %d sparse covariance (kind = %d) found.\n',dim,dim,cov_kind);
                //                    end
                //MATLAB END
            }
            //
            //   Read the possibly precomputed decomposition
            //
            const auto tag1 = current->find_tag(*this, FIFF_MNE_COV_EIGENVALUES);
            const auto tag2 = tag1 ? current->find_tag(*this, FIFF_MNE_COV_EIGENVECTORS) : std::nullopt;
            if (tag1 && tag2) {
                eig = VectorXd(Map<const VectorXd>(tag1->to_double(), dim));
                eigvec = tag2->to_float_matrix().cast<double>();
                eigvec.transposeInPlace();
            }
            //
            //   Read the projection operator
            //
            std::vector<FiffProj> projs = this->read_proj(current);
            //
            //   Read the bad channel list
            //
            bads = this->read_bad_channels(current);
            //
            //   Put it together
            //
            p_covData.clear();

            p_covData.kind = cov_kind;
            p_covData.diag = diagmat;
            p_covData.dim = dim;
            p_covData.names = names;

            if (cov_diag.size() > 0)
                p_covData.data = cov_diag;
            else if (cov.size() > 0)
                p_covData.data = cov;
            else if (cov_sparse.size() > 0)
                p_covData.data = cov_sparse;

            p_covData.projs = projs;
            p_covData.bads = bads;
            p_covData.nfree = nfree;
            p_covData.eig = eig;
            p_covData.eigvec = eigvec;

            //
            return true;
        }
    }

    Log::info("Did not find the desired covariance matrix\n");
    return false;
}

//=============================================================================================================

std::vector<FiffCtfComp> FiffStream::read_ctf_comp(const std::shared_ptr<FiffDirNode>& p_Node, const std::vector<FiffChInfo>& p_Chs)
{
    std::vector<FiffCtfComp> compdata;
    std::vector<std::shared_ptr<FiffDirNode>> t_qListComps = p_Node->dir_tree_find(FIFFB_MNE_CTF_COMP_DATA);

    std::int32_t k, p;
    fiff_int_t kind, pos;
    std::optional<FiffTag> t_pTag;
    for (k = 0; k < std::ssize(t_qListComps); ++k) {
        const std::shared_ptr<FiffDirNode>& node = t_qListComps[k];
        //
        //   Read the data we need
        //
        FiffNamedMatrix mat;
        this->read_named_matrix(node, FIFF_MNE_CTF_COMP_DATA, mat);
        for (p = 0; p < node->nent(); ++p) {
            kind = node->dir[p].kind;
            pos = node->dir[p].pos;
            if (kind == FIFF_MNE_CTF_COMP_KIND) {
                t_pTag = this->read_tag(pos);
                break;
            }
        }
        if (!t_pTag) {
            Log::warning("Compensation type not found\n");
            return {};
        }
        //
        //   Get the compensation kind and map it to a simple number
        //
        FiffCtfComp one;
        one.ctfkind = *t_pTag->to_int();

        t_pTag.reset();

        one.kind = -1;
        if (one.ctfkind == 1194410578) //hex2dec('47314252')
            one.kind = 1;
        else if (one.ctfkind == 1194476114) //hex2dec('47324252')
            one.kind = 2;
        else if (one.ctfkind == 1194541650) //hex2dec('47334252')
            one.kind = 3;
        else if (one.ctfkind == 1194479433)
            one.kind = 4;
        else if (one.ctfkind == 1194544969)
            one.kind = 5;
        else
            one.kind = one.ctfkind;

        for (p = 0; p < node->nent(); ++p) {
            kind = node->dir[p].kind;
            pos = node->dir[p].pos;
            if (kind == FIFF_MNE_CTF_COMP_CALIBRATED) {
                t_pTag = this->read_tag(pos);
                break;
            }
        }
        bool calibrated;
        if (!t_pTag)
            calibrated = false;
        else
            calibrated = static_cast<bool>(*t_pTag->to_int());

        one.save_calibrated = calibrated;
        one.data = std::move(mat);
        if (!one.calibrate(p_Chs))
            return {};
        compdata.push_back(one);
    }

    if (compdata.size() > 0)
        Log::info("\tRead {} compensation matrices\n", compdata.size());

    return compdata;
}

//=============================================================================================================

mne::Expected<FiffDigitizerData> FiffStream::read_digitizer_data(const std::shared_ptr<FiffDirNode>& node)
{
    FiffDigitizerData dig;
    std::vector<std::shared_ptr<FiffDirNode>> blocks = node->dir_tree_find(FIFFB_ISOTRAK);
    dig.coord_frame = FIFFV_COORD_HEAD;
    if (blocks.empty()) {
        blocks = node->dir_tree_find(FIFFB_MRI_SET);
        dig.coord_frame = FIFFV_COORD_MRI;
    }
    if (!blocks.empty()) {
        for (const FiffDirEntry& entry : blocks.front()->dir) {
            if (entry.kind != FIFF_DIG_POINT && entry.kind != FIFF_MNE_COORD_FRAME) {
                continue;
            }
            const std::optional<FiffTag> tag = this->read_tag(entry.pos);
            if (!tag) {
                return mne::fail(mne::ErrorCode::Format, "cannot read the digitizer tag at {} of {}", entry.pos, this->stream_name());
            }
            if (entry.kind == FIFF_DIG_POINT) {
                dig.points.push_back(tag->to_dig_point());
            } else {
                dig.coord_frame = *tag->to_int();
            }
        }
    }
    if (dig.points.empty()) {
        return mne::fail(mne::ErrorCode::NotFound, "no digitizer points in {}", this->stream_name());
    }
    for (FiffDigPoint& point : dig.points) {
        point.coord_frame = dig.coord_frame;
    }
    dig.filename = mne::path_from_utf8(this->stream_name());
    dig.active.assign(dig.points.size(), true);
    dig.discard.assign(dig.points.size(), false);
    return dig;
}

//=============================================================================================================

bool FiffStream::read_meas_info_base(const std::shared_ptr<FiffDirNode>& p_Node, FiffInfoBase& p_InfoForward)
{
    p_InfoForward.clear();

    //
    //   Find the desired blocks
    //
    std::vector<std::shared_ptr<FiffDirNode>> parent_meg = p_Node->dir_tree_find(FIFFB_MNE_PARENT_MEAS_FILE);

    if (parent_meg.size() == 0) {
        Log::warning("No parent MEG information found in operator\n");
        return false;
    }

    std::optional<FiffTag> t_pTag;

    std::vector<FiffChInfo> chs;
    FiffCoordTrans cand;
    fiff_int_t kind = -1;
    fiff_int_t pos = -1;

    for (std::int32_t k = 0; k < parent_meg[0]->nent(); ++k) {
        kind = parent_meg[0]->dir[k].kind;
        pos = parent_meg[0]->dir[k].pos;
        if (kind == FIFF_CH_INFO) {
            t_pTag = this->read_tag(pos);
            chs.push_back(t_pTag->to_ch_info());
        }
    }

    //
    //   Add the channel information and make a list of channel names
    //   for convenience
    //
    p_InfoForward.chs = chs;
    for (std::int32_t c = 0; c < std::ssize(p_InfoForward.chs); ++c)
        p_InfoForward.ch_names.push_back(p_InfoForward.chs[c].ch_name);

    p_InfoForward.nchan = static_cast<int>(chs.size());

    //
    //   Get the MEG device <-> head coordinate transformation
    //
    if (t_pTag = parent_meg[0]->find_tag(*this, FIFF_COORD_TRANS); t_pTag) {
        cand = t_pTag->to_coord_trans();
        if (cand.from == FIFFV_COORD_DEVICE && cand.to == FIFFV_COORD_HEAD)
            p_InfoForward.dev_head_t = cand;
        else if (cand.from == FIFFV_MNE_COORD_CTF_HEAD && cand.to == FIFFV_COORD_HEAD)
            p_InfoForward.ctf_head_t = cand;
        else
            Log::warning("MEG device/head coordinate transformation not found");
    } else
        Log::warning("MEG/head coordinate transformation not found.\n");

    //
    //   Load the bad channel list
    //
    p_InfoForward.bads = this->read_bad_channels(p_Node);

    return true;
}

//=============================================================================================================

bool FiffStream::read_meas_info(const std::shared_ptr<FiffDirNode>& p_Node, FiffInfo& info, std::shared_ptr<FiffDirNode>& p_NodeInfo)
{
    //    if (info)
    //        delete info;
    info.clear();
    //
    //   Find the desired blocks
    //
    std::vector<std::shared_ptr<FiffDirNode>> meas = p_Node->dir_tree_find(FIFFB_MEAS);

    if (meas.size() == 0) {
        Log::warning("Could not find measurement data\n");
        return false;
    }
    //
    std::vector<std::shared_ptr<FiffDirNode>> meas_info = meas[0]->dir_tree_find(FIFFB_MEAS_INFO);
    if (meas_info.size() == 0) {
        Log::warning("Could not find measurement info\n");
        //        delete meas[0];
        return false;
    }
    //
    //   Read measurement info
    //
    std::optional<FiffTag> t_pTag;

    fiff_int_t nchan = -1;
    float sfreq = -1.0f;
    float linefreq = -1.0f;
    std::vector<FiffChInfo> chs;
    float lowpass = -1.0f;
    float highpass = -1.0f;

    int proj_id = 0;
    std::string proj_name;
    std::string xplotter_layout;

    std::string utc_offset;
    fiff_int_t gantry_angle = -1;

    std::string experimenter;
    std::string description;

    FiffChInfo t_chInfo;

    FiffCoordTrans cand;       // = nullptr;
    FiffCoordTrans dev_head_t; // = nullptr;
    FiffCoordTrans ctf_head_t; // = nullptr;
    std::vector<FiffCoordTrans> all_coord_trans;

    fiff_int_t meas_date[2];
    meas_date[0] = -1;
    meas_date[1] = -1;

    fiff_int_t kind = -1;
    fiff_int_t pos = -1;

    for (std::int32_t k = 0; k < meas_info[0]->nent(); ++k) {
        kind = meas_info[0]->dir[k].kind;
        pos = meas_info[0]->dir[k].pos;
        switch (kind) {
            case FIFF_NCHAN:
                t_pTag = this->read_tag(pos);
                nchan = *t_pTag->to_int();
                break;
            case FIFF_SFREQ:
                t_pTag = this->read_tag(pos);
                sfreq = *t_pTag->to_float();
                break;
            case FIFF_LINE_FREQ:
                t_pTag = this->read_tag(pos);
                linefreq = *t_pTag->to_float();
                break;
            case FIFF_CH_INFO:
                t_pTag = this->read_tag(pos);
                chs.push_back(t_pTag->to_ch_info());
                break;
            case FIFF_LOWPASS:
                t_pTag = this->read_tag(pos);
                lowpass = *t_pTag->to_float();
                break;
            case FIFF_HIGHPASS:
                t_pTag = this->read_tag(pos);
                highpass = *t_pTag->to_float();
                break;
            case FIFF_MEAS_DATE:
                t_pTag = this->read_tag(pos);
                meas_date[0] = t_pTag->to_int()[0];
                meas_date[1] = t_pTag->to_int()[1];
                break;
            case FIFF_COORD_TRANS:
                //ToDo: This has to be debugged!!
                t_pTag = this->read_tag(pos);
                cand = t_pTag->to_coord_trans();
                all_coord_trans.push_back(cand);
                if (cand.from == FIFFV_COORD_DEVICE && cand.to == FIFFV_COORD_HEAD)
                    dev_head_t = cand;
                else if (cand.from == FIFFV_MNE_COORD_CTF_HEAD && cand.to == FIFFV_COORD_HEAD)
                    ctf_head_t = cand;
                break;
            case FIFF_PROJ_ID:
                t_pTag = this->read_tag(pos);
                proj_id = *t_pTag->to_int();
                break;
            case FIFF_PROJ_NAME:
                t_pTag = this->read_tag(pos);
                proj_name = t_pTag->to_string();
                break;
            case FIFF_XPLOTTER_LAYOUT:
                t_pTag = this->read_tag(pos);
                xplotter_layout = t_pTag->to_string();
                break;
            case FIFF_EXPERIMENTER:
                t_pTag = this->read_tag(pos);
                experimenter = t_pTag->to_string();
                break;
            case FIFF_DESCRIPTION:
                t_pTag = this->read_tag(pos);
                description = t_pTag->to_string();
                break;
            case FIFF_GANTRY_ANGLE:
                t_pTag = this->read_tag(pos);
                // MNE-Python writes this as float; older writers used int.
                if (t_pTag->data_type() == FIFFT_FLOAT) {
                    gantry_angle = static_cast<fiff_int_t>(*t_pTag->to_float());
                } else if (t_pTag->data_type() == FIFFT_INT) {
                    gantry_angle = *t_pTag->to_int();
                }
                break;
            case FIFF_UTC_OFFSET:
                t_pTag = this->read_tag(pos);
                utc_offset = t_pTag->to_string();
                break;
        }
    }
    //
    //   Check that we have everything we need
    //
    if (nchan < 0) {
        Log::warning("Number of channels in not defined\n");
        return false;
    }
    if (sfreq < 0) {
        Log::warning("Sampling frequency is not defined\n");
        return false;
    }
    if (linefreq < 0) {
        Log::warning("Line frequency is not defined\n");
    }
    if (chs.size() == 0) {
        Log::warning("Channel information not defined\n");
        return false;
    }
    if (std::ssize(chs) != nchan) {
        Log::warning("Incorrect number of channel definitions found\n");
        return false;
    }

    if (dev_head_t.empty() || ctf_head_t.empty()) {
        std::vector<std::shared_ptr<FiffDirNode>> hpi_result = meas_info[0]->dir_tree_find(FIFFB_HPI_RESULT);
        if (hpi_result.size() == 1) {
            for (std::int32_t k = 0; k < hpi_result[0]->nent(); ++k) {
                kind = hpi_result[0]->dir[k].kind;
                pos = hpi_result[0]->dir[k].pos;
                if (kind == FIFF_COORD_TRANS) {
                    t_pTag = this->read_tag(pos);
                    cand = t_pTag->to_coord_trans();
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
    std::vector<std::shared_ptr<FiffDirNode>> isotrak = meas_info[0]->dir_tree_find(FIFFB_ISOTRAK);

    FiffDigPointSet dig;
    fiff_int_t coord_frame = FIFFV_COORD_HEAD;
    FiffCoordTrans dig_trans;
    std::int32_t k = 0;

    if (isotrak.size() == 1) {
        for (k = 0; k < isotrak[0]->nent(); ++k) {
            kind = isotrak[0]->dir[k].kind;
            pos = isotrak[0]->dir[k].pos;
            if (kind == FIFF_DIG_POINT) {
                t_pTag = this->read_tag(pos);
                dig.push_back(t_pTag->to_dig_point());
            } else {
                if (kind == FIFF_MNE_COORD_FRAME) {
                    t_pTag = this->read_tag(pos);
                    coord_frame = *t_pTag->to_int();
                } else if (kind == FIFF_COORD_TRANS) {
                    t_pTag = this->read_tag(pos);
                    dig_trans = t_pTag->to_coord_trans();
                }
            }
        }
    }
    for (FiffDigPoint& point : dig)
        point.coord_frame = coord_frame;

    if (!dig_trans.empty()) //if exist('dig_trans','var')
        if (dig_trans.from != coord_frame && dig_trans.to != coord_frame)
            dig_trans.clear();

    //
    //   Locate the acquisition information
    //
    std::vector<std::shared_ptr<FiffDirNode>> acqpars = meas_info[0]->dir_tree_find(FIFFB_DACQ_PARS);
    std::string acq_pars;
    std::string acq_stim;
    if (acqpars.size() == 1) {
        for (k = 0; k < acqpars[0]->nent(); ++k) {
            kind = acqpars[0]->dir[k].kind;
            pos = acqpars[0]->dir[k].pos;
            if (kind == FIFF_DACQ_PARS) {
                t_pTag = this->read_tag(pos);
                acq_pars = t_pTag->to_string();
            } else if (kind == FIFF_DACQ_STIM) {
                t_pTag = this->read_tag(pos);
                acq_stim = t_pTag->to_string();
            }
        }
    }
    std::vector<std::shared_ptr<FiffDirNode>> device = meas_info[0]->dir_tree_find(FIFFB_DEVICE);
    FiffDeviceInfo device_info;
    if (device.size() == 1) {
        for (k = 0; k < device[0]->nent(); ++k) {
            kind = device[0]->dir[k].kind;
            std::string* field = kind == FIFF_DEVICE_TYPE ? &device_info.type
                : kind == FIFF_DEVICE_MODEL               ? &device_info.model
                : kind == FIFF_DEVICE_SERIAL              ? &device_info.serial
                : kind == FIFF_DEVICE_SITE                ? &device_info.site
                                                          : nullptr;
            if (field == nullptr) {
                continue;
            }
            if (const auto tag = this->read_tag(device[0]->dir[k].pos)) {
                *field = tag->to_string();
            }
        }
    }
    //
    //   Load the SSP data
    //
    std::vector<FiffProj> projs = this->read_proj(meas_info[0]); //ToDo Member Function
    //
    //   Load the CTF compensation data
    //
    std::vector<FiffCtfComp> comps = this->read_ctf_comp(meas_info[0], chs); //ToDo Member Function
    //
    //   Load the bad channel list
    //
    StringList bads = this->read_bad_channels(p_Node);
    //
    //   Put the data together
    //
    //    info = new FiffInfo();
    if (p_Node->id.version != -1)
        info.file_id = p_Node->id;
    else
        info.file_id.version = -1;

    //
    //  Make the most appropriate selection for the measurement id
    //
    if (meas_info[0]->parent_id.version == -1) {
        if (meas_info[0]->id.version == -1) {
            if (meas[0]->id.version == -1) {
                if (meas[0]->parent_id.version == -1)
                    info.meas_id = info.file_id;
                else
                    info.meas_id = meas[0]->parent_id;
            } else
                info.meas_id = meas[0]->id;
        } else
            info.meas_id = meas_info[0]->id;
    } else
        info.meas_id = meas_info[0]->parent_id;

    if (meas_date[0] == -1) {
        info.meas_date[0] = info.meas_id.time.secs;
        info.meas_date[1] = info.meas_id.time.usecs;
    } else {
        info.meas_date[0] = meas_date[0];
        info.meas_date[1] = meas_date[1];
    }

    info.nchan = nchan;
    info.sfreq = sfreq;
    info.linefreq = linefreq;

    if (highpass != -1.0f)
        info.highpass = highpass;
    else
        info.highpass = 0.0f;

    if (lowpass != -1.0f)
        info.lowpass = lowpass;
    else
        info.lowpass = info.sfreq / 2.0;

    //
    //   Add the channel information and make a list of channel names
    //   for convenience
    //
    info.chs = chs;
    for (std::int32_t c = 0; c < info.nchan; ++c)
        info.ch_names.push_back(info.chs[c].ch_name);

    //
    //  Add the coordinate transformations
    //
    info.dev_head_t = dev_head_t;
    info.ctf_head_t = ctf_head_t;
    info.all_coord_trans = all_coord_trans;
    if (!info.dev_head_t.empty() && !info.ctf_head_t.empty())
        info.dev_ctf_t = FiffCoordTrans(FIFFV_COORD_DEVICE, FIFFV_MNE_COORD_CTF_HEAD, ctf_head_t.invtrans * dev_head_t.trans);
    else
        info.dev_ctf_t.clear();

    //
    //   All kinds of auxliary stuff
    //
    info.dig = std::move(dig);
    if (!dig_trans.empty())
        info.dig_trans = dig_trans;

    info.experimenter = experimenter;
    info.description = description;
    info.proj_id = proj_id;
    info.proj_name = proj_name;
    info.xplotter_layout = xplotter_layout;
    info.gantry_angle = gantry_angle;
    info.utc_offset = utc_offset;

    info.bads = bads;
    info.projs = projs;
    info.comps = comps;
    info.acq_pars = acq_pars;
    info.acq_stim = acq_stim;
    info.device_info = device_info;

    //
    //   Locate the HPI coil excitation frequencies
    //
    //   They live in one FIFFB_HPI_COIL block per coil inside the
    //   FIFFB_HPI_MEAS block. Each coil block carries a FIFF_HPI_COIL_FREQ tag.
    //   Files without an HPI measurement simply leave the list empty.
    //
    std::vector<std::shared_ptr<FiffDirNode>> hpi_meas = meas_info[0]->dir_tree_find(FIFFB_HPI_MEAS);
    if (!hpi_meas.empty()) {
        std::vector<std::shared_ptr<FiffDirNode>> hpi_coils = hpi_meas[0]->dir_tree_find(FIFFB_HPI_COIL);

        for (const auto& hpi_coil : hpi_coils) {
            for (std::int32_t e = 0; e < hpi_coil->nent(); ++e) {
                if (hpi_coil->dir[e].kind != FIFF_HPI_COIL_FREQ) {
                    continue;
                }

                t_pTag = this->read_tag(hpi_coil->dir[e].pos);
                info.hpi_coil_freqs.push_back(*t_pTag->to_float());
                break;
            }
        }
    }

    p_NodeInfo = meas[0];

    return true;
}

//=============================================================================================================

bool FiffStream::read_named_matrix(const std::shared_ptr<FiffDirNode>& p_Node, fiff_int_t matkind, FiffNamedMatrix& mat)
{
    mat.clear();

    std::shared_ptr<FiffDirNode> node = p_Node;
    //
    //   Descend one level if necessary
    //
    bool found_it = false;
    if (node->type != FIFFB_MNE_NAMED_MATRIX) {
        for (int k = 0; k < node->nchild(); ++k) {
            if (node->children[k]->type == FIFFB_MNE_NAMED_MATRIX) {
                if (node->children[k]->has_tag(matkind)) {
                    node = node->children[k];
                    found_it = true;
                    break;
                }
            }
        }
        if (!found_it) {
            Log::warning("Fiff::read_named_matrix: Desired named matrix (kind = {}) not available\n", matkind);
            return false;
        }
    } else {
        if (!node->has_tag(matkind)) {
            Log::warning("Desired named matrix (kind = {}) not available", matkind);
            return false;
        }
    }

    std::optional<FiffTag> t_pTag;
    //
    //   Read everything we need
    //
    if (t_pTag = node->find_tag(*this, matkind); !t_pTag) {
        Log::warning("Matrix data missing.\n");
        return false;
    } else {
        // FIFF stores matrices row by row; transpose while widening, in one pass
        const MatrixXf stored = t_pTag->to_float_matrix();
        mat.data = stored.transpose().cast<double>();
    }

    mat.nrow = mat.data.rows();
    mat.ncol = mat.data.cols();

    if (t_pTag = node->find_tag(*this, FIFF_MNE_NROW); t_pTag)
        if (*t_pTag->to_int() != mat.nrow) {
            Log::warning("Number of rows in matrix data and FIFF_MNE_NROW tag do not match");
            return false;
        }
    if (t_pTag = node->find_tag(*this, FIFF_MNE_NCOL); t_pTag)
        if (*t_pTag->to_int() != mat.ncol) {
            Log::warning("Number of columns in matrix data and FIFF_MNE_NCOL tag do not match");
            return false;
        }

    std::string row_names;
    if (t_pTag = node->find_tag(*this, FIFF_MNE_ROW_NAMES); t_pTag)
        row_names = t_pTag->to_string();

    std::string col_names;
    if (t_pTag = node->find_tag(*this, FIFF_MNE_COL_NAMES); t_pTag)
        col_names = t_pTag->to_string();

    //
    //   Put it together
    //
    if (!row_names.empty())
        mat.row_names = split_name_list(row_names);

    if (!col_names.empty())
        mat.col_names = split_name_list(col_names);

    if (static_cast<int>(mat.row_names.size()) != mat.nrow) {
        Log::warning("FiffStream::read_named_matrix - Number of rows in matrix data and row names do not match\n");
    }

    if (static_cast<int>(mat.col_names.size()) != mat.ncol) {
        Log::warning("FiffStream::read_named_matrix - Number of columns in matrix data and column names do not match\n");
    }

    return true;
}

//=============================================================================================================

std::vector<FiffProj> FiffStream::read_proj(const std::shared_ptr<FiffDirNode>& p_Node)
{
    std::vector<FiffProj> projdata; // = struct('kind',{},'active',{},'desc',{},'data',{});
    //
    //   Locate the projection data
    //
    std::vector<std::shared_ptr<FiffDirNode>> t_qListNodes = p_Node->dir_tree_find(FIFFB_PROJ);
    if (t_qListNodes.size() == 0)
        return projdata;

    std::optional<FiffTag> t_pTag;
    t_pTag = t_qListNodes[0]->find_tag(*this, FIFF_NCHAN);
    fiff_int_t global_nchan = 0;
    if (t_pTag)
        global_nchan = *t_pTag->to_int();

    fiff_int_t nchan;
    std::vector<std::shared_ptr<FiffDirNode>> t_qListItems = t_qListNodes[0]->dir_tree_find(FIFFB_PROJ_ITEM);
    for (std::int32_t i = 0; i < std::ssize(t_qListItems); ++i) {
        //
        //   Find all desired tags in one item
        //
        const std::shared_ptr<FiffDirNode>& t_pFiffDirTreeItem = t_qListItems[i];
        t_pTag = t_pFiffDirTreeItem->find_tag(*this, FIFF_NCHAN);
        if (t_pTag)
            nchan = *t_pTag->to_int();
        else
            nchan = global_nchan;

        t_pTag = t_pFiffDirTreeItem->find_tag(*this, FIFF_DESCRIPTION);
        std::string desc;
        if (t_pTag) {
            desc = t_pTag->to_string();
        } else {
            t_pTag = t_pFiffDirTreeItem->find_tag(*this, FIFF_NAME);
            if (t_pTag)
                desc = t_pTag->to_string();
            else {
                Log::warning("Projection item description missing\n");
                return projdata;
            }
        }
        //            (t_pTag = t_pFiffDirTreeItem->find_tag(this, FIFF_PROJ_ITEM_CH_NAME_LIST));
        //            QString namelist;
        //            if (t_pTag)
        //            {
        //                namelist = t_pTag->toString();
        //            }
        //            else
        //            {
        //                return projdata;
        //            }
        t_pTag = t_pFiffDirTreeItem->find_tag(*this, FIFF_PROJ_ITEM_KIND);
        fiff_int_t kind;
        if (t_pTag) {
            kind = *t_pTag->to_int();
        } else {
            Log::warning("Projection item kind missing");
            return projdata;
        }
        t_pTag = t_pFiffDirTreeItem->find_tag(*this, FIFF_PROJ_ITEM_NVEC);
        fiff_int_t nvec;
        if (t_pTag) {
            nvec = *t_pTag->to_int();
        } else {
            Log::warning("Number of projection vectors not specified\n");
            return projdata;
        }
        t_pTag = t_pFiffDirTreeItem->find_tag(*this, FIFF_PROJ_ITEM_CH_NAME_LIST);
        StringList names;
        if (t_pTag) {
            names = split_name_list(t_pTag->to_string());
        } else {
            Log::warning("Projection item channel list missing\n");
            return projdata;
        }
        t_pTag = t_pFiffDirTreeItem->find_tag(*this, FIFF_PROJ_ITEM_VECTORS);
        MatrixXd data; // = nullptr;
        if (t_pTag) {
            data = t_pTag->to_float_matrix().cast<double>();
            data.transposeInPlace();
        } else {
            Log::warning("Projection item data missing\n");
            return projdata;
        }
        t_pTag = t_pFiffDirTreeItem->find_tag(*this, FIFF_MNE_PROJ_ITEM_ACTIVE);
        bool active;
        if (t_pTag)
            active = *t_pTag->to_int();
        else
            active = false;

        if (data.cols() != static_cast<int>(names.size())) {
            Log::warning("Number of channel names does not match the size of data matrix\n");
            return projdata;
        }

        //
        //   create a named matrix for the data
        //
        StringList defaultList;
        FiffNamedMatrix t_fiffNamedMatrix(nvec, nchan, defaultList, names, data);

        FiffProj one(kind, active, std::move(desc), t_fiffNamedMatrix);
        //
        projdata.push_back(one);
    }

    if (projdata.size() > 0) {
        Log::info("\tRead a total of {} projection items:\n", projdata.size());
        for (std::int32_t k = 0; k < std::ssize(projdata); ++k) {
            Log::info("\t\t{} ({} x {}) {}\n", projdata[k].desc, projdata[k].data.nrow, projdata[k].data.ncol, projdata[k].active ? "active" : "idle");
        }
    }

    return projdata;
}

//=============================================================================================================

bool FiffStream::read_tag_data(FiffTag& tag, fiff_long_t pos)
{
    if (pos >= 0) {
        seek(pos);
    }
    if (tag.size() > 0) {
        read_raw(tag.data(), tag.size());
        FiffTag::convert_tag_data(tag, FIFFV_BIG_ENDIAN, FIFFV_NATIVE_ENDIAN);
    }
    if (tag.next != FIFFV_NEXT_SEQ) {
        seek(tag.next);
    }
    return !std::exchange(m_readFailed, false);
}

//=============================================================================================================

std::optional<FiffTag> FiffStream::read_tag_info(bool skip)
{
    FiffTag tag;
    read(tag.kind);
    read(tag.type);
    std::int32_t size = 0;
    read(size);
    read(tag.next);
    if (std::exchange(m_readFailed, false)) {
        return std::nullopt;
    }
    tag.resize(size);

    if (skip) {
        if (tag.next > 0) {
            if (!seek(tag.next)) {
                return std::nullopt;
            }
        } else if (tag.size() > 0 && tag.next == FIFFV_NEXT_SEQ) {
            if (!seek(tell() + tag.size())) {
                return std::nullopt;
            }
        }
    }
    return tag;
}

//=============================================================================================================

std::optional<FiffTag> FiffStream::read_tag(fiff_long_t pos)
{
    if (pos >= 0) {
        seek(pos);
    }
    FiffTag tag;
    read(tag.kind);
    read(tag.type);
    std::int32_t size = 0;
    read(size);
    read(tag.next);
    if (std::exchange(m_readFailed, false) || size < 0) {
        return std::nullopt;
    }
    tag.resize(size);
    if (size > 0) {
        read_raw(tag.data(), size);
        FiffTag::convert_tag_data(tag, m_littleEndian ? FIFFV_LITTLE_ENDIAN : FIFFV_BIG_ENDIAN, FIFFV_NATIVE_ENDIAN);
    }
    if (tag.next != FIFFV_NEXT_SEQ) {
        seek(tag.next);
    }
    if (std::exchange(m_readFailed, false)) {
        return std::nullopt;
    }
    return tag;
}

//=============================================================================================================

StringList FiffStream::split_name_list(std::string_view p_sNameList)
{
    return mne::split(p_sNameList, ':', true);
}

//=============================================================================================================

fiff_long_t FiffStream::start_block(fiff_int_t kind)
{
    return this->write_int(FIFF_BLOCK_START, &kind);
}

//=============================================================================================================

std::unique_ptr<FiffStream> FiffStream::start_file(std::unique_ptr<mne::ByteStream> bytes)
{
    return bytes ? start_file(std::make_unique<FiffStream>(std::move(bytes))) : nullptr;
}

//=============================================================================================================

std::unique_ptr<FiffStream> FiffStream::open_update(const std::filesystem::path& path)
{
    const std::string name = mne::utf8_from_path(path);
    auto bytes = mne::FileByteStream::open(path, mne::FileByteStream::Mode::ReadWrite);
    if (!bytes) {
        Log::critical("Cannot open {} for update", name);
        return nullptr;
    }
    mne::FileByteStream& file = *bytes;
    auto stream = std::make_unique<FiffStream>(std::move(bytes), name);
    if (!stream->open()) {
        return nullptr;
    }

    // The last tag ends the file, so tags appended later must be chained to it explicitly
    fiff_long_t pointerpos = stream->dir()[stream->nent() - 2].pos;
    std::optional<FiffTag> tag = stream->read_tag(pointerpos);
    if (!tag) {
        Log::critical("Could not read last tag in the directory list!");
        return nullptr;
    }
    if (tag->next != FIFFV_NEXT_NONE) {
        tag->next = FIFFV_NEXT_NONE;
        stream->write_tag(*tag, pointerpos);
    }

    // A directory would be outdated by the update: drop it and cut it off the file
    pointerpos = stream->dir()[1].pos;
    if (tag = stream->read_tag(pointerpos); !tag) {
        Log::critical("Could not read directory pointer!");
        return nullptr;
    }
    if (const fiff_int_t dirpos = *tag->to_int(); dirpos > 0) {
        stream->write_dir_pointer(-1, pointerpos);
        if (!file.resize(static_cast<std::uint64_t>(dirpos))) {
            Log::critical("Cannot remove the directory of {}", name);
            return nullptr;
        }
    }
    stream->seek_to_end();
    return stream;
}

//=============================================================================================================

std::unique_ptr<FiffStream> FiffStream::start_writing_raw(const std::filesystem::path& path,
                                                          const FiffInfo& info,
                                                          RowVectorXd& cals,
                                                          MatrixXi sel,
                                                          bool bResetRange,
                                                          fiff_int_t dataType)
{
    return start_writing_raw(start_file(path), info, cals, std::move(sel), bResetRange, dataType);
}

//=============================================================================================================

std::unique_ptr<FiffStream> FiffStream::start_writing_raw(std::unique_ptr<mne::ByteStream> bytes,
                                                          const FiffInfo& info,
                                                          RowVectorXd& cals,
                                                          MatrixXi sel,
                                                          bool bResetRange,
                                                          fiff_int_t dataType)
{
    if (!bytes)
        return {};
    return start_writing_raw(start_file(std::make_unique<FiffStream>(std::move(bytes))), info, cals, std::move(sel), bResetRange, dataType);
}

//=============================================================================================================

std::unique_ptr<FiffStream> FiffStream::start_writing_raw(std::unique_ptr<FiffStream> t_pStream,
                                                          const FiffInfo& info,
                                                          RowVectorXd& cals,
                                                          MatrixXi sel,
                                                          bool bResetRange,
                                                          fiff_int_t dataType)
{
    fiff_int_t data_type = dataType;

    // The channel list is authoritative: nchan is -1 until a reader sets it
    if (sel.cols() == 0) {
        sel.resize(1, info.chs.size());
        for (std::int32_t k = 0; k < std::ssize(info.chs); ++k)
            sel(0, k) = k;
    }

    std::vector<FiffChInfo> chs;

    chs.reserve(static_cast<std::size_t>(sel.cols()));
    for (std::int32_t k = 0; k < sel.cols(); ++k)
        chs.push_back(info.chs.at(sel(0, k)));

    fiff_int_t nchan = static_cast<int>(chs.size());

    if (!t_pStream) {
        return t_pStream;
    }
    t_pStream->start_block(FIFFB_MEAS); //4
    t_pStream->write_id(FIFF_BLOCK_ID); //5
    if (info.meas_id.version != -1) {
        t_pStream->write_id(FIFF_PARENT_BLOCK_ID, info.meas_id); //6
    }
    //
    //
    //    Measurement info
    //
    t_pStream->start_block(FIFFB_MEAS_INFO); //7

#ifndef WASMBUILD
    //
    //    Blocks from the original
    //
    std::vector<fiff_int_t> blocks{FIFFB_SUBJECT, FIFFB_HPI_MEAS, FIFFB_HPI_RESULT, FIFFB_HPI_SUBSYSTEM, FIFFB_ISOTRAK, FIFFB_PROCESSING_HISTORY, FIFFB_DACQ_PARS, FIFFB_EVENTS};
    bool have_hpi_result = false;
    bool have_isotrak = false;
    if (blocks.size() > 0 && !info.filename.empty()) {
        FiffStream source(mne::path_from_utf8(info.filename));

        if (!source.open()) {
            Log::warning("Cannot copy the measurement blocks of {} ; writing the info without them", info.filename);
            blocks.clear();
        }

        for (const fiff_int_t block : blocks) {
            std::vector<std::shared_ptr<FiffDirNode>> nodes = source.dirtree()->dir_tree_find(block);
            FiffDirNode::copy_tree(source, source.dirtree()->id, nodes, *t_pStream);
            if (block == FIFFB_HPI_RESULT && nodes.size() > 0)
                have_hpi_result = true;

            if (block == FIFFB_ISOTRAK && nodes.size() > 0)
                have_isotrak = true;
        }
    }
#endif

    //
    //    megacq parameters
    //
    if (!info.acq_pars.empty() || !info.acq_stim.empty()) {
        t_pStream->start_block(FIFFB_DACQ_PARS);
        if (!info.acq_pars.empty())
            t_pStream->write_string(FIFF_DACQ_PARS, info.acq_pars);

        if (!info.acq_stim.empty())
            t_pStream->write_string(FIFF_DACQ_STIM, info.acq_stim);

        t_pStream->end_block(FIFFB_DACQ_PARS);
    }
    t_pStream->write_device_info(info.device_info);

#ifndef WASMBUILD
    //
    //    Coordinate transformations if the HPI result block was not there
    //
    if (!have_hpi_result) {
        if (!info.dev_head_t.empty())
            t_pStream->write_coord_trans(info.dev_head_t);

        if (!info.ctf_head_t.empty())
            t_pStream->write_coord_trans(info.ctf_head_t);
    }
    //
    //    Polhemus data
    //
    if (info.dig.size() > 0 && !have_isotrak) {
        t_pStream->start_block(FIFFB_ISOTRAK);
        for (std::int32_t k = 0; k < std::ssize(info.dig); ++k)
            t_pStream->write_dig_point(info.dig[k]);

        t_pStream->end_block(FIFFB_ISOTRAK);
    }
#endif

    //
    //    Projectors
    //
    t_pStream->write_proj(info.projs);
    //
    //    CTF compensation info
    //
    t_pStream->write_ctf_comp(info.comps);
    //
    //    Bad channels
    //
    if (info.bads.size() > 0) {
        t_pStream->start_block(FIFFB_MNE_BAD_CHANNELS);
        t_pStream->write_name_list(FIFF_MNE_CH_NAME_LIST, info.bads);
        t_pStream->end_block(FIFFB_MNE_BAD_CHANNELS);
    }
    //
    //    General
    //
    t_pStream->write_float(FIFF_SFREQ, &info.sfreq);
    t_pStream->write_float(FIFF_HIGHPASS, &info.highpass);
    t_pStream->write_float(FIFF_LOWPASS, &info.lowpass);
    if (info.linefreq >= 0.0f)
        t_pStream->write_float(FIFF_LINE_FREQ, &info.linefreq);
    t_pStream->write_int(FIFF_GANTRY_ANGLE, &info.gantry_angle);
    t_pStream->write_int(FIFF_NCHAN, &nchan);
    t_pStream->write_int(FIFF_DATA_PACK, &data_type);
    t_pStream->write_int(FIFF_PROJ_ID, &info.proj_id);
    t_pStream->write_string(FIFF_EXPERIMENTER, info.experimenter);
    t_pStream->write_string(FIFF_DESCRIPTION, info.description);
    t_pStream->write_string(FIFF_PROJ_NAME, info.proj_name);
    t_pStream->write_string(FIFF_XPLOTTER_LAYOUT, info.xplotter_layout);
    if (info.meas_date[0] != -1)
        t_pStream->write_int(FIFF_MEAS_DATE, info.meas_date, 2);
    //
    //    Channel info
    //
    cals = RowVectorXd(nchan);
    for (std::int32_t k = 0; k < nchan; ++k) {
        //
        //    Scan numbers may have been messed up
        //
        chs[k].scanNo = k + 1;
        if (bResetRange) {
            chs[k].range = 1.0; // Reset to 1.0 because we always write floats.
        }
        cals[k] = chs[k].cal;
        t_pStream->write_ch_info(chs[k]);
    }
    //
    //
    t_pStream->end_block(FIFFB_MEAS_INFO);
    //
    // Start the raw data
    //
    t_pStream->start_block(FIFFB_RAW_DATA);

    return t_pStream;
}

//=============================================================================================================

fiff_long_t FiffStream::write_tag(const FiffTag& tag, fiff_long_t pos)
{
    if (pos >= 0) {
        seek(pos);
    } else {
        seek_to_end();
    }
    pos = tell();

    const auto datasize = static_cast<fiff_int_t>(tag.size());
    write(static_cast<std::int32_t>(tag.kind));
    write(static_cast<std::int32_t>(tag.type));
    write(static_cast<std::int32_t>(datasize));
    write(static_cast<std::int32_t>(tag.next));

    if (datasize > 0) {
        // read_tag hands out host byte order; FIFF files are big-endian.
        FiffTag fileTag(tag);
        FiffTag::convert_tag_data(fileTag, FIFFV_NATIVE_ENDIAN, FIFFV_BIG_ENDIAN);
        write_raw(fileTag.data(), datasize);
    }
    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_ch_info(const FiffChInfo& ch)
{
    fiff_long_t pos = tell();

    //typedef struct _fiffChPosRec {
    //  fiff_int_t   coil_type;          /*!< What kind of coil. */
    //  fiff_float_t r0[3];              /*!< Coil coordinate system origin */
    //  fiff_float_t ex[3];              /*!< Coil coordinate system x-axis unit vector */
    //  fiff_float_t ey[3];              /*!< Coil coordinate system y-axis unit vector */
    //  fiff_float_t ez[3];             /*!< Coil coordinate system z-axis unit vector */
    //} fiffChPosRec,*fiffChPos;        /*!< Measurement channel position and coil type */

    //typedef struct _fiffChInfoRec {
    //  fiff_int_t    scanNo;        /*!< Scanning order # */
    //  fiff_int_t    logNo;         /*!< Logical channel # */
    //  fiff_int_t    kind;          /*!< Kind of channel */
    //  fiff_float_t  range;         /*!< Voltmeter range (only applies to raw data ) */
    //  fiff_float_t  cal;           /*!< Calibration from volts to... */
    //  fiff_ch_pos_t chpos;         /*!< Channel location */
    //  fiff_int_t    unit;          /*!< Unit of measurement */
    //  fiff_int_t    unit_mul;      /*!< Unit multiplier exponent */
    //  fiff_char_t   ch_name[16];   /*!< Descriptive name for the channel */
    //} fiffChInfoRec,*fiffChInfo;   /*!< Description of one channel */
    fiff_int_t datasize = 4 * 13 + 4 * 7 + 16;

    write(static_cast<std::int32_t>(FIFF_CH_INFO));
    write(static_cast<std::int32_t>(FIFFT_CH_INFO_STRUCT));
    write(static_cast<std::int32_t>(datasize));
    write(static_cast<std::int32_t>(FIFFV_NEXT_SEQ));

    //
    //   Start writing fiffChInfoRec
    //
    write(static_cast<std::int32_t>(ch.scanNo));
    write(static_cast<std::int32_t>(ch.logNo));
    write(static_cast<std::int32_t>(ch.kind));

    write(ch.range);
    write(ch.cal);

    //
    //   FiffChPos follows
    //
    write_ch_pos(ch.chpos);

    //
    //   unit and unit multiplier
    //
    write(static_cast<std::int32_t>(ch.unit));
    write(static_cast<std::int32_t>(ch.unit_mul));

    //
    //   Finally channel name
    //
    fiff_int_t len = static_cast<int>(ch.ch_name.size());
    std::string ch_name;
    if (len > 15)
        ch_name = ch.ch_name.substr(0, 15);
    else
        ch_name = ch.ch_name;

    len = static_cast<int>(ch_name.size());

    write_raw(ch_name.c_str(), len);

    if (len < 16) {
        const char* chNull = "";
        for (std::int32_t i = 0; i < 16 - len; ++i)
            write_raw(chNull, 1);
    }

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_ch_pos(const FiffChPos& chpos)
{
    fiff_long_t pos = tell();

    //
    //   FiffChPos
    //
    write(static_cast<std::int32_t>(chpos.coil_type));

    std::int32_t i;
    // r0
    for (i = 0; i < 3; ++i)
        write(chpos.r0[i]);
    // ex
    for (i = 0; i < 3; ++i)
        write(chpos.ex[i]);
    // ey
    for (i = 0; i < 3; ++i)
        write(chpos.ey[i]);
    // ez
    for (i = 0; i < 3; ++i)
        write(chpos.ez[i]);

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_coord_trans(const FiffCoordTrans& trans)
{
    fiff_long_t pos = tell();

    //?typedef struct _fiffCoordTransRec {
    //  fiff_int_t   from;                   /*!< Source coordinate system. */
    //  fiff_int_t   to;                     /*!< Destination coordinate system. */
    //  fiff_float_t rot[3][3];              /*!< The forward transform (rotation part) */
    //  fiff_float_t move[3];                /*!< The forward transform (translation part) */
    //  fiff_float_t invrot[3][3];           /*!< The inverse transform (rotation part) */
    //  fiff_float_t invmove[3];             /*!< The inverse transform (translation part) */
    //} *fiffCoordTrans, fiffCoordTransRec;  /*!< Coordinate transformation descriptor */
    fiff_int_t datasize = 4 * 2 * 12 + 4 * 2;

    write(static_cast<std::int32_t>(FIFF_COORD_TRANS));
    write(static_cast<std::int32_t>(FIFFT_COORD_TRANS_STRUCT));
    write(static_cast<std::int32_t>(datasize));
    write(static_cast<std::int32_t>(FIFFV_NEXT_SEQ));

    //
    //   Start writing fiffCoordTransRec
    //
    write(static_cast<std::int32_t>(trans.from));
    write(static_cast<std::int32_t>(trans.to));

    //
    //   The transform...
    //
    std::int32_t r, c;
    for (r = 0; r < 3; ++r)
        for (c = 0; c < 3; ++c)
            write(static_cast<float>(trans.trans(r, c)));
    for (r = 0; r < 3; ++r)
        write(static_cast<float>(trans.trans(r, 3)));

    //
    //   ...and its inverse
    //
    for (r = 0; r < 3; ++r)
        for (c = 0; c < 3; ++c)
            write(static_cast<float>(trans.invtrans(r, c)));
    for (r = 0; r < 3; ++r)
        write(static_cast<float>(trans.invtrans(r, 3)));

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_cov(const FiffCov& p_FiffCov)
{
    fiff_long_t pos = tell();

    this->start_block(FIFFB_MNE_COV);

    //
    //   Dimensions etc.
    //
    this->write_int(FIFF_MNE_COV_KIND, &p_FiffCov.kind);
    this->write_int(FIFF_MNE_COV_DIM, &p_FiffCov.dim);
    if (p_FiffCov.nfree > 0)
        this->write_int(FIFF_MNE_COV_NFREE, &p_FiffCov.nfree);
    //
    //   Channel names
    //
    if (p_FiffCov.names.size() > 0)
        this->write_name_list(FIFF_MNE_ROW_NAMES, p_FiffCov.names);
    //
    //   Data
    //
    if (p_FiffCov.diag)
        this->write_double(FIFF_MNE_COV_DIAG, p_FiffCov.data.col(0).data(), p_FiffCov.data.rows());
    else {
        //        if issparse(cov.data)
        //           fiff_write_float_sparse_rcs(fid,FIFF.FIFF_MNE_COV,cov.data);
        //        else
        //        {
        // Store lower triangle including diagonal (matches read_cov format)
        std::int32_t dim = p_FiffCov.dim;
        std::int32_t n = dim * (dim + 1) / 2;

        // The loop below writes all n entries, but that is not provable at
        // compile time, so start from a defined state rather than passing a
        // possibly-uninitialised buffer to write_double.
        VectorXd vals = VectorXd::Zero(n);
        std::int32_t count = 0;
        for (std::int32_t i = 0; i < dim; ++i)
            for (std::int32_t j = 0; j <= i; ++j)
                vals(count++) = p_FiffCov.data(i, j);

        this->write_double(FIFF_MNE_COV, vals.data(), vals.size());
        //        }
    }
    //
    //   Eigenvalues and vectors if present
    //
    if (p_FiffCov.eig.size() > 0 && p_FiffCov.eigvec.size() > 0) {
        this->write_float_matrix(FIFF_MNE_COV_EIGENVECTORS, p_FiffCov.eigvec.cast<float>());
        this->write_double(FIFF_MNE_COV_EIGENVALUES, p_FiffCov.eig.data(), p_FiffCov.eig.size());
    }
    //
    //   Projection operator
    //
    this->write_proj(p_FiffCov.projs);
    //
    //   Bad channels
    //
    if (p_FiffCov.bads.size() > 0) {
        this->start_block(FIFFB_MNE_BAD_CHANNELS);
        this->write_name_list(FIFF_MNE_CH_NAME_LIST, p_FiffCov.bads);
        this->end_block(FIFFB_MNE_BAD_CHANNELS);
    }
    //
    //   Done!
    //
    this->end_block(FIFFB_MNE_COV);

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_ctf_comp(const std::vector<FiffCtfComp>& comps)
{
    fiff_long_t pos = tell();

    if (comps.size() <= 0)
        return -1;
    //
    //  This is very simple in fact
    //
    this->start_block(FIFFB_MNE_CTF_COMP);
    for (std::int32_t k = 0; k < std::ssize(comps); ++k) {
        FiffCtfComp comp(comps[k]);
        this->start_block(FIFFB_MNE_CTF_COMP_DATA);
        //
        //    Write the compensation kind
        //
        this->write_int(FIFF_MNE_CTF_COMP_KIND, &comp.ctfkind);
        std::int32_t save_calibrated = comp.save_calibrated;
        this->write_int(FIFF_MNE_CTF_COMP_CALIBRATED, &save_calibrated);
        //
        //  Compensators are calibrated in memory; unless they are to be saved that way, undo it
        //  (mne _write_ctf_comp)
        //
        if (!comps[k].save_calibrated) {
            // A compensator built in memory has no calibration factors to undo
            if (comp.rowcals.size() == comp.data.data.rows())
                comp.data.data = comp.rowcals.reshaped().cwiseInverse().asDiagonal() * comp.data.data;
            if (comp.colcals.size() == comp.data.data.cols())
                comp.data.data = comp.data.data * comp.colcals.reshaped().cwiseInverse().asDiagonal();
        }

        this->write_named_matrix(FIFF_MNE_CTF_COMP_DATA, comp.data);
        this->end_block(FIFFB_MNE_CTF_COMP_DATA);
    }
    this->end_block(FIFFB_MNE_CTF_COMP);

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_dig_point(const FiffDigPoint& dig)
{
    fiff_long_t pos = tell();

    //?typedef struct _fiffDigPointRec {
    //  fiff_int_t kind;               /*!< FIFF_POINT_CARDINAL,
    //                                  *   FIFF_POINT_HPI, or
    //                                  *   FIFF_POINT_EEG */
    //  fiff_int_t ident;              /*!< Number identifying this point */
    //  fiff_float_t r[3];             /*!< Point location */
    //} *fiffDigPoint,fiffDigPointRec; /*!< Digitization point description */
    fiff_int_t datasize = 5 * 4;

    write(static_cast<std::int32_t>(FIFF_DIG_POINT));
    write(static_cast<std::int32_t>(FIFFT_DIG_POINT_STRUCT));
    write(static_cast<std::int32_t>(datasize));
    write(static_cast<std::int32_t>(FIFFV_NEXT_SEQ));

    //
    //   Start writing fiffDigPointRec
    //
    write(static_cast<std::int32_t>(dig.kind));
    write(static_cast<std::int32_t>(dig.ident));
    for (std::int32_t i = 0; i < 3; ++i)
        write(dig.r[i]);

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_dir_pointer(fiff_int_t dirpos, fiff_long_t pos, fiff_int_t next)
{
    /*
     * Write entires to specified position
     */
    if (pos >= 0) {
        seek(pos);
    } else { //SEEK_END
        seek_to_end();
    }
    pos = tell();

    fiff_int_t datasize = 1 * 4;

    write(static_cast<std::int32_t>(FIFF_DIR_POINTER));
    write(static_cast<std::int32_t>(FIFFT_INT));
    write(static_cast<std::int32_t>(datasize));
    write(static_cast<std::int32_t>(next));

    write(dirpos);

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_dir_entries(const std::vector<FiffDirEntry>& dir, fiff_long_t pos)
{
    //    /** Directories are composed of these structures. *
    //     typedef struct _fiffDirEntryRec {
    //      fiff_int_t  kind;		/**< Tag number *
    //      fiff_int_t  type;		/**< Data type *
    //      fiff_int_t  size;		/**< How many bytes *
    //      fiff_int_t  pos;		/**< Location in file
    //                      * Note: the data is located at pos +
    //                      * FIFFC_DATA_OFFSET *
    //     } fiffDirEntryRec,*fiffDirEntry;/**< Directory is composed of these *

    /*
     * Write entires to specified position
     */
    if (pos >= 0) {
        seek(pos);
    } else { //SEEK_END
        seek_to_end();
    }

    pos = tell();

    const auto nent = static_cast<fiff_int_t>(dir.size());
    fiff_int_t datasize = nent * static_cast<fiff_int_t>(sizeof(FiffDirEntry));

    write(static_cast<std::int32_t>(FIFF_DIR));
    write(static_cast<std::int32_t>(FIFFT_DIR_ENTRY_STRUCT));
    write(static_cast<std::int32_t>(datasize));
    write(static_cast<std::int32_t>(FIFFV_NEXT_NONE));

    //
    //   Start writing FiffDirEntries
    //
    for (std::int32_t i = 0; i < nent; ++i) {
        write(static_cast<std::int32_t>(dir[i].kind));
        write(static_cast<std::int32_t>(dir[i].type));
        write(static_cast<std::int32_t>(dir[i].size));
        write(static_cast<std::int32_t>(dir[i].pos));
    }

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_double(fiff_int_t kind, const double* data, fiff_int_t nel)
{
    fiff_long_t pos = tell();

    std::int32_t datasize = nel * 8;

    write(static_cast<std::int32_t>(kind));
    write(static_cast<std::int32_t>(FIFFT_DOUBLE));
    write(datasize);
    write(static_cast<std::int32_t>(FIFFV_NEXT_SEQ));

    write_values(data, static_cast<std::size_t>(nel));

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_float(fiff_int_t kind, const float* data, fiff_int_t nel)
{
    fiff_long_t pos = tell();

    std::int32_t datasize = nel * 4;

    write(static_cast<std::int32_t>(kind));
    write(static_cast<std::int32_t>(FIFFT_FLOAT));
    write(datasize);
    write(static_cast<std::int32_t>(FIFFV_NEXT_SEQ));

    write_values(data, static_cast<std::size_t>(nel));

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_float_matrix(fiff_int_t kind, const MatrixXf& mat)
{
    fiff_long_t pos = tell();

    std::int32_t numel = mat.rows() * mat.cols();

    fiff_int_t datasize = 4 * numel + 4 * 3;

    write(static_cast<std::int32_t>(kind));
    write(static_cast<std::int32_t> FIFFT_MATRIX_FLOAT);
    write(static_cast<std::int32_t>(datasize));
    write(static_cast<std::int32_t>(FIFFV_NEXT_SEQ));

    write_row_major(mat);
    std::int32_t i;

    std::int32_t dims[3];
    dims[0] = mat.cols();
    dims[1] = mat.rows();
    dims[2] = 2;

    for (i = 0; i < 3; ++i)
        write(dims[i]);

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_float_sparse_ccs(fiff_int_t kind, const SparseMatrix<float>& mat)
{
    fiff_long_t pos = tell();

    //
    //   nnz values
    //   nnz row indices
    //   ncol+1 pointers
    //   dims
    //   nnz
    //   ndim
    //
    std::int32_t nnzm = mat.nonZeros();
    std::int32_t ncol = mat.cols();
    fiff_int_t datasize = 4 * nnzm + 4 * nnzm + 4 * (ncol + 1) + 4 * 4;
    //
    //   Nonzero entries
    //
    using T = Eigen::Triplet<float>;
    std::vector<T> s;
    s.reserve(mat.nonZeros());
    for (int k = 0; k < mat.outerSize(); ++k)
        for (SparseMatrix<float>::InnerIterator it(mat, k); it; ++it)
            s.push_back(T(it.row(), it.col(), it.value()));

    s = Linalg::sortrows<float>(s, 1);

    //[ rows, starts ] = unique(s(:,1),'first');
    std::vector<std::int32_t> cols, starts;
    std::int32_t v_old = -1;
    std::uint32_t i;
    for (i = 0; i < s.size(); ++i) {
        if (static_cast<signed>(s[i].col()) != v_old) {
            v_old = s[i].col();
            cols.push_back(s[i].col());
            starts.push_back(i);
        }
    }

    write(static_cast<std::int32_t>(kind));
    write(static_cast<std::int32_t> FIFFT_CCS_MATRIX_FLOAT);
    write(static_cast<std::int32_t>(datasize));
    write(static_cast<std::int32_t>(FIFFV_NEXT_SEQ));

    //
    //  The data values
    //
    for (i = 0; i < s.size(); ++i)
        write(s[i].value());

    //
    //  Row indices
    //
    for (i = 0; i < s.size(); ++i)
        write(s[i].row());

    //
    //  Pointers
    //
    RowVectorXi ptrs = RowVectorXi::Ones(ncol + 1);
    ptrs.array() *= -1;
    std::uint32_t k;
    for (k = 0; k < cols.size(); ++k)
        ptrs[cols[k]] = starts[k];
    ptrs[ncol] = nnzm;
    //
    //  Fill in pointers for empty columns
    //
    for (k = ncol; k >= 1; --k)
        if (ptrs[k - 1] < 0)
            ptrs[k - 1] = ptrs[k];
    //
    for (i = 0; i < static_cast<std::uint32_t>(ptrs.size()); ++i)
        write(ptrs[i]);
    //
    //   Dimensions
    //
    std::int32_t dims[4];
    dims[0] = mat.nonZeros();
    dims[1] = mat.rows();
    dims[2] = mat.cols();
    dims[3] = 2;

    for (i = 0; i < 4; ++i)
        write(dims[i]);

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_float_sparse_rcs(fiff_int_t kind, const SparseMatrix<float>& mat)
{
    fiff_long_t pos = tell();

    //
    //   nnz values
    //   nnz column indices
    //   nrow+1 pointers
    //   dims
    //   nnz
    //   ndim
    //
    std::int32_t nnzm = mat.nonZeros();
    std::int32_t nrow = mat.rows();
    fiff_int_t datasize = 4 * nnzm + 4 * nnzm + 4 * (nrow + 1) + 4 * 4;
    //
    //   Nonzero entries
    //
    using T = Eigen::Triplet<float>;
    std::vector<T> s;
    s.reserve(mat.nonZeros());
    for (int k = 0; k < mat.outerSize(); ++k)
        for (SparseMatrix<float>::InnerIterator it(mat, k); it; ++it)
            s.push_back(T(it.row(), it.col(), it.value()));

    s = Linalg::sortrows<float>(s);

    //[ rows, starts ] = unique(s(:,1),'first');
    std::vector<std::int32_t> rows, starts;
    std::int32_t v_old = -1;
    std::uint32_t i;
    for (i = 0; i < s.size(); ++i) {
        if (static_cast<signed>(s[i].row()) != v_old) {
            v_old = s[i].row();
            rows.push_back(s[i].row());
            starts.push_back(i);
        }
    }

    //
    // Write tag info header
    //
    write(static_cast<std::int32_t>(kind));
    write(static_cast<std::int32_t> FIFFT_RCS_MATRIX_FLOAT);
    write(static_cast<std::int32_t>(datasize));
    write(static_cast<std::int32_t>(FIFFV_NEXT_SEQ));

    //
    //  The data values
    //
    for (i = 0; i < s.size(); ++i)
        write(s[i].value());

    //
    //  Column indices
    //
    for (i = 0; i < s.size(); ++i)
        write(s[i].col());

    //
    //  Pointers
    //
    RowVectorXi ptrs = RowVectorXi::Ones(nrow + 1);
    ptrs.array() *= -1;
    std::uint32_t k;
    for (k = 0; k < rows.size(); ++k)
        ptrs[rows[k]] = starts[k];
    ptrs[nrow] = nnzm;

    //
    //  Fill in pointers for empty rows
    //
    for (k = nrow; k >= 1; --k)
        if (ptrs[k - 1] < 0)
            ptrs[k - 1] = ptrs[k];

    //
    for (i = 0; i < static_cast<std::uint32_t>(ptrs.size()); ++i)
        write(ptrs[i]);

    //
    //  Dimensions
    //
    std::int32_t dims[4];
    dims[0] = mat.nonZeros();
    dims[1] = mat.rows();
    dims[2] = mat.cols();
    dims[3] = 2;

    for (i = 0; i < 4; ++i)
        write(dims[i]);

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_id(fiff_int_t kind, const FiffId& id)
{
    fiff_long_t pos = tell();

    FiffId t_id = id;

    if (t_id.empty()) {
        //
        // Create a new one
        //
        t_id = FiffId::new_file_id();
    }

    //
    //
    fiff_int_t datasize = 5 * 4; //   The id comprises five integers

    write(static_cast<std::int32_t>(kind));
    write(static_cast<std::int32_t>(FIFFT_ID_STRUCT));
    write(static_cast<std::int32_t>(datasize));
    write(static_cast<std::int32_t>(FIFFV_NEXT_SEQ));
    //
    // Collect the bits together for one write
    //
    std::int32_t data[5];
    data[0] = t_id.version;
    data[1] = t_id.machid[0];
    data[2] = t_id.machid[1];
    data[3] = t_id.time.secs;
    data[4] = t_id.time.usecs;

    for (std::int32_t i = 0; i < 5; ++i)
        write(data[i]);

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_info_base(const FiffInfoBase& p_FiffInfoBase)
{
    fiff_long_t pos = tell();

    //
    // Information from the MEG file
    //
    this->start_block(FIFFB_MNE_PARENT_MEAS_FILE);
    this->write_string(FIFF_MNE_FILE_NAME, p_FiffInfoBase.filename);
    if (!p_FiffInfoBase.meas_id.empty())
        this->write_id(FIFF_PARENT_BLOCK_ID, p_FiffInfoBase.meas_id);

    //
    //    General
    //
    this->write_int(FIFF_NCHAN, &p_FiffInfoBase.nchan);

    //
    //    Channel info
    //
    std::int32_t k;
    std::vector<FiffChInfo> chs(p_FiffInfoBase.chs.begin(), p_FiffInfoBase.chs.begin() + p_FiffInfoBase.nchan);

    for (k = 0; k < p_FiffInfoBase.nchan; ++k) {
        //
        //    Scan numbers may have been messed up
        //
        chs[k].scanNo = k + 1; //+1 because
        this->write_ch_info(chs[k]);
    }

    //
    //    Blocks from the original -> skip this
    //
    bool have_hpi_result = false;

    //
    //    Coordinate transformations if the HPI result block was not there
    //
    if (!have_hpi_result) {
        if (!p_FiffInfoBase.dev_head_t.empty())
            this->write_coord_trans(p_FiffInfoBase.dev_head_t);
        if (!p_FiffInfoBase.ctf_head_t.empty())
            this->write_coord_trans(p_FiffInfoBase.ctf_head_t);
    }

    //
    //    Bad channels
    //
    if (p_FiffInfoBase.bads.size() > 0) {
        this->start_block(FIFFB_MNE_BAD_CHANNELS);
        this->write_name_list(FIFF_MNE_CH_NAME_LIST, p_FiffInfoBase.bads);
        this->end_block(FIFFB_MNE_BAD_CHANNELS);
    }

    this->end_block(FIFFB_MNE_PARENT_MEAS_FILE);

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_int(fiff_int_t kind, const fiff_int_t* data, fiff_int_t nel, fiff_int_t next)
{
    fiff_long_t pos = tell();

    fiff_int_t datasize = nel * 4;

    write(static_cast<std::int32_t>(kind));
    write(static_cast<std::int32_t>(FIFFT_INT));
    write(static_cast<std::int32_t>(datasize));
    write(static_cast<std::int32_t>(next));

    write_values(data, static_cast<std::size_t>(nel));

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_int_matrix(fiff_int_t kind, const MatrixXi& mat)
{
    fiff_long_t pos = tell();

    //    std::int32_t FIFFT_MATRIX = 1 << 30;
    //    std::int32_t FIFFT_MATRIX_INT = FIFFT_INT | FIFFT_MATRIX;

    std::int32_t numel = mat.rows() * mat.cols();

    fiff_int_t datasize = 4 * numel + 4 * 3;

    write(static_cast<std::int32_t>(kind));
    write(static_cast<std::int32_t> FIFFT_MATRIX_INT);
    write(static_cast<std::int32_t>(datasize));
    write(static_cast<std::int32_t>(FIFFV_NEXT_SEQ));

    write_row_major(mat);
    std::int32_t i;

    std::int32_t dims[3];
    dims[0] = mat.cols();
    dims[1] = mat.rows();
    dims[2] = 2;

    for (i = 0; i < 3; ++i)
        write(dims[i]);

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_name_list(fiff_int_t kind, const StringList& data)
{
    return this->write_string(kind, mne::join(data, ":"));
}

//=============================================================================================================

fiff_long_t FiffStream::write_named_matrix(fiff_int_t kind, const FiffNamedMatrix& mat)
{
    fiff_long_t pos = tell();

    this->start_block(FIFFB_MNE_NAMED_MATRIX);
    this->write_int(FIFF_MNE_NROW, &mat.nrow);
    this->write_int(FIFF_MNE_NCOL, &mat.ncol);
    if (mat.row_names.size() > 0)
        this->write_name_list(FIFF_MNE_ROW_NAMES, mat.row_names);
    if (mat.col_names.size() > 0)
        this->write_name_list(FIFF_MNE_COL_NAMES, mat.col_names);
    this->write_float_matrix(kind, mat.data.cast<float>());
    this->end_block(FIFFB_MNE_NAMED_MATRIX);

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_proj(const std::vector<FiffProj>& projs)
{
    fiff_long_t pos = tell();

    if (projs.size() <= 0)
        return -1;

    this->start_block(FIFFB_PROJ);

    for (auto& proj : projs) {
        this->start_block(FIFFB_PROJ_ITEM);
        this->write_string(FIFF_NAME, proj.desc);
        this->write_int(FIFF_PROJ_ITEM_KIND, &proj.kind);
        if (proj.kind == FIFFV_PROJ_ITEM_FIELD) {
            float fValue = 0.0f;
            this->write_float(FIFF_PROJ_ITEM_TIME, &fValue);
        }

        this->write_int(FIFF_NCHAN, &proj.data.ncol);
        this->write_int(FIFF_PROJ_ITEM_NVEC, &proj.data.nrow);
        std::int32_t bValue = static_cast<std::int32_t>(proj.active);
        this->write_int(FIFF_MNE_PROJ_ITEM_ACTIVE, &bValue);
        this->write_name_list(FIFF_PROJ_ITEM_CH_NAME_LIST, proj.data.col_names);
        this->write_float_matrix(FIFF_PROJ_ITEM_VECTORS, proj.data.data.cast<float>()); //rows == length(names)
        this->end_block(FIFFB_PROJ_ITEM);
    }
    this->end_block(FIFFB_PROJ);

    return pos;
}

//=============================================================================================================

fiff_long_t FiffStream::write_evoked_set(const FiffEvokedSet& p_FiffEvokedSet)
{
    fiff_long_t pos = tell();

    if (p_FiffEvokedSet.evoked.empty())
        return -1;

    this->start_block(FIFFB_MEAS);
    this->write_id(FIFF_BLOCK_ID);

    if (p_FiffEvokedSet.info.meas_id.version != -1)
        this->write_id(FIFF_PARENT_BLOCK_ID, p_FiffEvokedSet.info.meas_id);

    //
    //  Write measurement info so that the file can be read back
    //
    this->start_block(FIFFB_MEAS_INFO);

    this->write_int(FIFF_NCHAN, &p_FiffEvokedSet.info.nchan);

    float sfreq = p_FiffEvokedSet.info.sfreq;
    this->write_float(FIFF_SFREQ, &sfreq);

    for (int k = 0; k < p_FiffEvokedSet.info.nchan; ++k) {
        FiffChInfo ch = p_FiffEvokedSet.info.chs[k];
        ch.scanNo = k + 1;
        this->write_ch_info(ch);
    }

    if (!p_FiffEvokedSet.info.dev_head_t.empty())
        this->write_coord_trans(p_FiffEvokedSet.info.dev_head_t);

    if (!p_FiffEvokedSet.info.projs.empty())
        this->write_proj(p_FiffEvokedSet.info.projs);

    if (!p_FiffEvokedSet.info.bads.empty())
        this->write_bad_channels(p_FiffEvokedSet.info.bads);

    this->end_block(FIFFB_MEAS_INFO);

    for (int j = 0; j < std::ssize(p_FiffEvokedSet.evoked); ++j) {
        const FiffEvoked& evoked = p_FiffEvokedSet.evoked[j];

        this->start_block(FIFFB_PROCESSED_DATA);
        this->start_block(FIFFB_EVOKED);

        this->write_string(FIFF_COMMENT, evoked.comment);

        int first = evoked.first;
        this->write_int(FIFF_FIRST_SAMPLE, &first);

        int last = evoked.last;
        this->write_int(FIFF_LAST_SAMPLE, &last);

        this->start_block(FIFFB_ASPECT);

        int aspectKind = FIFFV_ASPECT_AVERAGE;
        this->write_int(FIFF_ASPECT_KIND, &aspectKind);

        int nave = evoked.nave;
        this->write_int(FIFF_NAVE, &nave);

        // Readers multiply FIFF_EPOCH by each channel's cal, so store uncalibrated values.
        Eigen::VectorXd decal(evoked.data.rows());
        for (int k = 0; k < decal.size(); ++k) {
            const double cal = k < std::ssize(p_FiffEvokedSet.info.chs) ? p_FiffEvokedSet.info.chs[k].cal : 1.0;
            decal[k] = cal != 0.0 ? 1.0 / cal : 1.0;
        }
        Eigen::MatrixXf floatData = (decal.asDiagonal() * evoked.data).cast<float>();
        this->write_float_matrix(FIFF_EPOCH, floatData);

        this->end_block(FIFFB_ASPECT);
        this->end_block(FIFFB_EVOKED);
        this->end_block(FIFFB_PROCESSED_DATA);
    }

    this->end_block(FIFFB_MEAS);

    return pos;
}

//=============================================================================================================

bool FiffStream::write_raw_buffer(const MatrixXd& buf,
                                  const RowVectorXd& cals)
{
    if (buf.rows() != cals.cols()) {
        Log::warning("buffer and calibration sizes do not match\n");
        return false;
    }

    using T = Eigen::Triplet<double>;
    std::vector<T> tripletList;
    tripletList.reserve(cals.cols());
    for (std::int32_t i = 0; i < cals.cols(); ++i)
        tripletList.push_back(T(i, i, 1.0 / cals[i]));

    SparseMatrix<double> inv_calsMat(cals.cols(), cals.cols());
    inv_calsMat.setFromTriplets(tripletList.begin(), tripletList.end());

    MatrixXf tmp = (inv_calsMat * buf).cast<float>();
    this->write_float(FIFF_DATA_BUFFER, tmp.data(), tmp.rows() * tmp.cols());
    return true;
}

//=============================================================================================================

bool FiffStream::write_raw_buffer(const MatrixXd& buf,
                                  const SparseMatrix<double>& mult)
{
    if (buf.rows() != mult.cols()) {
        Log::warning("buffer and mult sizes do not match\n");
        return false;
    }

    SparseMatrix<double> inv_mult(mult.rows(), mult.cols());
    for (int k = 0; k < inv_mult.outerSize(); ++k)
        for (SparseMatrix<double>::InnerIterator it(mult, k); it; ++it)
            inv_mult.coeffRef(it.row(), it.col()) = 1 / it.value();

    MatrixXf tmp = (inv_mult * buf).cast<float>();
    this->write_float(FIFF_DATA_BUFFER, tmp.data(), tmp.rows() * tmp.cols());
    return true;
}

//=============================================================================================================

bool FiffStream::write_raw_buffer(const MatrixXd& buf)
{
    MatrixXf tmp = buf.cast<float>();
    this->write_float(FIFF_DATA_BUFFER, tmp.data(), tmp.rows() * tmp.cols());
    return true;
}

//=============================================================================================================

fiff_long_t FiffStream::write_string(fiff_int_t kind,
                                     std::string_view data)
{
    fiff_long_t pos = tell();
    const fiff_int_t datasize = static_cast<fiff_int_t>(data.size());
    write(static_cast<std::int32_t>(kind));
    write(static_cast<std::int32_t>(FIFFT_STRING));
    write(static_cast<std::int32_t>(datasize));
    write(static_cast<std::int32_t>(FIFFV_NEXT_SEQ));

    write_raw(data.data(), datasize);

    return pos;
}

//=============================================================================================================

void FiffStream::write_rt_command(fiff_int_t command, const std::string& data)
{
    const fiff_int_t datasize = static_cast<fiff_int_t>(data.size());
    write(static_cast<std::int32_t>(FIFF_MNE_RT_COMMAND));
    write(static_cast<std::int32_t>(FIFFT_VOID));
    write(4 + static_cast<std::int32_t>(datasize));
    write(static_cast<std::int32_t>(FIFFV_NEXT_SEQ));
    write(command);

    write_raw(data.data(), datasize);
}

//=============================================================================================================

std::vector<FiffDirEntry> FiffStream::make_dir(bool* ok)
{
    std::vector<FiffDirEntry> dir;
    if (ok) {
        *ok = false;
    }
    if (!seek(SEEK_SET)) {
        return dir;
    }
    for (;;) {
        const fiff_long_t pos = tell();
        // A header read past the end of the data ends the scan.
        const auto tag = read_tag_info();
        if (!tag || tag->kind == FIFF_DIR) {
            break;
        }
        dir.push_back({tag->kind, tag->type, static_cast<fiff_int_t>(tag->size()), static_cast<fiff_int_t>(pos)});
        if (tag->next < 0) {
            break;
        }
    }
    dir.push_back(FiffDirEntry{});
    if (ok) {
        *ok = true;
    }
    return dir;
}

//=============================================================================================================

bool FiffStream::copy_processing_history(const std::filesystem::path& fromPath, const std::filesystem::path& toPath)
{
    auto opened = FiffStream::open(fromPath);
    if (!opened) {
        Log::warning("FiffStream::copy_processing_history - Cannot parse source FIFF file {}", fromPath.string());
        return false;
    }
    FiffStream& srcStream = *opened;

    // Find the FIFFB_PROCESSING_HISTORY block in the directory tree
    std::vector<std::shared_ptr<FiffDirNode>> histNodes = srcStream.dirtree()->dir_tree_find(FIFFB_PROCESSING_HISTORY);
    if (histNodes.empty()) {
        Log::warning("FiffStream::copy_processing_history - No processing history block found in {}", fromPath.string());
        srcStream.close();
        return false;
    }

    // Read the whole block, including the processing records nested inside it
    struct Item
    {
        int blockStart = 0; // block kind to open, 0 for a tag
        int blockEnd = 0;   // block kind to close, 0 for a tag
        std::optional<FiffTag> tag;
    };
    std::vector<Item> items;
    std::function<void(const std::shared_ptr<FiffDirNode>&)> collect = [&](const std::shared_ptr<FiffDirNode>& node) {
        items.push_back({node->type, 0, std::nullopt});
        for (int i = 0; i < node->nent(); ++i) {
            const int kind = node->dir[i].kind;
            if (kind == FIFF_BLOCK_START || kind == FIFF_BLOCK_END)
                continue;
            if (auto tag = srcStream.read_tag(node->dir[i].pos))
                items.push_back({0, 0, std::move(tag)});
        }
        for (int c = 0; c < node->nchild(); ++c)
            collect(node->children[c]);
        items.push_back({0, node->type, std::nullopt});
    };
    collect(histNodes[0]);
    srcStream.close();

    // Open destination file for update
    auto dstStream = FiffStream::open_update(toPath);
    if (!dstStream) {
        Log::warning("FiffStream::copy_processing_history - Cannot open destination file {} for update", toPath.string());
        return false;
    }

    // open_update marks the last tag as the end of the file; chain it to the appended block
    const fiff_long_t lastPos = dstStream->dir()[dstStream->nent() - 2].pos;
    std::optional<FiffTag> lastTag;
    if (lastTag = dstStream->read_tag(lastPos); !lastTag) {
        dstStream->close();
        return false;
    }
    lastTag->next = FIFFV_NEXT_SEQ;
    dstStream->write_tag(*lastTag, lastPos);
    dstStream->seek_to_end();

    for (const Item& item : items) {
        if (item.blockStart)
            dstStream->start_block(item.blockStart);
        else if (item.blockEnd)
            dstStream->end_block(item.blockEnd);
        else
            dstStream->write_tag(*item.tag);
    }
    // open_update left the file without its terminating tag
    dstStream->end_file();

    dstStream->close();
    return true;
}

//=============================================================================================================

std::optional<FiffTag> FiffStream::check_beginning()
{
    auto tag = read_tag();
    if (!tag || tag->kind != FIFF_FILE_ID || tag->type != FIFFT_ID_STRUCT || tag->size() != 20) {
        Log::warning("Fiff::open: file does not start with a file id tag\n");
        return std::nullopt;
    }
    // Not rewound: a TCP stream cannot seek back, and the tag already holds the id.
    return tag;
}
