//=============================================================================================================
/**
 * @file     mne_meas_data.cpp
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
 * @brief    Definition of the MNE Meas Data (MneMeasData) Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_meas_data.h"
#include "mne_meas_data_set.h"
#include "mne_inverse_operator.h"

#include <mne/c/mne_types.h>
#include <mne/c/mne_named_matrix.h>

#include <fiff/fiff_types.h>

#include <time.h>

#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace INVERSELIB;

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

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define MALLOC_9(x,t) (t *)malloc((x)*sizeof(t))
#define REALLOC_9(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))

#define FREE_9(x) if ((char *)(x) != NULL) free((char *)(x))

#define ALLOC_CMATRIX_9(x,y) mne_cmatrix_9((x),(y))

#define FREE_CMATRIX_9(m) mne_free_cmatrix_9((m))

void fromFloatEigenMatrix_9(const Eigen::MatrixXf& from_mat, float **& to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

void fromFloatEigenMatrix_9(const Eigen::MatrixXf& from_mat, float **& to_mat)
{
    fromFloatEigenMatrix_9(from_mat, to_mat, from_mat.rows(), from_mat.cols());
}

void mne_free_cmatrix_9 (float **m)
{
    if (m) {
        FREE_9(*m);
        FREE_9(m);
    }
}

static void matrix_error_9(int kind, int nr, int nc)

{
    if (kind == 1)
        printf("Failed to allocate memory pointers for a %d x %d matrix\n",nr,nc);
    else if (kind == 2)
        printf("Failed to allocate memory for a %d x %d matrix\n",nr,nc);
    else
        printf("Allocation error for a %d x %d matrix\n",nr,nc);
    if (sizeof(void *) == 4) {
        printf("This is probably because you seem to be using a computer with 32-bit architecture.\n");
        printf("Please consider moving to a 64-bit platform.");
    }
    printf("Cannot continue. Sorry.\n");
    exit(1);
}

float **mne_cmatrix_9(int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC_9(nr,float *);
    if (!m) matrix_error_9(1,nr,nc);
    whole = MALLOC_9(nr*nc,float);
    if (!whole) matrix_error_9(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}

namespace INVERSELIB
{

typedef struct {
    int   size;		        /* Size of this buffer in floats */
    float *data;			/* The allocated buffer */
    float ***datap;		/* The matrix which uses this */
} *ringBufBuf_9,ringBufBufRec_9;

typedef struct {
    ringBufBuf_9 *bufs;
    int        nbuf;
    int        next;
} *ringBuf_9,ringBufRec_9;

}

void mne_free_ring_buffer_9(void *thisp)

{
    int k;
    ringBuf_9 this_buf = (ringBuf_9)thisp;

    if (!this_buf)
        return;

    for (k = 0; k < this_buf->nbuf; k++)
        FREE_9(this_buf->bufs[k]->data);
    FREE_9(this_buf->bufs);
    FREE_9(this_buf);
    return;
}

//============================= mne_events.c =============================

void mne_free_event_9(mneEvent e)
{
    if (!e)
        return;
    FREE_9(e->comment);
    FREE_9(e);
    return;
}

void mne_free_event_list_9(mneEventList list)

{
    int k;
    if (!list)
        return;
    for (k = 0; k < list->nevent; k++)
        mne_free_event_9(list->events[k]);
    FREE_9(list->events);
    FREE_9(list);
    return;
}

void mne_free_sparse_named_matrix_9(mneSparseNamedMatrix mat)
/*
      * Free the matrix and all the data from within
      */
{
    if (!mat)
        return;
    mat->rowlist.clear();
    mat->collist.clear();
    if(mat->data)
        delete mat->data;
    FREE_9(mat);
    return;
}

//============================= mne_raw_routines.c =============================

void mne_ch_selection_free_9(mneChSelection s)

{
    if (!s)
        return;
    s->name.clear();
    FREE_9(s->pick);
    FREE_9(s->pick_deriv);
    FREE_9(s->ch_kind);
    s->chspick.clear();
    s->chspick_nospace.clear();
    s->chdef.clear();
    FREE_9(s);
    return;
}

void mne_string_to_name_list_9(const QString& s, QStringList& listp,int &nlistp)
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

QString mne_name_list_to_string_9(const QStringList& list)
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

QString mne_channel_names_to_string_9(const QList<FIFFLIB::FiffChInfo>& chs,
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
    res = mne_name_list_to_string_9(names);
    return res;
}

void mne_channel_names_to_name_list_9(const QList<FIFFLIB::FiffChInfo>& chs,
                                      int nch,
                                      QStringList& listp,
                                      int &nlistp)

{
    QString s = mne_channel_names_to_string_9(chs,nch);

    mne_string_to_name_list_9(s,listp,nlistp);
    return;
}

//============================= read_ch_info.c =============================

static FiffDirNode::SPtr find_meas_9 (const FiffDirNode::SPtr& node)
/*
      * Find corresponding meas node
      */
{
    FiffDirNode::SPtr empty_node;
    FiffDirNode::SPtr tmp_node = node;

    while (tmp_node->type != FIFFB_MEAS) {
        if (tmp_node->parent == NULL)
            return empty_node;//(NULL);
        tmp_node = tmp_node->parent;
    }
    return (tmp_node);
}

static FiffDirNode::SPtr find_meas_info_9 (const FiffDirNode::SPtr& node)
/*
      * Find corresponding meas info node
      */
{
    int k;
    FiffDirNode::SPtr empty_node;
    FiffDirNode::SPtr tmp_node = node;

    while (tmp_node->type != FIFFB_MEAS) {
        if (tmp_node->parent == NULL)
            return empty_node;
        tmp_node = tmp_node->parent;
    }
    for (k = 0; k < tmp_node->nchild(); k++)
        if (tmp_node->children[k]->type == FIFFB_MEAS_INFO)
            return (tmp_node->children[k]);
    return empty_node;
}

//============================= mne_read_evoked.c =============================

#define MAXDATE 100

static FiffDirNode::SPtr find_evoked (const FiffDirNode::SPtr& node)
/*
 * Find corresponding FIFFB_EVOKED node
 */
{
    FiffDirNode::SPtr empty_node;
    FiffDirNode::SPtr tmp_node = node;
    while (tmp_node->type != FIFFB_EVOKED) {
        if (tmp_node->parent == NULL)
            return empty_node;
        tmp_node = tmp_node->parent;
    }
    return (tmp_node);
}

static QString get_comment (  FiffStream::SPtr& stream,
                              const FiffDirNode::SPtr& start)

{
    int k;
    FiffTag::SPtr t_pTag;
    QList<FiffDirEntry::SPtr> ent = start->dir;
    for (k = 0; k < start->nent(); k++)
        if (ent[k]->kind == FIFF_COMMENT) {
            if (stream->read_tag(t_pTag,ent[k]->pos)) {
                return t_pTag->toString();
            }
        }
    return QString("No comment");
}

static void get_aspect_name_type(   FiffStream::SPtr& stream,
                                    const FiffDirNode::SPtr& start,
                                    QString& namep, int *typep)

{
    int k;
    FiffTag::SPtr t_pTag;
    QList<FiffDirEntry::SPtr> ent = start->dir;
    QString res = "unknown";
    int  type = -1;

    for (k = 0; k < start->nent(); k++)
        if (ent[k]->kind == FIFF_ASPECT_KIND) {
            if (stream->read_tag(t_pTag,ent[k]->pos)) {
                type = *t_pTag->toInt();
                switch (type) {
                case FIFFV_ASPECT_AVERAGE :
                    res = "average";
                    break;
                case FIFFV_ASPECT_STD_ERR :
                    res = "std.error";
                    break;
                case FIFFV_ASPECT_SINGLE :
                    res = "single trace";
                    break;
                case FIFFV_ASPECT_SAMPLE :
                    res = "sample";
                    break;
                case FIFFV_ASPECT_SUBAVERAGE :
                    res = "subaverage";
                    break;
                case FIFFV_ASPECT_ALTAVERAGE :
                    res = "alt. average";
                    break;
                case FIFFV_ASPECT_POWER_DENSITY :
                    res = "power density spectrum";
                    break;
                case FIFFV_ASPECT_DIPOLE_WAVE :
                    res = "dipole amplitudes";
                    break;
                }
            }
            break;
        }
    //    if (namep)
    namep = res;
    if (typep)
        *typep = type;
    return;
}

static char *get_meas_date (    FiffStream::SPtr& stream,const FiffDirNode::SPtr& node  )
{
    int k;
    FiffTag::SPtr t_pTag;
    char *res = NULL;
    fiff_int_t kind, pos;
    FiffDirNode::SPtr meas_info;

    if (!(meas_info = find_meas_info_9(node))) {
        return res;
    }
    for (k = 0; k < meas_info->nent();k++) {
        kind = meas_info->dir[k]->kind;
        pos  = meas_info->dir[k]->pos;
        if (kind == FIFF_MEAS_DATE)
        {
            if (stream->read_tag(t_pTag,pos)) {
                fiffTime meas_date = (fiffTime)t_pTag->data();
                time_t   time = meas_date->secs;
                struct   tm *ltime;

                ltime = localtime(&time);
                res = MALLOC_9(MAXDATE,char);
                (void)strftime(res,MAXDATE,"%x %X",ltime);
                break;
            }
        }
    }
    return res;
}

static int get_meas_info (  FiffStream::SPtr& stream,       /* The stream we are reading */
                            const FiffDirNode::SPtr& node,  /* The directory node containing our data */
                            fiffId *id,                     /* The block id from the nearest FIFFB_MEAS parent */
                            fiffTime *meas_date,            /* Measurement date */
                            int *nchan,                     /* Number of channels */
                            float *sfreq,                   /* Sampling frequency */
                            float *highpass,                /* Highpass filter setting */
                            float *lowpass,                 /* Lowpass filter setting */
                            QList<FIFFLIB::FiffChInfo>& chp,                /* Channel descriptions */
                            FiffCoordTransOld* *trans)          /* Coordinate transformation (head <-> device) */
/*
 * Find channel information from
 * nearest FIFFB_MEAS_INFO parent of
 * node.
 */
{
    QList<FIFFLIB::FiffChInfo> ch;
    FIFFLIB::FiffChInfo this_ch;
    FiffCoordTransOld* t = Q_NULLPTR;
    int j,k;
    int to_find = 4;
    QList<FiffDirNode::SPtr> hpi;
    FiffDirNode::SPtr meas;
    FiffDirNode::SPtr meas_info;
    fiff_int_t kind, pos;
    FiffTag::SPtr t_pTag;

     *trans   = NULL;
     *id      = NULL;
    /*
     * Find desired parents
     */
    if (!(meas = find_meas_9(node))) {
        printf ("Meas. block not found!");
        goto bad;
    }
    if (!(meas_info = find_meas_info_9(node))) {
        printf ("Meas. info not found!");
        goto bad;
    }
    /*
     * Is there a block id is in the FIFFB_MEAS node?
     */
    if (!meas->id.isEmpty()) {
        *id = MALLOC_9(1,fiffIdRec);
        (*id)->version = meas->id.version;
        (*id)->machid[0] = meas->id.machid[0];
        (*id)->machid[1] = meas->id.machid[1];
        (*id)->time = meas->id.time;
    }
    /*
     * Others from FIFFB_MEAS_INFO
     */
     *lowpass = -1;
     *highpass = -1;
    for (k = 0; k < meas_info->nent(); k++) {
        kind = meas_info->dir[k]->kind;
        pos  = meas_info->dir[k]->pos;
        switch (kind) {

        case FIFF_NCHAN :
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            *nchan = *t_pTag->toInt();

            for (j = 0; j < *nchan; j++) {
                ch.append(FiffChInfo());
                ch[j].scanNo = -1;
            }
            to_find = to_find + *nchan - 1;
            break;

        case FIFF_SFREQ :
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            *sfreq = *t_pTag->toFloat();
            to_find--;
            break;

        case FIFF_MEAS_DATE :
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            if (*meas_date)
                FREE_9(*meas_date);
            *meas_date = MALLOC_9(1,fiffTimeRec);
            **meas_date = *(fiffTime)t_pTag->data();
            break;

        case FIFF_LOWPASS :
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            *lowpass = *t_pTag->toFloat();
            to_find--;
            break;

        case FIFF_HIGHPASS :
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            *highpass = *t_pTag->toFloat();
            to_find--;
            break;

        case FIFF_CH_INFO : /* Information about one channel */

            if (!stream->read_tag(t_pTag,pos))
                goto bad;

            this_ch = t_pTag->toChInfo();
            if (this_ch.scanNo <= 0 || this_ch.scanNo > *nchan) {
                qCritical ("FIFF_CH_INFO : scan # out of range!");
                goto bad;
            }
            else
                ch[this_ch.scanNo-1] = this_ch;
            to_find--;
            break;

        case FIFF_COORD_TRANS :
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            if(t)
                delete t;
            t = FiffCoordTransOld::read_helper( t_pTag );
            /*
            * Require this particular transform!
            */
            if (t->from == FIFFV_COORD_DEVICE && t->to == FIFFV_COORD_HEAD) {
                *trans = t;
                break;
            }
        }
    }
    /*
     * Search for the coordinate transformation from
     * HPI_RESULT block if it was not previously found
     */

    hpi = meas_info->dir_tree_find(FIFFB_HPI_RESULT);

    if (hpi.size() > 0 && *trans == NULL)
        for (k = 0; k < hpi[0]->nent(); k++)
            if (hpi[0]->dir[k]->kind ==  FIFF_COORD_TRANS) {
                if (!stream->read_tag(t_pTag,hpi[0]->dir[k]->pos))
                    goto bad;
                t = FiffCoordTransOld::read_helper( t_pTag );

                /*
                * Require this particular transform!
                */
                if (t->from == FIFFV_COORD_DEVICE && t->to == FIFFV_COORD_HEAD) {
                    *trans = t;
                    break;
                }
            }
    if (*lowpass < 0) {
        *lowpass = *sfreq/2.0;
        to_find--;
    }
    if (*highpass < 0) {
        *highpass = 0.0;
        to_find--;
    }
    if (to_find != 0) {
        printf ("Not all essential tags were found!");
        goto bad;
    }

    chp = ch;
    return (0);

bad : {
        return (-1);
    }
}

static int find_between (   FiffStream::SPtr& stream,
                            const FiffDirNode::SPtr& low_node,
                            const FiffDirNode::SPtr& high_node,
                            int kind,
                            fiff_byte_t **data)
{
    FiffTag::SPtr t_pTag;
    FiffDirNode::SPtr node;
    fiffDirEntry dir;
    fiff_int_t kind_1, pos;
    int k;

     *data = NULL;
    node = low_node;
    while (node != NULL) {
        for (k = 0; k < node->nent(); k++)
        {
            kind_1 = node->dir[k]->kind;
            pos  = node->dir[k]->pos;
            if (kind_1 == kind) {
                FREE_9(*data);
                if (!stream->read_tag(t_pTag,pos)) {
                    return (FIFF_FAIL);
                }
                else {
                    fiff_byte_t* tmp =  MALLOC_9(t_pTag->size(),fiff_byte_t);
                    fiff_byte_t* tmp_current = (fiff_byte_t *)t_pTag->data();

                    for( int k = 0; k < t_pTag->size(); ++k )
                        tmp[k] = tmp_current[k];

                    *data = tmp;
                    return (FIFF_OK);
                }
            }
        }
        if (node == high_node)
            break;
        node = node->parent;
    }
    return (FIFF_OK);
}

static int get_evoked_essentials (FiffStream::SPtr& stream,         /* This is our file */
                                  const FiffDirNode::SPtr& node,    /* The interesting node */
                                  float& sfreq,                     /* Sampling frequency
                                                                                                       * The value pointed by this is not
                                                                                                       * modified if individual sampling
                                                                                                       * frequency is found */
                                  float& tmin,                      /* Time scale minimum */
                                  int& nsamp,                       /* Number of samples */
                                  int& nave,                        /* Number of averaged responses */
                                  int& akind,                       /* Aspect type */
                                  int *& artefs,                /* Artefact removal parameters */
                                  int& nartef)
/*
      * Get the essential info for
      * given evoked response data
      */
{
    FiffTag::SPtr t_pTag;
    int k;
    int to_find = 2;
    int   first = -1;
    int   last = -1;
    int   my_nsamp = -1;
    float my_tmin = -1;
    int   res = -1;
    fiff_int_t kind, pos;

    fiff_byte_t *tempb;

    FiffDirNode::SPtr tmp_node = node;

    /*
     * This is rather difficult...
     */
    if (find_between (stream,tmp_node,tmp_node->parent,FIFF_NAVE,&tempb) == FIFF_FAIL)
        return res;
    if (tempb)
        nave = *(int *)tempb;
    FREE_9(tempb);
    if (find_between (stream,tmp_node,tmp_node->parent,
                      FIFF_SFREQ,&tempb) == FIFF_FAIL)
        return res;
    if (tempb)
        sfreq = *(float *)tempb;
    FREE_9(tempb);

    if (find_between (stream,tmp_node,tmp_node->parent,
                      FIFF_ASPECT_KIND,&tempb) == FIFF_FAIL)
        return res;
    if (tempb)
        akind = *(int *)tempb;
    else
        akind = FIFFV_ASPECT_AVERAGE; /* Just a guess */
    FREE_9(tempb);
    /*
   * Find evoked response descriptive data
   */
    tmp_node = tmp_node->parent;

    //    tag.data = NULL;
    for (k = 0; k < tmp_node->dir_tree.size(); k++) {
        kind = tmp_node->dir_tree[k]->kind;
        pos  = tmp_node->dir_tree[k]->pos;
        switch (kind) {

        case FIFF_FIRST_SAMPLE :
            if (!stream->read_tag(t_pTag,pos))
                goto out;
            first = *t_pTag->toInt(); to_find--;
            break;

        case FIFF_LAST_SAMPLE :
            if (!stream->read_tag(t_pTag,pos))
                goto out;
            last = *t_pTag->toInt(); to_find--;
            break;

        case FIFF_NO_SAMPLES :
            if (!stream->read_tag(t_pTag,pos))
                goto out;
            my_nsamp = *t_pTag->toInt(); to_find--;
            break;

        case FIFF_FIRST_TIME :
            if (!stream->read_tag(t_pTag,pos))
                goto out;
            my_tmin = *t_pTag->toFloat(); to_find--;
            break;
        case FIFF_ARTEF_REMOVAL :
            if (!stream->read_tag(t_pTag,pos))
                goto out;
            qDebug() << "TODO: check whether artefs contains the right stuff -> use MatrixXi instead";
            artefs = t_pTag->toInt();
            nartef = t_pTag->size()/(3*sizeof(int));
            break;
        }
    }
    if (to_find > 0) {
        printf ("Not all essential tags were found!");
        goto out;
    }
    if (first != -1 && last != -1) {
        nsamp = (last)-(first)+1;
        tmin  = (first)/(sfreq);
    }
    else if (my_tmin != -1 && my_nsamp != -1) {
        tmin = my_tmin;
        nsamp = my_nsamp;
    }
    else {
        printf("Not enough data for time scale definition!");
        goto out;
    }
    res = 0;

out : {
        return res;
    }
}

static int get_evoked_optional( FiffStream::SPtr& stream,
                                const FiffDirNode::SPtr& node, /* The directory node containing our data */
                                int *nchan,	 /* Number of channels */
                                QList<FiffChInfo>& chp)	 /* Channel descriptions */
/*
 * The channel info may have been modified
 */
{
    int res = FIFF_FAIL;
    QList<FiffChInfo> new_ch;
    int new_nchan = *nchan;
    int k,to_find;
    FiffTag::SPtr t_pTag;
    fiff_int_t kind, pos;
    FiffChInfo this_ch;
    FiffDirNode::SPtr evoked_node;

    if (!(evoked_node = find_evoked(node))) {
        res = FIFF_OK;
        goto out;
    }

    to_find = 0;
    if(evoked_node->find_tag(stream, FIFF_NCHAN, t_pTag))
        new_nchan = *t_pTag->toInt();
    else
        new_nchan = *nchan;

    for (k = 0; k < evoked_node->nent(); k++) {
        kind = evoked_node->dir[k]->kind;
        pos  = evoked_node->dir[k]->pos;
        if (kind == FIFF_CH_INFO) {     /* Information about one channel */
            if (new_ch.isEmpty()) {
                to_find = new_nchan;
                for (int i = 0; i < to_find; i++) {
                    new_ch.append(FiffChInfo());
                }
            }
            if (!stream->read_tag(t_pTag,pos))
                goto out;

            this_ch = t_pTag->toChInfo();
            if (this_ch.scanNo <= 0 || this_ch.scanNo > new_nchan) {
                printf ("FIFF_CH_INFO : scan # out of range!");
                goto out;
            }
            else
                new_ch[this_ch.scanNo-1] = this_ch;
            to_find--;
        }
    }
    if (to_find != 0) {
        printf("All channels were not specified "
               "at the FIFFB_EVOKED level.");
        goto out;
    }
    res = FIFF_OK;
    goto out;

out : {
        if (res == FIFF_OK) {
            *nchan = new_nchan;
            if (!new_ch.isEmpty()) {
                chp = new_ch;
            }
        }
        return res;
    }
}

static void unpack_data(double offset,
                        double scale,
                        short *packed,
                        int   nsamp,
                        float *orig)
{
    int k;
    for (k = 0; k < nsamp; k++)
        orig[k] = scale * packed[k] + offset;
    return;
}

static float **get_epochs ( FiffStream::SPtr& stream,       /* This is our file */
                            const FiffDirNode::SPtr& node,  /* The interesting node */
                            int nchan, int nsamp)            /* Number of channels and number of samples to be expected */
/*
 * Get the evoked response epochs
 */
{
    fiff_int_t kind, pos;
    FiffTag::SPtr t_pTag;
    int k;
    int ch;
    float **epochs = NULL;
    float offset,scale;
    short *packed;

    for (k = 0, ch = 0; k < node->nent() && ch < nchan; k++) {
        kind = node->dir[k]->kind;
        pos  = node->dir[k]->pos;
        if (kind == FIFF_EPOCH) {
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            if (t_pTag->type & FIFFT_MATRIX) {
                if ((t_pTag->type & ~FIFFT_MATRIX) != FIFFT_FLOAT) {
                    printf("Epochs in matrix should be floats!");
                    goto bad;
                }

                qint32 ndim;
                QVector<qint32> dims;
                t_pTag->getMatrixDimensions(ndim, dims);

                if (ndim != 2) {
                    printf("Data matrix dimension should be two!");
                    goto bad;
                }
                if (dims[0] != nsamp) {
                    printf("Incorrect number of samples in data matrix!");
                    goto bad;
                }
                if (dims[1] != nchan) {
                    printf("Incorrect number of channels in data matrix!");
                    goto bad;
                }
                MatrixXf tmp_epochs = t_pTag->toFloatMatrix().transpose();
                epochs = ALLOC_CMATRIX_9(tmp_epochs.rows(),tmp_epochs.cols());
                fromFloatEigenMatrix_9(tmp_epochs, epochs);
                ch = nchan;
                break;  /* We have the data */
            }
            else {      /* Individual epochs */
                if (epochs == NULL)
                    epochs = ALLOC_CMATRIX_9(nchan,nsamp);
                if (t_pTag->type == FIFFT_OLD_PACK) {
                    offset = ((float *)t_pTag->data())[0];
                    scale  = ((float *)t_pTag->data())[1];
                    packed = (short *)(((float *)t_pTag->data())+2);
                    unpack_data(offset,scale,packed,nsamp,epochs[ch++]);
                }
                else if (t_pTag->type == FIFFT_FLOAT)
                    memcpy(epochs[ch++],t_pTag->data(),nsamp*sizeof(float));
                else {
                    printf ("Unknown data packing type!");
                    FREE_CMATRIX_9(epochs);
                    return (NULL);
                }
            }
            if (ch == nchan)
                return (epochs);
        }
    }
    if (ch < nchan) {
        printf ("All epochs were not found!");
        goto bad;
    }
    return (epochs);

bad : {
        FREE_CMATRIX_9(epochs);
        return (NULL);
    }
}

int mne_find_evoked_types_comments (    FiffStream::SPtr& stream,
                                        QList<FiffDirNode::SPtr>& nodesp,
                                        int         **aspect_typesp,
                                        QStringList* commentsp)
/*
 * Find all data we are able to process
 */
{
    QList<FiffDirNode::SPtr> evoked;
    QList<FiffDirNode::SPtr> meas;
    QList<FiffDirNode::SPtr> nodes;
    int         evoked_count,count;
    QString     part,type,meas_date;
    QStringList comments;
    int         *types = NULL;
    int         j,k,p;

    if (stream == NULL)
        return 0;
    /*
     * First find all measurements
     */
    meas = stream->dirtree()->dir_tree_find(FIFFB_MEAS);
    /*
     * Process each measurement
     */
    for (count = 0,p = 0; p < meas.size(); p++) {
        evoked = meas[p]->dir_tree_find(FIFFB_EVOKED);
        /*
        * Count the entries
        */
        for (evoked_count = 0, j = 0; j < evoked.size(); j++) {
            for (k = 0; k < evoked[j]->nchild(); k++) {
                if (evoked[j]->children[k]->type == FIFFB_ASPECT) {
                    evoked_count++;
                }
            }
        }
        /*
        * Enlarge tables
        */
        types    = REALLOC_9(types,count+evoked_count+1,int);
        /*
        * Insert node references and compile associated comments...
        */
        for (j = 0; j < evoked.size(); j++)	/* Evoked data */
            for (k = 0; k < evoked[j]->nchild(); k++)
                if (evoked[j]->children[k]->type == FIFFB_ASPECT) {
                    meas_date = get_meas_date(stream,evoked[j]);
                    part      = get_comment(stream,evoked[j]);
                    get_aspect_name_type(stream,evoked[j]->children[k],type,types+count);
                    if (!meas_date.isEmpty()) {
                        comments.append(QString("%1>%2>%3").arg(meas_date).arg(part).arg(type));
                    }
                    else {
                        comments.append(QString("%1>%2").arg(part).arg(type));
                    }
                    nodes.append(evoked[j]->children[k]);
                    count++;
                }
    }
    if (count == 0) {   /* Nothing to report */
        comments.clear();
        nodesp.clear();
        if (commentsp)
            *commentsp = comments;
        if (aspect_typesp)
            *aspect_typesp = NULL;
        return 0;
    }
    else {              /* Return the appropriate variables */
//        comments[count] = NULL;
        types[count]    = -1;
        nodesp = nodes;
        if (commentsp)
            *commentsp = comments;
        else
            comments.clear();
        if (aspect_typesp)
            *aspect_typesp = types;
        else
            FREE_9(types);
        return count;
    }
}

QList<FiffDirNode::SPtr> mne_find_evoked ( FiffStream::SPtr& stream, QStringList* commentsp)
/* Optionally return the compiled comments here */
{
    QList<FiffDirNode::SPtr> evoked;
    mne_find_evoked_types_comments(stream,evoked,NULL,commentsp);
    return evoked;
}

static void remove_artefacts (float *resp,
                              int   nsamp,
                              int   *artefs,
                              int   nartef)
/*
 * Apply the artefact removal
 */
{
    int   start,end;
    int   j,k;
    float a,b;
    int   remove_jump;

    for (k = 0; k < nartef; k++) {
        if (artefs[3*k] == FIFFV_ARTEF_NONE || artefs[3*k] == FIFFV_ARTEF_KEEP)
            continue;
        remove_jump = (artefs[3*k] == FIFFV_ARTEF_NOJUMP);
        /*
        * Find out the indices for the start and end times
        */
        start = artefs[3*k+1];
        end   = artefs[3*k+2];
        start = MAX(0,MIN(start,nsamp));
        end   = MAX(0,MIN(end,nsamp));
        /*
        * Replace the artefact region with a straight line
        */
        if (start < end) {
            if (remove_jump) {	/* Remove jump... */
                a = resp[end] - resp[start];
                for (j = 0; j <=start; j++)
                    resp[j] = resp[j] + a;
                for (j = start+1 ; j < end; j++)
                    resp[j] = resp[end];
            }
            else {			/* Just connect... */
                a = (resp[end]-resp[start])/(end-start);
                b = (resp[start]*end - resp[end]*start)/(end-start);
                for (j = start+1 ; j < end; j++)
                    resp[j] = a*j+b;
            }
        }
    }
    return;
}

int mne_read_evoked(const QString& name,        /* Name of the file */
                    int        setno,           /* Which data set */
                    int        *nchanp,         /* How many channels */
                    int        *nsampp,         /* Number of time points */
                    float      *tminp,          /* First time point */
                    float      *sfreqp,         /* Sampling frequency */
                    QList<FiffChInfo>& chsp,           /* Channel info (this is now optional as well) */
                    float      ***epochsp,      /* Data, channel by channel */
                    /*
                                        * Optional items follow
                                        */
                    QString*    commentp,       /* Comment for these data */
                    float      *highpassp,      /* Highpass frequency */
                    float      *lowpassp,       /* Lowpass frequency */
                    int        *navep,          /* How many averages */
                    int        *aspect_kindp,   /* What kind of an evoked data */
                    FiffCoordTransOld* *transp,     /* Coordinate transformation */
                    fiffId         *idp,        /* Measurement id */
                    fiffTime       *meas_datep) /* Measurement date */
/*
 * Load evoked-response data from a fif file
 */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    QList<FiffDirNode::SPtr> evoked;    /* The evoked data nodes */
    int         nset    = 0;
    int         nchan   = 0;            /* How many channels */
    QStringList comments;               /* The associated comments */
    float       sfreq = 0.0;            /* What sampling frequency */
    FiffDirNode::SPtr start;
    QList<FiffChInfo>   chs;        /* Channel info */
    int          *artefs = NULL;        /* Artefact limits */
    int           nartef = 0;           /* How many */
    float       **epochs = NULL;        /* The averaged epochs */
    FiffCoordTransOld* trans = NULL;        /* The coordinate transformation */
    fiffId            id = NULL;        /* Measurement id */
    fiffTime          meas_date = NULL; /* Measurement date */
    int             nave = 1;           /* Number of averaged responses */
    float           tmin = 0;           /* Time scale minimum */
    float           lowpass;            /* Lowpass filter frequency */
    float           highpass = 0.0;     /* Highpass filter frequency */
    int             nsamp = 0;          /* Samples in epoch */
    int             aspect_kind;        /* What kind of data */
    int             res = FAIL;         /* A little bit of pessimism */

    float *epoch;
    int   j,k;

    if (setno < 0) {
        printf ("Evoked response selector must be positive!");
        goto out;
    }

    if(!stream->open())
        goto out;

    /*
     * Select correct data set
     */
    evoked = mne_find_evoked(stream,(commentp == NULL) ? NULL : &comments);
    if (!evoked.size()) {
        printf ("No evoked response data available here");
        goto out;
    }

    nset = evoked.size();

    if (setno < nset) {
        start = evoked[setno];
    }
    else {
        printf ("Too few evoked response data sets (how come?)");
        goto out;
    }
    /*
     * Get various things...
     */
    if (get_meas_info (stream,
                       start,
                       &id,
                       &meas_date,
                       &nchan,
                       &sfreq,
                       &highpass,
                       &lowpass,
                       chs,
                       &trans) == -1)
        goto out;

    /*
     * sfreq is listed here again because
     * there might be an individual one in the
     * evoked-response data
     */
    if (get_evoked_essentials(stream,start,sfreq,
                              tmin,nsamp,nave,aspect_kind,
                              artefs,nartef) == -1)
        goto out;
    /*
     * Some things may be redefined at a lower level
     */
    if (get_evoked_optional(stream,
                            start,
                            &nchan,
                            chs) == -1)
        goto out;
    /*
     * Omit nonmagnetic channels
     */
    if ((epochs = get_epochs(stream,start,nchan,nsamp)) == NULL)
        goto out;
    /*
     * Change artefact limits to start from 0
     */
    for (k = 0; k < nartef; k++) {
        qDebug() << "TODO: Artefact Vectors do not contain the right stuff!";
        artefs[2*k+1] = artefs[2*k+1] - sfreq*tmin;
        artefs[2*k+2] = artefs[2*k+2] - sfreq*tmin;
    }
    for (k = 0; k < nchan; k++) {
        epoch = epochs[k];
        for (j = 0; j < nsamp; j++)
            epoch[j] = chs[k].cal*epoch[j];
        remove_artefacts(epoch,nsamp,artefs,nartef);
    }
    /*
   * Ready to go
   */
    chsp    = chs;
    *tminp   = tmin;
    *nchanp  = nchan;
    *nsampp  = nsamp;
    *sfreqp  = sfreq;
    *epochsp = epochs; epochs = NULL;
    /*
     * Fill in the optional data
     */
    if (commentp) {
        *commentp = comments[setno];
        comments[setno] = "";
    }
    if (highpassp)
        *highpassp = highpass;
    if (lowpassp)
        *lowpassp = lowpass;
    if (transp) {
        *transp = trans;
        trans = NULL;
    }
    if (navep)
        *navep = nave;
    if (aspect_kindp)
        *aspect_kindp = aspect_kind;
    if (idp) {
        *idp = id;
        id = NULL;
    }
    if (meas_datep) {
        *meas_datep = meas_date;
        meas_date = NULL;
    }
    res = OK;
    /*
     * FREE all allocated data on exit
     */
out : {
        comments.clear();
        FREE_9(artefs);
        FREE_9(trans);
        FREE_9(id);
        FREE_9(meas_date);
        FREE_CMATRIX_9(epochs);
        stream->close();
        return res;
    }
}

//============================= mne_inverse_io.c =============================

#define MAXBUF 200

char *mne_format_file_id (fiffId id)

{
    char buf[MAXBUF];
    static char s[300];
    struct tm *ltime;
    time_t secs;

    secs = id->time.secs;
    ltime = localtime(&secs);
    (void)strftime(buf,MAXBUF,"%c",ltime);

    sprintf(s,"%d.%d 0x%x%x %s",id->version>>16,id->version & 0xFFFF,id->machid[0],id->machid[1],buf);
    return s;
}

int mne_read_meg_comp_eeg_ch_info_9(const QString& name,
                                    QList<FiffChInfo>& megp,	 /* MEG channels */
                                    int            *nmegp,
                                    QList<FiffChInfo>& meg_compp,
                                    int            *nmeg_compp,
                                    QList<FiffChInfo>& eegp,	 /* EEG channels */
                                    int            *neegp,
                                    FiffCoordTransOld* *meg_head_t,
                                    fiffId         *idp)	 /* The measurement ID */
/*
      * Read the channel information and split it into three arrays,
      * one for MEG, one for MEG compensation channels, and one for EEG
      */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    QList<FiffChInfo> chs;
    int        nchan = 0;
    QList<FiffChInfo> meg;
    int        nmeg  = 0;
    QList<FiffChInfo> meg_comp;
    int        nmeg_comp = 0;
    QList<FiffChInfo> eeg;
    int        neeg  = 0;
    fiffId     id    = NULL;
    QList<FiffDirNode::SPtr> nodes;
    FiffDirNode::SPtr info;
    FiffTag::SPtr t_pTag;
    FiffChInfo this_ch;
    FiffCoordTransOld* t = NULL;
    fiff_int_t kind, pos;
    int j,k,to_find;

    if(!stream->open())
        goto bad;

    nodes = stream->dirtree()->dir_tree_find(FIFFB_MNE_PARENT_MEAS_FILE);

    if (nodes.size() == 0) {
        nodes = stream->dirtree()->dir_tree_find(FIFFB_MEAS_INFO);
        if (nodes.size() == 0) {
            qCritical ("Could not find the channel information.");
            goto bad;
        }
    }
    info = nodes[0];
    to_find = 0;
    for (k = 0; k < info->nent(); k++) {
        kind = info->dir[k]->kind;
        pos  = info->dir[k]->pos;
        switch (kind) {
        case FIFF_NCHAN :
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            nchan = *t_pTag->toInt();

            for (j = 0; j < nchan; j++) {
                chs.append(FiffChInfo());
                chs[j].scanNo = -1;
            }
            to_find = nchan;
            break;

        case FIFF_PARENT_BLOCK_ID :
            if(!stream->read_tag(t_pTag, pos))
                goto bad;
            //            id = t_pTag->toFiffID();
            *id = *(fiffId)t_pTag->data();
            break;

        case FIFF_COORD_TRANS :
            if(!stream->read_tag(t_pTag, pos))
                goto bad;
            //            t = t_pTag->toCoordTrans();
            t = FiffCoordTransOld::read_helper( t_pTag );
            if (t->from != FIFFV_COORD_DEVICE || t->to   != FIFFV_COORD_HEAD)
                t = NULL;
            break;

        case FIFF_CH_INFO : /* Information about one channel */
            if(!stream->read_tag(t_pTag, pos))
                goto bad;

            this_ch = t_pTag->toChInfo();
            if (this_ch.scanNo <= 0 || this_ch.scanNo > nchan) {
                printf ("FIFF_CH_INFO : scan # out of range %d (%d)!",this_ch.scanNo,nchan);
                goto bad;
            }
            else
                chs[this_ch.scanNo-1] = this_ch;
            to_find--;
            break;
        }
    }
    if (to_find != 0) {
        qCritical("Some of the channel information was missing.");
        goto bad;
    }
    if (t == NULL && meg_head_t != NULL) {
        /*
     * Try again in a more general fashion
     */
        if ((t = FiffCoordTransOld::mne_read_meas_transform(name)) == NULL) {
            qCritical("MEG -> head coordinate transformation not found.");
            goto bad;
        }
    }
    /*
   * Sort out the channels
   */
    for (k = 0; k < nchan; k++) {
        if (chs[k].kind == FIFFV_MEG_CH) {
            meg.append(chs[k]);
            nmeg++;
        } else if (chs[k].kind == FIFFV_REF_MEG_CH) {
            meg_comp.append(chs[k]);
            nmeg_comp++;
        } else if (chs[k].kind == FIFFV_EEG_CH) {
            eeg.append(chs[k]);
            neeg++;
        }
    }
    //    fiff_close(in);
    stream->close();

    megp  = meg;
    if(nmegp) {
        *nmegp = nmeg;
    }

    meg_compp = meg_comp;
    if(nmeg_compp) {
        *nmeg_compp = nmeg_comp;
    }
    eegp  = eeg;
    if(neegp) {
        *neegp = neeg;
    }

    if (idp == NULL) {
        FREE_9(id);
    }
    else
        *idp   = id;
    if (meg_head_t == NULL) {
        FREE_9(t);
    }
    else
        *meg_head_t = t;

    return FIFF_OK;

bad : {
        //        fiff_close(in);
        stream->close();
        FREE_9(id);
        //        FREE(tag.data);
        FREE_9(t);
        return FIFF_FAIL;
    }
}

int mne_read_bad_channel_list_from_node_9(  FiffStream::SPtr& stream,
                                            const FiffDirNode::SPtr& pNode, QStringList& listp, int& nlistp)
{
    FiffDirNode::SPtr node,bad;
    QList<FiffDirNode::SPtr> temp;
    QStringList list;
    int  nlist  = 0;
    FiffTag::SPtr t_pTag;
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
            mne_string_to_name_list_9(names,list,nlist);
        }
    }
    listp = list;
    nlistp = nlist;
    return OK;
}

int mne_read_bad_channel_list_9(const QString& name, QStringList& listp, int& nlistp)

{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    int res;

    if(!stream->open())
        return FAIL;

    res = mne_read_bad_channel_list_from_node_9(stream,stream->dirtree(),listp,nlistp);

    stream->close();

    return res;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneMeasData::MneMeasData()
    :meas_id   (NULL)
    ,current   (NULL)
    ,ch_major  (FALSE)
    ,nset      (0)
    ,nchan     (0)
    ,op        (NULL)
    ,fwd       (NULL)
    ,meg_head_t(NULL)
    ,mri_head_t(NULL)
    ,proj      (NULL)
    ,comp      (NULL)
    ,raw       (NULL)
    ,chsel     (NULL)
    ,bad       (NULL)
    ,nbad      (0)
    ,badlist   (NULL)
{
    meas_date.secs = 0;
    meas_date.usecs = 0;
}

//=============================================================================================================

MneMeasData::~MneMeasData()
{
    int k;

    filename.clear();
    FREE_9(meas_id);
    if(meg_head_t)
        delete meg_head_t;
    if(mri_head_t)
        delete mri_head_t;
    if(proj)
        delete proj;
    if(comp)
        delete comp;
    FREE_9(bad);
    badlist.clear();

    for (k = 0; k < nset; k++)
        delete sets[k];

    if(raw)
        delete raw;
    mne_ch_selection_free_9(chsel);

    return;
}

//=============================================================================================================

void MneMeasData::adjust_baselines(float bmin, float bmax)
{
    int b1,b2;
    float sfreq,tmin,tmax;
    float **data;
    float ave;
    int s,c;

    if (!this->current)
        return;

    sfreq = 1.0/ this->current->tstep;
    tmin  =  this->current->tmin;
    tmax  =  this->current->tmin + ( this->current->np-1)/sfreq;

    if (bmin < tmin)
        b1 = 0;
    else if (bmin > tmax)
        b1 =  this->current->np;
    else {
        for (b1 = 0; b1/sfreq + tmin < bmin; b1++)
            ;
        if (b1 < 0)
            b1 = 0;
        else if (b1 >  this->current->np)
            b1 =  this->current->np;
    }
    if (bmax < tmin)
        b2 = 0;
    else if (bmax > tmax)
        b2 =  this->current->np;
    else {
        for (b2 =  this->current->np; b2/sfreq + tmin > bmax; b2--)
            ;
        if (b2 < 0)
            b2 = 0;
        else if (b2 >  this->current->np)
            b2 =  this->current->np;
    }
    data =  this->current->data;
    if (b2 > b1) {
        for (c = 0; c <  this->nchan; c++) {
            for (s = b1, ave = 0.0; s < b2; s++)
                ave += data[s][c];
            ave = ave/(b2-b1);
            this->current->baselines[c] += ave;
            for (s = 0; s <  this->current->np; s++)
                data[s][c] = data[s][c] - ave;
        }
        qDebug() << "TODO: Check comments content";
        fprintf(stderr,"\t%s : using baseline %7.1f ... %7.1f ms\n",
                this->current->comment.toUtf8().constData() ?  this->current->comment.toUtf8().constData() : "unknown",
                1000*(tmin+b1/sfreq),
                1000*(tmin+b2/sfreq));
    }
    return;
}

//=============================================================================================================

MneMeasData *MneMeasData::mne_read_meas_data_add(const QString &name,
                                                 int set,
                                                 MneInverseOperator* op,
                                                 MneNamedMatrix *fwd,
                                                 const QStringList& namesp,
                                                 int nnamesp,
                                                 MneMeasData *add_to)     /* Add to this */
/*
          * Read an evoked-response data file
          */
{
    /*
       * Data read from the file
       */
    QList<FiffChInfo> chs;
    int            nchan_file,nsamp;
    float          dtmin,dtmax,sfreq;
    QString        comment;
    float          **data   = NULL;
    float          lowpass,highpass;
    int            nave;
    int            aspect_kind;
    fiffId         id = NULL;
    FiffCoordTransOld* t = NULL;
    fiffTime       meas_date = NULL;
    QString        stim14_name;
    /*
     * Desired channels
     */
    QStringList         names;
    int         nchan   = 0;
    /*
       * Selected channels
       */
    int         *sel   = NULL;
    int         stim14 = -1;
    /*
       * Other stuff
       */
    float       *source,tmin,tmax;
    int         k,p,c,np,n1,n2;
    MneMeasData*    res = NULL;
    MneMeasData*    new_data = add_to;
    MneMeasDataSet* dataset = NULL;

    stim14_name = getenv(MNE_ENV_TRIGGER_CH);
    if (stim14_name.isEmpty() || stim14_name.size() == 0)
        stim14_name = MNE_DEFAULT_TRIGGER_CH;

    if (add_to)
        mne_channel_names_to_name_list_9(add_to->chs,add_to->nchan,names,nchan);
    else {
        if (op) {
            names = op->eigen_fields->collist;
            nchan = op->nchan;
        }
        else if (fwd) {
            names = fwd->collist;
            nchan = fwd->ncol;
        }
        else {
            names = namesp;
            nchan = nnamesp;
        }
        if (names.isEmpty())
            nchan = 0;
    }
    /*
       * Read the evoked data file
       */
    if (mne_read_evoked(name,set-1,
                        &nchan_file,
                        &nsamp,
                        &dtmin,
                        &sfreq,
                        chs,
                        &data,
                        &comment,
                        &highpass,
                        &lowpass,
                        &nave,
                        &aspect_kind,
                        &t,
                        &id,
                        &meas_date) == FAIL)
        goto out;

    if (id)
        printf("\tMeasurement file id: %s\n",mne_format_file_id(id));

#ifdef FOO
    if (add_to) {			/* Should add consistency check here */
        fprintf(stderr,"\tWarning: data set consistency check is still in the works.\n");
    }
#endif
    /*
       * Pick out the necessary channels
       */
    if (nchan > 0) {
        sel     = MALLOC_9(nchan,int);
        for (k = 0; k < nchan; k++)
            sel[k] = -1;
        for (c = 0; c < nchan_file; c++) {
            for (k = 0; k < nchan; k++) {
                if (sel[k] == -1 && QString::compare(chs[c].ch_name,names[k]) == 0) {
                    sel[k] = c;
                    break;
                }
            }
            if (QString::compare(stim14_name,chs[c].ch_name) == 0) {
                stim14 = c;
            }
        }
        for (k = 0; k < nchan; k++)
            if (sel[k] == -1) {
                printf("All channels needed were not in the MEG/EEG data file "
                       "(first missing: %s).",names[k].toUtf8().constData());
                goto out;
            }
    }
    else {  /* Load all channels */
        sel = MALLOC_9(nchan_file,int);
        for (c = 0, nchan = 0; c < nchan_file; c++) {
            if (chs[c].kind == FIFFV_MEG_CH || chs[c].kind == FIFFV_EEG_CH) {
                sel[nchan] = c;
                nchan++;
            }
            if (QString::compare(stim14_name,chs[c].ch_name) == 0) {
                stim14 = c;
            }
        }
    }
    /*
       * Cut the data to the analysis time range
       */
    n1    = 0;
    n2    = nsamp;
    np    = n2 - n1;
    dtmax = dtmin + (np-1)/sfreq;
    /*
       * Then the analysis time range
       */
    tmin = dtmin;
    tmax = dtmax;
    fprintf(stderr,"\tData time range: %8.1f ... %8.1f ms\n",1000*tmin,1000*tmax);
    /*
       * Just put it together
       */
    if (!new_data) {			/* We need a new meas data structure */
        new_data     = new MneMeasData;
        new_data->filename  = name;
        new_data->meas_id   = id; id = NULL;
        /*
         * Getting starting time from measurement ID is not too accurate...
         */
        if (meas_date)
            new_data->meas_date = *meas_date;
        else {
            if (new_data->meas_id)
                new_data->meas_date = new_data->meas_id->time;
            else {
                new_data->meas_date.secs = 0;
                new_data->meas_date.usecs = 0;
            }
        }
        new_data->lowpass   = lowpass;
        new_data->highpass  = highpass;
        new_data->nchan     = nchan;
        new_data->sfreq     = sfreq;

        if (t) {
            new_data->meg_head_t    = t;
            t = NULL;
            fprintf(stderr,"\tUsing MEG <-> head transform from the present data set\n");
        }
        if (op != NULL && op->mri_head_t != NULL) { /* Copy if available */
            if (!new_data->mri_head_t)
                new_data->mri_head_t = new FiffCoordTransOld;
            *(new_data->mri_head_t) = *(op->mri_head_t);
            fprintf(stderr,"\tPicked MRI <-> head transform from the inverse operator\n");
        }
        /*
         * Channel list
         */
        for (k = 0; k < nchan; k++) {
            new_data->chs.append(FiffChInfo());
            new_data->chs[k] = chs[sel[k]];
        }

        new_data->op  = op;		/* Attach inverse operator */
        new_data->fwd = fwd;		/* ...or a fwd operator */
        if (op) 			/* Attach the projection operator and CTF compensation info to the data, too */
            new_data->proj = MneProjOp::mne_dup_proj_op(op->proj);
        else {
            new_data->proj = MneProjOp::mne_read_proj_op(name);
            if (new_data->proj && new_data->proj->nitems > 0) {
                fprintf(stderr,"\tLoaded projection from %s:\n",name.toUtf8().data());
                MneProjOp::mne_proj_op_report(stderr,"\t\t",new_data->proj);
            }
            new_data->comp = MneCTFCompDataSet::mne_read_ctf_comp_data(name);
            if (new_data->comp == NULL)
                goto out;
            if (new_data->comp->ncomp > 0)
                fprintf(stderr,"\tRead %d compensation data sets from %s\n",new_data->comp->ncomp,name.toUtf8().data());
        }
        /*
         * Th bad channel stuff
         */
        {
            int b;

            new_data->bad = MALLOC_9(new_data->nchan,int);
            for (k = 0; k < new_data->nchan; k++)
                new_data->bad[k] = FALSE;

            if (mne_read_bad_channel_list_9(name,new_data->badlist,new_data->nbad) == OK) {
                for (b = 0; b < new_data->nbad; b++) {
                    for (k = 0; k < new_data->nchan; k++) {
                        if (QString::compare(new_data->chs[k].ch_name,new_data->badlist[b],Qt::CaseInsensitive) == 0) {
                            new_data->bad[k] = TRUE;
                            break;
                        }
                    }
                }
                fprintf(stderr,"\t%d bad channels read from %s%s",new_data->nbad,name.toUtf8().data(),new_data->nbad > 0 ? ":\n" : "\n");
                if (new_data->nbad > 0) {
                    fprintf(stderr,"\t\t");
                    for (k = 0; k < new_data->nbad; k++)
                        fprintf(stderr,"%s%c",new_data->badlist[k].toUtf8().constData(),k < new_data->nbad-1 ? ' ' : '\n');
                }
            }
        }
    }
    /*
       * New data set is created anyway
       */
    dataset = new MneMeasDataSet;
    dataset->tmin      = tmin;
    dataset->tstep     = 1.0/sfreq;
    dataset->first     = n1;
    dataset->np        = np;
    dataset->nave      = nave;
    dataset->kind      = aspect_kind;
    dataset->data      = ALLOC_CMATRIX_9(np,nchan);
    dataset->comment   = comment;    comment.clear();
    dataset->baselines = MALLOC_9(nchan,float);
    /*
       * Pick data from all channels
       */
    for (k = 0; k < nchan; k++) {
        source = data[sel[k]];
        /*
         * Shift the response
         */
        for (p = 0; p < np; p++)
            dataset->data[p][k] = source[p+n1];
        dataset->baselines[k] = 0.0;
    }
    /*
       * Pick the digital trigger channel, too
       */
    if (stim14 >= 0) {
        dataset->stim14 = MALLOC_9(np,float);
        source = data[stim14];
        for (p = 0; p < np; p++) 	/* Copy the data and correct for the possible non-unit calibration */
            dataset->stim14[p] = source[p+n1]/chs[stim14].cal;
    }
    new_data->sets.append(dataset); dataset = NULL;
    new_data->nset++;
    if (!add_to)
        new_data->current = new_data->sets[0];
    res = new_data;
    fprintf(stderr,"\t%s dataset %s from %s\n",
            add_to ? "Added" : "Loaded",
            new_data->sets[new_data->nset-1]->comment.toUtf8().constData() ? new_data->sets[new_data->nset-1]->comment.toUtf8().constData() : "unknown",name.toUtf8().data());

out : {
        FREE_9(sel);
        comment.clear();
        FREE_CMATRIX_9(data);
        FREE_9(t);
        FREE_9(id);
        if (res == NULL && !add_to)
            delete new_data;
        if (add_to)
            names.clear();
        return res;
    }
}

//=============================================================================================================

MneMeasData *MneMeasData::mne_read_meas_data(const QString &name,
                                             int set,
                                             MneInverseOperator* op,
                                             MneNamedMatrix *fwd,
                                             const QStringList& namesp,
                                             int nnamesp)

{
    return mne_read_meas_data_add(name,set,op,fwd,namesp,nnamesp,NULL);
}
