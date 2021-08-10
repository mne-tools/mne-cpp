
#include <fwd/fwd_types.h>

#include "dipole_fit_data.h"
#include "guess_data.h"
#include "../c/mne_meas_data.h"
#include "../c/mne_meas_data_set.h"
#include <mne/c/mne_proj_item.h>
#include <mne/c/mne_cov_matrix.h>
#include "ecd.h"

#include <fiff/fiff_stream.h>
#include <fwd/fwd_bem_model.h>
#include <mne/c/mne_surface_old.h>

#include <fwd/fwd_comp_data.h>

#include <Eigen/Dense>

#include <QFile>
#include <QCoreApplication>
#include <QDebug>

#define _USE_MATH_DEFINES
#include <math.h>

using namespace Eigen;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace FWDLIB;
using namespace INVERSELIB;

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

//============================= mne_named_matrix.c =============================

QString mne_name_list_to_string_3(const QStringList& list)
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

QString mne_channel_names_to_string_3(const QList<FiffChInfo>& chs, int nch)
/*
 * Make a colon-separated string out of channel names
 */
{
    QStringList names;
    QString res;
    int  k;

    if (nch <= 0)
        return NULL;
    for (k = 0; k < nch; k++)
        names.append(chs[k].ch_name);
    res = mne_name_list_to_string_3(names);
    names.clear();
    return res;
}

void mne_string_to_name_list_3(const QString& s, QStringList& listp,int &nlistp)
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

//============================= mne_sparse_matop.c =============================

FiffSparseMatrix* mne_convert_to_sparse_3(float **dense,        /* The dense matrix to be converted */
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
            small = maxval * std::fabs(small);
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

namespace MNELIB
{

typedef struct {
    float          limit;
    int            report_dim;
    float          *B;
    double         B2;
    DipoleForward*  fwd;
} *fitDipUser,fitDipUserRec;

}

int mne_is_diag_cov_3(MneCovMatrix* c)

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

void mne_free_cov_3(MneCovMatrix* c)
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
    c->names.clear();
    FREE_CMATRIX_3(c->eigen);
    FREE_3(c->lambda);
    FREE_3(c->inv_lambda);
    FREE_3(c->chol);
    FREE_3(c->ch_class);
    if(c->proj)
        delete c->proj;
    if (c->sss)
        delete c->sss;
    c->bads.clear();
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
        if (std::fabs(sfreq*integ) < EPS_3) { /* This is the single-sample case */
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
                if (std::fabs(f1 - 1.0) < 1e-3)
                    f1 = 1.0;
            }
            if (f1 < 1.0 && n1 > nsamp-2) {
                printf("Sample value out of range %d (0..%d) %.4f",n1,nsamp-1,f1);
                return(-1);
            }
            if (f1 < 1.0) {
                if (use_abs)
                    sum = f1*std::fabs(data[n1][ch]) + (1.0-f1)*std::fabs(data[n1+1][ch]);
                else
                    sum = f1*data[n1][ch] + (1.0-f1)*data[n1+1][ch];
            }
            else {
                if (use_abs)
                    sum = std::fabs(data[n1][ch]);
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
                    sum = 0.5*((f1+f2)*std::fabs(data[n1+1][ch]) + (2.0-f1-f2)*std::fabs(data[n1][ch]));
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
                        sum = 0.5 * std::fabs(data[n1][ch]);
                        for (k = n1+1; k < n2; k++)
                            sum = sum + std::fabs(data[k][ch]);
                        sum = sum + 0.5 * std::fabs(data[n2][ch]);
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
                        sum = sum + 0.5*f1*(f1*std::fabs(data[n1-1][ch]) + (2.0-f1)*std::fabs(data[n1][ch]));
                    if (f2 != 0.0)
                        sum = sum + 0.5*f2*(f2*std::fabs(data[n2+1][ch]) + (2.0-f2)*std::fabs(data[n2][ch]));
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
        if (std::fabs(mat[i]) > std::fabs(mat[maxi]))
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

int mne_add_inv_cov_3(MneCovMatrix* c)
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

static int condition_cov_3(MneCovMatrix* c, float rank_threshold, int use_rank)

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

int mne_classify_channels_cov(MneCovMatrix* cov,
                              const QList<FiffChInfo>& chs,
                              int nchan)
/*
 * Assign channel classes in a covariance matrix with help of channel infos
 */
{
    int k,p;
    FiffChInfo ch;

    if (chs.isEmpty()) {
        qCritical("Channel information not available in mne_classify_channels_cov");
        goto bad;
    }
    cov->ch_class = REALLOC_3(cov->ch_class,cov->ncov,int);
    for (k = 0; k < cov->ncov; k++) {
        cov->ch_class[k] = MNE_COV_CH_UNKNOWN;
        for (p = 0; p < nchan; p++) {
            if (QString::compare(chs[p].ch_name,cov->names[k]) == 0) {
                ch = chs[p];
                if (ch.kind == FIFFV_MEG_CH) {
                    if (ch.unit == FIFF_UNIT_T)
                        cov->ch_class[k] = MNE_COV_CH_MEG_MAG;
                    else
                        cov->ch_class[k] = MNE_COV_CH_MEG_GRAD;
                }
                else if (ch.kind == FIFFV_EEG_CH)
                    cov->ch_class[k] = MNE_COV_CH_EEG;
                break;
            }
        }
//        if (!ch) {
//            printf("Could not find channel info for channel %s in mne_classify_channels_cov",cov->names[k].toUtf8().constData());
//            goto bad;
//        }
    }
    return OK;

bad : {
        FREE_3(cov->ch_class);
        cov->ch_class = NULL;
        return FAIL;
    }
}

static int mne_decompose_eigen_cov_small_3(MneCovMatrix* c,float small, int use_rank)
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
                        eeglike += std::fabs(c->eigen[k][p]);
                    else if (c->ch_class[p] == MNE_COV_CH_MEG_MAG || c->ch_class[p] == MNE_COV_CH_MEG_GRAD)
                        meglike += std::fabs(c->eigen[k][p]);
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

int mne_decompose_eigen_cov_3(MneCovMatrix* c)

{
    return mne_decompose_eigen_cov_small_3(c,-1.0,-1);
}

//============================= mne_whiten.c =============================

int mne_whiten_data(float **data, float **whitened_data, int np, int nchan, MneCovMatrix* C)
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

int mne_whiten_one_data(float *data, float *whitened_data, int nchan, MneCovMatrix* C)

{
    float *datap[1];
    float *whitened_datap[1];

    datap[0] = data;
    whitened_datap[0] = whitened_data;

    return mne_whiten_data(datap,whitened_datap,1,nchan,C);
}

//=============================================================================================================
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

//static void regularize_cov(MneCovMatrix* c,       /* The matrix to regularize */
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

void mne_regularize_cov(MneCovMatrix* c,       /* The matrix to regularize */
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
        rtol = 2.0*std::fabs(y[ihi]-y[ilo])/(std::fabs(y[ihi])+std::fabs(y[ilo]));
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

//============================= fit_sphere.c =============================

namespace MNELIB
{

typedef struct {
    float **rr;
    int   np;
    int   report;
} *fitSphereUser,fitSphereUserRec;

}

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

//============================= mne_lin_proj.c =============================

void mne_proj_op_report_data_3(FILE *out,const char *tag, MneProjOp* op, int list_data,
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
                k+1,it->desc.toUtf8().constData(),it->nvec,it->vecs->ncol,
                it->has_meg ? "MEG" : "EEG",
                it->active ? "active" : "idle");
        if (list_data && tag)
            fprintf(out,"%s\n",tag);
        if (list_data) {
            vecs = op->items[k]->vecs;

            for (q = 0; q < vecs->ncol; q++) {
                fprintf(out,"%-10s",vecs->collist[q].toUtf8().constData());
                fprintf(out,q < vecs->ncol-1 ? " " : "\n");
            }
            for (p = 0; p < vecs->nrow; p++)
                for (q = 0; q < vecs->ncol; q++) {
                    for (j = 0, found  = 0; j < nexclude; j++) {
                        if (QString::compare(exclude[j],vecs->collist[q]) == 0) {
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

void mne_proj_op_report_3(FILE *out,const char *tag, MneProjOp* op)
{
    mne_proj_op_report_data_3(out,tag,op, FALSE, NULL, 0);
}

//============================= mne_named_vector.c =============================

int mne_pick_from_named_vector_3(mneNamedVector vec, const QStringList& names, int nnames, int require_all, float *res)
/*
 * Pick the desired elements from the named vector
 */
{
    int found;
    int k,p;

    if (vec->names.size() == 0) {
        qCritical("No names present in vector. Cannot pick.");
        return FAIL;
    }

    for (k = 0; k < nnames; k++)
        res[k] = 0.0;

    for (k = 0; k < nnames; k++) {
        found = 0;
        for (p = 0; p < vec->nvec; p++) {
            if (QString::compare(vec->names[p],names[k]) == 0) {
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

MneProjOp* mne_read_proj_op_from_node_3(//fiffFile in,
                                     FiffStream::SPtr& stream,
                                     const FiffDirNode::SPtr& start)
/*
 * Load all the linear projection data
 */
{
    MneProjOp*  op     = NULL;
    QList<FiffDirNode::SPtr> proj;
    FiffDirNode::SPtr start_node;
    QList<FiffDirNode::SPtr> items;
    FiffDirNode::SPtr node;
    int         k;
    QString     item_desc, desc_tag;
    int         global_nchan,item_nchan,nlist;
    QStringList item_names;
    int         item_kind;
    float       **item_vectors = NULL;
    int         item_nvec;
    int         item_active;
    MneNamedMatrix* item;
    FiffTag::SPtr t_pTag;
    QStringList emptyList;
    int pos;

    if (!stream) {
        qCritical("File not open mne_read_proj_op_from_node");
        goto bad;
    }

    if (!start || start->isEmpty())
        start_node = stream->dirtree();
    else
        start_node = start;

    op = new MneProjOp();
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
        item_desc.clear();

        if (node->find_tag(stream, FIFF_NAME, t_pTag)) {
            item_desc += t_pTag->toString();
        }

        /*
        * Take the first line of description if it exists
        */
        if (node->find_tag(stream, FIFF_DESCRIPTION, t_pTag)) {
            desc_tag = t_pTag->toString();
            if((pos = desc_tag.indexOf("\n")) >= 0)
                desc_tag.truncate(pos);
            if (!item_desc.isEmpty())
                item_desc += " ";
            item_desc += desc_tag;
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

        mne_string_to_name_list_3(t_pTag->toString(),item_names,nlist);
        if (nlist != item_nchan) {
            printf("Channel name list incorrectly specified for proj item # %d",k+1);
            item_names.clear();
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
        item = MneNamedMatrix::build_named_matrix(item_nvec,item_nchan,emptyList,item_names,item_vectors);
        MneProjOp::mne_proj_op_add_item_act(op,item,item_kind,item_desc,item_active);
        delete item;
        op->items[op->nitems-1]->active_file = item_active;
    }

out :
    return op;

bad : {
        if(op)
            delete op;
        return NULL;
    }
}

MneProjOp* mne_read_proj_op_3(const QString& name)

{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    if(!stream->open())
        return NULL;

    MneProjOp*  res = NULL;

    FiffDirNode::SPtr t_default;
    res = mne_read_proj_op_from_node_3(stream,t_default);

    stream->close();

    return res;
}

int mne_proj_op_chs_3(MneProjOp* op, const QStringList& list, int nlist)

{
    if (op == NULL)
        return OK;

    MneProjOp::mne_free_proj_op_proj(op);  /* These data are not valid any more */

    if (nlist == 0)
        return OK;

    op->names = list;
    op->nch   = nlist;

    return OK;
}

static void clear_these(float *data, const QStringList& names, int nnames, const QString& start)

{
    int k;
    for (k = 0; k < nnames; k++)
        if (names[k].contains(start))//strstr(names[k],start) == names[k])
            data[k] = 0.0;
}

#define USE_LIMIT   1e-5
#define SMALL_VALUE 1e-4

int mne_proj_op_make_proj_bad(MneProjOp* op, char **bad, int nbad)
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

    nvec = MneProjOp::mne_proj_op_affect(op,op->names,op->nch);
    if (nvec == 0)
        return OK;

    mat_meg = ALLOC_CMATRIX_3(nvec, op->nch);
    mat_eeg = ALLOC_CMATRIX_3(nvec, op->nch);

#ifdef DEBUG
    fprintf(stdout,"mne_proj_op_make_proj_bad\n");
#endif
    for (k = 0, nvec_meg = nvec_eeg = 0; k < op->nitems; k++) {
        if (op->items[k]->active && MneProjItem::mne_proj_item_affect(op->items[k],op->names,op->nch)) {
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
            if (QString::compare(op->names[r],bad[q]) == 0) {
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
            if (std::fabs(vv_meg[p][k]) < SMALL_VALUE)
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
            if (std::fabs(vv_eeg[p][k]) < SMALL_VALUE)
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
        if (op->names[k].contains("STI")){ //strstr(op->names[k],"STI") == op->names[k]) {
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

int mne_proj_op_make_proj(MneProjOp* op)
/*
 * Do the channel picking and SVD
 */
{
    return mne_proj_op_make_proj_bad(op,NULL,0);
}

//============================= mne_read_forward_solution.c =============================

int mne_read_meg_comp_eeg_ch_info_3(const QString& name,
                                    QList<FiffChInfo>& megp,	 /* MEG channels */
                                    int            *nmegp,
                                    QList<FiffChInfo>& meg_compp,
                                    int            *nmeg_compp,
                                    QList<FiffChInfo>& eegp,	 /* EEG channels */
                                    int            *neegp,
                                    FiffCoordTransOld** meg_head_t,
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
    FiffChInfo   this_ch;
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

            for (j = 0; j < nchan; j++)
                chs.append(FiffChInfo());
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

    megp = meg;
    *nmegp = nmeg;

    meg_compp = meg_comp;
    *nmeg_compp = nmeg_comp;

    eegp = eeg;
    *neegp = neeg;

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
        FREE_3(id);
//        FREE_3(tag.data);
        FREE_3(t);
        return FIFF_FAIL;
    }
}

//============================= data.c =============================

#if defined(_WIN32) || defined(_WIN64)
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

int is_selected_in_data(mshMegEegData d, const QString& ch_name)
/*
 * Is this channel selected in data
 */
{
    int issel = FALSE;
    int k;

    for (k = 0; k < d->meas->nchan; k++)
        if (QString::compare(ch_name,d->meas->chs[k].ch_name,Qt::CaseInsensitive) == 0) {
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

int mne_read_bad_channels_3(const QString& name, QStringList& listp, int& nlistp)
/*
 * Read bad channel names
 */
{
    FILE *in = NULL;
    QStringList list;
    char line[MAXLINE+1];
    char *next;

    if (name.isEmpty())
        return OK;

    if ((in = fopen(name.toUtf8().data(),"r")) == NULL) {
        qCritical() << name;
        goto bad;
    }
    while ((next = next_line_3(line,MAXLINE,in)) != NULL) {
        if (strlen(next) > 0) {
            if (next[strlen(next)-1] == '\n')
                next[strlen(next)-1] = '\0';
            list.append(next);
        }
    }
    if (ferror(in))
        goto bad;

    listp  = list;
    nlistp = list.size();

    return OK;

bad : {
        list.clear();
        if (in != NULL)
            fclose(in);
        return FAIL;
    }
}

int mne_read_bad_channel_list_from_node_3(FiffStream::SPtr& stream,
                                          const FiffDirNode::SPtr& pNode,
                                          QStringList& listp,
                                          int& nlistp)
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
            mne_string_to_name_list_3(names,list,nlist);
        }
    }
    listp = list;
    nlistp = list.size();
    return OK;
}

int mne_read_bad_channel_list_3(const QString& name, QStringList& listp, int& nlistp)

{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    int res;

    if(!stream->open())
        return FAIL;

    res = mne_read_bad_channel_list_from_node_3(stream,stream->dirtree(),listp,nlistp);

    stream->close();

    return res;
}

MneCovMatrix* mne_read_cov(const QString& name,int kind)
/*
 * Read a covariance matrix from a fiff
 */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    FiffTag::SPtr t_pTag;
    QList<FiffDirNode::SPtr> nodes;
    FiffDirNode::SPtr    covnode;

    QStringList     names;	/* Optional channel name list */
    int             nnames     = 0;
    double          *cov       = NULL;
    double          *cov_diag  = NULL;
    FiffSparseMatrix* cov_sparse = NULL;
    double          *lambda    = NULL;
    float           **eigen    = NULL;
    MatrixXf        tmp_eigen;
    QStringList     bads;
    int             nbad       = 0;
    int             ncov       = 0;
    int             nfree      = 1;
    MneCovMatrix*    res        = NULL;

    int            k,p,nn;
    float          *f;
    double         *d;
    MneProjOp*      op = NULL;
    MneSssData*     sss = NULL;

    if(!stream->open())
        goto out;

    nodes = stream->dirtree()->dir_tree_find(FIFFB_MNE_COV);

    if (nodes.size() == 0) {
        printf("No covariance matrix available in %s",name.toUtf8().data());
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
        printf("Desired covariance matrix not found from %s",name.toUtf8().data());
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
        mne_string_to_name_list_3(t_pTag->toString(),names,nnames);
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
        if (mne_read_bad_channel_list_from_node_3(stream,nodes[k],bads,nbad) == FAIL)
            goto out;
    }
    if (cov_sparse)
        res = MneCovMatrix::mne_new_cov_sparse(kind,ncov,names,cov_sparse);
    else if (cov)
        res = MneCovMatrix::mne_new_cov_dense(kind,ncov,names,cov);
    else if (cov_diag)
        res = MneCovMatrix::mne_new_cov_diag(kind,ncov,names,cov_diag);
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
        if(op)
            delete op;
        if(sss)
            delete sss;

        if (!res) {
            names.clear();
            bads.clear();
            FREE_3(cov);
            FREE_3(cov_diag);
            if(cov_sparse)
                delete cov_sparse;
        }
        return res;
    }
}

//============================= mne_coord_transforms.c =============================

namespace MNELIB
{

typedef struct {
    int frame;
    const char *name;
} frameNameRec_3;

}

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

void mne_merge_channels(const QList<FiffChInfo>& chs1,
                        int nch1,
                        const QList<FiffChInfo>& chs2,
                        int nch2,
                        QList<FiffChInfo>& resp,
                        int *nresp)

{
    resp.clear();
    resp.reserve(nch1+nch2);

    int k;
    for (k = 0; k < nch1; k++) {
        resp.append(chs1.at(k));
    }
    for (k = 0; k < nch2; k++) {
        resp.append(chs2.at(k));
    }

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
    for (k = 0; k < tmp_node->nchild(); k++)
        if (tmp_node->children[k]->type == FIFFB_MEAS_INFO)
            return (tmp_node->children[k]);
    return empty_node;
}

static int get_all_chs (//fiffFile file,	        /* The file we are reading */
                        FiffStream::SPtr& stream,
                        const FiffDirNode::SPtr& p_node,	/* The directory node containing our data */
                        fiffId *id,		/* The block id from the nearest FIFFB_MEAS parent */
                        QList<FiffChInfo>& chp,	/* Channel descriptions */
                        int *nchanp)		/* Number of channels */
/*
      * Find channel information from
      * nearest FIFFB_MEAS_INFO parent of
      * node.
      */
{
    QList<FiffChInfo> ch;
    FiffChInfo this_ch;
    int j,k,nchan;
    int to_find = 0;
    FiffDirNode::SPtr meas;
    FiffDirNode::SPtr meas_info;
    fiff_int_t kind, pos;
    FiffTag::SPtr t_pTag;

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
    for (k = 0; k < meas_info->nent(); k++) {
        kind = meas_info->dir[k]->kind;
        pos  = meas_info->dir[k]->pos;
        switch (kind) {
        case FIFF_NCHAN :
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            nchan = *t_pTag->toInt();
            *nchanp = nchan;

            for (j = 0; j < nchan; j++) {
                ch.append(FiffChInfo());
                ch[j].scanNo = -1;
            }

            to_find += nchan - 1;
            break;

        case FIFF_CH_INFO : /* Information about one channel */
            if (!stream->read_tag(t_pTag,pos))
                goto bad;
            this_ch = t_pTag->toChInfo();
            if (this_ch.scanNo <= 0 || this_ch.scanNo > nchan) {
                qCritical ("FIFF_CH_INFO : scan # out of range!");
                goto bad;
            }
            else
                ch[this_ch.scanNo-1] = this_ch;
            to_find--;
            break;
        }
    }

    chp = ch;
    return FIFF_OK;

bad : {
        return FIFF_FAIL;
    }
}

static int read_ch_info(const QString&  name,
                        QList<FiffChInfo>& chsp,
                        int             *nchanp,
                        fiffId          *idp)
/*
 * Read channel information from a measurement file
 */
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    QList<FiffChInfo> chs;
    int        nchan = 0;
    fiffId     id = NULL;

    QList<FiffDirNode::SPtr> meas;
    FiffDirNode::SPtr    node;

    if(!stream->open())
        goto bad;

    meas = stream->dirtree()->dir_tree_find(FIFFB_MEAS);
    if (meas.size() == 0) {
        qCritical ("%s : no MEG data available here",name.toUtf8().data());
        goto bad;
    }
    node = meas[0];
    if (get_all_chs(stream,
                    node,
                    &id,
                    chs,
                    &nchan) == FIFF_FAIL)
        goto bad;
     chsp   = chs;
     *nchanp = nchan;
     *idp = id;
    stream->close();
    return FIFF_OK;

bad : {
        FREE_3(id);
        stream->close();
        return FIFF_FAIL;
    }
}

#define TOO_CLOSE 1e-4

static int at_origin (const Eigen::Vector3f& rr)
{
    return (rr.norm() < TOO_CLOSE);
}

static int is_valid_eeg_ch(const FiffChInfo& ch)
/*
      * Is the electrode position information present?
      */
{
    if (ch.kind == FIFFV_EEG_CH) {
        if (at_origin(ch.chpos.r0) ||
                ch.chpos.coil_type == FIFFV_COIL_NONE)
            return FALSE;
        else
            return TRUE;
    }
    return FALSE;
}

static int accept_ch(const FiffChInfo& ch,
                     const QStringList& bads,
                     int        nbad)

{
    int k;
    for (k = 0; k < nbad; k++)
        if (QString::compare(ch.ch_name,bads[k]) == 0)
            return FALSE;
    return TRUE;
}

int read_meg_eeg_ch_info(const QString& name,       /* Input file */
                         int        do_meg,         /* Use MEG */
                         int        do_eeg,         /* Use EEG */
                         const QStringList& bads,   /* List of bad channels */
                         int        nbad,
                         QList<FiffChInfo>& chsp,          /* MEG + EEG channels */
                         int        *nmegp,         /* Count of each */
                         int        *neegp)
/*
      * Read the channel information and split it into two arrays,
      * one for MEG and one for EEG
      */
{
    QList<FiffChInfo> chs;
    int        nchan = 0;
    QList<FiffChInfo> meg;
    int        nmeg  = 0;
    QList<FiffChInfo> eeg;
    int        neeg  = 0;
    fiffId     id    = NULL;
    int        nch;

    int k;

    if (read_ch_info(name,
                     chs,
                     &nchan,
                     &id) != FIFF_OK)
        goto bad;
    /*
   * Sort out the channels
   */
    for (k = 0; k < nchan; k++) {
        if (accept_ch(chs[k],bads,nbad)) {
            if (do_meg && chs[k].kind == FIFFV_MEG_CH) {
                meg.append(chs[k]);
                nmeg++;
            } else if (do_eeg && chs[k].kind == FIFFV_EEG_CH && is_valid_eeg_ch(chs[k])) {
                eeg.append(chs[k]);
                neeg++;
            }
        }
    }

    mne_merge_channels(meg,
                       nmeg,
                       eeg,
                       neeg,
                       chsp,
                       &nch);

    if(nmegp) {
        *nmegp = nmeg;
    }
    if(neegp) {
        *neegp = neeg;
    }
    FREE_3(id);
    return FIFF_OK;

bad : {
        FREE_3(id);
        return FIFF_FAIL;
    }
}

void mne_revert_to_diag_cov(MneCovMatrix* c)
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

MneCovMatrix* mne_pick_chs_cov_omit(MneCovMatrix* c,
                                    const QStringList& new_names,
                                    int ncov,
                                    int omit_meg_eeg,
                                    const QList<FiffChInfo>& chs)
/*
 * Pick designated channels from a covariance matrix, optionally omit MEG/EEG correlations
 */
{
    int j,k;
    int *pick = NULL;
    double *cov = NULL;
    double *cov_diag = NULL;
    QStringList names;
    int   *is_meg = NULL;
    int   from,to;
    MneCovMatrix* res;

    if (ncov == 0) {
        qCritical("No channels specified for picking in mne_pick_chs_cov_omit");
        return NULL;
    }
    if (c->names.isEmpty()) {
        qCritical("No names in covariance matrix. Cannot do picking.");
        return NULL;
    }
    pick = MALLOC_3(ncov,int);
    for (j = 0; j < ncov; j++)
        pick[j] = -1;
    for (j = 0; j < ncov; j++)
        for (k = 0; k < c->ncov; k++)
            if (QString::compare(c->names[k],new_names[j]) == 0) {
                pick[j] = k;
                break;
            }
    for (j = 0; j < ncov; j++) {
        if (pick[j] < 0) {
            printf("All desired channels not found in the covariance matrix (at least missing %s).", new_names[j].toUtf8().constData());
            FREE_3(pick);
            return NULL;
        }
    }
    if (omit_meg_eeg) {
        is_meg = MALLOC_3(ncov,int);
        if (!chs.isEmpty()) {
            for (j = 0; j < ncov; j++)
                if (chs[j].kind == FIFFV_MEG_CH)
                    is_meg[j] = TRUE;
                else
                    is_meg[j] = FALSE;
        }
        else {
            for (j = 0; j < ncov; j++)
                if (new_names[j].startsWith("MEG"))
                    is_meg[j] = TRUE;
                else
                    is_meg[j] = FALSE;
        }
    }
    if (c->cov_diag) {
        cov_diag = MALLOC_3(ncov,double);
        for (j = 0; j < ncov; j++) {
            cov_diag[j] = c->cov_diag[pick[j]];
            names.append(c->names[pick[j]]);
        }
    }
    else {
        cov = MALLOC_3(ncov*(ncov+1)/2,double);
        for (j = 0; j < ncov; j++) {
            names.append(c->names[pick[j]]);
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

    res = MneCovMatrix::mne_new_cov(c->kind,ncov,names,cov,cov_diag);

    res->bads = c->bads;
    res->nbad = c->nbad;
    res->proj = MneProjOp::mne_dup_proj_op(c->proj);
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

int mne_proj_op_proj_dvector(MneProjOp* op, double *vec, int nch, int do_complement)
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

int mne_name_list_match(const QStringList& list1, int nlist1,
                        const QStringList& list2, int nlist2)
/*
 * Check whether two name lists are identical
 */
{
    int k;
    if (list1.isEmpty() && list2.isEmpty())
        return 0;
    if (list1.isEmpty() || list2.isEmpty())
        return 1;
    if (nlist1 != nlist2)
        return 1;
    for (k = 0; k < nlist1; k++)
        if (QString::compare(list1[k],list2[k]) != 0)
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

int mne_proj_op_apply_cov(MneProjOp* op, MneCovMatrix*& c)
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

    c->nproj = MneProjOp::mne_proj_op_affect(op,c->names,c->ncov);
    return OK;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DipoleFitData::DipoleFitData()
: mri_head_t (NULL)
, meg_head_t (NULL)
, meg_coils (NULL)
, eeg_els (NULL)
, nmeg (0)
, neeg (0)
, ch_names (NULL)
, pick (NULL)
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

//=============================================================================================================

DipoleFitData::~DipoleFitData()
{
    if(mri_head_t)
        delete mri_head_t;
    if(meg_head_t)
        delete meg_head_t;

    if(meg_coils)
        delete meg_coils;
    if(eeg_els)
        delete eeg_els;

    bemname.clear();

    mne_free_cov_3(noise);
    mne_free_cov_3(noise_orig);
    ch_names.clear();

    if(pick)
        delete pick;
    if(bem_model)
        delete bem_model;

    if(eeg_model)
        delete eeg_model;
    if (user_free)
        user_free(user);

    if(proj)
        delete proj;

    free_dipole_fit_funcs(sphere_funcs);
    free_dipole_fit_funcs(bem_funcs);
    free_dipole_fit_funcs(mag_dipole_funcs);
}

//=============================================================================================================

int DipoleFitData::setup_forward_model(DipoleFitData *d, MneCTFCompDataSet* comp_data, FwdCoilSet *comp_coils)
/*
     * Take care of some hairy details
     */
{
    FwdCompData* comp;
    dipoleFitFuncs f;
    int fit_sphere_to_bem = TRUE;

    if (!d->bemname.isEmpty()) {
        /*
         * Set up the boundary-element model
         */
        QString bemsolname = FwdBemModel::fwd_bem_make_bem_sol_name(d->bemname);
        d->bemname = bemsolname;

        printf("\nSetting up the BEM model using %s...\n",d->bemname.toUtf8().constData());
        printf("\nLoading surfaces...\n");
        d->bem_model = FwdBemModel::fwd_bem_load_three_layer_surfaces(d->bemname);
        if (d->bem_model) {
            printf("Three-layer model surfaces loaded.\n");
        }
        else {
            d->bem_model = FwdBemModel::fwd_bem_load_homog_surface(d->bemname);
            if (!d->bem_model)
                goto out;
            printf("Homogeneous model surface loaded.\n");
        }
        if (d->neeg > 0 && d->bem_model->nsurf == 1) {
            qCritical("Cannot use a homogeneous model in EEG calculations.");
            goto out;
        }
        printf("\nLoading the solution matrix...\n");
        if (FwdBemModel::fwd_bem_load_recompute_solution(d->bemname,FWD_BEM_UNKNOWN,FALSE,d->bem_model) == FAIL)
            goto out;
        printf("Employing the head->MRI coordinate transform with the BEM model.\n");
        if (FwdBemModel::fwd_bem_set_head_mri_t(d->bem_model,d->mri_head_t) == FAIL)
            goto out;
        printf("BEM model %s is now set up\n",d->bem_model->sol_name.toUtf8().constData());
        /*
         * Find the best-fitting sphere
         */
        if (fit_sphere_to_bem) {
            MneSurfaceOld* inner_skull;
            float      simplex_size = 2e-2;
            float      R;

            if ((inner_skull = d->bem_model->fwd_bem_find_surface(FIFFV_BEM_SURF_ID_BRAIN)) == NULL)
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
            comp = FwdCompData::fwd_make_comp_data(comp_data,d->meg_coils,comp_coils,
                                      FwdBemModel::fwd_bem_field,NULL,NULL,d->bem_model,NULL);
            if (!comp)
                goto out;
            printf("Compensation setup done.\n");

            printf("MEG solution matrix...");
            if (FwdBemModel::fwd_bem_specify_coils(d->bem_model,d->meg_coils) == FAIL)
                goto out;
            if (FwdBemModel::fwd_bem_specify_coils(d->bem_model,comp->comp_coils) == FAIL)
                goto out;
            printf("[done]\n");

            f->meg_field       = FwdCompData::fwd_comp_field;
            f->meg_vec_field   = NULL;
            f->meg_client      = comp;
            f->meg_client_free = FwdCompData::fwd_free_comp_data;
        }
        if (d->neeg > 0) {
            printf("\tEEG solution matrix...");
            if (FwdBemModel::fwd_bem_specify_els(d->bem_model,d->eeg_els) == FAIL)
                goto out;
            printf("[done]\n");
            f->eeg_pot     = FwdBemModel::fwd_bem_pot_els;
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
        comp = FwdCompData::fwd_make_comp_data(comp_data,d->meg_coils,comp_coils,
                                  FwdBemModel::fwd_sphere_field,
                                  FwdBemModel::fwd_sphere_field_vec,
                                  NULL,
                                  d->r0,NULL);
        if (!comp)
            goto out;
        f->meg_field       = FwdCompData::fwd_comp_field;
        f->meg_vec_field   = FwdCompData::fwd_comp_field_vec;
        f->meg_client      = comp;
        f->meg_client_free = FwdCompData::fwd_free_comp_data;
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
        comp = FwdCompData::fwd_make_comp_data(comp_data,d->meg_coils,comp_coils,
                                  FwdBemModel::fwd_mag_dipole_field,
                                  FwdBemModel::fwd_mag_dipole_field_vec,
                                  NULL,
                                  NULL,NULL);
        if (!comp)
            goto out;
        f->meg_field       = FwdCompData::fwd_comp_field;
        f->meg_vec_field   = FwdCompData::fwd_comp_field_vec;
        f->meg_client      = comp;
        f->meg_client_free = FwdCompData::fwd_free_comp_data;
    }
    f->eeg_pot     = FwdBemModel::fwd_mag_dipole_field;
    f->eeg_vec_pot = FwdBemModel::fwd_mag_dipole_field_vec;
    /*
        * Select the appropriate fitting function
        */
    d->funcs = !d->bemname.isEmpty() ? d->bem_funcs : d->sphere_funcs;

    fprintf (stderr,"\n");
    return OK;

out :
    return FAIL;
}

//=============================================================================================================

MneCovMatrix* DipoleFitData::ad_hoc_noise(FwdCoilSet *meg, FwdCoilSet *eeg, float grad_std, float mag_std, float eeg_std)
/*
     * Specify constant noise values
     */
{
    int    nchan;
    double *stds;
    QStringList names, ch_names;
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
            ch_names.append(meg->coils[k]->chname);
        }
    }
    if (eeg) {
        for (k = 0; k < eeg->ncoil; k++, n++) {
            stds[n]     = eeg_std*eeg_std;
            ch_names.append(eeg->coils[k]->chname);
        }
    }
    names = ch_names;
    return MneCovMatrix::mne_new_cov(FIFFV_MNE_NOISE_COV,nchan,names,NULL,stds);
}

//=============================================================================================================

int DipoleFitData::make_projection(const QList<QString> &projnames,
                                   const QList<FiffChInfo>& chs,
                                   int nch,
                                   MneProjOp* *res)
/*
          * Process the projection data
          */
{
    MneProjOp* all  = NULL;
    MneProjOp* one  = NULL;
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
            printf("No linear projection information in %s.\n",projnames[k].toUtf8().data());
            if(one)
                delete one;
            one = NULL;
        }
        else {
            printf("Loaded projection from %s:\n",projnames[k].toUtf8().data());
            mne_proj_op_report_3(stderr,"\t",one);
            all = MneProjOp::mne_proj_op_combine(all,one);
            if(one)
                delete one;
            one = NULL;
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
            if ((one = MneProjOp::mne_proj_op_average_eeg_ref(chs,nch)) != NULL) {
                printf("Average EEG reference projection added:\n");
                mne_proj_op_report_3(stderr,"\t",one);
                all = MneProjOp::mne_proj_op_combine(all,one);
                if(one)
                    delete one;
                one = NULL;
            }
        }
    }
    if (all && MneProjOp::mne_proj_op_affect_chs(all,chs,nch) == 0) {
        printf("Projection will not have any effect on selected channels. Projection omitted.\n");
        if(all)
            delete all;
        all = NULL;
    }
     *res = all;
    return OK;

bad :
    if(all)
        delete all;
    all = NULL;
    return FAIL;
}

//=============================================================================================================

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

//=============================================================================================================

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

//=============================================================================================================

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
        f->noise = MneCovMatrix::mne_dup_cov(f->noise_orig);
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
        f->noise = MneCovMatrix::mne_dup_cov(f->noise_orig);
    }

    return scale_dipole_fit_noise_cov(f,nave);
}

//=============================================================================================================

DipoleFitData *DipoleFitData::setup_dipole_fit_data(const QString &mriname,
                                                    const QString &measname,
                                                    const QString& bemname,
                                                    Vector3f *r0,
                                                    FwdEegSphereModel *eeg_model,
                                                    int accurate_coils,
                                                    const QString &badname,
                                                    const QString &noisename,
                                                    float grad_std,
                                                    float mag_std,
                                                    float eeg_std,
                                                    float mag_reg,
                                                    float grad_reg,
                                                    float eeg_reg,
                                                    int diagnoise,
                                                    const QList<QString> &projnames,
                                                    int include_meg,
                                                    int include_eeg)              /**< Include EEG in the fitting?. */
/*
          * Background work for modelling
          */
{
    DipoleFitData*  res = new DipoleFitData;
    int             k;
    QStringList     badlist;
    int             nbad      = 0;
    QStringList     file_bads;
    int             file_nbad;
    int             coord_frame = FIFFV_COORD_HEAD;
    MneCovMatrix*   cov;
    FwdCoilSet*     templates = NULL;
    MneCTFCompDataSet* comp_data  = NULL;
    FwdCoilSet*        comp_coils = NULL;

    /*
       * Read the coordinate transformations
       */
    if (!mriname.isEmpty()) {
        if ((res->mri_head_t = FiffCoordTransOld::mne_read_mri_transform(mriname)) == NULL)
            goto bad;
    }
    else if (!bemname.isEmpty()) {
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
        if (mne_read_bad_channels_3(badname,badlist,nbad) != OK)
            goto bad;
        printf("%d bad channels read from %s.\n",nbad,badname.toUtf8().data());
    }
    if (mne_read_bad_channel_list_3(measname,file_bads,file_nbad) == OK && file_nbad > 0) {
        if (badlist.isEmpty())
            nbad = 0;
        for (k = 0; k < file_nbad; k++) {
            badlist.append(file_bads[k]);
            nbad++;
        }
        file_bads.clear();
        printf("%d bad channels read from the data file.\n",file_nbad);
    }
    printf("%d bad channels total.\n",nbad);
    /*
       * Read the channel information
       */
    if (read_meg_eeg_ch_info(measname,
                             include_meg,
                             include_eeg,
                             badlist,
                             nbad,
                             res->chs,
                             &res->nmeg,
                             &res->neeg) != OK)
        goto bad;

    if (res->nmeg > 0)
        printf("Will use %3d MEG channels from %s\n",res->nmeg,measname.toUtf8().data());
    if (res->neeg > 0)
        printf("Will use %3d EEG channels from %s\n",res->neeg,measname.toUtf8().data());
    {
        QString s = mne_channel_names_to_string_3(res->chs,
                                                  res->nmeg+res->neeg);
        int  n;
        mne_string_to_name_list_3(s,res->ch_names,n);
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

        QString qPath = QString(QCoreApplication::applicationDirPath() + "/resources/general/coilDefinitions/coil_def.dat");
        QFile file(qPath);
        if ( !QCoreApplication::startingUp() )
            qPath = QCoreApplication::applicationDirPath() + QString("/resources/general/coilDefinitions/coil_def.dat");
        else if (!file.exists())
            qPath = "./resources/general/coilDefinitions/coil_def.dat";

        char *coilfile = MALLOC_3(strlen(qPath.toUtf8().data())+1,char);
        strcpy(coilfile,qPath.toUtf8().data());
        //#endif

        if (!coilfile)
            goto bad;
        if ((templates = FwdCoilSet::read_coil_defs(coilfile)) == NULL) {
            FREE_3(coilfile);
            goto bad;
        }

        if ((res->meg_coils = templates->create_meg_coils(res->chs,
                                                          res->nmeg,
                                                          accurate_coils ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                                                          res->meg_head_t)) == NULL)
            goto bad;
        if ((res->eeg_els = FwdCoilSet::create_eeg_els(res->chs.mid(res->nmeg),
                                                       res->neeg,
                                                       NULL)) == NULL)
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
    res->bemname   = bemname;
    if (r0) {
        res->r0[0]     = (*r0)[0];
        res->r0[1]     = (*r0)[1];
        res->r0[2]     = (*r0)[2];
    }
    res->eeg_model = eeg_model;
    /*
       * Compensation data
       */
    if ((comp_data = MneCTFCompDataSet::mne_read_ctf_comp_data(measname)) == NULL)
        goto bad;
    if (comp_data->ncomp > 0) {	/* Compensation channel information may be needed */
        QList<FiffChInfo> comp_chs;
        int        ncomp    = 0;

        printf("%d compensation data sets in %s\n",comp_data->ncomp,measname.toUtf8().data());
        QList<FiffChInfo> temp;
        if (mne_read_meg_comp_eeg_ch_info_3(measname,
                                            temp,
                                            NULL,
                                            comp_chs,
                                            &ncomp,
                                            temp,
                                            NULL,
                                            NULL,
                                            NULL) == FAIL)
            goto bad;
        if (ncomp > 0) {
            if ((comp_coils = templates->create_meg_coils(comp_chs,
                                                          ncomp,
                                                          FWD_COIL_ACCURACY_NORMAL,
                                                          res->meg_head_t)) == NULL) {
                goto bad;
            }
            printf("%d compensation channels in %s\n",comp_coils->ncoil,measname.toUtf8().data());
        }
    }
    else {          /* Get rid of the empty data set */
        if(comp_data)
            delete comp_data;
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
    if (make_projection(projnames,
                        res->chs,
                        res->nmeg+res->neeg,
                        &res->proj) == FAIL)
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
               cov->cov_diag ? "diagonal" : "full", noisename.toUtf8().data());
    }
    else {
        if ((cov = ad_hoc_noise(res->meg_coils,res->eeg_els,grad_std,mag_std,eeg_std)) == NULL)
            goto bad;
    }
    res->noise = mne_pick_chs_cov_omit(cov,
                                       res->ch_names,
                                       res->nmeg+res->neeg,
                                       TRUE,
                                       res->chs);
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
        if (mne_classify_channels_cov(res->noise,
                                      res->chs,
                                      res->nmeg+res->neeg) == FAIL)
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

    badlist.clear();
    delete templates;
    delete comp_coils;
    if(comp_data)
        delete comp_data;
    return res;

bad : {
        badlist.clear();
        delete templates;
        delete comp_coils;
        if(comp_data)
            delete comp_data;
        if(res)
            delete res;
        return NULL;
    }
}

//=============================================================================================================

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

//=============================================================================================================

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

//=============================================================================================================

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

//=============================================================================================================
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

    fwd = fuser->fwd = DipoleFitData::dipole_forward_one(fit,rd,fuser->fwd);
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
        rtol = 2.0*std::fabs(y[ihi]-y[ilo])/(std::fabs(y[ihi])+std::fabs(y[ilo]));
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

//=============================================================================================================
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

    if (MneProjOp::mne_proj_op_proj_vector(fit->proj,B,nchan,TRUE) == FAIL)
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
            fit->funcs = !fit->bemname.isEmpty() ? fit->bem_funcs : fit->sphere_funcs;

        simplex = make_initial_dipole_simplex(rd_guess,size);
        for (p = 0; p < 4; p++)
            vals[p] = fit_eval(simplex[p],3,fit);
        if (simplex_minimize(simplex,           /* The initial simplex */
                             vals,              /* Function values at the vertices */
                             3,                 /* Number of variables */
                             ftol[k],           /* Relative convergence tolerance for the target function */
                             atol[k],           /* Absolute tolerance for the change in the parameters */
                             fit_eval,          /* The function to be evaluated */
                             fit,               /* Data to be passed to the above function in each evaluation */
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

//=============================================================================================================

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
        if (MneProjOp::mne_proj_op_proj_vector(d->proj,fwd[k],d->nmeg+d->neeg,TRUE) == FAIL)
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
