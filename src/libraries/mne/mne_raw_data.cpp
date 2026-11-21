//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     mne_raw_data.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Johannes Behrens <behrens@neoscan-solutions.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January 2017
 * @brief    Implementation of @ref mne::MNERawData.
 *
 * Implements buffer-list construction by walking the @c FIFFB_RAW_DATA
 * block tree, per-buffer file seek/read with on-the-fly type conversion
 * to single precision and SSP/CTF application as the buffers stream
 * through.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_raw_data.h"

#include "mne_types.h"
#include "mne_ch_selection.h"
#include "mne_filter_def.h"
#include <base/log.h>
#include <base/string_utils.h>
#include <fiff/fiff_ch_name.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <unsupported/Eigen/FFT>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <vector>
#include <sstream>
#include <algorithm>
#include <complex>
#include <iterator>
#include <string>
#include <string_view>
#include <cmath>
#include <numbers>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace mne;

constexpr int FAIL = -1;
constexpr int OK = 0;

#if defined(_WIN32) || defined(_WIN64)
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

namespace mne
{

/** Row-major float matrix (file-local convenience alias). */
using RowMajorMatrixXf = MNERawData::RowMajorMatrixXf;

/**
 * @brief Circular ring buffer for managing raw-data matrix allocations.
 *
 * Each slot tracks a single RowMajorMatrixXf.  When a slot is reused the
 * previous occupant is evicted by resizing it to 0x0, which releases its
 * heap storage while keeping the Eigen object itself alive.
 */
struct RingBuffer
{
    struct Entry
    {
        RowMajorMatrixXf* user = nullptr;
    };

    std::vector<Entry> entries;
    int next = 0;

    explicit RingBuffer(int nslots)
    : entries(static_cast<size_t>(nslots))
    , next(0)
    {
    }

    /** Allocate (or reclaim) an entry for @p res with dimensions @p nrow x @p ncol. */
    void allocate(int nrow, int ncol, RowMajorMatrixXf* res)
    {
        if (next >= static_cast<int>(entries.size()))
            next = 0;
        Entry& e = entries[static_cast<size_t>(next++)];
        if (e.user) // evict old occupant
            e.user->resize(0, 0);
        res->resize(nrow, ncol);
        e.user = res;
    }
};

}

//============================= misc_util.c =============================

//============================= mne_apply_filter.c =============================

namespace mne
{

/**
 * @brief Pre-computed frequency-domain filter state used for FFT-based raw data filtering.
 *
 * Holds the frequency response arrays (normal + EOG) and an FFT
 * pre-calculation buffer.  Owned by MNERawData via unique_ptr.
 */
struct FilterData
{
    std::vector<float> freq_resp;     /**< Frequency response. */
    std::vector<float> eog_freq_resp; /**< Frequency response (EOG). */
    std::vector<float> precalc;       /**< Pre-calculated data for FFT. */

    explicit FilterData(int resp_size)
    : freq_resp(static_cast<size_t>(resp_size), 1.0f)
    , eog_freq_resp(static_cast<size_t>(resp_size), 1.0f)
    {
    }
};

}

static int mne_compare_filters(const MNEFilterDef& f1,
                               const MNEFilterDef& f2)
/*
      * Return 0 if the two filter definitions are same, 1 otherwise
      */
{
    if (f1.filter_on != f2.filter_on ||
        std::fabs(f1.lowpass - f2.lowpass) > 0.1 ||
        std::fabs(f1.lowpass_width - f2.lowpass_width) > 0.1 ||
        std::fabs(f1.highpass - f2.highpass) > 0.1 ||
        std::fabs(f1.highpass_width - f2.highpass_width) > 0.1 ||
        std::fabs(f1.eog_lowpass - f2.eog_lowpass) > 0.1 ||
        std::fabs(f1.eog_lowpass_width - f2.eog_lowpass_width) > 0.1 ||
        std::fabs(f1.eog_highpass - f2.eog_highpass) > 0.1 ||
        std::fabs(f1.eog_highpass_width - f2.eog_highpass_width) > 0.1)
        return 1;
    else
        return 0;
}

//============================= mne_fft.c =============================

void mne_fft_ana(float* data, int np, std::vector<float>& /*precalc*/)
/*
      * FFT analysis for real data, in place, FFTPACK rfftf layout:
      * r0, r1, i1, r2, i2, ..., and r(np/2) last when np is even.
      */
{
    Eigen::FFT<float> fft;
    std::vector<float> in(data, data + np);
    std::vector<std::complex<float>> spec;
    fft.fwd(spec, in);
    data[0] = spec[0].real();
    for (int k = 1, p = 1; p < np; ++k) {
        data[p++] = spec[k].real();
        if (p < np)
            data[p++] = spec[k].imag();
    }
}

void mne_fft_syn(float* data, int np, std::vector<float>& /*precalc*/)
/*
      * Inverse of mne_fft_ana (FFTPACK rfftb followed by MNE-C's 1/np scaling)
      */
{
    Eigen::FFT<float> fft;
    std::vector<std::complex<float>> spec(np);
    spec[0] = std::complex<float>(data[0], 0.0f);
    for (int k = 1, p = 1; p < np; ++k) {
        const float re = data[p++];
        const float im = p < np ? data[p++] : 0.0f;
        spec[k] = std::complex<float>(re, im);
        spec[np - k] = std::conj(spec[k]);
    }
    std::vector<float> out;
    fft.inv(out, spec);
    std::copy(out.begin(), out.begin() + np, data);
}

int mne_apply_filter(const MNEFilterDef& filter, FilterData* d, float* data, int ns, int zero_pad, float dc_offset, int kind)
/*
 * Do the magick trick
 */
{
    int k, p, n;
    float* freq_resp;

    if (ns != filter.size + 2 * filter.taper_size) {
        mne::Log::critical("Incorrect data length in apply_filter");
        return FAIL;
    }
    /*
   * Zero padding
   */
    if (zero_pad) {
        for (k = 0; k < filter.taper_size; k++)
            data[k] = 0.0;
        for (k = filter.taper_size + filter.size; k < ns; k++)
            data[k] = 0.0;
    }
    if (!filter.filter_on) /* Nothing else to do */
        return OK;
    /*
   * Make things nice by compensating for the dc offset
   */
    if (dc_offset != 0.0) {
        for (k = filter.taper_size; k < filter.taper_size + filter.size; k++)
            data[k] = data[k] - dc_offset;
    }
    if (!d)
        return OK;
    if (d->freq_resp.empty())
        return OK;
    /*
   * Next comes the FFT
   */
    mne_fft_ana(data, ns, d->precalc);
    /*
   * Multiply with the frequency response
   * See FFTpack doc for details of the arrangement
   */
    n = ns % 2 == 0 ? ns / 2 : (ns + 1) / 2;
    p = 0;
    /*
   * No imaginary part for the DC component
   */
    if (kind == FIFFV_EOG_CH)
        freq_resp = d->eog_freq_resp.data();
    else
        freq_resp = d->freq_resp.data();
    data[p] = data[p] * freq_resp[0];
    p++;
    /*
   * The other components
   */
    for (k = 1; k < n; k++) {
        data[p] = data[p] * freq_resp[k];
        p++;
        data[p] = data[p] * freq_resp[k];
        p++;
    }
    /*
   * Then the last value
   */
    if (ns % 2 == 0)
        data[p] = data[p] * freq_resp[k];

    mne_fft_syn(data, ns, d->precalc);

    return OK;
}

std::unique_ptr<FilterData> mne_create_filter_response(const MNEFilterDef& filter,
                                                       float sfreq,
                                                       int* highpass_effective)
/*
      * Create a frequency response
      */
{
    int resp_size;
    int k, s, w, f;
    int highpasss, lowpasss;
    int highpass_widths, lowpass_widths;
    float lowpass, highpass, lowpass_width, highpass_width;
    float* freq_resp;
    float pi4 = static_cast<float>(std::numbers::pi / 4.0);
    float mult, add, c;

    resp_size = (filter.size + 2 * filter.taper_size) / 2 + 1;

    auto filter_data = std::make_unique<FilterData>(resp_size);
    *highpass_effective = false;

    for (f = 0; f < 2; f++) {
        highpass = f == 0 ? filter.highpass : filter.eog_highpass;
        highpass_width = f == 0 ? filter.highpass_width : filter.eog_highpass_width;
        lowpass = f == 0 ? filter.lowpass : filter.eog_lowpass;
        lowpass_width = f == 0 ? filter.lowpass_width : filter.eog_lowpass_width;
        freq_resp = f == 0 ? filter_data->freq_resp.data() : filter_data->eog_freq_resp.data();
        /*
     * Start simple first
     */
        highpasss = ((resp_size - 1) * highpass) / (0.5 * sfreq);
        lowpasss = ((resp_size - 1) * lowpass) / (0.5 * sfreq);

        lowpass_widths = ((resp_size - 1) * lowpass_width) / (0.5 * sfreq);
        lowpass_widths = (lowpass_widths + 1) / 2; /* What user specified */

        if (filter.highpass_width > 0.0) {
            highpass_widths = ((resp_size - 1) * highpass_width) / (0.5 * sfreq);
            highpass_widths = (highpass_widths + 1) / 2; /* What user specified */
        } else
            highpass_widths = 3; /* Minimal */

        if (filter.filter_on) {
            mne::Log::info("filter : {:7.3f} ... {:6.1f} Hz   bins : {} ... {} of {} hpw : {} lpw : {}\n", highpass, lowpass, highpasss, lowpasss, resp_size, highpass_widths, lowpass_widths);
        }
        if (highpasss > highpass_widths + 1) {
            w = highpass_widths;
            mult = 1.0 / w;
            add = 3.0;
            for (k = 0; k < highpasss - w + 1; k++)
                freq_resp[k] = 0.0;
            for (k = -w + 1, s = highpasss - w + 1; k < w; k++, s++) {
                if (s >= 0 && s < resp_size) {
                    c = cos(pi4 * (k * mult + add));
                    freq_resp[s] = freq_resp[s] * c * c;
                }
            }
            *highpass_effective = true;
        } else
            *highpass_effective = *highpass_effective || (filter.highpass == 0.0);

        if (lowpass_widths > 0) {
            w = lowpass_widths;
            mult = 1.0 / w;
            add = 1.0;
            for (k = -w + 1, s = lowpasss - w + 1; k < w; k++, s++) {
                if (s >= 0 && s < resp_size) {
                    c = cos(pi4 * (k * mult + add));
                    freq_resp[s] = freq_resp[s] * c * c;
                }
            }
            for (k = s; k < resp_size; k++)
                freq_resp[k] = 0.0;
        } else {
            for (k = lowpasss; k < resp_size; k++)
                freq_resp[k] = 0.0;
        }
        if (filter.filter_on) {
            if (*highpass_effective)
                mne::Log::info("Highpass filter will work as specified.\n");
            else
                mne::Log::warning("NOTE: Highpass filter omitted due to a too low corner frequency.\n");
        } else
            mne::Log::warning("NOTE: Filter is presently switched off.\n");
    }
    return filter_data;
}

//============================= mne_raw_routines.c =============================

int mne_read_raw_buffer_t( //fiffFile     in,        /* Input file */
    FiffStream& stream,
    const FiffDirEntry& ent,                 /* The directory entry to read */
    RowMajorMatrixXf& data,                  /* Matrix [npick x nsamp] to fill */
    int nchan,                               /* Number of channels in the data */
    int nsamp,                               /* Expected number of samples */
    const std::vector<mne::FiffChInfo>& chs, /* Channel info for ALL channels */
    int* pickno,                             /* Which channels to pick */
    int npick)                               /* How many */

{
    std::optional<FiffTag> t_pTag;
    //    fiffTagRec   tag;
    const fiff_short_t* this_samples;
    const fiff_float_t* this_samplef;
    const fiff_int_t* this_sample;

    int s, c;

    //    tag.data = NULL;

    Eigen::VectorXi pickno_vec;
    if (npick == 0) {
        pickno_vec = Eigen::VectorXi::LinSpaced(nchan, 0, nchan - 1);
        pickno = pickno_vec.data();
        npick = nchan;
    }

    Eigen::VectorXf mult(npick);
    for (c = 0; c < npick; c++)
        mult[c] = chs[pickno[c]].cal * chs[pickno[c]].range;

    //    if (fiff_read_this_tag(in->fd,ent->pos,&tag) ==  FIFF_FAIL)
    //        goto bad;
    if (t_pTag = stream.read_tag(ent.pos); !t_pTag)
        return FAIL;

    if (ent.type == FIFFT_FLOAT) {
        if (static_cast<int>(t_pTag->size() / (sizeof(fiff_float_t) * nchan)) != nsamp) {
            mne::Log::critical("Incorrect number of samples in buffer.");
            return FAIL;
        }
        this_samplef = t_pTag->to_float();
        for (s = 0; s < nsamp; s++, this_samplef += nchan) {
            for (c = 0; c < npick; c++)
                data(c, s) = mult[c] * this_samplef[pickno[c]];
        }
    } else if (ent.type == FIFFT_SHORT || ent.type == FIFFT_DAU_PACK16) {
        if (static_cast<int>(t_pTag->size() / (sizeof(fiff_short_t) * nchan)) != nsamp) {
            mne::Log::critical("Incorrect number of samples in buffer.");
            return FAIL;
        }
        this_samples = reinterpret_cast<fiff_short_t*>(t_pTag->data());
        for (s = 0; s < nsamp; s++, this_samples += nchan) {
            for (c = 0; c < npick; c++)
                data(c, s) = mult[c] * this_samples[pickno[c]];
        }
    } else if (ent.type == FIFFT_INT) {
        if (static_cast<int>(t_pTag->size() / (sizeof(fiff_int_t) * nchan)) != nsamp) {
            mne::Log::critical("Incorrect number of samples in buffer.");
            return FAIL;
        }
        this_sample = t_pTag->to_int();
        for (s = 0; s < nsamp; s++, this_sample += nchan) {
            for (c = 0; c < npick; c++)
                data(c, s) = mult[c] * this_sample[pickno[c]];
        }
    } else if (ent.type == FIFFT_DOUBLE) {
        if (static_cast<int>(t_pTag->size() / (sizeof(double) * nchan)) != nsamp) {
            mne::Log::critical("Incorrect number of samples in buffer.");
            return FAIL;
        }
        const double* this_sampled = t_pTag->to_double();
        for (s = 0; s < nsamp; s++, this_sampled += nchan) {
            for (c = 0; c < npick; c++)
                data(c, s) = static_cast<float>(mult[c] * this_sampled[pickno[c]]);
        }
    } else {
        mne::Log::critical("We are not prepared to handle raw data type: {}", ent.type);
        return FAIL;
    }
    return OK;
}

int mne_sparse_vec_mult2(FiffSparseMatrix* mat, /* The sparse matrix */
                         float* vector,         /* Vector to be multiplied */
                         float* res)            /* Result of the multiplication */
/*
      * Multiply a vector by a sparse matrix using Eigen.
      */
{
    Eigen::Map<const Eigen::VectorXf> vecIn(vector, mat->cols());
    Eigen::Map<Eigen::VectorXf> vecOut(res, mat->rows());
    vecOut = mat->eigen() * vecIn;
    return 0;
}

int mne_sparse_mat_mult2(FiffSparseMatrix* mat,        /* The sparse matrix */
                         const RowMajorMatrixXf& mult, /* Matrix to be multiplied */
                         RowMajorMatrixXf& res)        /* Result of the multiplication */
/*
      * Multiply a dense matrix by a sparse matrix using Eigen.
      */
{
    // mat->eigen() is column-major sparse, mult is row-major dense
    // Result: res = sparse * mult  (rows: mat->rows(), cols: mult.cols())
    res = mat->eigen() * mult;
    return 0;
}

constexpr auto APPROX_RING_BUF_SIZE = (600 * 1024 * 1024);

static int approx_ring_buf_size = APPROX_RING_BUF_SIZE;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNERawData::MNERawData()
: info(nullptr)
, first_samp(0)
, omit_samp(0)
, first_samp_old(0)
, omit_samp_old(0)
, nsamp(0)
, proj(nullptr)
, sss(nullptr)
, comp(nullptr)
, comp_file(MNE_CTFV_NOGRAD)
, comp_now(MNE_CTFV_NOGRAD)
, max_event(0)
, dig_trigger_mask(0)
, deriv(nullptr)
, deriv_matched(nullptr)
{
}

//=============================================================================================================

MNERawData::~MNERawData()
{
    //    fiff_close(this->file);
    if (this->stream)
        this->stream->close();
    this->filename.clear();
    this->ch_names.clear();

    this->badlist.clear();

    this->dig_trigger.clear();
    this->event_list.reset();
}

//=============================================================================================================

void MNERawData::add_filter_response(int* highpass_effective)
/*
          * Add the standard filter frequency response function
          */
{
    /*
       * Free the previous filter definition
       */
    filter_data.reset();
    /*
       * Nothing more to do if there is no filter
       */
    if (!filter)
        return;
    /*
       * Create a new one
       */
    filter_data = mne_create_filter_response(*filter,
                                             info->sfreq,
                                             highpass_effective);
}

//=============================================================================================================

void MNERawData::setup_filter_bufs()
/*
     * These will hold the filtered data
     */
{
    int nfilt_buf;
    int k;
    int firstsamp;
    int nring_buf;
    int highpass_effective;

    this->filt_bufs.clear();
    this->filt_ring.reset();

    if (!this->filter || filter->size <= 0)
        return;

    for (nfilt_buf = 0, firstsamp = this->first_samp - filter->taper_size;
         firstsamp < this->nsamp + this->first_samp;
         firstsamp = firstsamp + filter->size)
        nfilt_buf++;
#ifdef DEBUG
    mne::Log::info("{} filter buffers needed\n", nfilt_buf);
#endif
    this->filt_bufs.resize(nfilt_buf);
    for (k = 0, firstsamp = this->first_samp - filter->taper_size; k < nfilt_buf; k++,
        firstsamp = firstsamp + filter->size) {
        filt_bufs[k].ns = filter->size + 2 * filter->taper_size;
        filt_bufs[k].firsts = firstsamp;
        filt_bufs[k].lasts = firstsamp + filt_bufs[k].ns - 1;
        //        bufs[k].ent         = NULL;
        filt_bufs[k].nchan = this->info->nchan;
        filt_bufs[k].is_skip = false;
        filt_bufs[k].valid = false;
        filt_bufs[k].ch_filtered = Eigen::VectorXi::Zero(this->info->nchan);
        filt_bufs[k].comp_status = MNE_CTFV_NOGRAD;
    }
    nring_buf = static_cast<int>(approx_ring_buf_size / ((2 * filter->taper_size + filter->size) * static_cast<std::size_t>(this->info->nchan) * sizeof(float)));
    this->filt_ring = std::make_unique<RingBuffer>(nring_buf);
    add_filter_response(&highpass_effective);

    return;
}

//=============================================================================================================

bool MNERawData::setFilter(const MNEFilterDef& newFilter)
{
    const bool sameLayout = filter && newFilter.size == filter->size && newFilter.taper_size == filter->taper_size;
    if (sameLayout && !mne_compare_filters(newFilter, *filter))
        return false;
    filter = std::make_unique<MNEFilterDef>(newFilter);
    if (sameLayout) {
        for (MNERawBufDef& buf : filt_bufs)
            buf.valid = false;
        int highpass_effective;
        add_filter_response(&highpass_effective);
    } else {
        setup_filter_bufs();
    }
    return true;
}

//=============================================================================================================

int MNERawData::load_one_buffer(MNERawBufDef* buf)
/*
     * load just one
     */
{
    if (buf->ent.kind == FIFF_DATA_SKIP) {
        mne::Log::critical("Cannot load a skip");
        return FAIL;
    }
    if (buf->vals.size() == 0) { /* The data space may have been reused */
        buf->valid = false;
        ring->allocate(buf->nchan, buf->ns, &buf->vals);
    }
    if (buf->valid)
        return OK;

#ifdef DEBUG
    mne::Log::debug("Read buffer {} .. {}\n", buf->firsts, buf->lasts);
#endif

    if (mne_read_raw_buffer_t(*stream,
                              buf->ent,
                              buf->vals,
                              buf->nchan,
                              buf->ns,
                              info->chInfo,
                              nullptr, 0) != OK) {
        buf->valid = false;
        return FAIL;
    }
    buf->valid = true;
    buf->comp_status = comp_file;
    return OK;
}

//=============================================================================================================

int MNERawData::compensate_buffer(MNERawBufDef* buf)
/*
     * Apply compensation channels
     */
{
    if (!comp)
        return OK;
    if (!comp->undo && !comp->current)
        return OK;
    if (buf->comp_status == comp_now)
        return OK;
    if (buf->vals.size() == 0)
        return OK;
    /*
       * vals is now a RowMajorMatrixXf — wrap in a column-major MatrixXf for compensation
       */
    {
        Eigen::MatrixXf dataMat = buf->vals; /* implicit copy/conversion */

        if (comp->undo) {
            std::swap(comp->current, comp->undo);
            /*
             * Undo the previous compensation
             */
            if (comp->apply_transpose(false, dataMat) != OK) {
                std::swap(comp->current, comp->undo);
                return FAIL;
            }
            std::swap(comp->current, comp->undo);
        }
        if (comp->current) {
            /*
             * Apply new compensation
             */
            if (comp->apply_transpose(true, dataMat) != OK)
                return FAIL;
        }
        /*
         * Copy result back to buf->vals
         */
        buf->vals = dataMat;
    }
    buf->comp_status = comp_now;
    return OK;
}

//=============================================================================================================

int MNERawData::pick_data(MNEChSelection* sel, int firsts, Eigen::Ref<RowMajorMatrixXf> picked)
/*
     * Data from a selection of channels
     */
{
    const int nrow = sel ? sel->nchan() : info->nchan;
    if (picked.rows() != nrow) {
        mne::Log::warning("{}: the buffer has {} rows for {} channels", __func__, picked.rows(), nrow);
        return FAIL;
    }
    const bool use_deriv = sel && deriv_matched && sel->nderiv() > 0;
    int ns = static_cast<int>(picked.cols());
    int k, s, p, start, c, fills;
    int ns2, s2;
    MNERawBufDef* this_buf;
    int need_some;

    RowMajorMatrixXf deriv_vals;
    int deriv_ns = 0;
    int nderiv = 0;

    if (firsts < first_samp) {
        for (s = 0, p = firsts; p < first_samp; s++, p++) {
            if (sel)
                for (c = 0; c < sel->nchan(); c++)
                    picked(c, s) = 0.0;
            else
                for (c = 0; c < info->nchan; c++)
                    picked(c, s) = 0.0;
        }
        ns = ns - s;
        firsts = first_samp;
    } else
        s = 0;
    /*
       * There is possibly nothing to do
       */
    if (sel) {
        for (c = 0, need_some = false; c < sel->nchan(); c++) {
            if (sel->pick[c] >= 0 || sel->pick_deriv[c] >= 0) {
                need_some = true;
                break;
            }
        }
        if (!need_some)
            return OK;
    }
    /*
       * Have to to the hard work
       */
    // s continues after any leading zero padding (MNE-C reset it here and overwrote the padding).
    for (k = 0, this_buf = bufs.data(); k < static_cast<int>(bufs.size()); k++, this_buf++) {
        if (this_buf->lasts >= firsts) {
            start = firsts - this_buf->firsts;
            if (start < 0)
                start = 0;
            if (this_buf->is_skip) {
                for (p = start; p < this_buf->ns && ns > 0; p++, ns--, s++) {
                    if (sel) {
                        for (c = 0; c < sel->nchan(); c++)
                            if (sel->pick[c] >= 0)
                                picked(c, s) = 0.0;
                    } else {
                        for (c = 0; c < info->nchan; c++)
                            picked(c, s) = 0.0;
                    }
                }
            } else {
                /*
             * Load the buffer
             */
                if (load_one_buffer(this_buf) != OK)
                    return FAIL;
                /*
             * Apply compensation
             */
                if (compensate_buffer(this_buf) != OK)
                    return FAIL;
                ns2 = s2 = 0;
                if (sel) {
                    /*
               * Do we need the derived channels?
               */
                    if (use_deriv) {
                        if (deriv_ns < this_buf->ns || nderiv != deriv_matched->deriv_data->nrow()) {
                            deriv_vals.resize(deriv_matched->deriv_data->nrow(), this_buf->ns);
                            nderiv = deriv_matched->deriv_data->nrow();
                            deriv_ns = this_buf->ns;
                        }
                        if (mne_sparse_mat_mult2(&deriv_matched->deriv_data->data, this_buf->vals, deriv_vals) == FAIL) {
                            return FAIL;
                        }
                    }
                    for (c = 0; c < sel->nchan(); c++) {
                        /*
                 * First pick the ordinary channels...
                 */
                        if (sel->pick[c] >= 0) {
                            for (p = start, s2 = s, ns2 = ns; p < this_buf->ns && ns2 > 0; p++, ns2--, s2++)
                                picked(c, s2) = this_buf->vals(sel->pick[c], p);
                        }
                        /*
                 * ...then the derived ones
                 */
                        else if (sel->pick_deriv[c] >= 0 && deriv_matched) {
                            for (p = start, s2 = s, ns2 = ns; p < this_buf->ns && ns2 > 0; p++, ns2--, s2++)
                                picked(c, s2) = deriv_vals(sel->pick_deriv[c], p);
                        }
                    }
                } else {
                    for (c = 0; c < info->nchan; c++)
                        for (p = start, s2 = s, ns2 = ns; p < this_buf->ns && ns2 > 0; p++, ns2--, s2++)
                            picked(c, s2) = this_buf->vals(c, p);
                }
                s = s2;
                ns = ns2;
            }
            if (ns == 0)
                break;
        }
    }
    /*
       * Extend with the last available sample or zero if the request is beyond the data
       */
    if (s > 0) {
        fills = s - 1;
        for (; ns > 0; ns--, s++) {
            if (sel)
                for (c = 0; c < sel->nchan(); c++)
                    picked(c, s) = picked(c, fills);
            else
                for (c = 0; c < info->nchan; c++)
                    picked(c, s) = picked(c, fills);
        }
    } else {
        for (; ns > 0; ns--, s++) {
            if (sel)
                for (c = 0; c < sel->nchan(); c++)
                    picked(c, s) = 0;
            else
                for (c = 0; c < info->nchan; c++)
                    picked(c, s) = 0;
        }
    }
    return OK;
}

//=============================================================================================================

int MNERawData::pick_data_proj(MNEChSelection* sel, int firsts, Eigen::Ref<RowMajorMatrixXf> picked)
/*
     * Data from a set of channels, apply projection
     */
{
    const int nrow = sel ? sel->nchan() : info->nchan;
    if (picked.rows() != nrow) {
        mne::Log::warning("{}: the buffer has {} rows for {} channels", __func__, picked.rows(), nrow);
        return FAIL;
    }
    const bool use_deriv = sel && deriv_matched && sel->nderiv() > 0;
    int ns = static_cast<int>(picked.cols());
    int k, s, p, start, c, fills;
    MNERawBufDef* this_buf;
    RowMajorMatrixXf* values;
    Eigen::VectorXf deriv_pvalues_vec;

    if (!proj || (sel && !proj->affect(sel->chspick) && !proj->affect(sel->chspick_nospace)))
        return pick_data(sel, firsts, picked);

    if (firsts < first_samp) {
        for (s = 0, p = firsts; p < first_samp; s++, p++) {
            if (sel)
                for (c = 0; c < sel->nchan(); c++)
                    picked(c, s) = 0.0;
            else
                for (c = 0; c < info->nchan; c++)
                    picked(c, s) = 0.0;
        }
        ns = ns - s;
        firsts = first_samp;
    } else
        s = 0;
    Eigen::VectorXf pvalues(info->nchan);
    for (k = 0, this_buf = bufs.data(); k < static_cast<int>(bufs.size()); k++, this_buf++) {
        if (this_buf->lasts >= firsts) {
            start = firsts - this_buf->firsts;
            if (start < 0)
                start = 0;
            if (this_buf->is_skip) {
                for (p = start; p < this_buf->ns && ns > 0; p++, ns--, s++) {
                    if (sel) {
                        for (c = 0; c < sel->nchan(); c++)
                            if (sel->pick[c] >= 0)
                                picked(c, s) = 0.0;
                    } else {
                        for (c = 0; c < info->nchan; c++)
                            picked(c, s) = 0.0;
                    }
                }
            } else {
                /*
             * Load the buffer
             */
                if (load_one_buffer(this_buf) != OK)
                    return FAIL;
                /*
             * Apply compensation
             */
                if (compensate_buffer(this_buf) != OK)
                    return FAIL;
                /*
             * Apply projection
             */
                values = &this_buf->vals;
                if (use_deriv) {
                    deriv_pvalues_vec.resize(deriv_matched->deriv_data->nrow());
                }
                for (p = start; p < this_buf->ns && ns > 0; p++, ns--, s++) {
                    for (c = 0; c < info->nchan; c++)
                        pvalues[c] = (*values)(c, p);
                    if (!proj->project_vector(pvalues, true))
                        mne::Log::warning("Error");
                    if (sel) {
                        if (use_deriv) {
                            if (mne_sparse_vec_mult2(&deriv_matched->deriv_data->data, pvalues.data(), deriv_pvalues_vec.data()) == FAIL)
                                return FAIL;
                        }
                        for (c = 0; c < sel->nchan(); c++) {
                            /*
                   * First try the ordinary channels...
                   */
                            if (sel->pick[c] >= 0)
                                picked(c, s) = pvalues[sel->pick[c]];
                            /*
                   * ...then the derived ones
                   */
                            else if (sel->pick_deriv[c] >= 0 && deriv_matched)
                                picked(c, s) = deriv_pvalues_vec[sel->pick_deriv[c]];
                        }
                    } else {
                        for (c = 0; c < info->nchan; c++) {
                            picked(c, s) = pvalues[c];
                        }
                    }
                }
            }
            if (ns == 0)
                break;
        }
    }

    /*
       * Extend with the last available sample or zero if the request is beyond the data
       */
    if (s > 0) {
        fills = s - 1;
        for (; ns > 0; ns--, s++) {
            if (sel)
                for (c = 0; c < sel->nchan(); c++)
                    picked(c, s) = picked(c, fills);
            else
                for (c = 0; c < info->nchan; c++)
                    picked(c, s) = picked(c, fills);
        }
    } else {
        for (; ns > 0; ns--, s++) {
            if (sel)
                for (c = 0; c < sel->nchan(); c++)
                    picked(c, s) = 0;
            else
                for (c = 0; c < info->nchan; c++)
                    picked(c, s) = 0;
        }
    }
    return OK;
}

//=============================================================================================================

int MNERawData::load_one_filt_buf(MNERawBufDef* buf)
/*
     * Load and filter one buffer
     */
{
    int k;
    int res;

    if (buf->vals.size() == 0) {
        buf->valid = false;
        filt_ring->allocate(buf->nchan, buf->ns, &buf->vals);
    }
    if (buf->valid)
        return OK;

    for (k = 0; k < buf->nchan; k++)
        buf->ch_filtered[k] = false;

    res = pick_data_proj(nullptr, buf->firsts + filter->taper_size, buf->vals.middleCols(filter->taper_size, buf->ns - 2 * filter->taper_size));

#ifdef DEBUG
    if (res == OK)
        mne::Log::debug("Loaded filtered buffer {}...{} {} {} last = {}\n", buf->firsts, buf->lasts, buf->lasts - buf->firsts + 1, buf->ns, first_samp + nsamp);
#endif
    buf->valid = res == OK;
    return res;
}

//=============================================================================================================

int MNERawData::pick_data_filt(MNEChSelection* sel, int firsts, Eigen::Ref<RowMajorMatrixXf> picked)
/*
     * Data for a selection (filtered and picked)
     */
{
    const int nrow = sel ? sel->nchan() : info->nchan;
    if (picked.rows() != nrow) {
        mne::Log::warning("{}: the buffer has {} rows for {} channels", __func__, picked.rows(), nrow);
        return FAIL;
    }
    const bool use_deriv = sel && deriv_matched && sel->nderiv() > 0;
    int ns = static_cast<int>(picked.cols());
    int k, s, bs, c;
    int bs1, s1, s2, lasts;
    [[maybe_unused]] int bs2; // only reported in DEBUG builds
    MNERawBufDef* this_buf;
    float* values;
    RowMajorMatrixXf deriv_vals;
    Eigen::VectorXf dc;
    float dc_offset;
    int deriv_ns = 0;
    int nderiv = 0;
    int filter_was;

    if (!filter->filter_on)
        return pick_data_proj(sel, firsts, picked);

    if (sel) {
        for (s = 0; s < ns; s++)
            for (c = 0; c < sel->nchan(); c++)
                picked(c, s) = 0.0;
    } else {
        for (s = 0; s < ns; s++)
            for (c = 0; c < info->nchan; c++)
                picked(c, s) = 0.0;
    }
    lasts = firsts + ns - 1;
    /*
       * Take into account the initial dc offset (compensate and project)
       */
    if (first_sample_val.size() > 0) {
        dc = first_sample_val;
        /*
         * Is this correct??
         */
        if (comp && comp->current)
            if (comp->apply(true, dc) != OK)
                return FAIL;
        if (proj)
            if (!proj->project_vector(dc, true))
                return FAIL;
    }
    filter_was = filter->filter_on;
    /*
       * Find the first buffer to consider
       */
    for (k = 0, this_buf = filt_bufs.data(); k < static_cast<int>(filt_bufs.size()); k++, this_buf++) {
        if (this_buf->lasts >= firsts)
            break;
    }
    for (; k < static_cast<int>(filt_bufs.size()) && this_buf->firsts <= lasts; k++, this_buf++) {
#ifdef DEBUG
        mne::Log::debug("this_buf ({}): {}..{}\n", k, this_buf->firsts, this_buf->lasts);
#endif
        /*
         * Load the buffer first and apply projection
         */
        if (load_one_filt_buf(this_buf) != OK)
            return FAIL;
        /*
         * Then filter all relevant channels (not stimuli)
         */
        if (sel) {
            for (c = 0; c < sel->nchan(); c++) {
                if (sel->pick[c] >= 0) {
                    if (!this_buf->ch_filtered[sel->pick[c]]) {
                        /*
                 * Do not filter stimulus channels
                 */
                        dc_offset = 0.0;
                        if (info->chInfo[sel->pick[c]].kind == FIFFV_STIM_CH)
                            filter->filter_on = false;
                        else if (dc.size() > 0)
                            dc_offset = dc[sel->pick[c]];
                        if (mne_apply_filter(*filter, filter_data.get(), this_buf->vals.row(sel->pick[c]).data(), this_buf->ns, true,
                                             dc_offset, info->chInfo[sel->pick[c]].kind) != OK) {
                            filter->filter_on = filter_was;
                            return FAIL;
                        }
                        this_buf->ch_filtered[sel->pick[c]] = true;
                        filter->filter_on = filter_was;
                    }
                }
            }
            /*
           * Also check channels included in derivations if they are used
           */
            if (use_deriv) {
                MNEDeriv* der = deriv_matched.get();
                for (c = 0; c < der->deriv_data->ncol(); c++) {
                    if (der->in_use[c] > 0 &&
                        !this_buf->ch_filtered[c]) {
                        /*
                 * Do not filter stimulus channels
                 */
                        dc_offset = 0.0;
                        if (info->chInfo[c].kind == FIFFV_STIM_CH)
                            filter->filter_on = false;
                        else if (dc.size() > 0)
                            dc_offset = dc[c];
                        if (mne_apply_filter(*filter, filter_data.get(), this_buf->vals.row(c).data(), this_buf->ns, true,
                                             dc_offset, info->chInfo[c].kind) != OK) {
                            filter->filter_on = filter_was;
                            return FAIL;
                        }
                        this_buf->ch_filtered[c] = true;
                        filter->filter_on = filter_was;
                    }
                }
            }
        } else {
            /*
           * Simply filter all channels if there is no selection
           */
            for (c = 0; c < info->nchan; c++) {
                if (!this_buf->ch_filtered[c]) {
                    /*
               * Do not filter stimulus channels
               */
                    dc_offset = 0.0;
                    if (info->chInfo[c].kind == FIFFV_STIM_CH)
                        filter->filter_on = false;
                    else if (dc.size() > 0)
                        dc_offset = dc[c];
                    if (mne_apply_filter(*filter, filter_data.get(), this_buf->vals.row(c).data(), this_buf->ns, true,
                                         dc_offset, info->chInfo[c].kind) != OK) {
                        filter->filter_on = filter_was;
                        return FAIL;
                    }
                    this_buf->ch_filtered[c] = true;
                    filter->filter_on = filter_was;
                }
            }
        }
        /*
         * Decide the picking limits
         */
        if (firsts >= this_buf->firsts) {
            bs1 = firsts - this_buf->firsts;
            s1 = 0;
        } else {
            bs1 = 0;
            s1 = this_buf->firsts - firsts;
        }
        if (lasts >= this_buf->lasts) {
            bs2 = this_buf->ns;
            s2 = this_buf->lasts - lasts + ns;
        } else {
            bs2 = lasts - this_buf->lasts + this_buf->ns;
            s2 = ns;
        }
#ifdef DEBUG
        mne::Log::debug("buf  : {}..{} {}\n", bs1, bs2, bs2 - bs1);
        mne::Log::debug("dest : {}..{} {}\n", s1, s2, s2 - s1);
#endif
        /*
         * Then pick data from all relevant channels
         */
        if (sel) {
            if (use_deriv) {
                /*
             * Compute derived data if we need it
             */
                if (deriv_ns < this_buf->ns || nderiv != deriv_matched->deriv_data->nrow()) {
                    deriv_vals.resize(deriv_matched->deriv_data->nrow(), this_buf->ns);
                    nderiv = deriv_matched->deriv_data->nrow();
                    deriv_ns = this_buf->ns;
                }
                if (mne_sparse_mat_mult2(&deriv_matched->deriv_data->data, this_buf->vals, deriv_vals) == FAIL)
                    return FAIL;
            }
            for (c = 0; c < sel->nchan(); c++) {
                /*
             * First the ordinary channels
             */
                if (sel->pick[c] >= 0) {
                    values = this_buf->vals.row(sel->pick[c]).data();
                    for (s = s1, bs = bs1; s < s2; s++, bs++)
                        picked(c, s) += values[bs];
                } else if (sel->pick_deriv[c] >= 0 && deriv_matched) {
                    for (s = s1, bs = bs1; s < s2; s++, bs++)
                        picked(c, s) += deriv_vals(sel->pick_deriv[c], bs);
                }
            }
        } else {
            for (c = 0; c < info->nchan; c++) {
                values = this_buf->vals.row(c).data();
                for (s = s1, bs = bs1; s < s2; s++, bs++)
                    picked(c, s) += values[bs];
            }
        }
    }
    return OK;
}

//=============================================================================================================

MNERawData* MNERawData::open_file_comp(const std::filesystem::path& name,
                                       int omit_skip,
                                       int allow_maxshield,
                                       const MNEFilterDef& filter,
                                       int comp_set)
/*
     * Open a raw data file
     */
{
    std::unique_ptr<MNERawInfo> info;
    std::unique_ptr<MNERawData> data;

    auto stream = std::make_unique<FiffStream>(name);

    std::optional<FiffTag> t_pTag;
    int k, nbuf;
    int current_dir0 = 0;


    if (MNERawInfo::load(name, allow_maxshield, info) == FAIL)
        return nullptr;

    for (k = 0; k < info->nchan; k++) {
        FiffChInfo& ch = info->chInfo[k];
        if (mne::same_channel(ch.ch_name, MNE_DEFAULT_TRIGGER_CH)) {
            if (std::fabs(1.0 - ch.range) > 1e-5) {
                ch.range = 1.0;
                mne::Log::info("{} range set to {:f}\n", MNE_DEFAULT_TRIGGER_CH, ch.range);
            }
        }
        /*
         * Take care of the nonzero unit multiplier
         */
        if (ch.unit_mul != 0) {
            ch.cal = pow(10.0, static_cast<double>(ch.unit_mul)) * ch.cal;
            mne::Log::info("Ch {} unit multiplier {} -> 0\n", ch.ch_name, ch.unit_mul);
            ch.unit_mul = 0;
        }
    }
    if (!stream->open())
        return nullptr;

    data = std::make_unique<MNERawData>();
    data->filename = utf8_from_path(name);
    data->stream = std::move(stream);
    data->info = std::move(info);
    /*
       * Add the channel name list
       */
    data->ch_names.clear();
    for (int i = 0; i < data->info->nchan; i++)
        data->ch_names.push_back(data->info->chInfo[i].ch_name);
    if (static_cast<int>(data->ch_names.size()) != data->info->nchan) {
        mne::Log::critical("Channel names were not translated correctly into a name list");
        return nullptr;
    }
    /*
       * Compensation data
       */
    data->comp = MNECTFCompDataSet::read(data->filename);
    if (data->comp) {
        if (data->comp->ncomp > 0)
            mne::Log::info("Read {} compensation data sets from {}\n", data->comp->ncomp, data->filename);
        else
            mne::Log::info("No compensation data in {}\n", data->filename);
    } else
        mne::Log::warning("err_print_error()");
    if ((data->comp_file = MNECTFCompDataSet::get_comp(data->info->chInfo, data->info->nchan)) == FAIL)
        return nullptr;
    mne::Log::info("Compensation in file : {}\n", MNECTFCompDataSet::explain_comp(MNECTFCompDataSet::map_comp_kind(data->comp_file)));
    if (comp_set < 0)
        data->comp_now = data->comp_file;
    else
        data->comp_now = comp_set;

    if (!data->comp) {
        if (data->comp_now != MNE_CTFV_NOGRAD) {
            mne::Log::critical("Cannot do compensation because compensation data are missing");
            return nullptr;
        }
    } else if (data->comp->set_compensation(data->comp_now,
                                            data->info->chInfo,
                                            data->info->nchan,
                                            std::vector<mne::FiffChInfo>(),
                                            0) == FAIL)
        return nullptr;
    /*
       * SSS data
       */
    data->sss = MNESssData::read(name);
    if (data->sss && data->sss->job != FIFFV_SSS_JOB_NOTHING && data->sss->comp_info.size() > 0) {
        mne::Log::info("SSS data read from {} :\n", data->filename);
        std::ostringstream report;
        data->sss->print(report);
        mne::Log::info("{}", report.str());
    } else {
        mne::Log::info("No SSS data in {}\n", data->filename);
        data->sss.reset();
    }
    /*
       * Buffers
       */
    const std::vector<FiffDirEntry>& dir0 = data->info->rawDir;
    /*
       * Take into account the first sample
       */
    if (dir0[current_dir0].kind == FIFF_FIRST_SAMPLE) {
        if (t_pTag = data->stream->read_tag(dir0[current_dir0].pos); !t_pTag)
            return nullptr;
        data->first_samp = *t_pTag->to_int();
        current_dir0++;
    }
    if (dir0[current_dir0].kind == FIFF_DATA_SKIP) {
        int nsamp_skip;
        if (t_pTag = data->stream->read_tag(dir0[current_dir0].pos); !t_pTag)
            return nullptr;
        nsamp_skip = data->info->buf_size * (*t_pTag->to_int());
        mne::Log::info("Data skip of {} samples in the beginning\n", nsamp_skip);
        current_dir0++;
        if (dir0[current_dir0].kind == FIFF_FIRST_SAMPLE) {
            if (t_pTag = data->stream->read_tag(dir0[current_dir0].pos); !t_pTag)
                return nullptr;
            data->first_samp += *t_pTag->to_int();
            current_dir0++;
        }
        if (omit_skip) {
            data->omit_samp = data->first_samp + nsamp_skip;
            data->omit_samp_old = nsamp_skip;
            data->first_samp = 0;
        } else {
            data->first_samp = data->first_samp + nsamp_skip;
        }
    } else if (omit_skip) {
        data->omit_samp = data->first_samp;
        data->first_samp = 0;
    }
#ifdef DEBUG
    mne::Log::info("data->first_samp = {}\n", data->first_samp);
#endif
    /*
       * Figure out the buffers
       */
    for (k = current_dir0, nbuf = 0; k < static_cast<int>(dir0.size()); k++)
        if (dir0[k].kind == FIFF_DATA_BUFFER ||
            dir0[k].kind == FIFF_DATA_SKIP)
            nbuf++;
    data->bufs.resize(nbuf);

    for (k = current_dir0, nbuf = 0; k < static_cast<int>(dir0.size()); k++)
        if (dir0[k].kind == FIFF_DATA_BUFFER ||
            dir0[k].kind == FIFF_DATA_SKIP) {
            data->bufs[nbuf].ns = 0;
            data->bufs[nbuf].ent = dir0[k];
            data->bufs[nbuf].nchan = data->info->nchan;
            data->bufs[nbuf].is_skip = dir0[k].kind == FIFF_DATA_SKIP;
            data->bufs[nbuf].valid = false;
            data->bufs[nbuf].comp_status = data->comp_file;
            nbuf++;
        }
    data->nsamp = 0;
    for (k = 0; k < nbuf; k++) {
        const FiffDirEntry& dir = data->bufs[k].ent;
        if (dir.kind == FIFF_DATA_BUFFER) {
            if (dir.type == FIFFT_DAU_PACK16 || dir.type == FIFFT_SHORT)
                data->bufs[k].ns = static_cast<int>(dir.size / (data->info->nchan * sizeof(fiff_dau_pack16_t)));
            else if (dir.type == FIFFT_FLOAT)
                data->bufs[k].ns = static_cast<int>(dir.size / (data->info->nchan * sizeof(fiff_float_t)));
            else if (dir.type == FIFFT_INT)
                data->bufs[k].ns = static_cast<int>(dir.size / (data->info->nchan * sizeof(fiff_int_t)));
            else if (dir.type == FIFFT_DOUBLE)
                data->bufs[k].ns = static_cast<int>(dir.size / (data->info->nchan * sizeof(double)));
            else {
                mne::Log::critical("We are not prepared to handle raw data type: {}", dir.type);
                return nullptr;
            }
        } else if (dir.kind == FIFF_DATA_SKIP) {
            if (t_pTag = data->stream->read_tag(dir.pos); !t_pTag)
                return nullptr;
            data->bufs[k].ns = data->info->buf_size * (*t_pTag->to_int());
        }
        data->bufs[k].firsts = k == 0 ? data->first_samp : data->bufs[k - 1].lasts + 1;
        data->bufs[k].lasts = data->bufs[k].firsts + data->bufs[k].ns - 1;
        data->nsamp += data->bufs[k].ns;
    }
    /*
       * Set up the first sample values
       */
    data->bad = Eigen::VectorXi::Zero(data->info->nchan);
    data->offsets = Eigen::VectorXf::Zero(data->info->nchan);
    /*
        * Th bad channel stuff
        */
    data->badlist = data->stream->read_bad_channels(data->stream->dirtree());
    for (const auto& bad_name : data->badlist) {
        for (k = 0; k < data->info->nchan; k++) {
            if (mne::iequals(data->info->chInfo[k].ch_name, bad_name)) {
                data->bad[k] = 1;
                break;
            }
        }
    }
    mne::Log::info("{} bad channels read from {}{}{}", data->badlist.size(), mne::utf8_from_path(name),
                   data->badlist.empty() ? "" : ": ", mne::join(data->badlist, " "));
    /*
       * Initialize the raw data buffers
       */
    nbuf = static_cast<int>(approx_ring_buf_size / (data->info->buf_size * static_cast<std::size_t>(data->info->nchan) * sizeof(float)));
    data->ring = std::make_unique<RingBuffer>(nbuf);
    /*
       * Initialize the filter buffers
       */
    data->filter = std::make_unique<MNEFilterDef>(filter);
    data->setup_filter_bufs();

    {
        RowMajorMatrixXf first(data->info->nchan, 1);
        if (data->pick_data(nullptr, data->first_samp, first) == FAIL)
            return nullptr;
        data->first_sample_val = first.col(0);
        mne::Log::info("Initial dc offsets determined\n");
    }
    mne::Log::info("Raw data file {}:\n", mne::utf8_from_path(name));
    mne::Log::info("\tnchan  = {}\n", data->info->nchan);
    mne::Log::info("\tnsamp  = {}\n", data->nsamp);
    mne::Log::info("\tsfreq  = {:<8.3f} Hz\n", data->info->sfreq);
    mne::Log::info("\tlength = {:<8.3f} sec\n", data->nsamp / data->info->sfreq);

    return data.release();
}

//=============================================================================================================

MNERawData* MNERawData::open_file(const std::filesystem::path& name, int omit_skip, int allow_maxshield, const MNEFilterDef& filter)
/*
     * Wrapper for open_file to work as before
     */
{
    return open_file_comp(name, omit_skip, allow_maxshield, filter, -1);
}

//=============================================================================================================

int MNERawData::attachDerivations(const MNEDerivSet& derivations, bool keepPrevious)
{
    if (!keepPrevious || !deriv) {
        deriv = std::make_unique<MNEDerivSet>();
    }
    deriv->append(derivations);
    deriv_matched = deriv->match(ch_names);
    if (!deriv_matched) {
        mne::Log::info("No derivations are valid for these raw data.");
        return 0;
    }
    const int nvalid = deriv_matched->validate(info->chInfo);
    mne::Log::info("{} of {} of the matched derivations are valid for these raw data.", nvalid, deriv_matched->deriv_data->nrow());
    return nvalid;
}
