//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_raw_data.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Florian Schlembach <fschlembach@web.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Ruben Doerfel <doerfelruben@aol.com>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Implementation of @ref FiffRawData: continuous-recording representation backed by a directory of FIFF_DATA_BUFFER tags.
 *
 * @ref FiffRawData::read_segment uses the @ref FiffRawDir directory to
 * seek straight to the buffers that cover a requested sample window,
 * decode them through the channel cals and active projectors and return
 * a contiguous Eigen matrix.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_raw_data.h"
#include <memory>
#include <base/log.h>
#include "fiff_events.h"
#include "fiff_tag.h"
#include "fiff_stream.h"
#include <base/byte_stream.h>
#include <base/string_utils.h>

#include <cstdlib>
#include <optional>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffRawData::FiffRawData(const std::filesystem::path& path)
: FiffRawData(read(path).value())
{
}

//=============================================================================================================

FiffRawData::FiffRawData(std::unique_ptr<mne::ByteStream> bytes)
: FiffRawData(read(std::move(bytes)).value())
{
}

//=============================================================================================================

mne::Expected<FiffRawData> FiffRawData::read(const std::filesystem::path& path, bool allow_maxshield)
{
    auto bytes = mne::FileByteStream::open(path, mne::FileByteStream::Mode::Read);
    if (!bytes) {
        return mne::fail(mne::ErrorCode::Io, "cannot open {}", mne::utf8FromPath(path));
    }
    return read(std::make_unique<FiffStream>(std::move(bytes), mne::utf8FromPath(path)), allow_maxshield);
}

//=============================================================================================================

mne::Expected<FiffRawData> FiffRawData::read(std::unique_ptr<mne::ByteStream> bytes, bool allow_maxshield)
{
    if (!bytes) {
        return mne::fail(mne::ErrorCode::Io, "no raw data bytes");
    }
    return read(std::make_unique<FiffStream>(std::move(bytes)), allow_maxshield);
}

//=============================================================================================================

mne::Expected<FiffRawData> FiffRawData::read(std::unique_ptr<FiffStream> stream, bool allow_maxshield)
{
    using mne::ErrorCode;
    using mne::fail;
    const std::string name = stream->stream_name();
    if (!stream->open()) {
        return fail(ErrorCode::Io, "cannot open {}", name);
    }
    if (stream->dirtree()->dir_tree_find(FIFFB_MEAS).empty()) {
        return fail(ErrorCode::NotFound, "no measurement in {}", name);
    }
    FiffRawData data;
    FiffInfo& info = data.info;
    std::shared_ptr<FiffDirNode> meas;
    if (!stream->read_meas_info(stream->dirtree(), info, meas)) {
        return fail(ErrorCode::Format, "cannot read the measurement info of {}", name);
    }

    // The block of interest, as mne.io.read_raw_fif looks for it
    std::vector<std::shared_ptr<FiffDirNode>> blocks = meas->dir_tree_find(FIFFB_RAW_DATA);
    if (blocks.empty()) {
        blocks = meas->dir_tree_find(FIFFB_CONTINUOUS_DATA);
    }
    if (blocks.empty()) {
        blocks = meas->dir_tree_find(FIFFB_SMSH_RAW_DATA);
        if (blocks.empty()) {
            return fail(ErrorCode::NotFound, "no raw data in {}", name);
        }
        if (!allow_maxshield) {
            return fail(ErrorCode::Format, "{} holds raw Internal Active Shielding data, which must be processed with MaxFilter first", name);
        }
    }
    const std::vector<FiffDirEntry>& dir = blocks.front()->dir;
    // An empty directory or no channels would leave no buffer to index and no sample count to derive
    if (dir.empty()) {
        return fail(ErrorCode::Format, "the raw data block of {} is empty", name);
    }
    if (info.nchan <= 0) {
        return fail(ErrorCode::Format, "the measurement info of {} reports {} channels", name, info.nchan);
    }
    const auto readInt = [&stream](const FiffDirEntry& entry) -> std::optional<fiff_int_t> {
        const std::optional<FiffTag> tag = stream->read_tag(entry.pos);
        const fiff_int_t* value = tag ? tag->to_int() : nullptr;
        return value ? std::optional<fiff_int_t>(*value) : std::nullopt;
    };

    std::size_t first = 0;
    fiff_int_t firstSamp = 0;
    fiff_int_t firstSkip = 0;
    if (dir[first].kind == FIFF_FIRST_SAMPLE) {
        const std::optional<fiff_int_t> value = readInt(dir[first++]);
        if (!value) {
            return fail(ErrorCode::Format, "cannot read the first sample of {}", name);
        }
        firstSamp = *value;
    }
    // An initial skip can only be applied once the buffer size is known
    if (first < dir.size() && dir[first].kind == FIFF_DATA_SKIP) {
        const std::optional<fiff_int_t> value = readInt(dir[first++]);
        if (!value) {
            return fail(ErrorCode::Format, "cannot read the initial skip of {}", name);
        }
        firstSkip = *value;
    }

    data.first_samp = firstSamp;
    fiff_int_t nskip = 0;
    for (std::size_t k = first; k < dir.size(); ++k) {
        const FiffDirEntry& ent = dir[k];
        if (ent.kind == FIFF_DATA_SKIP) {
            const std::optional<fiff_int_t> value = readInt(ent);
            if (!value) {
                return fail(ErrorCode::Format, "cannot read a skip of {}", name);
            }
            nskip = *value;
            continue;
        }
        if (ent.kind != FIFF_DATA_BUFFER) {
            continue;
        }
        fiff_int_t bytesPerValue = 0;
        switch (ent.type) {
            case FIFFT_DAU_PACK16:
            case FIFFT_SHORT:
                bytesPerValue = 2;
                break;
            case FIFFT_FLOAT:
            case FIFFT_INT:
                bytesPerValue = 4;
                break;
            case FIFFT_DOUBLE:
                bytesPerValue = 8;
                break;
            default:
                return fail(ErrorCode::Format, "{} has data buffers of type {}, which cannot be read", name, ent.type);
        }
        const fiff_int_t nsamp = ent.size / (bytesPerValue * info.nchan);
        if (firstSkip > 0) {
            firstSamp += nsamp * firstSkip;
            data.first_samp = firstSamp;
            firstSkip = 0;
        }
        if (nskip > 0) {
            // A skip has the default entry (kind -1); readers fill it with zeros
            data.rawdir.push_back({FiffDirEntry{}, firstSamp, firstSamp + nskip * nsamp - 1, nskip * nsamp});
            firstSamp += nskip * nsamp;
            nskip = 0;
        }
        data.rawdir.push_back({ent, firstSamp, firstSamp + nsamp - 1, nsamp});
        firstSamp += nsamp;
    }
    data.last_samp = firstSamp - 1;

    data.cals.resize(info.nchan);
    for (fiff_int_t k = 0; k < info.nchan; ++k) {
        data.cals[k] = static_cast<double>(info.chs[k].range) * info.chs[k].cal;
    }
    info.filename = name;
    data.file = std::move(stream);
    data.file->close();

    mne::Log::info("\tRange : {} ... {}  =  {:9.3f} ... {:9.3f} secs", data.first_samp, data.last_samp, static_cast<double>(data.first_samp) / data.info.sfreq, static_cast<double>(data.last_samp) / data.info.sfreq);
    return data;
}

//=============================================================================================================

FiffRawData::~FiffRawData()
{
}

//=============================================================================================================

void FiffRawData::clear()
{
    info.clear();
    first_samp = -1;
    last_samp = -1;
    cals = RowVectorXd();
    rawdir.clear();
    proj = MatrixXd();
    comp.clear();
}

//=============================================================================================================
#include <QElapsedTimer>
#include <QDebug>
#include <cstdint>
bool FiffRawData::read_raw_segment(MatrixXd& data,
                                   MatrixXd& times,
                                   fiff_int_t from,
                                   fiff_int_t to,
                                   const RowVectorXi& sel,
                                   bool do_debug) const
{
    bool projAvailable = true;

    if (this->proj.size() == 0) {
        projAvailable = false;
    }

    if (from == -1)
        from = this->first_samp;
    if (to == -1)
        to = this->last_samp;
    //
    //  Initial checks
    //
    if (from < this->first_samp)
        from = this->first_samp;
    if (to > this->last_samp)
        to = this->last_samp;
    //
    if (from > to) {
        mne::Log::warning("No data in this range {} ... {}  =  {:9.3f} ... {:9.3f} secs...", from, to, (static_cast<float>(from)) / this->info.sfreq, (static_cast<float>(to)) / this->info.sfreq);
        return false;
    }
    //
    //  Initialize the data and calibration vector
    //
    std::int32_t nchan = this->info.nchan;
    std::int32_t dest = 0; //1;
    std::int32_t i, k, r;

    using T = Eigen::Triplet<double>;
    std::vector<T> tripletList;
    tripletList.reserve(nchan);
    for (i = 0; i < nchan; ++i)
        tripletList.push_back(T(i, i, this->cals[i]));

    SparseMatrix<double> cal(nchan, nchan);
    cal.setFromTriplets(tripletList.begin(), tripletList.end());
    //    cal.makeCompressed();

    MatrixXd mult_full;
    //
    if (sel.size() == 0) {
        data = MatrixXd(nchan, to - from + 1);
        //            data->setZero();
        if (projAvailable || this->comp.kind != -1) {
            if (!projAvailable)
                mult_full = this->comp.data.data * cal;
            else if (this->comp.kind == -1)
                mult_full = this->proj * cal;
            else
                mult_full = this->proj * this->comp.data.data * cal;
        }
    } else {
        data = MatrixXd(sel.size(), to - from + 1);
        //            data->setZero();

        MatrixXd selVect(sel.size(), nchan);

        selVect.setZero();

        if (!projAvailable && this->comp.kind == -1) {
            tripletList.clear();
            tripletList.reserve(sel.size());
            for (i = 0; i < sel.size(); ++i)
                tripletList.push_back(T(i, i, this->cals[sel[i]]));
            cal = SparseMatrix<double>(sel.size(), sel.size());
            cal.setFromTriplets(tripletList.begin(), tripletList.end());
        } else {
            if (!projAvailable) {
                for (i = 0; i < sel.size(); ++i)
                    selVect.row(i) = this->comp.data.data.block(sel[i], 0, 1, nchan);
                mult_full = selVect * cal;
            } else if (this->comp.kind == -1) {
                for (i = 0; i < sel.size(); ++i)
                    selVect.row(i) = this->proj.block(sel[i], 0, 1, nchan);

                mult_full = selVect * cal;
            } else {
                for (i = 0; i < sel.size(); ++i)
                    selVect.row(i) = this->proj.block(sel[i], 0, 1, nchan);

                mult_full = selVect * this->comp.data.data * cal;
            }
        }
    }

    //
    // Make mult sparse
    //
    tripletList.clear();
    tripletList.reserve(mult_full.rows() * mult_full.cols());
    for (i = 0; i < mult_full.rows(); ++i)
        for (k = 0; k < mult_full.cols(); ++k)
            if (mult_full(i, k) != 0)
                tripletList.push_back(T(i, k, mult_full(i, k)));

    SparseMatrix<double> mult(mult_full.rows(), mult_full.cols());
    if (tripletList.size() > 0)
        mult.setFromTriplets(tripletList.begin(), tripletList.end());
    //    mult.makeCompressed();

    FiffStream& fid = *this->file;
    const bool open_here = !fid.is_open();
    if (open_here && !fid.reopen()) {
        mne::Log::warning("Cannot open file {}", this->info.filename);
        return false;
    }

    MatrixXd one, newData, tmp_data;
    std::optional<FiffTag> t_pTag;
    fiff_int_t first_pick, last_pick, picksamp;
    for (const FiffRawDir& thisRawDir : this->rawdir) {
        //
        //  Do we need this buffer
        //
        if (thisRawDir.last > from) {
            if (thisRawDir.ent.kind == -1) {
                //
                //  Take the easy route: skip is translated to zeros
                //
                if (do_debug)
                    mne::Log::debug("S");
                if (sel.cols() <= 0)
                    one.resize(nchan, thisRawDir.nsamp);
                else
                    one.resize(sel.cols(), thisRawDir.nsamp);

                one.setZero();
            } else {
                t_pTag = fid.read_tag(thisRawDir.ent.pos);
                //
                //   Depending on the state of the projection and selection
                //   we proceed a little bit differently
                //
                if (mult.cols() == 0) {
                    if (sel.cols() == 0) {
                        if (t_pTag->type == FIFFT_DAU_PACK16)
                            one = cal * (Map<const MatrixDau16>(t_pTag->to_dau_pack16(), nchan, thisRawDir.nsamp)).cast<double>();
                        else if (t_pTag->type == FIFFT_INT)
                            one = cal * (Map<const MatrixXi>(t_pTag->to_int(), nchan, thisRawDir.nsamp)).cast<double>();
                        else if (t_pTag->type == FIFFT_FLOAT)
                            one = cal * (Map<const MatrixXf>(t_pTag->to_float(), nchan, thisRawDir.nsamp)).cast<double>();
                        else if (t_pTag->type == FIFFT_SHORT)
                            one = cal * (Map<const MatrixShort>(t_pTag->to_short(), nchan, thisRawDir.nsamp)).cast<double>();
                        else if (t_pTag->type == FIFFT_DOUBLE)
                            one = cal * Map<const MatrixXd>(t_pTag->to_double(), nchan, thisRawDir.nsamp);
                        else {
                            mne::Log::warning("Data Storage Format not known yet [1]!! Type: {}\n", t_pTag->type);
                            if (open_here)
                                this->file->close();
                            return false;
                        }
                    } else {
                        //ToDo find a faster solution for this!! --> make cal and mul sparse like in MATLAB
                        newData.resize(sel.cols(), thisRawDir.nsamp); //ToDo this can be done much faster, without newData

                        if (t_pTag->type == FIFFT_DAU_PACK16) {
                            tmp_data = (Map<const MatrixDau16>(t_pTag->to_dau_pack16(), nchan, thisRawDir.nsamp)).cast<double>();

                            for (r = 0; r < sel.size(); ++r)
                                newData.block(r, 0, 1, thisRawDir.nsamp) = tmp_data.block(sel[r], 0, 1, thisRawDir.nsamp);
                        } else if (t_pTag->type == FIFFT_INT) {
                            tmp_data = (Map<const MatrixXi>(t_pTag->to_int(), nchan, thisRawDir.nsamp)).cast<double>();

                            for (r = 0; r < sel.size(); ++r)
                                newData.block(r, 0, 1, thisRawDir.nsamp) = tmp_data.block(sel[r], 0, 1, thisRawDir.nsamp);
                        } else if (t_pTag->type == FIFFT_FLOAT) {
                            tmp_data = (Map<const MatrixXf>(t_pTag->to_float(), nchan, thisRawDir.nsamp)).cast<double>();

                            for (r = 0; r < sel.size(); ++r)
                                newData.block(r, 0, 1, thisRawDir.nsamp) = tmp_data.block(sel[r], 0, 1, thisRawDir.nsamp);
                        } else if (t_pTag->type == FIFFT_SHORT) {
                            tmp_data = (Map<const MatrixShort>(t_pTag->to_short(), nchan, thisRawDir.nsamp)).cast<double>();

                            for (r = 0; r < sel.size(); ++r)
                                newData.block(r, 0, 1, thisRawDir.nsamp) = tmp_data.block(sel[r], 0, 1, thisRawDir.nsamp);
                        } else if (t_pTag->type == FIFFT_DOUBLE) {
                            tmp_data = Map<const MatrixXd>(t_pTag->to_double(), nchan, thisRawDir.nsamp);

                            for (r = 0; r < sel.size(); ++r)
                                newData.block(r, 0, 1, thisRawDir.nsamp) = tmp_data.block(sel[r], 0, 1, thisRawDir.nsamp);
                        } else {
                            mne::Log::warning("Data Storage Format not known yet [2]!! Type: {}\n", t_pTag->type);
                            if (open_here)
                                this->file->close();
                            return false;
                        }

                        one = cal * newData;
                    }
                } else {
                    if (t_pTag->type == FIFFT_DAU_PACK16)
                        one = mult * (Map<const MatrixDau16>(t_pTag->to_dau_pack16(), nchan, thisRawDir.nsamp)).cast<double>();
                    else if (t_pTag->type == FIFFT_INT)
                        one = mult * (Map<const MatrixXi>(t_pTag->to_int(), nchan, thisRawDir.nsamp)).cast<double>();
                    else if (t_pTag->type == FIFFT_FLOAT)
                        one = mult * (Map<const MatrixXf>(t_pTag->to_float(), nchan, thisRawDir.nsamp)).cast<double>();
                    else if (t_pTag->type == FIFFT_SHORT)
                        one = mult * (Map<const MatrixShort>(t_pTag->to_short(), nchan, thisRawDir.nsamp)).cast<double>();
                    else if (t_pTag->type == FIFFT_DOUBLE)
                        one = mult * Map<const MatrixXd>(t_pTag->to_double(), nchan, thisRawDir.nsamp);
                    else {
                        mne::Log::warning("Data Storage Format not known yet [3]!! Type: {}\n", t_pTag->type);
                        if (open_here)
                            this->file->close();
                        return false;
                    }
                }
            }
            //
            //  The picking logic is a bit complicated
            //
            if (to >= thisRawDir.last && from <= thisRawDir.first) {
                //
                //  We need the whole buffer
                //
                first_pick = 0; //1;
                last_pick = thisRawDir.nsamp - 1;
                if (do_debug)
                    mne::Log::debug("W");
            } else if (from > thisRawDir.first) {
                first_pick = from - thisRawDir.first; // + 1;
                if (to < thisRawDir.last) {
                    //
                    //  Something from the middle
                    //
                    last_pick = thisRawDir.nsamp + to - thisRawDir.last - 1; //is this alright?
                    if (do_debug)
                        mne::Log::debug("M");
                } else {
                    //
                    //  From the middle to the end
                    //
                    last_pick = thisRawDir.nsamp - 1;
                    if (do_debug)
                        mne::Log::debug("E");
                }
            } else {
                //
                //  From the beginning to the middle
                //
                first_pick = 0;                    //1;
                last_pick = to - thisRawDir.first; // + 1;
                if (do_debug)
                    mne::Log::debug("B");
            }
            //
            //  Now we are ready to pick
            //
            picksamp = last_pick - first_pick + 1;

            if (do_debug) {
                mne::Log::debug("first_pick: {}", first_pick);
                mne::Log::debug("last_pick: {}", last_pick);
                mne::Log::debug("picksamp: {}", picksamp);
            }

            if (picksamp > 0) {
                //                    for(r = 0; r < data->rows(); ++r)
                //                        for(c = 0; c < picksamp; ++c)
                //                            (*data)(r,dest + c) = one(r,first_pick + c);
                data.block(0, dest, data.rows(), picksamp) = one.block(0, first_pick, data.rows(), picksamp);

                dest += picksamp;
            }
        }
        //
        //  Done?
        //
        if (thisRawDir.last >= to) {
            break;
        }
    }

    if (open_here) {
        this->file->close();
    }

    times = MatrixXd(1, to - from + 1);

    for (i = 0; i < times.cols(); ++i)
        times(0, i) = static_cast<float>(from + i) / this->info.sfreq;

    return true;
}

//=============================================================================================================

bool FiffRawData::read_raw_segment(MatrixXd& data,
                                   MatrixXd& times,
                                   SparseMatrix<double>& multSegment,
                                   fiff_int_t from,
                                   fiff_int_t to,
                                   const RowVectorXi& sel,
                                   bool do_debug) const
{
    bool projAvailable = true;

    if (this->proj.size() == 0) {
        projAvailable = false;
    }

    if (from == -1)
        from = this->first_samp;
    if (to == -1)
        to = this->last_samp;
    //
    //  Initial checks
    //
    if (from < this->first_samp)
        from = this->first_samp;
    if (to > this->last_samp)
        to = this->last_samp;
    //
    if (from > to) {
        mne::Log::warning("No data in this range");
        return false;
    }
    //
    //  Initialize the data and calibration vector
    //
    std::int32_t nchan = this->info.nchan;
    std::int32_t dest = 0; //1;
    std::int32_t i, k, r;

    using T = Eigen::Triplet<double>;
    std::vector<T> tripletList;
    tripletList.reserve(nchan);
    for (i = 0; i < nchan; ++i)
        tripletList.push_back(T(i, i, this->cals[i]));

    SparseMatrix<double> cal(nchan, nchan);
    cal.setFromTriplets(tripletList.begin(), tripletList.end());
    //    cal.makeCompressed();

    MatrixXd mult_full;
    //
    if (sel.size() == 0) {
        data = MatrixXd(nchan, to - from + 1);
        //            data->setZero();
        if (projAvailable || this->comp.kind != -1) {
            if (!projAvailable)
                mult_full = this->comp.data.data * cal;
            else if (this->comp.kind == -1)
                mult_full = this->proj * cal;
            else
                mult_full = this->proj * this->comp.data.data * cal;
        }
    } else {
        data = MatrixXd(sel.size(), to - from + 1);
        //            data->setZero();

        MatrixXd selVect(sel.size(), nchan);

        selVect.setZero();

        if (!projAvailable && this->comp.kind == -1) {
            tripletList.clear();
            tripletList.reserve(sel.size());
            for (i = 0; i < sel.size(); ++i)
                tripletList.push_back(T(i, i, this->cals[sel[i]]));
            cal = SparseMatrix<double>(sel.size(), sel.size());
            cal.setFromTriplets(tripletList.begin(), tripletList.end());
        } else {
            if (!projAvailable) {
                for (i = 0; i < sel.size(); ++i)
                    selVect.row(i) = this->comp.data.data.block(sel[i], 0, 1, nchan);
                mult_full = selVect * cal;
            } else if (this->comp.kind == -1) {
                for (i = 0; i < sel.size(); ++i)
                    selVect.row(i) = this->proj.block(sel[i], 0, 1, nchan);

                mult_full = selVect * cal;
            } else {
                for (i = 0; i < sel.size(); ++i)
                    selVect.row(i) = this->proj.block(sel[i], 0, 1, nchan);

                mult_full = selVect * this->comp.data.data * cal;
            }
        }
    }

    //
    // Make mult sparse
    //
    tripletList.clear();
    tripletList.reserve(mult_full.rows() * mult_full.cols());
    for (i = 0; i < mult_full.rows(); ++i)
        for (k = 0; k < mult_full.cols(); ++k)
            if (mult_full(i, k) != 0)
                tripletList.push_back(T(i, k, mult_full(i, k)));

    SparseMatrix<double> mult(mult_full.rows(), mult_full.cols());
    if (tripletList.size() > 0)
        mult.setFromTriplets(tripletList.begin(), tripletList.end());
    //    mult.makeCompressed();

    //

    FiffStream& fid = *this->file;
    const bool open_here = !fid.is_open();
    if (open_here && !fid.reopen()) {
        mne::Log::warning("Cannot open file {}", this->info.filename);
        return false;
    }

    MatrixXd one;
    fiff_int_t first_pick, last_pick, picksamp;
    for (const FiffRawDir& thisRawDir : this->rawdir) {
        //
        //  Do we need this buffer
        //
        if (thisRawDir.last > from) {
            if (thisRawDir.ent.kind == -1) {
                //
                //  Take the easy route: skip is translated to zeros
                //
                if (do_debug)
                    mne::Log::debug("S");
                if (sel.cols() <= 0)
                    one.resize(nchan, thisRawDir.nsamp);
                else
                    one.resize(sel.cols(), thisRawDir.nsamp);

                one.setZero();
            } else {
                std::optional<FiffTag> t_pTag;
                t_pTag = fid.read_tag(thisRawDir.ent.pos);
                //
                //   Depending on the state of the projection and selection
                //   we proceed a little bit differently
                //
                if (mult.cols() == 0) {
                    if (sel.cols() == 0) {
                        if (t_pTag->type == FIFFT_DAU_PACK16)
                            one = cal * (Map<const MatrixDau16>(t_pTag->to_dau_pack16(), nchan, thisRawDir.nsamp)).cast<double>();
                        else if (t_pTag->type == FIFFT_INT)
                            one = cal * (Map<const MatrixXi>(t_pTag->to_int(), nchan, thisRawDir.nsamp)).cast<double>();
                        else if (t_pTag->type == FIFFT_FLOAT)
                            one = cal * (Map<const MatrixXf>(t_pTag->to_float(), nchan, thisRawDir.nsamp)).cast<double>();
                        else if (t_pTag->type == FIFFT_SHORT)
                            one = cal * (Map<const MatrixShort>(t_pTag->to_short(), nchan, thisRawDir.nsamp)).cast<double>();
                        else if (t_pTag->type == FIFFT_DOUBLE)
                            one = cal * Map<const MatrixXd>(t_pTag->to_double(), nchan, thisRawDir.nsamp);
                        else {
                            mne::Log::warning("Data Storage Format not known yet [1]!! Type: {}\n", t_pTag->type);
                            if (open_here)
                                this->file->close();
                            return false;
                        }
                    } else {
                        //ToDo find a faster solution for this!! --> make cal and mul sparse like in MATLAB
                        MatrixXd newData(sel.cols(), thisRawDir.nsamp); //ToDo this can be done much faster, without newData

                        if (t_pTag->type == FIFFT_DAU_PACK16) {
                            MatrixXd tmp_data = (Map<const MatrixDau16>(t_pTag->to_dau_pack16(), nchan, thisRawDir.nsamp)).cast<double>();

                            for (r = 0; r < sel.size(); ++r)
                                newData.block(r, 0, 1, thisRawDir.nsamp) = tmp_data.block(sel[r], 0, 1, thisRawDir.nsamp);
                        } else if (t_pTag->type == FIFFT_INT) {
                            MatrixXd tmp_data = (Map<const MatrixXi>(t_pTag->to_int(), nchan, thisRawDir.nsamp)).cast<double>();

                            for (r = 0; r < sel.size(); ++r)
                                newData.block(r, 0, 1, thisRawDir.nsamp) = tmp_data.block(sel[r], 0, 1, thisRawDir.nsamp);
                        } else if (t_pTag->type == FIFFT_FLOAT) {
                            MatrixXd tmp_data = (Map<const MatrixXf>(t_pTag->to_float(), nchan, thisRawDir.nsamp)).cast<double>();

                            for (r = 0; r < sel.size(); ++r)
                                newData.block(r, 0, 1, thisRawDir.nsamp) = tmp_data.block(sel[r], 0, 1, thisRawDir.nsamp);
                        } else if (t_pTag->type == FIFFT_SHORT) {
                            MatrixXd tmp_data = (Map<const MatrixShort>(t_pTag->to_short(), nchan, thisRawDir.nsamp)).cast<double>();

                            for (r = 0; r < sel.size(); ++r)
                                newData.block(r, 0, 1, thisRawDir.nsamp) = tmp_data.block(sel[r], 0, 1, thisRawDir.nsamp);
                        } else if (t_pTag->type == FIFFT_DOUBLE) {
                            MatrixXd tmp_data = Map<const MatrixXd>(t_pTag->to_double(), nchan, thisRawDir.nsamp);

                            for (r = 0; r < sel.size(); ++r)
                                newData.block(r, 0, 1, thisRawDir.nsamp) = tmp_data.block(sel[r], 0, 1, thisRawDir.nsamp);
                        } else {
                            mne::Log::warning("Data Storage Format not known yet [2]!! Type: {}\n", t_pTag->type);
                            if (open_here)
                                this->file->close();
                            return false;
                        }

                        one = cal * newData;
                    }
                } else {
                    if (t_pTag->type == FIFFT_DAU_PACK16)
                        one = mult * (Map<const MatrixDau16>(t_pTag->to_dau_pack16(), nchan, thisRawDir.nsamp)).cast<double>();
                    else if (t_pTag->type == FIFFT_INT)
                        one = mult * (Map<const MatrixXi>(t_pTag->to_int(), nchan, thisRawDir.nsamp)).cast<double>();
                    else if (t_pTag->type == FIFFT_FLOAT)
                        one = mult * (Map<const MatrixXf>(t_pTag->to_float(), nchan, thisRawDir.nsamp)).cast<double>();
                    else if (t_pTag->type == FIFFT_SHORT)
                        one = mult * (Map<const MatrixShort>(t_pTag->to_short(), nchan, thisRawDir.nsamp)).cast<double>();
                    else if (t_pTag->type == FIFFT_DOUBLE)
                        one = mult * Map<const MatrixXd>(t_pTag->to_double(), nchan, thisRawDir.nsamp);
                    else {
                        mne::Log::warning("Data Storage Format not known yet [3]!! Type: {}\n", t_pTag->type);
                        if (open_here)
                            this->file->close();
                        return false;
                    }
                }
            }
            //
            //  The picking logic is a bit complicated
            //
            if (to >= thisRawDir.last && from <= thisRawDir.first) {
                //
                //  We need the whole buffer
                //
                first_pick = 0; //1;
                last_pick = thisRawDir.nsamp - 1;
                if (do_debug)
                    mne::Log::debug("W");
            } else if (from > thisRawDir.first) {
                first_pick = from - thisRawDir.first; // + 1;
                if (to < thisRawDir.last) {
                    //
                    //  Something from the middle
                    //
                    last_pick = thisRawDir.nsamp + to - thisRawDir.last - 1; //is this alright?
                    if (do_debug)
                        mne::Log::debug("M");
                } else {
                    //
                    //  From the middle to the end
                    //
                    last_pick = thisRawDir.nsamp - 1;
                    if (do_debug)
                        mne::Log::debug("E");
                }
            } else {
                //
                //  From the beginning to the middle
                //
                first_pick = 0;                    //1;
                last_pick = to - thisRawDir.first; // + 1;
                if (do_debug)
                    mne::Log::debug("B");
            }
            //
            //  Now we are ready to pick
            //
            picksamp = last_pick - first_pick + 1;

            if (do_debug) {
                mne::Log::debug("first_pick: {}", first_pick);
                mne::Log::debug("last_pick: {}", last_pick);
                mne::Log::debug("picksamp: {}", picksamp);
            }

            if (picksamp > 0) {
                //                    for(r = 0; r < data->rows(); ++r)
                //                        for(c = 0; c < picksamp; ++c)
                //                            (*data)(r,dest + c) = one(r,first_pick + c);
                data.block(0, dest, data.rows(), picksamp) = one.block(0, first_pick, data.rows(), picksamp);

                dest += picksamp;
            }
        }
        //
        //  Done?
        //
        if (thisRawDir.last >= to) {
            break;
        }
    }

    if (mult.cols() == 0)
        multSegment = cal;
    else
        multSegment = mult;

    if (open_here) {
        this->file->close();
    }

    times = MatrixXd(1, to - from + 1);

    for (i = 0; i < times.cols(); ++i)
        times(0, i) = static_cast<float>(from + i) / this->info.sfreq;

    return true;
}

//=============================================================================================================

bool FiffRawData::read_raw_segment_times(MatrixXd& data,
                                         MatrixXd& times,
                                         float from,
                                         float to,
                                         const RowVectorXi& sel) const
{
    //
    //   Convert to samples
    //
    from = floor(static_cast<double>(from) * this->info.sfreq);
    to = ceil(static_cast<double>(to) * this->info.sfreq);
    //
    //   Read it
    //
    return this->read_raw_segment(data, times, static_cast<std::int32_t>(from), static_cast<std::int32_t>(to), sel);
}

//=============================================================================================================

int FiffRawData::activate_projections()
{
    if (info.projs.empty()) {
        return 0;
    }
    for (FiffProj& projection : info.projs) {
        projection.active = true;
    }
    return info.make_projector(proj);
}

//=============================================================================================================

mne::Expected<void> FiffRawData::apply_gradient_compensation(int grade)
{
    const int current = info.get_current_comp();
    if (current == grade) {
        return {};
    }
    if (!info.make_compensator(current, grade, comp)) {
        return mne::fail(mne::ErrorCode::Format, "cannot compensate from grade {} to grade {}", current, grade);
    }
    info.set_current_comp(grade);
    return {};
}

//=============================================================================================================

bool FiffRawData::save(const std::filesystem::path& path,
                       const RowVectorXi& picks,
                       int decim,
                       int from,
                       int to) const
{
    if (decim < 1)
        decim = 1;

    int firstSamp = (from >= 0) ? from : first_samp;
    int lastSamp = (to >= 0) ? to : last_samp;

    if (firstSamp > lastSamp) {
        mne::Log::warning("[FiffRawData::save] Invalid sample range.");
        return false;
    }

    // start_writing_raw picks the channels itself; picking info here as well applied picks twice.
    FiffInfo outInfo = info;

    // Adjust sampling frequency for decimation
    if (decim > 1) {
        outInfo.sfreq = info.sfreq / static_cast<float>(decim);
    }

    // Use the standard start_writing_raw pipeline
    RowVectorXd calsOut;
    auto pStream = FiffStream::start_writing_raw(path, outInfo, calsOut, picks);
    if (!pStream) {
        mne::Log::warning("[FiffRawData::save] Cannot start writing raw file.");
        return false;
    }

    // Without this the copy restarts at sample 0 instead of at its position in the recording.
    int firstOut = firstSamp / decim;
    pStream->write_int(FIFF_FIRST_SAMPLE, &firstOut);

    // Write data in blocks
    const int blockSize = 2000;
    int blockSamples = decim * blockSize;

    for (int samp = firstSamp; samp <= lastSamp; samp += blockSamples) {
        int nsamp = qMin(blockSamples, lastSamp - samp + 1);

        MatrixXd segData;
        MatrixXd segTimes;
        if (!read_raw_segment(segData, segTimes, samp, samp + nsamp - 1, picks)) {
            mne::Log::warning("[FiffRawData::save] Error reading data at sample {}", samp);
            pStream->finish_writing_raw();
            return false;
        }

        // Decimate if needed
        if (decim > 1) {
            int nOut = (nsamp + decim - 1) / decim;
            MatrixXd decimData(segData.rows(), nOut);
            for (int s = 0, idx = 0; s < nsamp && idx < nOut; s += decim, ++idx) {
                decimData.col(idx) = segData.col(s);
            }
            segData = decimData;
        }

        pStream->write_raw_buffer(segData, calsOut);
    }

    pStream->finish_writing_raw();

    mne::Log::info("[FiffRawData::save] Saved raw data from sample {} to {} (decim= {} )", firstSamp, lastSamp, decim);
    return true;
}
