
#include "fwd_types.h"


#include "dipole_fit_data.h"
#include "guess_data.h"
#include "mne_meas_data.h"
#include "mne_meas_data_set.h"
#include "mne_proj_item.h"
#include "ecd.h"

#include <Eigen/Dense>


#include <QFile>
#include <QCoreApplication>
#include <QDebug>



using namespace Eigen;
using namespace INVERSELIB;
using namespace FIFFLIB;



//============================= ctf_types.h =============================

#ifndef FIFFV_COIL_CTF_GRAD
#define FIFFV_COIL_CTF_GRAD           5001
#endif

#ifndef FIFFV_COIL_CTF_REF_MAG
#define FIFFV_COIL_CTF_REF_MAG        5002
#endif

#ifndef FIFFV_COIL_CTF_REF_GRAD
#define FIFFV_COIL_CTF_REF_GRAD       5003
#endif

#ifndef FIFFV_COIL_CTF_OFFDIAG_REF_GRAD
#define FIFFV_COIL_CTF_OFFDIAG_REF_GRAD 5004
#endif





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



#define X_3 0
#define Y_3 1
#define Z_3 2


#define MALLOC_3(x,t) (t *)malloc((x)*sizeof(t))
#define REALLOC_3(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))
#define FREE_3(x) if ((char *)(x) != NULL) free((char *)(x))

/*
 * Float, double, and int arrays
 */
#define ALLOC_FLOAT_3(x) MALLOC_3(x,float)




#define ALLOC_DCMATRIX_3(x,y) mne_dmatrix_3((x),(y))
#define ALLOC_CMATRIX_3(x,y) mne_cmatrix_3((x),(y))
#define FREE_CMATRIX_3(m) mne_free_cmatrix_3((m))
#define FREE_DCMATRIX_3(m) mne_free_dcmatrix_3((m))




static void matrix_error_3(int kind, int nr, int nc)

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



float **mne_cmatrix_3 (int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC_3(nr,float *);
    if (!m) matrix_error_3(1,nr,nc);
    whole = MALLOC_3(nr*nc,float);
    if (!whole) matrix_error_3(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}




double **mne_dmatrix_3(int nr, int nc)

{
    int i;
    double **m;
    double *whole;

    m = MALLOC_3(nr,double *);
    if (!m) matrix_error_3(1,nr,nc);
    whole = MALLOC_3(nr*nc,double);
    if (!whole) matrix_error_3(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}



void mne_free_dcmatrix_3 (double **m)

{
    if (m) {
        FREE_3(*m);
        FREE_3(m);
    }
}


/*
 * Dot product and length
 */
#define VEC_DOT_3(x,y) ((x)[X_3]*(y)[X_3] + (x)[Y_3]*(y)[Y_3] + (x)[Z_3]*(y)[Z_3])
#define VEC_LEN_3(x) sqrt(VEC_DOT_3(x,x))


/*
 * Others...
 */

#define VEC_DIFF_3(from,to,diff) {\
    (diff)[X_3] = (to)[X_3] - (from)[X_3];\
    (diff)[Y_3] = (to)[Y_3] - (from)[Y_3];\
    (diff)[Z_3] = (to)[Z_3] - (from)[Z_3];\
    }

#define VEC_COPY_3(to,from) {\
    (to)[X_3] = (from)[X_3];\
    (to)[Y_3] = (from)[Y_3];\
    (to)[Z_3] = (from)[Z_3];\
    }

#define CROSS_PRODUCT_3(x,y,xy) {\
    (xy)[X_3] =   (x)[Y_3]*(y)[Z_3]-(y)[Y_3]*(x)[Z_3];\
    (xy)[Y_3] = -((x)[X_3]*(y)[Z_3]-(y)[X_3]*(x)[Z_3]);\
    (xy)[Z_3] =   (x)[X_3]*(y)[Y_3]-(y)[X_3]*(x)[Y_3];\
    }







//============================= mne_matop.c =============================




#define MIN_3(a,b) ((a) < (b) ? (a) : (b))




void mne_transpose_square_3(float **mat, int n)
/*
      * In-place transpose of a square matrix
      */
{
    int j,k;
    float val;

    for (j = 1; j < n; j++)
        for (k = 0; k < j; k++) {
            val = mat[j][k];
            mat[j][k] = mat[k][j];
            mat[k][j] = val;
        }
    return;
}



float mne_dot_vectors_3(float *v1,
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



void mne_add_scaled_vector_to_3(float *v1,float scale, float *v2,int nn)

{
#ifdef BLAS
    float fscale = scale;
    int   one = 1;
    saxpy(&nn,&fscale,v1,&one,v2,&one);
#else
    int k;
    for (k = 0; k < nn; k++)
        v2[k] = v2[k] + scale*v1[k];
#endif
    return;
}







double **mne_dmatt_dmat_mult2_3 (double **m1,double **m2, int d1,int d2,int d3)
/* Matrix multiplication
      * result(d1 x d3) = m1(d2 x d1)^T * m2(d2 x d3) */

{
    double **result = ALLOC_DCMATRIX_3(d1,d3);
#ifdef BLAS
    char  *transa = "N";
    char  *transb = "T";
    double zero = 0.0;
    double one  = 1.0;

    dgemm (transa,transb,&d3,&d1,&d2,
           &one,m2[0],&d3,m1[0],&d1,&zero,result[0],&d3);

    return result;
#else
    int j,k,p;
    double sum;

    for (j = 0; j < d1; j++)
        for (k = 0; k < d3; k++) {
            sum = 0.0;
            for (p = 0; p < d2; p++)
                sum = sum + m1[p][j]*m2[p][k];
            result[j][k] = sum;
        }
    return result;
#endif
}







float **mne_mat_mat_mult_3 (float **m1,float **m2,int d1,int d2,int d3)
/* Matrix multiplication
      * result(d1 x d3) = m1(d1 x d2) * m2(d2 x d3) */

{
#ifdef BLAS
    float **result = ALLOC_CMATRIX_3(d1,d3);
    char  *transa = "N";
    char  *transb = "N";
    float zero = 0.0;
    float one  = 1.0;
    sgemm (transa,transb,&d3,&d1,&d2,
           &one,m2[0],&d3,m1[0],&d2,&zero,result[0],&d3);
    return (result);
#else
    float **result = ALLOC_CMATRIX_3(d1,d3);
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





void mne_mat_vec_mult2_3(float **m,float *v,float *result, int d1,int d2)
/*
      * Matrix multiplication
      * result(d1) = m(d1 x d2) * v(d2)
      */

{
    int j;

    for (j = 0; j < d1; j++)
        result[j] = mne_dot_vectors_3(m[j],v,d2);
    return;
}


//============================= misc_util.c =============================

char *mne_strdup_3(const char *s)
{
    char *res;
    if (s == NULL)
        return NULL;
    res = (char*) malloc(strlen(s)+1);
    strcpy(res,s);
    return res;
}



//============================= mne_named_matrix.c =============================














char **mne_dup_name_list_3(char **list, int nlist)
/*
 * Duplicate a name list
 */
{
    char **res;
    int  k;
    if (list == NULL || nlist == 0)
        return NULL;
    res = MALLOC_3(nlist,char *);

    for (k = 0; k < nlist; k++)
        res[k] = mne_strdup_3(list[k]);
    return res;
}



char *mne_name_list_to_string_3(char **list,int nlist)
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
    res = MALLOC_3(len,char);
    res[0] = '\0';
    for (k = len = 0; k < nlist-1; k++) {
        strcat(res,list[k]);
        strcat(res,":");
    }
    strcat(res,list[nlist-1]);
    return res;
}


char *mne_channel_names_to_string_3(fiffChInfo chs, int nch)
/*
* Make a colon-separated string out of channel names
*/
{
    char **names = MALLOC_3(nch,char *);
    char *res;
    int  k;

    if (nch <= 0)
        return NULL;
    for (k = 0; k < nch; k++)
        names[k] = chs[k].ch_name;
    res = mne_name_list_to_string_3(names,nch);
    FREE_3(names);
    return res;
}


void mne_string_to_name_list_3(char *s,char ***listp,int *nlistp)
/*
      * Convert a colon-separated list into a string array
      */
{
    char **list = NULL;
    int  nlist  = 0;
    char *one,*now=NULL;

    if (s != NULL && strlen(s) > 0) {
        s = mne_strdup_3(s);
        //strtok_r linux variant; strtok_s windows varainat
#ifdef __linux__
        for (one = strtok_r(s,":",&now); one != NULL; one = strtok_r(NULL,":",&now)) {
#elif _WIN32
        for (one = strtok_s(s,":",&now); one != NULL; one = strtok_s(NULL,":",&now)) {
#else
        for (one = strtok_r(s,":",&now); one != NULL; one = strtok_r(NULL,":",&now)) {
#endif
            list = REALLOC_3(list,nlist+1,char *);
            list[nlist++] = mne_strdup_3(one);
        }
        FREE_3(s);
    }
    *listp  = list;
    *nlistp = nlist;
    return;
}














//============================= mne_sparse_matop.c =============================






INVERSELIB::FiffSparseMatrix* mne_convert_to_sparse_3(float **dense,        /* The dense matrix to be converted */
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
    INVERSELIB::FiffSparseMatrix* sparse = NULL;
    int size;

    if (small < 0) {		/* Automatic scaling */
        float maxval = 0.0;
        float val;

        for (j = 0; j < nrow; j++)
            for (k = 0; k < ncol; k++) {
                val = fabs(dense[j][k]);
                if (val > maxval)
                    maxval = val;
            }
        if (maxval > 0)
            small = maxval*fabs(small);
        else
            small = fabs(small);
    }
    for (j = 0, nz = 0; j < nrow; j++)
        for (k = 0; k < ncol; k++) {
            if (fabs(dense[j][k]) > small)
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
    sparse = new INVERSELIB::FiffSparseMatrix;
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
                if (fabs(dense[j][k]) > small) {
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
                if (fabs(dense[j][k]) > small) {
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




int  mne_sparse_vec_mult2_3(INVERSELIB::FiffSparseMatrix* mat,     /* The sparse matrix */
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






















typedef struct {
    float          limit;
    int            report_dim;
    float          *B;
    double         B2;
    DipoleForward*  fwd;
} *fitDipUser,fitDipUserRec;





int mne_is_diag_cov_3(mneCovMatrix c)

{
    return c->cov_diag != NULL;
}






void mne_scale_vector_3 (double scale,float *v,int   nn)

{
#ifdef BLAS
    float  fscale = scale;
    int    one    = 1;
    sscal(&nn,&fscale,v,&one);
#else
    int k;
    for (k = 0; k < nn; k++)
        v[k] = v[k]*scale;
#endif
}









//float
Eigen::MatrixXf toFloatEigenMatrix_3(float **mat, const int m, const int n)
{
    Eigen::MatrixXf eigen_mat(m,n);

    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            eigen_mat(i,j) = mat[i][j];

    return eigen_mat;
}


void fromFloatEigenMatrix_3(const Eigen::MatrixXf& from_mat, float **& to_mat, const int m, const int n)
{
    for ( int i = 0; i < m; ++i)
        for ( int j = 0; j < n; ++j)
            to_mat[i][j] = from_mat(i,j);
}

void fromFloatEigenMatrix_3(const Eigen::MatrixXf& from_mat, float **& to_mat)
{
    fromFloatEigenMatrix_3(from_mat, to_mat, from_mat.rows(), from_mat.cols());
}





void fromFloatEigenVector_3(const Eigen::VectorXf& from_vec, float *to_vec, const int n)
{
    for ( int i = 0; i < n; ++i)
        to_vec[i] = from_vec[i];
}





float **mne_lu_invert_3(float **mat,int dim)
/*
      * Invert a matrix using the LU decomposition from
      * LAPACK
      */
{
    Eigen::MatrixXf eigen_mat = toFloatEigenMatrix_3(mat, dim, dim);
    Eigen::MatrixXf eigen_mat_inv = eigen_mat.inverse();
    fromFloatEigenMatrix_3(eigen_mat_inv, mat);
    return mat;
}













void mne_free_cmatrix_3 (float **m)
{
    if (m) {
        FREE_3(*m);
        FREE_3(m);
    }
}






int mne_svd_3(float **mat,	/* The matrix */
            int   m,int n,	/* m rows n columns */
            float *sing,	/* Singular values (must have size
                             * MIN(m,n)+1 */
            float **uu,		/* Left eigenvectors */
            float **vv)		/* Right eigenvectors */
/*
      * Compute the SVD of mat.
      * The singular vector calculations depend on whether
      * or not u and v are given.
      *
      * The allocations should be done as follows
      *
      * mat = ALLOC_CMATRIX_3(m,n);
      * vv  = ALLOC_CMATRIX_3(MIN(m,n),n);
      * uu  = ALLOC_CMATRIX_3(MIN(m,n),m);
      * sing = MALLOC_3(MIN(m,n),float);
      *
      * mat is modified by this operation
      *
      * This simply allocates the workspace and calls the
      * LAPACK Fortran routine
      */

{
    int    udim = MIN_3(m,n);

    Eigen::MatrixXf eigen_mat = toFloatEigenMatrix_3(mat, m, n);

    //ToDo Optimize computation depending of whether uu or vv are defined
    Eigen::JacobiSVD< Eigen::MatrixXf > svd(eigen_mat ,Eigen::ComputeFullU | Eigen::ComputeFullV);

    fromFloatEigenVector_3(svd.singularValues(), sing, svd.singularValues().size());

    if (uu != NULL)
        fromFloatEigenMatrix_3(svd.matrixU().transpose(), uu, udim, m);

    if (vv != NULL)
        fromFloatEigenMatrix_3(svd.matrixV().transpose(), vv, m, n);

    return 0;
    //  return info;
}







void mne_free_name_list_3(char **list, int nlist)
/*
* Free a name list array
*/
{
    int k;
    if (list == NULL || nlist == 0)
        return;
    for (k = 0; k < nlist; k++) {
#ifdef FOO
        fprintf(stderr,"%d %s\n",k,list[k]);
#endif
        FREE_3(list[k]);
    }
    FREE_3(list);
    return;
}







void mne_free_proj_op_proj_3(mneProjOp op)

{
    if (op == NULL)
        return;

    mne_free_name_list_3(op->names,op->nch);
    FREE_CMATRIX_3(op->proj_data);

    op->names  = NULL;
    op->nch  = 0;
    op->nvec = 0;
    op->proj_data = NULL;

    return;
}



void mne_free_proj_op_3(mneProjOp op)

{
    int k;

    if (op == NULL)
        return;

    for (k = 0; k < op->nitems; k++)
        if(op->items[k])
            delete op->items[k];
//    FREE_3(op->items);

    mne_free_proj_op_proj_3(op);

    FREE_3(op);
    return;
}





void mne_free_cov_3(mneCovMatrix c)
/*
* Free a covariance matrix and all its data
*/
{
    if (c == NULL)
        return;
    FREE_3(c->cov);
    FREE_3(c->cov_diag);
    if(c->cov_sparse)
        delete c->cov_sparse;
    mne_free_name_list_3(c->names,c->ncov);
    FREE_CMATRIX_3(c->eigen);
    FREE_3(c->lambda);
    FREE_3(c->inv_lambda);
    FREE_3(c->chol);
    FREE_3(c->ch_class);
    mne_free_proj_op_3(c->proj);
    if (c->sss)
        delete c->sss;
    mne_free_name_list_3(c->bads,c->nbad);
    FREE_3(c);
    return;
}










#define EPS_3 0.05

int mne_get_values_from_data_3 (float time,         /* Interesting time point */
                              float integ,	  /* Time integration */
                              float **data,	  /* The data values (time by time) */
                              int   nsamp,	  /* How many time points? */
                              int   nch,          /* How many channels */
                              float tmin,	  /* Time of first sample */
                              float sfreq,	  /* Sampling frequency */
                              int   use_abs,      /* Use absolute values */
                              float *value)	  /* The picked values */
/*
      * Pick a signal value using linear interpolation
      */
{
    int   n1,n2,k;
    float s1,s2;
    float f1,f2;
    float sum;
    float width;
    int   ch;

    for (ch = 0; ch < nch; ch++) {
        /*
     * Find out the correct samples
     */
        if (fabs(sfreq*integ) < EPS_3) { /* This is the single-sample case */
            s1 = sfreq*(time - tmin);
            n1 = floor(s1);
            f1 = 1.0 + n1 - s1;
            if (n1 < 0 || n1 > nsamp-1) {
                printf("Sample value out of range %d (0..%d)",n1,nsamp-1);
                return(-1);
            }
            /*
       * Avoid rounding error
       */
            if (n1 == nsamp-1) {
                if (fabs(f1-1.0) < 1e-3)
                    f1 = 1.0;
            }
            if (f1 < 1.0 && n1 > nsamp-2) {
                printf("Sample value out of range %d (0..%d) %.4f",n1,nsamp-1,f1);
                return(-1);
            }
            if (f1 < 1.0) {
                if (use_abs)
                    sum = f1*fabs(data[n1][ch]) + (1.0-f1)*fabs(data[n1+1][ch]);
                else
                    sum = f1*data[n1][ch] + (1.0-f1)*data[n1+1][ch];
            }
            else {
                if (use_abs)
                    sum = fabs(data[n1][ch]);
                else
                    sum = data[n1][ch];
            }
        }
        else {			/* Multiple samples */
            s1 = sfreq*(time - 0.5*integ - tmin);
            s2 = sfreq*(time + 0.5*integ - tmin);
            n1 = ceil(s1); n2 = floor(s2);
            if (n2 < n1) {		/* We are within one sample interval */
                n1 = floor(s1);
                if (n1 < 0 || n1 > nsamp-2)
                    return (-1);
                f1 = s1 - n1;
                f2 = s2 - n1;
                if (use_abs)
                    sum = 0.5*((f1+f2)*fabs(data[n1+1][ch]) + (2.0-f1-f2)*fabs(data[n1][ch]));
                else
                    sum = 0.5*((f1+f2)*data[n1+1][ch] + (2.0-f1-f2)*data[n1][ch]);
            }
            else {
                f1 = n1 - s1;
                f2 = s2 - n2;
                if (n1 < 0 || n1 > nsamp-1) {
                    printf("Sample value out of range %d (0..%d)",n1,nsamp-1);
                    return(-1);
                }
                if (n2 < 0 || n2 > nsamp-1) {
                    printf("Sample value out of range %d (0..%d)",n2,nsamp-1);
                    return(-1);
                }
                if (f1 != 0.0 && n1 < 1)
                    return(-1);
                if (f2 != 0.0 && n2 > nsamp-2)
                    return(-1);
                sum = 0.0;
                width = 0.0;
                if (n2 > n1) {		/* Do the whole intervals */
                    if (use_abs) {
                        sum = 0.5*fabs(data[n1][ch]);
                        for (k = n1+1; k < n2; k++)
                            sum = sum + fabs(data[k][ch]);
                        sum = sum + 0.5*fabs(data[n2][ch]);
                    }
                    else {
                        sum = 0.5*data[n1][ch];
                        for (k = n1+1; k < n2; k++)
                            sum = sum + data[k][ch];
                        sum = sum + 0.5*data[n2][ch];
                    }
                    width = n2 - n1;
                }
                /*
         * Add tails
         */
                if (use_abs) {
                    if (f1 != 0.0)
                        sum = sum + 0.5*f1*(f1*fabs(data[n1-1][ch]) + (2.0-f1)*fabs(data[n1][ch]));
                    if (f2 != 0.0)
                        sum = sum + 0.5*f2*(f2*fabs(data[n2+1][ch]) + (2.0-f2)*fabs(data[n2][ch]));
                }
                else {
                    if (f1 != 0.0)
                        sum = sum + 0.5*f1*(f1*data[n1-1][ch] + (2.0-f1)*data[n1][ch]);
                    if (f2 != 0.0)
                        sum = sum + 0.5*f2*(f2*data[n2+1][ch] + (2.0-f2)*data[n2][ch]);
                }
                width = width + f1 + f2;
                sum = sum/width;
            }
        }
        value[ch] = sum;
    }
    return (0);
}















int mne_proj_op_proj_vector_3(mneProjOp op, float *vec, int nvec, int do_complement)
/*
      * Apply projection operator to a vector (floats)
      * Assume that all dimension checking etc. has been done before
      */
{
    static float *res = NULL;
    int    res_size   = 0;
    float *pvec;
    float  w;
    int k,p;

    if (!op || op->nitems <= 0 || op->nvec <= 0)
        return OK;

    if (op->nch != nvec) {
        printf("Data vector size does not match projection operator");
        return FAIL;
    }

    if (op->nch > res_size) {
        res = REALLOC_3(res,op->nch,float);
        res_size = op->nch;
    }

    for (k = 0; k < op->nch; k++)
        res[k] = 0.0;

    for (p = 0; p < op->nvec; p++) {
        pvec = op->proj_data[p];
        w = mne_dot_vectors_3(pvec,vec,op->nch);
        for (k = 0; k < op->nch; k++)
            res[k] = res[k] + w*pvec[k];
    }
    if (do_complement) {
        for (k = 0; k < op->nch; k++)
            vec[k] = vec[k] - res[k];
    }
    else {
        for (k = 0; k < op->nch; k++)
            vec[k] = res[k];
    }
    return OK;
}






//============================= mne_decompose.c =============================


int mne_decompose_eigen_3(double *mat,
                         double *lambda,
                         float  **vectors, /* Eigenvectors fit into floats easily */
                         int    dim)
/*
      * Compute the eigenvalue decomposition of
      * a symmetric matrix using the LAPACK routines
      *
      * 'mat' contains the lower triangle of the matrix
      */
{
    int    np  =   dim*(dim+1)/2;
    double *w    = MALLOC_3(dim,double);
    double *z    = MALLOC_3(dim*dim,double);
    double *work = MALLOC_3(3*dim,double);
    double *dmat = MALLOC_3(np,double);
    float  *vecp = vectors[0];

    const char   *uplo  = "U";
    const char   *compz = "V";
    int    info,k;
    int    one = 1;
    int    maxi;
    double scale;

//    maxi = idamax(&np,mat,&one);
// idamax workaround begin
    maxi = 0;
    for(int i = 0; i < np; ++i)
        if (fabs(mat[i]) > fabs(mat[maxi]))
            maxi = i;
// idamax workaround end

    scale = 1.0/mat[maxi];//scale = 1.0/mat[maxi-1];

    for (k = 0; k < np; k++)
        dmat[k] = mat[k]*scale;
//    dspev(compz,uplo,&dim,dmat,w,z,&dim,work,&info);


// dspev workaround begin
    MatrixXd dmat_tmp = MatrixXd::Zero(dim,dim);
    int idx = 0;
    for (int i = 0; i < dim; ++i) {
        for(int j = 0; j <= i; ++j) {
            dmat_tmp(i,j) = dmat[idx];
            dmat_tmp(j,i) = dmat[idx];
            ++idx;
        }
    }
    SelfAdjointEigenSolver<MatrixXd> es;
    es.compute(dmat_tmp);
    for ( int i = 0; i < dim; ++i )
        w[i] = es.eigenvalues()[i];

    idx = 0;
    for ( int j = 0; j < dim; ++j ) {
        for( int i = 0; i < dim; ++i ) {
            z[idx] = es.eigenvectors()(i,j);// Column Major
            ++idx;
        }
    }
// dspev workaround end

    info = 0;

    qDebug() << "!!!DEBUG ToDo: dspev(compz,uplo,&dim,dmat,w,z,&dim,work,&info);";

    FREE_3(work);
    if (info != 0)
        printf("Eigenvalue decomposition failed (LAPACK info = %d)",info);
    else {
        scale = 1.0/scale;
        for (k = 0; k < dim; k++)
            lambda[k] = scale*w[k];
        for (k = 0; k < dim*dim; k++)
            vecp[k] = z[k];
    }
    FREE_3(w);
    FREE_3(z);
    if (info == 0)
        return 0;
    else
        return -1;
}





//============================= mne_cov_matrix.c =============================






/*
* Routines for handling the covariance matrices
*/
static mneCovMatrix new_cov_3(int    kind,
                            int    ncov,
                            char   **names,
                            double *cov,
                            double *cov_diag,
                            INVERSELIB::FiffSparseMatrix* cov_sparse)
/*
* Put it together from ingredients
*/
{
    mneCovMatrix new_cov    = MALLOC_3(1,mneCovMatrixRec);
    new_cov->kind           = kind;
    new_cov->ncov           = ncov;
    new_cov->nproj          = 0;
    new_cov->nzero          = 0;
    new_cov->names          = names;
    new_cov->cov            = cov;
    new_cov->cov_diag       = cov_diag;
    new_cov->cov_sparse     = cov_sparse;
    new_cov->eigen          = NULL;
    new_cov->lambda         = NULL;
    new_cov->chol           = NULL;
    new_cov->inv_lambda     = NULL;
    new_cov->nfree          = 1;
    new_cov->ch_class       = NULL;
    new_cov->proj           = NULL;
    new_cov->sss            = NULL;
    new_cov->bads           = NULL;
    new_cov->nbad           = 0;
    return new_cov;
}

















mneCovMatrix mne_new_cov_dense(int    kind,
                               int    ncov,
                               char   **names,
                               double *cov)

{
    return new_cov_3(kind,ncov,names,cov,NULL,NULL);
}


mneCovMatrix mne_new_cov_diag(int    kind,
                              int    ncov,
                              char   **names,
                              double *cov_diag)

{
    return new_cov_3(kind,ncov,names,NULL,cov_diag,NULL);
}


mneCovMatrix mne_new_cov_sparse(int             kind,
                                int             ncov,
                                char            **names,
                                INVERSELIB::FiffSparseMatrix* cov_sparse)
{
    return new_cov_3(kind,ncov,names,NULL,NULL,cov_sparse);
}









mneCovMatrix mne_new_cov_3(int    kind,
                         int    ncov,
                         char   **names,
                         double *cov,
                         double *cov_diag)

{
    return new_cov_3(kind,ncov,names,cov,cov_diag,NULL);
}



static int mne_lt_packed_index_3(int j, int k)

{
    if (j >= k)
        return k + j*(j+1)/2;
    else
        return j + k*(k+1)/2;
}






/*
* Handle the linear projection operators
*/
mneProjOp mne_new_proj_op_3()

{
    mneProjOp new_proj_op = MALLOC_3(1,mneProjOpRec);

//    new_proj_op->items     = NULL;
    new_proj_op->nitems    = 0;
    new_proj_op->names     = NULL;
    new_proj_op->nch       = 0;
    new_proj_op->nvec      = 0;
    new_proj_op->proj_data = NULL;
    return new_proj_op;
}




void mne_proj_op_add_item_act_3(mneProjOp op, MneNamedMatrix* vecs, int kind, const char *desc, int is_active)
/*
* Add a new item to an existing projection operator
*/
{
    MneProjItem* new_item;
    int         k;

//    op->items = REALLOC_3(op->items,op->nitems+1,mneProjItem);

//    op->items[op->nitems] = new_item = new MneProjItem();
    new_item = new MneProjItem();
    op->items.append(new_item);

    new_item->active      = is_active;
    new_item->vecs        = new MneNamedMatrix(*vecs);

    if (kind == FIFFV_MNE_PROJ_ITEM_EEG_AVREF) {
        new_item->has_meg = FALSE;
        new_item->has_eeg = TRUE;
    }
    else {
        for (k = 0; k < vecs->ncol; k++) {
            if (strstr(vecs->collist[k],"EEG") == vecs->collist[k])
                new_item->has_eeg = TRUE;
            if (strstr(vecs->collist[k],"MEG") == vecs->collist[k])
                new_item->has_meg = TRUE;
        }
        if (!new_item->has_meg && !new_item->has_eeg) {
            new_item->has_meg = TRUE;
            new_item->has_eeg = FALSE;
        }
        else if (new_item->has_meg && new_item->has_eeg) {
            new_item->has_meg = TRUE;
            new_item->has_eeg = FALSE;
        }
    }
    if (desc != NULL)
        new_item->desc = mne_strdup_3(desc);
    new_item->kind = kind;
    new_item->nvec = new_item->vecs->nrow;

    op->nitems++;

    mne_free_proj_op_proj_3(op);  /* These data are not valid any more */
    return;
}


void mne_proj_op_add_item_3(mneProjOp op, MneNamedMatrix* vecs, int kind, const char *desc)

{
    mne_proj_op_add_item_act_3(op, vecs, kind, desc, TRUE);
}






mneProjOp mne_dup_proj_op_3(mneProjOp op)
/*
* Provide a duplicate (item data only)
*/
{
    mneProjOp dup = mne_new_proj_op_3();
    MneProjItem* it;
    int k;

    if (!op)
        return NULL;

    for (k = 0; k < op->nitems; k++) {
        it = op->items[k];
        mne_proj_op_add_item_act_3(dup,it->vecs,it->kind,it->desc,it->active);
        dup->items[k]->active_file = it->active_file;
    }
    return dup;
}








mneCovMatrix mne_dup_cov_3(mneCovMatrix c)
{
    double       *vals;
    int          nval;
    int          k;
    mneCovMatrix res;

    if (c->cov_diag)
        nval = c->ncov;
    else
        nval = (c->ncov*(c->ncov+1))/2;

    vals = MALLOC_3(nval,double);
    if (c->cov_diag) {
        for (k = 0; k < nval; k++)
            vals[k] = c->cov_diag[k];
        res = mne_new_cov_3(c->kind,c->ncov,mne_dup_name_list_3(c->names,c->ncov),NULL,vals);
    }
    else {
        for (k = 0; k < nval; k++)
            vals[k] = c->cov[k];
        res = mne_new_cov_3(c->kind,c->ncov,mne_dup_name_list_3(c->names,c->ncov),vals,NULL);
    }
    /*
    * Duplicate additional items
    */
    if (c->ch_class) {
        res->ch_class = MALLOC_3(c->ncov,int);
        for (k = 0; k < c->ncov; k++)
            res->ch_class[k] = c->ch_class[k];
    }
    res->bads = mne_dup_name_list_3(c->bads,c->nbad);
    res->nbad = c->nbad;
    res->proj = mne_dup_proj_op_3(c->proj);
    res->sss  = new MneSssData(*(c->sss));

    return res;
}



int mne_add_inv_cov_3(mneCovMatrix c)
/*
      * Calculate the inverse square roots for whitening
      */
{
    double *src = c->lambda ? c->lambda : c->cov_diag;
    int k;

    if (src == NULL) {
        qCritical("Covariance matrix is not diagonal or not decomposed.");
        return FAIL;
    }
    c->inv_lambda = REALLOC_3(c->inv_lambda,c->ncov,double);
    for (k = 0; k < c->ncov; k++) {
        if (src[k] <= 0.0)
            c->inv_lambda[k] = 0.0;
        else
            c->inv_lambda[k] = 1.0/sqrt(src[k]);
    }
    return OK;
}













static int condition_cov_3(mneCovMatrix c, float rank_threshold, int use_rank)

{
    double *scale  = NULL;
    double *cov    = NULL;
    double *lambda = NULL;
    float  **eigen = NULL;
    double **data1 = NULL;
    double **data2 = NULL;
    double magscale,gradscale,eegscale;
    int    nmag,ngrad,neeg,nok;
    int    j,k;
    int    res = FAIL;

    if (c->cov_diag)
        return OK;
    if (!c->ch_class) {
        qCritical("Channels not classified. Rank cannot be determined.");
        return FAIL;
    }
    magscale = gradscale = eegscale = 0.0;
    nmag = ngrad = neeg = 0;
    for (k = 0; k < c->ncov; k++) {
        if (c->ch_class[k] == MNE_COV_CH_MEG_MAG) {
            magscale += c->cov[mne_lt_packed_index_3(k,k)]; nmag++;
        }
        else if (c->ch_class[k] == MNE_COV_CH_MEG_GRAD) {
            gradscale += c->cov[mne_lt_packed_index_3(k,k)]; ngrad++;
        }
        else if (c->ch_class[k] == MNE_COV_CH_EEG) {
            eegscale += c->cov[mne_lt_packed_index_3(k,k)]; neeg++;
        }
#ifdef DEBUG
        fprintf(stdout,"%d ",c->ch_class[k]);
#endif
    }
#ifdef DEBUG
    fprintf(stdout,"\n");
#endif
    if (nmag > 0)
        magscale = magscale > 0.0 ? sqrt(nmag/magscale) : 0.0;
    if (ngrad > 0)
        gradscale = gradscale > 0.0 ? sqrt(ngrad/gradscale) : 0.0;
    if (neeg > 0)
        eegscale = eegscale > 0.0 ? sqrt(neeg/eegscale) : 0.0;
#ifdef DEBUG
    fprintf(stdout,"%d %g\n",nmag,magscale);
    fprintf(stdout,"%d %g\n",ngrad,gradscale);
    fprintf(stdout,"%d %g\n",neeg,eegscale);
#endif
    scale = MALLOC_3(c->ncov,double);
    for (k = 0; k < c->ncov; k++) {
        if (c->ch_class[k] == MNE_COV_CH_MEG_MAG)
            scale[k] = magscale;
        else if (c->ch_class[k] == MNE_COV_CH_MEG_GRAD)
            scale[k] = gradscale;
        else if (c->ch_class[k] == MNE_COV_CH_EEG)
            scale[k] = eegscale;
        else
            scale[k] = 1.0;
    }
    cov    = MALLOC_3(c->ncov*(c->ncov+1)/2.0,double);
    lambda = MALLOC_3(c->ncov,double);
    eigen  = ALLOC_CMATRIX_3(c->ncov,c->ncov);
    for (j = 0; j < c->ncov; j++)
        for (k = 0; k <= j; k++)
            cov[mne_lt_packed_index_3(j,k)] = c->cov[mne_lt_packed_index_3(j,k)]*scale[j]*scale[k];
    if (mne_decompose_eigen_3(cov,lambda,eigen,c->ncov) == 0) {
#ifdef DEBUG
        for (k = 0; k < c->ncov; k++)
            fprintf(stdout,"%g ",lambda[k]/lambda[c->ncov-1]);
        fprintf(stdout,"\n");
#endif
        nok = 0;
        for (k = c->ncov-1; k >= 0; k--) {
            if (lambda[k] >= rank_threshold*lambda[c->ncov-1])
                nok++;
            else
                break;
        }
        printf("\n\tEstimated covariance matrix rank = %d (%g)\n",nok,lambda[c->ncov-nok]/lambda[c->ncov-1]);
        if (use_rank > 0 && use_rank < nok) {
            nok = use_rank;
            fprintf(stderr,"\tUser-selected covariance matrix rank = %d (%g)\n",nok,lambda[c->ncov-nok]/lambda[c->ncov-1]);
        }
        /*
     * Put it back together
     */
        for (j = 0; j < c->ncov-nok; j++)
            lambda[j] = 0.0;
        data1 = ALLOC_DCMATRIX_3(c->ncov,c->ncov);
        for (j = 0; j < c->ncov; j++) {
#ifdef DEBUG
            mne_print_vector(stdout,NULL,eigen[j],c->ncov);
#endif
            for (k = 0; k < c->ncov; k++)
                data1[j][k] = sqrt(lambda[j])*eigen[j][k];
        }
        data2 = mne_dmatt_dmat_mult2_3(data1,data1,c->ncov,c->ncov,c->ncov);
#ifdef DEBUG
        printf(">>>\n");
        for (j = 0; j < c->ncov; j++)
            mne_print_dvector(stdout,NULL,data2[j],c->ncov);
        printf(">>>\n");
#endif
        /*
     * Scale back
     */
        for (k = 0; k < c->ncov; k++)
            if (scale[k] > 0.0)
                scale[k] = 1.0/scale[k];
        for (j = 0; j < c->ncov; j++)
            for (k = 0; k <= j; k++)
                if (c->cov[mne_lt_packed_index_3(j,k)] != 0.0)
                    c->cov[mne_lt_packed_index_3(j,k)] = scale[j]*scale[k]*data2[j][k];
        res = nok;
    }
    FREE_3(cov);
    FREE_3(lambda);
    FREE_CMATRIX_3(eigen);
    FREE_DCMATRIX_3(data1);
    FREE_DCMATRIX_3(data2);
    return res;
}













static int check_cov_data(double *vals, int nval)

{
    int    k;
    double sum;

    for (k = 0, sum = 0.0; k < nval; k++)
        sum += vals[k];
    if (sum == 0.0) {
        qCritical("Sum of covariance matrix elements is zero!");
        return FAIL;
    }
    return OK;
}














int mne_classify_channels_cov(mneCovMatrix cov, fiffChInfo chs, int nchan)
/*
 * Assign channel classes in a covariance matrix with help of channel infos
 */
{
    int k,p;
    fiffChInfo ch;

    if (!chs) {
        qCritical("Channel information not available in mne_classify_channels_cov");
        goto bad;
    }
    cov->ch_class = REALLOC_3(cov->ch_class,cov->ncov,int);
    for (k = 0; k < cov->ncov; k++) {
        cov->ch_class[k] = MNE_COV_CH_UNKNOWN;
        for (p = 0, ch = NULL; p < nchan; p++) {
            if (strcmp(chs[p].ch_name,cov->names[k]) == 0) {
                ch = chs+p;
                if (ch->kind == FIFFV_MEG_CH) {
                    if (ch->unit == FIFF_UNIT_T)
                        cov->ch_class[k] = MNE_COV_CH_MEG_MAG;
                    else
                        cov->ch_class[k] = MNE_COV_CH_MEG_GRAD;
                }
                else if (ch->kind == FIFFV_EEG_CH)
                    cov->ch_class[k] = MNE_COV_CH_EEG;
                break;
            }
        }
        if (!ch) {
            printf("Could not find channel info for channel %s in mne_classify_channels_cov",cov->names[k]);
            goto bad;
        }
    }
    return OK;

bad : {
        FREE_3(cov->ch_class);
        cov->ch_class = NULL;
        return FAIL;
    }
}







static int mne_decompose_eigen_cov_small_3(mneCovMatrix c,float small, int use_rank)
/*
      * Do the eigenvalue decomposition
      */
{
    int   np,k,p,rank;
    float rank_threshold = 1e-6;

    if (small < 0)
        small = 1.0;

    if (!c)
        return OK;
    if (c->cov_diag)
        return mne_add_inv_cov_3(c);
    if (c->lambda && c->eigen) {
        fprintf(stderr,"\n\tEigenvalue decomposition had been precomputed.\n");
        c->nzero = 0;
        for (k = 0; k < c->ncov; k++, c->nzero++)
            if (c->lambda[k] > 0)
                break;
    }
    else {
        FREE_3(c->lambda); c->lambda = NULL;
        FREE_CMATRIX_3(c->eigen); c->eigen = NULL;

        if ((rank = condition_cov_3(c,rank_threshold,use_rank)) < 0)
            return FAIL;

        np = c->ncov*(c->ncov+1)/2;
        c->lambda = MALLOC_3(c->ncov,double);
        c->eigen  = ALLOC_CMATRIX_3(c->ncov,c->ncov);
        if (mne_decompose_eigen_3(c->cov,c->lambda,c->eigen,c->ncov) != 0)
            goto bad;
        c->nzero = c->ncov - rank;
        for (k = 0; k < c->nzero; k++)
            c->lambda[k] = 0.0;
        /*
     * Find which eigenvectors correspond to EEG/MEG
     */
        {
            float meglike,eeglike;
            int   nmeg,neeg;

            nmeg = neeg = 0;
            for (k = c->nzero; k < c->ncov; k++) {
                meglike = eeglike = 0.0;
                for (p = 0; p < c->ncov; p++)  {
                    if (c->ch_class[p] == MNE_COV_CH_EEG)
                        eeglike += fabs(c->eigen[k][p]);
                    else if (c->ch_class[p] == MNE_COV_CH_MEG_MAG || c->ch_class[p] == MNE_COV_CH_MEG_GRAD)
                        meglike += fabs(c->eigen[k][p]);
                }
                if (meglike > eeglike)
                    nmeg++;
                else
                    neeg++;
            }
            printf("\t%d MEG and %d EEG-like channels remain in the whitened data\n",nmeg,neeg);
        }
    }
    return mne_add_inv_cov_3(c);

bad : {
        FREE_3(c->lambda); c->lambda = NULL;
        FREE_CMATRIX_3(c->eigen); c->eigen = NULL;
        return FAIL;
    }
}


int mne_decompose_eigen_cov_3(mneCovMatrix c)

{
    return mne_decompose_eigen_cov_small_3(c,-1.0,-1);
}






//============================= mne_whiten.c =============================

int mne_whiten_data(float **data, float **whitened_data, int np, int nchan, mneCovMatrix C)
/*
      * Apply the whitening operation
      */
{
    int    j,k;
    float  *one = NULL,*orig,*white;
    double *inv;

    if (data == NULL || np <= 0)
        return OK;

    if (C->ncov != nchan) {
        printf("Incompatible covariance matrix. Cannot whiten the data.");
        return FAIL;
    }
    inv = C->inv_lambda;
    if (mne_is_diag_cov_3(C)) {
        //    printf("<DEBUG> Performing Diag\n");
        for (j = 0; j < np; j++) {
            orig = data[j];
            white = whitened_data[j];
            for (k = 0; k < nchan; k++)
                white[k] = orig[k]*inv[k];
        }
    }
    else {
        /*
     * This is arranged so that whitened_data can be the same matrix as the original
     */
        one = MALLOC_3(nchan,float);
        for (j = 0; j < np; j++) {
            orig = data[j];
            white = whitened_data[j];
            for (k = C->nzero; k < nchan; k++)
                one[k] = mne_dot_vectors_3(C->eigen[k],orig,nchan);
            for (k = 0; k < C->nzero; k++)
                white[k] = 0.0;
            for (k = C->nzero; k < nchan; k++)
                white[k] = one[k]*inv[k];
        }
        FREE_3(one);
    }
    return OK;
}


int mne_whiten_one_data(float *data, float *whitened_data, int nchan, mneCovMatrix C)

{
    float *datap[1];
    float *whitened_datap[1];

    datap[0] = data;
    whitened_datap[0] = whitened_data;

    return mne_whiten_data(datap,whitened_datap,1,nchan,C);
}












//*************************************************************************************************************
//============================= dipole_fit_setup.c =============================
static void free_dipole_fit_funcs(dipoleFitFuncs f)

{
    if (!f)
        return;

    if (f->meg_client_free && f->meg_client)
        f->meg_client_free(f->meg_client);
    if (f->eeg_client_free && f->eeg_client)
        f->eeg_client_free(f->eeg_client);

    FREE_3(f);
    return;
}







//static void regularize_cov(mneCovMatrix c,       /* The matrix to regularize */
//                           float        *regs,   /* Regularization values to apply (fractions of the
//                                                     * average diagonal values for each class */
//                           int          *active) /* Which of the channels are 'active' */
///*
//      * Regularize different parts of the noise covariance matrix differently
//      */
//{
//    int    j;
//    float  sums[3],nn[3];
//    int    nkind = 3;

//    if (!c->cov || !c->ch_class)
//        return;

//    for (j = 0; j < nkind; j++) {
//        sums[j] = 0.0;
//        nn[j]   = 0;
//    }
//    /*
//   * Compute the averages over the diagonal elements for each class
//   */
//    for (j = 0; j < c->ncov; j++) {
//        if (c->ch_class[j] >= 0) {
//            if (!active || active[j]) {
//                sums[c->ch_class[j]] += c->cov[mne_lt_packed_index_3(j,j)];
//                nn[c->ch_class[j]]++;
//            }
//        }
//    }
//    printf("Average noise-covariance matrix diagonals:\n");
//    for (j = 0; j < nkind; j++) {
//        if (nn[j] > 0) {
//            sums[j] = sums[j]/nn[j];
//            if (j == MNE_COV_CH_MEG_MAG)
//                printf("\tMagnetometers       : %-7.2f fT    reg = %-6.2f\n",1e15*sqrt(sums[j]),regs[j]);
//            else if (j == MNE_COV_CH_MEG_GRAD)
//                printf("\tPlanar gradiometers : %-7.2f fT/cm reg = %-6.2f\n",1e13*sqrt(sums[j]),regs[j]);
//            else
//                printf("\tEEG                 : %-7.2f uV    reg = %-6.2f\n",1e6*sqrt(sums[j]),regs[j]);
//            sums[j] = regs[j]*sums[j];
//        }
//    }
//    /*
//   * Add thee proper amount to the diagonal
//   */
//    for (j = 0; j < c->ncov; j++)
//        if (c->ch_class[j] >= 0)
//            c->cov[mne_lt_packed_index_3(j,j)] += sums[c->ch_class[j]];

//    printf("Noise-covariance regularized as requested.\n");
//    return;
//}


void mne_regularize_cov(mneCovMatrix c,       /* The matrix to regularize */
                        float        *regs)   /* Regularization values to apply (fractions of the
                           * average diagonal values for each class */
/*
 * Regularize different parts of the noise covariance matrix differently
 */
{
    int    j;
    float  sums[3],nn[3];
    int    nkind = 3;

    if (!c->cov || !c->ch_class)
        return;

    for (j = 0; j < nkind; j++) {
        sums[j] = 0.0;
        nn[j]   = 0;
    }
    /*
   * Compute the averages over the diagonal elements for each class
   */
    for (j = 0; j < c->ncov; j++) {
        if (c->ch_class[j] >= 0) {
            sums[c->ch_class[j]] += c->cov[mne_lt_packed_index_3(j,j)];
            nn[c->ch_class[j]]++;
        }
    }
    fprintf(stderr,"Average noise-covariance matrix diagonals:\n");
    for (j = 0; j < nkind; j++) {
        if (nn[j] > 0) {
            sums[j] = sums[j]/nn[j];
            if (j == MNE_COV_CH_MEG_MAG)
                fprintf(stderr,"\tMagnetometers       : %-7.2f fT    reg = %-6.2f\n",1e15*sqrt(sums[j]),regs[j]);
            else if (j == MNE_COV_CH_MEG_GRAD)
                fprintf(stderr,"\tPlanar gradiometers : %-7.2f fT/cm reg = %-6.2f\n",1e13*sqrt(sums[j]),regs[j]);
            else
                fprintf(stderr,"\tEEG                 : %-7.2f uV    reg = %-6.2f\n",1e6*sqrt(sums[j]),regs[j]);
            sums[j] = regs[j]*sums[j];
        }
    }
    /*
   * Add thee proper amount to the diagonal
   */
    for (j = 0; j < c->ncov; j++)
        if (c->ch_class[j] >= 0)
            c->cov[mne_lt_packed_index_3(j,j)] += sums[c->ch_class[j]];

    fprintf(stderr,"Noise-covariance regularized as requested.\n");
    return;
}




//============================= dipole_fit_setup.c =============================


static dipoleFitFuncs new_dipole_fit_funcs()

{
    dipoleFitFuncs f = MALLOC_3(1,dipoleFitFuncsRec);

    f->meg_field     = NULL;
    f->eeg_pot       = NULL;
    f->meg_vec_field = NULL;
    f->eeg_vec_pot   = NULL;
    f->meg_client      = NULL;
    f->meg_client_free = NULL;
    f->eeg_client      = NULL;
    f->eeg_client_free = NULL;

    return f;
}






















//============================= fwd_bem_model.c =============================

static struct {
    int  kind;
    const char *name;
} surf_expl[] = { { FIFFV_BEM_SURF_ID_BRAIN , "inner skull" },
{ FIFFV_BEM_SURF_ID_SKULL , "outer skull" },
{ FIFFV_BEM_SURF_ID_HEAD  , "scalp" },
{ -1                      , "unknown" } };


const char *fwd_bem_explain_surface(int kind)

{
    int k;

    for (k = 0; surf_expl[k].kind >= 0; k++)
        if (surf_expl[k].kind == kind)
            return surf_expl[k].name;

    return surf_expl[k].name;
}



static struct {
    int  method;
    const char *name;
} method_expl[] = { { FWD_BEM_CONSTANT_COLL , "constant collocation" },
{ FWD_BEM_LINEAR_COLL   , "linear collocation" },
{ -1                    , "unknown" } };


const char *fwd_bem_explain_method(int method)

{
    int k;

    for (k = 0; method_expl[k].method >= 0; k++)
        if (method_expl[k].method == method)
            return method_expl[k].name;

    return method_expl[k].name;
}










fwdBemModel fwd_bem_new_model()

{
    fwdBemModel m = MALLOC_3(1,fwdBemModelRec);

    m->surf_name   = NULL;
    m->surfs       = NULL;
    m->nsurf       = 0;
    m->ntri        = NULL;
    m->np          = NULL;
    m->sigma       = NULL;
    m->gamma       = NULL;
    m->source_mult = NULL;
    m->field_mult  = NULL;
    m->bem_method  = FWD_BEM_UNKNOWN;
    m->head_mri_t  = NULL;
    m->sol_name    = NULL;
    m->solution    = NULL;
    m->nsol        = 0;
    m->v0          = NULL;
    m->use_ip_approach = FALSE;
    m->ip_approach_limit = FWD_BEM_IP_APPROACH_LIMIT;
    return m;
}




void fwd_bem_free_solution_3(fwdBemModel m)

{
    if (!m)
        return;
    FREE_CMATRIX_3(m->solution); m->solution = NULL;
    FREE_3(m->sol_name); m->sol_name = NULL;
    FREE_3(m->v0); m->v0 = NULL;
    m->bem_method = FWD_BEM_UNKNOWN;
    m->nsol       = 0;

    return;
}






void fwd_bem_free_coil_solution(void *user)

{
    fwdBemSolution sol = (fwdBemSolution)user;

    if (!sol)
        return;
    FREE_CMATRIX_3(sol->solution);
    FREE_3(sol);
    return;
}

fwdBemSolution fwd_bem_new_coil_solution()

{
    fwdBemSolution sol = MALLOC_3(1,fwdBemSolutionRec);

    sol->solution = NULL;
    sol->ncoil    = 0;
    sol->np       = 0;

    return sol;
}





void fwd_bem_free_model(fwdBemModel m)

{
    int k;

    if (!m)
        return;

    FREE_3(m->surf_name);
    for (k = 0; k < m->nsurf; k++)
        delete m->surfs[k];
    FREE_3(m->surfs);
    FREE_3(m->ntri);
    FREE_3(m->np);
    FREE_3(m->sigma);
    FREE_3(m->source_mult);
    FREE_3(m->field_mult);
    FREE_CMATRIX_3(m->gamma);
    if(m->head_mri_t)
        delete m->head_mri_t;
    fwd_bem_free_solution_3(m);

    FREE_3(m);
    return;
}




MneSurfaceOrVolume::MneCSurface* fwd_bem_find_surface(fwdBemModel model, int kind)
/*
 * Return a pointer to a specific surface in a BEM
 */
{
    int k;
    if (!model) {
        printf("No model specified for fwd_bem_find_surface");
        return NULL;
    }
    for (k = 0; k < model->nsurf; k++)
        if (model->surfs[k]->id == kind)
            return model->surfs[k];
    printf("Desired surface (%d = %s) not found.",
           kind,fwd_bem_explain_surface(kind));
    return NULL;
}

fwdBemModel fwd_bem_load_surfaces(char *name,
                                  int  *kinds,
                                  int  nkind)
/*
 * Load a set of surfaces
 */
{
    MneSurfaceOrVolume::MneCSurface* *surfs = NULL;
    float      *sigma = NULL;
    float      *sigma1;
    fwdBemModel m = NULL;
    int         j,k;

    if (nkind <= 0) {
        printf("No surfaces specified to fwd_bem_load_surfaces");
        return NULL;
    }

    surfs = MALLOC_3(nkind,MneSurfaceOrVolume::MneCSurface*);
    sigma = MALLOC_3(nkind,float);
    for (k = 0; k < nkind; k++)
        surfs[k] = NULL;

    for (k = 0; k < nkind; k++) {
        if ((surfs[k] = MneSurfaceOrVolume::MneCSurface::read_bem_surface(name,kinds[k],TRUE,sigma+k)) == NULL)
            goto bad;
        if (sigma[k] < 0.0) {
            printf("No conductivity available for surface %s",fwd_bem_explain_surface(kinds[k]));
            goto bad;
        }
        if (surfs[k]->coord_frame != FIFFV_COORD_MRI) { /* We make our life much easier with this */
            printf("Surface %s not specified in MRI coordinates.",fwd_bem_explain_surface(kinds[k]));
            goto bad;
        }
    }
    m = fwd_bem_new_model();

    m->surf_name = mne_strdup_3(name);
    m->nsurf     = nkind;
    m->surfs     = surfs;
    m->sigma     = sigma;
    m->ntri      = MALLOC_3(nkind,int);
    m->np        = MALLOC_3(nkind,int);
    m->gamma = ALLOC_CMATRIX_3(nkind,nkind);
    m->source_mult = MALLOC_3(nkind,float);
    m->field_mult  = MALLOC_3(nkind,float);
    /*
   * Dirty trick for the zero conductivity outside
   */
    sigma1 = MALLOC_3(nkind+1,float);
    sigma1[0] = 0.0;
    sigma  = sigma1+1;
    for (k = 0; k < m->nsurf; k++)
        sigma[k] = m->sigma[k];
    /*
   * Gamma factors and multipliers
   */
    for (j = 0; j < m->nsurf; j++) {
        m->ntri[j] = m->surfs[j]->ntri;
        m->np[j]   = m->surfs[j]->np;
        m->source_mult[j] = 2.0/(sigma[j]+sigma[j-1]);
        m->field_mult[j] = sigma[j]-sigma[j-1];
        for (k = 0; k < m->nsurf; k++)
            m->gamma[j][k] = (sigma[k]-sigma[k-1])/(sigma[j]+sigma[j-1]);
    }
    FREE_3(sigma1);

    return m;

bad : {
        FREE_3(sigma);
        for (k = 0; k < nkind; k++)
            delete surfs[k];
        FREE_3(surfs);
        return NULL;
    }
}

fwdBemModel fwd_bem_load_homog_surface(char *name)
/*
 * Load surfaces for the homogeneous model
 */
{
    int kinds[] = { FIFFV_BEM_SURF_ID_BRAIN };
    int nkind   = 1;

    return fwd_bem_load_surfaces(name,kinds,nkind);
}

fwdBemModel fwd_bem_load_three_layer_surfaces(char *name)
/*
 * Load surfaces for three-layer model
 */
{
    int kinds[] = { FIFFV_BEM_SURF_ID_HEAD, FIFFV_BEM_SURF_ID_SKULL, FIFFV_BEM_SURF_ID_BRAIN };
    int nkind   = 3;

    return fwd_bem_load_surfaces(name,kinds,nkind);
}

static int get_int( FiffStream::SPtr& stream, const FiffDirNode::SPtr& node,int what,int *res)
/*
* Wrapper to get int's
*/
{
    FiffTag::SPtr t_pTag;
    if(node->find_tag(stream, what, t_pTag)) {
        if (t_pTag->getType() != FIFFT_INT) {
            printf("Expected an integer tag : %d (found data type %d instead)\n",what,t_pTag->getType() );
            return FAIL;
        }
        *res = *t_pTag->toInt();
        return OK;
    }
    return FAIL;
}


int fwd_bem_load_solution(char *name, int bem_method, fwdBemModel m)
/*
* Load the potential solution matrix and attach it to the model:
*
* return values:
*
*       TRUE   found a suitable solution
*       FALSE  did not find a suitable solution
*       FAIL   error in reading the solution
*
*/
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    float       **sol = NULL;
    FiffDirNode::SPtr bem_node;
    int         method;
    FiffTag::SPtr t_pTag;
    int         nsol;

    if(!stream->open())
        goto not_found;

    /*
   * Find the BEM data
   */
    {
        QList<FiffDirNode::SPtr> nodes = stream->tree()->dir_tree_find(FIFFB_BEM);

        if (nodes.size() == 0) {
            printf ("No BEM data in %s",name);
            goto not_found;
        }
        bem_node = nodes[0];
    }
    /*
    * Approximation method
    */
    if (get_int(stream,bem_node,FIFF_BEM_APPROX,&method) != OK)
        goto not_found;
    if (method == FIFFV_BEM_APPROX_CONST)
        method = FWD_BEM_CONSTANT_COLL;
    else if (method == FIFFV_BEM_APPROX_LINEAR)
        method = FWD_BEM_LINEAR_COLL;
    else {
        printf ("Cannot handle BEM approximation method : %d",method);
        goto bad;
    }
    if (bem_method != FWD_BEM_UNKNOWN && method != bem_method) {
        printf("Approximation method in file : %d desired : %d",method,bem_method);
        goto not_found;
    }
    {
        int         dim,k;

        if (!bem_node->find_tag(stream, FIFF_BEM_POT_SOLUTION, t_pTag))
            goto bad;
        qint32 ndim;
        QVector<qint32> dims;
        t_pTag->getMatrixDimensions(ndim, dims);

        if (ndim != 2) {
            printf("Expected a two-dimensional solution matrix instead of a %d dimensional one",ndim);
            goto bad;
        }
        for (k = 0, dim = 0; k < m->nsurf; k++)
            dim = dim + ((method == FWD_BEM_LINEAR_COLL) ? m->surfs[k]->np : m->surfs[k]->ntri);
        if (dims[0] != dim || dims[1] != dim) {
            printf("Expected a %d x %d solution matrix instead of a %d x %d  one",dim,dim,dims[0],dims[1]);
            goto not_found;
        }

        MatrixXf tmp_sol = t_pTag->toFloatMatrix().transpose();
        sol = ALLOC_CMATRIX_3(tmp_sol.rows(),tmp_sol.cols());
        fromFloatEigenMatrix_3(tmp_sol, sol);
        nsol = dims[1];
    }
    fwd_bem_free_solution_3(m);
    m->sol_name = mne_strdup_3(name);
    m->solution = sol;
    m->nsol     = nsol;
    m->bem_method = method;
    stream->close();

    return TRUE;

bad : {
        stream->close();
        FREE_CMATRIX_3(sol);
        return FAIL;
    }

not_found : {
        stream->close();
        FREE_CMATRIX_3(sol);
        return FALSE;
    }
}




int fwd_bem_set_head_mri_t(fwdBemModel m, FiffCoordTransOld* t)
/*
* Set the coordinate transformation
*/
{
    if (t->from == FIFFV_COORD_HEAD && t->to == FIFFV_COORD_MRI) {
        if(m->head_mri_t)
            delete m->head_mri_t;
        m->head_mri_t = new FiffCoordTransOld( *t );
        return OK;
    }
    else if (t->from == FIFFV_COORD_MRI && t->to == FIFFV_COORD_HEAD) {
        if(m->head_mri_t)
            delete m->head_mri_t;
        m->head_mri_t = t->fiff_invert_transform();
        return OK;
    }
    else {
        printf ("Improper coordinate transform delivered to fwd_bem_set_head_mri_t");
        return FAIL;
    }
}

/*
* Some filename utilities follow
*/
static char *ends_with(char *s, char *suffix)
/*
* Does a string end with the given suffix?
*/
{
    char *p;

    if (!s)
        return NULL;

    for (p = strstr(s,suffix); p ; s = p + strlen(suffix), p = strstr(s,suffix))
        if (p == s + strlen(s) - strlen(suffix))
            return p;
    return NULL;
}

static char *strip_from(char *s, char *suffix)

{
    char *p = ends_with(s,suffix);
    char c;
    char *res;

    if (p) {
        c = *p;
        *p = '\0';
        res = mne_strdup_3(s);
        *p = c;
    }
    else
        res = mne_strdup_3(s);
    return res;
}

#define BEM_SUFFIX     "-bem.fif"
#define BEM_SOL_SUFFIX "-bem-sol.fif"

//char *fwd_bem_make_bem_name(char *name)
///*
// * Make a standard BEM file name
// */
//{
//    char *s1,*s2;

//    s1 = strip_from(name,(char*)(".fif"));
//    s2 = strip_from(s1,(char*)("-sol"));
//    FREE_3(s1);
//    s1 = strip_from(s2,(char*)("-bem"));
//    FREE_3(s2);
//    s2 = MALLOC_3(strlen(s1)+strlen(BEM_SUFFIX)+1,char);
//    sprintf(s2,"%s%s",s1,BEM_SUFFIX);
//    FREE_3(s1);
//    return s2;
//}

char *fwd_bem_make_bem_sol_name(char *name)
/*
 * Make a standard BEM solution file name
 */
{
    char *s1,*s2;

    s1 = strip_from(name,(char*)(".fif"));
    s2 = strip_from(s1,(char*)("-sol"));
    FREE_3(s1);
    s1 = strip_from(s2,(char*)("-bem"));
    FREE_3(s2);
    s2 = MALLOC_3(strlen(s1)+strlen(BEM_SOL_SUFFIX)+1,char);
    sprintf(s2,"%s%s",s1,BEM_SOL_SUFFIX);
    FREE_3(s1);
    return s2;
}



























//============================= fwd_bem_solution.c =============================

float **fwd_bem_multi_solution (float **solids,    /* The solid-angle matrix */
                                float **gamma,     /* The conductivity multipliers */
                                int   nsurf,       /* Number of surfaces */
                                int   *ntri)       /* Number of triangles or nodes on each surface */
/*
      * Invert I - solids/(2*M_PI)
      * Take deflation into account
      * The matrix is destroyed after inversion
      * This is the general multilayer case
      */
{
    int j,k,p,q;
    float defl;
    float pi2 = 1.0/(2*M_PI);
    float mult;
    int   joff,koff,jup,kup,ntot;

    for (j = 0,ntot = 0; j < nsurf; j++)
        ntot += ntri[j];
    defl = 1.0/ntot;
    /*
   * Modify the matrix
   */
    for (p = 0, joff = 0; p < nsurf; p++) {
        jup = ntri[p] + joff;
        for (q = 0, koff = 0; q < nsurf; q++) {
            kup = ntri[q] + koff;
            mult = (gamma == NULL) ? pi2 : pi2*gamma[p][q];
            for (j = joff; j < jup; j++)
                for (k = koff; k < kup; k++)
                    solids[j][k] = defl - solids[j][k]*mult;
            koff = kup;
        }
        joff = jup;
    }
    for (k = 0; k < ntot; k++)
        solids[k][k] = solids[k][k] + 1.0;

    return (mne_lu_invert_3(solids,ntot));
}



float **fwd_bem_homog_solution (float **solids,int ntri)
/*
      * Invert I - solids/(2*M_PI)
      * Take deflation into account
      * The matrix is destroyed after inversion
      * This is the homogeneous model case
      */
{
    return fwd_bem_multi_solution (solids,NULL,1,&ntri);
}


void fwd_bem_ip_modify_solution(float **solution,           /* The original solution */
                                float **ip_solution,	    /* The isolated problem solution */
                                float ip_mult,		    /* Conductivity ratio */
                                int nsurf,		    /* Number of surfaces */
                                int *ntri)		    /* Number of triangles (nodes) on each surface */
/*
      * Modify the solution according to the IP approach
      */
{
    int s;
    int j,k,joff,koff,ntot,nlast;
    float mult;
    float *row = NULL;
    float **sub = NULL;

    for (s = 0, koff = 0; s < nsurf-1; s++)
        koff = koff + ntri[s];
    nlast = ntri[nsurf-1];
    ntot  = koff + nlast;

    row = MALLOC_3(nlast,float);
    sub = MALLOC_3(ntot,float *);
    mult = (1.0 + ip_mult)/ip_mult;

    fprintf(stderr,"\t\tCombining...");
#ifndef OLD
    fprintf(stderr,"t ");
    mne_transpose_square_3(ip_solution,nlast);
#endif
    for (s = 0, joff = 0; s < nsurf; s++) {
        fprintf(stderr,"%d3 ",s+1);
        /*
     * Pick the correct submatrix
     */
        for (j = 0; j < ntri[s]; j++)
            sub[j] = solution[j+joff]+koff;
        /*
     * Multiply
     */
#ifdef OLD
        for (j = 0; j < ntri[s]; j++) {
            for (k = 0; k < nlast; k++) {
                res = mne_dot_vectors_skip_skip(sub[j],1,ip_solution[0]+k,nlast,nlast);
                row[k] = sub[j][k] - 2.0*res;
            }
            for (k = 0; k < nlast; k++)
                sub[j][k] = row[k];
        }
#else
        for (j = 0; j < ntri[s]; j++) {
            for (k = 0; k < nlast; k++)
                row[k] = mne_dot_vectors_3(sub[j],ip_solution[k],nlast);
            mne_add_scaled_vector_to_3(row,-2.0,sub[j],nlast);
        }
#endif
        joff = joff+ntri[s];
    }
#ifndef OLD
    fprintf(stderr,"t ");
    mne_transpose_square_3(ip_solution,nlast);
#endif
    fprintf(stderr,"33 ");
    /*
   * The lower right corner is a special case
   */
    for (j = 0; j < nlast; j++)
        for (k = 0; k < nlast; k++)
            sub[j][k] = sub[j][k] + mult*ip_solution[j][k];
    /*
   * Final scaling
   */
    fprintf(stderr,"done.\n\t\tScaling...");
    mne_scale_vector_3(ip_mult,solution[0],ntot*ntot);
    fprintf(stderr,"done.\n");
    FREE_3(row); FREE_3(sub);
    return;
}



















//============================= fwd_bem_linear_collocation.c =============================







/*
 * The following approach is based on:
 *
 * de Munck JC: "A linear discretization of the volume conductor boundary integral equation using analytically integrated elements",
 * IEEE Trans Biomed Eng. 1992 39(9) : 986 - 990
 *
 */

static double calc_beta (double *rk,double *rk1)

{
    double rkk1[3];
    double size;
    double res;

    VEC_DIFF_3(rk,rk1,rkk1);
    size = VEC_LEN_3(rkk1);

    res = log((VEC_LEN_3(rk)*size + VEC_DOT_3(rk,rkk1))/
              (VEC_LEN_3(rk1)*size + VEC_DOT_3(rk1,rkk1)))/size;
    return (res);
}

static void lin_pot_coeff (float  *from,	/* Origin */
                           mneTriangle to,	/* The destination triangle */
                           double omega[3])	/* The final result */
/*
      * The linear potential matrix element computations
      */
{
    double y1[3],y2[3],y3[3];	/* Corners with origin at from */
    double *y[5];
    double **yy;
    double l1,l2,l3;		/* Lengths of y1, y2, and y3 */
    double solid;			/* The standard solid angle */
    double vec_omega[3];		/* The cross-product integral */
    double cross[3];		/* y1 x y2 */
    double triple;		/* VEC_DOT_3(y1 x y2,y3) */
    double ss;
    double beta[3],bbeta[3];
    int   j,k;
    double z[3];
    double n2,area2;
    double diff[3];
    static const double solid_eps = 4.0*M_PI/1.0E6;
    /*
   * This circularity makes things easy for us...
   */
    y[0] = y3;
    y[1] = y1;
    y[2] = y2;
    y[3] = y3;
    y[4] = y1;
    yy = y + 1;			/* yy can have index -1! */
    /*
   * The standard solid angle computation
   */
    VEC_DIFF_3(from,to->r1,y1);
    VEC_DIFF_3(from,to->r2,y2);
    VEC_DIFF_3(from,to->r3,y3);

    CROSS_PRODUCT_3(y1,y2,cross);
    triple = VEC_DOT_3(cross,y3);

    l1 = VEC_LEN_3(y1);
    l2 = VEC_LEN_3(y2);
    l3 = VEC_LEN_3(y3);
    ss = (l1*l2*l3+VEC_DOT_3(y1,y2)*l3+VEC_DOT_3(y1,y3)*l2+VEC_DOT_3(y2,y3)*l1);
    solid  = 2.0*atan2(triple,ss);
    if (fabs(solid) < solid_eps) {
        for (k = 0; k < 3; k++)
            omega[k] = 0.0;
    }
    else {
        /*
     * Calculate the magic vector vec_omega
     */
        for (j = 0; j < 3; j++)
            beta[j] = calc_beta(yy[j],yy[j+1]);
        bbeta[0] = beta[2] - beta[0];
        bbeta[1] = beta[0] - beta[1];
        bbeta[2] = beta[1] - beta[2];

        for (j = 0; j < 3; j++)
            vec_omega[j] = 0.0;
        for (j = 0; j < 3; j++)
            for (k = 0; k < 3; k++)
                vec_omega[k] = vec_omega[k] + bbeta[j]*yy[j][k];
        /*
     * Put it all together...
     */
        area2 = 2.0*to->area;
        n2 = 1.0/(area2*area2);
        for (k = 0; k < 3; k++) {
            CROSS_PRODUCT_3(yy[k+1],yy[k-1],z);
            VEC_DIFF_3(yy[k+1],yy[k-1],diff);
            omega[k] = n2*(-area2*VEC_DOT_3(z,to->nn)*solid +
                           triple*VEC_DOT_3(diff,vec_omega));
        }
    }
#ifdef CHECK
    /*
   * Check it out!
   *
   * omega1 + omega2 + omega3 = solid
   */
    rel1 = (solid + omega[X_3]+omega[Y_3]+omega[Z_3])/solid;
    /*
   * The other way of evaluating...
   */
    for (j = 0; j < 3; j++)
        check[j] = 0;
    for (k = 0; k < 3; k++) {
        CROSS_PRODUCT_3(to->nn[to],yy[k],z);
        for (j = 0; j < 3; j++)
            check[j] = check[j] + omega[k]*z[j];
    }
    for (j = 0; j < 3; j++)
        check[j] = -area2*check[j]/triple;
    fprintf (stderr,"(%g,%g,%g) =? (%g,%g,%g)\n",
             check[X_3],check[Y_3],check[Z_3],
             vec_omega[X_3],vec_omega[Y_3],vec_omega[Z_3]);
    for (j = 0; j < 3; j++)
        check[j] = check[j] - vec_omega[j];
    rel2 = sqrt(VEC_DOT_3(check,check)/VEC_DOT_3(vec_omega,vec_omega));
    fprintf (stderr,"err1 = %g, err2 = %g\n",100*rel1,100*rel2);
#endif
    return;
}



static void correct_auto_elements (MneSurfaceOrVolume::MneCSurface* surf,
                                   float      **mat)
/*
      * Improve auto-element approximation...
      */
{
    float *row;
    float sum,miss;
    int   nnode = surf->np;
    int   ntri  = surf->ntri;
    int   nmemb;
    int   j,k;
    float pi2 = 2.0*M_PI;
    mneTriangle   tri;

#ifdef SIMPLE
    for (j = 0; j < nnode; j++) {
        row = mat[j];
        sum = 0.0;
        for (k = 0; k < nnode; k++)
            sum = sum + row[k];
        fprintf (stderr,"row %d sum = %g missing = %g\n",j+1,sum/pi2,
                 1.0-sum/pi2);
        row[j] = pi2 - sum;
    }
#else
    for (j = 0; j < nnode; j++) {
        /*
     * How much is missing?
     */
        row = mat[j];
        sum = 0.0;
        for (k = 0; k < nnode; k++)
            sum = sum + row[k];
        miss  = pi2-sum;
        nmemb = surf->nneighbor_tri[j];
        /*
     * The node itself receives one half
     */
        row[j] = miss/2.0;
        /*
     * The rest is divided evenly among the member nodes...
     */
        miss = miss/(4.0*nmemb);
        for (k = 0,tri = surf->tris; k < ntri; k++,tri++) {
            if (tri->vert[0] == j) {
                row[tri->vert[1]] = row[tri->vert[1]] + miss;
                row[tri->vert[2]] = row[tri->vert[2]] + miss;
            }
            else if (tri->vert[1] == j) {
                row[tri->vert[0]] = row[tri->vert[0]] + miss;
                row[tri->vert[2]] = row[tri->vert[2]] + miss;
            }
            else if (tri->vert[2] == j) {
                row[tri->vert[0]] = row[tri->vert[0]] + miss;
                row[tri->vert[1]] = row[tri->vert[1]] + miss;
            }
        }
        /*
     * Just check it it out...
     *
    for (k = 0, sum = 0; k < nnode; k++)
      sum = sum + row[k];
    fprintf (stderr,"row %d sum = %g\n",j+1,sum/pi2);
    */
    }
#endif
    return;
}


static float **fwd_bem_lin_pot_coeff (MneSurfaceOrVolume::MneCSurface* *surfs,int nsurf)
/*
      * Calculate the coefficients for linear collocation approach
      */
{
    float **mat = NULL;
    float **sub_mat = NULL;
    int   np1,np2,ntri,np_tot,np_max;
    float **nodes;
    mneTriangle   tri;
    double omega[3];
    double *row = NULL;
    int    j,k,p,q,c;
    int    joff,koff;
    MneSurfaceOrVolume::MneCSurface* surf1;
    MneSurfaceOrVolume::MneCSurface* surf2;

    for (p = 0, np_tot = np_max = 0; p < nsurf; p++) {
        np_tot += surfs[p]->np;
        if (surfs[p]->np > np_max)
            np_max = surfs[p]->np;
    }

    mat = ALLOC_CMATRIX_3(np_tot,np_tot);
    for (j = 0; j < np_tot; j++)
        for (k = 0; k < np_tot; k++)
            mat[j][k] = 0.0;
    row        = MALLOC_3(np_max,double);
    sub_mat = MALLOC_3(np_max,float *);
    for (p = 0, joff = 0; p < nsurf; p++, joff = joff + np1) {
        surf1 = surfs[p];
        np1   = surf1->np;
        nodes = surf1->rr;
        for (q = 0, koff = 0; q < nsurf; q++, koff = koff + np2) {
            surf2 = surfs[q];
            np2   = surf2->np;
            ntri  = surf2->ntri;

            fprintf(stderr,"\t\t%s (%d) -> %s (%d) ... ",
                    fwd_bem_explain_surface(surf1->id),np1,
                    fwd_bem_explain_surface(surf2->id),np2);

            for (j = 0; j < np1; j++) {
                for (k = 0; k < np2; k++)
                    row[k] = 0.0;
                for (k = 0, tri = surf2->tris; k < ntri; k++,tri++) {
                    /*
           * No contribution from a triangle that
           * this vertex belongs to
           */
                    if (p == q && (tri->vert[0] == j || tri->vert[1] == j || tri->vert[2] == j))
                        continue;
                    /*
           * Otherwise do the hard job
           */
                    lin_pot_coeff (nodes[j],tri,omega);
                    for (c = 0; c < 3; c++)
                        row[tri->vert[c]] = row[tri->vert[c]] - omega[c];
                }
                for (k = 0; k < np2; k++)
                    mat[j+joff][k+koff] = row[k];
            }
            if (p == q) {
                for (j = 0; j < np1; j++)
                    sub_mat[j] = mat[j+joff]+koff;
                correct_auto_elements (surf1,sub_mat);
            }
            fprintf(stderr,"[done]\n");
        }
    }
    FREE_3(row);
    FREE_3(sub_mat);
    return(mat);
}





int fwd_bem_linear_collocation_solution(fwdBemModel m)
/*
 * Compute the linear collocation potential solution
 */
{
    float **coeff = NULL;
    float ip_mult;
    int k;

    fwd_bem_free_solution_3(m);

    fprintf(stderr,"\nComputing the linear collocation solution...\n");
    fprintf (stderr,"\tMatrix coefficients...\n");
    if ((coeff = fwd_bem_lin_pot_coeff (m->surfs,m->nsurf)) == NULL)
        goto bad;

    for (k = 0, m->nsol = 0; k < m->nsurf; k++)
        m->nsol += m->surfs[k]->np;

    fprintf (stderr,"\tInverting the coefficient matrix...\n");
    if ((m->solution = fwd_bem_multi_solution (coeff,m->gamma,m->nsurf,m->np)) == NULL)
        goto bad;

    /*
   * IP approach?
   */
    if ((m->nsurf == 3) &&
            (ip_mult = m->sigma[m->nsurf-2]/m->sigma[m->nsurf-1]) <= m->ip_approach_limit) {
        float **ip_solution = NULL;

        fprintf (stderr,"IP approach required...\n");

        fprintf (stderr,"\tMatrix coefficients (homog)...\n");
        if ((coeff = fwd_bem_lin_pot_coeff (m->surfs+m->nsurf-1,1)) == NULL)
            goto bad;

        fprintf (stderr,"\tInverting the coefficient matrix (homog)...\n");
        if ((ip_solution = fwd_bem_homog_solution (coeff,m->surfs[m->nsurf-1]->np)) == NULL)
            goto bad;

        fprintf (stderr,"\tModify the original solution to incorporate IP approach...\n");

        fwd_bem_ip_modify_solution(m->solution,ip_solution,ip_mult,m->nsurf,m->np);
        FREE_CMATRIX_3(ip_solution);

    }
    m->bem_method = FWD_BEM_LINEAR_COLL;
    fprintf(stderr,"Solution ready.\n");
    return OK;

bad : {
        fwd_bem_free_solution_3(m);
        FREE_CMATRIX_3(coeff);
        return FAIL;
    }
}










//============================= make_filter_source_sapces.c =============================

static double solid_angle (float       *from,	/* From this point... */
                           mneTriangle tri)	/* ...to this triangle */
/*
 * Compute the solid angle according to van Oosterom's
 * formula
 */
{
    double v1[3],v2[3],v3[3];
    double l1,l2,l3,s,triple;
    double cross[3];

    VEC_DIFF_3(from,tri->r1,v1);
    VEC_DIFF_3(from,tri->r2,v2);
    VEC_DIFF_3(from,tri->r3,v3);

    CROSS_PRODUCT_3(v1,v2,cross);
    triple = VEC_DOT_3(cross,v3);

    l1 = VEC_LEN_3(v1);
    l2 = VEC_LEN_3(v2);
    l3 = VEC_LEN_3(v3);
    s = (l1*l2*l3+VEC_DOT_3(v1,v2)*l3+VEC_DOT_3(v1,v3)*l2+VEC_DOT_3(v2,v3)*l1);

    return (2.0*atan2(triple,s));
}




//============================= fwd_bem_constant_collocation.c =============================


static int fwd_bem_check_solids (float **angles,int ntri1,int ntri2, float desired)
/*
 * Check the angle computations
 */
{
    float *sums = MALLOC_3(ntri1,float);
    float sum;
    int j,k;
    int res = 0;

    for (j = 0; j < ntri1; j++) {
        sum = 0;
        for (k = 0; k < ntri2; k++)
            sum = sum + angles[j][k];
        sums[j] = sum/(2*M_PI);
    }
    for (j = 0; j < ntri1; j++)
        /*
     * Three cases:
     * same surface: sum = 2*pi
     * to outer:     sum = 4*pi
     * to inner:     sum = 0*pi;
     */
        if (fabs(sums[j]-desired) > 1e-4) {
            printf("solid angle matrix: rowsum[%d] = 2PI*%g",
                   j+1,sums[j]);
            res = -1;
            break;
        }
    FREE_3(sums);
    return res;
}


static float **fwd_bem_solid_angles (MneSurfaceOrVolume::MneCSurface* *surfs, int nsurf)
/*
      * Compute the solid angle matrix
      */
{
    MneSurfaceOrVolume::MneCSurface* surf1;
    MneSurfaceOrVolume::MneCSurface* surf2;
    mneTriangle tri;
    int ntri1,ntri2,ntri_tot;
    int j,k,p,q;
    int joff,koff;
    float **solids;
    float result;
    float **sub_solids = NULL;
    float desired;

    for (p = 0,ntri_tot = 0; p < nsurf; p++)
        ntri_tot += surfs[p]->ntri;

    sub_solids = MALLOC_3(ntri_tot,float *);
    solids = ALLOC_CMATRIX_3(ntri_tot,ntri_tot);
    for (p = 0, joff = 0; p < nsurf; p++, joff = joff + ntri1) {
        surf1 = surfs[p];
        ntri1 = surf1->ntri;
        for (q = 0, koff = 0; q < nsurf; q++, koff = koff + ntri2) {
            surf2 = surfs[q];
            ntri2 = surf2->ntri;
            fprintf(stderr,"\t\t%s (%d) -> %s (%d) ... ",fwd_bem_explain_surface(surf1->id),ntri1,fwd_bem_explain_surface(surf2->id),ntri2);
            for (j = 0; j < ntri1; j++)
                for (k = 0, tri = surf2->tris; k < ntri2; k++, tri++) {
                    if (p == q && j == k)
                        result = 0.0;
                    else
                        result = solid_angle (surf1->tris[j].cent,tri);
                    solids[j+joff][k+koff] = result;
                }
            for (j = 0; j < ntri1; j++)
                sub_solids[j] = solids[j+joff]+koff;
            fprintf(stderr,"[done]\n");
            if (p == q)
                desired = 1;
            else if (p < q)
                desired = 0;
            else
                desired = 2;
            if (fwd_bem_check_solids(sub_solids,ntri1,ntri2,desired) == FAIL) {
                FREE_CMATRIX_3(solids);
                FREE_3(sub_solids);
                return NULL;
            }
        }
    }
    FREE_3(sub_solids);
    return (solids);
}



int fwd_bem_constant_collocation_solution(fwdBemModel m)
/*
 * Compute the solution for the constant collocation approach
 */
{
    float  **solids = NULL;
    int    k;
    float  ip_mult;

    fwd_bem_free_solution_3(m);

    fprintf(stderr,"\nComputing the constant collocation solution...\n");
    fprintf(stderr,"\tSolid angles...\n");
    if ((solids = fwd_bem_solid_angles(m->surfs,m->nsurf)) == NULL)
        goto bad;

    for (k = 0, m->nsol = 0; k < m->nsurf; k++)
        m->nsol += m->surfs[k]->ntri;

    fprintf (stderr,"\tInverting the coefficient matrix...\n");
    if ((m->solution = fwd_bem_multi_solution (solids,m->gamma,m->nsurf,m->ntri)) == NULL)
        goto bad;
    /*
   * IP approach?
   */
    if ((m->nsurf == 3) &&
            (ip_mult = m->sigma[m->nsurf-2]/m->sigma[m->nsurf-1]) <= m->ip_approach_limit) {
        float **ip_solution = NULL;

        fprintf (stderr,"IP approach required...\n");

        fprintf (stderr,"\tSolid angles (homog)...\n");
        if ((solids = fwd_bem_solid_angles (m->surfs+m->nsurf-1,1)) == NULL)
            goto bad;

        fprintf (stderr,"\tInverting the coefficient matrix (homog)...\n");
        if ((ip_solution = fwd_bem_homog_solution (solids,m->surfs[m->nsurf-1]->ntri)) == NULL)
            goto bad;

        fprintf (stderr,"\tModify the original solution to incorporate IP approach...\n");
        fwd_bem_ip_modify_solution(m->solution,ip_solution,ip_mult,m->nsurf,m->ntri);
        FREE_CMATRIX_3(ip_solution);
    }
    m->bem_method = FWD_BEM_CONSTANT_COLL;
    fprintf (stderr,"Solution ready.\n");

    return OK;

bad : {
        fwd_bem_free_solution_3(m);
        FREE_CMATRIX_3(solids);
        return FAIL;
    }
}





//============================= fwd_bem_model.c =============================


int fwd_bem_compute_solution(fwdBemModel m,
                             int         bem_method)
/*
* Compute the solution
*/
{
    /*
    * Compute the solution
    */
    if (bem_method == FWD_BEM_LINEAR_COLL)
        return fwd_bem_linear_collocation_solution(m);
    else if (bem_method == FWD_BEM_CONSTANT_COLL)
        return fwd_bem_constant_collocation_solution(m);

    fwd_bem_free_solution_3(m);
    printf ("Unknown BEM method: %d\n",bem_method);
    return FAIL;
}



int fwd_bem_load_recompute_solution(char        *name,
                                    int         bem_method,
                                    int         force_recompute,
                                    fwdBemModel m)
/*
* Load or recompute the potential solution matrix
*/
{
    int solres;

    if (!m) {
        printf ("No model specified for fwd_bem_load_recompute_solution");
        return FAIL;
    }

    if (!force_recompute) {
        fwd_bem_free_solution_3(m);
        solres = fwd_bem_load_solution(name,bem_method,m);
        if (solres == TRUE) {
            fprintf(stderr,"\nLoaded %s BEM solution from %s\n",fwd_bem_explain_method(m->bem_method),name);
            return OK;
        }
        else if (solres == FAIL)
            return FAIL;
#ifdef DEBUG
        else
            fprintf(stderr,"Desired BEM  solution not available in %s (%s)\n",name,err_get_error());
#endif
    }
    if (bem_method == FWD_BEM_UNKNOWN)
        bem_method = FWD_BEM_LINEAR_COLL;
    return fwd_bem_compute_solution(m,bem_method);
}




//============================= mne_simplex_fit.c =============================

/*
 * This routine comes from Numerical recipes
 */

#define ALPHA 1.0
#define BETA 0.5
#define GAMMA 2.0

static float tryit (float **p,
                    float *y,
                    float *psum,
                    int   ndim,
                    float (*func)(float *x,int npar,void *user_data),	  /* The function to be evaluated */
                    void  *user_data,				          /* Data to be passed to the above function in each evaluation */
                    int   ihi,
                    int   *neval,
                    float fac)

{
    int j;
    float fac1,fac2,ytry,*ptry;

    ptry = ALLOC_FLOAT_3(ndim);
    fac1 = (1.0-fac)/ndim;
    fac2 = fac1-fac;
    for (j = 0; j < ndim; j++)
        ptry[j] = psum[j]*fac1-p[ihi][j]*fac2;
    ytry = (*func)(ptry,ndim,user_data);
    ++(*neval);
    if (ytry < y[ihi]) {
        y[ihi] = ytry;
        for (j = 0; j < ndim; j++) {
            psum[j] +=  ptry[j]-p[ihi][j];
            p[ihi][j] = ptry[j];
        }
    }
    FREE_3(ptry);
    return ytry;
}

int mne_simplex_minimize(float **p,		                              /* The initial simplex */
                         float *y,		                              /* Function values at the vertices */
                         int   ndim,	                                      /* Number of variables */
                         float ftol,	                                      /* Relative convergence tolerance */
                         float (*func)(float *x,int npar,void *user_data),    /* The function to be evaluated */
                         void  *user_data,				      /* Data to be passed to the above function in each evaluation */
                         int   max_eval,	                              /* Maximum number of function evaluations */
                         int   *neval,	                                      /* Number of function evaluations */
                         int   report,                                        /* How often to report (-1 = no_reporting) */
                         int   (*report_func)(int loop,
                                              float *fitpar, int npar,
                                              double fval))                   /* The function to be called when reporting */

/*
      * Minimization with the simplex algorithm
      * Modified from Numerical recipes
      */

{
    int   i,j,ilo,ihi,inhi;
    int   mpts = ndim+1;
    float ytry,ysave,sum,rtol,*psum;
    int   result = 0;
    int   count = 0;
    int   loop  = 1;

    psum = ALLOC_FLOAT_3(ndim);
    *neval = 0;
    for (j = 0; j < ndim; j++) {
        for (i = 0,sum = 0.0; i<mpts; i++)
            sum +=  p[i][j];
        psum[j] = sum;
    }
    if (report_func != NULL && report > 0)
        (void)report_func (0,p[0],ndim,-1.0);

    for (;;count++,loop++) {
        ilo = 1;
        ihi  =  y[1]>y[2] ? (inhi = 2,1) : (inhi = 1,2);
        for (i = 0; i < mpts; i++) {
            if (y[i]  <  y[ilo]) ilo = i;
            if (y[i] > y[ihi]) {
                inhi = ihi;
                ihi = i;
            } else if (y[i] > y[inhi])
                if (i !=  ihi) inhi = i;
        }
        rtol = 2.0*fabs(y[ihi]-y[ilo])/(fabs(y[ihi])+fabs(y[ilo]));
        /*
     * Report that we are proceeding...
     */
        if (count == report && report_func != NULL) {
            if (report_func (loop,p[ilo],ndim,y[ilo])) {
                qWarning("Interation interrupted.");
                result = -1;
                break;
            }
            count = 0;
        }
        if (rtol < ftol) break;
        if (*neval >=  max_eval) {
            qWarning("Maximum number of evaluations exceeded.");
            result  =  -1;
            break;
        }
        ytry = tryit(p,y,psum,ndim,func,user_data,ihi,neval,-ALPHA);
        if (ytry <= y[ilo])
            ytry = tryit(p,y,psum,ndim,func,user_data,ihi,neval,GAMMA);
        else if (ytry >= y[inhi]) {
            ysave = y[ihi];
            ytry = tryit(p,y,psum,ndim,func,user_data,ihi,neval,BETA);
            if (ytry >= ysave) {
                for (i = 0; i < mpts; i++) {
                    if (i !=  ilo) {
                        for (j = 0; j < ndim; j++) {
                            psum[j] = 0.5*(p[i][j]+p[ilo][j]);
                            p[i][j] = psum[j];
                        }
                        y[i] = (*func)(psum,ndim,user_data);
                    }
                }
                *neval +=  ndim;
                for (j = 0; j < ndim; j++) {
                    for (i = 0,sum = 0.0; i < mpts; i++)
                        sum +=  p[i][j];
                    psum[j] = sum;
                }
            }
        }
    }
    FREE_3(psum);
    return (result);
}

#undef ALPHA
#undef BETA
#undef GAMMA









//============================= mne_project_to_surface.c =============================


typedef struct {
    float *a;
    float *b;
    float *c;
    int   *act;
    int   nactive;
} *projData,projDataRec;




void mne_triangle_coords(float       *r,       /* Location of a point */
                         MneSurfaceOrVolume::MneCSurface*  s,	       /* The surface */
                         int         tri,      /* Which triangle */
                         float       *x,       /* Coordinates of the point on the triangle */
                         float       *y,
                         float       *z)
/*
      * Compute the coordinates of a point within a triangle
      */
{
    double rr[3];			/* Vector from triangle corner #1 to r */
    double a,b,c,v1,v2,det;
    mneTriangle this_tri;

    this_tri = s->tris+tri;

    VEC_DIFF_3(this_tri->r1,r,rr);
    *z = VEC_DOT_3(rr,this_tri->nn);

    a =  VEC_DOT_3(this_tri->r12,this_tri->r12);
    b =  VEC_DOT_3(this_tri->r13,this_tri->r13);
    c =  VEC_DOT_3(this_tri->r12,this_tri->r13);

    v1 = VEC_DOT_3(rr,this_tri->r12);
    v2 = VEC_DOT_3(rr,this_tri->r13);

    det = a*b - c*c;

    *x = (b*v1 - c*v2)/det;
    *y = (a*v2 - c*v1)/det;

    return;
}





//============================= mne_project_to_surface.c =============================


























static int nearest_triangle_point(float       *r,    /* Location of a point */
                                  MneSurfaceOrVolume::MneCSurface*  s,     /* The surface */
                                  void        *user, /* Something precomputed */
                                  int         tri,   /* Which triangle */
                                  float       *x,    /* Coordinates of the point on the triangle */
                                  float       *y,
                                  float       *z)
/*
      * Find the nearest point from a triangle
      */
{

    double p,q,p0,q0,t0;
    double rr[3];			/* Vector from triangle corner #1 to r */
    double a,b,c,v1,v2,det;
    double best,dist,dist0;
    projData    pd = (projData)user;
    mneTriangle this_tri;

    this_tri = s->tris+tri;
    VEC_DIFF_3(this_tri->r1,r,rr);
    dist  = VEC_DOT_3(rr,this_tri->nn);

    if (pd) {
        if (!pd->act[tri])
            return FALSE;
        a = pd->a[tri];
        b = pd->b[tri];
        c = pd->c[tri];
    }
    else {
        a =  VEC_DOT_3(this_tri->r12,this_tri->r12);
        b =  VEC_DOT_3(this_tri->r13,this_tri->r13);
        c =  VEC_DOT_3(this_tri->r12,this_tri->r13);
    }

    v1 = VEC_DOT_3(rr,this_tri->r12);
    v2 = VEC_DOT_3(rr,this_tri->r13);

    det = a*b - c*c;

    p = (b*v1 - c*v2)/det;
    q = (a*v2 - c*v1)/det;
    /*
   * If the point projects into the triangle we are done
   */
    if (p >= 0.0 && p <= 1.0 &&
            q >= 0.0 && q <= 1.0 &&
            q <= 1.0 - p) {
        *x = p;
        *y = q;
        *z = dist;
        return TRUE;
    }
    /*
   * Tough: must investigate the sides
   * We might do something intelligent here. However, for now it is ok
   * to do it in the hard way
   */
    /*
   * Side 1 -> 2
   */
    p0 = p + 0.5*(q * c)/a;
    if (p0 < 0.0)
        p0 = 0.0;
    else if (p0 > 1.0)
        p0 = 1.0;
    q0 = 0.0;
    dist0 = sqrt((p-p0)*(p-p0)*a +
                 (q-q0)*(q-q0)*b +
                 (p-p0)*(q-q0)*c +
                 dist*dist);
    best = dist0;
    *x = p0;
    *y = q0;
    *z = dist0;
    /*
   * Side 2 -> 3
   */
    t0 = 0.5*((2.0*a-c)*(1.0-p) + (2.0*b-c)*q)/(a+b-c);
    if (t0 < 0.0)
        t0 = 0.0;
    else if (t0 > 1.0)
        t0 = 1.0;
    p0 = 1.0 - t0;
    q0 = t0;
    dist0 = sqrt((p-p0)*(p-p0)*a +
                 (q-q0)*(q-q0)*b +
                 (p-p0)*(q-q0)*c +
                 dist*dist);
    if (dist0 < best) {
        best = dist0;
        *x = p0;
        *y = q0;
        *z = dist0;
    }
    /*
   * Side 1 -> 3
   */
    p0 = 0.0;
    q0 = q + 0.5*(p * c)/b;
    if (q0 < 0.0)
        q0 = 0.0;
    else if (q0 > 1.0)
        q0 = 1.0;
    dist0 = sqrt((p-p0)*(p-p0)*a +
                 (q-q0)*(q-q0)*b +
                 (p-p0)*(q-q0)*c +
                 dist*dist);
    if (dist0 < best) {
        best = dist0;
        *x = p0;
        *y = q0;
        *z = dist0;
    }
    return TRUE;
}

static void project_to_triangle(MneSurfaceOrVolume::MneCSurface* s,
                                int        tri,
                                float      p,
                                float      q,
                                float      *r)

{
    int   k;
    mneTriangle this_tri;

    this_tri = s->tris+tri;

    for (k = 0; k < 3; k++)
        r[k] = this_tri->r1[k] + p*this_tri->r12[k] + q*this_tri->r13[k];

    return;
}

int mne_nearest_triangle_point(float       *r,    /* Location of a point */
                               MneSurfaceOrVolume::MneCSurface*  s,     /* The surface */
                               int         tri,   /* Which triangle */
                               float       *x,    /* Coordinates of the point on the triangle */
                               float       *y,
                               float       *z)
/*
 * This is for external use
 */
{
    return nearest_triangle_point(r,s,NULL,tri,x,y,z);
}





int mne_project_to_surface(MneSurfaceOrVolume::MneCSurface* s, void *proj_data, float *r, int project_it, float *distp)
/*
      * Project the point onto the closest point on the surface
      */
{
    float dist;			/* Distance to the triangle */
    float p,q;			/* Coordinates on the triangle */
    float p0,q0,dist0;
    int   best;
    int   k;

    p0 = q0 = 0.0;
    dist0 = 0.0;
    for (best = -1, k = 0; k < s->ntri; k++) {
        if (nearest_triangle_point(r,s,proj_data,k,&p,&q,&dist)) {
            if (best < 0 || fabs(dist) < fabs(dist0)) {
                dist0 = dist;
                best = k;
                p0 = p;
                q0 = q;
            }
        }
    }
    if (best >= 0 && project_it)
        project_to_triangle(s,best,p0,q0,r);
    if (distp)
        *distp = dist0;
    return best;
}








//============================= fit_sphere.c =============================


typedef struct {
    float **rr;
    int   np;
    int   report;
} *fitSphereUser,fitSphereUserRec;

static int report_func(int     loop,
                       float   *fitpar,
                       int     npar,
                       double  fval)
/*
      * Report periodically
      */
{
    float *r0 = fitpar;

    fprintf(stderr,"loop %d r0 %7.1f %7.1f %7.1f fval %g\n",
            loop,1000*r0[0],1000*r0[1],1000*r0[2],fval);

    return OK;
}

static float fit_sphere_eval(float *fitpar,
                             int   npar,
                             void  *user_data)
/*
      * Calculate the cost function value
      * Optimize for the radius inside here
      */
{
    fitSphereUser user = (fitSphereUser)user_data;
    float *r0 = fitpar;
    float diff[3];
    int   k;
    float sum,sum2,one,F;

    for (k = 0, sum = sum2 = 0.0; k < user->np; k++) {
        VEC_DIFF_3(r0,user->rr[k],diff);
        one = VEC_LEN_3(diff);
        sum  += one;
        sum2 += one*one;
    }
    F = sum2 - sum*sum/user->np;

    if (user->report)
        fprintf(stderr,"r0 %7.1f %7.1f %7.1f R %7.1f fval %g\n",
                1000*r0[0],1000*r0[1],1000*r0[2],1000*sum/user->np,F);

    return F;
}

static float opt_rad(float *r0,fitSphereUser user)

{
    float sum, diff[3], one;
    int   k;

    for (k = 0, sum = 0.0; k < user->np; k++) {
        VEC_DIFF_3(r0,user->rr[k],diff);
        one = VEC_LEN_3(diff);
        sum  += one;
    }
    return sum/user->np;
}


static void calculate_cm_ave_dist(float **rr, int np, float *cm, float *avep)

{
    int k,q;
    float ave,diff[3];

    for (q = 0; q < 3; q++)
        cm[q] = 0.0;

    for (k = 0; k < np; k++)
        for (q = 0; q < 3; q++)
            cm[q] += rr[k][q];

    if (np > 0) {
        for (q = 0; q < 3; q++)
            cm[q] = cm[q]/np;

        for (k = 0, ave = 0.0; k < np; k++) {
            for (q = 0; q < 3; q++)
                diff[q] = rr[k][q] - cm[q];
            ave += VEC_LEN_3(diff);
        }
        *avep = ave/np;
    }
    return;
}

static float **make_initial_simplex(float  *pars,
                                    int    npar,
                                    float  size)
/*
      * Make the initial tetrahedron
      */
{
    float **simplex = ALLOC_CMATRIX_3(npar+1,npar);
    int k;

    for (k = 0; k < npar+1; k++)
        memcpy (simplex[k],pars,npar*sizeof(float));

    for (k = 1; k < npar+1; k++)
        simplex[k][k-1] = simplex[k][k-1] + size;
    return (simplex);
}


int fit_sphere_to_points(float **rr,
                         int   np,
                         float simplex_size,
                         float *r0,
                         float *R)
/*
      * Find the optimal sphere origin
      */
{
    fitSphereUserRec user;
    float      ftol            = 1e-5;
    int        max_eval        = 500;
    int        report_interval = -1;
    int        neval = 0;
    float      **init_simplex  = NULL;
    float      *init_vals      = NULL;

    float      cm[3],R0;
    int        k;

    int        res = FAIL;

    user.rr = rr;
    user.np = np;

    calculate_cm_ave_dist(rr,np,cm,&R0);

#ifdef DEBUG
    fprintf(stderr,"cm %7.1f %7.1f %7.1f R %7.1f\n",
            1000*cm[0],1000*cm[1],1000*cm[2],1000*R0);
#endif

    init_simplex = make_initial_simplex(cm,3,simplex_size);

    init_vals = MALLOC_3(4,float);

#ifdef DEBUG
    user.report = TRUE;
#else
    user.report = FALSE;
#endif

    for (k = 0; k < 4; k++)
        init_vals[k] = fit_sphere_eval(init_simplex[k],3,&user);

    if (mne_simplex_minimize(init_simplex,		                  /* The initial simplex */
                             init_vals,		                          /* Function values at the vertices */
                             3,    	                                  /* Number of variables */
                             ftol,	                                  /* Relative convergence tolerance */
                             fit_sphere_eval,                                      /* The function to be evaluated */
                             &user,	  			          /* Data to be passed to the above function in each evaluation */
                             max_eval,	                                  /* Maximum number of function evaluations */
                             &neval,	                                  /* Number of function evaluations */
                             report_interval,	                          /* How often to report (-1 = no_reporting) */
                             report_func) != OK)                            /* The function to be called when reporting */
        goto out;

    r0[X_3] = init_simplex[0][X_3];
    r0[Y_3] = init_simplex[0][Y_3];
    r0[Z_3] = init_simplex[0][Z_3];
    *R    = opt_rad(r0,&user);

    res = OK;
    goto out;

out : {
        FREE_3(init_vals);
        FREE_CMATRIX_3(init_simplex);
        return res;
    }
}





//============================= fwd_bem_pot.c =============================

static float fwd_bem_inf_field(float *rd,      /* Dipole position */
                               float *Q,       /* Dipole moment */
                               float *rp,      /* Field point */
                               float *dir)     /* Which field component */
/*
 * Infinite-medium magnetic field
 * (without \mu_0/4\pi)
 */
{
    float diff[3],diff2,cross[3];

    VEC_DIFF_3(rd,rp,diff);
    diff2 = VEC_DOT_3(diff,diff);
    CROSS_PRODUCT_3(Q,diff,cross);

    return (VEC_DOT_3(cross,dir)/(diff2*sqrt(diff2)));
}


float fwd_bem_inf_pot (float *rd,	/* Dipole position */
                       float *Q,	/* Dipole moment */
                       float *rp)	/* Potential point */
/*
 * The infinite medium potential
 */
{
    float diff[3];
    float diff2;
    VEC_DIFF_3(rd,rp,diff);
    diff2 = VEC_DOT_3(diff,diff);
    return (VEC_DOT_3(Q,diff)/(4.0*M_PI*diff2*sqrt(diff2)));
}




int fwd_bem_specify_els(fwdBemModel m,
                        FwdCoilSet*  els)
/*
 * Set up for computing the solution at a set of electrodes
 */
{
    FwdCoil*     el;
    MneSurfaceOrVolume::MneCSurface*  scalp;
    int         k,p,q,v;
    float       *one_sol,*pick_sol;
    float       r[3],w[3],dist;
    int         best;
    mneTriangle tri;
    float       x,y,z;
    fwdBemSolution sol;

    extern fwdBemSolution fwd_bem_new_coil_solution();
    extern void fwd_bem_free_coil_solution(void *user);

    if (!m) {
        printf("Model missing in fwd_bem_specify_els");
        goto bad;
    }
    if (!m->solution) {
        printf("Solution not computed in fwd_bem_specify_els");
        goto bad;
    }
    if (!els || els->ncoil == 0)
        return OK;
    els->fwd_free_coil_set_user_data();
    /*
   * Hard work follows
   */
    els->user_data = sol = fwd_bem_new_coil_solution();
    els->user_data_free = fwd_bem_free_coil_solution;

    sol->ncoil = els->ncoil;
    sol->np    = m->nsol;
    sol->solution  = ALLOC_CMATRIX_3(sol->ncoil,sol->np);
    /*
   * Go through all coils
   */
    for (k = 0; k < els->ncoil; k++) {
        el = els->coils[k];
        one_sol = sol->solution[k];
        for (q = 0; q < m->nsol; q++)
            one_sol[q] = 0.0;
        scalp = m->surfs[0];
        /*
     * Go through all 'integration points'
     */
        for (p = 0; p < el->np; p++) {
            VEC_COPY_3(r,el->rmag[p]);
            if (m->head_mri_t != NULL)
                FiffCoordTransOld::fiff_coord_trans(r,m->head_mri_t,FIFFV_MOVE);
            best = mne_project_to_surface(scalp,NULL,r,FALSE,&dist);
            if (best < 0) {
                printf("One of the electrodes could not be projected onto the scalp surface. How come?");
                goto bad;
            }
            if (m->bem_method == FWD_BEM_CONSTANT_COLL) {
                /*
         * Simply pick the value at the triangle
         */
                pick_sol = m->solution[best];
                for (q = 0; q < m->nsol; q++)
                    one_sol[q] += el->w[p]*pick_sol[q];
            }
            else if (m->bem_method == FWD_BEM_LINEAR_COLL) {
                /*
         * Calculate a linear interpolation between the vertex values
         */
                tri = scalp->tris+best;
                mne_triangle_coords(r,scalp,best,&x,&y,&z);

                w[X_3] = el->w[p]*(1.0 - x - y);
                w[Y_3] = el->w[p]*x;
                w[Z_3] = el->w[p]*y;
                for (v = 0; v < 3; v++) {
                    pick_sol = m->solution[tri->vert[v]];
                    for (q = 0; q < m->nsol; q++)
                        one_sol[q] += w[v]*pick_sol[q];
                }
            }
            else {
                printf("Unknown BEM approximation method : %d\n",m->bem_method);
                goto bad;
            }
        }
    }
    return OK;

bad : {
        els->fwd_free_coil_set_user_data();
        return FAIL;
    }
}




static void fwd_bem_lin_pot_calc (float       *rd,		/* Dipole position */
                                  float       *Q,		/* Dipole orientation */
                                  fwdBemModel m,		/* The model */
                                  FwdCoilSet*  els,              /* Use this electrode set if available */
                                  int         all_surfs,	/* Compute on all surfaces? */
                                  float      *pot)              /* Put the result here */
/*
      * Compute the potentials due to a current dipole
      * using the linear potential approximation
      */
{
    float **rr;
    int   np;
    int   s,k,p,nsol;
    float mult,mri_rd[3],mri_Q[3];

    float *v0;
    float **solution;

    if (!m->v0)
        m->v0 = MALLOC_3(m->nsol,float);
    v0 = m->v0;

    VEC_COPY_3(mri_rd,rd);
    VEC_COPY_3(mri_Q,Q);
    if (m->head_mri_t) {
        FiffCoordTransOld::fiff_coord_trans(mri_rd,m->head_mri_t,FIFFV_MOVE);
        FiffCoordTransOld::fiff_coord_trans(mri_Q,m->head_mri_t,FIFFV_NO_MOVE);
    }
    for (s = 0, p = 0; s < m->nsurf; s++) {
        np     = m->surfs[s]->np;
        rr     = m->surfs[s]->rr;
        mult   = m->source_mult[s];
        for (k = 0; k < np; k++)
            v0[p++] = mult*fwd_bem_inf_pot(mri_rd,mri_Q,rr[k]);
    }
    if (els) {
        fwdBemSolution sol = (fwdBemSolution)els->user_data;
        solution = sol->solution;
        nsol     = sol->ncoil;
    }
    else {
        solution = m->solution;
        nsol     = all_surfs ? m->nsol : m->surfs[0]->np;
    }
    for (k = 0; k < nsol; k++)
        pot[k] = mne_dot_vectors_3(solution[k],v0,m->nsol);
    return;
}



static void fwd_bem_pot_calc (float       *rd,	     /* Dipole position */
                              float       *Q,        /* Dipole orientation */
                              fwdBemModel m,	     /* The model */
                              FwdCoilSet*  els,       /* Use this electrode set if available */
                              int         all_surfs, /* Compute solution on all surfaces? */
                              float       *pot)
/*
      * Compute the potentials due to a current dipole
      */
{
    mneTriangle tri;
    int         ntri;
    int         s,k,p,nsol;
    float       mult;
    float       *v0;
    float       **solution;
    float       mri_rd[3],mri_Q[3];

    if (!m->v0)
        m->v0 = MALLOC_3(m->nsol,float);
    v0 = m->v0;

    VEC_COPY_3(mri_rd,rd);
    VEC_COPY_3(mri_Q,Q);
    if (m->head_mri_t) {
        FiffCoordTransOld::fiff_coord_trans(mri_rd,m->head_mri_t,FIFFV_MOVE);
        FiffCoordTransOld::fiff_coord_trans(mri_Q,m->head_mri_t,FIFFV_NO_MOVE);
    }
    for (s = 0, p = 0; s < m->nsurf; s++) {
        ntri = m->surfs[s]->ntri;
        tri  = m->surfs[s]->tris;
        mult = m->source_mult[s];
        for (k = 0; k < ntri; k++, tri++)
            v0[p++] = mult*fwd_bem_inf_pot(mri_rd,mri_Q,tri->cent);
    }
    if (els) {
        fwdBemSolution sol = (fwdBemSolution)els->user_data;
        solution = sol->solution;
        nsol     = sol->ncoil;
    }
    else {
        solution = m->solution;
        nsol     = all_surfs ? m->nsol : m->surfs[0]->ntri;
    }
    for (k = 0; k < nsol; k++)
        pot[k] = mne_dot_vectors_3(solution[k],v0,m->nsol);
    return;
}


int fwd_bem_pot_els (float       *rd,	  /* Dipole position */
                     float       *Q,	  /* Dipole orientation */
                     FwdCoilSet*  els,     /* Electrode descriptors */
                     float       *pot,    /* Result */
                     void        *client) /* The model */
/*
 * This version calculates the potential on all surfaces
 */
{
    fwdBemModel    m = (fwdBemModel)client;
    fwdBemSolution sol = (fwdBemSolution)els->user_data;

    if (!m) {
        printf("No BEM model specified to fwd_bem_pot_els");
        return FAIL;
    }
    if (!m->solution) {
        printf("No solution available for fwd_bem_pot_els");
        return FAIL;
    }
    if (!sol || sol->ncoil != els->ncoil) {
        printf("No appropriate electrode-specific data available in fwd_bem_pot_coils");
        return FAIL;
    }
    if (m->bem_method == FWD_BEM_CONSTANT_COLL)
        fwd_bem_pot_calc(rd,Q,m,els,FALSE,pot);
    else if (m->bem_method == FWD_BEM_LINEAR_COLL)
        fwd_bem_lin_pot_calc(rd,Q,m,els,FALSE,pot);
    else {
        printf("Unknown BEM method : %d",m->bem_method);
        return FAIL;
    }
    return OK;
}








//============================= fwd_bem_field.c =============================


/*
 * These are some of the integration formulas listed in
 *
 * L. Urankar, Common compact analytical formulas for computation of
 * geometry integrals on a basic Cartesian sub-domain in boundary and
 * volume integral methods, Engineering Analysis with Boundary Elements,
 * 7 (3), 1990, 124 - 129.
 *
 */
#define ARSINH(x) log((x) + sqrt(1.0+(x)*(x)))


static void calc_f (double *xx,
                    double *yy,		/* Corner coordinates */
                    double *f0,
                    double *fx,
                    double *fy)	        /* The weights in the linear approximation */

{
    double det = -xx[Y_3]*yy[X_3] + xx[Z_3]*yy[X_3] +
            xx[X_3]*yy[Y_3] - xx[Z_3]*yy[Y_3] - xx[X_3]*yy[Z_3] + xx[Y_3]*yy[Z_3];
    int k;

    f0[X_3] = -xx[Z_3]*yy[Y_3] + xx[Y_3]*yy[Z_3];
    f0[Y_3] = xx[Z_3]*yy[X_3] - xx[X_3]*yy[Z_3];
    f0[Z_3] = -xx[Y_3]*yy[X_3] + xx[X_3]*yy[Y_3];

    fx[X_3] =  yy[Y_3] - yy[Z_3];
    fx[Y_3] = -yy[X_3] + yy[Z_3];
    fx[Z_3] = yy[X_3] - yy[Y_3];

    fy[X_3] = -xx[Y_3] + xx[Z_3];
    fy[Y_3] = xx[X_3] - xx[Z_3];
    fy[Z_3] = -xx[X_3] + xx[Y_3];

    for (k = 0; k < 3; k++) {
        f0[k] = f0[k]/det;
        fx[k] = fx[k]/det;
        fy[k] = fy[k]/det;
    }
}



static void calc_magic (double u,double z,
                        double A,
                        double B,
                        double *beta,
                        double *D)
/*
      * Calculate Urankar's magic numbers
      */
{
    double B2 = 1.0 + B*B;
    double ABu = A + B*u;
    *D = sqrt(u*u + z*z + ABu*ABu);
    beta[0] = ABu/sqrt(u*u + z*z);
    beta[1] = (A*B + B2*u)/sqrt(A*A + B2*z*z);
    beta[2] = (B*z*z - A*u)/(z*(*D));
}


static void field_integrals (float *from,
                             mneTriangle to,
                             double *I1p,
                             double *T,double *S1,double *S2,
                             double *f0,double *fx,double *fy)

{
    double y1[3],y2[3],y3[3];
    double xx[4],yy[4];
    double A,B,z,dx;
    double beta[3],I1,Tx,Ty,Txx,Tyy,Sxx,mult;
    double S1x,S1y,S2x,S2y;
    double D1,B2;
    int k;
    /*
   * Preliminaries...
   *
   * 1. Move origin to viewpoint...
   *
   */
    VEC_DIFF_3(from,to->r1,y1);
    VEC_DIFF_3(from,to->r2,y2);
    VEC_DIFF_3(from,to->r3,y3);
    /*
   * 2. Calculate local xy coordinates...
   */
    xx[0] = VEC_DOT_3(y1,to->ex);
    xx[1] = VEC_DOT_3(y2,to->ex);
    xx[2] = VEC_DOT_3(y3,to->ex);
    xx[3] = xx[0];

    yy[0] = VEC_DOT_3(y1,to->ey);
    yy[1] = VEC_DOT_3(y2,to->ey);
    yy[2] = VEC_DOT_3(y3,to->ey);
    yy[3] = yy[0];

    calc_f (xx,yy,f0,fx,fy);
    /*
   * 3. Distance of the plane from origin...
   */
    z = VEC_DOT_3(y1,to->nn);
    /*
   * Put together the line integral...
   * We use the convention where the local y-axis
   * is parallel to the last side and, therefore, dx = 0
   * on that side. We can thus omit the last side from this
   * computation in some cases.
   */
    I1 = 0.0;
    Tx = 0.0;
    Ty = 0.0;
    S1x = 0.0;
    S1y = 0.0;
    S2x = 0.0;
    S2y = 0.0;
    for (k = 0; k < 2; k++) {
        dx = xx[k+1] - xx[k];
        A = (yy[k]*xx[k+1] - yy[k+1]*xx[k])/dx;
        B = (yy[k+1]-yy[k])/dx;
        B2 = (1.0 + B*B);
        /*
     * Upper limit
     */
        calc_magic (xx[k+1],z,A,B,beta,&D1);
        I1 = I1 - xx[k+1]*ARSINH(beta[0]) - (A/sqrt(1.0+B*B))*ARSINH(beta[1])
                - z*atan(beta[2]);
        Txx = ARSINH(beta[1])/sqrt(B2);
        Tx = Tx + Txx;
        Ty = Ty + B*Txx;
        Sxx = (D1 - A*B*Txx)/B2;
        S1x = S1x + Sxx;
        S1y = S1y + B*Sxx;
        Sxx = (B*D1 + A*Txx)/B2;
        S2x = S2x + Sxx;
        /*
     * Lower limit
     */
        calc_magic (xx[k],z,A,B,beta,&D1);
        I1 = I1 + xx[k]*ARSINH(beta[0]) + (A/sqrt(1.0+B*B))*ARSINH(beta[1])
                + z*atan(beta[2]);
        Txx = ARSINH(beta[1])/sqrt(B2);
        Tx = Tx - Txx;
        Ty = Ty - B*Txx;
        Sxx = (D1 - A*B*Txx)/B2;
        S1x = S1x - Sxx;
        S1y = S1y - B*Sxx;
        Sxx = (B*D1 + A*Txx)/B2;
        S2x = S2x - Sxx;
    }
    /*
   * Handle last side (dx = 0) in a special way;
   */
    mult = 1.0/sqrt(xx[k]*xx[k]+z*z);
    /*
   * Upper...
   */
    Tyy = ARSINH(mult*yy[k+1]);
    Ty = Ty + Tyy;
    S1y = S1y + xx[k]*Tyy;
    /*
   * Lower...
   */
    Tyy = ARSINH(mult*yy[k]);
    Ty = Ty - Tyy;
    S1y = S1y - xx[k]*Tyy;
    /*
   * Set return values
   */
    *I1p = I1;
    T[X_3] = Tx;
    T[Y_3] = Ty;
    S1[X_3] = S1x;
    S1[Y_3] = S1y;
    S2[X_3] = S2x;
    S2[Y_3] = -S1x;
    return;
}


static double one_field_coeff (float       *dest,	/* The destination field point */
                               float       *normal,	/* The field direction we are interested in */
                               mneTriangle tri)
/*
 * Compute the integral over one triangle.
 * This looks magical but it is not.
 */
{
    double *yy[4];
    double y1[3],y2[3],y3[3];
    double beta[3];
    double bbeta[3];
    double coeff[3];
    int   j,k;

    yy[0] = y1;
    yy[1] = y2;
    yy[2] = y3;
    yy[3] = y1;
    VEC_DIFF_3(dest,tri->r1,y1);
    VEC_DIFF_3(dest,tri->r2,y2);
    VEC_DIFF_3(dest,tri->r3,y3);
    for (j = 0; j < 3; j++)
        beta[j] = calc_beta(yy[j],yy[j+1]);
    bbeta[0] = beta[2] - beta[0];
    bbeta[1] = beta[0] - beta[1];
    bbeta[2] = beta[1] - beta[2];

    for (j = 0; j < 3; j++)
        coeff[j] = 0.0;
    for (j = 0; j < 3; j++)
        for (k = 0; k < 3; k++)
            coeff[k] = coeff[k] + yy[j][k]*bbeta[j];
    return (VEC_DOT_3(coeff,normal));
}



float **fwd_bem_field_coeff(fwdBemModel m,	/* The model */
                            FwdCoilSet*  coils)	/* Gradiometer coil positions */
/*
 * Compute the weighting factors to obtain the magnetic field
 */
{
    MneSurfaceOrVolume::MneCSurface*     surf;
    mneTriangle    tri;
    FwdCoil*        coil;
    FwdCoilSet*     tcoils = NULL;
    int            ntri;
    float          **coeff = NULL;
    int            j,k,p,s,off;
    double         res;
    double         mult;

    if (m->solution == NULL) {
        printf("Solution matrix missing in fwd_bem_field_coeff");
        return NULL;
    }
    if (m->bem_method != FWD_BEM_CONSTANT_COLL) {
        printf("BEM method should be constant collocation for fwd_bem_field_coeff");
        return NULL;
    }
    if (coils->coord_frame != FIFFV_COORD_MRI) {
        if (coils->coord_frame == FIFFV_COORD_HEAD) {
            if (!m->head_mri_t) {
                printf("head -> mri coordinate transform missing in fwd_bem_field_coeff");
                return NULL;
            }
            else {
                if (!coils) {
                    qWarning("No coils to duplicate");
                    return NULL;
                }
                /*
                * Make a transformed duplicate
                */
                if ((tcoils = coils->dup_coil_set(m->head_mri_t)) == NULL)
                    return NULL;
                coils = tcoils;
            }
        }
        else {
            printf("Incompatible coil coordinate frame %d for fwd_bem_field_coeff",coils->coord_frame);
            return NULL;
        }
    }
    ntri  = m->nsol;
    coeff = ALLOC_CMATRIX_3(coils->ncoil,ntri);

    for (s = 0, off = 0; s < m->nsurf; s++) {
        surf = m->surfs[s];
        ntri = surf->ntri;
        tri  = surf->tris;
        mult = m->field_mult[s];

        for (k = 0; k < ntri; k++,tri++) {
            for (j = 0; j < coils->ncoil; j++) {
                coil = coils->coils[j];
                res = 0.0;
                for (p = 0; p < coil->np; p++)
                    res = res + coil->w[p]*one_field_coeff(coil->rmag[p],coil->cosmag[p],tri);
                coeff[j][k+off] = mult*res;
            }
        }
        off = off + ntri;
    }
    delete tcoils;
    return coeff;
}



/*
 * These are the formulas from Ferguson et al
 * A Complete Linear Discretization for Calculating the Magnetic Field
 * Using the Boundary-Element Method, IEEE Trans. Biomed. Eng., submitted
 */

static double calc_gamma (double *rk,double *rk1)

{
    double rkk1[3];
    double size;
    double res;

    VEC_DIFF_3(rk,rk1,rkk1);
    size = VEC_LEN_3(rkk1);

    res = log((VEC_LEN_3(rk1)*size + VEC_DOT_3(rk1,rkk1))/
              (VEC_LEN_3(rk)*size + VEC_DOT_3(rk,rkk1)))/size;
    return (res);
}




void fwd_bem_one_lin_field_coeff_ferg (float *dest,	/* The field point */
                                       float *dir,	/* The interesting direction */
                                       mneTriangle tri,	/* The destination triangle */
                                       double *res)	/* The results */

{
    double c[3];			/* Component of dest vector normal to
                                 * the triangle plane */
    double A[3];			/* Projection of dest onto the triangle */
    double c1[3],c2[3],c3[3];
    double y1[3],y2[3],y3[3];
    double *yy[4],*cc[4];
    double rjk[3][3];
    double cross[3],triple,l1,l2,l3,solid,clen;
    double common,sum,beta,gamma;
    int    k;

    yy[0] = y1;   cc[0] = c1;
    yy[1] = y2;   cc[1] = c2;
    yy[2] = y3;   cc[2] = c3;
    yy[3] = y1;   cc[3] = c1;

    VEC_DIFF_3(tri->r2,tri->r3,rjk[0]);
    VEC_DIFF_3(tri->r3,tri->r1,rjk[1]);
    VEC_DIFF_3(tri->r1,tri->r2,rjk[2]);

    for (k = 0; k < 3; k++) {
        y1[k] = tri->r1[k] - dest[k];
        y2[k] = tri->r2[k] - dest[k];
        y3[k] = tri->r3[k] - dest[k];
    }
    clen  = VEC_DOT_3(y1,tri->nn);
    for (k = 0; k < 3; k++) {
        c[k]  = clen*tri->nn[k];
        A[k]  = dest[k] + c[k];
        c1[k] = tri->r1[k] - A[k];
        c2[k] = tri->r2[k] - A[k];
        c3[k] = tri->r3[k] - A[k];
    }
    /*
   * beta and gamma...
   */
    for (sum = 0.0, k = 0; k < 3; k++) {
        CROSS_PRODUCT_3(cc[k],cc[k+1],cross);
        beta  = VEC_DOT_3(cross,tri->nn);
        gamma = calc_gamma (yy[k],yy[k+1]);
        sum = sum + beta*gamma;
    }
    /*
   * Solid angle...
   */
    CROSS_PRODUCT_3(y1,y2,cross);
    triple = VEC_DOT_3(cross,y3);

    l1 = VEC_LEN_3(y1);
    l2 = VEC_LEN_3(y2);
    l3 = VEC_LEN_3(y3);
    solid = 2.0*atan2(triple,
                      (l1*l2*l3+
                       VEC_DOT_3(y1,y2)*l3+
                       VEC_DOT_3(y1,y3)*l2+
                       VEC_DOT_3(y2,y3)*l1));
    /*
   * Now we are ready to assemble it all together
   */
    common = (sum-clen*solid)/(2.0*tri->area);
    for (k = 0; k < 3; k++)
        res[k] = -VEC_DOT_3(rjk[k],dir)*common;
    return;
}





void fwd_bem_one_lin_field_coeff_uran(float *dest,	/* The field point */
                                      float *dir,	/* The interesting direction */
                                      mneTriangle tri,	/* The destination triangle */
                                      double *res)	/* The results */

{
    double      I1,T[2],S1[2],S2[2];
    double      f0[3],fx[3],fy[3];
    double      res_x,res_y;
    double      x_fac,y_fac;
    int         k;
    double      len;
    /*
   * Compute the component integrals
   */
    field_integrals (dest,tri,&I1,T,S1,S2,f0,fx,fy);
    /*
   * Compute the coefficient for each node...
   */
    len = VEC_LEN_3(dir);
    dir[X_3] = dir[X_3]/len;
    dir[Y_3] = dir[Y_3]/len;
    dir[Z_3] = dir[Z_3]/len;

    x_fac = -VEC_DOT_3(dir,tri->ex);
    y_fac = -VEC_DOT_3(dir,tri->ey);
    for (k = 0; k < 3; k++) {
        res_x = f0[k]*T[X_3] + fx[k]*S1[X_3] + fy[k]*S2[X_3] + fy[k]*I1;
        res_y = f0[k]*T[Y_3] + fx[k]*S1[Y_3] + fy[k]*S2[Y_3] - fx[k]*I1;
        res[k] = x_fac*res_x + y_fac*res_y;
    }
    return;
}


void fwd_bem_one_lin_field_coeff_simple (float       *dest,    /* The destination field point */
                                         float       *normal,  /* The field direction we are interested in */
                                         mneTriangle source,   /* The source triangle */
                                         double      *res)     /* The result for each triangle node */
/*
      * Simple version...
      */
{
    float diff[3];
    float vec_result[3];
    float dl;
    int   k;
    float *rr[3];


    rr[0] = source->r1;
    rr[1] = source->r2;
    rr[2] = source->r3;

    for (k = 0; k < 3; k++) {
        VEC_DIFF_3(rr[k],dest,diff);
        dl = VEC_DOT_3(diff,diff);
        CROSS_PRODUCT_3(diff,source->nn,vec_result);
        res[k] = source->area*VEC_DOT_3(vec_result,normal)/(3.0*dl*sqrt(dl));
    }
    return;
}

typedef void (* linFieldIntFunc)(float *dest,float *dir,mneTriangle tri, double *res);

float **fwd_bem_lin_field_coeff (fwdBemModel m,	        /* The model */
                                 FwdCoilSet*  coils,	/* Coil information */
                                 int         method)	/* Which integration formula to use */
/*
      * Compute the weighting factors to obtain the magnetic field
      * in the linear potential approximation
      */
{
    MneSurfaceOrVolume::MneCSurface*  surf;
    mneTriangle tri;
    FwdCoil*     coil;
    FwdCoilSet*  tcoils = NULL;
    int         ntri;
    float       **coeff  = NULL;
    int         j,k,p,pp,off,s;
    double      res[3],one[3];
    float       mult;
    linFieldIntFunc func;

    if (m->solution == NULL) {
        printf("Solution matrix missing in fwd_bem_lin_field_coeff");
        return NULL;
    }
    if (m->bem_method != FWD_BEM_LINEAR_COLL) {
        printf("BEM method should be linear collocation for fwd_bem_lin_field_coeff");
        return NULL;
    }
    if (coils->coord_frame != FIFFV_COORD_MRI) {
        if (coils->coord_frame == FIFFV_COORD_HEAD) {
            if (!m->head_mri_t) {
                printf("head -> mri coordinate transform missing in fwd_bem_lin_field_coeff");
                return NULL;
            }
            else {
                if (!coils) {
                    qWarning("No coils to duplicate");
                    return NULL;
                }
                /*
                * Make a transformed duplicate
                */
                if ((tcoils = coils->dup_coil_set(m->head_mri_t)) == NULL)
                    return NULL;
                coils = tcoils;
            }
        }
        else {
            printf("Incompatible coil coordinate frame %d for fwd_bem_field_coeff",coils->coord_frame);
            return NULL;
        }
    }
    if (method == FWD_BEM_LIN_FIELD_FERGUSON)
        func = fwd_bem_one_lin_field_coeff_ferg;
    else if (method == FWD_BEM_LIN_FIELD_URANKAR)
        func = fwd_bem_one_lin_field_coeff_uran;
    else
        func = fwd_bem_one_lin_field_coeff_simple;

    coeff = ALLOC_CMATRIX_3(coils->ncoil,m->nsol);
    for (k = 0; k < m->nsol; k++)
        for (j = 0; j < coils->ncoil; j++)
            coeff[j][k] = 0.0;
    /*
   * Process each of the surfaces
   */
    for (s = 0, off = 0; s < m->nsurf; s++) {
        surf = m->surfs[s];
        ntri = surf->ntri;
        tri  = surf->tris;
        mult = m->field_mult[s];

        for (k = 0; k < ntri; k++,tri++) {
            for (j = 0; j < coils->ncoil; j++) {
                coil = coils->coils[j];
                for (pp = 0; pp < 3; pp++)
                    res[pp] = 0;
                /*
         * Accumulate the coefficients for each triangle node...
         */
                for (p = 0; p < coil->np; p++) {
                    func(coil->rmag[p],coil->cosmag[p],tri,one);
                    for (pp = 0; pp < 3; pp++)
                        res[pp] = res[pp] + coil->w[p]*one[pp];
                }
                /*
         * Add these to the corresponding coefficient matrix
         * elements...
         */
                for (pp = 0; pp < 3; pp++)
                    coeff[j][tri->vert[pp]+off] = coeff[j][tri->vert[pp]+off] + mult*res[pp];
            }
        }
        off = off + surf->np;
    }
    /*
   * Discard the duplicate
   */
    delete tcoils;
    return (coeff);
}



int fwd_bem_specify_coils(fwdBemModel m,
                          FwdCoilSet*  coils)
/*
 * Set up for computing the solution at a set of coils
  */
{
    float **sol = NULL;
    fwdBemSolution csol;

    if (!m) {
        printf("Model missing in fwd_bem_specify_coils");
        goto bad;
    }
    if (!m->solution) {
        printf("Solution not computed in fwd_bem_specify_coils");
        goto bad;
    }
    if(coils)
        coils->fwd_free_coil_set_user_data();
    if (!coils || coils->ncoil == 0)
        return OK;
    if (m->bem_method == FWD_BEM_CONSTANT_COLL)
        sol = fwd_bem_field_coeff(m,coils);
    else if (m->bem_method == FWD_BEM_LINEAR_COLL)
        sol = fwd_bem_lin_field_coeff(m,coils,FWD_BEM_LIN_FIELD_SIMPLE);
    else {
        printf("Unknown BEM method in fwd_bem_specify_coils : %d",m->bem_method);
        goto bad;
    }
    coils->user_data = csol = fwd_bem_new_coil_solution();
    coils->user_data_free   = fwd_bem_free_coil_solution;

    csol->ncoil     = coils->ncoil;
    csol->np        = m->nsol;
    csol->solution  = mne_mat_mat_mult_3(sol,m->solution,coils->ncoil,m->nsol,m->nsol);

    FREE_CMATRIX_3(sol);
    return OK;

bad : {
        FREE_CMATRIX_3(sol);
        return FAIL;

    }
}




#define MAG_FACTOR 1e-7		/* \mu_0/4\pi */




static void fwd_bem_lin_field_calc(float       *rd,
                                   float       *Q,
                                   FwdCoilSet*  coils,
                                   fwdBemModel m,
                                   float       *B)
/*
 * Calculate the magnetic field in a set of coils
 */
{
    float *v0;
    int   s,k,p,np;
    FwdCoil* coil;
    float  mult;
    float  **rr;
    float  my_rd[3],my_Q[3];
    fwdBemSolution sol = (fwdBemSolution)coils->user_data;
    /*
   * Infinite-medium potentials
   */
    if (!m->v0)
        m->v0 = MALLOC_3(m->nsol,float);
    v0 = m->v0;
    /*
   * The dipole location and orientation must be transformed
   */
    VEC_COPY_3(my_rd,rd);
    VEC_COPY_3(my_Q,Q);
    if (m->head_mri_t) {
        FiffCoordTransOld::fiff_coord_trans(my_rd,m->head_mri_t,FIFFV_MOVE);
        FiffCoordTransOld::fiff_coord_trans(my_Q,m->head_mri_t,FIFFV_NO_MOVE);
    }
    /*
   * Compute the inifinite-medium potentials at the vertices
   */
    for (s = 0, p = 0; s < m->nsurf; s++) {
        np     = m->surfs[s]->np;
        rr     = m->surfs[s]->rr;
        mult   = m->source_mult[s];
        for (k = 0; k < np; k++)
            v0[p++] = mult*fwd_bem_inf_pot(my_rd,my_Q,rr[k]);
    }
    /*
   * Primary current contribution
   * (can be calculated in the coil/dipole coordinates)
   */
    for (k = 0; k < coils->ncoil; k++) {
        coil = coils->coils[k];
        B[k] = 0.0;
        for (p = 0; p < coil->np; p++)
            B[k] = B[k] + coil->w[p]*fwd_bem_inf_field(rd,Q,coil->rmag[p],coil->cosmag[p]);
    }
    /*
   * Volume current contribution
   */
    for (k = 0; k < coils->ncoil; k++)
        B[k] = B[k] + mne_dot_vectors_3(sol->solution[k],v0,m->nsol);
    /*
   * Scale correctly
   */
    for (k = 0; k < coils->ncoil; k++)
        B[k] = MAG_FACTOR*B[k];
    return;
}


static void fwd_bem_field_calc(float       *rd,
                               float       *Q,
                               FwdCoilSet*  coils,
                               fwdBemModel m,
                               float       *B)
/*
 * Calculate the magnetic field in a set of coils
 */
{
    float *v0;
    int   s,k,p,ntri;
    FwdCoil* coil;
    mneTriangle tri;
    float   mult;
    float  my_rd[3],my_Q[3];
    fwdBemSolution sol = (fwdBemSolution)coils->user_data;
    /*
   * Infinite-medium potentials
   */
    if (!m->v0)
        m->v0 = MALLOC_3(m->nsol,float);
    v0 = m->v0;
    /*
   * The dipole location and orientation must be transformed
   */
    VEC_COPY_3(my_rd,rd);
    VEC_COPY_3(my_Q,Q);
    if (m->head_mri_t) {
        FiffCoordTransOld::fiff_coord_trans(my_rd,m->head_mri_t,FIFFV_MOVE);
        FiffCoordTransOld::fiff_coord_trans(my_Q,m->head_mri_t,FIFFV_NO_MOVE);
    }
    /*
   * Compute the inifinite-medium potentials at the centers of the triangles
   */
    for (s = 0, p = 0; s < m->nsurf; s++) {
        ntri = m->surfs[s]->ntri;
        tri  = m->surfs[s]->tris;
        mult = m->source_mult[s];
        for (k = 0; k < ntri; k++, tri++)
            v0[p++] = mult*fwd_bem_inf_pot(my_rd,my_Q,tri->cent);
    }
    /*
   * Primary current contribution
   * (can be calculated in the coil/dipole coordinates)
   */
    for (k = 0; k < coils->ncoil; k++) {
        coil = coils->coils[k];
        B[k] = 0.0;
        for (p = 0; p < coil->np; p++)
            B[k] = B[k] + coil->w[p]*fwd_bem_inf_field(rd,Q,coil->rmag[p],coil->cosmag[p]);
    }
    /*
   * Volume current contribution
   */
    for (k = 0; k < coils->ncoil; k++)
        B[k] = B[k] + mne_dot_vectors_3(sol->solution[k],v0,m->nsol);
    /*
   * Scale correctly
   */
    for (k = 0; k < coils->ncoil; k++)
        B[k] = MAG_FACTOR*B[k];
    return;
}




int fwd_bem_field(float       *rd,	/* Dipole position */
                  float       *Q,	/* Dipole orientation */
                  FwdCoilSet*  coils,    /* Coil descriptors */
                  float       *B,       /* Result */
                  void        *client)  /* The model */
/*
 * This version calculates the magnetic field in a set of coils
 * Call fwd_bem_specify_coils first to establish the coil-specific
 * solution matrix
 */
{
    fwdBemModel m = (fwdBemModel)client;
    fwdBemSolution sol = (fwdBemSolution)coils->user_data;

    if (!m) {
        printf("No BEM model specified to fwd_bem_field");
        return FAIL;
    }
    if (!sol || !sol->solution || sol->ncoil != coils->ncoil) {
        printf("No appropriate coil-specific data available in fwd_bem_field");
        return FAIL;
    }
    if (m->bem_method == FWD_BEM_CONSTANT_COLL)
        fwd_bem_field_calc(rd,Q,coils,m,B);
    else if (m->bem_method == FWD_BEM_LINEAR_COLL)
        fwd_bem_lin_field_calc(rd,Q,coils,m,B);
    else {
        printf("Unknown BEM method : %d",m->bem_method);
        return FAIL;
    }
    return OK;
}
















//============================= mne_lin_proj.c =============================











static char *add_string_3(char *old,char *add)

{
    char *news = NULL;
    if (!old) {
        if (add || strlen(add) > 0)
            news = mne_strdup_3(add);
    }
    else {
        old = REALLOC_3(old,strlen(old) + strlen(add) + 1,char);
        strcat(old,add);
        news = old;
    }
    return news;
}




void mne_proj_op_report_data_3(FILE *out,const char *tag, mneProjOp op, int list_data,
                             char **exclude, int nexclude)
/*
* Output info about the projection operator
*/
{
    int j,k,p,q;
    MneProjItem* it;
    MneNamedMatrix* vecs;
    int found;

    if (out == NULL)
        return;
    if (op == NULL)
        return;
    if (op->nitems <= 0) {
        fprintf(out,"Empty operator\n");
        return;
    }

    for (k = 0; k < op->nitems; k++) {
        it = op->items[k];
        if (list_data && tag)
            fprintf(out,"%s\n",tag);
        if (tag)
            fprintf(out,"%s",tag);
        fprintf(out,"# %d : %s : %d vecs : %d chs %s %s\n",
                k+1,it->desc,it->nvec,it->vecs->ncol,
                it->has_meg ? "MEG" : "EEG",
                it->active ? "active" : "idle");
        if (list_data && tag)
            fprintf(out,"%s\n",tag);
        if (list_data) {
            vecs = op->items[k]->vecs;

            for (q = 0; q < vecs->ncol; q++) {
                fprintf(out,"%-10s",vecs->collist[q]);
                fprintf(out,q < vecs->ncol-1 ? " " : "\n");
            }
            for (p = 0; p < vecs->nrow; p++)
                for (q = 0; q < vecs->ncol; q++) {
                    for (j = 0, found  = 0; j < nexclude; j++) {
                        if (strcmp(exclude[j],vecs->collist[q]) == 0) {
                            found = 1;
                            break;
                        }
                    }
                    fprintf(out,"%10.5g ",found ? 0.0 : vecs->data[p][q]);
                    fprintf(out,q < vecs->ncol-1 ? " " : "\n");
                }
            if (list_data && tag)
                fprintf(out,"%s\n",tag);
        }
    }
    return;
}


void mne_proj_op_report_3(FILE *out,const char *tag, mneProjOp op)
{
    mne_proj_op_report_data_3(out,tag,op, FALSE, NULL, 0);
}








mneProjOp mne_proj_op_combine_3(mneProjOp to, mneProjOp from)
/*
* Copy items from 'from' operator to 'to' operator
*/
{
    int k;
    MneProjItem* it;

    if (to == NULL)
        to = mne_new_proj_op_3();
    if (from) {
        for (k = 0; k < from->nitems; k++) {
            it = from->items[k];
            mne_proj_op_add_item_3(to,it->vecs,it->kind,it->desc);
            to->items[to->nitems-1]->active_file = it->active_file;
        }
    }
    return to;
}







mneProjOp mne_proj_op_average_eeg_ref_3(fiffChInfo chs,
                                      int nch)
/*
* Make the projection operator for average electrode reference
*/
{
    int eegcount = 0;
    int k;
    float       **vec_data;
    char        **names;
    MneNamedMatrix* vecs;
    mneProjOp      op;

    for (k = 0; k < nch; k++)
        if (chs[k].kind == FIFFV_EEG_CH)
            eegcount++;
    if (eegcount == 0) {
        qCritical("No EEG channels specified for average reference.");
        return NULL;
    }

    vec_data = ALLOC_CMATRIX_3(1,eegcount);
    names    = MALLOC_3(eegcount,char *);

    for (k = 0, eegcount = 0; k < nch; k++)
        if (chs[k].kind == FIFFV_EEG_CH)
            names[eegcount++] = mne_strdup_3(chs[k].ch_name);

    for (k = 0; k < eegcount; k++)
        vec_data[0][k] = 1.0/sqrt((double)eegcount);

    vecs = MneNamedMatrix::build_named_matrix(1,eegcount,NULL,names,vec_data);

    op = mne_new_proj_op_3();
    mne_proj_op_add_item_3(op,vecs,FIFFV_MNE_PROJ_ITEM_EEG_AVREF,"Average EEG reference");

    return op;
}



int mne_proj_item_affect_3(MneProjItem* it, char **list, int nlist)
/*
* Does this projection item affect this list of channels?
*/
{
    int k,p,q;

    if (it == NULL || it->vecs == NULL || it->nvec == 0)
        return FALSE;

    for (k = 0; k < nlist; k++)
        for (p = 0; p < it->vecs->ncol; p++)
            if (strcmp(it->vecs->collist[p],list[k]) == 0) {
                for (q = 0; q < it->vecs->nrow; q++) {
                    if (it->vecs->data[q][p] != 0.0)
                        return TRUE;
                }
            }
    return FALSE;
}



int mne_proj_op_affect_3(mneProjOp op, char **list, int nlist)

{
    int k;
    int naff;

    if (!op)
        return 0;

    for (k = 0, naff = 0; k < op->nitems; k++)
        if (op->items[k]->active && mne_proj_item_affect_3(op->items[k],list,nlist))
            naff += op->items[k]->nvec;

    return naff;
}

int mne_proj_op_affect_chs_3(mneProjOp op, fiffChInfo chs, int nch)
{
    char *ch_string;
    int  res;
    char **list;
    int  nlist;


    if (nch == 0)
        return FALSE;
    ch_string = mne_channel_names_to_string_3(chs,nch);
    mne_string_to_name_list_3(ch_string,&list,&nlist);
    FREE_3(ch_string);
    res = mne_proj_op_affect_3(op,list,nlist);
    mne_free_name_list_3(list,nlist);
    return res;
}





//============================= mne_named_vector.c =============================

int mne_pick_from_named_vector_3(mneNamedVector vec, char **names, int nnames, int require_all, float *res)
/*
* Pick the desired elements from the named vector
*/
{
    int found;
    int k,p;

    if (vec->names == 0) {
        qCritical("No names present in vector. Cannot pick.");
        return FAIL;
    }

    for (k = 0; k < nnames; k++)
        res[k] = 0.0;

    for (k = 0; k < nnames; k++) {
        found = 0;
        for (p = 0; p < vec->nvec; p++) {
            if (strcmp(vec->names[p],names[k]) == 0) {
                res[k] = vec->data[p];
                found = TRUE;
                break;
            }
        }
        if (!found && require_all) {
            qCritical("All required elements not found in named vector.");
            return FAIL;
        }
    }
    return OK;
}


//============================= mne_lin_proj_io.c =============================

mneProjOp mne_read_proj_op_from_node_3(//fiffFile in,
                                     FiffStream::SPtr& stream,
                                     const FiffDirNode::SPtr& start)
/*
* Load all the linear projection data
*/
{
    mneProjOp   op     = NULL;
    QList<FiffDirNode::SPtr> proj;
    FiffDirNode::SPtr start_node;
    QList<FiffDirNode::SPtr> items;
    FiffDirNode::SPtr node;
    int         k;
    char        *item_desc,*desc_tag,*lf;
    int         global_nchan,item_nchan,nlist;
    char        **item_names;
    int         item_kind;
    float       **item_vectors = NULL;
    int         item_nvec;
    int         item_active;
    MneNamedMatrix* item;
    FiffTag::SPtr t_pTag;

    if (!stream) {
        qCritical("File not open mne_read_proj_op_from_node");
        goto bad;
    }

    if (!start || start->isEmpty())
        start_node = stream->tree();
    else
        start_node = start;

    op = mne_new_proj_op_3();
    proj = start_node->dir_tree_find(FIFFB_PROJ);
    if (proj.size() == 0 || proj[0]->isEmpty())   /* The caller must recognize an empty projection */
        goto out;
    /*
    * Only the first projection block is recognized
    */
    items = proj[0]->dir_tree_find(FIFFB_PROJ_ITEM);
    if (items.size() == 0 || items[0]->isEmpty())   /* The caller must recognize an empty projection */
        goto out;
    /*
    * Get a common number of channels
    */
    node = proj[0];
    if(!node->find_tag(stream, FIFF_NCHAN, t_pTag))
        global_nchan = 0;
    else {
        global_nchan = *t_pTag->toInt();
//        TAG_FREE_3(tag);
    }
    /*
   * Proceess each item
   */
    for (k = 0; k < items.size(); k++) {
        node = items[k];
        /*
        * Complicated procedure for getting the description
        */
        item_desc = NULL;

        if (node->find_tag(stream, FIFF_NAME, t_pTag)) {
            item_desc = add_string_3(item_desc,(char *)t_pTag->data());
        }

        /*
        * Take the first line of description if it exists
        */
        if (node->find_tag(stream, FIFF_DESCRIPTION, t_pTag)) {
            desc_tag = (char *)t_pTag->data();
            if ((lf = strchr(desc_tag,'\n')) != NULL)
                *lf = '\0';
            printf("###################DEBUG ToDo: item_desc = add_string_3(item_desc," ");");
            item_desc = add_string_3(item_desc,(char *)desc_tag);
            FREE_3(desc_tag);
        }
        /*
        * Possibility to override number of channels here
        */
        if (!node->find_tag(stream, FIFF_NCHAN, t_pTag)) {
            item_nchan = global_nchan;
        }
        else {
            item_nchan = *t_pTag->toInt();
        }
        if (item_nchan <= 0) {
            qCritical("Number of channels incorrectly specified for one of the projection items.");
            goto bad;
        }
        /*
        * Take care of the channel names
        */
        if (!node->find_tag(stream, FIFF_PROJ_ITEM_CH_NAME_LIST, t_pTag))
            goto bad;

        mne_string_to_name_list_3((char *)(t_pTag->data()),&item_names,&nlist);
        if (nlist != item_nchan) {
            printf("Channel name list incorrectly specified for proj item # %d",k+1);
            mne_free_name_list_3(item_names,nlist);
            goto bad;
        }
        /*
        * Kind of item
        */
        if (!node->find_tag(stream, FIFF_PROJ_ITEM_KIND, t_pTag))
            goto bad;
        item_kind = *t_pTag->toInt();
        /*
        * How many vectors
        */
        if (!node->find_tag(stream,FIFF_PROJ_ITEM_NVEC, t_pTag))
            goto bad;
        item_nvec = *t_pTag->toInt();
        /*
        * The projection data
        */
        if (!node->find_tag(stream,FIFF_PROJ_ITEM_VECTORS, t_pTag))
            goto bad;

        MatrixXf tmp_item_vectors = t_pTag->toFloatMatrix().transpose();
        item_vectors = ALLOC_CMATRIX_3(tmp_item_vectors.rows(),tmp_item_vectors.cols());
        fromFloatEigenMatrix_3(tmp_item_vectors, item_vectors);

        /*
        * Is this item active?
        */
        if (node->find_tag(stream, FIFF_MNE_PROJ_ITEM_ACTIVE, t_pTag)) {
            item_active = *t_pTag->toInt();
        }
        else
            item_active = FALSE;
        /*
        * Ready to add
        */
        item = MneNamedMatrix::build_named_matrix(item_nvec,item_nchan,NULL,item_names,item_vectors);
        mne_proj_op_add_item_act_3(op,item,item_kind,item_desc,item_active);
        delete item;
        op->items[op->nitems-1]->active_file = item_active;
    }

out :
    return op;

bad : {
        mne_free_proj_op_3(op);
        return NULL;
    }
}




mneProjOp mne_read_proj_op_3(const QString& name)

{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    if(!stream->open())
        return NULL;

    mneProjOp   res = NULL;

    FiffDirNode::SPtr t_default;
    res = mne_read_proj_op_from_node_3(stream,t_default);

    stream->close();

    return res;
}







int mne_proj_op_chs_3(mneProjOp op, char **list, int nlist)

{
    if (op == NULL)
        return OK;

    mne_free_proj_op_proj_3(op);  /* These data are not valid any more */

    if (nlist == 0)
        return OK;

    op->names = mne_dup_name_list_3(list,nlist);
    op->nch   = nlist;

    return OK;
}






static void clear_these(float *data, char **names, int nnames, const char *start)

{
    int k;
    for (k = 0; k < nnames; k++)
        if (strstr(names[k],start) == names[k])
            data[k] = 0.0;
}



#define USE_LIMIT   1e-5
#define SMALL_VALUE 1e-4

int mne_proj_op_make_proj_bad(mneProjOp op, char **bad, int nbad)
/*
* Do the channel picking and SVD
* Include a bad list at this phase
* Input to the projection can include the bad channels
* but they are not affected
*/
{
    int   k,p,q,r,nvec;
    float **vv_meg  = NULL;
    float *sing_meg = NULL;
    float **vv_eeg  = NULL;
    float *sing_eeg = NULL;
    float **mat_meg = NULL;
    float **mat_eeg = NULL;
    int   nvec_meg;
    int   nvec_eeg;
    mneNamedVectorRec vec;
    float size;
    int   nzero;
#ifdef DEBUG
    char  name[20];
#endif

    FREE_CMATRIX_3(op->proj_data);
    op->proj_data = NULL;
    op->nvec      = 0;

    if (op->nch <= 0)
        return OK;
    if (op->nitems <= 0)
        return OK;

    nvec = mne_proj_op_affect_3(op,op->names,op->nch);
    if (nvec == 0)
        return OK;

    mat_meg = ALLOC_CMATRIX_3(nvec, op->nch);
    mat_eeg = ALLOC_CMATRIX_3(nvec, op->nch);

#ifdef DEBUG
    fprintf(stdout,"mne_proj_op_make_proj_bad\n");
#endif
    for (k = 0, nvec_meg = nvec_eeg = 0; k < op->nitems; k++) {
        if (op->items[k]->active && mne_proj_item_affect_3(op->items[k],op->names,op->nch)) {
            vec.nvec  = op->items[k]->vecs->ncol;
            vec.names = op->items[k]->vecs->collist;
            if (op->items[k]->has_meg) {
                for (p = 0; p < op->items[k]->nvec; p++, nvec_meg++) {
                    vec.data = op->items[k]->vecs->data[p];
                    if (mne_pick_from_named_vector_3(&vec,op->names,op->nch,FALSE,mat_meg[nvec_meg]) == FAIL)
                        goto bad;
#ifdef DEBUG
                    fprintf(stdout,"Original MEG:\n");
                    mne_print_vector(stdout,op->items[k]->desc,mat_meg[nvec_meg],op->nch);
                    fflush(stdout);
#endif
                }
            }
            else if (op->items[k]->has_eeg) {
                for (p = 0; p < op->items[k]->nvec; p++, nvec_eeg++) {
                    vec.data = op->items[k]->vecs->data[p];
                    if (mne_pick_from_named_vector_3(&vec,op->names,op->nch,FALSE,mat_eeg[nvec_eeg]) == FAIL)
                        goto bad;
#ifdef DEBUG
                    fprintf (stdout,"Original EEG:\n");
                    mne_print_vector(stdout,op->items[k]->desc,mat_eeg[nvec_eeg],op->nch);
                    fflush(stdout);
#endif
                }
            }
        }
    }
    /*
    * Replace bad channel entries with zeroes
    */
    for (q = 0; q < nbad; q++)
        for (r = 0; r < op->nch; r++)
            if (strcmp(op->names[r],bad[q]) == 0) {
                for (p = 0; p < nvec_meg; p++)
                    mat_meg[p][r] = 0.0;
                for (p = 0; p < nvec_eeg; p++)
                    mat_eeg[p][r] = 0.0;
            }
    /*
    * Scale the rows so that detection of linear dependence becomes easy
    */
    for (p = 0, nzero = 0; p < nvec_meg; p++) {
        size = sqrt(mne_dot_vectors_3(mat_meg[p],mat_meg[p],op->nch));
        if (size > 0) {
            for (k = 0; k < op->nch; k++)
                mat_meg[p][k] = mat_meg[p][k]/size;
        }
        else
            nzero++;
    }
    if (nzero == nvec_meg) {
        FREE_CMATRIX_3(mat_meg); mat_meg = NULL; nvec_meg = 0;
    }
    for (p = 0, nzero = 0; p < nvec_eeg; p++) {
        size = sqrt(mne_dot_vectors_3(mat_eeg[p],mat_eeg[p],op->nch));
        if (size > 0) {
            for (k = 0; k < op->nch; k++)
                mat_eeg[p][k] = mat_eeg[p][k]/size;
        }
        else
            nzero++;
    }
    if (nzero == nvec_eeg) {
        FREE_CMATRIX_3(mat_eeg); mat_eeg = NULL; nvec_eeg = 0;
    }
    if (nvec_meg + nvec_eeg == 0) {
        fprintf(stderr,"No projection remains after excluding bad channels. Omitting projection.\n");
        return OK;
    }
    /*
    * Proceed to SVD
    */
#ifdef DEBUG
    fprintf(stdout,"Before SVD:\n");
#endif
    if (nvec_meg > 0) {
#ifdef DEBUG
        fprintf(stdout,"---->>\n");
        for (p = 0; p < nvec_meg; p++) {
            sprintf(name,"MEG %02d",p+1);
            mne_print_vector(stdout,name,mat_meg[p],op->nch);
        }
        fprintf(stdout,"---->>\n");
#endif
        sing_meg = MALLOC_3(nvec_meg+1,float);
        vv_meg   = ALLOC_CMATRIX_3(nvec_meg,op->nch);
        if (mne_svd_3(mat_meg,nvec_meg,op->nch,sing_meg,NULL,vv_meg) != OK)
            goto bad;
    }
    if (nvec_eeg > 0) {
#ifdef DEBUG
        fprintf(stdout,"---->>\n");
        for (p = 0; p < nvec_eeg; p++) {
            sprintf(name,"EEG %02d",p+1);
            mne_print_vector(stdout,name,mat_eeg[p],op->nch);
        }
        fprintf(stdout,"---->>\n");
#endif
        sing_eeg = MALLOC_3(nvec_eeg+1,float);
        vv_eeg   = ALLOC_CMATRIX_3(nvec_eeg,op->nch);
        if (mne_svd_3(mat_eeg,nvec_eeg,op->nch,sing_eeg,NULL,vv_eeg) != OK)
            goto bad;
    }
    /*
    * Check for linearly dependent vectors
    */
    for (p = 0, op->nvec = 0; p < nvec_meg; p++, op->nvec++)
        if (sing_meg[p]/sing_meg[0] < USE_LIMIT)
            break;
    for (p = 0; p < nvec_eeg; p++, op->nvec++)
        if (sing_eeg[p]/sing_eeg[0] < USE_LIMIT)
            break;
#ifdef DEBUG
    fprintf(stderr,"Number of linearly independent vectors = %d\n",op->nvec);
#endif
    op->proj_data = ALLOC_CMATRIX_3(op->nvec,op->nch);
#ifdef DEBUG
    fprintf(stdout,"Final projection data:\n");
#endif
    for (p = 0, op->nvec = 0; p < nvec_meg; p++, op->nvec++) {
        if (sing_meg[p]/sing_meg[0] < USE_LIMIT)
            break;
        for (k = 0; k < op->nch; k++) {
            /*
            * Avoid crosstalk between MEG/EEG
            */
            if (fabs(vv_meg[p][k]) < SMALL_VALUE)
                op->proj_data[op->nvec][k] = 0.0;
            else
                op->proj_data[op->nvec][k] = vv_meg[p][k];

            /*
            * If the above did not work, this will (provided that EEG channels are called EEG*)
            */
            clear_these(op->proj_data[op->nvec],op->names,op->nch,"EEG");
        }
#ifdef DEBUG
        sprintf(name,"MEG %02d",p+1);
        mne_print_vector(stdout,name,op->proj_data[op->nvec],op->nch);
#endif
    }
    for (p = 0; p < nvec_eeg; p++, op->nvec++) {
        if (sing_eeg[p]/sing_eeg[0] < USE_LIMIT)
            break;
        for (k = 0; k < op->nch; k++) {
            /*
            * Avoid crosstalk between MEG/EEG
            */
            if (fabs(vv_eeg[p][k]) < SMALL_VALUE)
                op->proj_data[op->nvec][k] = 0.0;
            else
                op->proj_data[op->nvec][k] = vv_eeg[p][k];
            /*
            * If the above did not work, this will (provided that MEG channels are called MEG*)
            */
            clear_these(op->proj_data[op->nvec],op->names,op->nch,"MEG");
        }
#ifdef DEBUG
        sprintf(name,"EEG %02d",p+1);
        mne_print_vector(stdout,name,op->proj_data[op->nvec],op->nch);
        fflush(stdout);
#endif
    }
    FREE_3(sing_meg);
    FREE_CMATRIX_3(vv_meg);
    FREE_CMATRIX_3(mat_meg);
    FREE_3(sing_eeg);
    FREE_CMATRIX_3(vv_eeg);
    FREE_CMATRIX_3(mat_eeg);
    /*
     * Make sure that the stimulus channels are not modified
     */
    for (k = 0; k < op->nch; k++)
        if (strstr(op->names[k],"STI") == op->names[k]) {
            for (p = 0; p < op->nvec; p++)
                op->proj_data[p][k] = 0.0;
        }

    return OK;

bad : {
        FREE_3(sing_meg);
        FREE_CMATRIX_3(vv_meg);
        FREE_CMATRIX_3(mat_meg);
        FREE_3(sing_eeg);
        FREE_CMATRIX_3(vv_eeg);
        FREE_CMATRIX_3(mat_eeg);
        return FAIL;
    }
}


int mne_proj_op_make_proj(mneProjOp op)
/*
* Do the channel picking and SVD
*/
{
    return mne_proj_op_make_proj_bad(op,NULL,0);
}







//============================= mne_read_forward_solution.c =============================

int mne_read_meg_comp_eeg_ch_info_3(const QString& name,
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
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            nchan = *t_pTag->toInt();
            chs = MALLOC_3(nchan,fiffChInfoRec);
            for (j = 0; j < nchan; j++)
                chs[j].scanNo = -1;
            to_find = nchan;
            break;

        case FIFF_PARENT_BLOCK_ID :
            if(!FiffTag::read_tag(stream, t_pTag, pos))
                goto bad;
//            id = t_pTag->toFiffID();
            *id = *(fiffId)t_pTag->data();
            break;

        case FIFF_COORD_TRANS :
            if(!FiffTag::read_tag(stream, t_pTag, pos))
                goto bad;
//            t = t_pTag->toCoordTrans();
            t = FiffCoordTransOld::read_helper( t_pTag );
            if (t->from != FIFFV_COORD_DEVICE || t->to   != FIFFV_COORD_HEAD)
                t = NULL;
            break;

        case FIFF_CH_INFO : /* Information about one channel */
            if(!FiffTag::read_tag(stream, t_pTag, pos))
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
        meg = MALLOC_3(nmeg,fiffChInfoRec);
    if (neeg > 0)
        eeg = MALLOC_3(neeg,fiffChInfoRec);
    if (nmeg_comp > 0)
        meg_comp = MALLOC_3(nmeg_comp,fiffChInfoRec);
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
    FREE_3(chs);
    if (megp) {
        *megp  = meg;
        *nmegp = nmeg;
    }
    else
        FREE_3(meg);
    if (meg_compp) {
        *meg_compp = meg_comp;
        *nmeg_compp = nmeg_comp;
    }
    else
        FREE_3(meg_comp);
    if (eegp) {
        *eegp  = eeg;
        *neegp = neeg;
    }
    else
        FREE_3(eeg);
    if (idp == NULL) {
        FREE_3(id);
    }
    else
        *idp   = id;
    if (meg_head_t == NULL) {
        FREE_3(t);
    }
    else
        *meg_head_t = t;

    return FIFF_OK;

bad : {
//        fiff_close(in);
        stream->close();
        FREE_3(chs);
        FREE_3(meg);
        FREE_3(eeg);
        FREE_3(id);
//        FREE_3(tag.data);
        FREE_3(t);
        return FIFF_FAIL;
    }
}





//============================= mne_ctf_comp.c =============================


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
} compMap_3[] = { { MNE_CTFV_NOGRAD,       MNE_CTFV_COMP_NONE },
{ MNE_CTFV_GRAD1,        MNE_CTFV_COMP_G1BR },
{ MNE_CTFV_GRAD2,        MNE_CTFV_COMP_G2BR },
{ MNE_CTFV_GRAD3,        MNE_CTFV_COMP_G3BR },
{ MNE_4DV_COMP1,         MNE_4DV_COMP1 },             /* One-to-one mapping for 4D data */
{ MNE_CTFV_COMP_UNKNOWN, MNE_CTFV_COMP_UNKNOWN }};












int mne_unmap_ctf_comp_kind_3(int ctf_comp)

{
    int k;

    for (k = 0; compMap_3[k].grad_comp >= 0; k++)
        if (ctf_comp == compMap_3[k].ctf_comp)
            return compMap_3[k].grad_comp;
    return ctf_comp;
}








/*
 * Allocation and freeing of the data structures
 */
mneCTFcompData mne_new_ctf_comp_data_3()
{
    mneCTFcompData res = MALLOC_3(1,mneCTFcompDataRec);
    res->kind          = MNE_CTFV_COMP_UNKNOWN;
    res->mne_kind      = MNE_CTFV_COMP_UNKNOWN;
    res->calibrated    = FALSE;
    res->data          = NULL;
    res->presel        = NULL;
    res->postsel       = NULL;
    res->presel_data   = NULL;
    res->comp_data     = NULL;
    res->postsel_data  = NULL;

    return res;
}






mneCTFcompDataSet mne_new_ctf_comp_data_set_3()
{
    mneCTFcompDataSet res = MALLOC_3(1,mneCTFcompDataSetRec);

    res->comps   = NULL;
    res->ncomp   = 0;
    res->chs     = NULL;
    res->nch     = 0;
    res->current = NULL;
    res->undo    = NULL;
    return res;
}







void mne_free_ctf_comp_data_3(mneCTFcompData comp)

{
    if (!comp)
        return;

    if(comp->data)
        delete comp->data;
    if(comp->presel)
        delete comp->presel;
    if(comp->postsel)
        delete comp->postsel;
    FREE_3(comp->presel_data);
    FREE_3(comp->postsel_data);
    FREE_3(comp->comp_data);
    FREE_3(comp);
    return;
}


void mne_free_ctf_comp_data_set_3(mneCTFcompDataSet set)

{
    int k;

    if (!set)
        return;

    for (k = 0; k < set->ncomp; k++)
        mne_free_ctf_comp_data_3(set->comps[k]);
    FREE_3(set->comps);
    FREE_3(set->chs);
    mne_free_ctf_comp_data_3(set->current);
    FREE_3(set);
    return;
}








/*
 * Mapping from simple integer orders to the mysterious CTF compensation numbers
 */
int mne_map_ctf_comp_kind_3(int grad)
/*
 * Simple mapping
 */
{
    int k;

    for (k = 0; compMap_3[k].grad_comp >= 0; k++)
        if (grad == compMap_3[k].grad_comp)
            return compMap_3[k].ctf_comp;
    return grad;
}






const char *mne_explain_ctf_comp_3(int kind)
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




static int mne_calibrate_ctf_comp_3(mneCTFcompData one,
                                  fiffChInfo     chs,
                                  int            nch,
                                  int            do_it)
/*
* Calibrate or decalibrate a compensation data set
*/
{
    float *col_cals,*row_cals;
    int   j,k,p,found;
    char  *name;
    float **data;

    if (!one)
        return OK;
    if (one->calibrated)
        return OK;

    row_cals = MALLOC_3(one->data->nrow,float);
    col_cals = MALLOC_3(one->data->ncol,float);

    for (j = 0; j < one->data->nrow; j++) {
        name = one->data->rowlist[j];
        found = FALSE;
        for (p = 0; p < nch; p++)
            if (strcmp(name,chs[p].ch_name) == 0) {
                row_cals[j] = chs[p].range*chs[p].cal;
                found = TRUE;
                break;
            }
        if (!found) {
            printf("Channel %s not found. Cannot calibrate the compensation matrix.",name);
            return FAIL;
        }
    }
    for (k = 0; k < one->data->ncol; k++) {
        name = one->data->collist[k];
        found = FALSE;
        for (p = 0; p < nch; p++)
            if (strcmp(name,chs[p].ch_name) == 0) {
                col_cals[k] = chs[p].range*chs[p].cal;
                found = TRUE;
                break;
            }
        if (!found) {
            printf("Channel %s not found. Cannot calibrate the compensation matrix.",name);
            return FAIL;
        }
    }
    data = one->data->data;
    if (do_it) {
        for (j = 0; j < one->data->nrow; j++)
            for (k = 0; k < one->data->ncol; k++)
                data[j][k] = row_cals[j]*data[j][k]/col_cals[k];
    }
    else {
        for (j = 0; j < one->data->nrow; j++)
            for (k = 0; k < one->data->ncol; k++)
                data[j][k] = col_cals[k]*data[j][k]/row_cals[j];
    }
    return OK;
}





int mne_make_ctf_comp_3(mneCTFcompDataSet set,        /* The available compensation data */
                      fiffChInfo        chs,        /* Channels to compensate These may contain channels other than those requiring compensation */
                      int               nch,        /* How many of these */
                      fiffChInfo        compchs,    /* The compensation input channels These may contain channels other than the MEG compensation channels */
                      int               ncomp)      /* How many of these */
/*
* Make compensation data to apply to a set of channels to yield (or uncompensated) compensated data
*/
{
    int *comps = NULL;
    int need_comp;
    int first_comp;
    mneCTFcompData this_comp;
    int  *comp_sel = NULL;
    char **names   = NULL;
    char *name;
    int  j,k,p;

    INVERSELIB::FiffSparseMatrix* presel  = NULL;
    INVERSELIB::FiffSparseMatrix* postsel = NULL;
    MneNamedMatrix*  data    = NULL;

    if (!compchs) {
        compchs = chs;
        ncomp   = nch;
    }
    fprintf(stderr,"Setting up compensation data...\n");
    if (nch == 0)
        return OK;
    if (set) {
        mne_free_ctf_comp_data_3(set->current);
        set->current = NULL;
    }
    comps = MALLOC_3(nch,int);
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
        fprintf(stderr,"\tNo compensation set. Nothing more to do.\n");
        FREE_3(comps);
        return OK;
    }
    fprintf(stderr,"\t%d out of %d channels have the compensation set.\n",need_comp,nch);
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
               mne_explain_ctf_comp_3(mne_map_ctf_comp_kind_3(first_comp)));
        goto bad;
    }
    fprintf(stderr,"\tDesired compensation data (%s) found.\n",mne_explain_ctf_comp_3(mne_map_ctf_comp_kind_3(first_comp)));
    /*
    * Find the compensation channels
    */
    comp_sel = MALLOC_3(this_comp->data->ncol,int);
    for (k = 0; k < this_comp->data->ncol; k++) {
        comp_sel[k] = -1;
        name = this_comp->data->collist[k];
        for (p = 0; p < ncomp; p++)
            if (strcmp(name,compchs[p].ch_name) == 0) {
                comp_sel[k] = p;
                break;
            }
        if (comp_sel[k] < 0) {
            printf("Compensation channel %s not found",name);
            goto bad;
        }
    }
    fprintf(stderr,"\tAll compensation channels found.\n");
    /*
    * Create the preselector
    */
    {
        float **sel = ALLOC_CMATRIX_3(this_comp->data->ncol,ncomp);
        for (j = 0; j < this_comp->data->ncol; j++) {
            for (k = 0; k < ncomp; k++)
                sel[j][k] = 0.0;
            sel[j][comp_sel[j]] = 1.0;
        }
        if ((presel = mne_convert_to_sparse_3(sel,this_comp->data->ncol,ncomp,FIFFTS_MC_RCS,1e-30)) == NULL) {
            FREE_CMATRIX_3(sel);
            goto bad;
        }
        FREE_CMATRIX_3(sel);
        fprintf(stderr,"\tPreselector created.\n");
    }
    /*
    * Pick the desired channels
    */
    names = MALLOC_3(need_comp,char *);
    for (k = 0, p = 0; k < nch; k++) {
        if (comps[k] != MNE_CTFV_COMP_NONE)
            names[p++] = chs[k].ch_name;
    }
    if ((data = this_comp->data->pick_from_named_matrix(names,need_comp,NULL,0)) == NULL)
        goto bad;
    fprintf(stderr,"\tCompensation data matrix created.\n");
    /*
    * Create the postselector
    */
    {
        float **sel = ALLOC_CMATRIX_3(nch,data->nrow);
        for (j = 0, p = 0; j < nch; j++) {
            for (k = 0; k < data->nrow; k++)
                sel[j][k] = 0.0;
            if (comps[j] != MNE_CTFV_COMP_NONE)
                sel[j][p++] = 1.0;
        }
        if ((postsel = mne_convert_to_sparse_3(sel,nch,data->nrow,FIFFTS_MC_RCS,1e-30)) == NULL) {
            FREE_CMATRIX_3(sel);
            goto bad;
        }
        FREE_CMATRIX_3(sel);
        fprintf(stderr,"\tPostselector created.\n");
    }
    set->current           = mne_new_ctf_comp_data_3();
    set->current->kind     = this_comp->kind;
    set->current->mne_kind = this_comp->mne_kind;
    set->current->data     = data;
    set->current->presel   = presel;
    set->current->postsel  = postsel;

    fprintf(stderr,"\tCompensation set up.\n");

    FREE_3(names);
    FREE_3(comps);
    FREE_3(comp_sel);

    return OK;

bad : {
        if(presel)
            delete presel;
        if(postsel)
            delete postsel;
        if(data)
            delete data;
        FREE_3(names);
        FREE_3(comps);
        FREE_3(comp_sel);
        return FAIL;
    }
}











int mne_apply_ctf_comp_3(mneCTFcompDataSet set,		  /* The compensation data */
                       int               do_it,
                       float             *data,           /* The data to process */
                       int               ndata,
                       float             *compdata,       /* Data containing the compensation channels */
                       int               ncompdata)
/*
* Apply compensation or revert to uncompensated data
*/
{
    mneCTFcompData this_comp;
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
            this_comp->presel_data = MALLOC_3(this_comp->presel->m,float);
        if (mne_sparse_vec_mult2_3(this_comp->presel,compdata,this_comp->presel_data) != OK)
            return FAIL;
        presel = this_comp->presel_data;
    }
    else
        presel = compdata;
    /*
    * This always happens
    */
    if (!this_comp->comp_data)
        this_comp->comp_data = MALLOC_3(this_comp->data->nrow,float);
    mne_mat_vec_mult2_3(this_comp->data->data,presel,this_comp->comp_data,this_comp->data->nrow,this_comp->data->ncol);
    /*
    * Optional postselection
    */
    if (!this_comp->postsel)
        comp = this_comp->comp_data;
    else {
        if (!this_comp->postsel_data) {
            this_comp->postsel_data = MALLOC_3(this_comp->postsel->m,float);
        }
        if (mne_sparse_vec_mult2_3(this_comp->postsel,this_comp->comp_data,this_comp->postsel_data) != OK)
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















mneCTFcompDataSet mne_read_ctf_comp_data_3(const QString& name)
/*
* Read all CTF compensation data from a given file
*/
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    mneCTFcompDataSet set = NULL;
    mneCTFcompData    one;
    QList<FiffDirNode::SPtr> nodes;
    QList<FiffDirNode::SPtr> comps;
    int               ncomp;
    MneNamedMatrix*    mat = NULL;
    int               kind,k;
    FiffTag::SPtr t_pTag;
    fiffChInfo        chs = NULL;
    int               nch = 0;
    int               calibrated;
    /*
    * Read the channel information
    */
    {
        fiffChInfo        comp_chs = NULL;
        int               ncompch = 0;

        if (mne_read_meg_comp_eeg_ch_info_3(name,&chs,&nch,&comp_chs,&ncompch,NULL,NULL,NULL,NULL) == FAIL)
            goto bad;
        if (ncompch > 0) {
            chs = REALLOC_3(chs,nch+ncompch,fiffChInfoRec);
            for (k = 0; k < ncompch; k++)
                chs[k+nch] = comp_chs[k];
            nch = nch + ncompch;
            FREE_3(comp_chs);
        }
    }
    /*
    * Read the rest of the stuff
    */
    if(!stream->open())
        goto bad;
    set = mne_new_ctf_comp_data_set_3();
    /*
    * Locate the compensation data sets
    */
    nodes = stream->tree()->dir_tree_find(FIFFB_MNE_CTF_COMP);
    if (nodes.size() == 0)
        goto good;      /* Nothing more to do */
    comps = nodes[0]->dir_tree_find(FIFFB_MNE_CTF_COMP_DATA);
    if (comps.size() == 0)
        goto good;
    ncomp = comps.size();
    /*
    * Set the channel info
    */
    set->chs = chs; chs = NULL;
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
        one = mne_new_ctf_comp_data_3();
        one->data = mat; mat = NULL;
        one->kind                = kind;
        one->mne_kind            = mne_unmap_ctf_comp_kind_3(one->kind);
        one->calibrated          = calibrated;

        if (mne_calibrate_ctf_comp_3(one,set->chs,set->nch,TRUE) == FAIL) {
            printf("Warning: Compensation data for '%s' omitted\n", mne_explain_ctf_comp_3(one->kind));//,err_get_error(),mne_explain_ctf_comp(one->kind));
            mne_free_ctf_comp_data_3(one);
        }
        else {
            set->comps               = REALLOC_3(set->comps,set->ncomp+1,mneCTFcompData);
            set->comps[set->ncomp++] = one;
        }
    }
#ifdef DEBUG
    fprintf(stderr,"%d CTF compensation data sets read from %s\n",set->ncomp,name);
#endif
    goto good;

bad : {
        if(mat)
            delete mat;
        stream->close();
        mne_free_ctf_comp_data_set_3(set);
        return NULL;
    }

good : {
        FREE_3(chs);
        stream->close();
        return set;
    }
}



















//============================= fwd_comp.c =============================




int fwd_comp_field(float *rd,float *Q, FwdCoilSet* coils, float *res, void *client)
/*
      * Calculate the compensated field (one dipole component)
      */
{
    fwdCompData comp = (fwdCompData)client;

    if (!comp->field) {
        printf("Field computation function is missing in fwd_comp_field_vec");
        return FAIL;
    }
    /*
   * First compute the field in the primary set of coils
   */
    if (comp->field(rd,Q,coils,res,comp->client) == FAIL)
        return FAIL;
    /*
   * Compensation needed?
   */
    if (!comp->comp_coils || comp->comp_coils->ncoil <= 0 || !comp->set || !comp->set->current)
        return OK;
    /*
   * Workspace needed?
   */
    if (!comp->work)
        comp->work = MALLOC_3(comp->comp_coils->ncoil,float);
    /*
   * Compute the field in the compensation coils
   */
    if (comp->field(rd,Q,comp->comp_coils,comp->work,comp->client) == FAIL)
        return FAIL;
    /*
   * Compute the compensated field
   */
    return mne_apply_ctf_comp_3(comp->set,TRUE,res,coils->ncoil,comp->work,comp->comp_coils->ncoil);
}



/*
 * Routines to implement the reference channel compensation in field computations
 */

void fwd_free_comp_data(void *d)

{
    fwdCompData comp = (fwdCompData)d;

    if (!comp)
        return;
    delete comp->comp_coils;
    mne_free_ctf_comp_data_set_3(comp->set);
    FREE_3(comp->work);
    FREE_CMATRIX_3(comp->vec_work);

    if (comp->client_free && comp->client)
        comp->client_free(comp->client);

    FREE_3(comp);
    return;
}




fwdCompData fwd_new_comp_data()

{
    fwdCompData comp = MALLOC_3(1,fwdCompDataRec);

    comp->comp_coils  = NULL;
    comp->field       = NULL;
    comp->vec_field   = NULL;
    comp->field_grad  = NULL;
    comp->client      = NULL;
    comp->client_free = NULL;
    comp->set         = NULL;
    comp->work        = NULL;
    comp->vec_work    = NULL;
    return comp;
}




static int fwd_make_ctf_comp_coils(mneCTFcompDataSet set,          /* The available compensation data */
                                   FwdCoilSet*        coils,        /* The main coil set */
                                   FwdCoilSet*        comp_coils)   /* The compensation coil set */
/*
      * Call mne_make_ctf_comp using the information in the coil sets
      */
{
    fiffChInfo chs     = NULL;
    fiffChInfo compchs = NULL;
    int        nchan   = 0;
    int        ncomp   = 0;
    FwdCoil* coil;
    int k,res;

    if (!coils || coils->ncoil <= 0) {
        printf("Coil data missing in fwd_make_ctf_comp_coils");
        return FAIL;
    }
    /*
   * Create the fake channel info which contain just enough information
   * for mne_make_ctf_comp
   */
    chs = MALLOC_3(coils->ncoil,fiffChInfoRec);
    for (k = 0; k < coils->ncoil; k++) {
        coil = coils->coils[k];
        strcpy(chs[k].ch_name,coil->chname);
        chs[k].chpos.coil_type = coil->type;
        chs[k].kind = (coil->coil_class == FWD_COILC_EEG) ? FIFFV_EEG_CH : FIFFV_MEG_CH;
    }
    nchan = coils->ncoil;
    if (comp_coils && comp_coils->ncoil > 0) {
        compchs = MALLOC_3(comp_coils->ncoil,fiffChInfoRec);
        for (k = 0; k < comp_coils->ncoil; k++) {
            coil = comp_coils->coils[k];
            strcpy(compchs[k].ch_name,coil->chname);
            compchs[k].chpos.coil_type = coil->type;
            compchs[k].kind = (coil->coil_class == FWD_COILC_EEG) ? FIFFV_EEG_CH : FIFFV_MEG_CH;
        }
        ncomp = comp_coils->ncoil;
    }
    res = mne_make_ctf_comp_3(set,chs,nchan,compchs,ncomp);

    FREE_3(chs);
    FREE_3(compchs);

    return res;
}







mneCTFcompData mne_dup_ctf_comp_data_3(mneCTFcompData data)
{
    mneCTFcompData res;

    if (!data)
        return NULL;

    res = mne_new_ctf_comp_data_3();

    res->kind       = data->kind;
    res->mne_kind   = data->mne_kind;
    res->calibrated = data->calibrated;
    res->data       = new MneNamedMatrix(*data->data);

    res->presel     = new FiffSparseMatrix(*data->presel);
    res->postsel    = new FiffSparseMatrix(*data->postsel);

    return res;
}




mneCTFcompDataSet mne_dup_ctf_comp_data_set_3(mneCTFcompDataSet set)
/*
* Make a verbatim copy of a data set
*/
{
    mneCTFcompDataSet res;
    int  k;

    if (!set)
        return NULL;

    res = mne_new_ctf_comp_data_set_3();

    if (set->ncomp > 0) {
        res->comps = MALLOC_3(set->ncomp,mneCTFcompData);
        res->ncomp = set->ncomp;
        for (k = 0; k < res->ncomp; k++)
            res->comps[k] = mne_dup_ctf_comp_data_3(set->comps[k]);
    }
    res->current = mne_dup_ctf_comp_data_3(set->current);

    return res;
}





fwdCompData fwd_make_comp_data(mneCTFcompDataSet set,           /* The CTF compensation data read from the file */
                               FwdCoilSet*        coils,         /* The principal set of coils */
                               FwdCoilSet*        comp_coils,    /* The compensation coils */
                               fwdFieldFunc      field,	        /* The field computation functions */
                               fwdVecFieldFunc   vec_field,
                               fwdFieldGradFunc  field_grad,    /* The field and gradient computation function */
                               void              *client,       /* Client data to be passed to the above */
                               fwdUserFreeFunc   client_free)
/*
      * Compose a compensation data set
      */
{
    fwdCompData comp = fwd_new_comp_data();

    comp->set = mne_dup_ctf_comp_data_set_3(set);

    if (comp_coils) {
        comp->comp_coils = comp_coils->dup_coil_set(NULL);
    }
    else {
        qWarning("No coils to duplicate");
        comp->comp_coils = NULL;
    }
    comp->field       = field;
    comp->vec_field   = vec_field;
    comp->field_grad  = field_grad;
    comp->client      = client;
    comp->client_free = client_free;

    if (fwd_make_ctf_comp_coils(comp->set,
                                coils,
                                comp->comp_coils) != OK) {
        fwd_free_comp_data(comp);
        return NULL;
    }
    else {
        return comp;
    }
}



int fwd_comp_field_vec(float *rd, FwdCoilSet* coils, float **res, void *client)
/*
      * Calculate the compensated field (all dipole components)
      */
{
    fwdCompData comp = (fwdCompData)client;
    int k;

    if (!comp->vec_field) {
        printf("Field computation function is missing in fwd_comp_field_vec");
        return FAIL;
    }
    /*
   * First compute the field in the primary set of coils
   */
    if (comp->vec_field(rd,coils,res,comp->client) == FAIL)
        return FAIL;
    /*
   * Compensation needed?
   */
    if (!comp->comp_coils || comp->comp_coils->ncoil <= 0 || !comp->set || !comp->set->current)
        return OK;
    /*
   * Need workspace?
   */
    if (!comp->vec_work)
        comp->vec_work = ALLOC_CMATRIX_3(3,comp->comp_coils->ncoil);
    /*
   * Compute the field at the compensation sensors
   */
    if (comp->vec_field(rd,comp->comp_coils,comp->vec_work,comp->client) == FAIL)
        return FAIL;
    /*
   * Compute the compensated field of three orthogonal dipoles
   */
    for (k = 0; k < 3; k++) {
        if (mne_apply_ctf_comp_3(comp->set,TRUE,res[k],coils->ncoil,comp->vec_work[k],comp->comp_coils->ncoil) == FAIL)
            return FAIL;
    }
    return OK;
}





//============================= fwd_spherefield.c =============================

#define EPS   1e-5		/* Points closer to origin than this many
    meters are considered to be at the
    origin */
#define CEPS       1e-5




int fwd_sphere_field(float        *rd,	        /* The dipole location */
                     float        Q[],	        /* The dipole components (xyz) */
                     FwdCoilSet*   coils,	/* The coil definitions */
                     float        Bval[],	/* Results */
                     void         *client)	/* Client data will be the sphere model origin */

{
    /* This version uses Jukka Sarvas' field computation
     for details, see

     Jukka Sarvas:

     Basic mathematical and electromagnetic concepts
     of the biomagnetic inverse problem,

     Phys. Med. Biol. 1987, Vol. 32, 1, 11-22

     The formulas have been manipulated for efficient computation
     by Matti Hamalainen, February 1990

  */
    float *r0 = (float *)client;      /* The sphere model origin */
    float v[3],a_vec[3];
    float a,a2,r,r2;
    float ar,ar0,rr0;
    float vr,ve,re,r0e;
    float F,g0,gr,result,sum;
    int   j,k,p;
    FwdCoil* this_coil;
    float *this_pos,*this_dir;	/* These point to the coil structure! */
    int   np;
    float myrd[3];
    float pos[3];
    /*
   * Shift to the sphere model coordinates
   */
    for (p = 0; p < 3; p++)
        myrd[p] = rd[p] - r0[p];
    rd = myrd;
    /*
   * Check for a dipole at the origin
   */
    for (k = 0 ; k < coils->ncoil ; k++)
        if (FWD_IS_MEG_COIL(coils->coils[k]->coil_class))
            Bval[k] = 0.0;
    r = VEC_LEN_3(rd);
    if (r > EPS)	{		/* The hard job */

        CROSS_PRODUCT_3(Q,rd,v);

        for (k = 0; k < coils->ncoil; k++) {
            this_coil = coils->coils[k];
            if (FWD_IS_MEG_COIL(this_coil->type)) {

                np = this_coil->np;

                for (j = 0, sum = 0.0; j < np; j++) {

                    this_pos = this_coil->rmag[j];
                    this_dir = this_coil->cosmag[j];

                    for (p = 0; p < 3; p++)
                        pos[p] = this_pos[p] - r0[p];
                    this_pos = pos;
                    result = 0.0;

                    /* Vector from dipole to the field point */

                    VEC_DIFF_3(rd,this_pos,a_vec);

                    /* Compute the dot products needed */

                    a2  = VEC_DOT_3(a_vec,a_vec);       a = sqrt(a2);

                    if (a > 0.0) {
                        r2  = VEC_DOT_3(this_pos,this_pos); r = sqrt(r2);
                        if (r > 0.0) {
                            rr0 = VEC_DOT_3(this_pos,rd);
                            ar = (r2-rr0);
                            if (fabs(ar/(a*r)+1.0) > CEPS) { /* There is a problem on the negative 'z' axis if the dipole location
                                                * and the field point are on the same line */
                                ar0  = ar/a;

                                ve = VEC_DOT_3(v,this_dir); vr = VEC_DOT_3(v,this_pos);
                                re = VEC_DOT_3(this_pos,this_dir); r0e = VEC_DOT_3(rd,this_dir);

                                /* The main ingredients */

                                F  = a*(r*a + ar);
                                gr = a2/r + ar0 + 2.0*(a+r);
                                g0 = a + 2*r + ar0;

                                /* Mix them together... */

                                sum = sum + this_coil->w[j]*(ve*F + vr*(g0*r0e - gr*re))/(F*F);
                            }
                        }
                    }
                }				/* All points done */
                Bval[k] = MAG_FACTOR*sum;
            }
        }
    }
    return OK;			/* Happy conclusion: this works always */
}


int fwd_sphere_field_vec(float        *rd,	/* The dipole location */
                         FwdCoilSet*   coils,	/* The coil definitions */
                         float        **Bval,  /* Results: rows are the fields of the x,y, and z direction dipoles */
                         void         *client)	/* Client data will be the sphere model origin */

{
    /* This version uses Jukka Sarvas' field computation
     for details, see

     Jukka Sarvas:

     Basic mathematical and electromagnetic concepts
     of the biomagnetic inverse problem,

     Phys. Med. Biol. 1987, Vol. 32, 1, 11-22

     The formulas have been manipulated for efficient computation
     by Matti Hamalainen, February 1990

     The idea of matrix kernels is from

     Mosher, Leahy, and Lewis: EEG and MEG: Forward Solutions for Inverse Methods

     which has been simplified here using standard vector notation

  */
    float *r0 = (float *)client;      /* The sphere model origin */
    float a_vec[3],v1[3],v2[3];
    float a,a2,r,r2;
    float ar,ar0,rr0;
    float re,r0e;
    float F,g0,gr,g,sum[3];
    int   j,k,p;
    FwdCoil* this_coil;
    float *this_pos,*this_dir;	/* These point to the coil structure! */
    int   np;
    float myrd[3];
    float pos[3];
    /*
   * Shift to the sphere model coordinates
   */
    for (p = 0; p < 3; p++)
        myrd[p] = rd[p] - r0[p];
    rd = myrd;
    /*
   * Check for a dipole at the origin
   */
    r = VEC_LEN_3(rd);
    for (k = 0; k < coils->ncoil; k++) {
        this_coil = coils->coils[k];
        if (FWD_IS_MEG_COIL(this_coil->coil_class)) {
            if (r < EPS) {
                Bval[0][k] = Bval[1][k] = Bval[2][k] = 0.0;
            }
            else { 	/* The hard job */

                np = this_coil->np;
                sum[0] = sum[1] = sum[2] = 0.0;

                for (j = 0; j < np; j++) {

                    this_pos = this_coil->rmag[j];
                    this_dir = this_coil->cosmag[j];

                    for (p = 0; p < 3; p++)
                        pos[p] = this_pos[p] - r0[p];
                    this_pos = pos;

                    /* Vector from dipole to the field point */

                    VEC_DIFF_3(rd,this_pos,a_vec);

                    /* Compute the dot products needed */

                    a2  = VEC_DOT_3(a_vec,a_vec);       a = sqrt(a2);

                    if (a > 0.0) {
                        r2  = VEC_DOT_3(this_pos,this_pos); r = sqrt(r2);
                        if (r > 0.0) {
                            rr0 = VEC_DOT_3(this_pos,rd);
                            ar = (r2-rr0);
                            if (fabs(ar/(a*r)+1.0) > CEPS) { /* There is a problem on the negative 'z' axis if the dipole location
                                                * and the field point are on the same line */

                                /* The main ingredients */

                                ar0  = ar/a;
                                F  = a*(r*a + ar);
                                gr = a2/r + ar0 + 2.0*(a+r);
                                g0 = a + 2*r + ar0;

                                re = VEC_DOT_3(this_pos,this_dir); r0e = VEC_DOT_3(rd,this_dir);
                                CROSS_PRODUCT_3(rd,this_dir,v1);
                                CROSS_PRODUCT_3(rd,this_pos,v2);

                                g = (g0*r0e - gr*re)/(F*F);
                                /*
                 * Mix them together...
                 */
                                for (p = 0; p < 3; p++)
                                    sum[p] = sum[p] + this_coil->w[j]*(v1[p]/F + v2[p]*g);
                            }
                        }
                    }
                }				/* All points done */
                for (p = 0; p < 3; p++)
                    Bval[p][k] = MAG_FACTOR*sum[p];
            }
        }
    }
    return OK;			/* Happy conclusion: this works always */
}





//============================= fwd_mag_dipole_field.c =============================


#define EPS 1e-5

#ifndef OK
#define OK 0
#endif

/*
 * Compute the field of a magnetic dipole
 */


int fwd_mag_dipole_field(float        *rm,      /* The dipole location in the same coordinate system as the coils */
                         float        M[],	/* The dipole components (xyz) */
                         FwdCoilSet*   coils,	/* The coil definitions */
                         float        Bval[],	/* Results */
                         void         *client)	/* Client data will be the sphere model origin */
/*
 * This is for a specific dipole component
 */
{
    int     j,k,np;
    FwdCoil* this_coil;
    float   sum,diff[3],dist,dist2,dist5,*dir;


    for (k = 0; k < coils->ncoil; k++) {
        this_coil = coils->coils[k];
        if (FWD_IS_MEG_COIL(this_coil->type)) {
            np = this_coil->np;
            /*
       * Go through all points
       */
            for (j = 0, sum = 0.0; j < np; j++) {
                dir = this_coil->cosmag[j];
                VEC_DIFF_3(rm,this_coil->rmag[j],diff);
                dist = VEC_LEN_3(diff);
                if (dist > EPS) {
                    dist2 = dist*dist;
                    dist5 = dist2*dist2*dist;
                    sum = sum + this_coil->w[j]*(3*VEC_DOT_3(M,diff)*VEC_DOT_3(diff,dir) - dist2*VEC_DOT_3(M,dir))/dist5;
                }
            }				/* All points done */
            Bval[k] = MAG_FACTOR*sum;
        }
        else if (this_coil->type == FWD_COILC_EEG)
            Bval[k] = 0.0;
    }
    return OK;
}

int fwd_mag_dipole_field_vec(float        *rm,	        /* The dipole location */
                             FwdCoilSet*   coils,	/* The coil definitions */
                             float        **Bval,       /* Results: rows are the fields of the x,y, and z direction dipoles */
                             void         *client)	/* Client data will be the sphere model origin */
/*
 * This is for all dipole components
 * For EEG this produces a zero result
 */
{
    int     j,k,p,np;
    FwdCoil* this_coil;
    float   sum[3],diff[3],dist,dist2,dist5,*dir;


    for (k = 0; k < coils->ncoil; k++) {
        this_coil = coils->coils[k];
        if (FWD_IS_MEG_COIL(this_coil->type)) {
            np = this_coil->np;
            sum[0] = sum[1] = sum[2] = 0.0;
            /*
       * Go through all points
       */
            for (j = 0; j < np; j++) {
                dir = this_coil->cosmag[j];
                VEC_DIFF_3(rm,this_coil->rmag[j],diff);
                dist = VEC_LEN_3(diff);
                if (dist > EPS) {
                    dist2 = dist*dist;
                    dist5 = dist2*dist2*dist;
                    for (p = 0; p < 3; p++)
                        sum[p] = sum[p] + this_coil->w[j]*(3*diff[p]*VEC_DOT_3(diff,dir) - dist2*dir[p])/dist5;
                }
            }				/* All points done */
            for (p = 0; p < 3; p++)
                Bval[p][k] = MAG_FACTOR*sum[p];
        }
        else if (this_coil->type == FWD_COILC_EEG) {
            for (p = 0; p < 3; p++)
                Bval[p][k] = 0.0;
        }
    }
    return OK;
}

















//============================= data.c =============================

#if defined(_WIN32) || defined(_WIN64)
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

int is_selected_in_data(mshMegEegData d, char *ch_name)
/*
 * Is this channel selected in data
 */
{
    int issel = FALSE;
    int k;

    for (k = 0; k < d->meas->nchan; k++)
        if (strcasecmp(ch_name,d->meas->chs[k].ch_name) == 0) {
            issel = d->sels[k];
            break;
        }
    return issel;
}











//============================= mne_process_bads.c =============================

static int whitespace_3(char *text)

{
    if (text == NULL || strlen(text) == 0)
        return TRUE;
    if (strspn(text," \t\n\r") == strlen(text))
        return TRUE;
    return FALSE;
}


static char *next_line_3(char *line, int n, FILE *in)
{
    char *res;

    for (res = fgets(line,n,in); res != NULL; res = fgets(line,n,in))
        if (!whitespace_3(res))
            if (res[0] != '#')
                break;
    return res;
}

#define MAXLINE 500

int mne_read_bad_channels_3(const QString& name, char ***listp, int *nlistp)
/*
* Read bad channel names
*/
{
    FILE *in = NULL;
    char **list = NULL;
    int  nlist  = 0;
    char line[MAXLINE+1];
    char *next;


    if (name.isEmpty())
        return OK;

    if ((in = fopen(name.toLatin1().data(),"r")) == NULL) {
        qCritical() << name;
        goto bad;
    }
    while ((next = next_line_3(line,MAXLINE,in)) != NULL) {
        if (strlen(next) > 0) {
            if (next[strlen(next)-1] == '\n')
                next[strlen(next)-1] = '\0';
            list = REALLOC_3(list,nlist+1,char *);
            list[nlist++] = mne_strdup_3(next);
        }
    }
    if (ferror(in))
        goto bad;

    *listp  = list;
    *nlistp = nlist;

    return OK;

bad : {
        mne_free_name_list_3(list,nlist);
        if (in != NULL)
            fclose(in);
        return FAIL;
    }
}




int mne_read_bad_channel_list_from_node_3(FiffStream::SPtr& stream,
                                        const FiffDirNode::SPtr& pNode, char ***listp, int *nlistp)
{
    FiffDirNode::SPtr node,bad;
    QList<FiffDirNode::SPtr> temp;
    char **list = NULL;
    int  nlist  = 0;
    FiffTag::SPtr t_pTag;
    char *names;

    if (pNode->isEmpty())
        node = stream->tree();
    else
        node = pNode;

    temp = node->dir_tree_find(FIFFB_MNE_BAD_CHANNELS);
    if (temp.size() > 0) {
        bad = temp[0];

        bad->find_tag(stream, FIFF_MNE_CH_NAME_LIST, t_pTag);
        if (t_pTag) {
            names = (char *)t_pTag->data();
            mne_string_to_name_list_3(names,&list,&nlist);
        }
    }
    *listp = list;
    *nlistp = nlist;
    return OK;
}

int mne_read_bad_channel_list_3(const QString& name, char ***listp, int *nlistp)

{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    int res;

    if(!stream->open())
        return FAIL;

    res = mne_read_bad_channel_list_from_node_3(stream,stream->tree(),listp,nlistp);

    stream->close();

    return res;
}















mneCovMatrix mne_read_cov(const QString& name,int kind)
/*
* Read a covariance matrix from a fiff
*/
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    FiffTag::SPtr t_pTag;
    QList<FiffDirNode::SPtr> nodes;
    FiffDirNode::SPtr    covnode;

    char            **names    = NULL;	/* Optional channel name list */
    int             nnames     = 0;
    double          *cov       = NULL;
    double          *cov_diag  = NULL;
    INVERSELIB::FiffSparseMatrix* cov_sparse = NULL;
    double          *lambda    = NULL;
    float           **eigen    = NULL;
    MatrixXf        tmp_eigen;
    char            **bads     = NULL;
    int             nbad       = 0;
    int             ncov       = 0;
    int             nfree      = 1;
    mneCovMatrix    res        = NULL;

    int            k,p,nn;
    float          *f;
    double         *d;
    mneProjOp      op = NULL;
    MneSssData*     sss = NULL;

    if(!stream->open())
        goto out;

    nodes = stream->tree()->dir_tree_find(FIFFB_MNE_COV);

    if (nodes.size() == 0) {
        printf("No covariance matrix available in %s",name.toLatin1().data());
        goto out;
    }
    /*
    * Locate the desired matrix
    */
    for (k = 0 ; k < nodes.size(); ++k) {
        if (!nodes[k]->find_tag(stream, FIFF_MNE_COV_KIND, t_pTag))
            continue;

        if (*t_pTag->toInt() == kind) {
            covnode = nodes[k];
            break;
        }
    }
    if (covnode->isEmpty()) {
        printf("Desired covariance matrix not found from %s",name.toLatin1().data());
        goto out;
    }
    /*
    * Read the data
    */
    if (!nodes[k]->find_tag(stream, FIFF_MNE_COV_DIM, t_pTag))
        goto out;
    ncov = *t_pTag->toInt();

    if (nodes[k]->find_tag(stream, FIFF_MNE_COV_NFREE, t_pTag)) {
        nfree = *t_pTag->toInt();
    }
    if (covnode->find_tag(stream, FIFF_MNE_ROW_NAMES, t_pTag)) {
        mne_string_to_name_list_3((char *)(t_pTag->data()),&names,&nnames);
        if (nnames != ncov) {
            qCritical("Incorrect number of channel names for a covariance matrix");
            goto out;
        }
    }
    if (!nodes[k]->find_tag(stream, FIFF_MNE_COV, t_pTag)) {
        if (!nodes[k]->find_tag(stream, FIFF_MNE_COV_DIAG, t_pTag))
            goto out;
        else {
            if (t_pTag->getType() == FIFFT_DOUBLE) {
                cov_diag = MALLOC_3(ncov,double);
                qDebug() << "ToDo: Check whether cov_diag contains the right stuff!!! - use VectorXd instead";
                d = t_pTag->toDouble();
                for (p = 0; p < ncov; p++)
                    cov_diag[p] = d[p];
                if (check_cov_data(cov_diag,ncov) != OK)
                    goto out;
            }
            else if (t_pTag->getType() == FIFFT_FLOAT) {
                cov_diag = MALLOC_3(ncov,double);
                qDebug() << "ToDo: Check whether f contains the right stuff!!! - use VectorXf instead";
                f = t_pTag->toFloat();
                for (p = 0; p < ncov; p++)
                    cov_diag[p] = f[p];
            }
            else {
                printf("Illegal data type for covariance matrix");
                goto out;
            }
        }
    }
    else {
        nn = ncov*(ncov+1)/2;
        if (t_pTag->getType() == FIFFT_DOUBLE) {
            qDebug() << "ToDo: Check whether cov contains the right stuff!!! - use VectorXd instead";
//            qDebug() << t_pTag->size() / sizeof(double);
            cov = MALLOC_3(nn,double);
            d = t_pTag->toDouble();
            for (p = 0; p < nn; p++)
                cov[p] = d[p];
            if (check_cov_data(cov,nn) != OK)
                goto out;
        }
        else if (t_pTag->getType() == FIFFT_FLOAT) {
            cov = MALLOC_3(nn,double);
            f = t_pTag->toFloat();
            for (p = 0; p < nn; p++)
                cov[p] = f[p];
        }
        else if ((cov_sparse = FiffSparseMatrix::fiff_get_float_sparse_matrix(t_pTag)) == NULL) {
            goto out;
        }

        if (nodes[k]->find_tag(stream, FIFF_MNE_COV_EIGENVALUES, t_pTag)) {
            lambda = (double *)t_pTag->toDouble();
            if (nodes[k]->find_tag(stream, FIFF_MNE_COV_EIGENVECTORS, t_pTag))
                goto out;

            tmp_eigen = t_pTag->toFloatMatrix().transpose();
            eigen = ALLOC_CMATRIX_3(tmp_eigen.rows(),tmp_eigen.cols());
            fromFloatEigenMatrix_3(tmp_eigen, eigen);
        }
        /*
        * Read the optional projection operator
        */
        if ((op = mne_read_proj_op_from_node_3(stream,nodes[k])) == NULL)
            goto out;
        /*
        * Read the optional SSS data
        */
        if ((sss = MneSssData::read_sss_data_from_node(stream,nodes[k])) == NULL)
            goto out;
        /*
        * Read the optional bad channel list
        */
        if (mne_read_bad_channel_list_from_node_3(stream,nodes[k],&bads,&nbad) == FAIL)
            goto out;
    }
    if (cov_sparse)
        res = mne_new_cov_sparse(kind,ncov,names,cov_sparse);
    else if (cov)
        res = mne_new_cov_dense(kind,ncov,names,cov);
    else if (cov_diag)
        res = mne_new_cov_diag(kind,ncov,names,cov_diag);
    else {
        qCritical("mne_read_cov : covariance matrix data is not defined. How come?");
        goto out;
    }
    cov         = NULL;
    cov_diag    = NULL;
    cov_sparse  = NULL;
    res->eigen  = eigen;
    res->lambda = lambda;
    res->nfree  = nfree;
    res->bads   = bads;
    res->nbad   = nbad;
    /*
    * Count the non-zero eigenvalues
    */
    if (res->lambda) {
        res->nzero = 0;
        for (k = 0; k < res->ncov; k++, res->nzero++)
            if (res->lambda[k] > 0)
                break;
    }

    if (op && op->nitems > 0) {
        res->proj = op;
        op = NULL;
    }
    if (sss && sss->ncomp > 0 && sss->job != FIFFV_SSS_JOB_NOTHING) {
        res->sss = sss;
        sss = NULL;
    }

out : {
        stream->close();
        mne_free_proj_op_3(op);
        if(sss)
            delete sss;

        if (!res) {
            mne_free_name_list_3(names,nnames);
            mne_free_name_list_3(bads,nbad);
            FREE_3(cov);
            FREE_3(cov_diag);
            if(cov_sparse)
                delete cov_sparse;
        }
        return res;
    }

}








//============================= mne_coord_transforms.c =============================

typedef struct {
    int frame;
    const char *name;
} frameNameRec_3;


const char *mne_coord_frame_name_3(int frame)

{
    static frameNameRec_3 frames[] = {
        {FIFFV_COORD_UNKNOWN,"unknown"},
        {FIFFV_COORD_DEVICE,"MEG device"},
        {FIFFV_COORD_ISOTRAK,"isotrak"},
        {FIFFV_COORD_HPI,"hpi"},
        {FIFFV_COORD_HEAD,"head"},
        {FIFFV_COORD_MRI,"MRI (surface RAS)"},
        {FIFFV_MNE_COORD_MRI_VOXEL, "MRI voxel"},
        {FIFFV_COORD_MRI_SLICE,"MRI slice"},
        {FIFFV_COORD_MRI_DISPLAY,"MRI display"},
        {FIFFV_MNE_COORD_CTF_DEVICE,"CTF MEG device"},
        {FIFFV_MNE_COORD_CTF_HEAD,"CTF/4D/KIT head"},
        {FIFFV_MNE_COORD_RAS,"RAS (non-zero origin)"},
        {FIFFV_MNE_COORD_MNI_TAL,"MNI Talairach"},
        {FIFFV_MNE_COORD_FS_TAL_GTZ,"Talairach (MNI z > 0)"},
        {FIFFV_MNE_COORD_FS_TAL_LTZ,"Talairach (MNI z < 0)"},
        {-1,"unknown"}
    };
    int k;
    for (k = 0; frames[k].frame != -1; k++) {
        if (frame == frames[k].frame)
            return frames[k].name;
    }
    return frames[k].name;
}




//============================= mne_read_process_forward_solution.c =============================

void mne_merge_channels(fiffChInfo chs1, int nch1,
                        fiffChInfo chs2, int nch2,
                        fiffChInfo *resp, int *nresp)

{
    fiffChInfo res = MALLOC_3(nch1+nch2,fiffChInfoRec);
    int k,p;
    for (p = 0, k = 0; k < nch1; k++)
        res[p++] = chs1[k];
    for (k = 0; k < nch2;k++)
        res[p++] = chs2[k];
    *resp = res;
    *nresp = nch1+nch2;
    return;
}


//============================= read_ch_info.c =============================




static FiffDirNode::SPtr find_meas_3 (const FiffDirNode::SPtr& node)
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



static FiffDirNode::SPtr find_meas_info_3 (const FiffDirNode::SPtr& node)
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
    for (k = 0; k < tmp_node->nchild; k++)
        if (tmp_node->children[k]->type == FIFFB_MEAS_INFO)
            return (tmp_node->children[k]);
    return empty_node;
}



static int get_all_chs (//fiffFile file,	        /* The file we are reading */
                        FiffStream::SPtr& stream,
                        const FiffDirNode::SPtr& p_node,	/* The directory node containing our data */
                        fiffId *id,		/* The block id from the nearest FIFFB_MEAS
                                                   parent */
                        fiffChInfo *chp,	/* Channel descriptions */
                        int *nchan)		/* Number of channels */
/*
      * Find channel information from
      * nearest FIFFB_MEAS_INFO parent of
      * node.
      */
{
    fiffChInfo ch;
    fiffChInfo this_ch = NULL;
    int j,k;
    int to_find = 0;
    FiffDirNode::SPtr meas;
    FiffDirNode::SPtr meas_info;
    fiff_int_t kind, pos;
    FiffTag::SPtr t_pTag;

    *chp     = NULL;
    ch       = NULL;
    *id      = NULL;
    /*
    * Find desired parents
    */
    if (!(meas = find_meas_3(p_node))) {
        qCritical("Meas. block not found!");
        goto bad;
    }

    if (!(meas_info = find_meas_info_3(p_node))) {
        qCritical ("Meas. info not found!");
        goto bad;
    }
    /*
    * Is there a block id is in the FIFFB_MEAS node?
    */
    if (!meas->id.isEmpty()) {
        *id = MALLOC_3(1,fiffIdRec);
        (*id)->version = meas->id.version;
        (*id)->machid[0] = meas->id.machid[0];
        (*id)->machid[1] = meas->id.machid[1];
        (*id)->time = meas->id.time;
    }
    /*
    * Others from FIFFB_MEAS_INFO
    */
    for (k = 0; k < meas_info->nent; k++) {
        kind = meas_info->dir[k]->kind;
        pos  = meas_info->dir[k]->pos;
        switch (kind) {
        case FIFF_NCHAN :
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            *nchan = *t_pTag->toInt();
            ch = MALLOC_3(*nchan,fiffChInfoRec);
            for (j = 0; j < *nchan; j++)
                ch[j].scanNo = -1;
            to_find = to_find + *nchan - 1;
            break;

        case FIFF_CH_INFO : /* Information about one channel */
            if (!FiffTag::read_tag(stream,t_pTag,pos))
                goto bad;
            this_ch = (fiffChInfo)malloc(sizeof(fiffChInfoRec));
            *this_ch = *(fiffChInfo)t_pTag->data();
            if (this_ch->scanNo <= 0 || this_ch->scanNo > *nchan) {
                qCritical ("FIFF_CH_INFO : scan # out of range!");
                goto bad;
            }
            else
                memcpy(ch+this_ch->scanNo-1,this_ch,
                       sizeof(fiffChInfoRec));
            to_find--;
            break;
        }
    }
    *chp = ch;
    return FIFF_OK;

bad : {
        FREE_3(ch);
        return FIFF_FAIL;
    }
}




static int read_ch_info(const QString&  name,
                        fiffChInfo      *chsp,
                        int             *nchanp,
                        fiffId          *idp)
/*
* Read channel information from a measurement file
*/
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    fiffChInfo chs = NULL;
    int        nchan = 0;
    fiffId     id = NULL;

    QList<FiffDirNode::SPtr> meas;
    FiffDirNode::SPtr    node;

    if(!stream->open())
        goto bad;

    meas = stream->tree()->dir_tree_find(FIFFB_MEAS);
    if (meas.size() == 0) {
        qCritical ("%s : no MEG data available here",name.toLatin1().data());
        goto bad;
    }
    node = meas[0];
    if (get_all_chs (stream,node,&id,&chs,&nchan) == FIFF_FAIL)
        goto bad;
    *chsp   = chs;
    *nchanp = nchan;
    *idp = id;
    stream->close();
    return FIFF_OK;

bad : {
        FREE_3(chs);
        FREE_3(id);
        stream->close();
        return FIFF_FAIL;
    }
}




#define TOO_CLOSE 1e-4

static int at_origin (float *rr)

{
    return (VEC_LEN_3(rr) < TOO_CLOSE);
}




static int is_valid_eeg_ch(fiffChInfo ch)
/*
      * Is the electrode position information present?
      */
{
    if (ch->kind == FIFFV_EEG_CH) {
        if (at_origin(ch->chpos.r0) ||
                ch->chpos.coil_type == FIFFV_COIL_NONE)
            return FALSE;
        else
            return TRUE;
    }
    return FALSE;
}







static int accept_ch(fiffChInfo ch,
                     char       **bads,
                     int        nbad)

{
    int k;
    for (k = 0; k < nbad; k++)
        if (strcmp(ch->ch_name,bads[k]) == 0)
            return FALSE;
    return TRUE;
}








int read_meg_eeg_ch_info(const QString& name,       /* Input file */
                         int        do_meg,	 /* Use MEG */
                         int        do_eeg,	 /* Use EEG */
                         char       **bads,	 /* List of bad channels */
                         int        nbad,
                         fiffChInfo *chsp,	 /* MEG + EEG channels */
                         int        *nmegp,	 /* Count of each */
                         int        *neegp)
/*
      * Read the channel information and split it into two arrays,
      * one for MEG and one for EEG
      */
{
    fiffChInfo chs   = NULL;
    int        nchan = 0;
    fiffChInfo meg   = NULL;
    int        nmeg  = 0;
    fiffChInfo eeg   = NULL;
    int        neeg  = 0;
    fiffId     id    = NULL;
    int        nch;

    int k;

    if (read_ch_info(name,&chs,&nchan,&id) != FIFF_OK)
        goto bad;
    /*
   * Sort out the channels
   */
    for (k = 0; k < nchan; k++)
        if (chs[k].kind == FIFFV_MEG_CH)
            nmeg++;
        else if (chs[k].kind == FIFFV_EEG_CH && is_valid_eeg_ch(chs+k))
            neeg++;
    if (nmeg > 0)
        meg = MALLOC_3(nmeg,fiffChInfoRec);
    if (neeg > 0)
        eeg = MALLOC_3(neeg,fiffChInfoRec);
    neeg = nmeg = 0;
    for (k = 0; k < nchan; k++)
        if (accept_ch(chs+k,bads,nbad)) {
            if (do_meg && chs[k].kind == FIFFV_MEG_CH)
                meg[nmeg++] = chs[k];
            else if (do_eeg && chs[k].kind == FIFFV_EEG_CH && is_valid_eeg_ch(chs+k))
                eeg[neeg++] = chs[k];
        }
    FREE_3(chs);
    mne_merge_channels(meg,nmeg,eeg,neeg,chsp,&nch);
    FREE_3(meg);
    FREE_3(eeg);
    *nmegp = nmeg;
    *neegp = neeg;
    FREE_3(id);
    return FIFF_OK;

bad : {
        FREE_3(chs);
        FREE_3(meg);
        FREE_3(eeg);
        FREE_3(id);
        return FIFF_FAIL;
    }
}











void mne_revert_to_diag_cov(mneCovMatrix c)
/*
* Pick the diagonal elements of the full covariance matrix
*/
{
    int k,p;
    if (c->cov == NULL)
        return;
#define REALLY_REVERT
#ifdef REALLY_REVERT
    c->cov_diag = REALLOC_3(c->cov_diag,c->ncov,double);

    for (k = p = 0; k < c->ncov; k++) {
        c->cov_diag[k] = c->cov[p];
        p = p + k + 2;
    }
    FREE_3(c->cov);  c->cov = NULL;
#else
    for (j = 0, p = 0; j < c->ncov; j++)
        for (k = 0; k <= j; k++, p++)
            if (j != k)
                c->cov[p] = 0.0;
            else
#endif
                FREE_3(c->lambda); c->lambda = NULL;
    FREE_CMATRIX_3(c->eigen); c->eigen = NULL;
    return;

}






mneCovMatrix mne_pick_chs_cov_omit(mneCovMatrix c, char **new_names, int ncov, int omit_meg_eeg, fiffChInfo chs)
/*
* Pick designated channels from a covariance matrix, optionally omit MEG/EEG correlations
*/
{
    int j,k;
    int *pick = NULL;
    double *cov = NULL;
    double *cov_diag = NULL;
    char  **names = NULL;
    int   *is_meg = NULL;
    int   from,to;
    mneCovMatrix res;

    if (ncov == 0) {
        qCritical("No channels specified for picking in mne_pick_chs_cov_omit");
        return NULL;
    }
    if (c->names == NULL) {
        qCritical("No names in covariance matrix. Cannot do picking.");
        return NULL;
    }
    pick = MALLOC_3(ncov,int);
    for (j = 0; j < ncov; j++)
        pick[j] = -1;
    for (j = 0; j < ncov; j++)
        for (k = 0; k < c->ncov; k++)
            if (strcmp(c->names[k],new_names[j]) == 0) {
                pick[j] = k;
                break;
            }
    for (j = 0; j < ncov; j++) {
        if (pick[j] < 0) {
            printf("All desired channels not found in the covariance matrix (at least missing %s).", new_names[j]);
            FREE_3(pick);
            return NULL;
        }
    }
    if (omit_meg_eeg) {
        is_meg = MALLOC_3(ncov,int);
        if (chs) {
            for (j = 0; j < ncov; j++)
                if (chs[j].kind == FIFFV_MEG_CH)
                    is_meg[j] = TRUE;
                else
                    is_meg[j] = FALSE;
        }
        else {
            for (j = 0; j < ncov; j++)
                if (strncmp(new_names[j],"MEG",3) == 0)
                    is_meg[j] = TRUE;
                else
                    is_meg[j] = FALSE;
        }
    }
    names = MALLOC_3(ncov,char *);
    if (c->cov_diag) {
        cov_diag = MALLOC_3(ncov,double);
        for (j = 0; j < ncov; j++) {
            cov_diag[j] = c->cov_diag[pick[j]];
            names[j] = mne_strdup_3(c->names[pick[j]]);
        }
    }
    else {
        cov = MALLOC_3(ncov*(ncov+1)/2,double);
        for (j = 0; j < ncov; j++) {
            names[j] = mne_strdup_3(c->names[pick[j]]);
            for (k = 0; k <= j; k++) {
                from = mne_lt_packed_index_3(pick[j],pick[k]);
                to   = mne_lt_packed_index_3(j,k);
                if (to < 0 || to > ncov*(ncov+1)/2-1) {
                    printf("Wrong destination index in mne_pick_chs_cov : %d %d %d\n",j,k,to);
                    exit(1);
                }
                if (from < 0 || from > c->ncov*(c->ncov+1)/2-1) {
                    printf("Wrong source index in mne_pick_chs_cov : %d %d %d\n",pick[j],pick[k],from);
                    exit(1);
                }
                cov[to] = c->cov[from];
                if (omit_meg_eeg)
                    if (is_meg[j] != is_meg[k])
                        cov[to] = 0.0;
            }
        }
    }

    res = mne_new_cov_3(c->kind,ncov,names,cov,cov_diag);

    res->bads = mne_dup_name_list_3(c->bads,c->nbad);
    res->nbad = c->nbad;
    res->proj = mne_dup_proj_op_3(c->proj);
    res->sss  = c->sss ? new MneSssData(*(c->sss)) : NULL;

    if (c->ch_class) {
        res->ch_class = MALLOC_3(res->ncov,int);
        for (k = 0; k < res->ncov; k++)
            res->ch_class[k] = c->ch_class[pick[k]];
    }
    FREE_3(pick);
    FREE_3(is_meg);
    return res;
}





int mne_proj_op_proj_dvector(mneProjOp op, double *vec, int nch, int do_complement)
/*
* Apply projection operator to a vector (doubles)
* Assume that all dimension checking etc. has been done before
*/
{
    float *pvec;
    double w;
    int k,p;

    if (op->nvec <= 0)
        return OK;

    if (op->nch != nch) {
        qCritical("Data vector size does not match projection operator");
        return FAIL;
    }

    Eigen::VectorXd res = Eigen::VectorXd::Zero(op->nch);

    for (p = 0; p < op->nvec; p++) {
        pvec = op->proj_data[p];
        w = 0.0;
        for (k = 0; k < op->nch; k++)
            w += vec[k]*pvec[k];
        for (k = 0; k < op->nch; k++)
            res[k] = res[k] + w*pvec[k];
    }

    if (do_complement) {
        for (k = 0; k < op->nch; k++)
            vec[k] = vec[k] - res[k];
    }
    else {
        for (k = 0; k < op->nch; k++)
            vec[k] = res[k];
    }

    return OK;
}





int mne_name_list_match(char **list1, int nlist1,
                        char **list2, int nlist2)
/*
 * Check whether two name lists are identical
 */
{
    int k;
    if (list1 == NULL && list2 == NULL)
        return 0;
    if (list1 == NULL || list2 == NULL)
        return 1;
    if (nlist1 != nlist2)
        return 1;
    for (k = 0; k < nlist1; k++)
        if (strcmp(list1[k],list2[k]) != 0)
            return 1;
    return 0;
}



void mne_transpose_dsquare(double **mat, int n)
/*
      * In-place transpose of a square matrix
      */
{
    int j,k;
    double val;

    for (j = 1; j < n; j++)
        for (k = 0; k < j; k++) {
            val = mat[j][k];
            mat[j][k] = mat[k][j];
            mat[k][j] = val;
        }
    return;
}



int mne_proj_op_apply_cov(mneProjOp op, mneCovMatrix& c)
/*
* Apply the projection operator to a covariance matrix
*/
{
    double **dcov = NULL;
    int j,k,p;
    int do_complement = TRUE;

    if (op == NULL || op->nitems == 0)
        return OK;

    if (mne_name_list_match(op->names,op->nch,c->names,c->ncov) != OK) {
        qCritical("Incompatible data in mne_proj_op_apply_cov");
        return FAIL;
    }

    dcov = ALLOC_DCMATRIX_3(c->ncov,c->ncov);
    /*
    * Return the appropriate result
    */
    if (c->cov_diag) {  /* Pick the diagonals */
        for (j = 0, p = 0; j < c->ncov; j++)
            for (k = 0; k < c->ncov; k++)
                dcov[j][k] = (j == k) ? c->cov_diag[j] : 0;
    }
    else {              /* Return full matrix */
        for (j = 0, p = 0; j < c->ncov; j++)
            for (k = 0; k <= j; k++)
                dcov[j][k] = c->cov[p++];
        for (j = 0; j < c->ncov; j++)
            for (k = j+1; k < c->ncov; k++)
                dcov[j][k] = dcov[k][j];
    }

    /*
    * Project from front and behind
    */
    for (k = 0; k < c->ncov; k++) {
        if (mne_proj_op_proj_dvector(op,dcov[k],c->ncov,do_complement) != OK)
            return FAIL;
    }

    mne_transpose_dsquare(dcov,c->ncov);

    for (k = 0; k < c->ncov; k++)
        if (mne_proj_op_proj_dvector(op,dcov[k],c->ncov,do_complement) != OK)
            return FAIL;

    /*
    * Return the result
    */
    if (c->cov_diag) {  /* Pick the diagonal elements */
        for (j = 0; j < c->ncov; j++) {
            c->cov_diag[j] = dcov[j][j];
        }
        FREE_3(c->cov); c->cov = NULL;
    }
    else {              /* Put everything back */
        for (j = 0, p = 0; j < c->ncov; j++)
            for (k = 0; k <= j; k++)
                c->cov[p++] = dcov[j][k];
    }

    FREE_DCMATRIX_3(dcov);

    c->nproj = mne_proj_op_affect_3(op,c->names,c->ncov);
    return OK;
}



//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DipoleFitData::DipoleFitData()
: mri_head_t (NULL)
, meg_head_t (NULL)
, chs (NULL)
, meg_coils (NULL)
, eeg_els (NULL)
, nmeg (0)
, neeg (0)
, ch_names (NULL)
, pick (NULL)
, bemname (NULL)
, bem_model (NULL)
, eeg_model (NULL)
, fixed_noise (FALSE)
, noise (NULL)
, noise_orig (NULL)
, nave (1)
, user (NULL)
, user_free (NULL)
, proj (NULL)
, sphere_funcs (NULL)
, bem_funcs (NULL)
, mag_dipole_funcs (NULL)
, funcs (NULL)
, column_norm (COLUMN_NORM_NONE)
, fit_mag_dipoles (FALSE)
{
    r0[0] = 0.0f;
    r0[1] = 0.0f;
    r0[2] = 0.0f;
}


//*************************************************************************************************************

DipoleFitData::~DipoleFitData()
{
    if(mri_head_t)
        delete mri_head_t;
    if(meg_head_t)
        delete meg_head_t;

    FREE_3(chs);

    if(meg_coils)
        delete meg_coils;
    if(eeg_els)
        delete eeg_els;

    FREE_3(bemname);

    mne_free_cov_3(noise);
    mne_free_cov_3(noise_orig);
    mne_free_name_list_3(ch_names,nmeg+neeg);

    if(pick)
        delete pick;
    fwd_bem_free_model(bem_model);

    if(eeg_model)
        delete eeg_model;
    if (user_free)
        user_free(user);

    mne_free_proj_op_3(proj);

    free_dipole_fit_funcs(sphere_funcs);
    free_dipole_fit_funcs(bem_funcs);
    free_dipole_fit_funcs(mag_dipole_funcs);
}


//*************************************************************************************************************

int DipoleFitData::setup_forward_model(DipoleFitData *d, mneCTFcompDataSet comp_data, FwdCoilSet *comp_coils)
/*
     * Take care of some hairy details
     */
{
    fwdCompData comp;
    dipoleFitFuncs f;
    int fit_sphere_to_bem = TRUE;

    if (d->bemname) {
        /*
         * Set up the boundary-element model
         */
        char *bemsolname = fwd_bem_make_bem_sol_name(d->bemname);
        FREE_3(d->bemname); d->bemname = bemsolname;

        printf("\nSetting up the BEM model using %s...\n",d->bemname);
        printf("\nLoading surfaces...\n");
        d->bem_model = fwd_bem_load_three_layer_surfaces(d->bemname);
        if (d->bem_model) {
            printf("Three-layer model surfaces loaded.\n");
        }
        else {
            d->bem_model = fwd_bem_load_homog_surface(d->bemname);
            if (!d->bem_model)
                goto out;
            printf("Homogeneous model surface loaded.\n");
        }
        if (d->neeg > 0 && d->bem_model->nsurf == 1) {
            qCritical("Cannot use a homogeneous model in EEG calculations.");
            goto out;
        }
        printf("\nLoading the solution matrix...\n");
        if (fwd_bem_load_recompute_solution(d->bemname,FWD_BEM_UNKNOWN,FALSE,d->bem_model) == FAIL)
            goto out;
        printf("Employing the head->MRI coordinate transform with the BEM model.\n");
        if (fwd_bem_set_head_mri_t(d->bem_model,d->mri_head_t) == FAIL)
            goto out;
        printf("BEM model %s is now set up\n",d->bem_model->sol_name);
        /*
         * Find the best-fitting sphere
         */
        if (fit_sphere_to_bem) {
            MneSurfaceOrVolume::MneCSurface* inner_skull;
            float      simplex_size = 2e-2;
            float      R;

            if ((inner_skull = fwd_bem_find_surface(d->bem_model,FIFFV_BEM_SURF_ID_BRAIN)) == NULL)
                goto out;

            if (fit_sphere_to_points(inner_skull->rr,inner_skull->np,simplex_size,d->r0,&R) == FAIL)
                goto out;

            FiffCoordTransOld::fiff_coord_trans(d->r0,d->mri_head_t,TRUE);
            printf("Fitted sphere model origin : %6.1f %6.1f %6.1f mm rad = %6.1f mm.\n",
                   1000*d->r0[X_3],1000*d->r0[Y_3],1000*d->r0[Z_3],1000*R);
        }
        d->bem_funcs = f = new_dipole_fit_funcs();
        if (d->nmeg > 0) {
            /*
           * Use the new compensated field computation
           * It works the same way independent of whether or not the compensation is in effect
           */
            comp = fwd_make_comp_data(comp_data,d->meg_coils,comp_coils,
                                      fwd_bem_field,NULL,NULL,d->bem_model,NULL);
            if (!comp)
                goto out;
            printf("Compensation setup done.\n");

            printf("MEG solution matrix...");
            if (fwd_bem_specify_coils(d->bem_model,d->meg_coils) == FAIL)
                goto out;
            if (fwd_bem_specify_coils(d->bem_model,comp->comp_coils) == FAIL)
                goto out;
            printf("[done]\n");

            f->meg_field       = fwd_comp_field;
            f->meg_vec_field   = NULL;
            f->meg_client      = comp;
            f->meg_client_free = fwd_free_comp_data;
        }
        if (d->neeg > 0) {
            printf("\tEEG solution matrix...");
            if (fwd_bem_specify_els(d->bem_model,d->eeg_els) == FAIL)
                goto out;
            printf("[done]\n");
            f->eeg_pot     = fwd_bem_pot_els;
            f->eeg_vec_pot = NULL;
            f->eeg_client  = d->bem_model;
        }
    }
    if (d->neeg > 0 && !d->eeg_model) {
        qCritical("EEG sphere model not defined.");
        goto out;
    }
    d->sphere_funcs = f = new_dipole_fit_funcs();
    if (d->neeg > 0) {
        VEC_COPY_3(d->eeg_model->r0,d->r0);
        f->eeg_pot     = FwdEegSphereModel::fwd_eeg_spherepot_coil;
        f->eeg_vec_pot = FwdEegSphereModel::fwd_eeg_spherepot_coil_vec;
        f->eeg_client  = d->eeg_model;
    }
    if (d->nmeg > 0) {
        /*
         * Use the new compensated field computation
         * It works the same way independent of whether or not the compensation is in effect
         */
        comp = fwd_make_comp_data(comp_data,d->meg_coils,comp_coils,
                                  fwd_sphere_field,
                                  fwd_sphere_field_vec,
                                  NULL,
                                  d->r0,NULL);
        if (!comp)
            goto out;
        f->meg_field       = fwd_comp_field;
        f->meg_vec_field   = fwd_comp_field_vec;
        f->meg_client      = comp;
        f->meg_client_free = fwd_free_comp_data;
    }
    printf("Sphere model origin : %6.1f %6.1f %6.1f mm.\n",
           1000*d->r0[X_3],1000*d->r0[Y_3],1000*d->r0[Z_3]);
    /*
       * Finally add the magnetic dipole fitting functions (for special purposes)
       */
    d->mag_dipole_funcs = f = new_dipole_fit_funcs();
    if (d->nmeg > 0) {
        /*
         * Use the new compensated field computation
         * It works the same way independent of whether or not the compensation is in effect
         */
        comp = fwd_make_comp_data(comp_data,d->meg_coils,comp_coils,
                                  fwd_mag_dipole_field,
                                  fwd_mag_dipole_field_vec,
                                  NULL,
                                  NULL,NULL);
        if (!comp)
            goto out;
        f->meg_field       = fwd_comp_field;
        f->meg_vec_field   = fwd_comp_field_vec;
        f->meg_client      = comp;
        f->meg_client_free = fwd_free_comp_data;
    }
    f->eeg_pot     = fwd_mag_dipole_field;
    f->eeg_vec_pot = fwd_mag_dipole_field_vec;
    /*
        * Select the appropriate fitting function
        */
    d->funcs = d->bemname ? d->bem_funcs : d->sphere_funcs;

    fprintf (stderr,"\n");
    return OK;

out :
    return FAIL;
}


//*************************************************************************************************************

mneCovMatrix DipoleFitData::ad_hoc_noise(FwdCoilSet *meg, FwdCoilSet *eeg, float grad_std, float mag_std, float eeg_std)
/*
     * Specify constant noise values
     */
{
    int    nchan;
    double *stds;
    char  **names,**ch_names;
    int   k,n;

    printf("Using standard noise values "
           "(MEG grad : %6.1f fT/cm MEG mag : %6.1f fT EEG : %6.1f uV)\n",
           1e13*grad_std,1e15*mag_std,1e6*eeg_std);

    nchan = 0;
    if (meg)
        nchan = nchan + meg->ncoil;
    if (eeg)
        nchan = nchan + eeg->ncoil;

    stds = MALLOC_3(nchan,double);
    ch_names = MALLOC_3(nchan,char *);

    n = 0;
    if (meg) {
        for (k = 0; k < meg->ncoil; k++, n++) {
            if (meg->coils[k]->is_axial_coil()) {
                stds[n] = mag_std*mag_std;
#ifdef TEST_REF
                if (meg->coils[k]->type == FIFFV_COIL_CTF_REF_MAG ||
                        meg->coils[k]->type == FIFFV_COIL_CTF_REF_GRAD ||
                        meg->coils[k]->type == FIFFV_COIL_CTF_OFFDIAG_REF_GRAD)
                    stds[n] = 1e6*stds[n];
#endif
            }
            else
                stds[n] = grad_std*grad_std;
            ch_names[n] = meg->coils[k]->chname;
        }
    }
    if (eeg) {
        for (k = 0; k < eeg->ncoil; k++, n++) {
            stds[n]     = eeg_std*eeg_std;
            ch_names[n] = eeg->coils[k]->chname;
        }
    }
    names = mne_dup_name_list_3(ch_names,nchan);
    FREE_3(ch_names);
    return mne_new_cov_3(FIFFV_MNE_NOISE_COV,nchan,names,NULL,stds);
}


//*************************************************************************************************************

int DipoleFitData::make_projection(const QList<QString> &projnames, fiffChInfo chs, int nch, mneProjOp *res)
/*
          * Process the projection data
          */
{
    mneProjOp all  = NULL;
    mneProjOp one  = NULL;
    int       k,found;
    int       neeg;

    for (k = 0, neeg = 0; k < nch; k++)
        if (chs[k].kind == FIFFV_EEG_CH)
            neeg++;

    if (projnames.size() == 0 && neeg == 0)
        return OK;

    for (k = 0; k < projnames.size(); k++) {
        if ((one = mne_read_proj_op_3(projnames[k])) == NULL)
            goto bad;
        if (one->nitems == 0) {
            printf("No linear projection information in %s.\n",projnames[k].toLatin1().data());
            mne_free_proj_op_3(one); one = NULL;
        }
        else {
            printf("Loaded projection from %s:\n",projnames[k].toLatin1().data());
            mne_proj_op_report_3(stderr,"\t",one);
            all = mne_proj_op_combine_3(all,one);
            mne_free_proj_op_3(one); one = NULL;
        }
    }

    if (neeg > 0) {
        found = FALSE;
        if (all) {
            for (k = 0; k < all->nitems; k++)
                if (all->items[k]->kind == FIFFV_MNE_PROJ_ITEM_EEG_AVREF) {
                    found = TRUE;
                    break;
                }
        }
        if (!found) {
            if ((one = mne_proj_op_average_eeg_ref_3(chs,nch)) != NULL) {
                printf("Average EEG reference projection added:\n");
                mne_proj_op_report_3(stderr,"\t",one);
                all = mne_proj_op_combine_3(all,one);
                mne_free_proj_op_3(one); one = NULL;
            }
        }
    }
    if (all && mne_proj_op_affect_chs_3(all,chs,nch) == 0) {
        printf("Projection will not have any effect on selected channels. Projection omitted.\n");
        mne_free_proj_op_3(all);
        all = NULL;
    }
    *res = all;
    return OK;

bad :
    return FAIL;
}


//*************************************************************************************************************

int DipoleFitData::scale_noise_cov(DipoleFitData *f, int nave)
{
    float nave_ratio = ((float)f->nave)/(float)nave;
    int   k;

    if (!f->noise)
        return OK;

    if (f->noise->cov != NULL) {
        fprintf(stderr,"Decomposing the sensor noise covariance matrix...\n");
        if (mne_decompose_eigen_cov_3(f->noise) == FAIL)
            goto bad;

        for (k = 0; k < f->noise->ncov*(f->noise->ncov+1)/2; k++)
            f->noise->cov[k] = nave_ratio*f->noise->cov[k];
        for (k = 0; k < f->noise->ncov; k++) {
            f->noise->lambda[k] = nave_ratio*f->noise->lambda[k];
            if (f->noise->lambda[k] < 0.0)
                f->noise->lambda[k] = 0.0;
        }
        if (mne_add_inv_cov_3(f->noise) == FAIL)
            goto bad;
    }
    else {
        for (k = 0; k < f->noise->ncov; k++)
            f->noise->cov_diag[k] = nave_ratio*f->noise->cov_diag[k];
        fprintf(stderr,"Decomposition not needed for a diagonal noise covariance matrix.\n");
        if (mne_add_inv_cov_3(f->noise) == FAIL)
            goto bad;
    }
    fprintf(stderr,"Effective nave is now %d\n",nave);
    f->nave = nave;
    return OK;

bad :
    return FAIL;
}


//*************************************************************************************************************

int DipoleFitData::scale_dipole_fit_noise_cov(DipoleFitData *f, int nave)
{
    float nave_ratio = ((float)f->nave)/(float)nave;
    int   k;

    if (!f->noise)
        return OK;
    if (f->fixed_noise)
        return OK;

    if (f->noise->cov) {
        /*
         * Do the decomposition and check that the matrix is positive definite
         */
        fprintf(stderr,"Decomposing the noise covariance...");
        if (f->noise->cov) {
            if (mne_decompose_eigen_cov_3(f->noise) == FAIL)
                goto bad;
            for (k = 0; k < f->noise->ncov; k++) {
                if (f->noise->lambda[k] < 0.0)
                    f->noise->lambda[k] = 0.0;
            }
        }
        for (k = 0; k < f->noise->ncov*(f->noise->ncov+1)/2; k++)
            f->noise->cov[k] = nave_ratio*f->noise->cov[k];
        for (k = 0; k < f->noise->ncov; k++) {
            f->noise->lambda[k] = nave_ratio*f->noise->lambda[k];
            if (f->noise->lambda[k] < 0.0)
                f->noise->lambda[k] = 0.0;
        }
        if (mne_add_inv_cov_3(f->noise) == FAIL)
            goto bad;
    }
    else {
        for (k = 0; k < f->noise->ncov; k++)
            f->noise->cov_diag[k] = nave_ratio*f->noise->cov_diag[k];
        fprintf(stderr,"Decomposition not needed for a diagonal noise covariance matrix.\n");
        if (mne_add_inv_cov_3(f->noise) == FAIL)
            goto bad;
    }
    fprintf(stderr,"Effective nave is now %d\n",nave);
    f->nave = nave;
    return OK;

bad :
    return FAIL;
}


//*************************************************************************************************************

int DipoleFitData::select_dipole_fit_noise_cov(DipoleFitData *f, mshMegEegData d)
/*
     * Do the channel selection and scale with nave
     */
{
    int   nave,j,k;
    float nonsel_w  = 30;
    int   min_nchan = 20;

    if (!f || !f->noise_orig)
        return OK;
    if (!d)
        nave = 1;
    else {
        if (d->nave < 0)
            nave = d->meas->current->nave;
        else
            nave = d->nave;
    }
    /*
       * Channel selection
       */
    if (d) {
        float  *w    = MALLOC_3(f->noise_orig->ncov,float);
        int    nomit_meg,nomit_eeg,nmeg,neeg;
        double *val;

        nmeg = neeg = 0;
        nomit_meg = nomit_eeg = 0;
        for (k = 0; k < f->noise_orig->ncov; k++) {
            if (f->noise_orig->ch_class[k] == MNE_COV_CH_EEG)
                neeg++;
            else
                nmeg++;
            if (is_selected_in_data(d,f->noise_orig->names[k]))
                w[k] = 1.0;
            else {
                w[k] = nonsel_w;
                if (f->noise_orig->ch_class[k] == MNE_COV_CH_EEG)
                    nomit_eeg++;
                else
                    nomit_meg++;
            }
        }
        mne_free_cov_3(f->noise); f->noise = NULL;
        if (nmeg > 0 && nmeg-nomit_meg > 0 && nmeg-nomit_meg < min_nchan) {
            qCritical("Too few MEG channels remaining");
            return FAIL;
        }
        if (neeg > 0 && neeg-nomit_eeg > 0 && neeg-nomit_eeg < min_nchan) {
            qCritical("Too few EEG channels remaining");
            return FAIL;
        }
        f->noise = mne_dup_cov_3(f->noise_orig);
        if (nomit_meg+nomit_eeg > 0) {
            if (f->noise->cov) {
                for (j = 0; j < f->noise->ncov; j++)
                    for (k = 0; k <= j; k++) {
                        val = f->noise->cov+mne_lt_packed_index_3(j,k);
                        *val = w[j]*w[k]*(*val);
                    }
            }
            else {
                for (j = 0; j < f->noise->ncov; j++) {
                    val  = f->noise->cov_diag+j;
                    *val = w[j]*w[j]*(*val);
                }
            }
        }
        FREE_3(w);
    }
    else {
        if (f->noise && f->nave == nave)
            return OK;
        f->noise = mne_dup_cov_3(f->noise_orig);
    }

    return scale_dipole_fit_noise_cov(f,nave);
}


//*************************************************************************************************************

DipoleFitData *DipoleFitData::setup_dipole_fit_data(const QString &mriname, const QString &measname, char *bemname, Vector3f *r0, FwdEegSphereModel *eeg_model, int accurate_coils, const QString &badname, const QString &noisename, float grad_std, float mag_std, float eeg_std, float mag_reg, float grad_reg, float eeg_reg, int diagnoise, const QList<QString> &projnames, int include_meg, int include_eeg)              /**< Include EEG in the fitting? */
/*
          * Background work for modelling
          */
{
    DipoleFitData*  res = new DipoleFitData;
    int            k;
    char           **badlist = NULL;
    int            nbad      = 0;
    char           **file_bads;
    int            file_nbad;
    int            coord_frame = FIFFV_COORD_HEAD;
    mneCovMatrix cov;
    FwdCoilSet*     templates = NULL;
    mneCTFcompDataSet comp_data  = NULL;
    FwdCoilSet*        comp_coils = NULL;

    /*
       * Read the coordinate transformations
       */
    if (!mriname.isEmpty()) {
        if ((res->mri_head_t = FiffCoordTransOld::mne_read_mri_transform(mriname)) == NULL)
            goto bad;
    }
    else if (bemname) {
        qWarning("Source of MRI / head transform required for the BEM model is missing");
        goto bad;
    }
    else {
        float move[] = { 0.0, 0.0, 0.0 };
        float rot[3][3] = { { 1.0, 0.0, 0.0 },
                            { 0.0, 1.0, 0.0 },
                            { 0.0, 0.0, 1.0 } };
        res->mri_head_t = new FiffCoordTransOld(FIFFV_COORD_MRI,FIFFV_COORD_HEAD,rot,move);
    }

    FiffCoordTransOld::mne_print_coord_transform(stderr,res->mri_head_t);
    if ((res->meg_head_t = FiffCoordTransOld::mne_read_meas_transform(measname)) == NULL)
        goto bad;
    FiffCoordTransOld::mne_print_coord_transform(stderr,res->meg_head_t);
    /*
       * Read the bad channel lists
       */
    if (!badname.isEmpty()) {
        if (mne_read_bad_channels_3(badname,&badlist,&nbad) != OK)
            goto bad;
        printf("%d bad channels read from %s.\n",nbad,badname.toLatin1().data());
    }
    if (mne_read_bad_channel_list_3(measname,&file_bads,&file_nbad) == OK && file_nbad > 0) {
        if (!badlist)
            nbad = 0;
        badlist = REALLOC_3(badlist,nbad+file_nbad,char *);
        for (k = 0; k < file_nbad; k++)
            badlist[nbad++] = file_bads[k];
        FREE_3(file_bads);
        printf("%d bad channels read from the data file.\n",file_nbad);
    }
    printf("%d bad channels total.\n",nbad);
    /*
       * Read the channel information
       */
    if (read_meg_eeg_ch_info(measname,include_meg,include_eeg,badlist,nbad,
                             &res->chs,&res->nmeg,&res->neeg) != OK)
        goto bad;

    if (res->nmeg > 0)
        printf("Will use %3d MEG channels from %s\n",res->nmeg,measname.toLatin1().data());
    if (res->neeg > 0)
        printf("Will use %3d EEG channels from %s\n",res->neeg,measname.toLatin1().data());
    {
        char *s = mne_channel_names_to_string_3(res->chs,res->nmeg+res->neeg);
        int  n;
        mne_string_to_name_list_3(s,&res->ch_names,&n);
    }
    /*
       * Make coil definitions
       */
    res->coord_frame = coord_frame;
    if (coord_frame == FIFFV_COORD_HEAD) {
        //#ifdef USE_SHARE_PATH
        //        char *coilfile = mne_compose_mne_name("share/mne","coil_def.dat");
        //#else
        //    const char *path = "setup/mne";
        //    const char *filename = "coil_def.dat";
        //    const char *coilfile = mne_compose_mne_name(path,filename);

        //    QString qPath("/usr/pubsw/packages/mne/stable/share/mne/coil_def.dat");

        QString qPath = QString("./resources/coilDefinitions/coil_def.dat");
        QFile file(qPath);
        if ( !QCoreApplication::startingUp() )
            qPath = QCoreApplication::applicationDirPath() + QString("/resources/coilDefinitions/coil_def.dat");
        else if (!file.exists())
            qPath = "./bin/resources/coilDefinitions/coil_def.dat";

        char *coilfile = MALLOC_3(strlen(qPath.toLatin1().data())+1,char);
        strcpy(coilfile,qPath.toLatin1().data());
        //#endif

        if (!coilfile)
            goto bad;
        if ((templates = FwdCoilSet::read_coil_defs(coilfile)) == NULL) {
            FREE_3(coilfile);
            goto bad;
        }

        if ((res->meg_coils = templates->create_meg_coils(res->chs,res->nmeg,
                                                          accurate_coils ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                          res->meg_head_t)) == NULL)
            goto bad;
        if ((res->eeg_els = FwdCoilSet::create_eeg_els(res->chs+res->nmeg,res->neeg,NULL)) == NULL)
            goto bad;
        printf("Head coordinate coil definitions created.\n");
    }
    else {
        qWarning("Cannot handle computations in %s coordinates",mne_coord_frame_name_3(coord_frame));
        goto bad;
    }
    /*
       * Forward model setup
       */
    res->bemname   = mne_strdup_3(bemname);
    if (r0) {
        res->r0[0]     = (*r0)[0];
        res->r0[1]     = (*r0)[1];
        res->r0[2]     = (*r0)[2];
    }
    res->eeg_model = eeg_model;
    /*
       * Compensation data
       */
    if ((comp_data = mne_read_ctf_comp_data_3(measname)) == NULL)
        goto bad;
    if (comp_data->ncomp > 0) {	/* Compensation channel information may be needed */
        fiffChInfo comp_chs = NULL;
        int        ncomp    = 0;

        printf("%d compensation data sets in %s\n",comp_data->ncomp,measname.toLatin1().data());
        if (mne_read_meg_comp_eeg_ch_info_3(measname,NULL,0,&comp_chs,&ncomp,NULL,NULL,NULL,NULL) == FAIL)
            goto bad;
        if (ncomp > 0) {
            if ((comp_coils = templates->create_meg_coils(comp_chs,ncomp,
                                                          FWD_COIL_ACCURACY_NORMAL,res->meg_head_t)) == NULL) {
                FREE_3(comp_chs);
                goto bad;
            }
            printf("%d compensation channels in %s\n",comp_coils->ncoil,measname.toLatin1().data());
        }
        FREE_3(comp_chs);
    }
    else {			/* Get rid of the empty data set */
        mne_free_ctf_comp_data_set_3(comp_data);
        comp_data = NULL;
    }
    /*
       * Ready to set up the forward model
       */
    if (setup_forward_model(res,comp_data,comp_coils) == FAIL)
        goto bad;
    res->column_norm = COLUMN_NORM_LOC;
    /*
       * Projection data should go here
       */
    if (make_projection(projnames,res->chs,res->nmeg+res->neeg,&res->proj) == FAIL)
        goto bad;
    if (res->proj && res->proj->nitems > 0) {
        fprintf(stderr,"Final projection operator is:\n");
        mne_proj_op_report_3(stderr,"\t",res->proj);

        if (mne_proj_op_chs_3(res->proj,res->ch_names,res->nmeg+res->neeg) == FAIL)
            goto bad;
        if (mne_proj_op_make_proj(res->proj) == FAIL)
            goto bad;
    }
    else
        printf("No projection will be applied to the data.\n");

    /*
        * Noise covariance
        */
    if (!noisename.isEmpty()) {
        if ((cov = mne_read_cov(noisename,FIFFV_MNE_SENSOR_COV)) == NULL)
            goto bad;
        printf("Read a %s noise-covariance matrix from %s\n",
               cov->cov_diag ? "diagonal" : "full", noisename.toLatin1().data());
    }
    else {
        if ((cov = ad_hoc_noise(res->meg_coils,res->eeg_els,grad_std,mag_std,eeg_std)) == NULL)
            goto bad;
    }
    res->noise = mne_pick_chs_cov_omit(cov,res->ch_names,res->nmeg+res->neeg,TRUE,res->chs);
    if (res->noise == NULL) {
        mne_free_cov_3(cov);
        goto bad;
    }

    printf("Picked appropriate channels from the noise-covariance matrix.\n");
    mne_free_cov_3(cov);

    /*
       * Apply the projection operator to the noise-covariance matrix
       */
    if (res->proj && res->proj->nitems > 0 && res->proj->nvec > 0) {
        if (mne_proj_op_apply_cov(res->proj,res->noise) == FAIL)
            goto bad;
        printf("Projection applied to the covariance matrix.\n");
    }

    /*
       * Force diagonal noise covariance?
       */
    if (diagnoise) {
        mne_revert_to_diag_cov(res->noise);
        fprintf(stderr,"Using only the main diagonal of the noise-covariance matrix.\n");
    }

    /*
       * Regularize the possibly deficient noise-covariance matrix
       */
    if (res->noise->cov) {
        float regs[3];
        int   do_it;

        regs[MNE_COV_CH_MEG_MAG]  = mag_reg;
        regs[MNE_COV_CH_MEG_GRAD] = grad_reg;
        regs[MNE_COV_CH_EEG]      = eeg_reg;
        /*
         * Classify the channels
         */
        if (mne_classify_channels_cov(res->noise,res->chs,res->nmeg+res->neeg) == FAIL)
            goto bad;
        /*
         * Do we need to do anything?
         */
        for (k = 0, do_it = 0; k < res->noise->ncov; k++) {
            if (res->noise->ch_class[k] != MNE_COV_CH_UNKNOWN &&
                    regs[res->noise->ch_class[k]] > 0.0)
                do_it++;
        }
        /*
         * Apply regularization if necessary
         */
        if (do_it > 0)
            mne_regularize_cov(res->noise,regs);
        else
            printf("No regularization applied to the noise-covariance matrix\n");
    }

    /*
       * Do the decomposition and check that the matrix is positive definite
       */
    fprintf(stderr,"Decomposing the noise covariance...\n");
    if (res->noise->cov) {
        if (mne_decompose_eigen_cov_3(res->noise) == FAIL)
            goto bad;
        fprintf(stderr,"Eigenvalue decomposition done.\n");
        for (k = 0; k < res->noise->ncov; k++) {
            if (res->noise->lambda[k] < 0.0)
                res->noise->lambda[k] = 0.0;
        }
    }
    else {
        printf("Decomposition not needed for a diagonal covariance matrix.\n");
        if (mne_add_inv_cov_3(res->noise) == FAIL)
            goto bad;
    }

    mne_free_name_list_3(badlist,nbad);
    delete templates;
    delete comp_coils;
    mne_free_ctf_comp_data_set_3(comp_data);
    return res;


bad : {
        mne_free_name_list_3(badlist,nbad);
        delete templates;
        delete comp_coils;
        mne_free_ctf_comp_data_set_3(comp_data);
        if(res)
            delete res;
        return NULL;
    }
}


//*************************************************************************************************************

//============================= dipole_forward.c =============================

void print_fields(float       *rd,
                  float       *Q,
                  float       time,
                  float       integ,
                  DipoleFitData* fit,
                  MneMeasData* data)

{
    float *one = MALLOC_3(data->nchan,float);
    int   k;
    float **fwd = NULL;

    if (mne_get_values_from_data_3(time,integ,data->current->data,data->current->np,data->nchan,data->current->tmin,
                                   1.0/data->current->tstep,FALSE,one) == FAIL) {
        fprintf(stderr,"Cannot pick time: %7.1f ms\n",1000*time);
        return;
    }
    for (k = 0; k < data->nchan; k++)
        if (data->chs[k].chpos.coil_type == FIFFV_COIL_CTF_REF_GRAD ||
                data->chs[k].chpos.coil_type == FIFFV_COIL_CTF_OFFDIAG_REF_GRAD) {
            printf("%g ",1e15*one[k]);
        }
    printf("\n");


    fwd = ALLOC_CMATRIX_3(3,fit->nmeg+fit->neeg);
    if (DipoleFitData::compute_dipole_field(fit,rd,FALSE,fwd) == FAIL)
        goto out;

    for (k = 0; k < data->nchan; k++)
        if (data->chs[k].chpos.coil_type == FIFFV_COIL_CTF_REF_GRAD ||
                data->chs[k].chpos.coil_type == FIFFV_COIL_CTF_OFFDIAG_REF_GRAD) {
            printf("%g ",1e15*(Q[X_3]*fwd[X_3][k]+Q[Y_3]*fwd[Y_3][k]+Q[Z_3]*fwd[Z_3][k]));
        }
    printf("\n");

out : {
        FREE_3(one);
        FREE_CMATRIX_3(fwd);
    }
    return;
}


//*************************************************************************************************************

DipoleForward* dipole_forward(DipoleFitData* d,
                              float         **rd,
                              int           ndip,
                              DipoleForward* old)
/*
 * Compute the forward solution and do other nice stuff
 */
{
    DipoleForward* res;
    float         **this_fwd;
    float         S[3];
    int           k,p;
    /*
   * Allocate data if necessary
   */
    if (old && old->ndip == ndip && old->nch == d->nmeg+d->neeg) {
        res = old;
    }
    else {
        delete old; old = NULL;
        res = new DipoleForward;
        res->fwd  = ALLOC_CMATRIX_3(3*ndip,d->nmeg+d->neeg);
        res->uu   = ALLOC_CMATRIX_3(3*ndip,d->nmeg+d->neeg);
        res->vv   = ALLOC_CMATRIX_3(3*ndip,3);
        res->sing = MALLOC_3(3*ndip,float);
        res->nch  = d->nmeg+d->neeg;
        res->rd   = ALLOC_CMATRIX_3(ndip,3);
        res->scales = MALLOC_3(3*ndip,float);
        res->ndip = ndip;
    }

    for (k = 0; k < ndip; k++) {
        VEC_COPY_3(res->rd[k],rd[k]);
        this_fwd = res->fwd + 3*k;
        /*
     * Calculate the field of three orthogonal dipoles
     */
        if ((DipoleFitData::compute_dipole_field(d,rd[k],TRUE,this_fwd)) == FAIL)
            goto bad;
        /*
     * Choice of column normalization
     * (componentwise normalization is not recommended)
     */
        if (d->column_norm == COLUMN_NORM_LOC || d->column_norm == COLUMN_NORM_COMP) {
            for (p = 0; p < 3; p++)
                S[p] = mne_dot_vectors_3(res->fwd[3*k+p],res->fwd[3*k+p],res->nch);
            if (d->column_norm == COLUMN_NORM_COMP) {
                for (p = 0; p < 3; p++)
                    res->scales[3*k+p] = sqrt(S[p]);
            }
            else {
                /*
     * Divide by three or not?
     */
                res->scales[3*k+0] = res->scales[3*k+1] = res->scales[3*k+2] = sqrt(S[0]+S[1]+S[2])/3.0;
            }
            for (p = 0; p < 3; p++) {
                if (res->scales[3*k+p] > 0.0) {
                    res->scales[3*k+p] = 1.0/res->scales[3*k+p];
                    mne_scale_vector_3(res->scales[3*k+p],res->fwd[3*k+p],res->nch);
                }
                else
                    res->scales[3*k+p] = 1.0;
            }
        }
        else {
            res->scales[3*k]   = 1.0;
            res->scales[3*k+1] = 1.0;
            res->scales[3*k+2] = 1.0;
        }
    }

    /*
   * SVD
   */
    if (mne_svd_3(res->fwd,3*ndip,d->nmeg+d->neeg,res->sing,res->vv,res->uu) != 0)
        goto bad;

    return res;

bad : {
        if (!old)
            delete res;
        return NULL;
    }
}


//*************************************************************************************************************

DipoleForward* DipoleFitData::dipole_forward_one(DipoleFitData* d,
                                                 float         *rd,
                                                 DipoleForward* old)
/*
 * Convenience function to compute the field of one dipole
 */
{
    float *rds[1];
    rds[0] = rd;
    return dipole_forward(d,rds,1,old);
}


//*************************************************************************************************************
// fit_dipoles.c
static float fit_eval(float *rd,int npar,void *user)
/*
 * Calculate the residual sum of squares
 */
{
    DipoleFitData* fit   = (DipoleFitData*)user;
    DipoleForward* fwd;
    fitDipUser       fuser = (fitDipUser)fit->user;
    double        Bm2,one;
    int           ncomp,c;

    fwd   = fuser->fwd = DipoleFitData::dipole_forward_one(fit,rd,fuser->fwd);
    ncomp = fwd->sing[2]/fwd->sing[0] > fuser->limit ? 3 : 2;
    if (fuser->report_dim)
        fprintf(stderr,"ncomp = %d\n",ncomp);

    for (c = 0, Bm2 = 0.0; c < ncomp; c++) {
        one = mne_dot_vectors_3(fwd->uu[c],fuser->B,fwd->nch);
        Bm2 = Bm2 + one*one;
    }
    return fuser->B2-Bm2;
}








static int find_best_guess(float     *B,         /* The whitened data */
                           int       nch,
                           GuessData* guess,	 /* Guesses */
                           float     limit,	 /* Pseudoradial component omission limit */
                           int       *bestp,	 /* Which is the best */
                           float     *goodp)	 /* Best goodness of fit */
/*
 * Thanks to the precomputed SVD everything is really simple
 */
{
    int    k,c;
    double B2,Bm2,this_good,one;
    int    best = -1;
    float  good = 0.0;
    DipoleForward* fwd;
    int    ncomp;

    B2 = mne_dot_vectors_3(B,B,nch);
    for (k = 0; k < guess->nguess; k++) {
        fwd = guess->guess_fwd[k];
        if (fwd->nch == nch) {
            ncomp = fwd->sing[2]/fwd->sing[0] > limit ? 3 : 2;
            for (c = 0, Bm2 = 0.0; c < ncomp; c++) {
                one = mne_dot_vectors_3(fwd->uu[c],B,nch);
                Bm2 = Bm2 + one*one;
            }
            this_good = 1.0 - (B2 - Bm2)/B2;
            if (this_good > good) {
                best = k;
                good = this_good;
            }
        }
    }
    if (best < 0) {
        printf("No reasonable initial guess found.");
        return FAIL;
    }
    *bestp = best;
    *goodp = good;
    return OK;
}












static float **make_initial_dipole_simplex(float  *r0,
                                           float  size)
/*
      * Make the initial tetrahedron
      */
{
    /*
   * For this definition of a regular tetrahedron, see
   *
   * http://mathworld.wolfram.com/Tetrahedron.html
   *
   */
    float x = sqrt(3.0)/3.0;
    float r = sqrt(6.0)/12.0;
    float R = 3*r;
    float d = x/2.0;
    float rr[][3] = { { x , 0.0,  -r },
                      { -d, 0.5,  -r },
                      { -d, -0.5, -r },
                      { 0.0, 0.0, R } };

    float **simplex = ALLOC_CMATRIX_3(4,3);
    int j,k;

    for (j = 0; j < 4; j++) {
        VEC_COPY_3(simplex[j],rr[j]);
        for (k = 0; k < 3; k++)
            simplex[j][k] = size*simplex[j][k] + r0[k];
    }
    return simplex;
}


static int report_func(int     loop,
                       float   *fitpar,
                       int     npar,
                       double  fval_lo,
                       double  fval_hi,
                       double  par_diff)
/*
      * Report periodically
      */
{
    float *r0 = fitpar;

    fprintf(stdout,"loop %d rd %7.2f %7.2f %7.2f fval %g %g par diff %g\n",
            loop,1000*r0[0],1000*r0[1],1000*r0[2],fval_lo,fval_hi,1000*par_diff);

    return OK;
}












static int fit_Q(DipoleFitData* fit,	     /* The fit data */
                 float *B,		     /* Measurement */
                 float *rd,		     /* Dipole position */
                 float limit,		     /* Radial component omission limit */
                 float *Q,		     /* The result */
                 int   *ncomp,
                 float *res)	             /* Residual sum of squares */
/*
 * fit the dipole moment once the location is known
 */
{
    int c;
    DipoleForward* fwd = DipoleFitData::dipole_forward_one(fit,rd,NULL);
    float Bm2,one;

    if (!fwd)
        return FAIL;

    *ncomp = fwd->sing[2]/fwd->sing[0] > limit ? 3 : 2;

    Q[0] = Q[1] = Q[2] = 0.0;
    for (c = 0, Bm2 = 0.0; c < *ncomp; c++) {
        one = mne_dot_vectors_3(fwd->uu[c],B,fwd->nch);
        mne_add_scaled_vector_to_3(fwd->vv[c],one/fwd->sing[c],Q,3);
        Bm2 = Bm2 + one*one;
    }
    /*
   * Counteract the effect of column normalization
   */
    for (c = 0; c < 3; c++)
        Q[c] = fwd->scales[c]*Q[c];
    *res = mne_dot_vectors_3(B,B,fwd->nch) - Bm2;

    delete fwd;

    return OK;
}


static float rtol(float *vals,int nval)

{
    float minv,maxv;
    int   k;

    minv = maxv = vals[0];
    for (k = 1; k < nval; k++) {
        if (vals[k] < minv)
            minv = vals[k];
        if (vals[k] > maxv)
            maxv = vals[k];
    }
    return 2.0*(maxv-minv)/(maxv+minv);
}












/*
 * This routine comes from Numerical recipes
 */

#define ALPHA 1.0
#define BETA 0.5
#define GAMMA 2.0
#define MIN_STOL_LOOP 5

static float tryf (float **p,
                   float *y,
                   float *psum,
                   int   ndim,
                   float (*func)(float *x,int npar,void *user_data),	  /* The function to be evaluated */
                   void  *user_data,				          /* Data to be passed to the above function in each evaluation */
                   int   ihi,
                   int   *neval,
                   float fac)

{
    int j;
    float fac1,fac2,ytry,*ptry;

    ptry = ALLOC_FLOAT_3(ndim);
    fac1 = (1.0-fac)/ndim;
    fac2 = fac1-fac;
    for (j = 0; j < ndim; j++)
        ptry[j] = psum[j]*fac1-p[ihi][j]*fac2;
    ytry = (*func)(ptry,ndim,user_data);
    ++(*neval);
    if (ytry < y[ihi]) {
        y[ihi] = ytry;
        for (j = 0; j < ndim; j++) {
            psum[j] +=  ptry[j]-p[ihi][j];
            p[ihi][j] = ptry[j];
        }
    }
    FREE_3(ptry);
    return ytry;
}









int simplex_minimize(float **p,		                              /* The initial simplex */
                     float *y,		                              /* Function values at the vertices */
                     int   ndim,	                              /* Number of variables */
                     float ftol,	                              /* Relative convergence tolerance */
                     float stol,
                     float (*func)(float *x,int npar,void *user_data),/* The function to be evaluated */
                     void  *user_data,				      /* Data to be passed to the above function in each evaluation */
                     int   max_eval,	                              /* Maximum number of function evaluations */
                     int   *neval,	                              /* Number of function evaluations */
                     int   report,                                    /* How often to report (-1 = no_reporting) */
                     int   (*report_func)(int loop,
                                          float *fitpar, int npar,
                                          double fval_lo,
                                          double fval_hi,
                                          double par_diff))            /* The function to be called when reporting */
/*
      * Minimization with the simplex algorithm
      * Modified from Numerical recipes
      */

{
    int   i,j,ilo,ihi,inhi;
    int   mpts = ndim+1;
    float ytry,ysave,sum,rtol,*psum;
    double dsum,diff;
    int   result = 0;
    int   count = 0;
    int   loop  = 1;

    psum = ALLOC_FLOAT_3(ndim);
    *neval = 0;
    for (j = 0; j < ndim; j++) {
        for (i = 0,sum = 0.0; i<mpts; i++)
            sum +=  p[i][j];
        psum[j] = sum;
    }
    if (report_func != NULL && report > 0)
        (void)report_func (0,p[0],ndim,-1.0,-1.0,0.0);

    dsum = 0.0;
    for (;;count++,loop++) {
        ilo = 1;
        ihi  =  y[1]>y[2] ? (inhi = 2,1) : (inhi = 1,2);
        for (i = 0; i < mpts; i++) {
            if (y[i]  <  y[ilo]) ilo = i;
            if (y[i] > y[ihi]) {
                inhi = ihi;
                ihi = i;
            } else if (y[i] > y[inhi])
                if (i !=  ihi) inhi = i;
        }
        rtol = 2.0*fabs(y[ihi]-y[ilo])/(fabs(y[ihi])+fabs(y[ilo]));
        /*
     * Report that we are proceeding...
     */
        if (count == report && report_func != NULL) {
            if (report_func (loop,p[ilo],ndim,y[ilo],y[ihi],sqrt(dsum))) {
                printf("Interation interrupted.");
                result = -1;
                break;
            }
            count = 0;
        }
        if (rtol < ftol) break;
        if (*neval >=  max_eval) {
            printf("Maximum number of evaluations exceeded.");
            result  =  -1;
            break;
        }
        if (stol > 0) {		/* Has the simplex collapsed? */
            for (dsum = 0.0, j = 0; j < ndim; j++) {
                diff = p[ilo][j] - p[ihi][j];
                dsum += diff*diff;
            }
            if (loop > MIN_STOL_LOOP && sqrt(dsum) < stol)
                break;
        }
        ytry = tryf(p,y,psum,ndim,func,user_data,ihi,neval,-ALPHA);
        if (ytry <= y[ilo])
            ytry = tryf(p,y,psum,ndim,func,user_data,ihi,neval,GAMMA);
        else if (ytry >= y[inhi]) {
            ysave = y[ihi];
            ytry = tryf(p,y,psum,ndim,func,user_data,ihi,neval,BETA);
            if (ytry >= ysave) {
                for (i = 0; i < mpts; i++) {
                    if (i !=  ilo) {
                        for (j = 0; j < ndim; j++) {
                            psum[j] = 0.5*(p[i][j]+p[ilo][j]);
                            p[i][j] = psum[j];
                        }
                        y[i] = (*func)(psum,ndim,user_data);
                    }
                }
                *neval +=  ndim;
                for (j = 0; j < ndim; j++) {
                    for (i = 0,sum = 0.0; i < mpts; i++)
                        sum +=  p[i][j];
                    psum[j] = sum;
                }
            }
        }
    }
    FREE_3 (psum);
    return (result);
}


//*************************************************************************************************************
// fit_dipoles.c
bool DipoleFitData::fit_one(DipoleFitData* fit,	            /* Precomputed fitting data */
                    GuessData*     guess,	            /* The initial guesses */
                    float         time,              /* Which time is it? */
                    float         *B,	            /* The field to fit */
                    int           verbose,
                    ECD&          res               /* The fitted dipole */
                    )
{
    float  **simplex       = NULL;	       /* The simplex */
    float  vals[4];			       /* Values at the vertices */
    float  limit           = 0.2;	               /* (pseudo) radial component omission limit */
    float  size            = 1e-2;	       /* Size of the initial simplex */
    float  ftol[]          = { 1e-2, 1e-2 };     /* Tolerances on the the two passes */
    float  atol[]          = { 0.2e-3, 0.2e-3 }; /* If dipole movement between two iterations is less than this,
                                                  we consider to have converged */
    int    ntol            = 2;
    int    max_eval        = 1000;	       /* Limit for fit function evaluations */
    int    report_interval = verbose ? 1 : -1;   /* How often to report the intermediate result */

    int        best;
    float      good,rd_guess[3],rd_final[3],Q[3],final_val;
    fitDipUserRec user;
    int        k,p,neval,neval_tot,nchan,ncomp;
    int        fit_fail;

    nchan = fit->nmeg+fit->neeg;
    user.fwd = NULL;

    if (mne_proj_op_proj_vector_3(fit->proj,B,nchan,TRUE) == FAIL)
        goto bad;

    if (mne_whiten_one_data(B,B,nchan,fit->noise) == FAIL)
        goto bad;
    /*
   * Get the initial guess
   */
    if (find_best_guess(B,nchan,guess,limit,&best,&good) < 0)
        goto bad;


    user.limit = limit;
    user.B     = B;
    user.B2    = mne_dot_vectors_3(B,B,nchan);
    user.fwd   = NULL;
    user.report_dim = FALSE;
    fit->user  = &user;

    VEC_COPY_3(rd_guess,guess->rr[best]);
    VEC_COPY_3(rd_final,guess->rr[best]);

    neval_tot = 0;
    fit_fail = FALSE;
    for (k = 0; k < ntol; k++) {
        /*
     * Do first pass with the sphere model
     */
        if (k == 0)
            fit->funcs = fit->sphere_funcs;
        else
            fit->funcs = fit->bemname ? fit->bem_funcs : fit->sphere_funcs;

        simplex = make_initial_dipole_simplex(rd_guess,size);
        for (p = 0; p < 4; p++)
            vals[p] = fit_eval(simplex[p],3,fit);
        if (simplex_minimize(simplex,           /* The initial simplex */
                             vals,              /* Function values at the vertices */
                             3,	            /* Number of variables */
                             ftol[k],           /* Relative convergence tolerance for the target function */
                             atol[k],           /* Absolute tolerance for the change in the parameters */
                             fit_eval,          /* The function to be evaluated */
                             fit,	            /* Data to be passed to the above function in each evaluation */
                             max_eval,          /* Maximum number of function evaluations */
                             &neval,            /* Number of function evaluations */
                             report_interval,   /* How often to report (-1 = no_reporting) */
                             report_func) != OK) {
            if (k == 0)
                goto bad;
            else {
                printf("\nWarning (t = %8.1f ms) : g = %6.1f %% final val = %7.3f rtol = %f\n",
                       1000*time,100*(1 - vals[0]/user.B2),vals[0],rtol(vals,4));
                fit_fail = TRUE;
            }
        }
        VEC_COPY_3(rd_final,simplex[0]);
        VEC_COPY_3(rd_guess,simplex[0]);
        FREE_CMATRIX_3(simplex); simplex = NULL;

        neval_tot += neval;
        final_val  = vals[0];
    }
    /*
   * Confidence limits should be computed here
   */
    /*
   * Compute the dipole moment at the final point
   */
    if (fit_Q(fit,user.B,rd_final,user.limit,Q,&ncomp,&final_val) == OK) {
        res.time  = time;
        res.valid = true;
        for(int i = 0; i < 3; ++i)
            res.rd[i] = rd_final[i];
        for(int i = 0; i < 3; ++i)
            res.Q[i] = Q[i];
        res.good  = 1.0 - final_val/user.B2;
        if (fit_fail)
            res.good = -res.good;
        res.khi2  = final_val;
        if (fit->proj)
            res.nfree = nchan-3-ncomp-fit->proj->nvec;
        else
            res.nfree = nchan-3-ncomp;
        res.neval = neval_tot;
    }
    else
        goto bad;
    delete user.fwd;
    FREE_CMATRIX_3(simplex);

    return true;

bad : {
        delete user.fwd;
        FREE_CMATRIX_3(simplex);
        return false;
    }
}












//*************************************************************************************************************

int DipoleFitData::compute_dipole_field(DipoleFitData* d, float *rd, int whiten, float **fwd)
/*
 * Compute the field and take whitening and projection into account
 */
{
    float *eeg_fwd[3];
    static float Qx[] = {1.0,0.0,0.0};
    static float Qy[] = {0.0,1.0,0.0};
    static float Qz[] = {0.0,0.0,1.0};
    int k;
    /*
   * Compute the fields
   */
    if (d->nmeg > 0) {
        if (d->funcs->meg_vec_field) {
            if (d->funcs->meg_vec_field(rd,d->meg_coils,fwd,d->funcs->meg_client) != OK)
                goto bad;
        }
        else {
            if (d->funcs->meg_field(rd,Qx,d->meg_coils,fwd[0],d->funcs->meg_client) != OK)
                goto bad;
            if (d->funcs->meg_field(rd,Qy,d->meg_coils,fwd[1],d->funcs->meg_client) != OK)
                goto bad;
            if (d->funcs->meg_field(rd,Qz,d->meg_coils,fwd[2],d->funcs->meg_client) != OK)
                goto bad;
        }
    }

    if (d->neeg > 0) {
        if (d->funcs->eeg_vec_pot) {
            eeg_fwd[0] = fwd[0]+d->nmeg;
            eeg_fwd[1] = fwd[1]+d->nmeg;
            eeg_fwd[2] = fwd[2]+d->nmeg;
            if (d->funcs->eeg_vec_pot(rd,d->eeg_els,eeg_fwd,d->funcs->eeg_client) != OK)
                goto bad;
        }
        else {
            if (d->funcs->eeg_pot(rd,Qx,d->eeg_els,fwd[0]+d->nmeg,d->funcs->eeg_client) != OK)
                goto bad;
            if (d->funcs->eeg_pot(rd,Qy,d->eeg_els,fwd[1]+d->nmeg,d->funcs->eeg_client) != OK)
                goto bad;
            if (d->funcs->eeg_pot(rd,Qz,d->eeg_els,fwd[2]+d->nmeg,d->funcs->eeg_client) != OK)
                goto bad;
        }
    }

    /*
   * Apply projection
   */
#ifdef DEBUG
    fprintf(stdout,"orig : ");
    for (k = 0; k < 3; k++)
        fprintf(stdout,"%g ",sqrt(mne_dot_vectors_3(fwd[k],fwd[k],d->nmeg+d->neeg)));
    fprintf(stdout,"\n");
#endif

    for (k = 0; k < 3; k++)
        if (mne_proj_op_proj_vector_3(d->proj,fwd[k],d->nmeg+d->neeg,TRUE) == FAIL)
            goto bad;

#ifdef DEBUG
    fprintf(stdout,"proj : ");
    for (k = 0; k < 3; k++)
        fprintf(stdout,"%g ",sqrt(mne_dot_vectors_3(fwd[k],fwd[k],d->nmeg+d->neeg)));
    fprintf(stdout,"\n");
#endif

    /*
   * Whiten
   */
    if (d->noise && whiten) {
        if (mne_whiten_data(fwd,fwd,3,d->nmeg+d->neeg,d->noise) == FAIL)
            goto bad;
    }

#ifdef DEBUG
    fprintf(stdout,"white : ");
    for (k = 0; k < 3; k++)
        fprintf(stdout,"%g ",sqrt(mne_dot_vectors_3(fwd[k],fwd[k],d->nmeg+d->neeg)));
    fprintf(stdout,"\n");
#endif

    return OK;

bad :
    return FAIL;
}
