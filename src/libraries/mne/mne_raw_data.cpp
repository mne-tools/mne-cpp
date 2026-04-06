//=============================================================================================================
/**
 * @file     mne_raw_data.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the MNERawData Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_raw_data.h"

#include <QFile>
#include <QTextStream>

#include <Eigen/Core>

#define _USE_MATH_DEFINES
#include <math.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef FAIL
#define FAIL -1
#endif

#ifndef OK
#define OK 0
#endif

#if defined(_WIN32) || defined(_WIN64)
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

namespace MNELIB
{

/**
 * @brief Circular ring buffer for managing raw-data matrix allocations.
 *
 * Each slot tracks a single RowMajorMatrixXf.  When a slot is reused the
 * previous occupant is evicted by resizing it to 0x0, which releases its
 * heap storage while keeping the Eigen object itself alive.
 */
struct RingBuffer
{
    struct Entry {
        MNERawBufDef::RowMajorMatrixXf *user = nullptr;
    };

    std::vector<Entry> entries;
    int next = 0;

    explicit RingBuffer(int nslots)
        : entries(static_cast<size_t>(nslots))
        , next(0)
    {}

    /** Allocate (or reclaim) an entry for @p res with dimensions @p nrow x @p ncol. */
    void allocate(int nrow, int ncol, MNERawBufDef::RowMajorMatrixXf *res)
    {
        if (next >= static_cast<int>(entries.size()))
            next = 0;
        Entry &e = entries[static_cast<size_t>(next++)];
        if (e.user)          // evict old occupant
            e.user->resize(0, 0);
        res->resize(nrow, ncol);
        e.user = res;
    }
};

}

void mne_free_name_list_36(char **list, int nlist)
/*
 * Free a name list array
 */
{
    int k;
    if (list == NULL || nlist == 0)
        return;
    for (k = 0; k < nlist; k++) {
#ifdef FOO
        printf("%d %s\n",k,list[k]);
#endif
        delete[] list[k];
    }
    delete[] list;
    return;
}

//============================= misc_util.c =============================

void mne_string_to_name_list(const QString& s, QStringList& listp,int &nlistp)
/*
 * Convert a colon-separated list into a string array
 */
{
    QStringList list;

    if (!s.isEmpty() && s.size() > 0) {
        list = FIFFLIB::FiffStream::split_name_list(s);
        //list = s.split(":");
    }
    listp  = list;
    nlistp = list.size();
    return;
}

QString mne_name_list_to_string(const QStringList& list)
/*
 * Convert a string array to a colon-separated string
 */
{
    int nlist = list.size();
    QString res;
    if (nlist == 0 || list.isEmpty())
        return res;
//    res[0] = '\0';
    for (int k = 0; k < nlist-1; k++) {
        res += list[k];
        res += ":";
    }
    res += list[nlist-1];
    return res;
}

QString mne_channel_names_to_string(const QList<FIFFLIB::FiffChInfo>& chs,
                                    int nch)
/*
 * Make a colon-separated string out of channel names
 */
{
    QStringList names;
    QString res;
    if (nch <= 0)
        return res;
    for (int k = 0; k < nch; k++)
        names.append(chs[k].ch_name);
    res = mne_name_list_to_string(names);
    return res;
}

void mne_channel_names_to_name_list(const QList<FIFFLIB::FiffChInfo>& chs,
                                    int nch,
                                    QStringList& listp,
                                    int &nlistp)

{
    QString s = mne_channel_names_to_string(chs,nch);
    mne_string_to_name_list(s,listp,nlistp);
    return;
}

//============================= mne_apply_filter.c =============================

namespace MNELIB
{

/**
 * @brief Pre-computed frequency-domain filter state used for FFT-based raw data filtering.
 *
 * Holds the frequency response arrays (normal + EOG) and an FFT
 * pre-calculation buffer.  Owned by MNERawData via unique_ptr.
 */
struct FilterData
{
    std::vector<float> freq_resp;      /**< Frequency response. */
    std::vector<float> eog_freq_resp;  /**< Frequency response (EOG). */
    std::vector<float> precalc;        /**< Pre-calculated data for FFT. */

    explicit FilterData(int resp_size)
        : freq_resp(static_cast<size_t>(resp_size), 1.0f)
        , eog_freq_resp(static_cast<size_t>(resp_size), 1.0f)
    {}
};

}

int mne_compare_filters(const MNEFilterDef& f1,
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

void mne_fft_ana(float *data, int np, std::vector<float>& /*precalc*/)
/*
      * FFT analysis for real data
      */
{
    Q_UNUSED(data);
    Q_UNUSED(np);
    printf("##################### DEBUG Error: FFT analysis needs to be implemented");
    return;
}

void mne_fft_syn(float *data, int np, std::vector<float>& /*precalc*/)
/*
      * FFT synthesis for real data
      */
{
    Q_UNUSED(data);
    Q_UNUSED(np);
    printf("##################### DEBUG Error: FFT synthesis needs to be implemented");
    return;
}

int mne_apply_filter(const MNEFilterDef& filter, FilterData *d, float *data, int ns, int zero_pad, float dc_offset, int kind)
/*
 * Do the magick trick
 */
{
    int   k,p,n;
    float *freq_resp;

    if (ns != filter.size + 2*filter.taper_size) {
        printf("Incorrect data length in apply_filter");
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
    if (!filter.filter_on)	/* Nothing else to do */
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
    mne_fft_ana(data,ns,d->precalc);
    /*
   * Multiply with the frequency response
   * See FFTpack doc for details of the arrangement
   */
    n = ns % 2 == 0 ? ns/2 : (ns+1)/2;
    p = 0;
    /*
   * No imaginary part for the DC component
   */
    if (kind == FIFFV_EOG_CH)
        freq_resp = d->eog_freq_resp.data();
    else
        freq_resp = d->freq_resp.data();
    data[p] = data[p]*freq_resp[0]; p++;
    /*
   * The other components
   */
    for (k = 1 ; k < n ; k++) {
        data[p] = data[p]*freq_resp[k]; p++;
        data[p] = data[p]*freq_resp[k]; p++;
    }
    /*
   * Then the last value
   */
    if (ns % 2 == 0)
        data[p] = data[p]*freq_resp[k];

    mne_fft_syn(data,ns,d->precalc);

    return OK;
}

std::unique_ptr<FilterData> mne_create_filter_response(const MNEFilterDef&   filter,
                                float           sfreq,
                                int             *highpass_effective)
/*
      * Create a frequency response
      */
{
    int resp_size;
    int k,s,w,f;
    int highpasss,lowpasss;
    int highpass_widths,lowpass_widths;
    float lowpass,highpass,lowpass_width,highpass_width;
    float *freq_resp;
    float pi4 = M_PI/4.0;
    float mult,add,c;

    resp_size = (filter.size + 2*filter.taper_size)/2 + 1;

    auto filter_data = std::make_unique<FilterData>(resp_size);
     *highpass_effective = FALSE;

    for (f = 0; f < 2; f++) {
        highpass       = f == 0 ? filter.highpass  : filter.eog_highpass;
        highpass_width = f == 0 ? filter.highpass_width : filter.eog_highpass_width;
        lowpass        = f == 0 ? filter.lowpass   : filter.eog_lowpass;
        lowpass_width  = f == 0 ? filter.lowpass_width  : filter.eog_lowpass_width;
        freq_resp      = f == 0 ? filter_data->freq_resp.data() : filter_data->eog_freq_resp.data();
        /*
     * Start simple first
     */
        highpasss = ((resp_size-1)*highpass)/(0.5*sfreq);
        lowpasss = ((resp_size-1)*lowpass)/(0.5*sfreq);

        lowpass_widths = ((resp_size-1)*lowpass_width)/(0.5*sfreq);
        lowpass_widths = (lowpass_widths+1)/2;    /* What user specified */

        if (filter.highpass_width > 0.0) {
            highpass_widths = ((resp_size-1)*highpass_width)/(0.5*sfreq);
            highpass_widths  = (highpass_widths+1)/2;    /* What user specified */
        }
        else
            highpass_widths = 3;	   	             /* Minimal */

        if (filter.filter_on) {
            printf("filter : %7.3f ... %6.1f Hz   bins : %d ... %d of %d hpw : %d lpw : %d\n",
                    highpass,
                    lowpass,
                    highpasss,
                    lowpasss,
                    resp_size,
                    highpass_widths,
                    lowpass_widths);
        }
        if (highpasss > highpass_widths + 1) {
            w    = highpass_widths;
            mult = 1.0/w;
            add  = 3.0;
            for (k = 0; k < highpasss-w+1; k++)
                freq_resp[k] = 0.0;
            for (k = -w+1, s = highpasss-w+1; k < w; k++, s++) {
                if (s >= 0 && s < resp_size) {
                    c = cos(pi4*(k*mult+add));
                    freq_resp[s] = freq_resp[s]*c*c;
                }
            }
            *highpass_effective = TRUE;
        }
        else
            *highpass_effective = *highpass_effective || (filter.highpass == 0.0);

        if (lowpass_widths > 0) {
            w    = lowpass_widths;
            mult = 1.0/w;
            add  = 1.0;
            for (k = -w+1, s = lowpasss-w+1; k < w; k++, s++) {
                if (s >= 0 && s < resp_size) {
                    c = cos(pi4*(k*mult+add));
                    freq_resp[s] = freq_resp[s]*c*c;
                }
            }
            for (k = s; k < resp_size; k++)
                freq_resp[k] = 0.0;
        }
        else {
            for (k = lowpasss; k < resp_size; k++)
                freq_resp[k] = 0.0;
        }
        if (filter.filter_on) {
            if (*highpass_effective)
                printf("Highpass filter will work as specified.\n");
            else
                printf("NOTE: Highpass filter omitted due to a too low corner frequency.\n");
        }
        else
            printf("NOTE: Filter is presently switched off.\n");
    }
    return filter_data;
}

//============================= mne_raw_routines.c =============================

int mne_read_raw_buffer_t(//fiffFile     in,        /* Input file */
                          FiffStream::SPtr& stream,
                          const FiffDirEntry::SPtr& ent,         /* The directory entry to read */
                          MNERawBufDef::RowMajorMatrixXf& data,  /* Matrix [npick x nsamp] to fill */
                          int          nchan,       /* Number of channels in the data */
                          int          nsamp,       /* Expected number of samples */
                          const QList<FIFFLIB::FiffChInfo>&   chs,         /* Channel info for ALL channels */
                          int          *pickno,     /* Which channels to pick */
                          int          npick)       /* How many */

{
    FiffTag::UPtr t_pTag;
//    fiffTagRec   tag;
    fiff_short_t *this_samples;
    fiff_float_t *this_samplef;
    fiff_int_t   *this_sample;

    int s,c;
    int do_all;
    float *mult;

//    tag.data = NULL;

    if (npick == 0) {
        pickno = new int[nchan];
        for (c = 0; c < nchan; c++)
            pickno[c] = c;
        do_all = TRUE;
        npick = nchan;
    }
    else
        do_all = FALSE;

    mult = new float[npick];
    for (c = 0; c < npick; c++)
        mult[c] = chs[pickno[c]].cal*chs[pickno[c]].range;

//    if (fiff_read_this_tag(in->fd,ent->pos,&tag) ==  FIFF_FAIL)
//        goto bad;
    if (!stream->read_tag(t_pTag,ent->pos))
        goto bad;

    if (ent->type == FIFFT_FLOAT) {
        if ((int)(t_pTag->size()/(sizeof(fiff_float_t)*nchan)) != nsamp) {
            printf("Incorrect number of samples in buffer.");
            goto bad;
        }
        qDebug() << "ToDo: Check whether this_samplef contains the right stuff!!! - use VectorXf instead";
        this_samplef = t_pTag->toFloat();
        for (s = 0; s < nsamp; s++, this_samplef += nchan) {
            for (c = 0; c < npick; c++)
                data(c,s) = mult[c]*this_samplef[pickno[c]];
        }
    }
    else if (ent->type == FIFFT_SHORT || ent->type == FIFFT_DAU_PACK16) {
        if ((int)(t_pTag->size()/(sizeof(fiff_short_t)*nchan)) != nsamp) {
            printf("Incorrect number of samples in buffer.");
            goto bad;
        }
        qDebug() << "ToDo: Check whether this_samples contains the right stuff!!! - use VectorXi instead";
        this_samples = (fiff_short_t *)t_pTag->data();
        for (s = 0; s < nsamp; s++, this_samples += nchan) {
            for (c = 0; c < npick; c++)
                data(c,s) = mult[c]*this_samples[pickno[c]];
        }
    }
    else if (ent->type == FIFFT_INT) {
        if ((int)(t_pTag->size()/(sizeof(fiff_int_t)*nchan)) != nsamp) {
            printf("Incorrect number of samples in buffer.");
            goto bad;
        }
        qDebug() << "ToDo: Check whether this_sample contains the right stuff!!! - use VectorXi instead";
        this_sample = t_pTag->toInt();
        for (s = 0; s < nsamp; s++, this_sample += nchan) {
            for (c = 0; c < npick; c++)
                data(c,s) = mult[c]*this_sample[pickno[c]];
        }
    }
    else {
        printf("We are not prepared to handle raw data type: %d",ent->type);
        goto bad;
    }
    if (do_all)
        delete[] pickno;
    delete[] mult;
//    FREE_36(tag.data);
    return OK;

bad : {
        if (do_all)
            delete[] pickno;
//        FREE_36(tag.data);
        return FAIL;
    }
}

//============================= mne_process_bads.c =============================

int mne_read_bad_channel_list_from_node(FiffStream::SPtr& stream,
                                        const FiffDirNode::SPtr& pNode, QStringList& listp, int& nlistp)
{
    FiffDirNode::SPtr node,bad;
    QList<FiffDirNode::SPtr> temp;
    QStringList list;
    int  nlist  = 0;
    FiffTag::UPtr t_pTag;
    QString names;

    if (pNode->isEmpty())
        node = stream->dirtree();
    else
        node = pNode;

    temp = node->dir_tree_find(FIFFB_MNE_BAD_CHANNELS);
    if (temp.size() > 0) {
        bad = temp[0];

        bad->find_tag(stream, FIFF_MNE_CH_NAME_LIST, t_pTag);
        if (t_pTag) {
            names = t_pTag->toString();
            mne_string_to_name_list(names,list,nlist);
        }
    }
    listp = list;
    nlistp = nlist;
    return OK;
}

int mne_read_bad_channel_list(const QString& name, QStringList& listp, int& nlistp)

{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    int res;

    if(!stream->open())
        return FAIL;

    res = mne_read_bad_channel_list_from_node(stream,stream->dirtree(),listp,nlistp);

    stream->close();

    return res;
}

int  mne_sparse_vec_mult2(FiffSparseMatrix* mat,     /* The sparse matrix */
                          float           *vector, /* Vector to be multiplied */
                          float           *res)    /* Result of the multiplication */
/*
      * Multiply a vector by a sparse matrix.
      */
{
    int i,j;

    if (mat->coding == FIFFTS_MC_RCS) {
        for (i = 0; i < mat->m; i++) {
            res[i] = 0.0;
            for (j = mat->ptrs[i]; j < mat->ptrs[i+1]; j++)
                res[i] += mat->data[j]*vector[mat->inds[j]];
        }
        return 0;
    }
    else if (mat->coding == FIFFTS_MC_CCS) {
        for (i = 0; i < mat->m; i++)
            res[i] = 0.0;
        for (i = 0; i < mat->n; i++)
            for (j = mat->ptrs[i]; j < mat->ptrs[i+1]; j++)
                res[mat->inds[j]] += mat->data[j]*vector[i];
        return 0;
    }
    else {
        qWarning("mne_sparse_vec_mult2: unknown sparse matrix storage type: %d",mat->coding);
        return -1;
    }
}

int  mne_sparse_mat_mult2(FiffSparseMatrix* mat,     /* The sparse matrix */
                          const MNERawBufDef::RowMajorMatrixXf& mult,  /* Matrix to be multiplied */
                          int             ncol,	   /* How many columns in the above */
                          MNERawBufDef::RowMajorMatrixXf& res)   /* Result of the multiplication */
/*
      * Multiply a dense matrix by a sparse matrix.
      */
{
    int i,j,k;
    float val;

    if (mat->coding == FIFFTS_MC_RCS) {
        for (i = 0; i < mat->m; i++) {
            for (k = 0; k < ncol; k++) {
                val = 0.0;
                for (j = mat->ptrs[i]; j < mat->ptrs[i+1]; j++)
                    val += mat->data[j]*mult(mat->inds[j],k);
                res(i,k) = val;
            }
        }
    }
    else if (mat->coding == FIFFTS_MC_CCS) {
        for (k = 0; k < ncol; k++) {
            for (i = 0; i < mat->m; i++)
                res(i,k) = 0.0;
            for (i = 0; i < mat->n; i++)
                for (j = mat->ptrs[i]; j < mat->ptrs[i+1]; j++)
                    res(mat->inds[j],k) += mat->data[j]*mult(i,k);
        }
    }
    else {
        qWarning("mne_sparse_mat_mult2: unknown sparse matrix storage type: %d",mat->coding);
        return -1;
    }
    return 0;
}

#define APPROX_RING_BUF_SIZE (600*1024*1024)

static int approx_ring_buf_size = APPROX_RING_BUF_SIZE;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNERawData::MNERawData()
:info(nullptr)
,nbad(0)
,first_samp(0)
,omit_samp(0)
,first_samp_old(0)
,omit_samp_old(0)
,nsamp(0)
,proj(nullptr)
,sss(nullptr)
,comp(nullptr)
,comp_file(MNE_CTFV_NOGRAD)
,comp_now(MNE_CTFV_NOGRAD)
,max_event(0)
,dig_trigger_mask(0)
,deriv(nullptr)
,deriv_matched(nullptr)
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

void MNERawData::add_filter_response(int *highpass_effective)
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
    MNEFilterDef* filter = this->filter.get();
    int       nfilt_buf;
    int       k;
    int       firstsamp;
    int       nring_buf;
    int       highpass_effective;

    this->filt_bufs.clear();
    this->filt_ring.reset();

    if (!this->filter || filter->size <= 0)
        return;

    for (nfilt_buf = 0, firstsamp = this->first_samp-filter->taper_size;
         firstsamp < this->nsamp + this->first_samp;
         firstsamp = firstsamp + filter->size)
        nfilt_buf++;
#ifdef DEBUG
    printf("%d filter buffers needed\n",nfilt_buf);
#endif
    this->filt_bufs.resize(nfilt_buf);
    for (k = 0, firstsamp = this->first_samp-filter->taper_size; k < nfilt_buf; k++,
         firstsamp = firstsamp + filter->size) {
        filt_bufs[k].ns          = filter->size + 2*filter->taper_size;
        filt_bufs[k].firsts      = firstsamp;
        filt_bufs[k].lasts       = firstsamp + filt_bufs[k].ns - 1;
        //        bufs[k].ent         = NULL;
        filt_bufs[k].nchan       = this->info->nchan;
        filt_bufs[k].is_skip     = FALSE;
        filt_bufs[k].valid       = FALSE;
        filt_bufs[k].ch_filtered = Eigen::VectorXi::Zero(this->info->nchan);
        filt_bufs[k].comp_status = MNE_CTFV_NOGRAD;
    }
    nring_buf       = approx_ring_buf_size/((2*filter->taper_size+filter->size)*
                                            this->info->nchan*sizeof(float));
    this->filt_ring = std::make_unique<RingBuffer>(nring_buf);
    add_filter_response(&highpass_effective);

    return;
}

//=============================================================================================================

int MNERawData::load_one_buffer(MNERawBufDef *buf)
/*
     * load just one
     */
{
    if (buf->ent->kind == FIFF_DATA_SKIP) {
        printf("Cannot load a skip");
        return FAIL;
    }
    if (buf->vals.size() == 0) {	/* The data space may have been reused */
        buf->valid = FALSE;
        ring->allocate(buf->nchan,buf->ns,&buf->vals);
    }
    if (buf->valid)
        return OK;

#ifdef DEBUG
    printf("Read buffer %d .. %d\n",buf->firsts,buf->lasts);
#endif

    if (mne_read_raw_buffer_t(stream,
                              buf->ent,
                              buf->vals,
                              buf->nchan,
                              buf->ns,
                              info->chInfo,
                              NULL,0) != OK) {
        buf->valid = FALSE;
        return FAIL;
    }
    buf->valid       = TRUE;
    buf->comp_status = comp_file;
    return OK;
}

//=============================================================================================================

int MNERawData::compensate_buffer(MNERawBufDef *buf)
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
        Eigen::MatrixXf dataMat = buf->vals;  /* implicit copy/conversion */

        if (comp->undo) {
            std::swap(comp->current, comp->undo);
            /*
             * Undo the previous compensation
             */
            if (comp->apply_transpose(FALSE, dataMat) != OK) {
                std::swap(comp->current, comp->undo);
                goto bad;
            }
            std::swap(comp->current, comp->undo);
        }
        if (comp->current) {
            /*
             * Apply new compensation
             */
            if (comp->apply_transpose(TRUE, dataMat) != OK)
                goto bad;
        }
        /*
         * Copy result back to buf->vals
         */
        buf->vals = dataMat;
    }
    buf->comp_status = comp_now;
    return OK;

bad :
    return FAIL;
}

//=============================================================================================================

int MNERawData::pick_data(mneChSelection sel, int firsts, int ns, float **picked)
/*
     * Data from a selection of channels
     */
{
    int          k,s,p,start,c,fills;
    int          ns2,s2;
    MNERawBufDef* this_buf;
    float        *values;
    int          need_some;

    MNERawBufDef::RowMajorMatrixXf deriv_vals;
    int          deriv_ns     = 0;
    int          nderiv       = 0;

    if (firsts < first_samp) {
        for (s = 0, p = firsts; p < first_samp; s++, p++) {
            if (sel)
                for (c = 0; c < sel->nchan; c++)
                    picked[c][s] = 0.0;
            else
                for (c = 0; c < info->nchan; c++)
                    picked[c][s] = 0.0;
        }
        ns     = ns - s;
        firsts = first_samp;
    }
    else
        s = 0;
    /*
       * There is possibly nothing to do
       */
    if (sel) {
        for (c = 0, need_some = FALSE; c < sel->nchan; c++) {
            if (sel->pick[c] >= 0 || sel->pick_deriv[c] >= 0) {
                need_some = TRUE;
                break;
            }
        }
        if (!need_some)
            return OK;
    }
    /*
       * Have to to the hard work
       */
    for (k = 0, this_buf = bufs.data(), s = 0; k < (int)bufs.size(); k++, this_buf++) {
        if (this_buf->lasts >= firsts) {
            start = firsts - this_buf->firsts;
            if (start < 0)
                start = 0;
            if (this_buf->is_skip) {
                for (p = start; p < this_buf->ns && ns > 0; p++, ns--, s++) {
                    if (sel) {
                        for (c = 0; c < sel->nchan; c++)
                            if (sel->pick[c] >= 0)
                                picked[c][s] = 0.0;
                    }
                    else {
                        for (c = 0; c < info->nchan; c++)
                            picked[c][s] = 0.0;
                    }
                }
            }
            else {
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
                    if (sel->nderiv > 0 && deriv_matched) {
                        if (deriv_ns < this_buf->ns || nderiv != deriv_matched->deriv_data->nrow) {
                            deriv_vals.resize(deriv_matched->deriv_data->nrow, this_buf->ns);
                            nderiv      = deriv_matched->deriv_data->nrow;
                            deriv_ns    = this_buf->ns;
                        }
                        if (mne_sparse_mat_mult2(deriv_matched->deriv_data->data.get(),this_buf->vals,this_buf->ns,deriv_vals) == FAIL) {
                            return FAIL;
                        }
                    }
                    for (c = 0; c < sel->nchan; c++) {
                        /*
                 * First pick the ordinary channels...
                 */
                        if (sel->pick[c] >= 0) {
                            for (p = start, s2 = s, ns2 = ns; p < this_buf->ns && ns2 > 0; p++, ns2--, s2++)
                                picked[c][s2] = this_buf->vals(sel->pick[c], p);
                        }
                        /*
                 * ...then the derived ones
                 */
                        else if (sel->pick_deriv[c] >= 0 && deriv_matched) {
                            for (p = start, s2 = s, ns2 = ns; p < this_buf->ns && ns2 > 0; p++, ns2--, s2++)
                                picked[c][s2] = deriv_vals(sel->pick_deriv[c], p);
                        }
                    }
                }
                else {
                    for (c = 0; c < info->nchan; c++)
                        for (p = start, s2 = s, ns2 = ns; p < this_buf->ns && ns2 > 0; p++, ns2--, s2++)
                            picked[c][s2] = this_buf->vals(c, p);
                }
                s  = s2;
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
        fills = s-1;
        for (; ns > 0; ns--, s++) {
            if (sel)
                for (c = 0; c < sel->nchan; c++)
                    picked[c][s] = picked[c][fills];
            else
                for (c = 0; c < info->nchan; c++)
                    picked[c][s] = picked[c][fills];
        }
    }
    else {
        for (; ns > 0; ns--, s++) {
            if (sel)
                for (c = 0; c < sel->nchan; c++)
                    picked[c][s] = 0;
            else
                for (c = 0; c < info->nchan; c++)
                    picked[c][s] = 0;
        }
    }
    return OK;
}

//=============================================================================================================

int MNERawData::pick_data_proj(mneChSelection sel, int firsts, int ns, float **picked)
/*
     * Data from a set of channels, apply projection
     */
{
    int          k,s,p,start,c,fills;
    MNERawBufDef* this_buf;
    MNERawBufDef::RowMajorMatrixXf *values;
    float        *pvalues;
    float        *deriv_pvalues = NULL;

    if (!proj || (sel && !proj->affect(sel->chspick,sel->nchan) && !proj->affect(sel->chspick_nospace,sel->nchan)))
        return pick_data(sel,firsts,ns,picked);

    if (firsts < first_samp) {
        for (s = 0, p = firsts; p < first_samp; s++, p++) {
            if (sel)
                for (c = 0; c < sel->nchan; c++)
                    picked[c][s] = 0.0;
            else
                for (c = 0; c < info->nchan; c++)
                    picked[c][s] = 0.0;
        }
        ns         = ns - s;
        firsts     = first_samp;
    }
    else
        s = 0;
    pvalues = new float[info->nchan];
    for (k = 0, this_buf = bufs.data(); k < (int)bufs.size(); k++, this_buf++) {
        if (this_buf->lasts >= firsts) {
            start = firsts - this_buf->firsts;
            if (start < 0)
                start = 0;
            if (this_buf->is_skip) {
                for (p = start; p < this_buf->ns && ns > 0; p++, ns--, s++) {
                    if (sel) {
                        for (c = 0; c < sel->nchan; c++)
                            if (sel->pick[c] >= 0)
                                picked[c][s] = 0.0;
                    }
                    else {
                        for (c = 0; c < info->nchan; c++)
                            picked[c][s] = 0.0;
                    }
                }
            }
            else {
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
                if (sel && sel->nderiv > 0 && deriv_matched)
                    deriv_pvalues = new float[deriv_matched->deriv_data->nrow];
                for (p = start; p < this_buf->ns && ns > 0; p++, ns--, s++) {
                    for (c = 0; c < info->nchan; c++)
                        pvalues[c] = (*values)(c,p);
                    if (proj->project_vector(pvalues,info->nchan,TRUE) != OK)
                        qWarning()<<"Error";
                    if (sel) {
                        if (sel->nderiv > 0 && deriv_matched) {
                            if (mne_sparse_vec_mult2(deriv_matched->deriv_data->data.get(),pvalues,deriv_pvalues) == FAIL)
                                return FAIL;
                        }
                        for (c = 0; c < sel->nchan; c++) {
                            /*
                   * First try the ordinary channels...
                   */
                            if (sel->pick[c] >= 0)
                                picked[c][s] = pvalues[sel->pick[c]];
                            /*
                   * ...then the derived ones
                   */
                            else if (sel->pick_deriv[c] >= 0 && deriv_matched)
                                picked[c][s] = deriv_pvalues[sel->pick_deriv[c]];
                        }
                    }
                    else {
                        for (c = 0; c < info->nchan; c++) {
                            picked[c][s] = pvalues[c];
                        }
                    }
                }
            }
            if (ns == 0)
                break;
        }
    }
    delete[] deriv_pvalues;
    delete[] pvalues;
    /*
       * Extend with the last available sample or zero if the request is beyond the data
       */
    if (s > 0) {
        fills = s-1;
        for (; ns > 0; ns--, s++) {
            if (sel)
                for (c = 0; c < sel->nchan; c++)
                    picked[c][s] = picked[c][fills];
            else
                for (c = 0; c < info->nchan; c++)
                    picked[c][s] = picked[c][fills];
        }
    }
    else {
        for (; ns > 0; ns--, s++) {
            if (sel)
                for (c = 0; c < sel->nchan; c++)
                    picked[c][s] = 0;
            else
                for (c = 0; c < info->nchan; c++)
                    picked[c][s] = 0;
        }
    }
    return OK;
}

//=============================================================================================================

int MNERawData::load_one_filt_buf(MNERawBufDef *buf)
/*
     * Load and filter one buffer
     */
{
    int k;
    int res;

    float **vals;

    if (buf->vals.size() == 0) {
        buf->valid = FALSE;
        filt_ring->allocate(buf->nchan, buf->ns,&buf->vals);
    }
    if (buf->valid)
        return OK;

    vals = new float*[buf->nchan];
    for (k = 0; k < buf->nchan; k++) {
        buf->ch_filtered[k] = FALSE;
        vals[k] = buf->vals.row(k).data() + filter->taper_size;
    }

    res = pick_data_proj(NULL,buf->firsts + filter->taper_size,buf->ns - 2*filter->taper_size,vals);

    delete[] vals;

#ifdef DEBUG
    if (res == OK)
        printf("Loaded filtered buffer %d...%d %d %d last = %d\n",
                buf->firsts,buf->lasts,buf->lasts-buf->firsts+1,buf->ns,first_samp + nsamp);
#endif
    buf->valid = res == OK;
    return res;
}

//=============================================================================================================

int MNERawData::pick_data_filt(mneChSelection sel, int firsts, int ns, float **picked)
/*
     * Data for a selection (filtered and picked)
     */
{
    int          k,s,bs,c;
    int          bs1,bs2,s1,s2,lasts;
    MNERawBufDef* this_buf;
    float        *values;
    MNERawBufDef::RowMajorMatrixXf deriv_vals;
    Eigen::VectorXf dc;
    float        dc_offset;
    int          deriv_ns     = 0;
    int          nderiv       = 0;
    int          filter_was;

    if (!filter->filter_on)
        return pick_data_proj(sel,firsts,ns,picked);

    if (sel) {
        for (s = 0; s < ns; s++)
            for (c = 0; c < sel->nchan; c++)
                picked[c][s] = 0.0;
    }
    else {
        for (s = 0; s < ns; s++)
            for (c = 0; c < info->nchan; c++)
                picked[c][s] = 0.0;
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
            if (comp->apply(TRUE,dc) != OK)
                goto bad;
        if (proj)
            if (proj->project_vector(dc.data(),info->nchan,TRUE) != OK)
                goto bad;
    }
    filter_was = filter->filter_on;
    /*
       * Find the first buffer to consider
       */
    for (k = 0, this_buf = filt_bufs.data(); k < (int)filt_bufs.size(); k++, this_buf++) {
        if (this_buf->lasts >= firsts)
            break;
    }
    for (; k < (int)filt_bufs.size() && this_buf->firsts <= lasts; k++, this_buf++) {
#ifdef DEBUG
        printf("this_buf (%d): %d..%d\n",k,this_buf->firsts,this_buf->lasts);
#endif
        /*
         * Load the buffer first and apply projection
         */
        if (load_one_filt_buf(this_buf) != OK)
            goto bad;
        /*
         * Then filter all relevant channels (not stimuli)
         */
        if (sel) {
            for (c = 0; c < sel->nchan; c++) {
                if (sel->pick[c] >= 0) {
                    if (!this_buf->ch_filtered[sel->pick[c]]) {
                        /*
                 * Do not filter stimulus channels
                 */
                        dc_offset = 0.0;
                        if (info->chInfo[sel->pick[c]].kind == FIFFV_STIM_CH)
                            filter->filter_on = FALSE;
                        else if (dc.size() > 0)
                            dc_offset = dc[sel->pick[c]];
                        if (mne_apply_filter(*filter,filter_data.get(),this_buf->vals.row(sel->pick[c]).data(),this_buf->ns,TRUE,
                                             dc_offset,info->chInfo[sel->pick[c]].kind) != OK) {
                            filter->filter_on = filter_was;
                            goto bad;
                        }
                        this_buf->ch_filtered[sel->pick[c]] = TRUE;
                        filter->filter_on = filter_was;
                    }
                }
            }
            /*
           * Also check channels included in derivations if they are used
           */
            if (sel->nderiv > 0 && deriv_matched) {
                MNEDeriv* der = deriv_matched.get();
                for (c = 0; c < der->deriv_data->ncol; c++) {
                    if (der->in_use[c] > 0 &&
                            !this_buf->ch_filtered[c]) {
                        /*
                 * Do not filter stimulus channels
                 */
                        dc_offset = 0.0;
                        if (info->chInfo[c].kind == FIFFV_STIM_CH)
                            filter->filter_on = FALSE;
                        else if (dc.size() > 0)
                            dc_offset = dc[c];
                        if (mne_apply_filter(*filter,filter_data.get(),this_buf->vals.row(c).data(),this_buf->ns,TRUE,
                                             dc_offset,info->chInfo[c].kind) != OK) {
                            filter->filter_on = filter_was;
                            goto bad;
                        }
                        this_buf->ch_filtered[c] = TRUE;
                        filter->filter_on = filter_was;
                    }
                }
            }
        }
        else {
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
                        filter->filter_on = FALSE;
                    else if (dc.size() > 0)
                        dc_offset = dc[c];
                    if (mne_apply_filter(*filter,filter_data.get(),this_buf->vals.row(c).data(),this_buf->ns,TRUE,
                                         dc_offset,info->chInfo[c].kind) != OK) {
                        filter->filter_on = filter_was;
                        goto bad;
                    }
                    this_buf->ch_filtered[c] = TRUE;
                    filter->filter_on = filter_was;
                }
            }
        }
        /*
         * Decide the picking limits
         */
        if (firsts >= this_buf->firsts) {
            bs1 = firsts - this_buf->firsts;
            s1  = 0;
        }
        else {
            bs1 = 0;
            s1  = this_buf->firsts - firsts;
        }
        if (lasts >= this_buf->lasts) {
            bs2 = this_buf->ns;
            s2  = this_buf->lasts - lasts + ns;
        }
        else {
            bs2 = lasts - this_buf->lasts + this_buf->ns;
            s2  = ns;
        }
#ifdef DEBUG
        printf("buf  : %d..%d %d\n",bs1,bs2,bs2-bs1);
        printf("dest : %d..%d %d\n",s1,s2,s2-s1);
#endif
        /*
         * Then pick data from all relevant channels
         */
        if (sel) {
            if (sel->nderiv > 0 && deriv_matched) {
                /*
             * Compute derived data if we need it
             */
                if (deriv_ns < this_buf->ns || nderiv != deriv_matched->deriv_data->nrow) {
                    deriv_vals.resize(deriv_matched->deriv_data->nrow, this_buf->ns);
                    nderiv      = deriv_matched->deriv_data->nrow;
                    deriv_ns    = this_buf->ns;
                }
                if (mne_sparse_mat_mult2(deriv_matched->deriv_data->data.get(),this_buf->vals,this_buf->ns,deriv_vals) == FAIL)
                    goto bad;
            }
            for (c = 0; c < sel->nchan; c++) {
                /*
             * First the ordinary channels
             */
                if (sel->pick[c] >= 0) {
                    values = this_buf->vals.row(sel->pick[c]).data();
                    for (s = s1, bs = bs1; s < s2; s++, bs++)
                        picked[c][s] += values[bs];
                }
                else if (sel->pick_deriv[c] >= 0 && deriv_matched) {
                    for (s = s1, bs = bs1; s < s2; s++, bs++)
                        picked[c][s] += deriv_vals(sel->pick_deriv[c], bs);
                }
            }
        }
        else {
            for (c = 0; c < info->nchan; c++) {
                values = this_buf->vals.row(c).data();
                for (s = s1, bs = bs1; s < s2; s++, bs++)
                    picked[c][s] += values[bs];
            }
        }
    }
    (void)bs2; // squash compiler warning, this is unused
    return OK;

bad : {
        return FAIL;
    }
}

//=============================================================================================================

MNERawData *MNERawData::open_file_comp(const QString& name,
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

    auto filePtr = std::make_unique<QFile>(name);
    FiffStream::SPtr stream(new FiffStream(filePtr.get()));
    //    fiffFile           in    = NULL;

    FiffDirEntry::SPtr dir;
    QList<FiffDirEntry::SPtr> dir0;
    //    fiffTagRec   tag;
    FiffTag::UPtr t_pTag;
    FiffChInfo   ch;
    int k, b, nbuf, ndir, nnames;
    int current_dir0 = 0;

    //    tag.data = NULL;

    if (MNERawInfo::load(name,allow_maxshield,info) == FAIL)
        goto bad;

    for (k = 0; k < info->nchan; k++) {
        ch = info->chInfo.at(k);
        if (QString::compare(ch.ch_name,MNE_DEFAULT_TRIGGER_CH) == 0) {
            if (std::fabs(1.0 - ch.range) > 1e-5) {
                ch.range = 1.0;
                printf("%s range set to %f\n",MNE_DEFAULT_TRIGGER_CH,ch.range);
            }
        }
        /*
         * Take care of the nonzero unit multiplier
         */
        if (ch.unit_mul != 0) {
            ch.cal = pow(10.0,(double)(ch.unit_mul))*ch.cal;
            printf("Ch %s unit multiplier %d -> 0\n",ch.ch_name.toLatin1().data(),ch.unit_mul);
            ch.unit_mul = 0;
        }
    }
    //    if ((in = fiff_open(name)) == NULL)
    //        goto bad;
    if(!stream->open())
        goto bad;

    data           = std::make_unique<MNERawData>();
    data->filename = name;
    data->file     = std::move(filePtr);
    data->stream   = stream;
    data->info = std::move(info);
    /*
       * Add the channel name list
       */
    mne_channel_names_to_name_list(data->info->chInfo,
                                   data->info->nchan,
                                   data->ch_names,
                                   nnames);
    if (nnames != data->info->nchan) {
        printf("Channel names were not translated correctly into a name list");
        goto bad;
    }
    /*
       * Compensation data
       */
    data->comp = MNECTFCompDataSet::read(data->filename);
    if (data->comp) {
        if (data->comp->ncomp > 0)
            printf("Read %d compensation data sets from %s\n",data->comp->ncomp,data->filename.toUtf8().constData());
        else
            printf("No compensation data in %s\n",data->filename.toUtf8().constData());
    }
    else
        qWarning() << "err_print_error()";
    if ((data->comp_file = MNECTFCompDataSet::get_comp(data->info->chInfo,data->info->nchan)) == FAIL)
        goto bad;
    printf("Compensation in file : %s\n",MNECTFCompDataSet::explain_comp(MNECTFCompDataSet::map_comp_kind(data->comp_file)).toUtf8().constData());
    if (comp_set < 0)
        data->comp_now = data->comp_file;
    else
        data->comp_now = comp_set;

    if (!data->comp) {
        if (data->comp_now != MNE_CTFV_NOGRAD) {
            printf("Cannot do compensation because compensation data are missing");
            goto bad;
        }
    } else if (data->comp->set_compensation(data->comp_now,
                                    data->info->chInfo,
                                    data->info->nchan,
                                    QList<FIFFLIB::FiffChInfo>(),
                                    0) == FAIL)
        goto bad;
    /*
       * SSS data
       */
    data->sss.reset(MNESssData::read(data->filename));
    if (data->sss && data->sss->job != FIFFV_SSS_JOB_NOTHING && data->sss->comp_info.size() > 0) {
        printf("SSS data read from %s :\n",data->filename.toUtf8().constData());
        QTextStream errStream(stderr);
        data->sss->print(errStream);
    }
    else {
        printf("No SSS data in %s\n",data->filename.toUtf8().constData());
        data->sss.reset();
    }
    /*
       * Buffers
       */
    dir0 = data->info->rawDir;
    ndir = data->info->ndir;
    /*
       * Take into account the first sample
       */
    if (dir0[current_dir0]->kind == FIFF_FIRST_SAMPLE) {
        //        if (fiff_read_this_tag(in->fd,dir0->pos,&tag) == FIFF_FAIL)
        //            goto bad;
        if (!stream->read_tag(t_pTag,dir0[current_dir0]->pos))
            goto bad;
        data->first_samp = *t_pTag->toInt();
        current_dir0++;
        ndir--;
    }
    if (dir0[current_dir0]->kind == FIFF_DATA_SKIP) {
        int nsamp_skip;
        //        if (fiff_read_this_tag(in->fd,dir0->pos,&tag) == FIFF_FAIL)
        //            goto bad;
        if (!stream->read_tag(t_pTag,dir0[current_dir0]->pos))
            goto bad;
        nsamp_skip = data->info->buf_size*(*t_pTag->toInt());
        printf("Data skip of %d samples in the beginning\n",nsamp_skip);
        current_dir0++;
        ndir--;
        if (dir0[current_dir0]->kind == FIFF_FIRST_SAMPLE) {
            //            if (fiff_read_this_tag(in->fd,dir0->pos,&tag) == FIFF_FAIL)
            //                goto bad;
            if (!stream->read_tag(t_pTag,dir0[current_dir0]->pos))
                goto bad;
            data->first_samp += *t_pTag->toInt();
            current_dir0++;
            ndir--;
        }
        if (omit_skip) {
            data->omit_samp     = data->first_samp + nsamp_skip;
            data->omit_samp_old = nsamp_skip;
            data->first_samp    = 0;
        }
        else {
            data->first_samp     = data->first_samp + nsamp_skip;
        }
    }
    else if (omit_skip) {
        data->omit_samp  = data->first_samp;
        data->first_samp = 0;
    }
#ifdef DEBUG
    printf("data->first_samp = %d\n",data->first_samp);
#endif
    /*
       * Figure out the buffers
       */
    //    for (k = 0, dir = dir0, nbuf = 0; k < ndir; k++, dir++)
    for (k = 0, nbuf = 0; k < ndir; k++)
        if (dir0[k]->kind == FIFF_DATA_BUFFER ||
                dir0[k]->kind == FIFF_DATA_SKIP)
            nbuf++;
    data->bufs.resize(nbuf);

    //    for (k = 0, nbuf = 0, dir = dir0; k < ndir; k++, dir++)
    for (k = 0, nbuf = 0; k < ndir; k++)
        if (dir0[k]->kind == FIFF_DATA_BUFFER ||
                dir0[k]->kind == FIFF_DATA_SKIP) {
            data->bufs[nbuf].ns          = 0;
            data->bufs[nbuf].ent         = dir0[k];
            data->bufs[nbuf].nchan       = data->info->nchan;
            data->bufs[nbuf].is_skip     = dir0[k]->kind == FIFF_DATA_SKIP;
            data->bufs[nbuf].valid       = FALSE;
            data->bufs[nbuf].comp_status = data->comp_file;
            nbuf++;
        }
    data->nsamp = 0;
    for (k = 0; k < nbuf; k++) {
        dir = data->bufs[k].ent;
        if (dir->kind == FIFF_DATA_BUFFER) {
            if (dir->type == FIFFT_DAU_PACK16 || dir->type == FIFFT_SHORT)
                data->bufs[k].ns = dir->size/(data->info->nchan*sizeof(fiff_dau_pack16_t));
            else if (dir->type == FIFFT_FLOAT)
                data->bufs[k].ns = dir->size/(data->info->nchan*sizeof(fiff_float_t));
            else if (dir->type == FIFFT_INT)
                data->bufs[k].ns = dir->size/(data->info->nchan*sizeof(fiff_int_t));
            else {
                printf("We are not prepared to handle raw data type: %d",dir->type);
                goto bad;
            }
        }
        else if (dir->kind == FIFF_DATA_SKIP) {
            //            if (fiff_read_this_tag(in->fd,dir->pos,&tag) == FIFF_FAIL)
            //                goto bad;
            if (!stream->read_tag(t_pTag,dir->pos))
                goto bad;
            data->bufs[k].ns = data->info->buf_size*(*t_pTag->toInt());
        }
        data->bufs[k].firsts = k == 0 ? data->first_samp : data->bufs[k-1].lasts + 1;
        data->bufs[k].lasts  = data->bufs[k].firsts + data->bufs[k].ns - 1;
        data->nsamp += data->bufs[k].ns;
    }
    //    FREE_36(tag.data);
    /*
       * Set up the first sample values
       */
    data->bad = Eigen::VectorXi::Zero(data->info->nchan);
    data->offsets = Eigen::VectorXf::Zero(data->info->nchan);
    /*
        * Th bad channel stuff
        */
    {
        if (mne_read_bad_channel_list(name,data->badlist,data->nbad) == OK) {
            for (b = 0; b < data->nbad; b++) {
                for (k = 0; k < data->info->nchan; k++) {
                    if (QString::compare(data->info->chInfo[k].ch_name,data->badlist[b],Qt::CaseInsensitive) == 0) {
                        data->bad[k] = TRUE;
                        break;
                    }
                }
            }
            printf("%d bad channels read from %s%s",data->nbad,name.toUtf8().constData(),data->nbad > 0 ? ":\n" : "\n");
            if (data->nbad > 0) {
                printf("\t");
                for (k = 0; k < data->nbad; k++)
                    printf("%s%c",data->badlist[k].toUtf8().constData(),k < data->nbad-1 ? ' ' : '\n');
            }
        }
    }
    /*
       * Initialize the raw data buffers
       */
    nbuf = approx_ring_buf_size/(data->info->buf_size*data->info->nchan*sizeof(float));
    data->ring = std::make_unique<RingBuffer>(nbuf);
    /*
       * Initialize the filter buffers
       */
    data->filter = std::make_unique<MNEFilterDef>(filter);
    data->setup_filter_bufs();

    {
        std::vector<float> vals_storage(data->info->nchan, 0.0f);
        std::vector<float*> vals_rows(data->info->nchan);
        for (int i = 0; i < data->info->nchan; i++)
            vals_rows[i] = &vals_storage[i];
        float **vals = vals_rows.data();

        if (data->pick_data(NULL,data->first_samp,1,vals) == FAIL)
            goto bad;
        data->first_sample_val.resize(data->info->nchan);
        for (k = 0; k < data->info->nchan; k++)
            data->first_sample_val[k] = vals[k][0];
        printf("Initial dc offsets determined\n");
    }
    printf("Raw data file %s:\n",name.toUtf8().constData());
    printf("\tnchan  = %d\n",data->info->nchan);
    printf("\tnsamp  = %d\n",data->nsamp);
    printf("\tsfreq  = %-8.3f Hz\n",data->info->sfreq);
    printf("\tlength = %-8.3f sec\n",data->nsamp/data->info->sfreq);

    return data.release();

bad : {
        return nullptr;
    }
}

//=============================================================================================================

MNERawData *MNERawData::open_file(const QString& name, int omit_skip, int allow_maxshield, const MNEFilterDef& filter)
/*
     * Wrapper for open_file to work as before
     */
{
    return open_file_comp(name,omit_skip,allow_maxshield,filter,-1);
}
