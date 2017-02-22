

#include "compute_fwd.h"

#include <inverse/dipoleFit/mne_source_space_old.h>
#include <inverse/dipoleFit/fiff_coord_trans_old.h>
#include <inverse/dipoleFit/fwd_coil_set.h>
#include <inverse/dipoleFit/mne_ctf_comp_data_set.h>
#include <inverse/dipoleFit/fwd_eeg_sphere_model_set.h>
#include <inverse/dipoleFit/fwd_bem_model.h>
#include <inverse/dipoleFit/mne_named_matrix.h>

#include <inverse/dipoleFit/fiff_sparse_matrix.h>

#include <inverse/dipoleFit/mne_nearest.h>

#include <fiff/fiff_types.h>

#include <time.h>

#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QDateTime>
//#include <QNetworkInterface>

using namespace Eigen;
using namespace INVERSELIB;
using namespace FIFFLIB;
using namespace FWDLIB;

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
#define REALLOC_41(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))


#define FREE_41(x) if ((char *)(x) != NULL) free((char *)(x))


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
    if(!stream->open())
        return NULL;
    else {
        id = MALLOC_41(1,fiffIdRec);
        FiffId id_new = stream->id();

        id->version = id_new.version;       /**< File version */
        id->machid[0] = id_new.machid[0];   /**< Unique machine ID */
        id->machid[1] = id_new.machid[1];
        id->time = id_new.time;             /**< Time of the ID creation */

        stream->close();
        return id;
    }
}


//============================= mne_read_forward_solution.c =============================

int mne_read_meg_comp_eeg_ch_info_41(const QString& name,
                                     fiffChInfo     *megp,	 /* MEG channels */
                                     int            *nmegp,
                                     fiffChInfo     *meg_compp,
                                     int            *nmeg_compp,
                                     fiffChInfo     *eegp,	 /* EEG channels */
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


    fiffChInfo chs   = NULL;
    int        nchan = 0;
    fiffChInfo meg   = NULL;
    int        nmeg  = 0;
    fiffChInfo meg_comp = NULL;
    int        nmeg_comp = 0;
    fiffChInfo eeg   = NULL;
    int        neeg  = 0;
    fiffId     id    = NULL;
    QList<FiffDirNode::SPtr> nodes;
    FiffDirNode::SPtr info;
    FiffTag::SPtr t_pTag;
    fiffChInfo   this_ch = NULL;
    FiffCoordTransOld* t = NULL;
    fiff_int_t kind, pos;
    int j,k,to_find;

    if(!stream->open())
        goto bad;

    nodes = stream->tree()->dir_tree_find(FIFFB_MNE_PARENT_MEAS_FILE);

    if (nodes.size() == 0) {
        nodes = stream->tree()->dir_tree_find(FIFFB_MEAS_INFO);
        if (nodes.size() == 0) {
            qCritical ("Could not find the channel information.");
            goto bad;
        }
    }
    info = nodes[0];
    to_find = 0;
    for (k = 0; k < info->nent; k++) {
        kind = info->dir[k]->kind;
        pos  = info->dir[k]->pos;
        switch (kind) {
        case FIFF_NCHAN :
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            nchan = *t_pTag->toInt();
            chs = MALLOC_41(nchan,fiffChInfoRec);
            for (j = 0; j < nchan; j++)
                chs[j].scanNo = -1;
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
            //            this_ch = t_pTag->toChInfo();
            this_ch = (fiffChInfo)malloc(sizeof(fiffChInfoRec));
            *this_ch = *(fiffChInfo)(t_pTag->data());
            if (this_ch->scanNo <= 0 || this_ch->scanNo > nchan) {
                printf ("FIFF_CH_INFO : scan # out of range %d (%d)!",this_ch->scanNo,nchan);
                goto bad;
            }
            else
                chs[this_ch->scanNo-1] = *this_ch;
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
    for (k = 0; k < nchan; k++)
        if (chs[k].kind == FIFFV_MEG_CH)
            nmeg++;
        else if (chs[k].kind == FIFFV_REF_MEG_CH)
            nmeg_comp++;
        else if (chs[k].kind == FIFFV_EEG_CH)
            neeg++;
    if (nmeg > 0)
        meg = MALLOC_41(nmeg,fiffChInfoRec);
    if (neeg > 0)
        eeg = MALLOC_41(neeg,fiffChInfoRec);
    if (nmeg_comp > 0)
        meg_comp = MALLOC_41(nmeg_comp,fiffChInfoRec);
    neeg = nmeg = nmeg_comp = 0;

    for (k = 0; k < nchan; k++)
        if (chs[k].kind == FIFFV_MEG_CH)
            meg[nmeg++] = chs[k];
        else if (chs[k].kind == FIFFV_REF_MEG_CH)
            meg_comp[nmeg_comp++] = chs[k];
        else if (chs[k].kind == FIFFV_EEG_CH)
            eeg[neeg++] = chs[k];
    //    fiff_close(in);
    stream->close();
    FREE_41(chs);
    if (megp) {
        *megp  = meg;
        *nmegp = nmeg;
    }
    else
        FREE_41(meg);
    if (meg_compp) {
        *meg_compp = meg_comp;
        *nmeg_compp = nmeg_comp;
    }
    else
        FREE_41(meg_comp);
    if (eegp) {
        *eegp  = eeg;
        *neegp = neeg;
    }
    else
        FREE_41(eeg);
    if (idp == NULL) {
        FREE_41(id);
    }
    else
        *idp   = id;
    if (meg_head_t == NULL) {
        FREE_41(t);
    }
    else
        *meg_head_t = t;

    return FIFF_OK;

bad : {
        //        fiff_close(in);
        stream->close();
        FREE_41(chs);
        FREE_41(meg);
        FREE_41(eeg);
        FREE_41(id);
        //        FREE_41(tag.data);
        FREE_41(t);
        return FIFF_FAIL;
    }
}



int mne_check_chinfo(fiffChInfo chs,
                     int        nch)
/*
 * Check that all EEG channels have reasonable locations
 */
{
    int k;
    fiffChInfo ch;
    float close = 0.02;

    for (k = 0, ch = chs; k < nch; k++, ch++) {
        if (ch->kind == FIFFV_EEG_CH) {
            if (VEC_LEN_41(ch->chpos.r0) < close) {
                qCritical("Some EEG channels do not have locations assigned.");
                return FAIL;
            }
        }
    }
    return OK;
}


//*************************************************************************************************************
// Temporary Helpers
//*************************************************************************************************************

void write_id_old(FiffStream::SPtr& t_pStream, fiff_int_t kind, fiffId id)
{
    fiffId t_id = id;
    if(t_id->version == -1)
    {
        /* initialize random seed: */
        srand ( time(NULL) );
        double rand_1 = (double)(rand() % 100);rand_1 /= 100;
        double rand_2 = (double)(rand() % 100);rand_2 /= 100;

        time_t seconds;
        seconds = time (NULL);

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


//*************************************************************************************************************

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
            *t_pStream << (float)trans->rot[r][c];
    for (r = 0; r < 3; ++r)
        *t_pStream << (float)trans->move[r];

    //
    //   ...and its inverse
    //
    for (r = 0; r < 3; ++r)
        for (c = 0; c < 3; ++c)
            *t_pStream << (float)trans->invrot[r][c];
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
    //        { FIFF_BLOCK_START,      FIFFT_INT,    0, FIFFV_NEXT_SEQ, NULL },
    //        { FIFF_MNE_CH_NAME_LIST, FIFFT_STRING, 0, FIFFV_NEXT_SEQ, NULL },
    //        { FIFF_BLOCK_END,        FIFFT_INT,    0, FIFFV_NEXT_SEQ, NULL }};
    //    int         nbad_channel_block_tags = 3;
    //    char        *names = NULL;
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
    //  tag.data = NULL;
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
    //  tag.data = NULL;
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
    if (mat->coding == FIFFTS_MC_CCS)
        qDebug() << "TODO write CCS - use eigen";
    else if (mat->coding == FIFFTS_MC_RCS)
        qDebug() << "TODO write RCS - use eigen";
    else {
        qCritical("Incomprehensible sparse matrix coding");
        return FIFF_FAIL;
    }
    return FIFF_OK;
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
    if (npoint > 1 && points != NULL)
        qsort(points,npoint,sizeof(MneNearest),comp_points2);
    return;
}







FiffSparseMatrix* mne_create_sparse_rcs(int nrow,           /* Number of rows */
                                        int ncol,           /* Number of columns */
                                        int *nnz,           /* Number of non-zero elements on each row */
                                        int **colindex,     /* Column indices of non-zero elements on each row */
                                        float **vals)       /* The nonzero elements on each row
                                                     * If null, the matrix will be all zeroes */

{
    FiffSparseMatrix* sparse = NULL;
    int j,k,nz,ptr,size,ind;
    int stor_type = FIFFTS_MC_RCS;

    for (j = 0, nz = 0; j < nrow; j++)
        nz = nz + nnz[j];

    if (nz <= 0) {
        qCritical("No nonzero elements specified.");
        return NULL;
    }
    if (stor_type == FIFFTS_MC_RCS) {
        size = nz*(sizeof(fiff_float_t) + sizeof(fiff_int_t)) +
                (nrow+1)*(sizeof(fiff_int_t));
    }
    else {
        qCritical("Illegal sparse matrix storage type: %d",stor_type);
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
        return NULL;
    }
}



FiffSparseMatrix* mne_pick_lower_triangle_rcs(FiffSparseMatrix* mat)
/*
* Fill in upper triangle with the lower triangle values
*/
{
    int             *nnz       = NULL;
    int             **colindex = NULL;
    float           **vals     = NULL;
    FiffSparseMatrix* res = NULL;
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
            colindex[i] = NULL;
            vals[i] = NULL;
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
    int *nneighbors = NULL;
    int *neighbors = NULL;
    int *inuse_map = NULL;
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
        if (ss->interpolator && ss->MRI_volume) {
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
        if (ss->interpolator && ss->MRI_volume) {
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
    float **sel = NULL;
    int   **tris = NULL;
    int   *nearest = NULL;
    float *nearest_dist = NULL;
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
    if (ss->subject && strlen(ss->subject) > 0) {
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
        FREE_CMATRIX_41(sel); sel = NULL;
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
            FREE_ICMATRIX(tris); tris = NULL;
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
            FREE_ICMATRIX_41(tris); tris = NULL;
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
            FREE_ICMATRIX_41(tris); tris = NULL;
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

            FREE_41(nearest); nearest = NULL;
            FREE_41(nearest_dist); nearest_dist = NULL;
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




char *mne_name_list_to_string_41(char **list,int nlist)
/*
* Convert a string array to a colon-separated string
*/
{
    int k,len;
    char *res;
    if (nlist == 0 || list == NULL)
        return NULL;
    for (k = len = 0; k < nlist; k++)
        len += strlen(list[k])+1;
    res = MALLOC_41(len,char);
    res[0] = '\0';
    for (k = len = 0; k < nlist-1; k++) {
        strcat(res,list[k]);
        strcat(res,":");
    }
    strcat(res,list[nlist-1]);
    return res;
}


int mne_write_named_matrix( FiffStream::SPtr& t_pStream,
                            int  kind,
                            MneNamedMatrix* mat)
/*
* Write a block which contains information about one named matrix
*/
{
    char *names;

    t_pStream->start_block(FIFFB_MNE_NAMED_MATRIX);

    t_pStream->write_int(FIFF_MNE_NROW,&mat->nrow);
    t_pStream->write_int(FIFF_MNE_NCOL,&mat->ncol);
    if (mat->rowlist) {
        names = mne_name_list_to_string_41(mat->rowlist,mat->nrow);
        t_pStream->write_string(FIFF_MNE_ROW_NAMES,names);
        FREE_41(names);
    }
    if (mat->collist) {
        names = mne_name_list_to_string_41(mat->collist,mat->ncol);
        t_pStream->write_string(FIFF_MNE_COL_NAMES,names);
        FREE_41(names);
    }
    fiff_write_float_matrix_old (t_pStream,kind,mat->data,mat->nrow,mat->ncol);

    t_pStream->end_block(FIFFB_MNE_NAMED_MATRIX);

    return FIFF_OK;

bad :
    return FIFF_FAIL;
}






//int fiff_put_dir (FILE *fd, fiffDirEntry dir)
///*
//      * Put in new directory
//      */
//{
//    int nent = fiff_how_many_entries (dir);
//    int k;
//    fiffTagRec tag;
//    fiffTagRec dirtag;
//    fiff_int_t dirpos;

//    tag.data = NULL;
//    for (k = 0; k < nent; k++) {
//        if (dir[k].kind == FIFF_DIR_POINTER) {
//            /*
//       * Read current value of directory pointer
//       */
//            if (fiff_read_this_tag(fd,dir[k].pos,&tag) == -1) {
//                fprintf (stderr,"Could not read FIFF_DIR_POINTER!\n");
//                return (-1);
//            }
//            /*
//       * If there is no directory, append the new one
//       */
//            dirpos = *(fiff_int_t *)(tag.data);
//            FREE(tag.data);
//            if (dirpos <= 0)
//                dirpos = -1;
//            /*
//       * Put together the directory tag
//       */
//            dirtag.kind = FIFF_DIR;
//            dirtag.type = FIFFT_DIR_ENTRY_STRUCT;
//            dirtag.size = nent*sizeof(fiff_dir_entry_t);
//            dirtag.next = -1;
//            dirtag.data = (fiff_byte_t *)dir;
//            dirpos = fiff_write_this_tag(fd,-1L,&dirtag);
//            if (dirpos < 0)
//                fprintf (stderr,"Could not update directory!\n");
//            else {
//                tag.data = (fiff_byte_t *)(&dirpos);
//                if (fiff_write_this_tag(fd,dir[k].pos,&tag) == -1) {
//                    fprintf (stderr,"Could not update directory pointer!\n");
//                    return (-1);
//                }
//            }
//            return (0);
//        }
//    }
//    fprintf (stderr,"Could not find place for directory!\n");
//    return (-1);
//}





//============================= write_solution.c =============================

int write_solution(const QString& name,         /* Destination file */
                   MneSourceSpaceOld* *spaces,  /* The source spaces */
                   int            nspace,
                   const QString& mri_file,     /* MRI file and data obtained from there */
                   fiffId         mri_id,
                   FiffCoordTransOld* mri_head_t,

                   const QString& meas_file,    /* MEG file and data obtained from there */
                   fiffId         meas_id,
                   FiffCoordTransOld* meg_head_t,
                   fiffChInfo     meg_chs,
                   int            nmeg,
                   fiffChInfo     eeg_chs,
                   int            neeg,
                   int            fixed_ori,    /* Fixed orientation dipoles? */
                   int            coord_frame,  /* Coordinate frame employed in the forward calculations */
                   MneNamedMatrix* meg_solution,
                   MneNamedMatrix* eeg_solution,
                   MneNamedMatrix* meg_solution_grad,
                   MneNamedMatrix* eeg_solution_grad)

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
        if (mri_id != NULL)
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
        if (meas_id != NULL)
            write_id_old(t_pStream, FIFF_PARENT_BLOCK_ID, meas_id);//t_pStream->write_id(FIFF_PARENT_BLOCK_ID, meas_id);
        write_coord_trans_old(t_pStream, meg_head_t);//t_pStream->write_coord_trans(meg_head_t);

        int nchan = nmeg+neeg;
        t_pStream->write_int(FIFF_NCHAN,&nchan);

        FiffChInfo chInfo;
        int k, p;
        for (k = 0, p = 0; k < nmeg; k++) {
            meg_chs[k].scanNo = ++p;
            chInfo.scanNo = meg_chs[k].scanNo;
            chInfo.logNo = meg_chs[k].logNo;
            chInfo.kind = meg_chs[k].kind;
            chInfo.range = meg_chs[k].range;
            chInfo.cal = meg_chs[k].cal;
            chInfo.chpos.coil_type = meg_chs[k].chpos.coil_type;
            chInfo.chpos.r0[0] = meg_chs[k].chpos.r0[0];
            chInfo.chpos.r0[1] = meg_chs[k].chpos.r0[1];
            chInfo.chpos.r0[2] = meg_chs[k].chpos.r0[2];
            chInfo.chpos.ex[0] = meg_chs[k].chpos.ex[0];
            chInfo.chpos.ex[1] = meg_chs[k].chpos.ex[1];
            chInfo.chpos.ex[2] = meg_chs[k].chpos.ex[2];
            chInfo.chpos.ey[0] = meg_chs[k].chpos.ey[0];
            chInfo.chpos.ey[1] = meg_chs[k].chpos.ey[1];
            chInfo.chpos.ey[2] = meg_chs[k].chpos.ey[2];
            chInfo.chpos.ez[0] = meg_chs[k].chpos.ez[0];
            chInfo.chpos.ez[1] = meg_chs[k].chpos.ez[1];
            chInfo.chpos.ez[2] = meg_chs[k].chpos.ez[2];
            chInfo.unit = meg_chs[k].unit;
            chInfo.unit_mul = meg_chs[k].unit_mul;
            chInfo.ch_name = QString(meg_chs[k].ch_name);

            t_pStream->write_ch_info(chInfo);
        }

        for (k = 0; k < neeg; k++) {
            eeg_chs[k].scanNo = ++p;
            chInfo.scanNo = eeg_chs[k].scanNo;
            chInfo.logNo = eeg_chs[k].logNo;
            chInfo.kind = eeg_chs[k].kind;
            chInfo.range = eeg_chs[k].range;
            chInfo.cal = eeg_chs[k].cal;
            chInfo.chpos.coil_type = eeg_chs[k].chpos.coil_type;
            chInfo.chpos.r0[0] = eeg_chs[k].chpos.r0[0];
            chInfo.chpos.r0[1] = eeg_chs[k].chpos.r0[1];
            chInfo.chpos.r0[2] = eeg_chs[k].chpos.r0[2];
            chInfo.chpos.ex[0] = eeg_chs[k].chpos.ex[0];
            chInfo.chpos.ex[1] = eeg_chs[k].chpos.ex[1];
            chInfo.chpos.ex[2] = eeg_chs[k].chpos.ex[2];
            chInfo.chpos.ey[0] = eeg_chs[k].chpos.ey[0];
            chInfo.chpos.ey[1] = eeg_chs[k].chpos.ey[1];
            chInfo.chpos.ey[2] = eeg_chs[k].chpos.ey[2];
            chInfo.chpos.ez[0] = eeg_chs[k].chpos.ez[0];
            chInfo.chpos.ez[1] = eeg_chs[k].chpos.ez[1];
            chInfo.chpos.ez[2] = eeg_chs[k].chpos.ez[2];
            chInfo.unit = eeg_chs[k].unit;
            chInfo.unit_mul = eeg_chs[k].unit_mul;
            chInfo.ch_name = QString(eeg_chs[k].ch_name);

            t_pStream->write_ch_info(chInfo);
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
        file_bads  = t_pStreamBads->read_bad_channels(t_pStreamBads->tree());

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
        if (mne_write_named_matrix(t_pStream,FIFF_MNE_FORWARD_SOLUTION,meg_solution) == FIFF_FAIL)
            goto bad;
        if (meg_solution_grad)
            if (mne_write_named_matrix(t_pStream,FIFF_MNE_FORWARD_SOLUTION_GRAD,meg_solution_grad) == FIFF_FAIL)
                goto bad;

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
        if (mne_write_named_matrix(t_pStream,FIFF_MNE_FORWARD_SOLUTION,eeg_solution) == FIFF_FAIL)
            goto bad;
        if (eeg_solution_grad)
            if (mne_write_named_matrix(t_pStream,FIFF_MNE_FORWARD_SOLUTION_GRAD,eeg_solution_grad) == FIFF_FAIL)
                goto bad;

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

    qDebug() << "TODO fiff_put_dir";
//    if (fiff_put_dir(in->fd,in->dir) == FIFF_FAIL)
//        goto bad;
    if(t_pStreamIn)
        t_pStreamIn->close();

    return FIFF_OK;

bad : {
        if(t_pStream)
            t_pStream->close();
        if(t_pStreamIn)
            t_pStreamIn->close();
        return FIFF_FAIL;
    }
}




#define FIFFC_MAJOR_VERSION 1L
#define FIFFC_MINOR_VERSION 1L

#define FIFFC_VERSION (FIFFC_MAJOR_VERSION<<16 | FIFFC_MINOR_VERSION)


int fiff_get_machid(int *fixed_id)
{
    qDebug() << "TODO fiff_get_machid: resemble QNetworkInterface::hardwareAddress()";
    fixed_id[0] = 0;//QNetworkInterface::hardwareAddress(); //gethostid();
    fixed_id[1] = 0;

    return FIFF_OK;
}


int fiff_new_file_id (fiffId id)
/*
* Return a (hopefully) unique file id
*/
{
    int fixed_id[2];

    fixed_id[0] = fixed_id[1] = 0;
    if (fiff_get_machid(fixed_id) == FIFF_FAIL) {
        /*
        * Never mind...
        */
        fixed_id[0] = fixed_id[1] = 0;
    }
    /*
   * Internet address in the first two words
   */
    id->machid[0] = fixed_id[0];
    id->machid[1] = fixed_id[1];
    /*
   * Time in the third and fourth words
   */
    /*
   * Time in the third and fourth words
   * Since practically no system gives times in
   * true micro seconds, the last three digits
   * are randomized to insure uniqueness.
   */
    {
//        long secs,usecs;
//        if (fiff_get_time(&secs, &usecs) == FIFF_FAIL) //
//            return FIFF_FAIL;
        id->time.secs  = QDateTime::currentMSecsSinceEpoch()/1000;
        id->time.usecs = rand() % 1000;
    }
    id->version = FIFFC_VERSION;
    return FIFF_OK;
}




/*
 * Process the environment information
 */
#define MAX_WD 1024


int mne_attach_env(const QString& name, const QString& command)
/*
* Add the environment info for future reference
*/
{
    int  insert_blocks[]  = { FIFFB_MNE , FIFFB_MEAS, FIFFB_MRI, FIFFB_BEM, -1 };
    QString cwd = QDir::currentPath();
    fiffIdRec id;
    fiffFile  file = NULL;
    int       b,k, insert;
    fiffDirEntry ent;
    fiffTagRec   tag;
    fiffTag      tags = NULL,this_tag;
    int          ntag = 0;
    int          res = FIFF_FAIL;
    QFile t_file(name);

    tag.data = NULL;

    if (fiff_new_file_id(&id) == FIFF_FAIL)
        goto out;
//#ifdef DEBUG
//    fprintf(stderr,"\n");
//    fprintf(stderr,"cwd   = %s\n",cwd);
//    fprintf(stderr,"com   = %s\n",command);
//    fprintf(stderr,"envid = %s\n",mne_format_file_id(&id));
//#endif

    if (!t_file.exists()) {
        qCritical("File %s does not exist. Cannot attach env info.",name.toLatin1().constData());
        goto out;
    }
    if (!t_file.isWritable()) {
        qCritical("File %s is not writable. Cannot attach env info.",name.toLatin1().constData());
        goto out;
    }
    /*
    * Open the file to modify
    */
//    if ((file = fiff_open_update(name)) == NULL)
//        goto out;
//    /*
//   * Find an appropriate position to insert
//   */
//    for (insert = -1, b = 0; insert_blocks[b] >= 0; b++) {
//        for (ent = file->dir, k = 0; k < file->nent; k++, ent++) {
//            if (ent->kind == FIFF_BLOCK_START) {
//                if (fiff_read_this_tag (file->fd,ent->pos,&tag) == -1)
//                    goto out;
//                if (*(int *)tag.data == insert_blocks[b]) {
//                    insert = k;
//                    break;
//                }
//            }
//        }
//        if (insert >= 0)
//            break;
//    }
//    if (insert < 0) {
//        qCritical("Suitable place for environment insertion not found.");
//        goto out;
//    }
//    /*
//   * Build the list of tags to insert
//   */
//    ntag = 5;
//    tags = MALLOC(ntag,fiffTagRec);
//    for (k = 0; k < ntag; k++) {
//        tags[k].next = FIFFV_NEXT_SEQ;
//        tags[k].data = NULL;
//        tags[k].size = 0;
//    }
//    this_tag = tags;
//    this_tag->kind = FIFF_BLOCK_START;
//    this_tag->type = FIFFT_INT;
//    this_tag->size = sizeof(fiff_int_t);
//    this_tag->data = malloc(sizeof(fiff_int_t));
//    *(fiff_int_t *)this_tag->data = FIFFB_MNE_ENV;
//    this_tag++;

//    this_tag->kind = FIFF_BLOCK_ID;
//    this_tag->type = FIFFT_ID_STRUCT;
//    this_tag->size = sizeof(fiffIdRec);
//    this_tag->data = malloc(sizeof(fiffIdRec));
//    *(fiffId)this_tag->data = id;
//    this_tag++;

//    this_tag->kind = FIFF_MNE_ENV_WORKING_DIR;
//    this_tag->type = FIFFT_STRING;
//    this_tag->size = strlen(cwd);
//    this_tag->data = (fiff_byte_t *)strdup(cwd);
//    this_tag++;

//    this_tag->kind = FIFF_MNE_ENV_COMMAND_LINE;
//    this_tag->type = FIFFT_STRING;
//    this_tag->size = strlen(command);
//    this_tag->data = (fiff_byte_t *)strdup(command);
//    this_tag++;

//    this_tag->kind = FIFF_BLOCK_END;
//    this_tag->type = FIFFT_INT;
//    this_tag->size = sizeof(fiff_int_t);
//    this_tag->data = malloc(sizeof(fiff_int_t));
//    *(fiff_int_t *)this_tag->data = FIFFB_MNE_ENV;

//    if (fiff_insert_after (file,insert,tags,ntag) == FIFF_FAIL)
//        goto out;
    res = FIFF_OK;

out : {
//        for (k = 0; k < ntag; k++)
//            FREE_41(tags[k].data);
//        FREE_41(tags);
//        FREE_41(tag.data);
//        fiff_close(file);
//        FREE_41(cwd);
        return res;
    }
}


//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ComputeFwd::ComputeFwd(ComputeFwdSettings* p_settings)
    : settings(p_settings)
{
}


//*************************************************************************************************************

ComputeFwd::~ComputeFwd()
{
    //ToDo Garbage collection
}


//*************************************************************************************************************

void ComputeFwd::calculateFwd() const
{
    bool                res = false;
    MneSourceSpaceOld*   *spaces = NULL;  /* The source spaces */
    int                 nspace  = 0;
    int                 nsource = 0;     /* Number of source space points */

    FiffCoordTransOld* mri_head_t = NULL;   /* MRI <-> head coordinate transformation */
    FiffCoordTransOld* meg_head_t = NULL;   /* MEG <-> head coordinate transformation */

    fiffChInfo     megchs   = NULL; /* The MEG channel information */
    int            nmeg     = 0;
    fiffChInfo     eegchs   = NULL; /* The EEG channel information */
    int            neeg     = 0;
    fiffChInfo     compchs = NULL;
    int            ncomp    = 0;

    FwdCoilSet*             megcoils = NULL;     /* The coil descriptions */
    FwdCoilSet*             compcoils = NULL;    /* MEG compensation coils */
    MneCTFCompDataSet*      comp_data  = NULL;
    FwdCoilSet*             eegels = NULL;
    FwdEegSphereModelSet*   eeg_models = NULL;

    MneNamedMatrix* meg_forward      = NULL;    /* Result of the MEG forward calculation */
    MneNamedMatrix* eeg_forward      = NULL;    /* Result of the EEG forward calculation */
    MneNamedMatrix* meg_forward_grad = NULL;    /* Result of the MEG forward gradient calculation */
    MneNamedMatrix* eeg_forward_grad = NULL;    /* Result of the EEG forward gradient calculation */
    int            k;
    fiffId         mri_id  = NULL;
    fiffId         meas_id = NULL;
    FILE           *out = NULL;     /* Output filtered points here */

    FwdCoilSet*       templates = NULL;
    FwdEegSphereModel* eeg_model = NULL;
    FwdBemModel*       bem_model = NULL;

    QString qPath;
    QFile file;


    /*
    * Report the setup
    */
    //    printf("\n");
    //    mne_print_version_info(stderr,argv[0],PROGRAM_VERSION,__DATE__,__TIME__);
    printf("\n");
    printf("Source space                 : %s\n",settings->srcname.toLatin1().constData());
    if (!(settings->transname.isEmpty()) || !(settings->mriname.isEmpty()))
        printf("MRI -> head transform source : %s\n",!(settings->mriname.isEmpty()) ? settings->mriname.toLatin1().constData() : settings->transname.toLatin1().constData());
    else
        printf("MRI and head coordinates are assumed to be identical.\n");
    printf("Measurement data             : %s\n",settings->measname.toLatin1().constData());
    if (!settings->bemname.isEmpty())
        printf("BEM model                    : %s\n",settings->bemname.toLatin1().constData());
    else {
        printf("Sphere model                 : origin at (% 7.2f % 7.2f % 7.2f) mm\n",
               1000*settings->r0[X_41],1000*settings->r0[Y_41],1000*settings->r0[Z_41]);
        if (settings->include_eeg) {
            printf("\n");

            if (settings->eeg_model_file.isEmpty()) {
                qCritical("!!!!!!!!!!TODO: default_eeg_model_file();");
                //                settings->eeg_model_file = default_eeg_model_file();
            }
            eeg_models = FwdEegSphereModelSet::fwd_load_eeg_sphere_models(settings->eeg_model_file,eeg_models);
            eeg_models->fwd_list_eeg_sphere_models(stderr);

            if (settings->eeg_model_name.isEmpty())
                settings->eeg_model_name = QString("Default");
            if ((eeg_model = eeg_models->fwd_select_eeg_sphere_model(settings->eeg_model_name)) == NULL)
                goto out;

            if (!eeg_model->fwd_setup_eeg_sphere_model(settings->eeg_sphere_rad,settings->use_equiv_eeg,3))
                goto out;

            printf("Using EEG sphere model \"%s\" with scalp radius %7.1f mm\n",
                   settings->eeg_model_name.toLatin1().constData(),1000*settings->eeg_sphere_rad);
            printf("%s the electrode locations to scalp\n",settings->scale_eeg_pos ? "Scale" : "Do not scale");

            eeg_model->scale_pos = settings->scale_eeg_pos;
            VEC_COPY_41(eeg_model->r0,settings->r0);

            printf("\n");
        }
    }
    printf("%s field computations\n",settings->accurate ? "Accurate" : "Standard");
    printf("Do computations in %s coordinates.\n",FiffCoordTransOld::mne_coord_frame_name(settings->coord_frame));
    printf("%s source orientations\n",settings->fixed_ori ? "Fixed" : "Free");
    if (settings->compute_grad)
        printf("Compute derivatives with respect to source location coordinates\n");
    printf("Destination for the solution : %s\n",settings->solname.toLatin1().constData());
    if (settings->do_all)
        printf("Calculate solution for all source locations.\n");
    if (settings->nlabel > 0)
        printf("Source space will be restricted to sources in %d labels\n",settings->nlabel);
    /*
     * Read the source locations
     */
    printf("\n");
    printf("Reading %s...\n",settings->srcname.toLatin1().constData());
    if (MneSurfaceOrVolume::mne_read_source_spaces(settings->srcname,&spaces,&nspace) != OK)
        goto out;
    for (k = 0, nsource = 0; k < nspace; k++) {
        if (settings->do_all)
            MneSurfaceOrVolume::enable_all_sources(spaces[k]);
        nsource += spaces[k]->nuse;
    }
    if (nsource == 0) {
        qCritical("No sources are active in these source spaces. --all option should be used.");
        goto out;
    }
    printf("Read %d source spaces a total of %d active source locations\n", nspace,nsource);
    if (MneSurfaceOrVolume::restrict_sources_to_labels(spaces,nspace,settings->labels,settings->nlabel) == FAIL)
        goto out;
    /*
     * Read the MRI -> head coordinate transformation
     */
    printf("\n");
    if (!settings->mriname.isEmpty()) {
        if ((mri_head_t = FiffCoordTransOld::mne_read_mri_transform(settings->mriname)) == NULL)
            goto out;
        if ((mri_id = get_file_id(settings->mriname)) == NULL) {
            qCritical("Couln't read MRI file id (How come?)");
            goto out;
        }
    }
    else if (!settings->transname.isEmpty()) {
        FiffCoordTransOld* t;
        if ((t = FiffCoordTransOld::mne_read_FShead2mri_transform(settings->transname.toLatin1().data())) == NULL)
            goto out;
        mri_head_t = t->fiff_invert_transform();
        if(t)
            delete t;
    }
    else
        mri_head_t = FiffCoordTransOld::mne_identity_transform(FIFFV_COORD_MRI,FIFFV_COORD_HEAD);
    FiffCoordTransOld::mne_print_coord_transform(stderr,mri_head_t);
    /*
    * Read the channel information
    * and the MEG device -> head coordinate transformation
    */
    printf("\n");
    if (mne_read_meg_comp_eeg_ch_info_41(settings->measname,&megchs,&nmeg,&compchs,&ncomp,&eegchs,&neeg,&meg_head_t,&meas_id) != OK)
        goto out;
    if (nmeg > 0)
        printf("Read %3d MEG channels from %s\n",nmeg,settings->measname.toLatin1().constData());
    if (ncomp > 0)
        printf("Read %3d MEG compensation channels from %s\n",ncomp,settings->measname.toLatin1().constData());
    if (neeg > 0)
        printf("Read %3d EEG channels from %s\n",neeg,settings->measname.toLatin1().constData());
    if (!settings->include_meg) {
        printf("MEG not requested. MEG channels omitted.\n");
        FREE_41(megchs); megchs = NULL;
        FREE_41(compchs); compchs = NULL;
        nmeg = 0;
        ncomp = 0;
    }
    else
        FiffCoordTransOld::mne_print_coord_transform(stderr,meg_head_t);
    if (!settings->include_eeg) {
        printf("EEG not requested. EEG channels omitted.\n");
        FREE_41(eegchs); eegchs = NULL;
        neeg = 0;
    }
    else {
        if (mne_check_chinfo(eegchs,neeg) != OK)
            goto out;
    }
    /*
    * Create coil descriptions with transformation to head or MRI frame
    */
    if (settings->include_meg) {
        //#ifdef USE_SHARE_PATH
        //        char *coilfile = mne_compose_mne_name("share/mne","coil_def.dat");
        //#else
        //        char *coilfile = mne_compose_mne_name("setup/mne","coil_def.dat");

        qPath = QString("./resources/coilDefinitions/coil_def.dat");
        file.setFileName(qPath);
        if ( !QCoreApplication::startingUp() )
            qPath = QCoreApplication::applicationDirPath() + QString("/resources/coilDefinitions/coil_def.dat");
        else if (!file.exists())
            qPath = "./bin/resources/coilDefinitions/coil_def.dat";

        char *coilfile = MALLOC_41(strlen(qPath.toLatin1().data())+1,char);
        strcpy(coilfile,qPath.toLatin1().data());

        //#endif

        if (!coilfile)
            goto out;
        templates = FwdCoilSet::read_coil_defs(coilfile);
        if (!templates)
            goto out;
        FREE_41(coilfile);
        /*
        * Compensation data
        */
        if ((comp_data = MneCTFCompDataSet::mne_read_ctf_comp_data(settings->measname)) == NULL)
            goto out;
        if (comp_data->ncomp > 0) /* Compensation channel information may be needed */
            printf("%d compensation data sets in %s\n",comp_data->ncomp,settings->measname.toLatin1().constData());
        else {
            FREE_41(compchs); compchs = NULL;
            ncomp = 0;

            if(comp_data)
                delete comp_data;
            comp_data = NULL;
        }
    }
    if (settings->coord_frame == FIFFV_COORD_MRI) {
        FiffCoordTransOld* head_mri_t = mri_head_t->fiff_invert_transform();
        FiffCoordTransOld* meg_mri_t = FiffCoordTransOld::fiff_combine_transforms(FIFFV_COORD_DEVICE,FIFFV_COORD_MRI,meg_head_t,head_mri_t);
        if (meg_mri_t == NULL)
            goto out;
        if ((megcoils = templates->create_meg_coils(megchs,nmeg,
                                                    settings->accurate ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                    meg_mri_t)) == NULL)
            goto out;
        if (ncomp > 0) {
            if ((compcoils = templates->create_meg_coils(   compchs,ncomp,
                                                            FWD_COIL_ACCURACY_NORMAL,meg_mri_t)) == NULL)
                goto out;
        }
        if ((eegels = FwdCoilSet::create_eeg_els(eegchs,neeg,head_mri_t)) == NULL)
            goto out;
        FREE_41(head_mri_t);
        printf("MRI coordinate coil definitions created.\n");
    }
    else {
        if ((megcoils = templates->create_meg_coils(megchs,nmeg,
                                                    settings->accurate ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                    meg_head_t)) == NULL)
            goto out;
        if (ncomp > 0) {
            if ((compcoils = templates->create_meg_coils(compchs,ncomp,
                                                         FWD_COIL_ACCURACY_NORMAL,meg_head_t)) == NULL)
                goto out;
        }
        if ((eegels = FwdCoilSet::create_eeg_els(eegchs,neeg,NULL)) == NULL)
            goto out;
        printf("Head coordinate coil definitions created.\n");
    }
    /*
    * Transform the source spaces into the appropriate coordinates
    */
    if (MneSurfaceOrVolume::mne_transform_source_spaces_to(settings->coord_frame,mri_head_t,spaces,nspace) != OK)
        goto out;
    printf("Source spaces are now in %s coordinates.\n",FiffCoordTransOld::mne_coord_frame_name(settings->coord_frame));
    /*
    * Prepare the BEM model if necessary
    */
    if (!settings->bemname.isEmpty()) {
        char *bemsolname = FwdBemModel::fwd_bem_make_bem_sol_name(settings->bemname.toLatin1().data());
        //        FREE(bemname);
        settings->bemname = QString(bemsolname);

        printf("\nSetting up the BEM model using %s...\n",settings->bemname.toLatin1().constData());
        printf("\nLoading surfaces...\n");
        bem_model = FwdBemModel::fwd_bem_load_three_layer_surfaces(settings->bemname);
        if (bem_model) {
            printf("Three-layer model surfaces loaded.\n");
        }
        else {
            bem_model = FwdBemModel::fwd_bem_load_homog_surface(settings->bemname);
            if (!bem_model)
                goto out;
            printf("Homogeneous model surface loaded.\n");
        }
        if (neeg > 0 && bem_model->nsurf == 1) {
            qCritical("Cannot use a homogeneous model in EEG calculations.");
            goto out;
        }
        printf("\nLoading the solution matrix...\n");
        if (FwdBemModel::fwd_bem_load_recompute_solution(settings->bemname.toLatin1().data(),FWD_BEM_UNKNOWN,FALSE,bem_model) == FAIL)
            goto out;
        if (settings->coord_frame == FIFFV_COORD_HEAD) {
            printf("Employing the head->MRI coordinate transform with the BEM model.\n");
            if (FwdBemModel::fwd_bem_set_head_mri_t(bem_model,mri_head_t) == FAIL)
                goto out;
        }
        printf("BEM model %s is now set up\n",bem_model->sol_name.toLatin1().constData());
    }
    else
        printf("Using the sphere model.\n");
    printf ("\n");
    /*
    * Try to circumvent numerical problems by excluding points too close our ouside the inner skull surface
    */
    if (settings->filter_spaces) {
        if (!settings->mindistoutname.isEmpty()) {
            out = fopen(settings->mindistoutname.toLatin1().constData(),"w");
            if (out == NULL) {
                qCritical() << settings->mindistoutname;
                goto out;
            }
            printf("Omitted source space points will be output to : %s\n",settings->mindistoutname.toLatin1().constData());
        }
        if (MneSurfaceOrVolume::filter_source_spaces(settings->mindist,
                                                     settings->bemname.toLatin1().data(),
                                                     mri_head_t,
                                                     spaces,
                                                     nspace,out,settings->use_threads) == FAIL)
            goto out;
        if (out) {
            fclose(out);
            out = NULL;
        }
    }
    /*
    * Do the actual computation
    */
    if (!bem_model)
        settings->use_threads = false;
    if (nmeg > 0)
        if ((FwdBemModel::compute_forward_meg(spaces,nspace,megcoils,compcoils,comp_data,
                                              settings->fixed_ori,bem_model,&settings->r0,settings->use_threads,&meg_forward,
                                              settings->compute_grad ? &meg_forward_grad : NULL)) == FAIL)
            goto out;
    if (neeg > 0)
        if ((FwdBemModel::compute_forward_eeg(spaces,nspace,eegels,
                                              settings->fixed_ori,bem_model,eeg_model,settings->use_threads,&eeg_forward,
                                              settings->compute_grad ? &eeg_forward_grad : NULL)) == FAIL)
            goto out;
    /*
    * Transform the source spaces back into MRI coordinates
    */
    if (MneSourceSpaceOld::mne_transform_source_spaces_to(FIFFV_COORD_MRI,mri_head_t,spaces,nspace) != OK)
        goto out;
    /*
    * We are ready to spill it out
    */
    printf("\nwriting %s...",settings->solname.toLatin1().constData());
    if (write_solution(settings->solname,               /* Destination file */
                       spaces,                          /* The source spaces */
                       nspace,
                       settings->mriname,mri_id,        /* MRI file and data obtained from there */
                       mri_head_t,
                       settings->measname,meas_id,      /* MEG file and data obtained from there */
                       meg_head_t,
                       megchs, nmeg,
                       eegchs, neeg,
                       settings->fixed_ori,             /* Fixed orientation dipoles? */
                       settings->coord_frame,           /* Coordinate frame */
                       meg_forward, eeg_forward,
                       meg_forward_grad, eeg_forward_grad) == FIFF_FAIL)
        goto out;
    if (mne_attach_env(settings->solname,settings->command) == FIFF_FAIL)
        goto out;
    printf("done\n");
    res = true;
    printf("\nFinished.\n");

out : {
        //        if (out)
        //            fclose(out);
        for (k = 0; k < nspace; k++)
            if(spaces[k])
                delete spaces[k];
        if(mri_head_t)
            delete mri_head_t;
        if(meg_head_t)
            delete meg_head_t;
        FREE_41(megchs);
        FREE_41(eegchs);
        if(megcoils)
            delete megcoils;
        if(eegels)
            delete eegels;

        if(meg_forward)
            delete meg_forward;
        if(eeg_forward)
            delete eeg_forward;
        if(meg_forward_grad)
            delete meg_forward_grad;
        if(eeg_forward_grad)
            delete eeg_forward_grad;

        if (!res)
            qCritical("err_print_error();");//err_print_error();
    }
}

