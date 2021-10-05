//=============================================================================================================
/**
 * @file     mne_ctf_comp_data_set.cpp
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
 * @brief    Definition of the MneCTFCompDataSet Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_ctf_comp_data_set.h"
#include "mne_ctf_comp_data.h"

#include "mne_types.h"

#include <fiff/c/fiff_coord_trans_old.h>

#include <fiff/fiff_types.h>

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>

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

#define MALLOC_32(x,t) (t *)malloc((x)*sizeof(t))
#define REALLOC_32(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))

#define FREE_32(x) if ((char *)(x) != NULL) free((char *)(x))

#define FREE_CMATRIX_32(m) mne_free_cmatrix_32((m))
#define ALLOC_CMATRIX_32(x,y) mne_cmatrix_32((x),(y))

static void matrix_error_32(int kind, int nr, int nc)

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

float **mne_cmatrix_32(int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC_32(nr,float *);
    if (!m) matrix_error_32(1,nr,nc);
    whole = MALLOC_32(nr*nc,float);
    if (!whole) matrix_error_32(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}

void mne_free_cmatrix_32 (float **m)
{
    if (m) {
        FREE_32(*m);
        FREE_32(m);
    }
}

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;

//============================= mne_read_forward_solution.c =============================

int mne_read_meg_comp_eeg_ch_info_32(const QString& name,
                                  QList<FIFFLIB::FiffChInfo>& megp,	 /* MEG channels */
                                  int *nmegp,
                                  QList<FIFFLIB::FiffChInfo>& meg_compp,
                                  int *nmeg_compp,
                                  QList<FIFFLIB::FiffChInfo>& eegp,	 /* EEG channels */
                                  int *neegp,
                                  FiffCoordTransOld* *meg_head_t,
                                  fiffId *idp)	 /* The measurement ID */
/*
      * Read the channel information and split it into three arrays,
      * one for MEG, one for MEG compensation channels, and one for EEG
      */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    QList<FIFFLIB::FiffChInfo> chs;
    int        nchan = 0;
    QList<FIFFLIB::FiffChInfo> meg;
    int        nmeg  = 0;
    QList<FIFFLIB::FiffChInfo> meg_comp;
    int        nmeg_comp = 0;
    QList<FIFFLIB::FiffChInfo> eeg;
    int        neeg  = 0;
    fiffId     id    = NULL;
    QList<FiffDirNode::SPtr> nodes;
    FiffDirNode::SPtr info;
    FiffTag::SPtr t_pTag;
    FIFFLIB::FiffChInfo   this_ch;
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

    eegp = eeg;
    if(neegp) {
        *neegp = neeg;
    }

    if (idp == NULL) {
        FREE_32(id);
    }
    else
        *idp   = id;
    if (meg_head_t == NULL) {
        FREE_32(t);
    }
    else
        *meg_head_t = t;

    return FIFF_OK;

bad : {
//        fiff_close(in);
        stream->close();
        FREE_32(id);
//        FREE_32(tag.data);
        FREE_32(t);
        return FIFF_FAIL;
    }
}

#define MNE_CTFV_COMP_UNKNOWN -1
#define MNE_CTFV_COMP_NONE    0
#define MNE_CTFV_COMP_G1BR    0x47314252
#define MNE_CTFV_COMP_G2BR    0x47324252
#define MNE_CTFV_COMP_G3BR    0x47334252
#define MNE_CTFV_COMP_G2OI    0x47324f49
#define MNE_CTFV_COMP_G3OI    0x47334f49

static struct {
    int grad_comp;
    int ctf_comp;
} compMap[] = { { MNE_CTFV_NOGRAD,       MNE_CTFV_COMP_NONE },
{ MNE_CTFV_GRAD1,        MNE_CTFV_COMP_G1BR },
{ MNE_CTFV_GRAD2,        MNE_CTFV_COMP_G2BR },
{ MNE_CTFV_GRAD3,        MNE_CTFV_COMP_G3BR },
{ MNE_4DV_COMP1,         MNE_4DV_COMP1 },             /* One-to-one mapping for 4D data */
{ MNE_CTFV_COMP_UNKNOWN, MNE_CTFV_COMP_UNKNOWN }};

int mne_unmap_ctf_comp_kind(int ctf_comp)

{
    int k;

    for (k = 0; compMap[k].grad_comp >= 0; k++)
        if (ctf_comp == compMap[k].ctf_comp)
            return compMap[k].grad_comp;
    return ctf_comp;
}

FiffSparseMatrix* mne_convert_to_sparse(float **dense,        /* The dense matrix to be converted */
                                      int   nrow,           /* Number of rows in the dense matrix */
                                      int   ncol,           /* Number of columns in the dense matrix */
                                      int   stor_type,      /* Either FIFFTS_MC_CCS or FIFFTS_MC_RCS */
                                      float small)          /* How small elements should be ignored? */
/*
 * Create the compressed row or column storage sparse matrix representation
 * including a vector containing the nonzero matrix element values,
 * the row or column pointer vector and the appropriate index vector(s).
 */
{
    int j,k;
    int nz;
    int ptr;
    FiffSparseMatrix* sparse = NULL;
    int size;

    if (small < 0) {		/* Automatic scaling */
        float maxval = 0.0;
        float val;

        for (j = 0; j < nrow; j++)
            for (k = 0; k < ncol; k++) {
                val = std::fabs(dense[j][k]);
                if (val > maxval)
                    maxval = val;
            }
        if (maxval > 0)
            small = maxval*std::fabs(small);
        else
            small = std::fabs(small);
    }
    for (j = 0, nz = 0; j < nrow; j++)
        for (k = 0; k < ncol; k++) {
            if (std::fabs(dense[j][k]) > small)
                nz++;
        }

    if (nz <= 0) {
        printf("No nonzero elements found.");
        return NULL;
    }
    if (stor_type == FIFFTS_MC_CCS) {
        size = nz*(sizeof(fiff_float_t) + sizeof(fiff_int_t)) +
                (ncol+1)*(sizeof(fiff_int_t));
    }
    else if (stor_type == FIFFTS_MC_RCS) {
        size = nz*(sizeof(fiff_float_t) + sizeof(fiff_int_t)) +
                (nrow+1)*(sizeof(fiff_int_t));
    }
    else {
        printf("Unknown sparse matrix storage type: %d",stor_type);
        return NULL;
    }
    sparse = new FiffSparseMatrix;
    sparse->coding = stor_type;
    sparse->m      = nrow;
    sparse->n      = ncol;
    sparse->nz     = nz;
    sparse->data   = (float *)malloc(size);
    sparse->inds   = (int *)(sparse->data+nz);
    sparse->ptrs   = sparse->inds+nz;

    if (stor_type == FIFFTS_MC_RCS) {
        for (j = 0, nz = 0; j < nrow; j++) {
            ptr = -1;
            for (k = 0; k < ncol; k++)
                if (std::fabs(dense[j][k]) > small) {
                    sparse->data[nz] = dense[j][k];
                    if (ptr < 0)
                        ptr = nz;
                    sparse->inds[nz++] = k;
                }
            sparse->ptrs[j] = ptr;
        }
        sparse->ptrs[nrow] = nz;
        for (j = nrow - 1; j >= 0; j--) /* Take care of the empty rows */
            if (sparse->ptrs[j] < 0)
                sparse->ptrs[j] = sparse->ptrs[j+1];
    }
    else if (stor_type == FIFFTS_MC_CCS) {
        for (k = 0, nz = 0; k < ncol; k++) {
            ptr = -1;
            for (j = 0; j < nrow; j++)
                if (std::fabs(dense[j][k]) > small) {
                    sparse->data[nz] = dense[j][k];
                    if (ptr < 0)
                        ptr = nz;
                    sparse->inds[nz++] = j;
                }
            sparse->ptrs[k] = ptr;
        }
        sparse->ptrs[ncol] = nz;
        for (k = ncol-1; k >= 0; k--) /* Take care of the empty columns */
            if (sparse->ptrs[k] < 0)
                sparse->ptrs[k] = sparse->ptrs[k+1];
    }
    return sparse;
}

int  mne_sparse_mat_mult2_32(FiffSparseMatrix* mat,     /* The sparse matrix */
                          float           **mult,  /* Matrix to be multiplied */
                          int             ncol,	   /* How many columns in the above */
                          float           **res)   /* Result of the multiplication */
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
                    val += mat->data[j]*mult[mat->inds[j]][k];
                res[i][k] = val;
            }
        }
    }
    else if (mat->coding == FIFFTS_MC_CCS) {
        for (k = 0; k < ncol; k++) {
            for (i = 0; i < mat->m; i++)
                res[i][k] = 0.0;
            for (i = 0; i < mat->n; i++)
                for (j = mat->ptrs[i]; j < mat->ptrs[i+1]; j++)
                    res[mat->inds[j]][k] += mat->data[j]*mult[i][k];
        }
    }
    else {
        printf("mne_sparse_mat_mult2: unknown sparse matrix storage type: %d",mat->coding);
        return -1;
    }
    return 0;
}

float **mne_mat_mat_mult_32 (float **m1,float **m2,int d1,int d2,int d3)
/* Matrix multiplication
      * result(d1 x d3) = m1(d1 x d2) * m2(d2 x d3) */

{
#ifdef BLAS
    float **result = ALLOC_CMATRIX_32(d1,d3);
    char  *transa = "N";
    char  *transb = "N";
    float zero = 0.0;
    float one  = 1.0;
    sgemm (transa,transb,&d3,&d1,&d2,
           &one,m2[0],&d3,m1[0],&d2,&zero,result[0],&d3);
    return (result);
#else
    float **result = ALLOC_CMATRIX_32(d1,d3);
    int j,k,p;
    float sum;

    for (j = 0; j < d1; j++)
        for (k = 0; k < d3; k++) {
            sum = 0.0;
            for (p = 0; p < d2; p++)
                sum = sum + m1[j][p]*m2[p][k];
            result[j][k] = sum;
        }
    return (result);
#endif
}

int  mne_sparse_vec_mult2_32(FiffSparseMatrix* mat,     /* The sparse matrix */
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
        printf("mne_sparse_vec_mult2: unknown sparse matrix storage type: %d",mat->coding);
        return -1;
    }
}

float mne_dot_vectors_32 (float *v1,
                       float *v2,
                       int   nn)

{
#ifdef BLAS
    int one = 1;
    float res = sdot(&nn,v1,&one,v2,&one);
    return res;
#else
    float res = 0.0;
    int   k;

    for (k = 0; k < nn; k++)
        res = res + v1[k]*v2[k];
    return res;
#endif
}

void mne_mat_vec_mult2_32 (float **m,float *v,float *result, int d1,int d2)
/*
      * Matrix multiplication
      * result(d1) = m(d1 x d2) * v(d2)
      */

{
    int j;

    for (j = 0; j < d1; j++)
        result[j] = mne_dot_vectors_32 (m[j],v,d2);
    return;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneCTFCompDataSet::MneCTFCompDataSet()
:ncomp(0)
,nch(0)
,current(NULL)
,undo(NULL)
{
}

//=============================================================================================================

MneCTFCompDataSet::MneCTFCompDataSet(const MneCTFCompDataSet &set)
{
//    if (!set)
//        return NULL;

    if (set.ncomp > 0) {
//        this->comps = MALLOC_32(set.ncomp,mneCTFcompData);
        this->ncomp = set.comps.size();
        for (int k = 0; k < this->ncomp; k++)
            if(set.comps[k])
                this->comps.append(new MneCTFCompData(*set.comps[k]));
    }

    if(set.current)
        this->current = new MneCTFCompData(*set.current);
}

//=============================================================================================================

MneCTFCompDataSet::~MneCTFCompDataSet()
{

    for (int k = 0; k < comps.size(); k++)
        if(comps[k])
            delete comps[k];

    if(current)
        delete current;
}

//=============================================================================================================

MneCTFCompDataSet *MneCTFCompDataSet::mne_read_ctf_comp_data(const QString &name)
/*
     * Read all CTF compensation data from a given file
     */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    MneCTFCompDataSet* set = NULL;
    MneCTFCompData* one;
    QList<FiffDirNode::SPtr> nodes;
    QList<FiffDirNode::SPtr> comps;
    int ncomp;
    MneNamedMatrix* mat = NULL;
    int kind,k;
    FiffTag::SPtr t_pTag;
    QList<FiffChInfo> chs;
    int nch = 0;
    int calibrated;
    /*
        * Read the channel information
        */
    {
        QList<FiffChInfo> comp_chs, temp;
        int ncompch = 0;

        if (mne_read_meg_comp_eeg_ch_info_32(name,
                                             chs,
                                             &nch,
                                             comp_chs,
                                             &ncompch,
                                             temp,
                                             NULL,
                                             NULL,
                                             NULL) == FAIL)
            goto bad;
        if (ncompch > 0) {
            for (k = 0; k < ncompch; k++)
                chs.append(comp_chs[k]);
            nch = nch + ncompch;
        }
    }
    /*
        * Read the rest of the stuff
        */
    if(!stream->open())
        goto bad;
    set = new MneCTFCompDataSet();
    /*
        * Locate the compensation data sets
        */
    nodes = stream->dirtree()->dir_tree_find(FIFFB_MNE_CTF_COMP);
    if (nodes.size() == 0)
        goto good;      /* Nothing more to do */
    comps = nodes[0]->dir_tree_find(FIFFB_MNE_CTF_COMP_DATA);
    if (comps.size() == 0)
        goto good;
    ncomp = comps.size();
    /*
        * Set the channel info
        */
    set->chs = chs;
    set->nch = nch;
    /*
        * Read each data set
        */
    for (k = 0; k < ncomp; k++) {
        mat = MneNamedMatrix::read_named_matrix(stream,comps[k],FIFF_MNE_CTF_COMP_DATA);
        if (!mat)
            goto bad;
        comps[k]->find_tag(stream, FIFF_MNE_CTF_COMP_KIND, t_pTag);
        if (t_pTag) {
            kind = *t_pTag->toInt();
        }
        else
            goto bad;
        comps[k]->find_tag(stream, FIFF_MNE_CTF_COMP_CALIBRATED, t_pTag);
        if (t_pTag) {
            calibrated = *t_pTag->toInt();
        }
        else
            calibrated = FALSE;
        /*
            * Add these data to the set
            */
        one = new MneCTFCompData;
        one->data = mat; mat = NULL;
        one->kind                = kind;
        one->mne_kind            = mne_unmap_ctf_comp_kind(one->kind);
        one->calibrated          = calibrated;

        if (MneCTFCompData::mne_calibrate_ctf_comp(one,set->chs,set->nch,TRUE) == FAIL) {
            printf("Warning: Compensation data for '%s' omitted\n", mne_explain_ctf_comp(one->kind));//,err_get_error(),mne_explain_ctf_comp(one->kind));
            if(one)
                delete one;
        }
        else {
            //            set->comps               = REALLOC_9(set->comps,set->ncomp+1,mneCTFcompData);
            //            set->comps[set->ncomp++] = one;
            set->comps.append(one);
            set->ncomp++;
        }
    }
#ifdef DEBUG
    printf("%d CTF compensation data sets read from %s\n",set->ncomp,name);
#endif
    goto good;

bad : {
        if(mat)
            delete mat;
        stream->close();
        if(set)
            delete set;
        return NULL;
    }

good : {
        stream->close();
        return set;
    }
}

//=============================================================================================================

int MneCTFCompDataSet::mne_make_ctf_comp(MneCTFCompDataSet* set,
                                         const QList<FiffChInfo>& chs,
                                         int nch,
                                         QList<FiffChInfo> compchs,
                                         int ncomp)      /* How many of these */
/*
     * Make compensation data to apply to a set of channels to yield (or uncompensated) compensated data
     */
{
    int *comps = NULL;
    int need_comp;
    int first_comp;
    MneCTFCompData* this_comp;
    int  *comp_sel = NULL;
    QStringList names;
    QString name;
    int  j,k,p;

    FiffSparseMatrix* presel  = NULL;
    FiffSparseMatrix* postsel = NULL;
    MneNamedMatrix*  data    = NULL;

    QStringList emptyList;

    if (compchs.isEmpty()) {
        compchs = chs;
        ncomp   = nch;
    }
    printf("Setting up compensation data...\n");
    if (nch == 0)
        return OK;
    if (set) {
        if(set->current)
            delete set->current;
        set->current = NULL;
    }
    comps = MALLOC_32(nch,int);
    for (k = 0, need_comp = 0, first_comp = MNE_CTFV_COMP_NONE; k < nch; k++) {
        if (chs[k].kind == FIFFV_MEG_CH) {
            comps[k] = chs[k].chpos.coil_type >> 16;
            if (comps[k] != MNE_CTFV_COMP_NONE) {
                if (first_comp == MNE_CTFV_COMP_NONE)
                    first_comp = comps[k];
                else {
                    if (comps[k] != first_comp) {
                        printf("We do not support nonuniform compensation yet.");
                        goto bad;
                    }
                }
                need_comp++;
            }
        }
        else
            comps[k] = MNE_CTFV_COMP_NONE;
    }
    if (need_comp == 0) {
        printf("\tNo compensation set. Nothing more to do.\n");
        FREE_32(comps);
        return OK;
    }
    printf("\t%d out of %d channels have the compensation set.\n",need_comp,nch);
    if (!set) {
        printf("No compensation data available for the required compensation.");
        return FAIL;
    }
    /*
        * Find the desired compensation data matrix
        */
    for (k = 0, this_comp = NULL; k < set->ncomp; k++) {
        if (set->comps[k]->mne_kind == first_comp) {
            this_comp = set->comps[k];
            break;
        }
    }
    if (!this_comp) {
        printf("Did not find the desired compensation data : %s",
               mne_explain_ctf_comp(mne_map_ctf_comp_kind(first_comp)));
        goto bad;
    }
    printf("\tDesired compensation data (%s) found.\n",mne_explain_ctf_comp(mne_map_ctf_comp_kind(first_comp)));
    /*
        * Find the compensation channels
        */
    comp_sel = MALLOC_32(this_comp->data->ncol,int);
    for (k = 0; k < this_comp->data->ncol; k++) {
        comp_sel[k] = -1;
        name = this_comp->data->collist[k];
        for (p = 0; p < ncomp; p++)
            if (QString::compare(name,compchs[p].ch_name) == 0) {
                comp_sel[k] = p;
                break;
            }
        if (comp_sel[k] < 0) {
            printf("Compensation channel %s not found",name.toUtf8().constData());
            goto bad;
        }
    }
    printf("\tAll compensation channels found.\n");
    /*
        * Create the preselector
        */
    {
        float **sel = ALLOC_CMATRIX_32(this_comp->data->ncol,ncomp);
        for (j = 0; j < this_comp->data->ncol; j++) {
            for (k = 0; k < ncomp; k++)
                sel[j][k] = 0.0;
            sel[j][comp_sel[j]] = 1.0;
        }
        if ((presel = mne_convert_to_sparse(sel,this_comp->data->ncol,ncomp,FIFFTS_MC_RCS,1e-30)) == NULL) {
            FREE_CMATRIX_32(sel);
            goto bad;
        }
        FREE_CMATRIX_32(sel);
        printf("\tPreselector created.\n");
    }
    /*
     * Pick the desired channels
     */
    for (k = 0; k < nch; k++) {
        if (comps[k] != MNE_CTFV_COMP_NONE)
            names.append(chs[k].ch_name);
    }

    if ((data = this_comp->data->pick_from_named_matrix(names,need_comp,emptyList,0)) == NULL)
        goto bad;
    printf("\tCompensation data matrix created.\n");
    /*
        * Create the postselector
        */
    {
        float **sel = ALLOC_CMATRIX_32(nch,data->nrow);
        for (j = 0, p = 0; j < nch; j++) {
            for (k = 0; k < data->nrow; k++)
                sel[j][k] = 0.0;
            if (comps[j] != MNE_CTFV_COMP_NONE)
                sel[j][p++] = 1.0;
        }
        if ((postsel = mne_convert_to_sparse(sel,nch,data->nrow,FIFFTS_MC_RCS,1e-30)) == NULL) {
            FREE_CMATRIX_32(sel);
            goto bad;
        }
        FREE_CMATRIX_32(sel);
        printf("\tPostselector created.\n");
    }
    set->current           = new MneCTFCompData();
    set->current->kind     = this_comp->kind;
    set->current->mne_kind = this_comp->mne_kind;
    set->current->data     = data;
    set->current->presel   = presel;
    set->current->postsel  = postsel;

    printf("\tCompensation set up.\n");

    names.clear();
    FREE_32(comps);
    FREE_32(comp_sel);

    return OK;

bad : {
        if(presel)
            delete presel;
        if(postsel)
            delete postsel;
        if(data)
            delete data;
        names.clear();
        FREE_32(comps);
        FREE_32(comp_sel);
        return FAIL;
    }
}

//=============================================================================================================

int MneCTFCompDataSet::mne_set_ctf_comp(QList<FIFFLIB::FiffChInfo>& chs,
                                        int nch,
                                        int comp)
/*
     * Set the compensation bits to the desired value
     */
{
    int k;
    int nset;
    for (k = 0, nset = 0; k < nch; k++) {
        if (chs[k].kind == FIFFV_MEG_CH) {
            chs[k].chpos.coil_type = (chs[k].chpos.coil_type & 0xFFFF) | (comp << 16);
            nset++;
        }
    }
    printf("A new compensation value (%s) was assigned to %d MEG channels.\n",
            mne_explain_ctf_comp(mne_map_ctf_comp_kind(comp)),nset);
    return nset;
}

//=============================================================================================================

int MneCTFCompDataSet::mne_apply_ctf_comp(MneCTFCompDataSet *set, int do_it, float *data, int ndata, float *compdata, int ncompdata)
/*
     * Apply compensation or revert to uncompensated data
     */
{
    MneCTFCompData* this_comp;
    float *presel,*comp;
    int   k;

    if (compdata == NULL) {
        compdata  = data;
        ncompdata = ndata;
    }
    if (!set || !set->current)
        return OK;
    this_comp = set->current;
    /*
       * Dimension checks
       */
    if (this_comp->presel) {
        if (this_comp->presel->n != ncompdata) {
            printf("Compensation data dimension mismatch. Expected %d, got %d channels.",
                   this_comp->presel->n,ncompdata);
            return FAIL;
        }
    }
    else if (this_comp->data->ncol != ncompdata) {
        printf("Compensation data dimension mismatch. Expected %d, got %d channels.",
               this_comp->data->ncol,ncompdata);
        return FAIL;
    }
    if (this_comp->postsel) {
        if (this_comp->postsel->m != ndata) {
            printf("Data dimension mismatch. Expected %d, got %d channels.",
                   this_comp->postsel->m,ndata);
            return FAIL;
        }
    }
    else if (this_comp->data->nrow != ndata) {
        printf("Data dimension mismatch. Expected %d, got %d channels.",
               this_comp->data->nrow,ndata);
        return FAIL;
    }
    /*
        * Preselection is optional
        */
    if (this_comp->presel) {
        if (!this_comp->presel_data)
            this_comp->presel_data = MALLOC_32(this_comp->presel->m,float);
        if (mne_sparse_vec_mult2_32(this_comp->presel,compdata,this_comp->presel_data) != OK)
            return FAIL;
        presel = this_comp->presel_data;
    }
    else
        presel = compdata;
    /*
        * This always happens
        */
    if (!this_comp->comp_data)
        this_comp->comp_data = MALLOC_32(this_comp->data->nrow,float);
    mne_mat_vec_mult2_32(this_comp->data->data,presel,this_comp->comp_data,this_comp->data->nrow,this_comp->data->ncol);
    /*
        * Optional postselection
        */
    if (!this_comp->postsel)
        comp = this_comp->comp_data;
    else {
        if (!this_comp->postsel_data) {
            this_comp->postsel_data = MALLOC_32(this_comp->postsel->m,float);
        }
        if (mne_sparse_vec_mult2_32(this_comp->postsel,this_comp->comp_data,this_comp->postsel_data) != OK)
            return FAIL;
        comp = this_comp->postsel_data;
    }
    /*
        * Compensate or revert compensation?
        */
    if (do_it) {
        for (k = 0; k < ndata; k++)
            data[k] = data[k] - comp[k];
    }
    else {
        for (k = 0; k < ndata; k++)
            data[k] = data[k] + comp[k];
    }
    return OK;
}

//=============================================================================================================

int MneCTFCompDataSet::mne_apply_ctf_comp_t(MneCTFCompDataSet *set, int do_it, float **data, int ndata, int ns)      /* Number of samples */
/*
     * Apply compensation or revert to uncompensated data
     */
{
    MneCTFCompData* this_comp;
    float **presel,**comp;
    float **compdata = data;
    int   ncompdata  = ndata;
    int   k,p;

    if (!set || !set->current)
        return OK;
    this_comp = set->current;
    /*
        * Dimension checks
        */
    if (this_comp->presel) {
        if (this_comp->presel->n != ncompdata) {
            printf("Compensation data dimension mismatch. Expected %d, got %d channels.",
                   this_comp->presel->n,ncompdata);
            return FAIL;
        }
    }
    else if (this_comp->data->ncol != ncompdata) {
        printf("Compensation data dimension mismatch. Expected %d, got %d channels.",
               this_comp->data->ncol,ncompdata);
        return FAIL;
    }
    if (this_comp->postsel) {
        if (this_comp->postsel->m != ndata) {
            printf("Data dimension mismatch. Expected %d, got %d channels.",
                   this_comp->postsel->m,ndata);
            return FAIL;
        }
    }
    else if (this_comp->data->nrow != ndata) {
        printf("Data dimension mismatch. Expected %d, got %d channels.",
               this_comp->data->nrow,ndata);
        return FAIL;
    }
    /*
        * Preselection is optional
        */
    if (this_comp->presel) {
        presel = ALLOC_CMATRIX_32(this_comp->presel->m,ns);
        if (mne_sparse_mat_mult2_32(this_comp->presel,compdata,ns,presel) != OK) {
            FREE_CMATRIX_32(presel);
            return FAIL;
        }
    }
    else
        presel = data;
    /*
        * This always happens
        */
    comp = mne_mat_mat_mult_32(this_comp->data->data,presel,this_comp->data->nrow,this_comp->data->ncol,ns);
    if (this_comp->presel)
        FREE_CMATRIX_32(presel);
    /*
        * Optional postselection
        */
    if (this_comp->postsel) {
        float **postsel = ALLOC_CMATRIX_32(this_comp->postsel->m,ns);
        if (mne_sparse_mat_mult2_32(this_comp->postsel,comp,ns,postsel) != OK) {
            FREE_CMATRIX_32(postsel);
            return FAIL;
        }
        FREE_CMATRIX_32(comp);
        comp = postsel;
    }
    /*
        * Compensate or revert compensation?
        */
    if (do_it) {
        for (k = 0; k < ndata; k++)
            for (p = 0; p < ns; p++)
                data[k][p] = data[k][p] - comp[k][p];
    }
    else {
        for (k = 0; k < ndata; k++)
            for (p = 0; p < ns; p++)
                data[k][p] = data[k][p] + comp[k][p];
    }
    FREE_CMATRIX_32(comp);
    return OK;
}

//=============================================================================================================

int MneCTFCompDataSet::mne_get_ctf_comp(const QList<FIFFLIB::FiffChInfo> &chs, int nch)
{
    int res = MNE_CTFV_NOGRAD;
    int first_comp,comp;
    int k;

    for (k = 0, first_comp = -1; k < nch; k++) {
        if (chs[k].kind == FIFFV_MEG_CH) {
            comp = chs[k].chpos.coil_type >> 16;
            if (first_comp < 0)
                first_comp = comp;
            else if (first_comp != comp) {
                printf("Non uniform compensation not supported.");
                return FAIL;
            }
        }
    }
    if (first_comp >= 0)
        res = first_comp;
    return res;
}

//=============================================================================================================

int MneCTFCompDataSet::mne_map_ctf_comp_kind(int grad)
/*
     * Simple mapping
     */
{
    int k;

    for (k = 0; compMap[k].grad_comp >= 0; k++)
        if (grad == compMap[k].grad_comp)
            return compMap[k].ctf_comp;
    return grad;
}

//=============================================================================================================

const char *MneCTFCompDataSet::mne_explain_ctf_comp(int kind)
{
    static struct {
        int kind;
        const char *expl;
    } explain[] = { { MNE_CTFV_COMP_NONE,    "uncompensated" },
    { MNE_CTFV_COMP_G1BR,    "first order gradiometer" },
    { MNE_CTFV_COMP_G2BR,    "second order gradiometer" },
    { MNE_CTFV_COMP_G3BR,    "third order gradiometer" },
    { MNE_4DV_COMP1,         "4D comp 1" },
    { MNE_CTFV_COMP_UNKNOWN, "unknown" } };
    int k;

    for (k = 0; explain[k].kind != MNE_CTFV_COMP_UNKNOWN; k++)
        if (explain[k].kind == kind)
            return explain[k].expl;
    return explain[k].expl;
}

//=============================================================================================================

int MneCTFCompDataSet::mne_ctf_set_compensation(MneCTFCompDataSet *set,
                                                int compensate_to,
                                                QList<FiffChInfo>& chs,
                                                int nchan,
                                                QList<FiffChInfo> comp_chs,
                                                int ncomp_chan)     /* How many */
/*
     * Make data which has the third-order gradient compensation applied
     */
{
    int k;
    int have_comp_chs;
    int comp_was = MNE_CTFV_COMP_UNKNOWN;

    if (!set) {
        if (compensate_to == MNE_CTFV_NOGRAD)
            return OK;
        else {
            printf("Cannot do compensation because compensation data are missing");
            return FAIL;
        }
    }
    if (comp_chs.isEmpty()) {
        comp_chs = chs;
        ncomp_chan = nchan;
    }
    if (set) {
        if(set->undo)
            delete set->undo;
        set->undo = NULL;
        if(set->current)
            delete set->current;
        set->current = NULL;
    }
    for (k = 0, have_comp_chs = 0; k < ncomp_chan; k++)
        if (comp_chs[k].kind == FIFFV_REF_MEG_CH)
            have_comp_chs++;
    if (have_comp_chs == 0 && compensate_to != MNE_CTFV_NOGRAD) {
        printf("No compensation channels in these data.");
        return FAIL;
    }
    /*
        * Update the 'current' field in 'set' to reflect the compensation possibly present in the data now
        */
    if (mne_make_ctf_comp(set,chs,nchan,comp_chs,ncomp_chan) == FAIL)
        goto bad;
    /*
        * Are we there already?
        */
    if (set->current && set->current->mne_kind == compensate_to) {
        printf("No further compensation necessary (comp = %s)\n",mne_explain_ctf_comp(set->current->kind));
        if(set->current)
            delete set->current;
        set->current = NULL;
        return OK;
    }
    set->undo    = set->current;
    set->current = NULL;
    if (compensate_to == MNE_CTFV_NOGRAD) {
        printf("No compensation was requested.\n");
        mne_set_ctf_comp(chs,nchan,compensate_to);
        return OK;
    }
    if (mne_set_ctf_comp(chs,nchan,compensate_to) > 0) {
        if (set->undo)
            comp_was = set->undo->mne_kind;
        else
            comp_was = MNE_CTFV_NOGRAD;
        if (mne_make_ctf_comp(set,chs,nchan,comp_chs,ncomp_chan) == FAIL)
            goto bad;
        printf("Compensation set up as requested (%s -> %s).\n",
                mne_explain_ctf_comp(mne_map_ctf_comp_kind(comp_was)),
                mne_explain_ctf_comp(set->current->kind));
    }
    return OK;

bad : {
        if (comp_was != MNE_CTFV_COMP_UNKNOWN)
            mne_set_ctf_comp(chs,nchan,comp_was);
        return FAIL;
    }
}
