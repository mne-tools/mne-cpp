

#include "compute_fwd.h"

#include <fiff/fiff.h>
#include <fiff/c/fiff_coord_trans_old.h>
#include "../fwd_coil_set.h"
#include "../fwd_comp_data.h"
#include <mne/c/mne_ctf_comp_data_set.h>
#include "../fwd_eeg_sphere_model_set.h"
#include "../fwd_bem_model.h"

#include <mne/c/mne_named_matrix.h>
#include <mne/c/mne_nearest.h>
#include <mne/c/mne_source_space_old.h>
#include <mne/mne_forwardsolution.h>

#include <fiff/c/fiff_sparse_matrix.h>

#include <fiff/fiff_types.h>

#include <time.h>

#include <Eigen/Dense>

#include <QCoreApplication>
#include <QFile>
#include <QDir>

using namespace Eigen;
using namespace FWDLIB;
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

#define X_41 0
#define Y_41 1
#define Z_41 2

#define ALLOC_ICMATRIX_41(x,y) mne_imatrix_41((x),(y))

#define MALLOC_41(x,t) (t *)malloc((x)*sizeof(t))
#define REALLOC_41(x,y,t) (t *)((x == Q_NULLPTR) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))

#define FREE_41(x) if ((char *)(x) != Q_NULLPTR) free((char *)(x))

#define VEC_COPY_41(to,from) {\
    (to)[X_41] = (from)[X_41];\
    (to)[Y_41] = (from)[Y_41];\
    (to)[Z_41] = (from)[Z_41];\
    }

#define VEC_DOT_41(x,y) ((x)[X_41]*(y)[X_41] + (x)[Y_41]*(y)[Y_41] + (x)[Z_41]*(y)[Z_41])

#define VEC_LEN_41(x) sqrt(VEC_DOT_41(x,x))

#define ALLOC_CMATRIX_41(x,y) mne_cmatrix_41((x),(y))
#define FREE_CMATRIX_41(m) mne_free_cmatrix_41((m))
#define FREE_ICMATRIX_41(m) mne_free_icmatrix_41((m))

static void matrix_error_41(int kind, int nr, int nc)

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

float **mne_cmatrix_41(int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC_41(nr,float *);
    if (!m) matrix_error_41(1,nr,nc);
    whole = MALLOC_41(nr*nc,float);
    if (!whole) matrix_error_41(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}

int **mne_imatrix_41(int nr,int nc)

{
    int i,**m;
    int *whole;

    m = MALLOC_41(nr,int *);
    if (!m) matrix_error_41(1,nr,nc);
    whole = MALLOC_41(nr*nc,int);
    if (!whole) matrix_error_41(2,nr,nc);
    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}

void mne_free_cmatrix_41 (float **m)
{
    if (m) {
        FREE_41(*m);
        FREE_41(m);
    }
}

void mne_free_icmatrix_41 (int **m)

{
    if (m) {
        FREE_41(*m);
        FREE_41(m);
    }
}

fiffId get_file_id(const QString& name)
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));
    fiffId   id;
    if(!stream->open()) {
        stream->close();
        return Q_NULLPTR;
    }
    else {
        id = MALLOC_41(1,fiffIdRec);
        id->version = stream->id().version;       /**< File version. */
        id->machid[0] = stream->id().machid[0];   /**< Unique machine ID. */
        id->machid[1] = stream->id().machid[1];
        id->time = stream->id().time;             /**< Time of the ID creation. */

        stream->close();
        return id;
    }
}

//============================= mne_read_forward_solution.c =============================

//int mne_read_meg_comp_eeg_ch_info_41(const QString& name,
//                                     QList<FiffChInfo>& megp,	 /* MEG channels */
//                                     int* nmegp,
//                                     QList<FiffChInfo>& meg_compp,
//                                     int* nmeg_compp,
//                                     QList<FiffChInfo>& eegp,	 /* EEG channels */
//                                     int* neegp,
//                                     FiffCoordTransOld** meg_head_t,
//                                     fiffId* idp)	 /* The measurement ID */
///*
//      * Read the channel information and split it into three arrays,
//      * one for MEG, one for MEG compensation channels, and one for EEG
//      */
//{
//    QFile file(name);
//    FiffStream::SPtr stream(new FiffStream(&file));

//    QList<FiffChInfo> chs;
//    int nchan = 0;
//    QList<FiffChInfo> meg;
//    int nmeg = 0;
//    QList<FiffChInfo> meg_comp;
//    int nmeg_comp = 0;
//    QList<FiffChInfo> eeg;
//    int neeg = 0;
//    fiffId id = Q_NULLPTR;
//    QList<FiffDirNode::SPtr> nodes;
//    FiffDirNode::SPtr info;
//    FiffTag::SPtr t_pTag;
//    FiffChInfo this_ch;
//    FiffCoordTransOld* t = Q_NULLPTR;
//    fiff_int_t kind, pos;
//    int j,k,to_find;

//    if(!stream->open())
//        goto bad;

//    nodes = stream->dirtree()->dir_tree_find(FIFFB_MNE_PARENT_MEAS_FILE);

//    if (nodes.size() == 0) {
//        nodes = stream->dirtree()->dir_tree_find(FIFFB_MEAS_INFO);
//        if (nodes.size() == 0) {
//            qCritical ("Could not find the channel information.");
//            goto bad;
//        }
//    }
//    info = nodes[0];
//    to_find = 0;
//    for (k = 0; k < info->nent(); k++) {
//        kind = info->dir[k]->kind;
//        pos  = info->dir[k]->pos;
//        switch (kind) {
//        case FIFF_NCHAN :
//            if (!stream->read_tag(t_pTag,pos))
//                goto bad;
//            nchan = *t_pTag->toInt();

//            for (j = 0; j < nchan; j++) {
//                chs.append(FiffChInfo());
//                chs[j].scanNo = -1;
//            }
//            to_find = nchan;
//            break;

//        case FIFF_PARENT_BLOCK_ID :
//            if(!stream->read_tag(t_pTag, pos))
//                goto bad;
//            //            id = t_pTag->toFiffID();
//            *id = *(fiffId)t_pTag->data();
//            break;

//        case FIFF_COORD_TRANS :
//            if(!stream->read_tag(t_pTag, pos))
//                goto bad;
//            //            t = t_pTag->toCoordTrans();
//            t = FiffCoordTransOld::read_helper( t_pTag );
//            if (t->from != FIFFV_COORD_DEVICE || t->to   != FIFFV_COORD_HEAD)
//                t = Q_NULLPTR;
//            break;

//        case FIFF_CH_INFO : /* Information about one channel */
//            if(!stream->read_tag(t_pTag, pos))
//                goto bad;
//            this_ch = t_pTag->toChInfo();
//            if (this_ch.scanNo <= 0 || this_ch.scanNo > nchan) {
//                printf ("FIFF_CH_INFO : scan # out of range %d (%d)!",this_ch.scanNo,nchan);
//                goto bad;
//            }
//            else
//                chs[this_ch.scanNo-1] = this_ch;
//            to_find--;
//            break;
//        }
//    }
//    if (to_find != 0) {
//        qCritical("Some of the channel information was missing.");
//        goto bad;
//    }
//    if (t == Q_NULLPTR && meg_head_t != Q_NULLPTR) {
//    /*
//     * Try again in a more general fashion
//     */
//        if ((t = FiffCoordTransOld::mne_read_meas_transform(name)) == Q_NULLPTR) {
//            qCritical("MEG -> head coordinate transformation not found.");
//            goto bad;
//        }
//    }
//   /*
//    * Sort out the channels
//    */
//    for (k = 0; k < nchan; k++) {
//        if (chs[k].kind == FIFFV_MEG_CH) {
//            meg.append(chs[k]);
//            nmeg++;
//        } else if (chs[k].kind == FIFFV_REF_MEG_CH) {
//            meg_comp.append(chs[k]);
//            nmeg_comp++;
//        } else if (chs[k].kind == FIFFV_EEG_CH) {
//            eeg.append(chs[k]);
//            neeg++;
//        }
//    }
//    //    fiff_close(in);

//    stream->close();

//    megp  = meg;
//    if(nmegp) {
//        *nmegp = nmeg;
//    }

//    meg_compp = meg_comp;
//    if(nmeg_compp) {
//        *nmeg_compp = nmeg_comp;
//    }

//    eegp = eeg;
//    if(neegp) {
//        *neegp = neeg;
//    }

//    if (idp == Q_NULLPTR) {
//        FREE_41(id);
//    } else {
//        *idp   = id;
//    }

//    if (meg_head_t == Q_NULLPTR) {
//        FREE_41(t);
//    } else {
//        *meg_head_t = t;
//    }

//    return FIFF_OK;

//bad : {
//        //        fiff_close(in);
//        stream->close();
//        FREE_41(id);
//        //        FREE_41(tag.data);
//        FREE_41(t);
//        return FIFF_FAIL;
//    }
//}

int ComputeFwd::mne_read_meg_comp_eeg_ch_info_41(FIFFLIB::FiffInfoBase::SPtr pFiffInfoBase,
                                                 QList<FiffChInfo>& listMegCh,
                                                 int& iNMeg,
                                                 QList<FiffChInfo>& listMegComp,
                                                 int& iNMegCmp,
                                                 QList<FiffChInfo>& listEegCh,
                                                 int &iNEeg,
                                                 FiffCoordTransOld** transDevHeadOld,
                                                 FiffId& id)
{
    int iNumCh = pFiffInfoBase->nchan;
    for (int k = 0; k < iNumCh; k++) {
        if (pFiffInfoBase->chs[k].kind == FIFFV_MEG_CH) {
            listMegCh.append(pFiffInfoBase->chs[k]);
            iNMeg++;
        } else if (pFiffInfoBase->chs[k].kind == FIFFV_REF_MEG_CH) {
            listMegComp.append(pFiffInfoBase->chs[k]);
            iNMegCmp++;
        } else if (pFiffInfoBase->chs[k].kind == FIFFV_EEG_CH) {
            listEegCh.append(pFiffInfoBase->chs[k]);
            iNEeg++;
        }
    }

    if(!m_pSettings->meg_head_t) {
        *transDevHeadOld = new FiffCoordTransOld(pFiffInfoBase->dev_head_t.toOld());
    } else {
        *transDevHeadOld = m_pSettings->meg_head_t;
    }
    if(!m_meg_head_t) {
        qCritical("MEG -> head coordinate transformation not found.");
        return FIFF_FAIL;
    }
    id = pFiffInfoBase->meas_id;
    return OK;
}

int mne_check_chinfo(const QList<FiffChInfo>& chs,
                     int nch)
/*
 * Check that all EEG channels have reasonable locations
 */
{
    int k;
    FiffChInfo ch;
    float close = 0.02f;

    for (k = 0; k < nch; k++) {
        if (chs.at(k).kind == FIFFV_EEG_CH) {
            if (chs.at(k).chpos.r0.norm() < close) {
                qCritical("Some EEG channels do not have locations assigned.");
                return FAIL;
            }
        }
    }

    return OK;
}

//=============================================================================================================
// Temporary Helpers
//=============================================================================================================

void write_id_old(FiffStream::SPtr& t_pStream, fiff_int_t kind, fiffId id)
{
    fiffId t_id = id;
    if(t_id->version == -1)
    {
        /* initialize random seed: */
        srand ( time(Q_NULLPTR) );
        double rand_1 = (double)(rand() % 100);rand_1 /= 100;
        double rand_2 = (double)(rand() % 100);rand_2 /= 100;

        time_t seconds;
        seconds = time (Q_NULLPTR);

        //fiff_int_t timezone = 5;      //   Matlab does not know the timezone
        t_id->version   = (1 << 16) | 2;   //   Version (1 << 16) | 2
        t_id->machid[0] = 65536*rand_1;    //   Machine id is random for now
        t_id->machid[1] = 65536*rand_2;    //   Machine id is random for now
        t_id->time.secs = (int)seconds;    //seconds since January 1, 1970 //3600*(24*(now-datenum(1970,1,1,0,0,0))+timezone);
        t_id->time.usecs = 0;              //   Do not know how we could get this
    }

    //
    //
    fiff_int_t datasize = 5*4;                       //   The id comprises five integers

     *t_pStream << (qint32)kind;
     *t_pStream << (qint32)FIFFT_ID_STRUCT;
     *t_pStream << (qint32)datasize;
     *t_pStream << (qint32)FIFFV_NEXT_SEQ;
    //
    // Collect the bits together for one write
    //
    qint32 data[5];
    data[0] = t_id->version;
    data[1] = t_id->machid[0];
    data[2] = t_id->machid[1];
    data[3] = t_id->time.secs;
    data[4] = t_id->time.usecs;

    for(qint32 i = 0; i < 5; ++i)
        *t_pStream << data[i];
}

//=============================================================================================================

void write_coord_trans_old(FiffStream::SPtr& t_pStream, const FiffCoordTransOld* trans)
{
    //?typedef struct _fiffCoordTransRec {
    //  fiff_int_t   from;                   /*!< Source coordinate system. */
    //  fiff_int_t   to;                     /*!< Destination coordinate system. */
    //  fiff_float_t rot[3][3];              /*!< The forward transform (rotation part) */
    //  fiff_float_t move[3];                /*!< The forward transform (translation part) */
    //  fiff_float_t invrot[3][3];           /*!< The inverse transform (rotation part) */
    //  fiff_float_t invmove[3];             /*!< The inverse transform (translation part) */
    //} *fiffCoordTrans, fiffCoordTransRec;  /*!< Coordinate transformation descriptor */
    fiff_int_t datasize = 4*2*12 + 4*2;

     *t_pStream << (qint32)FIFF_COORD_TRANS;
     *t_pStream << (qint32)FIFFT_COORD_TRANS_STRUCT;
     *t_pStream << (qint32)datasize;
     *t_pStream << (qint32)FIFFV_NEXT_SEQ;

    //
    //   Start writing fiffCoordTransRec
    //
     *t_pStream << (qint32)trans->from;
     *t_pStream << (qint32)trans->to;

    //
    //   The transform...
    //
    qint32 r, c;
    for (r = 0; r < 3; ++r)
        for (c = 0; c < 3; ++c)
            *t_pStream << (float)trans->rot(r,c);
    for (r = 0; r < 3; ++r)
        *t_pStream << (float)trans->move[r];

    //
    //   ...and its inverse
    //
    for (r = 0; r < 3; ++r)
        for (c = 0; c < 3; ++c)
            *t_pStream << (float)trans->invrot(r,c);
    for (r = 0; r < 3; ++r)
        *t_pStream << (float)trans->invmove[r];
}

static int **make_file_triangle_list_41(int **tris, int ntri)
/*
      * In the file the numbering starts from one
      */
{
    int **res = ALLOC_ICMATRIX_41(ntri,3);
    int j,k;

    for (j = 0; j < ntri; j++)
        for (k = 0; k < 3; k++)
            res[j][k] = tris[j][k]+1;
    return res;
}

void mne_write_bad_channel_list_new(FiffStream::SPtr& t_pStream, const QStringList& t_badList)//FILE *out, char **list, int nlist)
{

    t_pStream->start_block(FIFFB_MNE_BAD_CHANNELS);
    t_pStream->write_name_list(FIFF_MNE_CH_NAME_LIST,t_badList);
    t_pStream->end_block(FIFFB_MNE_BAD_CHANNELS);

    /////////////////////////////////////////////////////////

    //    fiff_int_t  bad_channel_block = FIFFB_MNE_BAD_CHANNELS;
    //    fiffTagRec  bad_channel_block_tags[] = {
    //        { FIFF_BLOCK_START,      FIFFT_INT,    0, FIFFV_NEXT_SEQ, Q_NULLPTR },
    //        { FIFF_MNE_CH_NAME_LIST, FIFFT_STRING, 0, FIFFV_NEXT_SEQ, Q_NULLPTR },
    //        { FIFF_BLOCK_END,        FIFFT_INT,    0, FIFFV_NEXT_SEQ, Q_NULLPTR }};
    //    int         nbad_channel_block_tags = 3;
    //    char        *names = Q_NULLPTR;
    //    int         k;

    //    if (nlist <= 0)
    //        return OK;

    //    names = mne_name_list_to_string(list,nlist);
    //    bad_channel_block_tags[0].size = sizeof(fiff_int_t);
    //    bad_channel_block_tags[0].data = &bad_channel_block;

    //    bad_channel_block_tags[1].size = strlen(names);
    //    bad_channel_block_tags[1].data = names;

    //    bad_channel_block_tags[2].size = sizeof(fiff_int_t);
    //    bad_channel_block_tags[2].data = &bad_channel_block;

    //    for (k = 0; k < nbad_channel_block_tags; k++)
    //        if (fiff_write_tag(out,bad_channel_block_tags+k) == FIFF_FAIL) {
    //            FREE(names);
    //            return FAIL;
    //        }
    //    FREE(names);
    //    return OK;
}

void fiff_write_float_matrix_old (  FiffStream::SPtr& t_pStream,    /* Destination file name */
                                    int          kind,              /* What kind of tag */
                                    fiff_float_t **data,            /* The data to write */
                                    int          rows,
                                    int          cols)              /* Number of rows and columns */
/*
 * Write out a 2D floating-point matrix
 */
{
    MatrixXf mat(rows,cols);

    for (int i = 0; i < rows; ++i) {
        for(int j = 0; j < cols; ++j) {
            mat(i,j) = data[i][j];
        }
    }

    t_pStream->write_float_matrix(kind, mat);

    //  int res,*dims;
    //  fiffTagRec tag;
    //#ifdef INTEL_X86_ARCH
    //  int c;
    //#endif
    //  int k;
    //  int rowsize;

    //  tag.kind = kind;
    //  tag.type = FIFFT_FLOAT | FIFFT_MATRIX;
    //  tag.size = rows*cols*sizeof(fiff_float_t) + 3*sizeof(fiff_int_t);
    //  tag.data = Q_NULLPTR;
    //  tag.next = FIFFV_NEXT_SEQ;

    //  if ((res = fiff_write_tag_info(out,&tag)) == -1)
    //    return FIFF_FAIL;

    //  rowsize = cols*sizeof(fiff_float_t);
    //  for (k = 0; k < rows; k++) {
    //#ifdef INTEL_X86_ARCH
    //    for (c = 0; c < cols; c++)
    //      swap_float(data[k]+c);
    //#endif
    //    if (fwrite (data[k],rowsize,1,out) != 1) {
    //      if (ferror(out))
    //    err_set_sys_error("fwrite");
    //      else
    //    err_set_error("fwrite failed");
    //#ifdef INTEL_X86_ARCH
    //      for (c = 0; c < cols; c++)
    //    swap_float(data[k]+c);
    //#endif
    //      return FIFF_FAIL;
    //    }
    //#ifdef INTEL_X86_ARCH
    //    for (c = 0; c < cols; c++)
    //      swap_float(data[k]+c);
    //#endif
    //  }
    //  dims = MALLOC_41(3,fiff_int_t);
    //  dims[0] = swap_int(cols);
    //  dims[1] = swap_int(rows);
    //  dims[2] = swap_int(2);
    //  if (fwrite (dims,3*sizeof(fiff_int_t),1,out) != 1) {
    //    if (ferror(out))
    //      err_set_sys_error("fwrite");
    //    else
    //      err_set_error("fwrite failed");
    //    FREE(dims);
    //    return FIFF_FAIL;
    //  }
    //  FREE(dims);
    //  return res;
}

void fiff_write_int_matrix_old (    FiffStream::SPtr& t_pStream,
                                    int        kind,        /* What kind of tag */
                                    fiff_int_t **data,      /* The data to write */
                                    int        rows,
                                    int        cols)        /* Number of rows and columns */
/*
      * Write out a 2D integer matrix
      */
{
    MatrixXi mat(rows,cols);

    for (int i = 0; i < rows; ++i) {
        for(int j = 0; j < cols; ++j) {
            mat(i,j) = data[i][j];
        }
    }

    t_pStream->write_int_matrix(kind, mat);

    //  int res,*dims;
    //  fiffTagRec tag;
    //#ifdef INTEL_X86_ARCH
    //  int c;
    //#endif
    //  int k;
    //  int rowsize;

    //  tag.kind = kind;
    //  tag.type = FIFFT_INT | FIFFT_MATRIX;
    //  tag.size = rows*cols*sizeof(fiff_int_t) + 3*sizeof(fiff_int_t);
    //  tag.data = Q_NULLPTR;
    //  tag.next = FIFFV_NEXT_SEQ;

    //  if ((res = fiff_write_tag_info(out,&tag)) == -1)
    //    return -1;

    //  rowsize = cols*sizeof(fiff_int_t);
    //  for (k = 0; k < rows; k++) {
    //#ifdef INTEL_X86_ARCH
    //    for (c = 0; c < cols; c++)
    //      data[k][c] = swap_int(data[k][c]);
    //#endif
    //    if (fwrite (data[k],rowsize,1,out) != 1) {
    //      if (ferror(out))
    //    err_set_sys_error("fwrite");
    //      else
    //    err_set_error("fwrite failed");
    //      return -1;
    //#ifdef INTEL_X86_ARCH
    //      for (c = 0; c < cols; c++)
    //    data[k][c] = swap_int(data[k][c]);
    //#endif
    //    }
    //#ifdef INTEL_X86_ARCH
    //    for (c = 0; c < cols; c++)
    //      data[k][c] = swap_int(data[k][c]);
    //#endif
    //  }
    //  dims    = MALLOC_41(3,fiff_int_t);
    //  dims[0] = swap_int(cols);
    //  dims[1] = swap_int(rows);
    //  dims[2] = swap_int(2);
    //  if (fwrite (dims,3*sizeof(fiff_int_t),1,out) != 1) {
    //    if (ferror(out))
    //      err_set_sys_error("fwrite");
    //    else
    //      err_set_error("fwrite failed");
    //    FREE(dims);
    //    return -1;
    //  }
    //  FREE(dims);
    //  return res;
}

int fiff_write_float_sparse_matrix_old(FiffStream::SPtr& t_pStream, int kind, FiffSparseMatrix* mat)
/*
 * Write a sparse matrix
 */
{
    FiffTag::SPtr tag;
    int     k;
    int     type;
    int     datasize,idxsize,ptrsize;
    int     two = 2;
    int     res;
    int     val;

    datasize = mat->nz * sizeof(fiff_float_t);
    idxsize  = mat->nz * sizeof(fiff_int_t);
    if (mat->coding == FIFFTS_MC_CCS)
        ptrsize  = (mat->n+1) * sizeof(fiff_int_t);
    else if (mat->coding == FIFFTS_MC_RCS)
        ptrsize  = (mat->m+1) * sizeof(fiff_int_t);
    else {
        qCritical("Incomprehensible sparse matrix coding");
        return FIFF_FAIL;
    }

    if (datasize <= 0 || idxsize <= 0 || ptrsize <= 0) {
        qCritical("fiff_write_float_ccs_matrix: negative vector size(s) in sparse matrix!\n");
        return FIFF_FAIL;
    }

//    tag.kind = kind;
    if (mat->coding == FIFFTS_MC_CCS)
        type = FIFFT_FLOAT | FIFFT_CCS_MATRIX;//tag.type = FIFFT_FLOAT | FIFFT_CCS_MATRIX;
    else if (mat->coding == FIFFTS_MC_RCS)
        type = FIFFT_FLOAT | FIFFT_RCS_MATRIX;//tag.type = FIFFT_FLOAT | FIFFT_RCS_MATRIX;
    else {
        qCritical("Incomprehensible sparse matrix coding");
        return FIFF_FAIL;
    }

//    tag.size = datasize+idxsize+ptrsize+4*sizeof(fiff_int_t);
//    tag.data = Q_NULLPTR;
//    tag.next = FIFFV_NEXT_SEQ;

    //Write Tag Info
     *t_pStream << (qint32)kind;
     *t_pStream << (qint32)type;
     *t_pStream << (qint32)(datasize+idxsize+ptrsize+4*sizeof(fiff_int_t));
     *t_pStream << (qint32)FIFFV_NEXT_SEQ;
//    if (fiff_write_tag_info(out,&tag) == FIFF_FAIL)
//        return FIFF_FAIL;

    /*
     * Write data
     */
    for(k = 0; k < mat->nz; ++k)
        *t_pStream << mat->data[k];

//    /*
//    * Write data with swapping
//    */
//#ifdef INTEL_X86_ARCH
//    for (k = 0; k < mat->nz; k++)
//        swap_floatp(mat->data+k);
//#endif
//    res = fwrite (mat->data,datasize,1,out);
//#ifdef INTEL_X86_ARCH
//    for (k = 0; k < mat->nz; k++)
//        swap_floatp(mat->data+k);
//#endif
//    if (res != 1)
//        goto fwrite_error;

    /*
     * Write indexes
     */
    for(k = 0; k < mat->nz; ++k)
        *t_pStream << mat->inds[k];

//    /*
//    * Write indexes with swapping
//    */
//#ifdef INTEL_X86_ARCH
//    for (k = 0; k < mat->nz; k++)
//        swap_intp(mat->inds+k);
//#endif
//    res = fwrite (mat->inds,idxsize,1,out);
//#ifdef INTEL_X86_ARCH
//    for (k = 0; k < mat->nz; k++)
//        swap_intp(mat->inds+k);
//#endif
//    if (res != 1)
//        goto fwrite_error;

    if (mat->coding == FIFFTS_MC_CCS) {

        for(k = 0; k < mat->n+1; ++k)
            *t_pStream << mat->ptrs[k];

//#ifdef INTEL_X86_ARCH
//        for (k = 0; k < mat->n+1; k++)
//            swap_intp(mat->ptrs+k);
//#endif
//        res = fwrite (mat->ptrs,ptrsize,1,out);
//#ifdef INTEL_X86_ARCH
//        for (k = 0; k < mat->n+1; k++)
//            swap_intp(mat->ptrs+k);
//#endif
//        if (res != 1)
//            goto fwrite_error;
    }
    else {      /* Row-compressed format */

        for(k = 0; k < mat->m+1; ++k)
            *t_pStream << mat->ptrs[k];

//#ifdef INTEL_X86_ARCH
//        for (k = 0; k < mat->m+1; k++)
//            swap_intp(mat->ptrs+k);
//#endif
//        res = fwrite (mat->ptrs,ptrsize,1,out);
//#ifdef INTEL_X86_ARCH
//        for (k = 0; k < mat->m+1; k++)
//            swap_intp(mat->ptrs+k);
//#endif
//        if (res != 1)
//            goto fwrite_error;
    }
    /*
     * Write the dimensions
     */
     *t_pStream << (qint32)mat->nz;
//    val = swap_int(mat->nz);
//    if (fwrite (&val,sizeof(fiff_int_t),1,out) != 1)
//        goto fwrite_error;

     *t_pStream << (qint32)mat->m;
//    val = swap_int(mat->m);
//    if (fwrite (&val,sizeof(fiff_int_t),1,out) != 1)
//        goto fwrite_error;

     *t_pStream << (qint32)mat->n;
//    val = swap_int(mat->n);
//    if (fwrite (&val,sizeof(fiff_int_t),1,out) != 1)
//        goto fwrite_error;

     *t_pStream << (qint32)two;
//    val = swap_int(two);
//    if (fwrite (&val,sizeof(fiff_int_t),1,out) != 1)
//        goto fwrite_error;
    return FIFF_OK;

//fwrite_error : {
////        if (ferror(out))
////            qCritical("fwrite");
////        else
////            err_set_error("fwrite failed");
//        return FIFF_FAIL;
//    }
}

static int comp_points2(const void *vp1,const void *vp2)

{
    MneNearest* v1 = (MneNearest*)vp1;
    MneNearest* v2 = (MneNearest*)vp2;

    if (v1->vert > v2->vert)
        return 1;
    else if (v1->vert == v2->vert)
        return 0;
    else
        return -1;
}

void mne_sort_nearest_by_vertex(MneNearest* points, int npoint)

{
    if (npoint > 1 && points != Q_NULLPTR)
        qsort(points,npoint,sizeof(MneNearest),comp_points2);
    return;
}

FiffSparseMatrix* mne_create_sparse_rcs(int nrow,           /* Number of rows */
                                        int ncol,           /* Number of columns */
                                        int *nnz,           /* Number of non-zero elements on each row */
                                        int **colindex,     /* Column indices of non-zero elements on each row */
                                        float **vals)       /* The nonzero elements on each row
                                                     * If Q_NULLPTR, the matrix will be all zeroes */

{
    FiffSparseMatrix* sparse = Q_NULLPTR;
    int j,k,nz,ptr,size,ind;
    int stor_type = FIFFTS_MC_RCS;

    for (j = 0, nz = 0; j < nrow; j++)
        nz = nz + nnz[j];

    if (nz <= 0) {
        qCritical("No nonzero elements specified.");
        return Q_NULLPTR;
    }
    if (stor_type == FIFFTS_MC_RCS) {
        size = nz*(sizeof(fiff_float_t) + sizeof(fiff_int_t)) +
                (nrow+1)*(sizeof(fiff_int_t));
    }
    else {
        qCritical("Illegal sparse matrix storage type: %d",stor_type);
        return Q_NULLPTR;
    }
    sparse = new FiffSparseMatrix;
    sparse->coding = stor_type;
    sparse->m      = nrow;
    sparse->n      = ncol;
    sparse->nz     = nz;
    sparse->data   = (float *)malloc(size);
    sparse->inds   = (int *)(sparse->data+nz);
    sparse->ptrs   = sparse->inds+nz;

    for (j = 0, nz = 0; j < nrow; j++) {
        ptr = -1;
        for (k = 0; k < nnz[j]; k++) {
            if (ptr < 0)
                ptr = nz;
            ind = sparse->inds[nz] = colindex[j][k];
            if (ind < 0 || ind >= ncol) {
                qCritical("Column index out of range in mne_create_sparse_rcs");
                goto bad;
            }
            if (vals)
                sparse->data[nz] = vals[j][k];
            else
                sparse->data[nz] = 0.0;
            nz++;
        }
        sparse->ptrs[j] = ptr;
    }
    sparse->ptrs[nrow] = nz;
    for (j = nrow-1; j >= 0; j--) /* Take care of the empty rows */
        if (sparse->ptrs[j] < 0)
            sparse->ptrs[j] = sparse->ptrs[j+1];
    return sparse;

bad : {
        if(sparse)
            delete sparse;
        return Q_NULLPTR;
    }
}

FiffSparseMatrix* mne_pick_lower_triangle_rcs(FiffSparseMatrix* mat)
/*
 * Fill in upper triangle with the lower triangle values
 */
{
    int             *nnz       = Q_NULLPTR;
    int             **colindex = Q_NULLPTR;
    float           **vals     = Q_NULLPTR;
    FiffSparseMatrix* res = Q_NULLPTR;
    int             i,j,k;

    if (mat->coding != FIFFTS_MC_RCS) {
        qCritical("The input matrix to mne_add_upper_triangle_rcs must be in RCS format");
        goto out;
    }
    if (mat->m != mat->n) {
        qCritical("The input matrix to mne_pick_lower_triangle_rcs must be square");
        goto out;
    }
    /*
   * Pick the lower triangle elements
   */
    nnz      = MALLOC_41(mat->m,int);
    colindex = MALLOC_41(mat->m,int *);
    vals     = MALLOC_41(mat->m,float *);
    for (i = 0; i < mat->m; i++) {
        nnz[i]      = mat->ptrs[i+1] - mat->ptrs[i];
        if (nnz[i] > 0) {
            colindex[i] = MALLOC_41(nnz[i],int);
            vals[i]   = MALLOC_41(nnz[i],float);
            for (j = mat->ptrs[i], k = 0; j < mat->ptrs[i+1]; j++) {
                if (mat->inds[j] <= i) {
                    vals[i][k] = mat->data[j];
                    colindex[i][k] = mat->inds[j];
                    k++;
                }
            }
            nnz[i] = k;
        }
        else {
            colindex[i] = Q_NULLPTR;
            vals[i] = Q_NULLPTR;
        }
    }
    /*
   * Assemble the matrix
   */
    res = mne_create_sparse_rcs(mat->m,mat->n,nnz,colindex,vals);

out : {
        for (i = 0; i < mat->m; i++) {
            FREE_41(colindex[i]);
            FREE_41(vals[i]);
        }
        FREE_41(nnz);
        FREE_41(vals);
        FREE_41(colindex);
        return res;
    }
}

static int write_volume_space_info(FiffStream::SPtr& t_pStream, MneSourceSpaceOld* ss, int selected_only)
/*
 * Write the vertex neighbors and other information for a volume source space
 */
{
    int ntot,nvert;
    int *nneighbors = Q_NULLPTR;
    int *neighbors = Q_NULLPTR;
    int *inuse_map = Q_NULLPTR;
    int nneigh,*neigh;
    int k,p;
    fiffTagRec tag;
    int res = FAIL;

    if (ss->type != FIFFV_MNE_SPACE_VOLUME)
        return OK;
    if (!ss->neighbor_vert || !ss->nneighbor_vert)
        return OK;
    if (selected_only) {
        inuse_map = MALLOC_41(ss->np,int);
        for (k = 0,p = 0, ntot = 0; k < ss->np; k++) {
            if (ss->inuse[k]) {
                ntot += ss->nneighbor_vert[k];
                inuse_map[k] = p++;
            }
            else
                inuse_map[k] = -1;
        }
        nneighbors = MALLOC_41(ss->nuse,int);
        neighbors = MALLOC_41(ntot,int);
        /*
        * Pick the neighbors and fix the vertex numbering to refer
        * to the vertices in use only
        */
        for (k = 0, nvert = 0, ntot = 0; k < ss->np; k++) {
            if (ss->inuse[k]) {
                neigh  = ss->neighbor_vert[k];
                nneigh = ss->nneighbor_vert[k];
                nneighbors[nvert++] = nneigh;
                for (p = 0; p < nneigh; p++)
                    neighbors[ntot++] = neigh[p] < 0 ? -1 : inuse_map[neigh[p]];
            }
        }
    }
    else {
        for (k = 0, ntot = 0; k < ss->np; k++)
            ntot += ss->nneighbor_vert[k];
        nneighbors = MALLOC_41(ss->np,int);
        neighbors = MALLOC_41(ntot,int);
        nvert     = ss->np;
        for (k = 0, ntot = 0; k < ss->np; k++) {
            neigh  = ss->neighbor_vert[k];
            nneigh = ss->nneighbor_vert[k];
            nneighbors[k] = nneigh;
            for (p = 0; p < nneigh; p++)
                neighbors[ntot++] = neigh[p];
        }
    }

    t_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NNEIGHBORS,nneighbors,nvert);
    //    tag.next = FIFFV_NEXT_SEQ;
    //    tag.kind = FIFF_MNE_SOURCE_SPACE_NNEIGHBORS;
    //    tag.type = FIFFT_INT;
    //    tag.size = nvert*sizeof(fiff_int_t);
    //    tag.data = (fiff_byte_t *)nneighbors;
    //    if (fiff_write_tag(out,&tag) == FIFF_FAIL)
    //        goto out;

    t_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NEIGHBORS,neighbors,ntot);
    //    tag.next = FIFFV_NEXT_SEQ;
    //    tag.kind = FIFF_MNE_SOURCE_SPACE_NEIGHBORS;
    //    tag.type = FIFFT_INT;
    //    tag.size = ntot*sizeof(fiff_int_t);
    //    tag.data = (fiff_byte_t *)neighbors;
    //    if (fiff_write_tag(out,&tag) == FIFF_FAIL)
    //        goto out;

    /*
     * Write some additional stuff
     */
    if (!selected_only) {
        if (ss->voxel_surf_RAS_t) {
            write_coord_trans_old(t_pStream, ss->voxel_surf_RAS_t);//t_pStream->write_coord_trans(ss->voxel_surf_RAS_t);

            t_pStream->write_int(FIFF_MNE_SOURCE_SPACE_VOXEL_DIMS,ss->vol_dims,3);
            //            tag.next = FIFFV_NEXT_SEQ;
            //            tag.kind = FIFF_MNE_SOURCE_SPACE_VOXEL_DIMS;
            //            tag.type = FIFFT_INT;
            //            tag.size = 3*sizeof(fiff_int_t);
            //            tag.data = (fiff_byte_t *)ss->vol_dims;
            //            if (fiff_write_tag(out,&tag) == FIFF_FAIL)
            //                goto out;
        }
        if (ss->interpolator && !ss->MRI_volume.isEmpty()) {
            t_pStream->start_block(FIFFB_MNE_PARENT_MRI_FILE);
            if (ss->MRI_surf_RAS_RAS_t)
                write_coord_trans_old(t_pStream, ss->MRI_surf_RAS_RAS_t);//t_pStream->write_coord_trans(ss->MRI_surf_RAS_RAS_t);
            if (ss->MRI_voxel_surf_RAS_t)
                write_coord_trans_old(t_pStream, ss->MRI_voxel_surf_RAS_t);//t_pStream->write_coord_trans(ss->MRI_voxel_surf_RAS_t);
            t_pStream->write_string(FIFF_MNE_FILE_NAME,ss->MRI_volume);
            if (ss->interpolator)
                fiff_write_float_sparse_matrix_old(t_pStream,FIFF_MNE_SOURCE_SPACE_INTERPOLATOR,ss->interpolator);
            if (ss->MRI_vol_dims[0] > 0 && ss->MRI_vol_dims[1] > 0 && ss->MRI_vol_dims[2] > 0) {
                t_pStream->write_int(FIFF_MRI_WIDTH,&ss->MRI_vol_dims[0]);
                t_pStream->write_int(FIFF_MRI_HEIGHT,&ss->MRI_vol_dims[1]);
                t_pStream->write_int(FIFF_MRI_DEPTH,&ss->MRI_vol_dims[2]);
            }
            t_pStream->end_block(FIFFB_MNE_PARENT_MRI_FILE);
        }
    }
    else {
        if (ss->interpolator && !ss->MRI_volume.isEmpty()) {
            t_pStream->write_string(FIFF_MNE_SOURCE_SPACE_MRI_FILE,ss->MRI_volume);
            qCritical("Cannot write the interpolator for selection yet");
            goto out;
        }
    }
    res = OK;
    goto out;

out : {
        FREE_41(inuse_map);
        FREE_41(nneighbors);
        FREE_41(neighbors);
        return res;
    }
}

int mne_write_one_source_space(FiffStream::SPtr& t_pStream, MneSourceSpaceOld* ss,bool selected_only)
{
    float **sel = Q_NULLPTR;
    int   **tris = Q_NULLPTR;
    int   *nearest = Q_NULLPTR;
    float *nearest_dist = Q_NULLPTR;
    int   p,pp;

    if (ss->np <= 0) {
        qCritical("No points in the source space being saved");
        goto bad;
    }

    t_pStream->start_block(FIFFB_MNE_SOURCE_SPACE);

    /*
     * General information
     */
    if (ss->type != FIFFV_MNE_SPACE_UNKNOWN)
        t_pStream->write_int(FIFF_MNE_SOURCE_SPACE_TYPE,&ss->type);
    if (ss->id != FIFFV_MNE_SURF_UNKNOWN)
        t_pStream->write_int(FIFF_MNE_SOURCE_SPACE_ID,&ss->id);
    if (!ss->subject.isEmpty() && ss->subject.size() > 0) {
        QString subj(ss->subject);
        t_pStream->write_string(FIFF_SUBJ_HIS_ID,subj);
    }

    t_pStream->write_int(FIFF_MNE_COORD_FRAME,&ss->coord_frame);

    if (selected_only) {
        if (ss->nuse == 0) {
            qCritical("No vertices in use. Cannot write active-only vertices from this source space");
            goto bad;
        }
        sel = ALLOC_CMATRIX_41(ss->nuse,3);
        t_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NPOINTS,&ss->nuse);
        for (p = 0, pp = 0; p < ss->np; p++) {
            if (ss->inuse[p]) {
                sel[pp][X_41] = ss->rr[p][X_41];
                sel[pp][Y_41] = ss->rr[p][Y_41];
                sel[pp][Z_41] = ss->rr[p][Z_41];
                pp++;
            }
        }
        fiff_write_float_matrix_old(t_pStream, FIFF_MNE_SOURCE_SPACE_POINTS, sel,ss->nuse,3);

        for (p = 0, pp = 0; p < ss->np; p++) {
            if (ss->inuse[p]) {
                sel[pp][X_41] = ss->nn[p][X_41];
                sel[pp][Y_41] = ss->nn[p][Y_41];
                sel[pp][Z_41] = ss->nn[p][Z_41];
                pp++;
            }
        }
        fiff_write_float_matrix_old(t_pStream, FIFF_MNE_SOURCE_SPACE_NORMALS, sel,ss->nuse,3);
        FREE_CMATRIX_41(sel); sel = Q_NULLPTR;
#ifdef WRONG
        /*
     * This code is incorrect because the numbering in the nuse triangulation refers to the complete source space
     */
        if (ss->nuse_tri > 0) {		/* Write the triangulation information */
            /*
       * The 'use' triangulation is identical to the complete one
       */
            if (fiff_write_int_tag(out,FIFF_MNE_SOURCE_SPACE_NTRI,ss->nuse_tri) == FIFF_FAIL)
                goto bad;
            tris = make_file_triangle_list(ss->use_itris,ss->nuse_tri);
            if (fiff_write_int_matrix(out,FIFF_MNE_SOURCE_SPACE_TRIANGLES,tris,
                                      ss->nuse_tri,3) == FIFF_FAIL)
                goto bad;

            if (fiff_write_int_tag(out,FIFF_MNE_SOURCE_SPACE_NUSE_TRI,ss->nuse_tri) == FIFF_FAIL)
                goto bad;
            if (fiff_write_int_matrix(out,FIFF_MNE_SOURCE_SPACE_USE_TRIANGLES,tris,
                                      ss->nuse_tri,3) == FIFF_FAIL)
                goto bad;
            FREE_ICMATRIX(tris); tris = Q_NULLPTR;
        }
#endif
    }
    else {
        //        fiffTagRec tag;
        t_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NPOINTS,&ss->np);

        fiff_write_float_matrix_old(t_pStream, FIFF_MNE_SOURCE_SPACE_POINTS, ss->rr, ss->np, 3);

        fiff_write_float_matrix_old(t_pStream, FIFF_MNE_SOURCE_SPACE_NORMALS, ss->nn, ss->np, 3);

        if (ss->nuse > 0 && ss->inuse) {
            t_pStream->write_int(FIFF_MNE_SOURCE_SPACE_SELECTION,ss->inuse,ss->np);
            //            tag.next = 0;
            //            tag.kind = FIFF_MNE_SOURCE_SPACE_SELECTION;
            //            tag.type = FIFFT_INT;
            //            tag.size = (ss->np)*sizeof(fiff_int_t);
            //            tag.data = (fiff_byte_t *)(ss->inuse);
            //            if (fiff_write_tag(out,&tag) == FIFF_FAIL)
            //                goto bad;
            t_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NUSE,&ss->nuse);
            //            if (fiff_write_int_tag (out, FIFF_MNE_SOURCE_SPACE_NUSE,ss->nuse) == FIFF_FAIL)
            //                goto bad;
        }
        if (ss->ntri > 0) { /* Write the triangulation information */
            t_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NTRI,&ss->ntri);
            //            if (fiff_write_int_tag(out,FIFF_MNE_SOURCE_SPACE_NTRI,ss->ntri) == FIFF_FAIL)
            //                goto bad;
            tris = make_file_triangle_list_41(ss->itris,ss->ntri);

            fiff_write_int_matrix_old(t_pStream, FIFF_MNE_SOURCE_SPACE_TRIANGLES, tris, ss->ntri, 3);
            //            if (fiff_write_int_matrix(out,FIFF_MNE_SOURCE_SPACE_TRIANGLES,tris,
            //                                      ss->ntri,3) == FIFF_FAIL)
            //                goto bad;
            FREE_ICMATRIX_41(tris); tris = Q_NULLPTR;
        }
        if (ss->nuse_tri > 0) { /* Write the triangulation information for the vertices in use */
            t_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NUSE_TRI,&ss->nuse_tri);
            //            if (fiff_write_int_tag(out,FIFF_MNE_SOURCE_SPACE_NUSE_TRI,ss->nuse_tri) == FIFF_FAIL)
            //                goto bad;
            tris = make_file_triangle_list_41(ss->use_itris,ss->nuse_tri);

            fiff_write_int_matrix_old(t_pStream, FIFF_MNE_SOURCE_SPACE_USE_TRIANGLES, tris, ss->nuse_tri, 3);
            //            if (fiff_write_int_matrix(out,FIFF_MNE_SOURCE_SPACE_USE_TRIANGLES,tris,
            //                                      ss->nuse_tri,3) == FIFF_FAIL)
            //                goto bad;
            FREE_ICMATRIX_41(tris); tris = Q_NULLPTR;
        }
        if (ss->nearest) {    /* Write the patch information */
            nearest = MALLOC_41(ss->np,int);
            nearest_dist = MALLOC_41(ss->np,float);

            mne_sort_nearest_by_vertex(ss->nearest,ss->np);
            for (p = 0; p < ss->np; p++) {
                nearest[p] = ss->nearest[p].nearest;
                nearest_dist[p] = ss->nearest[p].dist;
            }

            t_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NEAREST,nearest,ss->np);
            //            tag.next = FIFFV_NEXT_SEQ;
            //            tag.kind = FIFF_MNE_SOURCE_SPACE_NEAREST;
            //            tag.type = FIFFT_INT;
            //            tag.size = (ss->np)*sizeof(fiff_int_t);
            //            tag.data = (fiff_byte_t *)(nearest);
            //            if (fiff_write_tag(out,&tag) == FIFF_FAIL)
            //                goto bad;

            t_pStream->write_float(FIFF_MNE_SOURCE_SPACE_NEAREST_DIST,nearest_dist,ss->np);
            //            tag.next = FIFFV_NEXT_SEQ;
            //            tag.kind = FIFF_MNE_SOURCE_SPACE_NEAREST_DIST;
            //            tag.type = FIFFT_FLOAT;
            //            tag.size = (ss->np)*sizeof(fiff_float_t);
            //            tag.data = (fiff_byte_t *)(nearest_dist);
            //            if (fiff_write_tag(out,&tag) == FIFF_FAIL)
            //                goto bad;

            FREE_41(nearest); nearest = Q_NULLPTR;
            FREE_41(nearest_dist); nearest_dist = Q_NULLPTR;
        }
        if (ss->dist) {     /* Distance information */
            FiffSparseMatrix* m = mne_pick_lower_triangle_rcs(ss->dist);
            if (!m)
                goto bad;
            if (fiff_write_float_sparse_matrix_old(t_pStream,FIFF_MNE_SOURCE_SPACE_DIST,m) == FIFF_FAIL) {
                if(m)
                    delete m;
                goto bad;
            }
            if(m)
                delete m;

            t_pStream->write_float(FIFF_MNE_SOURCE_SPACE_DIST_LIMIT,&ss->dist_limit);
        }
    }
    /*
     * Volume source spaces have additional information
     */
    //    if (write_volume_space_info(out,ss,selected_only) == FIFF_FAIL)
    //        goto bad;

    t_pStream->end_block(FIFFB_MNE_SOURCE_SPACE);
    return FIFF_OK;

bad : {
        FREE_ICMATRIX_41(tris);
        FREE_CMATRIX_41(sel);
        FREE_41(nearest);
        FREE_41(nearest_dist);
        return FIFF_FAIL;
    }
}

QString mne_name_list_to_string_41(const QStringList& list)
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

int mne_write_named_matrix( FiffStream::SPtr& t_pStream,
                            int  kind,
                            MneNamedMatrix* mat)
/*
 * Write a block which contains information about one named matrix
 */
{
    QString names;

    t_pStream->start_block(FIFFB_MNE_NAMED_MATRIX);

    t_pStream->write_int(FIFF_MNE_NROW,&mat->nrow);
    t_pStream->write_int(FIFF_MNE_NCOL,&mat->ncol);
    if (!mat->rowlist.isEmpty()) {
        names = mne_name_list_to_string_41(mat->rowlist);
        t_pStream->write_string(FIFF_MNE_ROW_NAMES,names);
    }
    if (!mat->collist.isEmpty()) {
        names = mne_name_list_to_string_41(mat->collist);
        t_pStream->write_string(FIFF_MNE_COL_NAMES,names);
    }
    fiff_write_float_matrix_old (t_pStream,kind,mat->data,mat->nrow,mat->ncol);

    t_pStream->end_block(FIFFB_MNE_NAMED_MATRIX);

    return FIFF_OK;

bad :
    return FIFF_FAIL;
}

bool fiff_put_dir (FiffStream::SPtr& t_pStream, const QList<FiffDirEntry::SPtr>& dir)
/*
 * Put in new directory
 */
{
    int nent = dir.size();
    int k;
    FiffTag::SPtr t_pTag;
    fiff_int_t dirpos;

    for (k = 0; k < nent; k++) {
        if (dir[k]->kind == FIFF_DIR_POINTER) {
            /*
            * Read current value of directory pointer
            */
            if (!t_pStream->read_tag(t_pTag,dir[k]->pos)) {
                fprintf (stderr,"Could not read FIFF_DIR_POINTER!\n");
                return false;
            }
            /*
            * If there is no directory, append the new one
            */
            dirpos = *t_pTag->toInt();//2GB restriction
            if (dirpos <= 0)
                dirpos = -1;
            /*
            * Put together the directory tag
            */
            dirpos = (fiff_int_t)t_pStream->write_dir_entries(dir);//2GB restriction
            if (dirpos < 0)
                printf ("Could not update directory!\n");
            else {
                t_pTag->setNum(dirpos);
//                t_pStream->write_tag(t_pTag,dir[k]->pos);

                t_pStream->write_dir_pointer(dirpos, dir[k]->pos);

//                t_pStream->device()->seek(dir[k]->pos);

//                fiff_int_t datasize = 1 * 4;

//                *t_pStream << (qint32)t_pTag->kind;
//                *t_pStream << (qint32)t_pTag->type;
//                *t_pStream << (qint32)datasize;
//                *t_pStream << (qint32)t_pTag->next;

//                *t_pStream << dirpos;

            }
            return true;
        }
    }
    printf ("Could not find place for directory!\n");
    return false;
}

//============================= write_solution.c =============================

bool write_solution(const QString& name,         /* Destination file */
                    MneSourceSpaceOld* *spaces,  /* The source spaces */
                    int nspace,
                    const QString& mri_file,     /* MRI file and data obtained from there */
                    fiffId mri_id,
                    FiffCoordTransOld* mri_head_t,
                    const QString& meas_file,    /* MEG file and data obtained from there */
                    FiffId meas_id,
                    FiffCoordTransOld* meg_head_t,
                    QList<FiffChInfo> meg_chs,
                    int nmeg,
                    QList<FiffChInfo> eeg_chs,
                    int neeg,
                    int fixed_ori,    /* Fixed orientation dipoles? */
                    int coord_frame,  /* Coordinate frame employed in the forward calculations */
                    FiffNamedMatrix& meg_solution,
                    FiffNamedMatrix& eeg_solution,
                    FiffNamedMatrix& meg_solution_grad,
                    FiffNamedMatrix& eeg_solution_grad,
                    bool bDoGrad)
{
    // New Stuff
    QFile file(name);

    QFile fileIn(name);
    FiffStream::SPtr t_pStreamIn;

    int nvert;
    int k;

    //
    //   Open the file, create directory
    //

    // Create the file and save the essentials
    FiffStream::SPtr t_pStream = FiffStream::start_file(file);

    t_pStream->start_block(FIFFB_MNE);

    /*
     * Information from the MRI file
     */
    {
        t_pStream->start_block(FIFFB_MNE_PARENT_MRI_FILE);

        t_pStream->write_string(FIFF_MNE_FILE_NAME, mri_file);
        if (mri_id != Q_NULLPTR)
            write_id_old(t_pStream, FIFF_PARENT_FILE_ID, mri_id);//t_pStream->write_id(FIFF_PARENT_FILE_ID, mri_id);
        write_coord_trans_old(t_pStream, mri_head_t);//t_pStream->write_coord_trans(mri_head_t);

        t_pStream->end_block(FIFFB_MNE_PARENT_MRI_FILE);
    }

    /*
     * Information from the MEG file
     */
    {
        QStringList file_bads;
        int  file_nbad   = 0;

        t_pStream->start_block(FIFFB_MNE_PARENT_MEAS_FILE);

        t_pStream->write_string(FIFF_MNE_FILE_NAME, meas_file);
        if (!meas_id.isEmpty()) {
            t_pStream->write_id(FIFF_PARENT_BLOCK_ID, meas_id);
        }
        write_coord_trans_old(t_pStream, meg_head_t);//t_pStream->write_coord_trans(meg_head_t);

        int nchan = nmeg+neeg;
        t_pStream->write_int(FIFF_NCHAN,&nchan);

        FiffChInfo chInfo;
        int k, p;
        for (k = 0, p = 0; k < nmeg; k++) {
//            meg_chs[k].scanNo = ++p;
//            chInfo.scanNo = meg_chs[k].scanNo;
//            chInfo.logNo = meg_chs[k].logNo;
//            chInfo.kind = meg_chs[k].kind;
//            chInfo.range = meg_chs[k].range;
//            chInfo.cal = meg_chs[k].cal;
//            chInfo.chpos.coil_type = meg_chs[k].chpos.coil_type;
//            chInfo.chpos.r0[0] = meg_chs[k].chpos.r0[0];
//            chInfo.chpos.r0[1] = meg_chs[k].chpos.r0[1];
//            chInfo.chpos.r0[2] = meg_chs[k].chpos.r0[2];
//            chInfo.chpos.ex[0] = meg_chs[k].chpos.ex[0];
//            chInfo.chpos.ex[1] = meg_chs[k].chpos.ex[1];
//            chInfo.chpos.ex[2] = meg_chs[k].chpos.ex[2];
//            chInfo.chpos.ey[0] = meg_chs[k].chpos.ey[0];
//            chInfo.chpos.ey[1] = meg_chs[k].chpos.ey[1];
//            chInfo.chpos.ey[2] = meg_chs[k].chpos.ey[2];
//            chInfo.chpos.ez[0] = meg_chs[k].chpos.ez[0];
//            chInfo.chpos.ez[1] = meg_chs[k].chpos.ez[1];
//            chInfo.chpos.ez[2] = meg_chs[k].chpos.ez[2];
//            chInfo.unit = meg_chs[k].unit;
//            chInfo.unit_mul = meg_chs[k].unit_mul;
//            chInfo.ch_name = QString(meg_chs[k].ch_name);

            t_pStream->write_ch_info(meg_chs[k]);
        }

        for (k = 0; k < neeg; k++) {
//            eeg_chs[k].scanNo = ++p;
//            chInfo.scanNo = eeg_chs[k].scanNo;
//            chInfo.logNo = eeg_chs[k].logNo;
//            chInfo.kind = eeg_chs[k].kind;
//            chInfo.range = eeg_chs[k].range;
//            chInfo.cal = eeg_chs[k].cal;
//            chInfo.chpos.coil_type = eeg_chs[k].chpos.coil_type;
//            chInfo.chpos.r0[0] = eeg_chs[k].chpos.r0[0];
//            chInfo.chpos.r0[1] = eeg_chs[k].chpos.r0[1];
//            chInfo.chpos.r0[2] = eeg_chs[k].chpos.r0[2];
//            chInfo.chpos.ex[0] = eeg_chs[k].chpos.ex[0];
//            chInfo.chpos.ex[1] = eeg_chs[k].chpos.ex[1];
//            chInfo.chpos.ex[2] = eeg_chs[k].chpos.ex[2];
//            chInfo.chpos.ey[0] = eeg_chs[k].chpos.ey[0];
//            chInfo.chpos.ey[1] = eeg_chs[k].chpos.ey[1];
//            chInfo.chpos.ey[2] = eeg_chs[k].chpos.ey[2];
//            chInfo.chpos.ez[0] = eeg_chs[k].chpos.ez[0];
//            chInfo.chpos.ez[1] = eeg_chs[k].chpos.ez[1];
//            chInfo.chpos.ez[2] = eeg_chs[k].chpos.ez[2];
//            chInfo.unit = eeg_chs[k].unit;
//            chInfo.unit_mul = eeg_chs[k].unit_mul;
//            chInfo.ch_name = QString(eeg_chs[k].ch_name);

            t_pStream->write_ch_info(eeg_chs[k]);
        }
        /*
        * Copy the bad channel list from the measurement file
        */

        //
        // mne_read_bad_channel_list replacement
        //
        QFile fileBad(meas_file);
        FiffStream::SPtr t_pStreamBads(new FiffStream(&fileBad));
        if(!t_pStreamBads->open())
            return false;
        file_bads  = t_pStreamBads->read_bad_channels(t_pStreamBads->dirtree());

        //
        // mne_write_bad_channel_list replacement
        //
        mne_write_bad_channel_list_new(t_pStream,file_bads);

        t_pStream->end_block(FIFFB_MNE_PARENT_MEAS_FILE);
    }

    /*
     * Write the source spaces (again)
     */
    for (k = 0, nvert = 0; k < nspace; k++) {
        if (mne_write_one_source_space(t_pStream,spaces[k],FALSE) == FIFF_FAIL)
            goto bad;
        nvert += spaces[k]->nuse;
    }

    /*
     * MEG forward solution
     */
    if (nmeg > 0) {
        t_pStream->start_block(FIFFB_MNE_FORWARD_SOLUTION);

        int val = FIFFV_MNE_MEG;
        t_pStream->write_int(FIFF_MNE_INCLUDED_METHODS,&val);
        t_pStream->write_int(FIFF_MNE_COORD_FRAME,&coord_frame);
        val = fixed_ori ? FIFFV_MNE_FIXED_ORI : FIFFV_MNE_FREE_ORI;
        t_pStream->write_int(FIFF_MNE_SOURCE_ORIENTATION,&val);
        t_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NPOINTS,&nvert);
        t_pStream->write_int(FIFF_NCHAN,&nmeg);

        meg_solution.transpose_named_matrix();
        t_pStream->write_named_matrix(FIFF_MNE_FORWARD_SOLUTION,meg_solution);
        meg_solution.transpose_named_matrix();
//        if (mne_write_named_matrix(t_pStream,FIFF_MNE_FORWARD_SOLUTION,meg_solution) == FIFF_FAIL)
//            goto bad;
        if (bDoGrad) {
            meg_solution_grad.transpose_named_matrix();
            t_pStream->write_named_matrix(FIFF_MNE_FORWARD_SOLUTION_GRAD,meg_solution_grad);
            meg_solution_grad.transpose_named_matrix();
        }

//            if (mne_write_named_matrix(t_pStream,FIFF_MNE_FORWARD_SOLUTION_GRAD,meg_solution_grad) == FIFF_FAIL)
//                goto bad;
        t_pStream->end_block(FIFFB_MNE_FORWARD_SOLUTION);
    }
    /*
     * EEG forward solution
     */
    if (neeg > 0) {
        t_pStream->start_block(FIFFB_MNE_FORWARD_SOLUTION);

        int val = FIFFV_MNE_EEG;
        t_pStream->write_int(FIFF_MNE_INCLUDED_METHODS,&val);
        t_pStream->write_int(FIFF_MNE_COORD_FRAME,&coord_frame);
        val = fixed_ori ? FIFFV_MNE_FIXED_ORI : FIFFV_MNE_FREE_ORI;
        t_pStream->write_int(FIFF_MNE_SOURCE_ORIENTATION,&val);
        t_pStream->write_int(FIFF_NCHAN,&neeg);
        t_pStream->write_int(FIFF_MNE_SOURCE_SPACE_NPOINTS,&nvert);

        eeg_solution.transpose_named_matrix();
        t_pStream->write_named_matrix(FIFF_MNE_FORWARD_SOLUTION,eeg_solution);
//        if (mne_write_named_matrix(t_pStream,FIFF_MNE_FORWARD_SOLUTION,eeg_solution) == FIFF_FAIL)
        eeg_solution.transpose_named_matrix();
//            goto bad;
        if (bDoGrad) {
            eeg_solution_grad.transpose_named_matrix();
            t_pStream->write_named_matrix(FIFF_MNE_FORWARD_SOLUTION_GRAD,eeg_solution_grad);
            eeg_solution_grad.transpose_named_matrix();
        }
//            if (mne_write_named_matrix(t_pStream,FIFF_MNE_FORWARD_SOLUTION_GRAD,eeg_solution_grad) == FIFF_FAIL)
//                goto bad;

        t_pStream->end_block(FIFFB_MNE_FORWARD_SOLUTION);
    }

    t_pStream->end_block(FIFFB_MNE);
    t_pStream->end_file();
    t_pStream->close();
    t_pStream.clear();

    /*
     * Add directory
     */
    t_pStreamIn = FiffStream::open_update(fileIn);

    if (!fiff_put_dir(t_pStreamIn,t_pStreamIn->dir()))
        goto bad;
    if(t_pStreamIn)
        t_pStreamIn->close();

    return true;

bad : {
        if(t_pStream)
            t_pStream->close();
        if(t_pStreamIn)
            t_pStreamIn->close();
        return false;
    }
}

/*
 * Process the environment information
 */

bool mne_attach_env(const QString& name, const QString& command)
/*
 * Add the environment info for future reference
 */
{
    int  insert_blocks[]  = { FIFFB_MNE , FIFFB_MEAS, FIFFB_MRI, FIFFB_BEM, -1 };
    QString cwd = QDir::currentPath();
    FiffId id;
    int     b,k, insert;
    FiffTag::SPtr t_pTag;
//    QList<FiffTag::SPtr> tags;
    QFile fileInOut(name);
    FiffStream::SPtr t_pStreamInOut;

//    if (fiff_new_file_id(&id) == FIFF_FAIL)
//        return false;
//    id = FiffId::new_file_id();

//#ifdef DEBUG
//    fprintf(stderr,"\n");
//    fprintf(stderr,"cwd   = %s\n",cwd);
//    fprintf(stderr,"com   = %s\n",command);
//    fprintf(stderr,"envid = %s\n",mne_format_file_id(&id));
//#endif

    if (!fileInOut.exists()) {
        qCritical("File %s does not exist. Cannot attach env info.",name.toUtf8().constData());
        return false;
    }
//    if (!fileInOut.isWritable()) {
//        qCritical("File %s is not writable. Cannot attach env info.",name.toUtf8().constData());
//        return false;
//    }
    /*
     * Open the file to modify
     */
    if (!(t_pStreamInOut = FiffStream::open_update(fileInOut)))
        return false;
    /*
     * Find an appropriate position to insert
     */
    for (insert = -1, b = 0; insert_blocks[b] >= 0; b++) {
        for (k = 0; k < t_pStreamInOut->nent(); k++) {
            if (t_pStreamInOut->dir()[k]->kind == FIFF_BLOCK_START) {
                if (!t_pStreamInOut->read_tag(t_pTag, t_pStreamInOut->dir()[k]->pos))
                    return false;
                if (*(t_pTag->toInt()) == insert_blocks[b]) {
                    insert = k;
                    break;
                }
            }
        }
        if (insert >= 0)
            break;
    }
    if (insert < 0) {
        qCritical("Suitable place for environment insertion not found.");
        return false;
    }

    /*
     * Do not build the list of tags to insert -> Do insertion right away
     */

    // Modified of fiff_insert_after
    int where = insert;
    /*
     * Insert new tags into a file
     * The directory information in dest is updated
     */
    if (where < 0 || where >= t_pStreamInOut->nent()-1) {
        qCritical("illegal insertion point in fiff_insert_after!");
        return false;
    }

    FiffTag::SPtr t_pTagNext;
    QList<FiffDirEntry::SPtr> old_dir = t_pStreamInOut->dir();
    QList<FiffDirEntry::SPtr> this_ent = old_dir.mid(where);//this_ent = old_dir + where;

    if (!t_pStreamInOut->read_tag(t_pTagNext, this_ent[0]->pos))
        return false;
    /*
     * Update next info to be sequential
     */
    qint64 next_tmp = t_pStreamInOut->device()->pos();
    /*
     * Go to the end of the file
     */
    t_pStreamInOut->device()->seek(fileInOut.size());//SEEK_END
    /*
     * Allocate new directory
     * Copy the beginning of old directory
     */
    QList<FiffDirEntry::SPtr> new_dir = old_dir.mid(0,where+1);

    /*
     * Save the old size for future purposes
     */
    qint64 old_end = t_pStreamInOut->device()->pos();
    /*
     * Write tags, check for errors
     */

    //Don't use the for loop here instead do it explicitly for specific tags
    FiffDirEntry::SPtr new_this;

    new_this = FiffDirEntry::SPtr(new FiffDirEntry);
    new_this->kind = FIFF_BLOCK_START;
    new_this->type = FIFFT_INT;
    new_this->size = 1 * 4;
    new_this->pos = t_pStreamInOut->start_block(FIFFB_MNE_ENV);
    new_dir.append(new_this);

    new_this = FiffDirEntry::SPtr(new FiffDirEntry);
    new_this->kind = FIFF_BLOCK_ID;
    new_this->type = FIFFT_ID_STRUCT;
    new_this->size =  5 * 4;
    new_this->pos = t_pStreamInOut->write_id(FIFF_BLOCK_ID,id);
    new_dir.append(new_this);

    new_this = FiffDirEntry::SPtr(new FiffDirEntry);
    new_this->kind = FIFF_MNE_ENV_WORKING_DIR;
    new_this->type = FIFFT_STRING;
    new_this->size =  cwd.size();
    new_this->pos = t_pStreamInOut->write_string(FIFF_MNE_ENV_WORKING_DIR,cwd);
    new_dir.append(new_this);

    new_this = FiffDirEntry::SPtr(new FiffDirEntry);
    new_this->kind = FIFF_MNE_ENV_COMMAND_LINE;
    new_this->type = FIFFT_STRING;
    new_this->size =  command.size();
    new_this->pos = t_pStreamInOut->write_string(FIFF_MNE_ENV_COMMAND_LINE,command);
    new_dir.append(new_this);

    new_this = FiffDirEntry::SPtr(new FiffDirEntry);
    new_this->kind = FIFF_BLOCK_END;
    new_this->type = FIFFT_INT;
    new_this->size =  1 * 4;

    new_this->pos = t_pStreamInOut->end_block(FIFFB_MNE_ENV,next_tmp);
    new_dir.append(new_this);

    /*
     * Copy the rest of the old directory
     */
    new_dir.append(old_dir.mid(where+1));
    /*
     * Now, it is time to update the braching tag
     * If something goes wrong here, we cannot be sure that
     * the file is readable. Let's hope for the best...
     */
    t_pTagNext->next = (qint32)old_end;//2GB cut of
    t_pStreamInOut->write_tag(t_pTagNext,this_ent[0]->pos);

    /*
     * Update
     */
    t_pStreamInOut->dir() = new_dir;

    // Finished fiff_insert_after

    return true;
}

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ComputeFwd::ComputeFwd(ComputeFwdSettings::SPtr pSettings)
    : sol(new FiffNamedMatrix)
    , sol_grad(new FiffNamedMatrix)
    , m_meg_forward(new FiffNamedMatrix)
    , m_meg_forward_grad(new FiffNamedMatrix)
    , m_eeg_forward(new FiffNamedMatrix)
    , m_eeg_forward_grad(new FiffNamedMatrix)
    , m_pSettings(pSettings)
{
    initFwd();
}

//=============================================================================================================

ComputeFwd::~ComputeFwd()
{
    //ToDo Garbage collection
    for (int k = 0; k < m_iNSpace; k++)
        if(m_spaces[k])
            delete m_spaces[k];
    if(m_mri_head_t)
        delete m_mri_head_t;
    if(m_meg_head_t)
        delete m_meg_head_t;
    if(m_megcoils)
        delete m_megcoils;
    if(m_eegels)
        delete m_eegels;
    if(m_eegModel)
        delete m_eegModel;
}

//=============================================================================================================

void ComputeFwd::initFwd()
{
    // TODO: This only temporary until we have the fwd dlibrary refactored. This is only done in order to provide easy testing in test_forward_solution.
    m_spaces                = Q_NULLPTR;
    m_iNSpace               = 0;
    m_iNSource              = 0;

    m_mri_head_t            = Q_NULLPTR;
    m_meg_head_t            = Q_NULLPTR;

    m_listMegChs = QList<FiffChInfo>();
    m_listEegChs = QList<FiffChInfo>();
    m_listCompChs = QList<FiffChInfo>();

    if(!m_pSettings->compute_grad) {
        m_meg_forward_grad = Q_NULLPTR;
        m_eeg_forward_grad = Q_NULLPTR;
    }

    int iNMeg               = 0;
    int iNEeg               = 0;
    int iNComp              = 0;

    m_templates             = Q_NULLPTR;
    m_megcoils              = Q_NULLPTR;
    m_compcoils             = Q_NULLPTR;
    m_compData              = Q_NULLPTR;
    m_eegels                = Q_NULLPTR;
    m_eegModels             = Q_NULLPTR;
    m_iNChan                = 0;

    int k;
    m_mri_id                = Q_NULLPTR;
    m_meas_id.clear();

    FILE* out               = Q_NULLPTR;     /**< Output filtered points here. */

    m_eegModel              = Q_NULLPTR;
    m_bemModel              = Q_NULLPTR;

    // Report the setup

    //    printf("\n");
    //    mne_print_version_info(stderr,argv[0],PROGRAM_VERSION,__DATE__,__TIME__);
    printf("\n");
    printf("Source space                 : %s\n",m_pSettings->srcname.toUtf8().constData());
    if (!(m_pSettings->transname.isEmpty()) || !(m_pSettings->mriname.isEmpty())) {
        printf("MRI -> head transform source : %s\n",!(m_pSettings->mriname.isEmpty()) ? m_pSettings->mriname.toUtf8().constData() : m_pSettings->transname.toUtf8().constData());
    } else {
        printf("MRI and head coordinates are assumed to be identical.\n");
    }
    printf("Measurement data             : %s\n",m_pSettings->measname.toUtf8().constData());
    if (!m_pSettings->bemname.isEmpty()) {
        printf("BEM model                    : %s\n",m_pSettings->bemname.toUtf8().constData());
    } else {
        printf("Sphere model                 : origin at (% 7.2f % 7.2f % 7.2f) mm\n",
               1000.0f*m_pSettings->r0[X_41],1000.0f*m_pSettings->r0[Y_41],1000.0f*m_pSettings->r0[Z_41]);
        if (m_pSettings->include_eeg) {
            printf("\n");

            if (m_pSettings->eeg_model_file.isEmpty()) {
                qCritical("!!!!!!!!!!TODO: default_eeg_model_file();");
                //                m_pSettings->eeg_model_file = default_eeg_model_file();
            }
            m_eegModels = FwdEegSphereModelSet::fwd_load_eeg_sphere_models(m_pSettings->eeg_model_file,m_eegModels);
            m_eegModels->fwd_list_eeg_sphere_models(stderr);

            if (m_pSettings->eeg_model_name.isEmpty()) {
                m_pSettings->eeg_model_name = QString("Default");
            }
            if ((m_eegModel = m_eegModels->fwd_select_eeg_sphere_model(m_pSettings->eeg_model_name)) == Q_NULLPTR) {
                return;
            }

            if (!m_eegModel->fwd_setup_eeg_sphere_model(m_pSettings->eeg_sphere_rad,m_pSettings->use_equiv_eeg,3)) {
                return;
            }

            printf("Using EEG sphere model \"%s\" with scalp radius %7.1f mm\n",
                   m_pSettings->eeg_model_name.toUtf8().constData(),1000*m_pSettings->eeg_sphere_rad);
            printf("%s the electrode locations to scalp\n",m_pSettings->scale_eeg_pos ? "Scale" : "Do not scale");

            m_eegModel->scale_pos = m_pSettings->scale_eeg_pos;
            VEC_COPY_41(m_eegModel->r0,m_pSettings->r0);

            printf("\n");
        }
    }
    printf("%s field computations\n",m_pSettings->accurate ? "Accurate" : "Standard");
    printf("Do computations in %s coordinates.\n",FiffCoordTransOld::mne_coord_frame_name(m_pSettings->coord_frame));
    printf("%s source orientations\n",m_pSettings->fixed_ori ? "Fixed" : "Free");
    if (m_pSettings->compute_grad) {
        printf("Compute derivatives with respect to source location coordinates\n");
    }
    printf("Destination for the solution : %s\n",m_pSettings->solname.toUtf8().constData());
    if (m_pSettings->do_all) {
        printf("Calculate solution for all source locations.\n");
    }
    if (m_pSettings->nlabel > 0) {
        printf("Source space will be restricted to sources in %d labels\n",m_pSettings->nlabel);
    }

    // Read the source locations

    printf("\n");
    printf("Reading %s...\n",m_pSettings->srcname.toUtf8().constData());
    if (MneSurfaceOrVolume::mne_read_source_spaces(m_pSettings->srcname,&m_spaces,&m_iNSpace) != OK) {
        return;
    }
    for (k = 0, m_iNSource = 0; k < m_iNSpace; k++) {
        if (m_pSettings->do_all) {
            MneSurfaceOrVolume::enable_all_sources(m_spaces[k]);
        }
        m_iNSource += m_spaces[k]->nuse;
    }
    if (m_iNSource == 0) {
        qCritical("No sources are active in these source spaces. --all option should be used.");
        return;
    }
    printf("Read %d source spaces a total of %d active source locations\n", m_iNSpace,m_iNSource);
    if (MneSurfaceOrVolume::restrict_sources_to_labels(m_spaces,m_iNSpace,m_pSettings->labels,m_pSettings->nlabel) == FAIL) {
        return;
    }

    // Read the MRI -> head coordinate transformation
    printf("\n");
    if (!m_pSettings->mriname.isEmpty()) {
        if ((m_mri_head_t = FiffCoordTransOld::mne_read_mri_transform(m_pSettings->mriname)) == Q_NULLPTR) {
            return;
        }
        if ((m_mri_id = get_file_id(m_pSettings->mriname)) == Q_NULLPTR) {
            qCritical("Couln't read MRI file id (How come?)");
            return;
        }
    }
    else if (!m_pSettings->transname.isEmpty()) {
        FiffCoordTransOld* t;
        if ((t = FiffCoordTransOld::mne_read_FShead2mri_transform(m_pSettings->transname.toUtf8().data())) == Q_NULLPTR) {
            return;
        }
        m_mri_head_t = t->fiff_invert_transform();
        if(t) {
            delete t;
        }
    } else {
        m_mri_head_t = FiffCoordTransOld::mne_identity_transform(FIFFV_COORD_MRI,FIFFV_COORD_HEAD);
    }
    FiffCoordTransOld::mne_print_coord_transform(stderr,m_mri_head_t);

    // Read the channel information and the MEG device -> head coordinate transformation
    // replace mne_read_meg_comp_eeg_ch_info_41()

    if(!m_pSettings->pFiffInfo) {

        QFile measname(m_pSettings->measname);
        FIFFLIB::FiffDirNode::SPtr DirNode;
        FiffStream::SPtr pStream(new FiffStream(&measname));
        FIFFLIB::FiffInfo fiffInfo;
        if(!pStream->open()) {
            qCritical() << "Could not open Stream.";
            return;
        }

        //Get Fiff info
        if(!pStream->read_meas_info(pStream->dirtree(), fiffInfo, DirNode)){
            qCritical() << "Could not find the channel information.";
            return;
        }
        pStream->close();
        m_pInfoBase = QSharedPointer<FIFFLIB::FiffInfo>(new FiffInfo(fiffInfo));
    } else {
        m_pInfoBase = m_pSettings->pFiffInfo;
    }
    if(!m_pInfoBase) {
        qCritical ("ComputeFwd::initFwd(): no FiffInfo");
        return;
    }
    if (mne_read_meg_comp_eeg_ch_info_41(m_pInfoBase,
                                         m_listMegChs,
                                         iNMeg,
                                         m_listCompChs,
                                         iNComp,
                                         m_listEegChs,
                                         iNEeg,
                                         &m_meg_head_t,
                                         m_meas_id) != OK) {
        return;
    }

    m_iNChan = iNMeg + iNEeg;

    printf("\n");
    if (iNMeg > 0) {
        printf("Read %3d MEG channels from %s\n",iNMeg,m_pSettings->measname.toUtf8().constData());
    }
    if (iNComp > 0) {
        printf("Read %3d MEG compensation channels from %s\n",iNComp,m_pSettings->measname.toUtf8().constData());
    }
    if (iNEeg > 0) {
        printf("Read %3d EEG channels from %s\n",iNEeg,m_pSettings->measname.toUtf8().constData());
    }
    if (!m_pSettings->include_meg) {
        printf("MEG not requested. MEG channels omitted.\n");
        m_listMegChs.clear();
        m_listCompChs.clear();
        iNMeg = 0;
        iNComp = 0;
    }
    else
        FiffCoordTransOld::mne_print_coord_transform(stderr,m_meg_head_t);
    if (!m_pSettings->include_eeg) {
        printf("EEG not requested. EEG channels omitted.\n");
        m_listEegChs.clear();
        iNEeg = 0;
    } else {
        if (mne_check_chinfo(m_listEegChs,iNEeg) != OK) {
            return;
        }
    }

    // Create coil descriptions with transformation to head or MRI frame

    if (m_pSettings->include_meg) {
        //#ifdef USE_SHARE_PATH
        //        char *coilfile = mne_compose_mne_name("share/mne","coil_def.dat");
        //#else
        //        char *coilfile = mne_compose_mne_name("setup/mne","coil_def.dat");

        qPath = QString(QCoreApplication::applicationDirPath() + "/resources/general/coilDefinitions/coil_def.dat");
        file.setFileName(qPath);
        if ( !QCoreApplication::startingUp() ) {
            qPath = QCoreApplication::applicationDirPath() + QString("/resources/general/coilDefinitions/coil_def.dat");
        } else if (!file.exists()) {
            qPath = "./resources/general/coilDefinitions/coil_def.dat";
        }

        char *coilfile = MALLOC_41(strlen(qPath.toUtf8().data())+1,char);
        strcpy_s(coilfile, strlen(qPath.toUtf8()), qPath.toUtf8().data());
        //#endif

        if (!coilfile) {
            return;
        }

        m_templates = FwdCoilSet::read_coil_defs(coilfile);
        FREE_41(coilfile);
        if (!m_templates) {
            return;
        }

        // Compensation data

        if ((m_compData = MneCTFCompDataSet::mne_read_ctf_comp_data(m_pSettings->measname)) == Q_NULLPTR) {
            return;
        }
        // Compensation channel information may be needed
        if (m_compData->ncomp > 0) {
            printf("%d compensation data sets in %s\n",m_compData->ncomp,m_pSettings->measname.toUtf8().constData());
        } else {
            m_listCompChs.clear();
            iNComp = 0;

            if(m_compData) {
                delete m_compData;
            }
            m_compData = Q_NULLPTR;
        }
    }
    if (m_pSettings->coord_frame == FIFFV_COORD_MRI) {
        FiffCoordTransOld* head_mri_t = m_mri_head_t->fiff_invert_transform();
        FiffCoordTransOld* meg_mri_t = FiffCoordTransOld::fiff_combine_transforms(FIFFV_COORD_DEVICE,FIFFV_COORD_MRI,m_meg_head_t,head_mri_t);
        if (meg_mri_t == Q_NULLPTR) {
            return;
        }
        if ((m_megcoils = m_templates->create_meg_coils(m_listMegChs,
                                                      iNMeg,
                                                      m_pSettings->accurate ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                      meg_mri_t)) == Q_NULLPTR) {
            return;
        }
        if (iNComp > 0) {
            if ((m_compcoils = m_templates->create_meg_coils(m_listCompChs,
                                                             iNComp,
                                                             FWD_COIL_ACCURACY_NORMAL,
                                                             meg_mri_t)) == Q_NULLPTR) {
                return;
            }
        }
        if ((m_eegels = FwdCoilSet::create_eeg_els(m_listEegChs,
                                                   iNEeg,
                                                   head_mri_t)) == Q_NULLPTR) {
            return;
        }

        FREE_41(head_mri_t);
        printf("MRI coordinate coil definitions created.\n");
    } else {
        if ((m_megcoils = m_templates->create_meg_coils(m_listMegChs,
                                                        iNMeg,
                                                        m_pSettings->accurate ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                        m_meg_head_t)) == Q_NULLPTR) {
            return;
        }

        if (iNComp > 0) {
            if ((m_compcoils = m_templates->create_meg_coils(m_listCompChs,
                                                             iNComp,
                                                             FWD_COIL_ACCURACY_NORMAL,m_meg_head_t)) == Q_NULLPTR) {
                return;
            }
        }
        if ((m_eegels = FwdCoilSet::create_eeg_els(m_listEegChs,
                                                   iNEeg,
                                                   Q_NULLPTR)) == Q_NULLPTR) {
            return;
        }
        printf("Head coordinate coil definitions created.\n");
    }

    // Transform the source spaces into the appropriate coordinates

    if (MneSurfaceOrVolume::mne_transform_source_spaces_to(m_pSettings->coord_frame,m_mri_head_t,m_spaces,m_iNSpace) != OK) {
        return;
    }
    printf("Source spaces are now in %s coordinates.\n",FiffCoordTransOld::mne_coord_frame_name(m_pSettings->coord_frame));

    // Prepare the BEM model if necessary

    if (!m_pSettings->bemname.isEmpty()) {
        QString bemsolname = FwdBemModel::fwd_bem_make_bem_sol_name(m_pSettings->bemname);
        //        FREE(bemname);
        m_pSettings->bemname = bemsolname;

        printf("\nSetting up the BEM model using %s...\n",m_pSettings->bemname.toUtf8().constData());
        printf("\nLoading surfaces...\n");
        m_bemModel = FwdBemModel::fwd_bem_load_three_layer_surfaces(m_pSettings->bemname);

        if (m_bemModel) {
            printf("Three-layer model surfaces loaded.\n");
        }
        else {
            m_bemModel = FwdBemModel::fwd_bem_load_homog_surface(m_pSettings->bemname);
            if (!m_bemModel) {
                return;
            }
            printf("Homogeneous model surface loaded.\n");
        }
        if (iNEeg > 0 && m_bemModel->nsurf == 1) {
            qCritical("Cannot use a homogeneous model in EEG calculations.");
            return;
        }
        printf("\nLoading the solution matrix...\n");
        if (FwdBemModel::fwd_bem_load_recompute_solution(m_pSettings->bemname.toUtf8().data(),FWD_BEM_UNKNOWN,FALSE,m_bemModel) == FAIL) {
            return;
        }
        if (m_pSettings->coord_frame == FIFFV_COORD_HEAD) {
            printf("Employing the head->MRI coordinate transform with the BEM model.\n");
            if (FwdBemModel::fwd_bem_set_head_mri_t(m_bemModel,m_mri_head_t) == FAIL) {
                return;
            }
        }
        printf("BEM model %s is now set up\n",m_bemModel->sol_name.toUtf8().constData());
    } else {
        printf("Using the sphere model.\n");
    }
    printf ("\n");

    // Try to circumvent numerical problems by excluding points too close our ouside the inner skull surface

    if (m_pSettings->filter_spaces) {
        if (!m_pSettings->mindistoutname.isEmpty()) {
            out = fopen(m_pSettings->mindistoutname.toUtf8().constData(),"w");
            if (out == Q_NULLPTR) {
                qCritical() << m_pSettings->mindistoutname;
                return;
            }
            printf("Omitted source space points will be output to : %s\n",m_pSettings->mindistoutname.toUtf8().constData());
        }
        if (MneSurfaceOrVolume::filter_source_spaces(m_pSettings->mindist,
                                                     m_pSettings->bemname.toUtf8().data(),
                                                     m_mri_head_t,
                                                     m_spaces,
                                                     m_iNSpace,out,m_pSettings->use_threads) == FAIL) {
            if(out)
                fclose(out);
            return;
        }
    }
}

//=========================================================================================================

void ComputeFwd::calculateFwd()
{
    int iNMeg = 0;
    int iNEeg = 0;

    if(m_megcoils) {
        iNMeg = m_megcoils->ncoil;
    }
    if(m_eegels) {
        iNEeg = m_eegels->ncoil;
    }
    if (!m_bemModel) {
        m_pSettings->use_threads = false;
    }

    // check if source spaces are still in head space
    if(m_spaces[0]->coord_frame != FIFFV_COORD_HEAD) {
        if (MneSurfaceOrVolume::mne_transform_source_spaces_to(m_pSettings->coord_frame,m_mri_head_t,m_spaces,m_iNSpace) != OK) {
            return;
        }
    }

    // Do the actual computation
    if (iNMeg > 0) {
        if ((FwdBemModel::compute_forward_meg(m_spaces,
                                              m_iNSpace,
                                              m_megcoils,
                                              m_compcoils,
                                              m_compData,
                                              m_pSettings->fixed_ori,
                                              m_bemModel,
                                              &m_pSettings->r0,
                                              m_pSettings->use_threads,
                                              *m_meg_forward.data(),
                                              *m_meg_forward_grad.data(),
                                              m_pSettings->compute_grad)) == FAIL) {
            return;
        }
    }
    if (iNEeg > 0) {
        if ((FwdBemModel::compute_forward_eeg(m_spaces,
                                              m_iNSpace,
                                              m_eegels,
                                              m_pSettings->fixed_ori,
                                              m_bemModel,
                                              m_eegModel,
                                              m_pSettings->use_threads,
                                              *m_eeg_forward.data(),
                                              *m_eeg_forward_grad.data(),
                                              m_pSettings->compute_grad))== FAIL) {
            return;
        }
    }
    if(iNMeg > 0 && iNEeg > 0) {
        if(m_meg_forward->data.cols() != m_eeg_forward->data.cols()) {
            qWarning() << "The MEG and EEG forward solutions do not match";
            return;
        }
        sol->clear();
        sol->data = MatrixXd(m_meg_forward->nrow + m_eeg_forward->nrow, m_meg_forward->ncol);
        sol->data.block(0,0,m_meg_forward->nrow,m_meg_forward->ncol) = m_meg_forward->data;
        sol->data.block(m_meg_forward->nrow,0,m_eeg_forward->nrow,m_eeg_forward->ncol) = m_eeg_forward->data;
        sol->nrow = m_meg_forward->nrow + m_eeg_forward->nrow;
        sol->row_names.append(m_meg_forward->row_names);
    } else if (iNMeg > 0) {
        sol = m_meg_forward;
    } else {
        sol = m_eeg_forward;
    }

    if(m_pSettings->compute_grad) {
        if(iNMeg > 0 && iNEeg > 0) {
            if(m_meg_forward_grad->data.cols() != m_eeg_forward_grad->data.cols()) {
                qWarning() << "The MEG and EEG forward solutions do not match";
                return;
            }
            sol_grad->clear();
            sol_grad->data = MatrixXd(m_meg_forward_grad->nrow + m_eeg_forward_grad->nrow, m_meg_forward_grad->ncol);
            sol_grad->data.block(0,0,m_meg_forward_grad->nrow,m_meg_forward_grad->ncol) = m_meg_forward_grad->data;
            sol_grad->data.block(m_meg_forward_grad->nrow,0,m_eeg_forward_grad->nrow,m_eeg_forward_grad->ncol) = m_eeg_forward_grad->data;
            sol_grad->nrow = m_meg_forward_grad->nrow + m_eeg_forward_grad->nrow;
            sol_grad->row_names.append(m_meg_forward_grad->row_names);
        } else if (iNMeg > 0) {
            sol_grad = m_meg_forward_grad;
        } else {
            sol_grad = m_eeg_forward_grad;
        }
    }
}

//=========================================================================================================

void ComputeFwd::updateHeadPos(FiffCoordTransOld* transDevHeadOld)
{

    int iNMeg = 0;
    if(m_megcoils) {
        iNMeg = m_megcoils->ncoil;
    }

    int iNComp = 0;
    if(m_compcoils) {
        iNMeg = m_megcoils->ncoil;
    }
//    // transformation in head space
//    FiffCoordTransOld* transHeadHeadOld = new FiffCoordTransOld;
//
//    // get transformation matrix in Headspace between two meg -> head transformation
//    // ToDo: the following part has to be tested
//    if(transDevHeadOld->from != m_megcoils->coord_frame) {
//        if(transDevHeadOld->to != m_megcoils->coord_frame) {
//            qWarning() << "ComputeFwd::updateHeadPos: Target Space not in Head Space - Space: " << transDevHeadOld->to;
//            return;
//        }
//        // calclulate rotation
//        Matrix3f matRot = m_meg_head_t->rot;              // meg->head to origin
//        Matrix3f matRotDest = transDevHeadOld->rot;       // meg->head after movement
//        Matrix3f matRotNew = Matrix3f::Zero(3,3);         // rotation between origin and updated point
//
//        // get Quaternions
//        Quaternionf quat(matRot);
//        Quaternionf quatDest(matRotDest);
//        Quaternionf quatNew;
//        // Rotation between two rotation matrices is calculated by multiplying with the inverse
//        // this is computitional easier with quaternions because for inverse we just have to flip signs
//        quatNew = quat*quatDest.inverse();
//        matRotNew = quatNew.toRotationMatrix();           // rotation between origin and updated point
//
//        // calculate translation
//        // translation between to points = vector rsulting from substraction
//        VectorXf vecTrans = m_meg_head_t->move;
//        VectorXf vecTransDest = transDevHeadOld->move;
//        VectorXf vecTransNew = vecTrans - vecTransDest;
//
//        // update new transformation matrix
//        transHeadHeadOld->from = m_megcoils->coord_frame;
//        transHeadHeadOld->to = m_megcoils->coord_frame;
//        transHeadHeadOld->rot = matRotNew;
//        transHeadHeadOld->move = vecTransNew;
//        transHeadHeadOld->add_inverse(transHeadHeadOld);
//    }
//
//    FwdCoilSet* megcoilsNew = m_megcoils->dup_coil_set(transHeadHeadOld);

    // create new coilset with updated head position
    if (m_pSettings->coord_frame == FIFFV_COORD_MRI) {
        FiffCoordTransOld* head_mri_t = m_mri_head_t->fiff_invert_transform();
        FiffCoordTransOld* meg_mri_t = FiffCoordTransOld::fiff_combine_transforms(FIFFV_COORD_DEVICE,FIFFV_COORD_MRI,transDevHeadOld,head_mri_t);
        if (meg_mri_t == Q_NULLPTR) {
            return;
        }
        if ((m_megcoils = m_templates->create_meg_coils(m_listMegChs,
                                                        iNMeg,
                                                        m_pSettings->accurate ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                        meg_mri_t)) == Q_NULLPTR) {
            return;
        }
        if (iNComp > 0) {
            if ((m_compcoils = m_templates->create_meg_coils(m_listCompChs,
                                                             iNComp,
                                                             FWD_COIL_ACCURACY_NORMAL,
                                                             meg_mri_t)) == Q_NULLPTR) {
                return;
            }
        }
    } else {
        if ((m_megcoils = m_templates->create_meg_coils(m_listMegChs,
                                                        iNMeg,
                                                        m_pSettings->accurate ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                        transDevHeadOld)) == Q_NULLPTR) {
            return;
        }

        if (iNComp > 0) {
            if ((m_compcoils = m_templates->create_meg_coils(m_listCompChs,
                                                             iNComp,
                                                             FWD_COIL_ACCURACY_NORMAL,transDevHeadOld)) == Q_NULLPTR) {
                return;
            }
        }
    }

    // check if source spaces are still in head space
    if(m_spaces[0]->coord_frame != FIFFV_COORD_HEAD) {
        if (MneSurfaceOrVolume::mne_transform_source_spaces_to(m_pSettings->coord_frame,m_mri_head_t,m_spaces,m_iNSpace) != OK) {
            return;
        }
    }

    // recompute meg forward
    if ((FwdBemModel::compute_forward_meg(m_spaces,
                                          m_iNSpace,
                                          m_megcoils,
                                          m_compcoils,
                                          m_compData,                   // we might have to update this too
                                          m_pSettings->fixed_ori,
                                          m_bemModel,
                                          &m_pSettings->r0,
                                          m_pSettings->use_threads,
                                          *m_meg_forward.data(),
                                          *m_meg_forward_grad.data(),
                                          m_pSettings->compute_grad)) == FAIL) {
        return;
    }

    // Update new Transformation Matrix
    m_meg_head_t = new FiffCoordTransOld(*transDevHeadOld);
    // update solution
    sol->data.block(0,0,m_meg_forward->nrow,m_meg_forward->ncol) = m_meg_forward->data;
    if(m_pSettings->compute_grad) {
        sol_grad->data.block(0,0,m_meg_forward_grad->nrow,m_meg_forward_grad->ncol) = m_meg_forward_grad->data;
    }
}

//=========================================================================================================

void ComputeFwd::storeFwd(const QString& sSolName)
{
    // We are ready to spill it out
    // Transform the source spaces back into MRI coordinates
    if (MneSourceSpaceOld::mne_transform_source_spaces_to(FIFFV_COORD_MRI,m_mri_head_t,m_spaces,m_iNSpace) != OK) {
        return;
    }

    int iNMeg = m_megcoils->ncoil;
    int iNEeg = m_eegels->ncoil;

    QString sName;

    if(sSolName == "default") {
        sName = m_pSettings->solname;
    } else {
        sName = sSolName;
    }

    printf("\nwriting %s...",sSolName.toUtf8().constData());

    if (!write_solution(sName,                                  /* Destination file */
                        m_spaces,                               /* The source spaces */
                        m_iNSpace,
                        m_pSettings->mriname,m_mri_id,          /* MRI file and data obtained from there */
                        m_mri_head_t,
                        m_pSettings->measname,m_meas_id,        /* MEG file and data obtained from there */
                        m_meg_head_t,
                        m_listMegChs,
                        iNMeg,
                        m_listEegChs,
                        iNEeg,
                        m_pSettings->fixed_ori,                 /* Fixed orientation dipoles? */
                        m_pSettings->coord_frame,               /* Coordinate frame */
                        *m_meg_forward.data(),
                        *m_eeg_forward.data(),
                        *m_meg_forward_grad.data(),
                        *m_eeg_forward_grad.data(),
                        m_pSettings->compute_grad)) {
        return;
    }

    if (!mne_attach_env(m_pSettings->solname,m_pSettings->command)) {
        return;
    }
    printf("done\n");
    printf("\nFinished.\n");
}
