//=============================================================================================================
/**
* @file     main.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Example of dipole fit
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>


#include <fiff/fiff.h>
//#include <mne/mne.h>
#include <utils/sphere.h>

#include "fiff_file.h"





#include <stdio.h>
#include <string.h>



//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QCommandLineParser>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
//using namespace MNELIB;



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



//============================= dot.h =============================

#define X 0
#define Y 1
#define Z 2
/*
 * Dot product and length
 */
#define VEC_DOT(x,y) ((x)[X]*(y)[X] + (x)[Y]*(y)[Y] + (x)[Z]*(y)[Z])
#define VEC_LEN(x) sqrt(VEC_DOT(x,x))


/*
 * Others...
 */

#define VEC_DIFF(from,to,diff) {\
(diff)[X] = (to)[X] - (from)[X];\
(diff)[Y] = (to)[Y] - (from)[Y];\
(diff)[Z] = (to)[Z] - (from)[Z];\
}

#define VEC_COPY(to,from) {\
(to)[X] = (from)[X];\
(to)[Y] = (from)[Y];\
(to)[Z] = (from)[Z];\
}

#define CROSS_PRODUCT(x,y,xy) {\
(xy)[X] =   (x)[Y]*(y)[Z]-(y)[Y]*(x)[Z];\
(xy)[Y] = -((x)[X]*(y)[Z]-(y)[X]*(x)[Z]);\
(xy)[Z] =   (x)[X]*(y)[Y]-(y)[X]*(x)[Y];\
}







//============================= mne_allocs.h =============================

/*
 * Basics...
 */
#define MALLOC(x,t) (t *)malloc((x)*sizeof(t))
#define REALLOC(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))
#define FREE(x) if ((char *)(x) != NULL) free((char *)(x))
/*
 * Float, double, and int arrays
 */
#define ALLOC_FLOAT(x) MALLOC(x,float)
#define ALLOC_DOUBLE(x) MALLOC(x,double)
#define ALLOC_INT(x) MALLOC(x,int)
#define REALLOC_FLOAT(x,y) REALLOC(x,y,float)
#define REALLOC_DOUBLE(x,y) REALLOC(x,y,double)
#define REALLOC_INT(x,y) REALLOC(x,y,int)
/*
 * float matrices
 */
#define ALLOC_CMATRIX(x,y) mne_cmatrix((x),(y))
#define FREE_CMATRIX(m) mne_free_cmatrix((m))
#define CMATRIX ALLOC_CMATRIX
/*
 * double matrices
 */
#define ALLOC_DCMATRIX(x,y) mne_dmatrix((x),(y))
#define ALLOC_COMPLEX_DCMATRIX(x,y) mne_complex_dmatrix((x),(y))
#define FREE_DCMATRIX(m) mne_free_dcmatrix((m))
#define FREE_COMPLEX_DCMATRIX(m) mne_free_dcmatrix((m))

//============================= mne_allocs.c =============================

static void matrix_error(int kind, int nr, int nc)

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

float **mne_cmatrix(int nr,int nc)

{
  int i;
  float **m;
  float *whole;

  m = MALLOC(nr,float *);
  if (!m) matrix_error(1,nr,nc);
  whole = MALLOC(nr*nc,float);
  if (!whole) matrix_error(2,nr,nc);

  for(i=0;i<nr;i++)
    m[i] = whole + i*nc;
  return m;
}



double **mne_dmatrix(int nr, int nc)

{
  int i;
  double **m;
  double *whole;

  m = MALLOC(nr,double *);
  if (!m) matrix_error(1,nr,nc);
  whole = MALLOC(nr*nc,double);
  if (!whole) matrix_error(2,nr,nc);

  for(i=0;i<nr;i++)
    m[i] = whole + i*nc;
  return m;
}



void mne_free_cmatrix (float **m)
{
  if (m) {
    FREE(*m);
    FREE(m);
  }
}


void mne_free_dcmatrix (double **m)

{
  if (m) {
    FREE(*m);
    FREE(m);
  }
}

/*
 * float matrices
 */

#define FREE_CMATRIX(m) mne_free_cmatrix((m))


#include "fiff_types.h"
#include "mne_types.h"
#include "fit_types.h"
#include "analyze_types.h"





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






//============================= mne_matop.c =============================

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

float mne_dot_vectors (float *v1,
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




int mne_svd(float **mat,	/* The matrix */
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
      * mat = ALLOC_CMATRIX(m,n);
      * vv  = ALLOC_CMATRIX(MIN(m,n),n);
      * uu  = ALLOC_CMATRIX(MIN(m,n),m);
      * sing = MALLOC(MIN(m,n),float);
      *
      * mat is modified by this operation
      *
      * This simply allocates the workspace and calls the
      * LAPACK Fortran routine
      */

{
  int    lwork;
  float  *work;
  float  **uutemp = NULL;
  int    udim = MIN(m,n);
  int    info;
  const char   *jobu;
  const char   *jobvt;
  int    j,k;
  float  dum[1];
  float  *vvp,*uup;

  lwork = MAX(3*MIN(m,n)+MAX(m,n),5*MIN(m,n)-4) + 100*MAX(m,n);
  work  = (float *)MALLOC(lwork,float);
  /*
   * Do SVD
   */
  if (vv == NULL) {
    jobu = "N";
    vvp = dum;
  }
  else {
    jobu = "S";
    vvp  = vv[0];
  }
  if (uu == NULL) {
    jobvt = "N";
    uup   = dum;
  }
  else {
    jobvt = "S";
    uutemp = ALLOC_CMATRIX(m,udim);
    uup = uutemp[0];
  }

  qDebug() << "ToDo: sgesvd(jobu,jobvt,&n,&m,mat[0],&n,sing,vvp,&n,uup,&udim,work,&lwork,&info); MISSING!!!!";
//  sgesvd(jobu,jobvt,&n,&m,mat[0],&n,sing,
//     vvp,&n,uup,&udim,work,&lwork,&info);
  if (info == 0) {
    if (uu != NULL) {
      /*
       * Transpose U to get rid of the
       * LAPACK convention.
       */
      for (j = 0; j < udim; j++)
    for (k = 0; k < m; k++)
      uu[j][k] = uutemp[k][j];
    }
  }
  else
    printf("sgesvd returned error: %d",info);
  FREE(work);
  FREE_CMATRIX(uutemp);
  return info;
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



double **mne_dmatt_dmat_mult2 (double **m1,double **m2, int d1,int d2,int d3)
     /* Matrix multiplication
      * result(d1 x d3) = m1(d2 x d1)^T * m2(d2 x d3) */

{
  double **result = ALLOC_DCMATRIX(d1,d3);
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











//============================= fiff_type_spec.h =============================

/*
 * These return information about a fiff type.
 */

fiff_int_t fiff_type_base(fiff_int_t type)
{
  return type & FIFFTS_BASE_MASK;
}


fiff_int_t fiff_type_fundamental(fiff_int_t type)
{
  return type & FIFFTS_FS_MASK;
}


fiff_int_t fiff_type_matrix_coding(fiff_int_t type)
{
  return type & FIFFTS_MC_MASK;
}















//============================= fiff_combat.c =============================

static short swap_short (fiff_short_t source)

{
  unsigned char *csource = (unsigned char *)(&source);
  fiff_short_t result;
  unsigned char *cresult =  (unsigned char *)(&result);

  cresult[0] = csource[1];
  cresult[1] = csource[0];
  return (result);
}


static fiff_int_t swap_int (fiff_int_t source)

{
  unsigned char *csource =  (unsigned char *)(&source);
  fiff_int_t result;
  unsigned char *cresult =  (unsigned char *)(&result);

  cresult[0] = csource[3];
  cresult[1] = csource[2];
  cresult[2] = csource[1];
  cresult[3] = csource[0];
  return (result);
}

static fiff_long_t swap_long (fiff_long_t source)

{
  unsigned char *csource =  (unsigned char *)(&source);
  fiff_long_t    result;
  unsigned char *cresult =  (unsigned char *)(&result);

  cresult[0] = csource[7];
  cresult[1] = csource[6];
  cresult[2] = csource[5];
  cresult[3] = csource[4];
  cresult[4] = csource[3];
  cresult[5] = csource[2];
  cresult[6] = csource[1];
  cresult[7] = csource[0];
  return (result);
}

static void swap_longp (fiff_long_t *source)

{
  unsigned char *csource =  (unsigned char *)(source);
  unsigned char c;

  c = csource[0];
  csource[0] = csource[7];
  csource[7] = c;

  c = csource[1];
  csource[1] = csource[6];
  csource[6] = c;

  c = csource[2];
  csource[2] = csource[5];
  csource[5] = c;

  c = csource[3];
  csource[3] = csource[4];
  csource[4] = c;

  return;
}

static void swap_intp (fiff_int_t *source)

{
  unsigned char *csource =  (unsigned char *)(source);

  unsigned char c;

  c = csource[3];
  csource[3] = csource[0];
  csource[0] = c;
  c = csource[2];
  csource[2] = csource[1];
  csource[1] = c;

  return;
}


static void swap_floatp (float *source)

{
  unsigned char *csource =  (unsigned char *)(source);
  unsigned char c;

  c = csource[3];
  csource[3] = csource[0];
  csource[0] = c;
  c = csource[2];
  csource[2] = csource[1];
  csource[1] = c;

  return;
}

static void swap_doublep(double *source)

{
  unsigned char *csource =  (unsigned char *)(source);
  unsigned char c;

  c = csource[7];
  csource[7] = csource[0];
  csource[0] = c;

  c = csource[6];
  csource[6] = csource[1];
  csource[1] = c;

  c = csource[5];
  csource[5] = csource[2];
  csource[2] = c;

  c = csource[4];
  csource[4] = csource[3];
  csource[3] = c;

  return;
}


static void convert_ch_pos(fiffChPos pos)

{
  int k;
  pos->coil_type  = swap_int(pos->coil_type);
  for (k = 0; k < 3; k++) {
    swap_floatp(&pos->r0[k]);
    swap_floatp(&pos->ex[k]);
    swap_floatp(&pos->ey[k]);
    swap_floatp(&pos->ez[k]);
  }
  return;
}





/*! Machine dependent data type conversions (tag info only)
 */

void fiff_convert_tag_info(fiffTag tag)

{
  tag->kind = swap_int(tag->kind);
  tag->type = swap_int(tag->type);
  tag->size = swap_int(tag->size);
  tag->next = swap_int(tag->next);
  return;
}




static void convert_matrix_from_file_data(fiffTag tag)
/*
 * Assumes that the input is in the non-native byte order and needs to be swapped to the other one
 */
{
  int ndim;
  int k;
  int *dimp,*data,kind,np,nz;
  float *fdata;
  double *ddata;
  unsigned int tsize = tag->size;

  if (fiff_type_fundamental(tag->type) != FIFFTS_FS_MATRIX)
    return;
  if (tag->data == NULL)
    return;
  if (tsize < sizeof(fiff_int_t))
    return;

  dimp = ((fiff_int_t *)(((char *)tag->data)+tag->size-sizeof(fiff_int_t)));
  swap_intp(dimp);
  ndim = *dimp;
  if (fiff_type_matrix_coding(tag->type) == FIFFTS_MC_DENSE) {
    if (tsize < (ndim+1)*sizeof(fiff_int_t))
      return;
    dimp = dimp - ndim;
    for (k = 0, np = 1; k < ndim; k++) {
      swap_intp(dimp+k);
      np = np*dimp[k];
    }
  }
  else {
    if (tsize < (ndim+2)*sizeof(fiff_int_t))
      return;
    if (ndim > 2)		/* Not quite sure what to do */
      return;
    dimp = dimp - ndim - 1;
    for (k = 0; k < ndim+1; k++)
      swap_intp(dimp+k);
    nz = dimp[0];
    if (fiff_type_matrix_coding(tag->type) == FIFFTS_MC_CCS)
      np = nz + dimp[2] + 1; /* nz + n + 1 */
    else if (fiff_type_matrix_coding(tag->type) == FIFFTS_MC_RCS)
      np = nz + dimp[1] + 1; /* nz + m + 1 */
    else
      return;			/* Don't know what to do */
    /*
     * Take care of the indices
     */
    for (data = (int *)(tag->data)+nz, k = 0; k < np; k++)
      swap_intp(data+k);
    np = nz;
  }
  /*
   * Now convert data...
   */
  kind = fiff_type_base(tag->type);
  if (kind == FIFFT_INT) {
    for (data = (int *)(tag->data), k = 0; k < np; k++)
      swap_intp(data+k);
  }
  else if (kind == FIFFT_FLOAT) {
    for (fdata = (float *)(tag->data), k = 0; k < np; k++)
      swap_floatp(fdata+k);
  }
  else if (kind == FIFFT_DOUBLE) {
    for (ddata = (double *)(tag->data), k = 0; k < np; k++)
      swap_doublep(ddata+k);
  }
  return;
}


static void convert_matrix_to_file_data(fiffTag tag)
/*
 * Assumes that the input is in the NATIVE_ENDIAN byte order and needs to be swapped to the other one
 */
{
  int ndim;
  int k;
  int *dimp,*data,kind,np;
  float *fdata;
  double *ddata;
  unsigned int tsize = tag->size;

  if (fiff_type_fundamental(tag->type) != FIFFTS_FS_MATRIX)
    return;
  if (tag->data == NULL)
    return;
  if (tsize < sizeof(fiff_int_t))
    return;

  dimp = ((fiff_int_t *)(((char *)tag->data)+tag->size-sizeof(fiff_int_t)));
  ndim = *dimp;
  swap_intp(dimp);

  if (fiff_type_matrix_coding(tag->type) == FIFFTS_MC_DENSE) {
    if (tsize < (ndim+1)*sizeof(fiff_int_t))
      return;
    dimp = dimp - ndim;
    for (k = 0, np = 1; k < ndim; k++) {
      np = np*dimp[k];
      swap_intp(dimp+k);
    }
  }
  else {
    if (tsize < (ndim+2)*sizeof(fiff_int_t))
      return;
    if (ndim > 2)		/* Not quite sure what to do */
      return;
    dimp = dimp - ndim - 1;
    if (fiff_type_matrix_coding(tag->type) == FIFFTS_MC_CCS)
      np = dimp[0] + dimp[2] + 1; /* nz + n + 1 */
    else if (fiff_type_matrix_coding(tag->type) == FIFFTS_MC_RCS)
      np = dimp[0] + dimp[1] + 1; /* nz + m + 1 */
    else
      return;			/* Don't know what to do */
    for (k = 0; k < ndim+1; k++)
      swap_intp(dimp+k);
  }
  /*
   * Now convert data...
   */
  kind = fiff_type_base(tag->type);
  if (kind == FIFFT_INT) {
    for (data = (int *)(tag->data), k = 0; k < np; k++)
      swap_intp(data+k);
  }
  else if (kind == FIFFT_FLOAT) {
    for (fdata = (float *)(tag->data), k = 0; k < np; k++)
      swap_floatp(fdata+k);
  }
  else if (kind == FIFFT_DOUBLE) {
    for (ddata = (double *)(tag->data), k = 0; k < np; k++)
      swap_doublep(ddata+k);
  }
  else if (kind == FIFFT_COMPLEX_FLOAT) {
    for (fdata = (float *)(tag->data), k = 0; k < 2*np; k++)
      swap_floatp(fdata+k);
  }
  else if (kind == FIFFT_COMPLEX_DOUBLE) {
    for (ddata = (double *)(tag->data), k = 0; k < 2*np; k++)
      swap_doublep(ddata+k);
  }
  return;
}


/*
 * Data type conversions for the little endian systems.
 */

/*! Machine dependent data type conversions (tag info only)
 *
 * from_endian defines the byte order of the input
 * to_endian   defines the byte order of the output
 *
 * Either of these may be specified as FIFFV_LITTLE_ENDIAN, FIFFV_BIG_ENDIAN, or FIFFV_NATIVE_ENDIAN.
 * The last choice means that the native byte order value will be substituted here before proceeding
 */

void fiff_convert_tag_data(fiffTag tag, int from_endian, int to_endian)

{
  int            np;
  int            k,r,c;
  fiff_int_t     *ithis;
  fiff_short_t   *sthis;
  fiff_long_t    *lthis;
  float          *fthis;
  double         *dthis;
  fiffDirEntry   dethis;
  fiffId         idthis;
  fiffChInfo     chthis;
  fiffChPos      cpthis;
  fiffCoordTrans ctthis;
  fiffDigPoint   dpthis;
  fiffDataRef    drthis;

  if (tag->data == NULL || tag->size == 0)
    return;

  if (from_endian == FIFFV_NATIVE_ENDIAN)
    from_endian = NATIVE_ENDIAN;
  if (to_endian == FIFFV_NATIVE_ENDIAN)
    to_endian = NATIVE_ENDIAN;

  if (from_endian == to_endian)
    return;

  if (fiff_type_fundamental(tag->type) == FIFFTS_FS_MATRIX) {
    if (from_endian == NATIVE_ENDIAN)
      convert_matrix_to_file_data(tag);
    else
      convert_matrix_from_file_data(tag);
    return;
  }

  switch (tag->type) {

  case FIFFT_INT :
  case FIFFT_JULIAN :
  case FIFFT_UINT :
    np = tag->size/sizeof(fiff_int_t);
    for (ithis = (fiff_int_t *)tag->data, k = 0; k < np; k++, ithis++)
      swap_intp(ithis);
    break;

  case FIFFT_LONG :
  case FIFFT_ULONG :
    np = tag->size/sizeof(fiff_long_t);
    for (lthis = (fiff_long_t *)tag->data, k = 0; k < np; k++, lthis++)
      swap_longp(lthis);
    break;

  case FIFFT_SHORT :
  case FIFFT_DAU_PACK16 :
  case FIFFT_USHORT :
    np = tag->size/sizeof(fiff_short_t);
    for (sthis = (fiff_short_t *)tag->data, k = 0; k < np; k++, sthis++)
      *sthis = swap_short(*sthis);
    break;

  case FIFFT_FLOAT :
  case FIFFT_COMPLEX_FLOAT :
    np = tag->size/sizeof(fiff_float_t);
    for (fthis = (fiff_float_t *)tag->data, k = 0; k < np; k++, fthis++)
      swap_floatp(fthis);
    break;

  case FIFFT_DOUBLE :
  case FIFFT_COMPLEX_DOUBLE :
    np = tag->size/sizeof(fiff_double_t);
    for (dthis = (fiff_double_t *)tag->data, k = 0; k < np; k++, dthis++)
      swap_doublep(dthis);
    break;


  case FIFFT_OLD_PACK :
    fthis = (float *)tag->data;
    /*
     * Offset and scale...
     */
    swap_floatp(fthis+0);
    swap_floatp(fthis+1);
    sthis = (short *)(fthis+2);
    np = (tag->size - 2*sizeof(float))/sizeof(short);
    for (k = 0; k < np; k++,sthis++)
      *sthis = swap_short(*sthis);
    break;

  case FIFFT_DIR_ENTRY_STRUCT :
    np = tag->size/sizeof(fiffDirEntryRec);
    for (dethis = (fiffDirEntry)tag->data, k = 0; k < np; k++, dethis++) {
      dethis->kind = swap_int(dethis->kind);
      dethis->type = swap_int(dethis->type);
      dethis->size = swap_int(dethis->size);
      dethis->pos  = swap_int(dethis->pos);
    }
    break;

  case FIFFT_ID_STRUCT :
    np = tag->size/sizeof(fiffIdRec);
    for (idthis = (fiffId)tag->data, k = 0; k < np; k++, idthis++) {
      idthis->version = swap_int(idthis->version);
      idthis->machid[0] = swap_int(idthis->machid[0]);
      idthis->machid[1] = swap_int(idthis->machid[1]);
      idthis->time.secs  = swap_int(idthis->time.secs);
      idthis->time.usecs = swap_int(idthis->time.usecs);
    }
    break;

  case FIFFT_CH_INFO_STRUCT :
    np = tag->size/sizeof(fiffChInfoRec);
    for (chthis = (fiffChInfo)tag->data, k = 0; k < np; k++, chthis++) {
      chthis->scanNo    = swap_int(chthis->scanNo);
      chthis->logNo     = swap_int(chthis->logNo);
      chthis->kind      = swap_int(chthis->kind);
      swap_floatp(&chthis->range);
      swap_floatp(&chthis->cal);
      chthis->unit      = swap_int(chthis->unit);
      chthis->unit_mul  = swap_int(chthis->unit_mul);
      convert_ch_pos(&(chthis->chpos));
    }
    break;

  case FIFFT_CH_POS_STRUCT :
    np = tag->size/sizeof(fiffChPosRec);
    for (cpthis = (fiffChPos)tag->data, k = 0; k < np; k++, cpthis++)
      convert_ch_pos(cpthis);
    break;

  case FIFFT_DIG_POINT_STRUCT :
    np = tag->size/sizeof(fiffDigPointRec);
    for (dpthis = (fiffDigPoint)tag->data, k = 0; k < np; k++, dpthis++) {
      dpthis->kind = swap_int(dpthis->kind);
      dpthis->ident = swap_int(dpthis->ident);
      for (r = 0; r < 3; r++)
    swap_floatp(&dpthis->r[r]);
    }
    break;

  case FIFFT_COORD_TRANS_STRUCT :
    np = tag->size/sizeof(fiffCoordTransRec);
    for (ctthis = (fiffCoordTrans)tag->data, k = 0; k < np; k++, ctthis++) {
      ctthis->from = swap_int(ctthis->from);
      ctthis->to   = swap_int(ctthis->to);
      for (r = 0; r < 3; r++) {
    swap_floatp(&ctthis->move[r]);
    swap_floatp(&ctthis->invmove[r]);
    for (c = 0; c < 3; c++) {
      swap_floatp(&ctthis->rot[r][c]);
      swap_floatp(&ctthis->invrot[r][c]);
    }
      }
    }
    break;

  case FIFFT_DATA_REF_STRUCT :
    np = tag->size/sizeof(fiffDataRefRec);
    for (drthis = (fiffDataRef)tag->data, k = 0; k < np; k++, drthis++) {
      drthis->type   = swap_int(drthis->type);
      drthis->endian = swap_int(drthis->endian);
      drthis->size   = swap_long(drthis->size);
      drthis->offset = swap_long(drthis->offset);
    }
    break;

  default :
    break;
  }
  return;
}






//============================= fiff_ext_data.c =============================


void *fiff_ext_read_data(fiffFile    file,
             fiffDataRef ref)

{
  printf("external data : type = %d endian = %d size = %ld offset = %ld\n",
      ref->type,ref->endian,(long)ref->size,(long)ref->offset);
  qCritical("Cannot read external data yet");
  return NULL;
}



//============================= fiff_io.c =============================


typedef struct {		/* One channel is described here */
  fiff_int_t scanNo;		/* Scanning order # */
  fiff_int_t logNo;		/* Logical channel # */
  fiff_int_t kind;		/* Kind of channel:
                 * 1 = magnetic
                 * 2 = electric
                 * 3 = stimulus */
  fiff_float_t range;		/* Voltmeter range (-1 = auto ranging) */
  fiff_float_t cal;		/* Calibration from volts to... */
  fiff_float_t loc[9];		/* Location for a magnetic channel */
} oldChInfoRec,*oldChInfo;


static void convert_loc (float oldloc[9], /*!< These are the old magic numbers */
                         float r0[3],     /*!< Coil coordinate system origin */
                         float *ex,       /*!< Coil coordinate system unit x-vector */
                         float *ey,       /*!< Coil coordinate system unit y-vector */
                         float *ez)       /*!< Coil coordinate system unit z-vector */
     /*
      * Convert the traditional location
      * information to new format...
      */
{
  float len;
  int j;
  VEC_DIFF(oldloc+3,oldloc,ex);	/* From - coil to + coil */
  len = VEC_LEN(ex);
  for (j = 0; j < 3; j++) {
    ex[j] = ex[j]/len;		/* Normalize ex */
    ez[j] = oldloc[j+6];	/* ez along coil normal */
  }
  CROSS_PRODUCT(ez,ex,ey);	/* ey is defined by the other two */
  len = VEC_LEN(ey);
  for (j = 0; j < 3; j++) {
    ey[j] = ey[j]/len;		/* Normalize ey */
    r0[j] = (oldloc[j] + oldloc[j+3])/2.0;
                /* Origin lies halfway between the coils */
  }
  return;
}


static void fix_ch_info (fiffTag tag)
     /*
      * Fiddle around a little bit...
      */
{
  fiff_ch_info_t *ch;
  fiff_byte_t *help;
  oldChInfo old;
  fiff_ch_pos_t  *pos;

  if (tag->type == FIFFT_CH_INFO_STRUCT) {
    if (tag->size < (int)sizeof(fiff_ch_info_t)) { /* Old structure */
      help = (fiff_byte_t *)malloc(sizeof(fiff_ch_info_t));
      old = (oldChInfo)tag->data;
      tag->data = (fiff_data_t*) help;
      ch = (fiff_ch_info_t *)(tag->data);
      pos = &(ch->chpos);
      /*
       * Set up the new structure
       */
      ch->scanNo = old->scanNo;
      ch->logNo  = old->logNo;
      ch->kind   = old->kind;
      ch->range  = old->range;
      ch->cal    = old->cal;
      if (ch->kind == FIFFV_MAGN_CH) {
    pos->coil_type = FIFFV_COIL_NM_122;
    convert_loc (old->loc,pos->r0,pos->ex,pos->ey,pos->ez);
    sprintf(ch->ch_name,"MEG %03d",ch->logNo % 1000);
    ch->unit = FIFF_UNIT_T_M;
      }
      else if (ch->kind == FIFFV_EL_CH) {
    pos->coil_type = FIFFV_COIL_EEG;
    pos->r0[X] = old->loc[X];
    pos->r0[Y] = old->loc[Y];
    pos->r0[Z] = old->loc[Z];
    sprintf(ch->ch_name,"EEG %03d",ch->logNo);
    ch->unit = FIFF_UNIT_V;
      }
      else {
    pos->coil_type = FIFFV_COIL_NONE;
    sprintf(ch->ch_name,"STI %03d",ch->logNo);
    ch->unit = FIFF_UNIT_V;
      }
      FREE(old);
      ch->unit_mul = FIFF_UNITM_NONE;
    }
  }

}

int fiff_read_tag_info (FILE *in,fiffTag tag)
     /*
      * Read next tag from file
      * Do not read data
      */
{
  long pos = ftell(in);

  tag->data = NULL;
  if (fread (tag,FIFFC_TAG_INFO_SIZE,1,in) != 1)
    return (-1);
  fiff_convert_tag_info(tag);
  if (tag->next > 0) {
    if (fseek(in,tag->next,SEEK_SET) == -1) {
      qCritical ("fseek");
      pos = -1;
    }
  }
  else if (tag->size > 0 && tag->next == 0)
    if (fseek(in,tag->size,SEEK_CUR) == -1) {
      qCritical ("fseek");
      pos = -1;
    }
  return (pos);
}

int fiff_read_tag (FILE  *in,
           fiffTag tag)	/* Note: data member must be initialized
                 * to NULL on first call.
                 * This routine automatically reallocs
                 * the needed space */
     /*
      * Read next tag including its data from file
      */
{
  long pos = ftell(in);

  if (fread (tag,FIFFC_TAG_INFO_SIZE,1,in) != 1) {
    qCritical("Failed to read tag info (pos = %d)",pos);
    return (-1);
  }
  fiff_convert_tag_info(tag);
  if (tag->size > 0) {
    if (tag->data == NULL)
      tag->data = (fiff_data_t*)malloc(tag->size + ((tag->type == FIFFT_STRING) ? 1 : 0));
    else
      tag->data = (fiff_data_t*)realloc(tag->data,tag->size +
              ((tag->type == FIFFT_STRING) ? 1 : 0));
    if (tag->data == NULL) {
      qCritical("fiff_read_tag: memory allocation failed.");
      return -1;
    }
    if (fread (tag->data,tag->size,1,in) != 1) {
      qCritical("Failed to read tag data (pos = %d kind = %d size = %d)",pos,tag->kind,tag->size);
      return(-1);
    }
    if (tag->type == FIFFT_STRING)  /* Null-terminated strings */
      ((char *)tag->data)[tag->size] = '\0';
    else if (tag->type == FIFFT_CH_INFO_STRUCT)
      fix_ch_info (tag);
  }
  if (tag->next > 0)
    if (fseek(in,tag->next,SEEK_SET) == -1) {
      qCritical ("fseek");
      pos = -1;
    }
  fiff_convert_tag_data(tag,FIFFV_BIG_ENDIAN,FIFFV_NATIVE_ENDIAN);
  return (pos);
}




int fiff_read_tag_ext (fiffFile file,
               fiffTag  tag)	/* Note: data member must be initialized
                     * to NULL on first call.
                     * This routine automatically reallocs
                     * the needed space */
     /*
      * Read next tag including its data from file
      */
{
  long pos = ftell(file->fd);

  if (fread (tag,FIFFC_TAG_INFO_SIZE,1,file->fd) != 1) {
    printf("Failed to read tag info (pos = %d)",pos);
    return (-1);
  }
  fiff_convert_tag_info(tag);
  if (tag->size > 0) {
    if (tag->data == NULL)
      tag->data = (fiff_data_t *)malloc(tag->size + ((tag->type == FIFFT_STRING) ? 1 : 0));
    else
      tag->data = (fiff_data_t *)realloc(tag->data,tag->size + ((tag->type == FIFFT_STRING) ? 1 : 0));
    if (tag->data == NULL) {
      qCritical("fiff_read_tag: memory allocation failed.");
      return -1;
    }
    if (fread (tag->data,tag->size,1,file->fd) != 1) {
      printf("Failed to read tag data (pos = %d kind = %d size = %d)",pos,tag->kind,tag->size);
      return(-1);
    }
    if (tag->type == FIFFT_STRING)  /* Null-terminated strings */
      ((char *)tag->data)[tag->size] = '\0';
    else if (tag->type == FIFFT_CH_INFO_STRUCT)
      fix_ch_info (tag);
    fiff_convert_tag_data(tag,FIFFV_BIG_ENDIAN,FIFFV_NATIVE_ENDIAN);
  }
  if (tag->next > 0)
    if (fseek(file->fd,tag->next,SEEK_SET) == -1) {
      printf("fseek");
      pos = -1;
    }
  /*
   * Does the data actually refer to an external file
   */
  if (tag->type == FIFFT_DATA_REF_STRUCT) {
    fiffDataRef ref = (fiffDataRef)tag->data;
    tag->data = (fiff_data_t *)fiff_ext_read_data(file,ref);
    FREE(ref);
    if (!tag->data)
      pos = -1;
  }
  return (pos);
}




int fiff_read_this_tag (FILE *in,		/* Read from here */
            long pos,		/* File position from beginning */
            fiffTag tag)		/* Result goes here */
     /*
      * Read tag from specified position
      */
{
  if (fseek(in,pos,SEEK_SET) == -1) {
    qCritical ("fseek");
    return (-1);
  }
  else
    return (fiff_read_tag(in,tag));
}

int fiff_read_this_tag_ext (fiffFile file,	/* Read from here */
                long     pos,	/* File position from beginning */
                fiffTag  tag)	/* Result goes here */
/*
 * Read tag from specified position
 */
{
  if (fseek(file->fd,pos,SEEK_SET) == -1) {
    qCritical ("fseek");
    return (-1);
  }
  else
    return (fiff_read_tag_ext(file,tag));
}






#include "fiff_explain.h"


//============================= fiff_explain.c =============================


void fiff_explain (int kind)
     /*
      * Try to explain...
      *
      */
{
  int k;
  for (k = 0; _fiff_explanations[k].kind >= 0; k++) {
    if (_fiff_explanations[k].kind == kind) {
      printf ("%d = %s",kind,_fiff_explanations[k].text);
      return;
    }
  }
  printf ("Cannot explain: %d",kind);
}


const char *fiff_get_tag_explanation (int kind)
     /*
      * Get textual explanation of a tag
      */
{
  int k;
  for (k = 0; _fiff_explanations[k].kind >= 0; k++) {
    if (_fiff_explanations[k].kind == kind)
      return _fiff_explanations[k].text;
  }
  return "unknown";
}


void fiff_explain_block (int kind)
     /*
      * Try to explain a block...
      */
{
  int k;
  for (k = 0; _fiff_block_explanations[k].kind >= 0; k++) {
    if (_fiff_block_explanations[k].kind == kind) {
      printf ("%d = %s",kind,_fiff_block_explanations[k].text);
      return;
    }
  }
  printf ("Cannot explain: %d",kind);
}









//============================= fiff_dir_tree.c =============================

/*
 * Take care of directory trees
 */

void fiff_dir_tree_free(fiffDirNode node)

{
  int k;
  if (node == NULL)
    return;
  FREE(node->dir);
  FREE(node->id);
  for (k = 0; k < node->nchild; k++)
    fiff_dir_tree_free(node->children[k]);
  FREE(node);
}





static fiffDirNode make_subtree(fiffFile file,fiffDirEntry dentry)

{
  fiffDirNode node = (fiffDirNode)malloc(sizeof(fiffDirNodeRec));
  fiffDirNode child;
  fiffTagRec tag;
  int        k;
  int        level;
  fiffDirEntry dir;
  int          nent;

  dir  = node->dir  = MALLOC(file->nent,fiffDirEntryRec);
  nent = node->nent = 0;
  node->dir_tree    = dentry;
  node->nent_tree   = 1;
  node->parent      = NULL;
  node->children    = NULL;
  node->nchild      = 0;
  node->id          = NULL;
  tag.data = NULL;

  node->type = FIFFB_ROOT;
  if (dentry->kind == FIFF_BLOCK_START) {
    if (fiff_read_this_tag (file->fd,dentry->pos,&tag) == -1)
      goto bad;
    else
      node->type = *(int *)tag.data;
  }
  else {
    node->id   = (fiffId)malloc(sizeof(fiffIdRec));
    memcpy(node->id,file->id,sizeof(fiffIdRec));
  }
  dentry++;

  for (level = 0,k = dentry-file->dir; k < file->nent; k++,dentry++) {
    node->nent_tree = node->nent_tree + 1;
    if (dentry->kind == FIFF_BLOCK_START) {
      level++;
      if (level == 1) {
    if ((child = make_subtree(file,dentry)) == NULL)
      goto bad;
    child->parent = node;
    node->children = REALLOC(node->children,node->nchild+1,fiffDirNode);
    node->children[node->nchild++] = child;
      }
    }
    else if (dentry->kind == FIFF_BLOCK_END) {
      level--;
      if (level < 0)
    break;
    }
    else if (dentry->kind == -1)
      break;
    else if (level == 0) {
      /*
       * Take the node id from the parent block id,
       * block id, or file id. Let the block id
       * take precedence over parent block id and file id
       */
      if (((dentry->kind == FIFF_PARENT_BLOCK_ID || dentry->kind == FIFF_FILE_ID)
       && node->id == NULL) ||
      dentry->kind == FIFF_BLOCK_ID) {
    FREE(node->id);
    node->id = NULL;
    if (fiff_read_this_tag (file->fd,dentry->pos,&tag) == -1)
      goto bad;
    node->id = (fiffId)tag.data;
    tag.data = NULL;
      }
      memcpy(dir+nent,dentry,sizeof(fiffDirEntryRec));
      nent++;
    }
  }
  /*
   * Strip unused entries
   */
  node->nent = nent;
  node->dir = REALLOC(node->dir,node->nent,fiffDirEntryRec);
  FREE(tag.data);
  return (node);

  bad :
    fiff_dir_tree_free(node);
  return (NULL);
}


int fiff_dir_tree_create(fiffFile file)
     /*
      * Make a directory tree
      */
{
  fiff_dir_tree_free(file->dirtree);
  file->dirtree = NULL;
  if (file->fd == NULL)
    return (FIFF_FAIL);
  if ((file->dirtree = make_subtree(file,file->dir)) == NULL)
    return (FIFF_FAIL);
  else {
    file->dirtree->parent = NULL;
    return (FIFF_OK);
  }
}





static void print_id (fiffId id)

{
  printf ("\t%d.%d ",id->version>>16,id->version & 0xFFFF);
  printf ("0x%x%x ",id->machid[0],id->machid[1]);
  printf ("%d %d ",id->time.secs,id->time.usecs);
}


static void print_tree(fiffDirNode node,int indent)

{
  int j,k;
  int prev_kind,count;
  fiffDirEntry dentry;

  if (node == NULL)
    return;
  for (k = 0; k < indent; k++)
    putchar(' ');
  fiff_explain_block (node->type);
  printf (" { ");
  if (node->id != NULL)
    print_id(node->id);
  printf ("\n");

  for (j = 0, prev_kind = -1, count = 0, dentry = node->dir;
       j < node->nent; j++,dentry++) {
    if (dentry->kind != prev_kind) {
      if (count > 1)
    printf (" [%d]\n",count);
      else if (j > 0)
    putchar('\n');
      for (k = 0; k < indent+2; k++)
    putchar(' ');
      fiff_explain (dentry->kind);
      prev_kind = dentry->kind;
      count = 1;
    }
    else
      count++;
    prev_kind = dentry->kind;
  }
  if (count > 1)
    printf (" [%d]\n",count);
  else if (j > 0)
    putchar ('\n');
  for (j = 0; j < node->nchild; j++)
    print_tree(node->children[j],indent+5);
  for (k = 0; k < indent; k++)
    putchar(' ');
  printf ("}\n");
}

void fiff_dir_tree_print(fiffDirNode tree)
     /*
      * Print contents of the directory tree for
      * checking
      */
{
  print_tree(tree,0);
}

int fiff_dir_tree_count(fiffDirNode tree)
     /*
      * Find the number of nodes
      */
{
  int res,k;
  if (tree == NULL)
    res = 0;
  else {
    res = 1;
    for (k = 0; k < tree->nchild; k++)
      res = res + fiff_dir_tree_count(tree->children[k]);
  }
  return (res);
}












static fiffDirNode *found = NULL;
static int count = 0;

static void find_nodes (fiffDirNode tree,int kind)

{
  int k;
  if (tree->type == kind) {
    found[count++] = tree;
    found[count] = NULL;
  }
  for (k = 0; k < tree->nchild; k++)
    find_nodes(tree->children[k],kind);
  return;
}

fiffDirNode *fiff_dir_tree_find(fiffDirNode tree,
                int kind)

{
  /*
   * Make room for all nodes
   */
  found = MALLOC(fiff_dir_tree_count(tree)+1,fiffDirNode);
  count = 0;
  found[count] = NULL;
  find_nodes(tree,kind);
  /*
   * Shrink the size
   */
  found = REALLOC(found,count+1,fiffDirNode);
  return (found);
}



fiffTag fiff_dir_tree_get_tag(fiffFile file,fiffDirNode node,int kind)
     /*
      * Scan a dir node for a tag and read it
      */
{
  fiffTag tag;
  int k;
  fiffDirEntry dir;

  for (k = 0, dir = node->dir; k < node->nent; k++,dir++)
    if (dir->kind == kind) {
      tag = MALLOC(1,fiffTagRec);
      tag->data = NULL;
      if (fiff_read_this_tag_ext(file,dir->pos,tag) == -1) {
    FREE(tag->data);
    FREE(tag);
    return (NULL);
      }
      else
    return (tag);
    }
  printf("Desired tag (%s [%d]) not found",
            fiff_get_tag_explanation(kind),kind);
  return (NULL);
}








//============================= fiff_dir.c =============================

#define ALLOC_SIZE 100		/* Allocate this many entries at once */


fiffDirEntry fiff_make_dir (FILE *fd)
     /*
      * Scan the tag list to create a directory
      */
{
  fiffTagRec tag;
  fiffDirEntry dir = NULL;
  int nent = 0;
  int still_free= 0;
  fiff_int_t pos;
  /*
   * Start from the very beginning...
   */
  if (fseek(fd,0L,SEEK_SET) == -1)
    return (NULL);
  tag.data = NULL;
  while ((pos = fiff_read_tag_info(fd,&tag)) != -1) {
    /*
     * Check that we haven't run into the directory
     */
    if (tag.kind == FIFF_DIR)
      break;
    /*
     * Alloc in chunks for better performance...
     */
    if (still_free < 1) {
      dir = REALLOC(dir,nent+ALLOC_SIZE,fiffDirEntryRec);
      still_free = ALLOC_SIZE;
    }
    /*
     * Put in the new entry
     */
    dir[nent].kind = tag.kind;
    dir[nent].type = tag.type;
    dir[nent].size = tag.size;
    dir[nent].pos = pos;
    nent++; still_free--;
    if (tag.next < 0)
      break;
  }
  if (ferror(fd)) {
    FREE(dir);
    return (NULL);
  }
  else {
    /*
     * Put in the new the terminating entry
     */
    if (still_free < 1) {
      dir = REALLOC(dir,nent+ALLOC_SIZE,fiffDirEntryRec);
      still_free = ALLOC_SIZE;
    }
    dir[nent].kind = -1;
    dir[nent].type = -1;
    dir[nent].size = -1;
    dir[nent].pos  = -1;
    nent++; still_free--;
    /*
     * Possibly shrink a little bit
     */
    if (still_free > 0)
      dir = REALLOC(dir,nent,fiffDirEntryRec);
    return (dir);
  }
}


int fiff_how_many_entries (fiffDirEntry dir)
     /*
      * Count directory entries
      */
{
  int nent = 0;
  if (dir != NULL)
    for (nent = 1; dir->kind != -1; nent++,dir++)
      ;
  return (nent);
}




//============================= fiff_open.c =============================


/*! Check that the file starts properly!
 */

static int check_beginning (FILE *in, const char *name)

{
  fiffTagRec tag;
  if (fread (&tag,FIFFC_TAG_INFO_SIZE,1,in) != 1)
    return -1;
  fiff_convert_tag_info(&tag);
  if (tag.kind != FIFF_FILE_ID ||
      tag.type != FIFFT_ID_STRUCT ||
      tag.size != sizeof(fiff_id_t)) {
    printf("File %s does not start properly!",name);
    return -1;
  }
  rewind(in);
  return 0;
}

/*! Close file, free structures
 *
 * \param file File to be closed
 */

void fiff_close (fiffFile file)

{
  if (file == NULL)
    return;
  if (file->fd != NULL)
    (void)fclose(file->fd);
  FREE(file->file_name);
  FREE(file->dir);
  FREE(file->id);
  FREE(file->ext_file_name);
  if (file->ext_fd)
    (void)fclose(file->ext_fd);
  /*
   * Destroy the directory tree
   */
  fiff_dir_tree_free(file->dirtree);
  FREE(file);
}


/*
 * This is the same for both update open
 * and read-only open
 */

static fiffFile open_file (const char *name, const char *mode)

{
  void fiff_close();
  fiffFile file = MALLOC(1,fiffFileRec);
  fiffTagRec tag;
  long dirpos;
  /*
   * Clean fiff file descriptor
   */
  file->fd = NULL;
  file->file_name = NULL;
  file->id = NULL;
  file->dir = NULL;
  file->nent = 0;
  file->dirtree = NULL;
  file->ext_file_name = NULL;
  file->ext_fd        = NULL;
  /*
   * Try to open...
   */
  if ((file->fd = fopen(name,mode)) == NULL) {
    qCritical((char *)name);
    fiff_close(file);
    return (NULL);
  }
  file->file_name = MALLOC(strlen(name)+1,char);
  strcpy(file->file_name,name);
  tag.data = NULL;

  if (check_beginning(file->fd,name) == -1)
    goto bad;
  /*
   * Read id and directory pointer
   */
  if (fiff_read_tag(file->fd,&tag) == -1)
    goto bad;
  if (tag.kind != FIFF_FILE_ID) {
    qCritical("FIFF file should start with FIFF_FILE_ID!");
    goto bad;
  }
  file->id = (fiffId)tag.data;
  tag.data = NULL;
  if (fiff_read_tag(file->fd,&tag) == -1)
    goto bad;
  if (tag.kind != FIFF_DIR_POINTER) {
    qCritical("FIFF_DIR_POINTER should follow FIFF_FILE_ID!");
    goto bad;
  }
  /*
   * Do we have a directory or not?
   */
  dirpos = *(fiff_int_t *)(tag.data);
  FREE(tag.data); tag.data = NULL;
  if (dirpos <= 0) {		/* Must do it in the hard way... */
    if ((file->dir = fiff_make_dir (file->fd)) == NULL) {
      qCritical ("Could not create tag directory!");
      goto bad;
    }
  }
  else {			/* Just read the directory */
    if (fiff_read_this_tag(file->fd,dirpos,&tag) == -1) {
      qCritical ("Could not read the tag directory (file probably damaged)!");
      goto bad;
    }
    file->dir = (fiffDirEntry)tag.data;
  }
  file->nent = fiff_how_many_entries(file->dir);
  /*
   * Check for a mistake
   */
  if (file->dir[file->nent-2].kind == FIFF_DIR) {
    file->nent--;
    file->dir[file->nent-1].kind = -1;
    file->dir[file->nent-1].type = -1;
    file->dir[file->nent-1].size = -1;
    file->dir[file->nent-1].pos  = -1;
  }
  if (fiff_dir_tree_create(file) == -1)
    goto bad;
  (void)fseek(file->fd,0L,SEEK_SET);
  return (file);

  bad : {
    fiff_close(file);
    return(NULL);
  }
}


/*! Open fiff file for reading
 *
 * \param name File to open
 * \return Function returns a fiffFile object representing the
 * open file.
 */

fiffFile fiff_open (const char *name)
{
  fiffFile res = open_file(name,"rb");

  return (res);
}


//============================= mne_decompose.c =============================


int mne_decompose_eigen (double *mat,
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
  double *w    = MALLOC(dim,double);
  double *z    = MALLOC(dim*dim,double);
  double *work = MALLOC(3*dim,double);
  double *dmat = MALLOC(np,double);
  float  *vecp = vectors[0];

  const char   *uplo  = "U";
  const char   *compz = "V";
  int    info,k;
  int    one = 1;
  int    maxi;
  double scale;

  maxi = 0;//idamax(&np,mat,&one);
  qDebug() << "ToDo: idamax(&np,mat,&one);";
  scale = 1.0/mat[maxi-1];

  for (k = 0; k < np; k++)
    dmat[k] = mat[k]*scale;
//  dspev(compz,uplo,&dim,dmat,w,z,&dim,work,&info);
  qDebug() << "ToDo: dspev(compz,uplo,&dim,dmat,w,z,&dim,work,&info);";
  FREE(work);
  if (info != 0)
    printf("Eigenvalue decomposition failed (LAPACK info = %d)",info);
  else {
    scale = 1.0/scale;
    for (k = 0; k < dim; k++)
      lambda[k] = scale*w[k];
    for (k = 0; k < dim*dim; k++)
      vecp[k] = z[k];
  }
  FREE(w);
  FREE(z);
  if (info == 0)
    return 0;
  else
    return -1;
}



















//============================= mne_read_forward_solution.c =============================




int mne_read_meg_comp_eeg_ch_info(char           *name,
                  fiffChInfo     *megp,	 /* MEG channels */
                  int            *nmegp,
                  fiffChInfo     *meg_compp,
                  int            *nmeg_compp,
                  fiffChInfo     *eegp,	 /* EEG channels */
                  int            *neegp,
                  fiffCoordTrans *meg_head_t,
                  fiffId         *idp)	 /* The measurement ID */
     /*
      * Read the channel information and split it into three arrays,
      * one for MEG, one for MEG compensation channels, and one for EEG
      */
{
  fiffChInfo chs   = NULL;
  int        nchan = 0;
  fiffChInfo meg   = NULL;
  int        nmeg  = 0;
  fiffChInfo meg_comp = NULL;
  int        nmeg_comp = 0;
  fiffChInfo eeg   = NULL;
  int        neeg  = 0;
  fiffId     id    = NULL;
  fiffDirNode *nodes = NULL;
  fiffDirNode info = NULL;
  fiffDirEntry this_ent;
  fiffTagRec   tag;
  fiffChInfo   this_ch;
  fiffFile     in = NULL;
  fiffCoordTrans t = NULL;
  int j,k,to_find;
  extern fiffCoordTrans mne_read_meas_transform(char *name);

  tag.data = NULL;

  if ((in = fiff_open(name)) == NULL)
    goto bad;

  nodes = fiff_dir_tree_find(in->dirtree,FIFFB_MNE_PARENT_MEAS_FILE);
  if (nodes[0] == NULL) {
    FREE(nodes);
    nodes = fiff_dir_tree_find(in->dirtree,FIFFB_MEAS_INFO);
    if (nodes[0] == NULL) {
      qCritical ("Could not find the channel information.");
      goto bad;
    }
  }
  info = nodes[0]; FREE(nodes);
  to_find = 0;
  for (k = 0,this_ent = info->dir; k < info->nent; k++,this_ent++) {
    switch (this_ent->kind) {

    case FIFF_NCHAN :
      if (fiff_read_this_tag (in->fd,this_ent->pos,&tag) == FIFF_FAIL)
    goto bad;
      nchan = *(int *)(tag.data);
      chs = MALLOC(nchan,fiffChInfoRec);
      for (j = 0; j < nchan; j++)
    chs[j].scanNo = -1;
      to_find = nchan;
      break;

    case FIFF_PARENT_BLOCK_ID :
      if (fiff_read_this_tag (in->fd,this_ent->pos,&tag) == FIFF_FAIL)
    goto bad;
      id = MALLOC(1,fiffIdRec);
      *id = *(fiffId)tag.data;
      break;

    case FIFF_COORD_TRANS :
      if (fiff_read_this_tag (in->fd,this_ent->pos,&tag) == FIFF_FAIL)
    goto bad;
      t = (fiffCoordTrans)tag.data;
      if (t->from != FIFFV_COORD_DEVICE ||
      t->to   != FIFFV_COORD_HEAD)
    t = NULL;
      else
    tag.data = NULL;
      break;

    case FIFF_CH_INFO :		/* Information about one channel */
      if (fiff_read_this_tag (in->fd,this_ent->pos,&tag) == FIFF_FAIL)
    goto bad;
      this_ch = (fiffChInfo)(tag.data);
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
    if ((t = mne_read_meas_transform(name)) == NULL) {
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
    meg = MALLOC(nmeg,fiffChInfoRec);
  if (neeg > 0)
    eeg = MALLOC(neeg,fiffChInfoRec);
  if (nmeg_comp > 0)
    meg_comp = MALLOC(nmeg_comp,fiffChInfoRec);
  neeg = nmeg = nmeg_comp = 0;

  for (k = 0; k < nchan; k++)
    if (chs[k].kind == FIFFV_MEG_CH)
      meg[nmeg++] = chs[k];
    else if (chs[k].kind == FIFFV_REF_MEG_CH)
      meg_comp[nmeg_comp++] = chs[k];
    else if (chs[k].kind == FIFFV_EEG_CH)
      eeg[neeg++] = chs[k];
  fiff_close(in);
  FREE(chs);
  if (megp) {
    *megp  = meg;
    *nmegp = nmeg;
  }
  else
    FREE(meg);
  if (meg_compp) {
    *meg_compp = meg_comp;
    *nmeg_compp = nmeg_comp;
  }
  else
    FREE(meg_comp);
  if (eegp) {
    *eegp  = eeg;
    *neegp = neeg;
  }
  else
    FREE(eeg);
  if (idp == NULL) {
    FREE(id);
  }
  else
    *idp   = id;
  if (meg_head_t == NULL) {
    FREE(t);
  }
  else
    *meg_head_t = t;

  return FIFF_OK;

  bad : {
    fiff_close(in);
    FREE(chs);
    FREE(meg);
    FREE(eeg);
    FREE(id);
    FREE(tag.data);
    FREE(t);
    return FIFF_FAIL;
  }
}

















//============================= mne_read_process_forward_solution.c =============================


void mne_merge_channels(fiffChInfo chs1, int nch1,
            fiffChInfo chs2, int nch2,
            fiffChInfo *resp, int *nresp)

{
  fiffChInfo res = MALLOC(nch1+nch2,fiffChInfoRec);
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

static fiffDirNode find_meas (fiffDirNode node)
     /*
      * Find corresponding meas node
      */
{
  while (node->type != FIFFB_MEAS) {
    if (node->parent == NULL)
      return (NULL);
    node = node->parent;
  }
  return (node);
}

static fiffDirNode find_meas_info (fiffDirNode node)
     /*
      * Find corresponding meas info node
      */
{
  int k;

  while (node->type != FIFFB_MEAS) {
    if (node->parent == NULL)
      return (NULL);
    node = node->parent;
  }
  for (k = 0; k < node->nchild; k++)
    if (node->children[k]->type == FIFFB_MEAS_INFO)
      return (node->children[k]);
  return (NULL);
}

static int get_all_chs (fiffFile file,	        /* The file we are reading */
            fiffDirNode node,	/* The directory node containing our data */
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
  fiffTagRec tag;
  fiffDirEntry this_entry;
  fiffChInfo ch;
  fiffChInfo this_ch;
  int j,k;
  int to_find = 0;
  fiffDirNode meas;

  tag.data = NULL;
  *chp     = NULL;
  ch       = NULL;
  *id      = NULL;
  /*
   * Find desired parents
   */
  if ((meas = find_meas(node)) == NULL) {
    qCritical ("Meas. block not found!");
    goto bad;
  }
  if ((node = find_meas_info(node)) == NULL) {
    qCritical ("Meas. info not found!");
    goto bad;
  }
  /*
   * Is there a block id is in the FIFFB_MEAS node?
   */
  if (meas->id != NULL) {
    *id = MALLOC(1,fiffIdRec);
    memcpy (*id,meas->id,sizeof(fiffIdRec));
  }
  /*
   * Others from FIFFB_MEAS_INFO
   */
  for (k = 0,this_entry = node->dir; k < node->nent; k++,this_entry++)
    switch (this_entry->kind) {

    case FIFF_NCHAN :
      if (fiff_read_this_tag (file->fd,this_entry->pos,&tag) == -1)
    goto bad;
      *nchan = *(int *)(tag.data);
      ch = MALLOC(*nchan,fiffChInfoRec);
      for (j = 0; j < *nchan; j++)
    ch[j].scanNo = -1;
      to_find = to_find + *nchan - 1;
      break;

    case FIFF_CH_INFO :		/* Information about one channel */
      if (fiff_read_this_tag (file->fd,this_entry->pos,&tag) == -1)
    goto bad;
      this_ch = (fiffChInfo)(tag.data);
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
  FREE (tag.data);
  *chp = ch;
  return FIFF_OK;

  bad : {
    FREE (ch);
    FREE (tag.data);
    return FIFF_FAIL;
  }
}


static int read_ch_info(char       *name,
            fiffChInfo *chsp,
            int        *nchanp,
            fiffId     *idp)
     /*
      * Read channel information from a measurement file
      */
{
  fiffChInfo chs = NULL;
  int        nchan = 0;
  fiffId     id = NULL;

  fiffFile       in = NULL;
  fiffTagRec     tag;
  fiffDirNode    *meas = NULL;
  fiffDirNode    node;

  tag.data = NULL;
  if ((in = fiff_open(name)) == NULL)
    goto bad;
  meas = fiff_dir_tree_find(in->dirtree,FIFFB_MEAS);
  if (meas[0] == NULL) {
    qCritical ("%s : no MEG data available here",name);
    goto bad;
  }
  node = meas[0]; FREE(meas);
  if (get_all_chs (in,node,&id,&chs,&nchan) == FIFF_FAIL)
    goto bad;
  *chsp   = chs;
  *nchanp = nchan;
  *idp = id;
  fiff_close(in);
  return FIFF_OK;

  bad : {
    FREE(tag.data);
    FREE(chs);
    FREE(id);
    fiff_close(in);
    return FIFF_FAIL;
  }
}


#define TOO_CLOSE 1e-4

static int at_origin (float *rr)

{
  return (VEC_LEN(rr) < TOO_CLOSE);
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



int read_meg_eeg_ch_info(char       *name,       /* Input file */
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
    meg = MALLOC(nmeg,fiffChInfoRec);
  if (neeg > 0)
    eeg = MALLOC(neeg,fiffChInfoRec);
  neeg = nmeg = 0;
  for (k = 0; k < nchan; k++)
    if (accept_ch(chs+k,bads,nbad)) {
      if (do_meg && chs[k].kind == FIFFV_MEG_CH)
    meg[nmeg++] = chs[k];
      else if (do_eeg && chs[k].kind == FIFFV_EEG_CH && is_valid_eeg_ch(chs+k))
    eeg[neeg++] = chs[k];
    }
  FREE(chs);
  mne_merge_channels(meg,nmeg,eeg,neeg,chsp,&nch);
  FREE(meg);
  FREE(eeg);
  *nmegp = nmeg;
  *neegp = neeg;
  FREE(id);
  return FIFF_OK;

  bad : {
    FREE(chs);
    FREE(meg);
    FREE(eeg);
    FREE(id);
    return FIFF_FAIL;
  }
}




//============================= mne_filename_util.c =============================


char *mne_compose_mne_name(const char *path, const char *filename)
     /*
      * Compose a filename under the "$MNE_ROOT" directory
      */
{
  char *res;
  char *mne_root;

  if (filename == NULL) {
    qCritical("No file name specified to mne_compose_mne_name");
    return NULL;
  }
  mne_root = getenv(MNE_ENV_ROOT);
  if (mne_root == NULL || strlen(mne_root) == 0) {
    qCritical("Environment variable MNE_ROOT not set");
    return NULL;
  }
  if (path == NULL || strlen(path) == 0) {
    res = MALLOC(strlen(mne_root)+strlen(filename)+2,char);
    strcpy(res,mne_root);
    strcat(res,"/");
    strcat(res,filename);
  }
  else {
    res = MALLOC(strlen(mne_root)+strlen(filename)+strlen(path)+3,char);
    strcpy(res,mne_root);
    strcat(res,"/");
    strcat(res,path);
    strcat(res,"/");
    strcat(res,filename);
  }
  return res;
}




//============================= misc_util.c =============================

char *mne_strdup(const char *s)
{
  char *res;
  if (s == NULL)
    return NULL;
  res = (char*) malloc(strlen(s)+1);
  strcpy(res,s);
  return res;
}








//============================= fiff_trans.c =============================

static void add_inverse(fiffCoordTrans t)
     /*
      * Add inverse transform to an existing one
      */
{
  int j,k;
  float res;

  for (j = 0; j < 3; j++)
    for (k = 0; k < 3; k++)
      t->invrot[j][k] = t->rot[k][j];
  for (j = 0; j < 3; j++) {
    for (res = 0.0, k = 0; k < 3; k++)
      res += t->invrot[j][k]*t->move[k];
    t->invmove[j] = -res;
  }
}








fiffCoordTrans fiff_invert_transform (fiffCoordTrans t)

{
  fiffCoordTrans ti = (fiffCoordTrans)malloc(sizeof(fiffCoordTransRec));
  int j,k;

  for (j = 0; j < 3; j++) {
    ti->move[j] = t->invmove[j];
    ti->invmove[j] = t->move[j];
    for (k = 0; k < 3; k++) {
      ti->rot[j][k]    = t->invrot[j][k];
      ti->invrot[j][k] = t->rot[j][k];
    }
  }
  ti->from = t->to;
  ti->to   = t->from;
  return (ti);
}








fiffCoordTrans fiff_make_transform (int from,int to,float rot[3][3],float move[3])
     /*
      * Compose the coordinate transformation structure
      * from a known forward transform
      */
{
  fiffCoordTrans t = (fiffCoordTrans)malloc(sizeof(fiffCoordTransRec));
  int j,k;

  t->from = from;
  t->to   = to;

  for (j = 0; j < 3; j++) {
    t->move[j] = move[j];
    for (k = 0; k < 3; k++)
      t->rot[j][k] = rot[j][k];
  }
  add_inverse (t);
  return (t);
}

void fiff_coord_trans (float r[3],fiffCoordTrans t,int do_move)
     /*
      * Apply coordinate transformation
      */
{
  int j,k;
  float res[3];

  for (j = 0; j < 3; j++) {
    res[j] = (do_move ? t->move[j] :  0.0);
    for (k = 0; k < 3; k++)
      res[j] += t->rot[j][k]*r[k];
  }
  for (j = 0; j < 3; j++)
    r[j] = res[j];
}






//============================= mne_coord_transforms.c =============================

typedef struct {
  int frame;
  const char *name;
} frameNameRec;


const char *mne_coord_frame_name(int frame)

{
    static frameNameRec frames[] = {
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


fiffCoordTrans mne_read_transform(char *name,int from, int to)
     /*
      * Read the specified coordinate transformation
      */
{
  fiffCoordTrans res = NULL;
  fiffFile       in = NULL;
  fiffTagRec     tag;
  fiffDirEntry   dir;
  int k;

  tag.data = NULL;
  if ((in = fiff_open(name)) == NULL)
    goto out;
  for (k = 0, dir = in->dir; k < in->nent; k++,dir++)
    if (dir->kind == FIFF_COORD_TRANS) {
      if (fiff_read_this_tag (in->fd,dir->pos,&tag) == FIFF_FAIL)
        goto out;
      res = (fiffCoordTrans)tag.data;
      if (res->from == from && res->to == to) {
    tag.data = NULL;
    goto out;
      }
      else if (res->from == to && res->to == from) {
    res = fiff_invert_transform(res);
    goto out;
      }
      res = NULL;
    }
  qCritical("No suitable coordinate transformation found in %s.",name);
  goto out;

  out : {
    FREE(tag.data);
    fiff_close(in);
    return res;
  }

  return res;
}

fiffCoordTrans mne_read_mri_transform(char *name)
     /*
      * Read the MRI -> HEAD coordinate transformation
      */
{
  return mne_read_transform(name,FIFFV_COORD_MRI,FIFFV_COORD_HEAD);
}


fiffCoordTrans mne_read_meas_transform(char *name)
     /*
      * Read the MEG device -> HEAD coordinate transformation
      */
{
  return mne_read_transform(name,FIFFV_COORD_DEVICE,FIFFV_COORD_HEAD);
}






//============================= dipole_forward.c =============================


dipoleForward new_dipole_forward()

{
  dipoleForward res = MALLOC(1,dipoleForwardRec);

  res->rd     = NULL;
  res->fwd    = NULL;
  res->scales = NULL;
  res->uu     = NULL;
  res->vv     = NULL;
  res->sing   = NULL;
  res->nch    = 0;
  res->ndip   = 0;

  return res;
}




void free_dipole_forward ( dipoleForward f )
{
    if (!f)
        return;
    FREE_CMATRIX(f->rd);
    FREE_CMATRIX(f->fwd);
    FREE_CMATRIX(f->uu);
    FREE_CMATRIX(f->vv);
    FREE(f->sing);
    FREE(f->scales);
    FREE(f);
    return;
}





int compute_dipole_field(dipoleFitData d, float *rd, int whiten, float **fwd)
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
    fprintf(stdout,"%g ",sqrt(mne_dot_vectors(fwd[k],fwd[k],d->nmeg+d->neeg)));
  fprintf(stdout,"\n");
#endif

  for (k = 0; k < 3; k++)
    if (mne_proj_op_proj_vector(d->proj,fwd[k],d->nmeg+d->neeg,TRUE) == FAIL)
      goto bad;

#ifdef DEBUG
  fprintf(stdout,"proj : ");
  for (k = 0; k < 3; k++)
    fprintf(stdout,"%g ",sqrt(mne_dot_vectors(fwd[k],fwd[k],d->nmeg+d->neeg)));
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
    fprintf(stdout,"%g ",sqrt(mne_dot_vectors(fwd[k],fwd[k],d->nmeg+d->neeg)));
  fprintf(stdout,"\n");
#endif

  return OK;

 bad :
    return FAIL;
}





dipoleForward dipole_forward(dipoleFitData d,
                 float         **rd,
                 int           ndip,
                 dipoleForward old)
/*
 * Compute the forward solution and do other nice stuff
 */
{
  dipoleForward res;
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
    free_dipole_forward(old); old = NULL;
    res = new_dipole_forward();
    res->fwd  = ALLOC_CMATRIX(3*ndip,d->nmeg+d->neeg);
    res->uu   = ALLOC_CMATRIX(3*ndip,d->nmeg+d->neeg);
    res->vv   = ALLOC_CMATRIX(3*ndip,3);
    res->sing = MALLOC(3*ndip,float);
    res->nch  = d->nmeg+d->neeg;
    res->rd   = ALLOC_CMATRIX(ndip,3);
    res->scales = MALLOC(3*ndip,float);
    res->ndip = ndip;
  }
  for (k = 0; k < ndip; k++) {
    VEC_COPY(res->rd[k],rd[k]);
    this_fwd = res->fwd + 3*k;
    /*
     * Calculate the field of three orthogonal dipoles
     */
    if ((compute_dipole_field(d,rd[k],TRUE,this_fwd)) == FAIL)
      goto bad;
    /*
     * Choice of column normalization
     * (componentwise normalization is not recommended)
     */
    if (d->column_norm == COLUMN_NORM_LOC || d->column_norm == COLUMN_NORM_COMP) {
      for (p = 0; p < 3; p++)
    S[p] = mne_dot_vectors(res->fwd[3*k+p],res->fwd[3*k+p],res->nch);
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
      mne_scale_vector(res->scales[3*k+p],res->fwd[3*k+p],res->nch);
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
  if (mne_svd(res->fwd,3*ndip,d->nmeg+d->neeg,res->sing,res->vv,res->uu) != 0)
    goto bad;
  return res;

 bad : {
    if (!old)
      free_dipole_forward(res);
    return NULL;
  }
}

dipoleForward dipole_forward_one(dipoleFitData d,
                 float         *rd,
                 dipoleForward old)
/*
 * Convenience function to compute the field of one dipole
 */
{
  float *rds[1];
  rds[0] = rd;
  return dipole_forward(d,rds,1,old);
}









//============================= fwd_coil_def.c =============================

#define MAXWORD 1000

#define BIG 0.5



/*
 * This is the old interface which should be eventually deleted
 */
static fwdCoil    fwd_new_coil(int np)

{
  fwdCoil res = MALLOC(1,fwdCoilRec);
  int     k;

  res->chname     = NULL;
  res->desc       = NULL;
  res->coil_class = FWD_COILC_UNKNOWN;
  res->accuracy   = FWD_COIL_ACCURACY_POINT;
  res->base       = 0.0;
  res->size       = 0.0;
  res->np         = np;
  res->rmag       = ALLOC_CMATRIX(np,3);
  res->cosmag     = ALLOC_CMATRIX(np,3);
  res->w          = MALLOC(np,float);
  /*
   * Reasonable defaults
   */
  for (k = 0; k < 3; k++) {
    res->r0[k] = 0.0;
    res->ex[k] = 0.0;
    res->ey[k] = 0.0;
    res->ez[k] = 0.0;
  }
  res->ex[0] = 1.0;
  res->ey[1] = 1.0;
  res->ez[2] = 1.0;

  return res;
}

static void fwd_free_coil(fwdCoil coil)

{
  if (!coil)
    return;

  FREE(coil->chname);
  FREE(coil->desc);
  FREE_CMATRIX(coil->rmag);
  FREE_CMATRIX(coil->cosmag);
  FREE(coil->w);
  FREE(coil);
}


fwdCoilSet fwd_new_coil_set()

{
  fwdCoilSet s = MALLOC(1,fwdCoilSetRec);

  s->coils = NULL;
  s->ncoil = 0;
  s->coord_frame = FIFFV_COORD_UNKNOWN;
  s->user_data = NULL;
  s->user_data_free = NULL;
  return s;
}

void fwd_free_coil_set_user_data(fwdCoilSet set)

{
  if (!set)
    return;
  if (set->user_data_free && set->user_data)
    set->user_data_free(set->user_data);
  set->user_data = NULL;
  return;
}

void fwd_free_coil_set(fwdCoilSet set)

{
  int k;

  if (!set)
    return;

  for (k = 0; k < set->ncoil; k++)
    fwd_free_coil(set->coils[k]);
  FREE(set->coils);

  fwd_free_coil_set_user_data(set);

  FREE(set);
  return;
}

int fwd_is_axial_coil(fwdCoil coil)

{
  return (coil->coil_class == FWD_COILC_MAG ||
      coil->coil_class == FWD_COILC_AXIAL_GRAD ||
      coil->coil_class == FWD_COILC_AXIAL_GRAD2);
}

int fwd_is_magnetometer_coil(fwdCoil coil)

{
  return coil->coil_class == FWD_COILC_MAG;
}

int fwd_is_planar_coil(fwdCoil coil)

{
  return coil->coil_class == FWD_COILC_PLANAR_GRAD;
}

int fwd_is_eeg_electrode(fwdCoil coil)

{
  return coil->coil_class == FWD_COILC_EEG;
}

int fwd_is_planar_coil_type(int type,            /* This is the coil type we are interested in */
                fwdCoilSet set)	 /* Set of templates */

{
  int k;

  if (type == FIFFV_COIL_EEG)
    return FALSE;
  if (!set)
    return FALSE;
  for (k = 0; k < set->ncoil; k++)
    if (set->coils[k]->type == type)
      return set->coils[k]->coil_class == FWD_COILC_PLANAR_GRAD;
  return FALSE;
}


int fwd_is_axial_coil_type(int type,             /* This is the coil type we are interested in */
               fwdCoilSet set)	 /* Set of templates */

{
  int k;

  if (type == FIFFV_COIL_EEG)
    return FALSE;
  if (!set)
    return FALSE;
  for (k = 0; k < set->ncoil; k++)
    if (set->coils[k]->type == type)
      return (set->coils[k]->coil_class == FWD_COILC_MAG ||
          set->coils[k]->coil_class == FWD_COILC_AXIAL_GRAD ||
          set->coils[k]->coil_class == FWD_COILC_AXIAL_GRAD2);
  return FALSE;
}

int fwd_is_magnetometer_coil_type(int type,             /* This is the coil type we are interested in */
                  fwdCoilSet set)	/* Set of templates */

{
  int k;

  if (type == FIFFV_COIL_EEG)
    return FALSE;
  if (!set)
    return FALSE;
  for (k = 0; k < set->ncoil; k++)
    if (set->coils[k]->type == type)
      return set->coils[k]->coil_class == FWD_COILC_MAG;
  return FALSE;
}

int fwd_is_eeg_electrode_type(int type,		 /*  */
                  fwdCoilSet set)	 /* Templates are included for symmetry */

{
  return type == FIFFV_COIL_EEG;
}


static void normalize(float *rr)
     /*
      * Scale vector to unit length
      */
{
  float ll = VEC_LEN(rr);
  int k;
  if (ll > 0) {
    for (k = 0; k < 3; k++)
      rr[k] = rr[k]/ll;
  }
  return;
}

static fwdCoil fwd_add_coil_to_set(fwdCoilSet set,
                   int type, int coil_class, int acc, int np, float size, float base, char *desc)

{
  fwdCoil def;

  if (set == NULL) {
    qDebug() << "qCritical (No coil definition set to augment.)";
    return NULL;
  }
  if (np <= 0) {
    qDebug() << "err_printf_set_error(Number of integration points should be positive (type = %d acc = %d),type,acc)";
    return NULL;
  }
  if (! (acc == FWD_COIL_ACCURACY_POINT ||
        acc == FWD_COIL_ACCURACY_NORMAL ||
        acc == FWD_COIL_ACCURACY_ACCURATE) ) {
    qDebug() << "err_printf_set_error(Illegal accuracy (type = %d acc = %d),type,acc)";
    return NULL;
  }
  if (! (coil_class == FWD_COILC_MAG ||
        coil_class == FWD_COILC_AXIAL_GRAD ||
        coil_class == FWD_COILC_PLANAR_GRAD ||
        coil_class == FWD_COILC_AXIAL_GRAD2) ) {
    qDebug() << "err_printf_set_error(Illegal coil class (type = %d acc = %d class = %d),type,acc,coil_class)";
    return NULL;
  }

  set->coils = REALLOC(set->coils,set->ncoil+1,fwdCoil);
  def = set->coils[set->ncoil++] = fwd_new_coil(np);

  def->type       = type;
  def->coil_class = coil_class;
  def->accuracy   = acc;
  def->np         = np;
  def->base       = size;
  def->base       = base;
  if (desc)
    def->desc = mne_strdup(desc);
  return def;
}



static void skip_comments(FILE *in)

{
  int c;

  while (1) {
    c = fgetc(in);
    if (c == '#') {
      for (c = fgetc(in); c != EOF && c != '\n'; c = fgetc(in))
    ;
    }
    else {
      ungetc(c,in);
      return;
    }
  }
}

static int whitespace(int c)

{
  if (c == '\t' || c == '\n' || c == ' ')
    return TRUE;
  else
    return FALSE;
}

static int whitespace_quote(int c, int inquote)

{
  if (inquote)
    return (c == '"');
  else
    return (c == '\t' || c == '\n' || c == ' ');
}

static char *next_word(FILE *in)

{
  char *next = MALLOC(MAXWORD,char);
  int c;
  int  p,k;
  int  inquote;

  skip_comments(in);

  inquote = FALSE;
  for (k = 0, p = 0, c = fgetc(in); c != EOF && !whitespace_quote(c,inquote) ; c = fgetc(in), k++) {
    if (k == 0 && c == '"')
      inquote = TRUE;
    else
      next[p++] = c;
  }
  if (c == EOF && k == 0) {
    FREE(next);
    return NULL;
  }
  else
    next[p] = '\0';
  if (c != EOF) {
    for (k = 0, c = fgetc(in); whitespace(c) ; c = fgetc(in), k++)
      ;
    if (c != EOF)
      ungetc(c,in);
  }
#ifdef DEBUG
  if (next)
    printf("<%s>\n",next);
#endif
  return next;
}

static int get_ival(FILE *in, int *ival)

{
  char *next = next_word(in);
  if (next == NULL) {
    qDebug() << "err_printf_set_error(missing integer)";
    return FAIL;
  }
  else if (sscanf(next,"%d",ival) != 1) {
    qDebug() << "err_printf_set_error(bad integer : %s,next)";
    FREE(next);
    return FAIL;
  }
  FREE(next);
  return OK;
}

static int get_fval(FILE *in, float *fval)

{
  char *next = next_word(in);
  if (next == NULL) {
    qDebug() << "qCritical (bad integer)";
    return FAIL;
  }
  else if (sscanf(next,"%g",fval) != 1) {
    qDebug() << "err_printf_set_error(bad floating point number : %s,next)";
    FREE(next);
    return FAIL;
  }
  FREE(next);
  return OK;
}






fwdCoilSet fwd_read_coil_defs(char *name)
     /*
      * Read a coil definition file
      */
{
  FILE    *in = fopen(name,"r");
  char    *desc = NULL;
  int     type,coil_class,acc,np;
  int     p;
  float   size,base;
  fwdCoilSet res = NULL;
  fwdCoil def;

  if (in == NULL) {
    qDebug() << "err_set_sys_error(name)";
    goto bad;
  }

  res = fwd_new_coil_set();
  while (1) {
    /*
     * Read basic info
     */
    if (get_ival(in,&coil_class) != OK)
      break;
    if (get_ival(in,&type) != OK)
      goto bad;
    if (get_ival(in,&acc) != OK)
      goto bad;
    if (get_ival(in,&np) != OK)
      goto bad;
    if (get_fval(in,&size) != OK)
      goto bad;
    if (get_fval(in,&base) != OK)
      goto bad;
    desc = next_word(in);
    if (!desc)
      goto bad;

    def = fwd_add_coil_to_set(res,type,coil_class,acc,np,size,base,desc);
    if (!def)
      goto bad;
    FREE(desc); desc = NULL;

    for (p = 0; p < def->np; p++) {
      /*
       * Read and verify data for each integration point
       */
      if (get_fval(in,def->w+p) != OK)
    goto bad;
      if (get_fval(in,def->rmag[p]+X) != OK)
    goto bad;
      if (get_fval(in,def->rmag[p]+Y) != OK)
    goto bad;
      if (get_fval(in,def->rmag[p]+Z) != OK)
    goto bad;
      if (get_fval(in,def->cosmag[p]+X) != OK)
    goto bad;
      if (get_fval(in,def->cosmag[p]+Y) != OK)
    goto bad;
      if (get_fval(in,def->cosmag[p]+Z) != OK)
    goto bad;

    if (VEC_LEN(def->rmag[p]) > BIG) {
        qDebug() << "err_printf_set_error(Unreasonable integration point: %f %f %f mm (coil type = %d acc = %d), 1000*def->rmag[p][X],1000*def->rmag[p][Y],1000*def->rmag[p][Z], def->type,def->accuracy)";
    goto bad;
      }
      size = VEC_LEN(def->cosmag[p]);
      if (size <= 0) {
        qDebug() << "err_printf_set_error(Unreasonable normal: %f %f %f (coil type = %d acc = %d), def->cosmag[p][X],def->cosmag[p][Y],def->cosmag[p][Z], def->type,def->accuracy)";
    goto bad;
      }
      normalize(def->cosmag[p]);
    }
  }
  printf("%d coil definitions read\n",res->ncoil);
  return res;

  bad : {
    fwd_free_coil_set(res);
    FREE(desc);
    return NULL;
  }
}




fwdCoil fwd_create_meg_coil(fwdCoilSet     set,      /* These are the available coil definitions */
                fiffChInfo     ch,       /* Channel information to use */
                int            acc,	     /* Required accuracy */
                fiffCoordTrans t)	     /* Transform the points using this */
     /*
      * Create a MEG coil definition using a database of templates
      * Change the coordinate frame if so desired
      */
{
  int        k,p,c;
  fwdCoil    def;
  fwdCoil    res = NULL;

  if (ch->kind != FIFFV_MEG_CH && ch->kind != FIFFV_REF_MEG_CH) {
    printf("%s is not a MEG channel. Cannot create a coil definition.",ch->ch_name);
    goto bad;
  }
  /*
   * Simple linear search from the coil definitions
   */
  for (k = 0, def = NULL; k < set->ncoil; k++) {
    if ((set->coils[k]->type == (ch->chpos.coil_type & 0xFFFF)) &&
    set->coils[k]->accuracy == acc) {
      def = set->coils[k];
    }
  }
  if (!def) {
    printf("Desired coil definition not found (type = %d acc = %d)",ch->chpos.coil_type,acc);
    goto bad;
  }
  /*
   * Create the result
   */
  res = fwd_new_coil(def->np);

  res->chname   = mne_strdup(ch->ch_name);
  if (def->desc)
    res->desc   = mne_strdup(def->desc);
  res->coil_class = def->coil_class;
  res->accuracy   = def->accuracy;
  res->base       = def->base;
  res->size       = def->size;
  res->type       = ch->chpos.coil_type;

  VEC_COPY(res->r0,ch->chpos.r0);
  VEC_COPY(res->ex,ch->chpos.ex);
  VEC_COPY(res->ey,ch->chpos.ey);
  VEC_COPY(res->ez,ch->chpos.ez);
  /*
   * Apply a coordinate transformation if so desired
   */
  if (t) {
    fiff_coord_trans(res->r0,t,FIFFV_MOVE);
    fiff_coord_trans(res->ex,t,FIFFV_NO_MOVE);
    fiff_coord_trans(res->ey,t,FIFFV_NO_MOVE);
    fiff_coord_trans(res->ez,t,FIFFV_NO_MOVE);
    res->coord_frame = t->to;
  }
  else
    res->coord_frame = FIFFV_COORD_DEVICE;

  for (p = 0; p < res->np; p++) {
    res->w[p] = def->w[p];
    for (c = 0; c < 3; c++) {
      res->rmag[p][c]   = res->r0[c] + def->rmag[p][X]*res->ex[c] + def->rmag[p][Y]*res->ey[c] + def->rmag[p][Z]*res->ez[c];
      res->cosmag[p][c] = def->cosmag[p][X]*res->ex[c] + def->cosmag[p][Y]*res->ey[c] + def->cosmag[p][Z]*res->ez[c];
    }
  }
  return res;

  bad : {
    return NULL;
  }
}




fwdCoilSet fwd_create_meg_coils(fwdCoilSet      set,      /* These are the available coil definitions */
                fiffChInfo      chs,      /* Channel information to use */
                int             nch,
                int             acc,	  /* Required accuracy */
                fiffCoordTrans t)	  /* Transform the points using this */

{
  fwdCoilSet res = fwd_new_coil_set();
  fwdCoil    next;
  int        k;

  for (k = 0; k < nch; k++) {
    if ((next = fwd_create_meg_coil(set,chs+k,acc,t)) == NULL)
      goto bad;
    res->coils = REALLOC(res->coils,res->ncoil+1,fwdCoil);
    res->coils[res->ncoil++] = next;
  }
  if (t)
    res->coord_frame = t->to;
  return res;

 bad : {
    fwd_free_coil_set(res);
    return NULL;
  }
}



fwdCoil fwd_create_eeg_el(fiffChInfo     ch,         /* Channel information to use */
              fiffCoordTrans t)	     /* Transform the points using this */
     /*
      * Create an electrode definition. Transform coordinate frame if so desired.
      */
{
  fwdCoil    res = NULL;
  int        c;

  if (ch->kind != FIFFV_EEG_CH) {
    printf("%s is not an EEG channel. Cannot create an electrode definition.",ch->ch_name);
    goto bad;
  }
  if (t && t->from != FIFFV_COORD_HEAD) {
    printf("Inappropriate coordinate transformation in fwd_create_eeg_el");
    goto bad;
  }

  if (VEC_LEN(ch->chpos.ex) < 1e-4)
    res = fwd_new_coil(1);	             /* No reference electrode */
  else
    res = fwd_new_coil(2);		     /* Reference electrode present */

  res->chname     = mne_strdup(ch->ch_name);
  res->desc       = mne_strdup("EEG electrode");
  res->coil_class = FWD_COILC_EEG;
  res->accuracy   = FWD_COIL_ACCURACY_NORMAL;
  res->type       = ch->chpos.coil_type;
  VEC_COPY(res->r0,ch->chpos.r0);
  VEC_COPY(res->ex,ch->chpos.ex);
  /*
   * Optional coordinate transformation
   */
  if (t) {
    fiff_coord_trans(res->r0,t,FIFFV_MOVE);
    fiff_coord_trans(res->ex,t,FIFFV_MOVE);
    res->coord_frame = t->to;
  }
  else
    res->coord_frame = FIFFV_COORD_HEAD;
  /*
   * The electrode location
   */
  for (c = 0; c < 3; c++)
    res->rmag[0][c] = res->cosmag[0][c] = res->r0[c];
  normalize(res->cosmag[0]);
  res->w[0] = 1.0;
  /*
   * Add the reference electrode, if appropriate
   */
  if (res->np == 2) {
    for (c = 0; c < 3; c++)
      res->rmag[1][c] = res->cosmag[1][c] = res->ex[c];
    normalize(res->cosmag[1]);
    res->w[1] = -1.0;
  }
  return res;

  bad : {
    return NULL;
  }
}


fwdCoilSet fwd_create_eeg_els(fiffChInfo      chs,      /* Channel information to use */
                  int             nch,
                  fiffCoordTrans t)	 /* Transform the points using this */

{
  fwdCoilSet res = fwd_new_coil_set();
  fwdCoil    next;
  int        k;

  for (k = 0; k < nch; k++) {
    if ((next = fwd_create_eeg_el(chs+k,t)) == NULL)
      goto bad;
    res->coils = REALLOC(res->coils,res->ncoil+1,fwdCoil);
    res->coils[res->ncoil++] = next;
  }
  if (t)
    res->coord_frame = t->to;
  return res;

 bad : {
    fwd_free_coil_set(res);
    return NULL;
  }
}




//============================= fiff_matrix.c =============================


int *fiff_get_matrix_dims(fiffTag tag)
     /*
      * Interpret dimensions from matrix data (dense and sparse)
      */
{
  int ndim;
  int *dims;
  int *res,k;
  unsigned int tsize = tag->size;
  /*
   * Initial checks
   */
  if (tag->data == NULL) {
    qCritical("fiff_get_matrix_dims: no data available!");
    return NULL;
  }
  if (fiff_type_fundamental(tag->type) != FIFFTS_FS_MATRIX) {
    qCritical("fiff_get_matrix_dims: tag does not contain a matrix!");
    return NULL;
  }
  if (tsize < sizeof(fiff_int_t)) {
    qCritical("fiff_get_matrix_dims: too small matrix data!");
    return NULL;
  }
  /*
   * Get the number of dimensions and check
   */
  ndim = *((fiff_int_t *)((fiff_byte_t *)(tag->data)+tag->size-sizeof(fiff_int_t)));
  if (ndim <= 0 || ndim > FIFFC_MATRIX_MAX_DIM) {
    qCritical("fiff_get_matrix_dims: unreasonable # of dimensions!");
    return NULL;
  }
  if (fiff_type_matrix_coding(tag->type) == FIFFTS_MC_DENSE) {
    if (tsize < (ndim+1)*sizeof(fiff_int_t)) {
      qCritical("fiff_get_matrix_dims: too small matrix data!");
      return NULL;
    }
    res = MALLOC(ndim+1,int);
    res[0] = ndim;
    dims = ((fiff_int_t *)((fiff_byte_t *)(tag->data)+tag->size)) - ndim - 1;
    for (k = 0; k < ndim; k++)
      res[k+1] = dims[k];
  }
  else if (fiff_type_matrix_coding(tag->type) == FIFFTS_MC_CCS ||
       fiff_type_matrix_coding(tag->type) == FIFFTS_MC_RCS) {
    if (tsize < (ndim+2)*sizeof(fiff_int_t)) {
      qCritical("fiff_get_matrix_sparse_dims: too small matrix data!");
      return NULL; }

    res = MALLOC(ndim+2,int);
    res[0] = ndim;
    dims = ((fiff_int_t *)((fiff_byte_t *)(tag->data)+tag->size)) - ndim - 1;
    for (k = 0; k < ndim; k++)
      res[k+1] = dims[k];
    res[ndim+1] = dims[-1];
  }
  else {
    qCritical("fiff_get_matrix_dims: unknown matrix coding.");
    return NULL;
  }
  return res;
}


float **fiff_get_float_matrix(fiffTag tag)
     /*
      * Conversion into the standard
      * representation
      */
{
  int *dims;
  int k;
  float **res;
  float *data;
  unsigned int tsize = tag->size;
  /*
   * Checks first!
   */
  if ( fiff_type_fundamental(tag->type)   != FIFFT_MATRIX ||
       fiff_type_base(tag->type)          != FIFFT_FLOAT ||
       fiff_type_matrix_coding(tag->type) != FIFFTS_MC_DENSE) {
    qCritical("fiff_get_float_matrix: wrong data type!");
    return NULL;
  }
  if ((dims = fiff_get_matrix_dims(tag)) == NULL)
    return NULL;
  if (dims[0] != 2) {
    qCritical("fiff_get_float_matrix: wrong # of dimensions!");
    return NULL;
  }
  if (tsize != dims[1]*dims[2]*sizeof(fiff_float_t) +
      3*sizeof(fiff_int_t)) {
    qCritical("fiff_get_float_matrix: wrong data size!");
    FREE(dims);
    return NULL;
  }
  /*
   * Set up pointers
   */
  res = MALLOC(dims[2],float *);
  data = (float *)(tag->data);
  for (k = 0; k < dims[2]; k++)
    res[k] = data+k*dims[1];
  /*
   * Free unnecessary data and exit
   */
  FREE(dims);
  tag->data = NULL;
  return res;
}









//============================= fiff_sparse.c =============================



fiff_int_t *fiff_get_matrix_sparse_dims(fiffTag tag)
  /*
   * Interpret dimensions and nz from matrix data
   */
{
  return fiff_get_matrix_dims(tag);
}




fiff_sparse_matrix_t *fiff_get_float_sparse_matrix(fiffTag tag)
  /*
   * Conversion into the standard representation
   */
{
  int *dims;
  fiff_sparse_matrix_t *res = NULL;
  int   m,n,nz;
  int   coding,correct_size;

  if ( fiff_type_fundamental(tag->type)   != FIFFT_MATRIX ||
       fiff_type_base(tag->type)          != FIFFT_FLOAT ||
       (fiff_type_matrix_coding(tag->type) != FIFFTS_MC_CCS &&
    fiff_type_matrix_coding(tag->type) != FIFFTS_MC_RCS) ) {
    printf("fiff_get_float_ccs_matrix: wrong data type!");
    return NULL;
  }

  if ((dims = fiff_get_matrix_sparse_dims(tag)) == NULL)
    return NULL;

  if (dims[0] != 2) {
    printf("fiff_get_float_sparse_matrix: wrong # of dimensions!");
    return NULL;
  }

  m   = dims[1];
  n   = dims[2];
  nz  = dims[3];

  coding = fiff_type_matrix_coding(tag->type);
  if (coding == FIFFTS_MC_CCS)
    correct_size = nz*(sizeof(fiff_float_t) + sizeof(fiff_int_t)) +
             (n+1+dims[0]+2)*(sizeof(fiff_int_t));
  else if (coding == FIFFTS_MC_RCS)
    correct_size = nz*(sizeof(fiff_float_t) + sizeof(fiff_int_t)) +
             (m+1+dims[0]+2)*(sizeof(fiff_int_t));
  else {
    printf("fiff_get_float_sparse_matrix: Incomprehensible sparse matrix coding");
    return NULL;
  }
  if (tag->size != correct_size) {
    printf("fiff_get_float_sparse_matrix: wrong data size!");
    FREE(dims);
    return NULL;
  }
  /*
   * Set up structure
   */
  res = MALLOC(1,fiff_sparse_matrix_t);
  res->m      = m;
  res->n      = n;
  res->nz     = nz;
  res->data   = (float *)(tag->data);  tag->data = NULL;
  res->coding = coding;
  res->inds   = (int *)(res->data + res->nz);
  res->ptrs   = res->inds + res->nz;

  FREE(dims);

  return res;
}





//============================= mne_named_vector.c =============================


int mne_pick_from_named_vector(mneNamedVector vec, char **names, int nnames, int require_all, float *res)
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



//============================= mne_named_matrix.c =============================

#define TAG_FREE(x) if (x) {\
               free(x->data);\
               free(x);\
              }



/*
 * Handle matrices whose rows and/or columns are named with a list
 */
mneNamedMatrix mne_build_named_matrix(int  nrow,        /* Number of rows */
                      int  ncol,        /* Number of columns */
                      char **rowlist,   /* List of row (channel) names */
                      char **collist,   /* List of column (channel) names */
                      float **data)
     /*
      * Build a named matrix from the ingredients
      */
{
  mneNamedMatrix mat = MALLOC(1,mneNamedMatrixRec);
  mat->nrow    = nrow;
  mat->ncol    = ncol;
  mat->rowlist = rowlist;
  mat->collist = collist;
  mat->data    = data;
  return mat;
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




void mne_free_name_list(char **list, int nlist)
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
    FREE(list[k]);
  }
  FREE(list);
  return;
}


void mne_free_named_matrix(mneNamedMatrix mat)
     /*
      * Free the matrix and all the data from within
      */
{

  if (!mat)
    return;
  mne_free_name_list(mat->rowlist,mat->nrow);
  mne_free_name_list(mat->collist,mat->ncol);
  FREE_CMATRIX(mat->data);
  FREE(mat);
  return;
}


char **mne_dup_name_list(char **list, int nlist)
/*
 * Duplicate a name list
 */
{
  char **res;
  int  k;
  if (list == NULL || nlist == 0)
    return NULL;
  res = MALLOC(nlist,char *);

  for (k = 0; k < nlist; k++)
    res[k] = mne_strdup(list[k]);
  return res;
}


mneNamedMatrix mne_dup_named_matrix(mneNamedMatrix mat)
/*
 * Duplicate a named matrix
 */
{
  float **data = ALLOC_CMATRIX(mat->nrow,mat->ncol);
  int   j,k;

  for (j = 0; j < mat->nrow; j++)
    for (k = 0; k < mat->ncol; k++)
      data[j][k] = mat->data[j][k];
  return mne_build_named_matrix(mat->nrow,mat->ncol,
                mne_dup_name_list(mat->rowlist,mat->nrow),
                mne_dup_name_list(mat->collist,mat->ncol),data);
}


void mne_string_to_name_list(char *s,char ***listp,int *nlistp)
     /*
      * Convert a colon-separated list into a string array
      */
{
  char **list = NULL;
  int  nlist  = 0;
  char *one,*now=NULL;

  if (s != NULL && strlen(s) > 0) {
    s = mne_strdup(s);
    //strtok_r linux variant; strtok_s windows varainat
    for (one = strtok_s(s,":",&now); one != NULL; one = strtok_s(NULL,":",&now)) {
      list = REALLOC(list,nlist+1,char *);
      list[nlist++] = mne_strdup(one);
    }
    FREE(s);
  }
  *listp  = list;
  *nlistp = nlist;
  return;
}




char *mne_name_list_to_string(char **list,int nlist)
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
  res = MALLOC(len,char);
  res[0] = '\0';
  for (k = len = 0; k < nlist-1; k++) {
    strcat(res,list[k]);
    strcat(res,":");
  }
  strcat(res,list[nlist-1]);
  return res;
}



char *mne_channel_names_to_string(fiffChInfo chs, int nch)
     /*
      * Make a colon-separated string out of channel names
      */
{
  char **names = MALLOC(nch,char *);
  char *res;
  int  k;

  if (nch <= 0)
    return NULL;
  for (k = 0; k < nch; k++)
    names[k] = chs[k].ch_name;
  res = mne_name_list_to_string(names,nch);
  FREE(names);
  return res;
}



void mne_channel_names_to_name_list(fiffChInfo chs, int nch,
                    char ***listp, int *nlistp)

{
  char *s = mne_channel_names_to_string(chs,nch);
  mne_string_to_name_list(s,listp,nlistp);
  FREE(s);
  return;
}


mneNamedMatrix mne_read_named_matrix(fiffFile in,fiffDirNode node,int kind)
     /*
      * Read a named matrix from the specified node
      */
{
  char **colnames = NULL;
  char **rownames = NULL;
  int  ncol = 0;
  int  nrow = 0;
  int  *dims = NULL;
  float **data = NULL;
  int  val;
  char *s;
  fiffTag tag;
  int     k;
  /*
   * If the node is a named-matrix mode, use it.
   * Otherwise, look in first-generation children
   */
  if (node->type == FIFFB_MNE_NAMED_MATRIX) {
    if ((tag = fiff_dir_tree_get_tag(in,node,kind)) == NULL)
      goto bad;
    if ((dims = fiff_get_matrix_dims(tag)) == NULL)
      goto bad;
    if (dims[0] != 2) {
      qCritical("mne_read_named_matrix only works with two-dimensional matrices");
      goto bad;
    }
    if ((data = fiff_get_float_matrix(tag)) == NULL) {
      TAG_FREE(tag);
      goto bad;
    }
  }
  else {
    for (k = 0; k < node->nchild; k++) {
      if (node->children[k]->type == FIFFB_MNE_NAMED_MATRIX) {
    if ((tag = fiff_dir_tree_get_tag(in,node->children[k],kind)) != NULL) {
      if ((dims = fiff_get_matrix_dims(tag)) == NULL)
        goto bad;
      if (dims[0] != 2) {
        qCritical("mne_read_named_matrix only works with two-dimensional matrices");
        goto bad;
      }
      if ((data = fiff_get_float_matrix(tag)) == NULL) {
        TAG_FREE(tag);
        goto bad;
      }
      FREE(tag);
      node = node->children[k];
      break;
    }
      }
    }
    if (!data)
      goto bad;
  }
  /*
   * Separate FIFF_MNE_NROW is now optional
   */
  if ((tag = fiff_dir_tree_get_tag(in,node,FIFF_MNE_NROW)) == NULL)
    nrow = dims[2];
  else {
    nrow = *(int *)(tag->data);
    if (nrow != dims[2]) {
      qCritical("Number of rows in the FIFF_MNE_NROW tag and in the matrix data conflict.");
      goto bad;
    }
  }
  TAG_FREE(tag);
  /*
   * Separate FIFF_MNE_NCOL is now optional
   */
  if ((tag = fiff_dir_tree_get_tag(in,node,FIFF_MNE_NCOL)) == NULL)
    ncol = dims[1];
  else {
    ncol = *(int *)(tag->data);
    if (ncol != dims[1]) {
      qCritical("Number of columns in the FIFF_MNE_NCOL tag and in the matrix data conflict.");
      goto bad;
    }
  }
  TAG_FREE(tag);

  if ((tag = fiff_dir_tree_get_tag(in,node,FIFF_MNE_ROW_NAMES)) != NULL) {
    s = (char *)(tag->data);
    mne_string_to_name_list(s,&rownames,&val);
    TAG_FREE(tag);
    if (val != nrow) {
      qCritical("Incorrect number of entries in the row name list");
      nrow = val;
      goto bad;
    }
  }
  if ((tag = fiff_dir_tree_get_tag(in,node,FIFF_MNE_COL_NAMES)) != NULL) {
    s = (char *)(tag->data);
    mne_string_to_name_list(s,&colnames,&val);
    TAG_FREE(tag);
    if (val != ncol) {
      qCritical("Incorrect number of entries in the column name list");
      ncol = val;
      goto bad;
    }
  }
  FREE(dims);
  return mne_build_named_matrix(nrow,ncol,rownames,colnames,data);

  bad : {
    mne_free_name_list(rownames,nrow);
    mne_free_name_list(colnames,ncol);
    FREE_CMATRIX(data);
    FREE(dims);
    return NULL;
  }
}





//============================= mne_process_bads.c =============================

static int whitespace(char *text)

{
  if (text == NULL || strlen(text) == 0)
    return TRUE;
  if (strspn(text," \t\n\r") == strlen(text))
    return TRUE;
  return FALSE;
}


static char *next_line(char *line, int n, FILE *in)

{
  char *res;

  for (res = fgets(line,n,in); res != NULL; res = fgets(line,n,in))
    if (!whitespace(res))
      if (res[0] != '#')
    break;
  return res;
}

#define MAXLINE 500

int mne_read_bad_channels(char *name, char ***listp, int *nlistp)
     /*
      * Read bad channel names
      */
{
  FILE *in = NULL;
  char **list = NULL;
  int  nlist  = 0;
  char line[MAXLINE+1];
  char *next;


  if (!name || strlen(name) == 0)
    return OK;

  if ((in = fopen(name,"r")) == NULL) {
    qCritical(name);
    goto bad;
  }
  while ((next = next_line(line,MAXLINE,in)) != NULL) {
    if (strlen(next) > 0) {
      if (next[strlen(next)-1] == '\n')
    next[strlen(next)-1] = '\0';
      list = REALLOC(list,nlist+1,char *);
      list[nlist++] = mne_strdup(next);
    }
  }
  if (ferror(in))
    goto bad;

  *listp  = list;
  *nlistp = nlist;

  return OK;

  bad : {
    mne_free_name_list(list,nlist);
    if (in != NULL)
      fclose(in);
    return FAIL;
  }
}



int mne_read_bad_channel_list_from_node(fiffFile in, fiffDirNode node, char ***listp, int *nlistp)

{
  qDebug() << "ToDo: int mne_read_bad_channel_list_from_node(fiffFile in, fiffDirNode node, char ***listp, int *nlistp)";

//  fiffDirNode bad,*temp;
//  char **list = NULL;
//  int  nlist  = 0;
//  fiffTag tag;
//  char *names;

//  if (!node)
//    node = in->dirtree;

//  temp = fiff_dir_tree_find(node,FIFFB_MNE_BAD_CHANNELS);
//  if (temp && temp[0]) {
//    bad = temp[0];
//    FREE(temp);

//    if ((tag = fiff_dir_tree_get_tag(in,bad,FIFF_MNE_CH_NAME_LIST)) != NULL) {
//      names = (char *)tag->data;
//      FREE(tag);
//      mne_string_to_name_list(names,&list,&nlist);
//      FREE(names);
//    }
//  }
//  *listp = list;
//  *nlistp = nlist;
  return OK;
}

int mne_read_bad_channel_list(char *name, char ***listp, int *nlistp)

{
//  fiffFile in = fiff_open(name);
  int res;


  qDebug() << "ToDo: int mne_read_bad_channel_list(char *name, char ***listp, int *nlistp)";

//  if (in == NULL)
//    return FAIL;

//  res = mne_read_bad_channel_list_from_node(in,in->dirtree,listp,nlistp);

//  fiff_close(in);

  return res;
}




//============================= mne_lin_proj.c =============================

/*
* Handle the linear projection operators
*/
mneProjOp mne_new_proj_op()

{
 mneProjOp new_proj_op = MALLOC(1,mneProjOpRec);

 new_proj_op->items     = NULL;
 new_proj_op->nitems    = 0;
 new_proj_op->names     = NULL;
 new_proj_op->nch       = 0;
 new_proj_op->nvec      = 0;
 new_proj_op->proj_data = NULL;
 return new_proj_op;
}




mneProjItem mne_new_proj_op_item()

{
  mneProjItem new_proj_item = MALLOC(1,mneProjItemRec);

  new_proj_item->vecs        = NULL;
  new_proj_item->kind        = FIFFV_PROJ_ITEM_NONE;
  new_proj_item->desc        = NULL;
  new_proj_item->nvec        = 0;
  new_proj_item->active      = TRUE;
  new_proj_item->active_file = FALSE;
  new_proj_item->has_meg     = FALSE;
  new_proj_item->has_eeg     = FALSE;
  return new_proj_item;
}





void mne_free_proj_op_proj(mneProjOp op)

{
  if (op == NULL)
    return;

  mne_free_name_list(op->names,op->nch);
  FREE_CMATRIX(op->proj_data);

  op->names  = NULL;
  op->nch  = 0;
  op->nvec = 0;
  op->proj_data = NULL;

  return;
}


void mne_free_proj_op_item(mneProjItem it)

{
  if (it == NULL)
    return;

  mne_free_named_matrix(it->vecs);
  FREE(it->desc);
  FREE(it);
  return;
}


void mne_free_proj_op(mneProjOp op)

{
  int k;

  if (op == NULL)
    return;

  for (k = 0; k < op->nitems; k++)
    mne_free_proj_op_item(op->items[k]);
  FREE(op->items);

  mne_free_proj_op_proj(op);

  FREE(op);
  return;
}




void mne_proj_op_add_item_act(mneProjOp op, mneNamedMatrix vecs, int kind, const char *desc, int is_active)
     /*
      * Add a new item to an existing projection operator
      */
{
  mneProjItem new_item;
  int         k;

  op->items = REALLOC(op->items,op->nitems+1,mneProjItem);

  op->items[op->nitems] = new_item = mne_new_proj_op_item();

  new_item->active      = is_active;
  new_item->vecs        = mne_dup_named_matrix(vecs);

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
    new_item->desc = mne_strdup(desc);
  new_item->kind = kind;
  new_item->nvec = new_item->vecs->nrow;

  op->nitems++;

  mne_free_proj_op_proj(op);	/* These data are not valid any more */
  return;
}



void mne_proj_op_add_item(mneProjOp op, mneNamedMatrix vecs, int kind, const char *desc)

{
  mne_proj_op_add_item_act(op, vecs, kind, desc, TRUE);
}




mneProjOp mne_dup_proj_op(mneProjOp op)
     /*
      * Provide a duplicate (item data only)
      */
{
  mneProjOp dup = mne_new_proj_op();
  mneProjItem it;
  int k;

  if (!op)
    return NULL;

  for (k = 0; k < op->nitems; k++) {
    it = op->items[k];
    mne_proj_op_add_item_act(dup,it->vecs,it->kind,it->desc,it->active);
    dup->items[k]->active_file = it->active_file;
  }
  return dup;
}



static char *add_string(char *old,char *add)

{
  char *news = NULL;
  if (!old) {
    if (add || strlen(add) > 0)
      news = mne_strdup(add);
  }
  else {
    old = REALLOC(old,strlen(old) + strlen(add) + 1,char);
    strcat(old,add);
    news = old;
  }
  return news;
}




int mne_proj_op_chs(mneProjOp op, char **list, int nlist)

{
  if (op == NULL)
    return OK;

  mne_free_proj_op_proj(op);	/* These data are not valid any more */

  if (nlist == 0)
    return OK;

  op->names = mne_dup_name_list(list,nlist);
  op->nch   = nlist;

  return OK;
}


void mne_proj_op_report_data(FILE *out,const char *tag, mneProjOp op, int list_data,
                 char **exclude, int nexclude)
     /*
      * Output info about the projection operator
      */
{
  int j,k,p,q;
  mneProjItem it;
  mneNamedMatrix vecs;
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


void mne_proj_op_report(FILE *out,const char *tag, mneProjOp op)

{
  mne_proj_op_report_data(out,tag,op, FALSE, NULL, 0);
}


int mne_proj_item_affect(mneProjItem it, char **list, int nlist)
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

int mne_proj_op_affect(mneProjOp op, char **list, int nlist)

{
  int k;
  int naff;

  if (!op)
    return 0;

  for (k = 0, naff = 0; k < op->nitems; k++)
    if (op->items[k]->active && mne_proj_item_affect(op->items[k],list,nlist))
      naff += op->items[k]->nvec;

  return naff;
}

static void clear_these(float *data, char **names, int nnames, const char *start)

{
  int k;
  for (k = 0; k < nnames; k++)
    if (strstr(names[k],start) == names[k])
      data[k] = 0.0;
}



int mne_proj_op_affect_chs(mneProjOp op, fiffChInfo chs, int nch)

{
  char *ch_string;
  int  res;
  char **list;
  int  nlist;


  if (nch == 0)
    return FALSE;
  ch_string = mne_channel_names_to_string(chs,nch);
  mne_string_to_name_list(ch_string,&list,&nlist);
  FREE(ch_string);
  res = mne_proj_op_affect(op,list,nlist);
  mne_free_name_list(list,nlist);
  return res;
}




int mne_proj_op_proj_dvector(mneProjOp op, double *vec, int nch, int do_complement)
     /*
      * Apply projection operator to a vector (doubles)
      * Assume that all dimension checking etc. has been done before
      */
{
  static double *res = NULL;
  static int   res_size = 0;
  float *pvec;
  double w;
  int k,p;

  if (op->nvec <= 0)
    return OK;

  if (op->nch != nch) {
    qCritical("Data vector size does not match projection operator");
    return FAIL;
  }

  if (op->nch > res_size) {
    res = REALLOC(res,op->nch,double);
    res_size = op->nch;
  }

  for (k = 0; k < op->nch; k++)
    res[k] = 0.0;

  for (p = 0; p < op->nvec; p++) {
    pvec = op->proj_data[p];
    for (k = 0, w = 0.0; k < op->nch; k++)
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







int mne_proj_op_apply_cov(mneProjOp op, mneCovMatrix c)
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

  dcov = ALLOC_DCMATRIX(c->ncov,c->ncov);
  /*
   * Return the appropriate result
   */
  if (c->cov_diag) {		/* Pick the diagonals */
    for (j = 0, p = 0; j < c->ncov; j++)
      for (k = 0; k < c->ncov; k++)
    dcov[j][k] = (j == k) ? c->cov_diag[j] : 0;
  }
  else {			/* Return full matrix */
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
  for (k = 0; k < c->ncov; k++)
    if (mne_proj_op_proj_dvector(op,dcov[k],c->ncov,do_complement) != OK)
      return FAIL;
  mne_transpose_dsquare(dcov,c->ncov);
  for (k = 0; k < c->ncov; k++)
    if (mne_proj_op_proj_dvector(op,dcov[k],c->ncov,do_complement) != OK)
      return FAIL;
  /*
   * Return the result
   */
  if (c->cov_diag) {		/* Pick the diagonal elements */
    for (j = 0; j < c->ncov; j++)
      c->cov_diag[j] = dcov[j][j];
    FREE(c->cov); c->cov = NULL;
  }
  else {			/* Put everything back */
    for (j = 0, p = 0; j < c->ncov; j++)
      for (k = 0; k <= j; k++)
    c->cov[p++] = dcov[j][k];
  }
  FREE_DCMATRIX(dcov);

  c->nproj = mne_proj_op_affect(op,c->names,c->ncov);
  return OK;
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

  FREE_CMATRIX(op->proj_data);
  op->proj_data = NULL;
  op->nvec      = 0;

  if (op->nch <= 0)
    return OK;
  if (op->nitems <= 0)
    return OK;

  nvec = mne_proj_op_affect(op,op->names,op->nch);
  if (nvec == 0)
    return OK;

  mat_meg = ALLOC_CMATRIX(nvec, op->nch);
  mat_eeg = ALLOC_CMATRIX(nvec, op->nch);

#ifdef DEBUG
  fprintf(stdout,"mne_proj_op_make_proj_bad\n");
#endif
  for (k = 0, nvec_meg = nvec_eeg = 0; k < op->nitems; k++) {
    if (op->items[k]->active && mne_proj_item_affect(op->items[k],op->names,op->nch)) {
      vec.nvec  = op->items[k]->vecs->ncol;
      vec.names = op->items[k]->vecs->collist;
      if (op->items[k]->has_meg) {
    for (p = 0; p < op->items[k]->nvec; p++, nvec_meg++) {
      vec.data = op->items[k]->vecs->data[p];
      if (mne_pick_from_named_vector(&vec,op->names,op->nch,FALSE,mat_meg[nvec_meg]) == FAIL)
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
      if (mne_pick_from_named_vector(&vec,op->names,op->nch,FALSE,mat_eeg[nvec_eeg]) == FAIL)
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
    size = sqrt(mne_dot_vectors(mat_meg[p],mat_meg[p],op->nch));
    if (size > 0) {
      for (k = 0; k < op->nch; k++)
    mat_meg[p][k] = mat_meg[p][k]/size;
    }
    else
      nzero++;
  }
  if (nzero == nvec_meg) {
    FREE_CMATRIX(mat_meg); mat_meg = NULL; nvec_meg = 0;
  }
  for (p = 0, nzero = 0; p < nvec_eeg; p++) {
    size = sqrt(mne_dot_vectors(mat_eeg[p],mat_eeg[p],op->nch));
    if (size > 0) {
      for (k = 0; k < op->nch; k++)
    mat_eeg[p][k] = mat_eeg[p][k]/size;
    }
    else
      nzero++;
  }
  if (nzero == nvec_eeg) {
    FREE_CMATRIX(mat_eeg); mat_eeg = NULL; nvec_eeg = 0;
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
    sing_meg = MALLOC(nvec_meg+1,float);
    vv_meg   = ALLOC_CMATRIX(nvec_meg,op->nch);
    if (mne_svd(mat_meg,nvec_meg,op->nch,sing_meg,NULL,vv_meg) != OK)
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
    sing_eeg = MALLOC(nvec_eeg+1,float);
    vv_eeg   = ALLOC_CMATRIX(nvec_eeg,op->nch);
    if (mne_svd(mat_eeg,nvec_eeg,op->nch,sing_eeg,NULL,vv_eeg) != OK)
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
  op->proj_data = ALLOC_CMATRIX(op->nvec,op->nch);
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
  FREE(sing_meg);
  FREE_CMATRIX(vv_meg);
  FREE_CMATRIX(mat_meg);
  FREE(sing_eeg);
  FREE_CMATRIX(vv_eeg);
  FREE_CMATRIX(mat_eeg);
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
    FREE(sing_meg);
    FREE_CMATRIX(vv_meg);
    FREE_CMATRIX(mat_meg);
    FREE(sing_eeg);
    FREE_CMATRIX(vv_eeg);
    FREE_CMATRIX(mat_eeg);
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



mneProjOp mne_proj_op_combine(mneProjOp to, mneProjOp from)
     /*
      * Copy items from 'from' operator to 'to' operator
      */
{
  int k;
  mneProjItem it;

  if (to == NULL)
    to = mne_new_proj_op();
  if (from) {
    for (k = 0; k < from->nitems; k++) {
      it = from->items[k];
      mne_proj_op_add_item(to,it->vecs,it->kind,it->desc);
      to->items[to->nitems-1]->active_file = it->active_file;
    }
  }
  return to;
}


mneProjOp mne_proj_op_average_eeg_ref(fiffChInfo chs,
                      int nch)
     /*
      * Make the projection operator for average electrode reference
      */
{
  int eegcount = 0;
  int k;
  float       **vec_data;
  char        **names;
  mneNamedMatrix vecs;
  mneProjOp      op;

  for (k = 0; k < nch; k++)
    if (chs[k].kind == FIFFV_EEG_CH)
      eegcount++;
  if (eegcount == 0) {
    qCritical("No EEG channels specified for average reference.");
    return NULL;
  }

  vec_data = ALLOC_CMATRIX(1,eegcount);
  names    = MALLOC(eegcount,char *);

  for (k = 0, eegcount = 0; k < nch; k++)
    if (chs[k].kind == FIFFV_EEG_CH)
      names[eegcount++] = mne_strdup(chs[k].ch_name);

  for (k = 0; k < eegcount; k++)
    vec_data[0][k] = 1.0/sqrt((double)eegcount);

  vecs = mne_build_named_matrix(1,eegcount,NULL,names,vec_data);

  op = mne_new_proj_op();
  mne_proj_op_add_item(op,vecs,FIFFV_MNE_PROJ_ITEM_EEG_AVREF,"Average EEG reference");

  return op;
}







//============================= mne_lin_proj_io.c =============================





mneProjOp mne_read_proj_op_from_node(fiffFile in, fiffDirNode start)
     /*
      * Load all the linear projection data
      */
{
  mneProjOp   op     = NULL;
  fiffDirNode *proj  = NULL;
  fiffDirNode *items = NULL;
  fiffDirNode node;
  fiffTag     tag;
  int         k;
  char        *item_desc,*desc_tag,*lf;
  int         global_nchan,item_nchan,nlist;
  char        **item_names;
  int         item_kind;
  float       **item_vectors;
  int         item_nvec;
  int         item_active;
  mneNamedMatrix item;

  if (!in) {
    qCritical("File not open mne_read_proj_op_from_node");
    goto bad;
  }
  if (!start)
    start = in->dirtree;

  op = mne_new_proj_op();
  proj = fiff_dir_tree_find(start,FIFFB_PROJ);
  if (proj == NULL || proj[0] == NULL)   /* The caller must recognize an empty projection */
    goto out;
  /*
   * Only the first projection block is recognized
   */
  items = fiff_dir_tree_find(proj[0],FIFFB_PROJ_ITEM);
  if (items == NULL || items[0] == NULL)   /* The caller must recognize an empty projection */
    goto out;
  /*
   * Get a common number of channels
   */
  node = proj[0];
  if ((tag = fiff_dir_tree_get_tag(in,node,FIFF_NCHAN)) == NULL)
    global_nchan = 0;
  else {
    global_nchan = *(int *)tag->data;
    TAG_FREE(tag);
  }
  /*
   * Proceess each item
   */
  for (node = items[0],k = 0; node != NULL; k++, node = items[k]) {
    /*
     * Complicated procedure for getting the description
     */
    item_desc = NULL;
    if ((tag = fiff_dir_tree_get_tag(in,node,
                     FIFF_NAME)) != NULL) {
      item_desc = add_string(item_desc,(char *)tag->data);
    }
    FREE(tag);
    /*
     * Take the first line of description if it exists
     */
    if ((tag = fiff_dir_tree_get_tag(in,node,
                     FIFF_DESCRIPTION)) != NULL) {
      desc_tag = (char *)tag->data;
      if ((lf = strchr(desc_tag,'\n')) != NULL)
    *lf = '\0';
//      if (item_desc != NULL)
//        item_desc = add_string(item_desc," ");
      qDebug() << "ToDo: item_desc = add_string(item_desc," ");";
      item_desc = add_string(item_desc,(char *)desc_tag);
      FREE(desc_tag);
    }
    FREE(tag);
    /*
     * Possibility to override number of channels here
     */
    if ((tag = fiff_dir_tree_get_tag(in,node,FIFF_NCHAN)) == NULL)
      item_nchan = global_nchan;
    else {
      item_nchan = *(int *)tag->data;
      TAG_FREE(tag); tag = NULL;
    }
    if (item_nchan <= 0) {
      qCritical("Number of channels incorrectly specified for one of the projection items.");
      goto bad;
    }
    /*
     * Take care of the channel names
     */
    if ((tag = fiff_dir_tree_get_tag(in,node,FIFF_PROJ_ITEM_CH_NAME_LIST)) == NULL)
      goto bad;
    mne_string_to_name_list((char *)(tag->data),&item_names,&nlist);
    if (nlist != item_nchan) {
      printf("Channel name list incorrectly specified for proj item # %d",k+1);
      mne_free_name_list(item_names,nlist);
      TAG_FREE(tag);
      goto bad;
    }
    TAG_FREE(tag);
    /*
     * Kind of item
     */
    if ((tag = fiff_dir_tree_get_tag(in,node,
                     FIFF_PROJ_ITEM_KIND)) == NULL)
      goto bad;
    item_kind = *(int *)tag->data;
    TAG_FREE(tag);
    /*
     * How many vectors
     */
    if ((tag = fiff_dir_tree_get_tag(in,node,
                     FIFF_PROJ_ITEM_NVEC)) == NULL)
      goto bad;
    item_nvec = *(int *)tag->data;
    TAG_FREE(tag);
    /*
     * The projection data
     */
    if ((tag = fiff_dir_tree_get_tag(in,node,
                     FIFF_PROJ_ITEM_VECTORS)) == NULL)
      goto bad;
    if ((item_vectors = fiff_get_float_matrix(tag)) == NULL) {
      TAG_FREE(tag);
      goto bad;
    }
    FREE(tag); tag = NULL;
    /*
     * Is this item active?
     */
    if ((tag = fiff_dir_tree_get_tag(in,node,
                     FIFF_MNE_PROJ_ITEM_ACTIVE)) != NULL) {
      item_active = *(int *)tag->data;
      TAG_FREE(tag);
    }
    else
      item_active = FALSE;
    /*
     * Ready to add
     */
    item = mne_build_named_matrix(item_nvec,item_nchan,NULL,item_names,item_vectors);
    mne_proj_op_add_item_act(op,item,item_kind,item_desc,item_active);
    mne_free_named_matrix(item);
    op->items[op->nitems-1]->active_file = item_active;
  }

  out :
    return op;

  bad : {
    mne_free_proj_op(op);
    return NULL;
  }
}

mneProjOp mne_read_proj_op(char *name)

{
  fiffFile    in  = fiff_open(name);
  mneProjOp   res = NULL;

  if (in == NULL)
    return NULL;

  res = mne_read_proj_op_from_node(in,NULL);

  fiff_close(in);

  return res;
}







//============================= mne_sparse_matop.c =============================

void mne_free_sparse(mneSparseMatrix mat)

{
  if (mat) {
    FREE(mat->data);
    FREE(mat);
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
} compMap[] = { { MNE_CTFV_NOGRAD,       MNE_CTFV_COMP_NONE },
        { MNE_CTFV_GRAD1,        MNE_CTFV_COMP_G1BR },
        { MNE_CTFV_GRAD2,        MNE_CTFV_COMP_G2BR },
        { MNE_CTFV_GRAD3,        MNE_CTFV_COMP_G3BR },
        { MNE_4DV_COMP1,         MNE_4DV_COMP1 },             /* One-to-one mapping for 4D data */
        { MNE_CTFV_COMP_UNKNOWN, MNE_CTFV_COMP_UNKNOWN }};

/*
 * Allocation and freeing of the data structures
 */
mneCTFcompData mne_new_ctf_comp_data()

{
  mneCTFcompData res = MALLOC(1,mneCTFcompDataRec);
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

mneCTFcompDataSet mne_new_ctf_comp_data_set()

{
  mneCTFcompDataSet res = MALLOC(1,mneCTFcompDataSetRec);

  res->comps   = NULL;
  res->ncomp   = 0;
  res->chs     = NULL;
  res->nch     = 0;
  res->current = NULL;
  res->undo    = NULL;
  return res;
}



void mne_free_ctf_comp_data(mneCTFcompData comp)

{
  if (!comp)
    return;

  mne_free_named_matrix(comp->data);
  mne_free_sparse(comp->presel);
  mne_free_sparse(comp->postsel);
  FREE(comp->presel_data);
  FREE(comp->postsel_data);
  FREE(comp->comp_data);
  FREE(comp);
  return;
}




void mne_free_ctf_comp_data_set(mneCTFcompDataSet set)

{
  int k;

  if (!set)
    return;

  for (k = 0; k < set->ncomp; k++)
    mne_free_ctf_comp_data(set->comps[k]);
  FREE(set->comps);
  FREE(set->chs);
  mne_free_ctf_comp_data(set->current);
  FREE(set);
  return;
}



int mne_unmap_ctf_comp_kind(int ctf_comp)

{
  int k;

  for (k = 0; compMap[k].grad_comp >= 0; k++)
    if (ctf_comp == compMap[k].ctf_comp)
      return compMap[k].grad_comp;
  return ctf_comp;
}


static int mne_calibrate_ctf_comp(mneCTFcompData one,
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

  row_cals = MALLOC(one->data->nrow,float);
  col_cals = MALLOC(one->data->ncol,float);

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





mneCTFcompDataSet mne_read_ctf_comp_data(char *name)
/*
 * Read all CTF compensation data from a given file
 */
{
  fiffFile          in = NULL;
  mneCTFcompDataSet set = NULL;
  mneCTFcompData    one;
  fiffDirNode       *nodes = NULL;
  fiffDirNode       *comps = NULL;
  int               ncomp;
  mneNamedMatrix    mat = NULL;
  int               kind,k;
  fiffTag           tag;
  fiffChInfo        chs = NULL;
  int               nch = 0;
  int               calibrated;
  /*
   * Read the channel information
   */
  {
    fiffChInfo        comp_chs = NULL;
    int               ncompch = 0;

    if (mne_read_meg_comp_eeg_ch_info(name,&chs,&nch,&comp_chs,&ncompch,NULL,NULL,NULL,NULL) == FAIL)
      goto bad;
    if (ncompch > 0) {
      chs = REALLOC(chs,nch+ncompch,fiffChInfoRec);
      for (k = 0; k < ncompch; k++)
    chs[k+nch] = comp_chs[k];
      nch = nch + ncompch;
      FREE(comp_chs);
    }
  }
  /*
   * Read the rest of the stuff
   */
  if ((in = fiff_open(name)) == NULL)
    goto bad;
  set = mne_new_ctf_comp_data_set();
  /*
   * Locate the compensation data sets
   */
  nodes = fiff_dir_tree_find(in->dirtree,FIFFB_MNE_CTF_COMP);
  if (!nodes || !nodes[0])
    goto good;			/* Nothing more to do */
  comps = fiff_dir_tree_find(nodes[0],FIFFB_MNE_CTF_COMP_DATA);
  if (!comps || !comps[0])
    goto good;
  for (ncomp = 0; comps[ncomp] != NULL; ncomp++)
    ;
  FREE(nodes); nodes = NULL;
  /*
   * Set the channel info
   */
  set->chs = chs; chs = NULL;
  set->nch = nch;
  /*
   * Read each data set
   */
  for (k = 0; k < ncomp; k++) {
    mat = mne_read_named_matrix(in,comps[k],FIFF_MNE_CTF_COMP_DATA);
    if (!mat)
      goto bad;
    tag = fiff_dir_tree_get_tag(in,comps[k],FIFF_MNE_CTF_COMP_KIND);
    if (tag) {
      kind = *(int *)tag->data;
      TAG_FREE(tag);
    }
    else
      goto bad;
    tag = fiff_dir_tree_get_tag(in,comps[k],FIFF_MNE_CTF_COMP_CALIBRATED);
    if (tag) {
      calibrated = *(int *)tag->data;
      TAG_FREE(tag);
    }
    else
      calibrated = FALSE;
    /*
     * Add these data to the set
     */
    one = mne_new_ctf_comp_data();
    one->data = mat; mat = NULL;
    one->kind                = kind;
    one->mne_kind            = mne_unmap_ctf_comp_kind(one->kind);
    one->calibrated          = calibrated;

    if (mne_calibrate_ctf_comp(one,set->chs,set->nch,TRUE) == FAIL) {
      printf("Warning: %s Compensation data for '%s' omitted\n");//,err_get_error(),mne_explain_ctf_comp(one->kind));
      mne_free_ctf_comp_data(one);
    }
    else {
      set->comps               = REALLOC(set->comps,set->ncomp+1,mneCTFcompData);
      set->comps[set->ncomp++] = one;
    }
  }
#ifdef DEBUG
  fprintf(stderr,"%d CTF compensation data sets read from %s\n",set->ncomp,name);
#endif
  goto good;

  bad : {
    mne_free_named_matrix(mat);
    FREE(nodes);
    FREE(comps);
    fiff_close(in);
    mne_free_ctf_comp_data_set(set);
    return NULL;
  }

 good : {
    FREE(chs);
    FREE(nodes);
    FREE(comps);
    fiff_close(in);
    return set;
  }
}












//============================= mne_sss_data.c =============================


static mneSssData mne_new_sss_data()

{
  mneSssData s   = MALLOC(1,mneSssDataRec);
  s->job         = FIFFV_SSS_JOB_NOTHING;
  s->coord_frame = FIFFV_COORD_UNKNOWN;
  s->nchan       = 0;
  s->in_order    = 0;
  s->out_order   = 0;
  s->in_nuse     = 0;
  s->out_nuse    = 0;
  s->comp_info   = NULL;
  s->ncomp       = 0;
  return s;
}


void mne_free_sss_data(mneSssData s)

{
  if (!s)
    return;
  FREE(s->comp_info);
  FREE(s);
  return;
}



mneSssData mne_dup_sss_data(mneSssData s)

{
  mneSssData dup = mne_new_sss_data();
  int        k;

  if (!s)
    return NULL;

  *dup = *s;
  dup->comp_info = MALLOC(dup->ncomp,int);

  for (k = 0; k < dup->ncomp; k++)
    dup->comp_info[k] = s->comp_info[k];

  return dup;
}



mneSssData mne_read_sss_data_from_node(fiffFile in, fiffDirNode start)
/*
 * Read the SSS data from the given node of an open file
 */
{
  mneSssData s  = mne_new_sss_data();
  fiffDirNode *sss = NULL;
  fiffDirNode node;
  fiffTag     tag;
  float       *r0;
  int j,p,q,n;
  /*
   * Locate the SSS information
   */
  sss = fiff_dir_tree_find(start,FIFFB_SSS_INFO);
  if (sss && sss[0]) {
    node = sss[0]; FREE(sss);
    /*
     * Read the SSS information, require all tags to be present
     */
    if ((tag = fiff_dir_tree_get_tag(in,node,FIFF_SSS_JOB)) == NULL)
    goto bad;
    s->job = *(fiff_int_t *)tag->data;
    TAG_FREE(tag);

    if ((tag = fiff_dir_tree_get_tag(in,node,FIFF_SSS_FRAME)) == NULL)
      goto bad;
    s->coord_frame = *(fiff_int_t *)tag->data;
    TAG_FREE(tag);

    if ((tag = fiff_dir_tree_get_tag(in,node,FIFF_SSS_ORIGIN)) == NULL)
      goto bad;
    r0 = (float *)tag->data;
    VEC_COPY(s->origin,r0);
    TAG_FREE(tag);

    if ((tag = fiff_dir_tree_get_tag(in,node,FIFF_SSS_ORD_IN)) == NULL)
      goto bad;
    s->in_order = *(fiff_int_t *)tag->data;
    TAG_FREE(tag);

    if ((tag = fiff_dir_tree_get_tag(in,node,FIFF_SSS_ORD_OUT)) == NULL)
      goto bad;
    s->out_order = *(fiff_int_t *)tag->data;
    TAG_FREE(tag);

    if ((tag = fiff_dir_tree_get_tag(in,node,FIFF_SSS_NMAG)) == NULL)
      goto bad;
    s->nchan = *(fiff_int_t *)tag->data;
    TAG_FREE(tag);

    if ((tag = fiff_dir_tree_get_tag(in,node,FIFF_SSS_COMPONENTS)) == NULL)
      goto bad;
    s->comp_info = (fiff_int_t *)tag->data; tag->data = NULL;
    s->ncomp     = tag->size/sizeof(fiff_int_t);
    TAG_FREE(tag);

    if (s->ncomp != (s->in_order*(2+s->in_order) + s->out_order*(2+s->out_order))) {
      printf("Number of SSS components does not match the expansion orders listed in the file");
      goto bad;
    }
    /*
     * Count the components in use
     */
    for (j = 0, n = 3, p = 0; j < s->in_order; j++, n = n + 2) {
      for (q = 0; q < n; q++, p++)
    if (s->comp_info[p])
      s->in_nuse++;
    }
    for (j = 0, n = 3; j < s->out_order; j++, n = n + 2) {
      for (q = 0; q < n; q++, p++)
    s->out_nuse++;
    }
  }
  /*
   * There it is!
   */
  return s;

 bad : {
    /*
     * Not entirely happy
     */
    mne_free_sss_data(s);
    return NULL;
  }
}





//============================= mne_cov_matrix.c =============================

/*
 * Routines for handling the covariance matrices
 */

static mneCovMatrix new_cov(int    kind,
                int    ncov,
                char   **names,
                double *cov,
                double *cov_diag,
                mneSparseMatrix cov_sparse)
     /*
      * Put it together from ingredients
      */
{
  mneCovMatrix new_cov = MALLOC(1,mneCovMatrixRec);
  new_cov->kind       = kind;
  new_cov->ncov       = ncov;
  new_cov->nproj      = 0;
  new_cov->nzero      = 0;
  new_cov->names      = names;
  new_cov->cov        = cov;
  new_cov->cov_diag   = cov_diag;
  new_cov->cov_sparse = cov_sparse;
  new_cov->eigen      = NULL;
  new_cov->lambda     = NULL;
  new_cov->chol       = NULL;
  new_cov->inv_lambda = NULL;
  new_cov->nfree      = 1;
  new_cov->ch_class   = NULL;
  new_cov->proj       = NULL;
  new_cov->sss        = NULL;
  new_cov->bads       = NULL;
  new_cov->nbad       = 0;
  return new_cov;
}

mneCovMatrix mne_new_cov(int    kind,
             int    ncov,
             char   **names,
             double *cov,
             double *cov_diag)

{
  return new_cov(kind,ncov,names,cov,cov_diag,NULL);
}


mneCovMatrix mne_new_cov_dense(int    kind,
                   int    ncov,
                   char   **names,
                   double *cov)

{
  return new_cov(kind,ncov,names,cov,NULL,NULL);
}


mneCovMatrix mne_new_cov_diag(int    kind,
                  int    ncov,
                  char   **names,
                  double *cov_diag)

{
  return new_cov(kind,ncov,names,NULL,cov_diag,NULL);
}


mneCovMatrix mne_new_cov_sparse(int             kind,
                int             ncov,
                char            **names,
                mneSparseMatrix cov_sparse)

{
  return new_cov(kind,ncov,names,NULL,NULL,cov_sparse);
}


static int mne_lt_packed_index(int j, int k)

{
  if (j >= k)
    return k + j*(j+1)/2;
  else
    return j + k*(k+1)/2;
}


mneCovMatrix mne_dup_cov(mneCovMatrix c)

{
  double       *vals;
  int          nval;
  int          k;
  mneCovMatrix res;

  if (c->cov_diag)
    nval = c->ncov;
  else
    nval = (c->ncov*(c->ncov+1))/2;

  vals = MALLOC(nval,double);
  if (c->cov_diag) {
    for (k = 0; k < nval; k++)
      vals[k] = c->cov_diag[k];
    res = mne_new_cov(c->kind,c->ncov,mne_dup_name_list(c->names,c->ncov),NULL,vals);
  }
  else {
    for (k = 0; k < nval; k++)
      vals[k] = c->cov[k];
    res = mne_new_cov(c->kind,c->ncov,mne_dup_name_list(c->names,c->ncov),vals,NULL);
  }
  /*
   * Duplicate additional items
   */
  if (c->ch_class) {
    res->ch_class = MALLOC(c->ncov,int);
    for (k = 0; k < c->ncov; k++)
      res->ch_class[k] = c->ch_class[k];
  }
  res->bads = mne_dup_name_list(c->bads,c->nbad);
  res->nbad = c->nbad;
  res->proj = mne_dup_proj_op(c->proj);
  res->sss  = mne_dup_sss_data(c->sss);

  return res;
}



void mne_free_cov(mneCovMatrix c)
     /*
      * Free a covariance matrix and all its data
      */
{
  if (c == NULL)
    return;
  FREE(c->cov);
  FREE(c->cov_diag);
  mne_free_sparse(c->cov_sparse);
  mne_free_name_list(c->names,c->ncov);
  FREE_CMATRIX(c->eigen);
  FREE(c->lambda);
  FREE(c->inv_lambda);
  FREE(c->chol);
  FREE(c->ch_class);
  mne_free_proj_op(c->proj);
  mne_free_sss_data(c->sss);
  mne_free_name_list(c->bads,c->nbad);
  FREE(c);
  return;
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



mneCovMatrix mne_read_cov(char *name,int kind)
     /*
      * Read a covariance matrix from a fiff
      */
{
  fiffFile       in = NULL;
  fiffTag        tag;
  fiffDirNode    *nodes = NULL;
  fiffDirNode    covnode;

  char            **names    = NULL;	/* Optional channel name list */
  int             nnames     = 0;
  double          *cov       = NULL;
  double          *cov_diag  = NULL;
  mneSparseMatrix cov_sparse = NULL;
  double          *lambda    = NULL;
  float           **eigen    = NULL;
  char            **bads     = NULL;
  int             nbad       = 0;
  int             ncov       = 0;
  int             nfree      = 1;
  mneCovMatrix    res        = NULL;

  int            k,p,nn;
  float          *f;
  mneProjOp      op = NULL;
  mneSssData     sss = NULL;


  if ((in = fiff_open(name)) == NULL)
    goto out;
  nodes = fiff_dir_tree_find(in->dirtree,FIFFB_MNE_COV);
  if (nodes[0] == NULL) {
    printf("No covariance matrix available in %s",name);
    goto out;
  }
  /*
   * Locate the desired matrix
   */
  for (covnode = NULL, k = 0 ; nodes[k] != NULL; k++) {
    if ((tag = fiff_dir_tree_get_tag(in,nodes[k],FIFF_MNE_COV_KIND)) == NULL)
      continue;
    if (*(int *)tag->data == kind) {
      covnode = nodes[k];
      TAG_FREE(tag);
      break;
    }
    TAG_FREE(tag);
  }
  if (covnode == NULL) {
    printf("Desired covariance matrix not found from %s",name);
    goto out;
  }
  /*
   * Read the data
   */
  if ((tag = fiff_dir_tree_get_tag(in,nodes[k],FIFF_MNE_COV_DIM)) == NULL)
    goto out;
  ncov = *(int *)(tag->data);
  TAG_FREE(tag);

  if ((tag = fiff_dir_tree_get_tag(in,nodes[k],FIFF_MNE_COV_NFREE)) != NULL) {
    nfree = *(int *)(tag->data);
    TAG_FREE(tag);
  }
  if ((tag = fiff_dir_tree_get_tag(in,covnode,FIFF_MNE_ROW_NAMES)) != NULL) {
    mne_string_to_name_list((char *)(tag->data),&names,&nnames);
    TAG_FREE(tag);
    if (nnames != ncov) {
      qCritical("Incorrect number of channel names for a covariance matrix");
      goto out;
    }
  }
  if ((tag = fiff_dir_tree_get_tag(in,nodes[k],FIFF_MNE_COV)) == NULL) {
    if ((tag = fiff_dir_tree_get_tag(in,nodes[k],FIFF_MNE_COV_DIAG)) == NULL)
      goto out;
    else {
      if (tag->type == FIFFT_DOUBLE) {
    cov_diag = (double *)(tag->data);
    if (check_cov_data(cov_diag,ncov) != OK)
      goto out;
      }
      else if (tag->type == FIFFT_FLOAT) {
    cov_diag = MALLOC(ncov,double);
    f = (float *)(tag->data);
    for (p = 0; p < ncov; p++)
      cov_diag[p] = f[p];
    FREE(tag->data);
      }
      else {
    printf("Illegal data type for covariance matrix");
    goto out;
      }
      FREE(tag);
    }
  }
  else {
    nn = ncov*(ncov+1)/2;
    if (tag->type == FIFFT_DOUBLE) {
      cov = (double *)(tag->data);
      if (check_cov_data(cov,nn) != OK)
    goto out;
    }
    else if (tag->type == FIFFT_FLOAT) {
      cov = MALLOC(nn,double);
      f = (float *)(tag->data);
      for (p = 0; p < nn; p++)
    cov[p] = f[p];
      FREE(tag->data);
    }
    else if ((cov_sparse = fiff_get_float_sparse_matrix(tag)) == NULL) {
      TAG_FREE(tag);
      goto out;
    }
    FREE(tag);
    if ((tag = fiff_dir_tree_get_tag(in,nodes[k],FIFF_MNE_COV_EIGENVALUES)) != NULL) {
      lambda = (double *)tag->data;
      FREE(tag);
      if ((tag = fiff_dir_tree_get_tag(in,nodes[k],FIFF_MNE_COV_EIGENVECTORS)) == NULL)
    goto out;
      if ((eigen = fiff_get_float_matrix(tag)) == NULL) {
    TAG_FREE(tag);
    goto out;
      }
    }
    /*
     * Read the optional projection operator
     */
    if ((op = mne_read_proj_op_from_node(in,nodes[k])) == NULL)
      goto out;
    /*
     * Read the optional SSS data
     */
    if ((sss = mne_read_sss_data_from_node(in,nodes[k])) == NULL)
      goto out;
    /*
     * Read the optional bad channel list
     */
    if (mne_read_bad_channel_list_from_node(in,nodes[k],&bads,&nbad) == FAIL)
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
    fiff_close(in);
    mne_free_proj_op(op);
    mne_free_sss_data(sss);

    if (!res) {
      mne_free_name_list(names,nnames);
      mne_free_name_list(bads,nbad);
      FREE(cov);
      FREE(cov_diag);
      mne_free_sparse(cov_sparse);
    }
    FREE(nodes);
    return res;
  }

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
  pick = MALLOC(ncov,int);
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
      FREE(pick);
      return NULL;
    }
  }
  if (omit_meg_eeg) {
    is_meg = MALLOC(ncov,int);
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
  names = MALLOC(ncov,char *);
  if (c->cov_diag) {
    cov_diag = MALLOC(ncov,double);
    for (j = 0; j < ncov; j++) {
      cov_diag[j] = c->cov_diag[pick[j]];
      names[j] = mne_strdup(c->names[pick[j]]);
    }
  }
  else {
    cov = MALLOC(ncov*(ncov+1)/2,double);
    for (j = 0; j < ncov; j++) {
      names[j] = mne_strdup(c->names[pick[j]]);
      for (k = 0; k <= j; k++) {
    from = mne_lt_packed_index(pick[j],pick[k]);
    to   = mne_lt_packed_index(j,k);
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
  res = mne_new_cov(c->kind,ncov,names,cov,cov_diag);

  res->bads = mne_dup_name_list(c->bads,c->nbad);
  res->nbad = c->nbad;
  res->proj = mne_dup_proj_op(c->proj);
  res->sss  = mne_dup_sss_data(c->sss);

  if (c->ch_class) {
    res->ch_class = MALLOC(res->ncov,int);
    for (k = 0; k < res->ncov; k++)
      res->ch_class[k] = c->ch_class[pick[k]];
  }
  FREE(pick);
  FREE(is_meg);
  return res;
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
  c->cov_diag = REALLOC(c->cov_diag,c->ncov,double);

  for (k = p = 0; k < c->ncov; k++) {
    c->cov_diag[k] = c->cov[p];
    p = p + k + 2;
  }
  FREE(c->cov);  c->cov = NULL;
#else
  for (j = 0, p = 0; j < c->ncov; j++)
    for (k = 0; k <= j; k++, p++)
      if (j != k)
    c->cov[p] = 0.0;
      else
#endif
  FREE(c->lambda); c->lambda = NULL;
  FREE_CMATRIX(c->eigen); c->eigen = NULL;
  return;

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
  cov->ch_class = REALLOC(cov->ch_class,cov->ncov,int);
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
    FREE(cov->ch_class);
    cov->ch_class = NULL;
    return FAIL;
  }
}



int mne_add_inv_cov(mneCovMatrix c)
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
  c->inv_lambda = REALLOC(c->inv_lambda,c->ncov,double);
  for (k = 0; k < c->ncov; k++) {
    if (src[k] <= 0.0)
      c->inv_lambda[k] = 0.0;
    else
      c->inv_lambda[k] = 1.0/sqrt(src[k]);
  }
  return OK;
}




#define SMALL      1e-29
#define MEG_SMALL  1e-29
#define EEG_SMALL  1e-18

typedef struct {
  double lambda;
  int    no;
} *covSort,covSortRec;

static int comp_cov(const void *v1, const void *v2)

{
  covSort s1 = (covSort)v1;
  covSort s2 = (covSort)v2;
  if (s1->lambda < s2->lambda)
    return -1;
  if (s1->lambda > s2->lambda)
    return 1;
  return 0;
}

static int condition_cov(mneCovMatrix c, float rank_threshold, int use_rank)

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
      magscale += c->cov[mne_lt_packed_index(k,k)]; nmag++;
    }
    else if (c->ch_class[k] == MNE_COV_CH_MEG_GRAD) {
      gradscale += c->cov[mne_lt_packed_index(k,k)]; ngrad++;
    }
    else if (c->ch_class[k] == MNE_COV_CH_EEG) {
      eegscale += c->cov[mne_lt_packed_index(k,k)]; neeg++;
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
  scale = MALLOC(c->ncov,double);
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
  cov    = MALLOC(c->ncov*(c->ncov+1)/2.0,double);
  lambda = MALLOC(c->ncov,double);
  eigen  = ALLOC_CMATRIX(c->ncov,c->ncov);
  for (j = 0; j < c->ncov; j++)
    for (k = 0; k <= j; k++)
      cov[mne_lt_packed_index(j,k)] = c->cov[mne_lt_packed_index(j,k)]*scale[j]*scale[k];
  if (mne_decompose_eigen (cov,lambda,eigen,c->ncov) == 0) {
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
    data1 = ALLOC_DCMATRIX(c->ncov,c->ncov);
    for (j = 0; j < c->ncov; j++) {
#ifdef DEBUG
      mne_print_vector(stdout,NULL,eigen[j],c->ncov);
#endif
      for (k = 0; k < c->ncov; k++)
    data1[j][k] = sqrt(lambda[j])*eigen[j][k];
    }
    data2 = mne_dmatt_dmat_mult2 (data1,data1,c->ncov,c->ncov,c->ncov);
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
    if (c->cov[mne_lt_packed_index(j,k)] != 0.0)
      c->cov[mne_lt_packed_index(j,k)] = scale[j]*scale[k]*data2[j][k];
    res = nok;
  }
  FREE(cov);
  FREE(lambda);
  FREE_CMATRIX(eigen);
  FREE_DCMATRIX(data1);
  FREE_DCMATRIX(data2);
  return res;
}











static int mne_decompose_eigen_cov_small(mneCovMatrix c,float small, int use_rank)
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
    return mne_add_inv_cov(c);
  if (c->lambda && c->eigen) {
    fprintf(stderr,"\n\tEigenvalue decomposition had been precomputed.\n");
    c->nzero = 0;
    for (k = 0; k < c->ncov; k++, c->nzero++)
      if (c->lambda[k] > 0)
    break;
  }
  else {
    FREE(c->lambda); c->lambda = NULL;
    FREE_CMATRIX(c->eigen); c->eigen = NULL;

    if ((rank = condition_cov(c,rank_threshold,use_rank)) < 0)
      return FAIL;

    np = c->ncov*(c->ncov+1)/2;
    c->lambda = MALLOC(c->ncov,double);
    c->eigen  = ALLOC_CMATRIX(c->ncov,c->ncov);
    if (mne_decompose_eigen (c->cov,c->lambda,c->eigen,c->ncov) != 0)
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
  return mne_add_inv_cov(c);

  bad : {
    FREE(c->lambda); c->lambda = NULL;
    FREE_CMATRIX(c->eigen); c->eigen = NULL;
    return FAIL;
  }
}


int mne_decompose_eigen_cov(mneCovMatrix c)

{
  return mne_decompose_eigen_cov_small(c,-1.0,-1);
}






//============================= dipole_fit_setup.c =============================


guessData new_guess_data()

{
  guessData res = MALLOC(1,guessDataRec);

  res->rr        = NULL;
  res->guess_fwd = NULL;
  res->nguess    = 0;
  return res;
}

void free_guess_data(guessData g)

{
  int k;
  if (!g)
    return;

  FREE_CMATRIX(g->rr);
  if (g->guess_fwd) {
    for (k = 0; k < g->nguess; k++)
      free_dipole_forward(g->guess_fwd[k]);
    FREE(g->guess_fwd);
  }
  FREE(g);
  return;
}

static dipoleFitFuncs new_dipole_fit_funcs()

{
  dipoleFitFuncs f = MALLOC(1,dipoleFitFuncsRec);

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

static void free_dipole_fit_funcs(dipoleFitFuncs f)

{
  if (!f)
    return;

  if (f->meg_client_free && f->meg_client)
    f->meg_client_free(f->meg_client);
  if (f->eeg_client_free && f->eeg_client)
    f->eeg_client_free(f->eeg_client);

  FREE(f);
  return;
}


dipoleFitData new_dipole_fit_data()

{
  dipoleFitData res = MALLOC(1,dipoleFitDataRec);

  res->mri_head_t    = NULL;
  res->meg_head_t    = NULL;
  res->chs           = NULL;
  res->meg_coils     = NULL;
  res->eeg_els       = NULL;
  res->nmeg          = 0;
  res->neeg          = 0;
  res->ch_names      = NULL;
  res->pick          = NULL;
  res->r0[0]         = 0.0;
  res->r0[1]         = 0.0;
  res->r0[2]         = 0.0;
  res->bemname       = NULL;
  res->bem_model     = NULL;
  res->eeg_model     = NULL;
  res->fixed_noise   = FALSE;
  res->noise         = NULL;
  res->noise_orig    = NULL;
  res->nave          = 1;
  res->user          = NULL;
  res->user_free     = NULL;
  res->proj          = NULL;

  res->sphere_funcs     = NULL;
  res->bem_funcs        = NULL;
  res->mag_dipole_funcs = NULL;
  res->funcs            = NULL;
  res->column_norm      = COLUMN_NORM_NONE;
  res->fit_mag_dipoles  = FALSE;

  return res;
}


void free_dipole_fit_data(dipoleFitData d)

{
  if (!d)
    return;

  FREE(d->mri_head_t);
  FREE(d->meg_head_t);
  FREE(d->chs);
  fwd_free_coil_set(d->meg_coils);
  fwd_free_coil_set(d->eeg_els);
  FREE(d->bemname);
  mne_free_cov(d->noise);
  mne_free_cov(d->noise_orig);
  mne_free_name_list(d->ch_names,d->nmeg+d->neeg);
  mne_free_sparse(d->pick);
  fwd_bem_free_model(d->bem_model);
  fwd_free_eeg_sphere_model(d->eeg_model);
  if (d->user_free)
    d->user_free(d->user);

  mne_free_proj_op(d->proj);

  free_dipole_fit_funcs(d->sphere_funcs);
  free_dipole_fit_funcs(d->bem_funcs);
  free_dipole_fit_funcs(d->mag_dipole_funcs);

  FREE(d);
  return;
}


static int setup_forward_model(dipoleFitData d, mneCTFcompDataSet comp_data, fwdCoilSet comp_coils)
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
    FREE(d->bemname); d->bemname = bemsolname;

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
        mneSurface inner_skull;
        float      simplex_size = 2e-2;
        float      R;

        if ((inner_skull = fwd_bem_find_surface(d->bem_model,FIFFV_BEM_SURF_ID_BRAIN)) == NULL)
            goto out;

//        static Sphere fit_sphere_simplex(const Eigen::MatrixX3f& points, double simplex_size = 2e-2);
        Eigen::MatrixX3f rr(inner_skull->np,3);
        for (int i = 0; i < inner_skull->np; ++i) {
            rr(i,0) = inner_skull->rr[i][0];
            rr(i,1) = inner_skull->rr[i][1];
            rr(i,2) = inner_skull->rr[i][2];
        }
        Sphere t_sphere = Sphere::fit_sphere_simplex(rr, simplex_size);
//        if (fit_sphere_to_points(inner_skull->rr,inner_skull->np,simplex_size,d->r0,&R) == FAIL)
//            goto out;

        fiff_coord_trans(d->r0,d->mri_head_t,TRUE);
        printf("Fitted sphere model origin : %6.1f %6.1f %6.1f mm rad = %6.1f mm.\n",
        1000*d->r0[X],1000*d->r0[Y],1000*d->r0[Z],1000*R);
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
    VEC_COPY(d->eeg_model->r0,d->r0);
    f->eeg_pot     = fwd_eeg_spherepot_coil;
    f->eeg_vec_pot = fwd_eeg_spherepot_coil_vec;
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
      1000*d->r0[X],1000*d->r0[Y],1000*d->r0[Z]);
#ifdef MAG_DIPOLES
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
#endif
  /*
   * Select the appropriate fitting function
   */
  d->funcs = d->bemname ? d->bem_funcs : d->sphere_funcs;
  fprintf (stderr,"\n");
  return OK;


 out :
  return FAIL;
}



static mneCovMatrix ad_hoc_noise(fwdCoilSet meg,          /* Channel name lists to define which channels are gradiometers */
                 fwdCoilSet eeg,
                 float      grad_std,
                 float      mag_std,
                 float      eeg_std)
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

  stds = MALLOC(nchan,double);
  ch_names = MALLOC(nchan,char *);

  n = 0;
  if (meg) {
    for (k = 0; k < meg->ncoil; k++, n++) {
      if (fwd_is_axial_coil(meg->coils[k])) {
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
  names = mne_dup_name_list(ch_names,nchan);
  FREE(ch_names);
  return mne_new_cov(FIFFV_MNE_NOISE_COV,nchan,names,NULL,stds);
}

static int make_projection(char       **projnames,
               int        nproj,
               fiffChInfo chs,
               int        nch,
               mneProjOp  *res)
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

  if (nproj == 0 && neeg == 0)
    return OK;

  for (k = 0; k < nproj; k++) {
    if ((one = mne_read_proj_op(projnames[k])) == NULL)
      goto bad;
    if (one->nitems == 0) {
      printf("No linear projection information in %s.\n",projnames[k]);
      mne_free_proj_op(one); one = NULL;
    }
    else {
      printf("Loaded projection from %s:\n",projnames[k]);
      mne_proj_op_report(stderr,"\t",one);
      all = mne_proj_op_combine(all,one);
      mne_free_proj_op(one); one = NULL;
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
      if ((one = mne_proj_op_average_eeg_ref(chs,nch)) != NULL) {
    printf("Average EEG reference projection added:\n");
    mne_proj_op_report(stderr,"\t",one);
    all = mne_proj_op_combine(all,one);
    mne_free_proj_op(one); one = NULL;
      }
    }
  }
  if (all && mne_proj_op_affect_chs(all,chs,nch) == 0) {
    printf("Projection will not have any effect on selected channels. Projection omitted.\n");
    mne_free_proj_op(all);
    all = NULL;
  }
  *res = all;
  return OK;

  bad :
    return FAIL;
}

fwdEegSphereModel setup_eeg_sphere_model(char  *eeg_model_file,   /* Contains the model specifications */
                     char  *eeg_model_name,	  /* Name of the model to use */
                     float eeg_sphere_rad)    /* Outer surface radius */
     /*
      * Set up the desired sphere model for EEG
      */
{
  fwdEegSphereModelSet eeg_models = NULL;
  fwdEegSphereModel    eeg_model  = NULL;

  if (!eeg_model_name)
    eeg_model_name = mne_strdup("Default");
  else
    eeg_model_name = mne_strdup(eeg_model_name);

  eeg_models = fwd_load_eeg_sphere_models(eeg_model_file,NULL);
  fwd_list_eeg_sphere_models(stderr,eeg_models);

  if ((eeg_model = fwd_select_eeg_sphere_model(eeg_model_name,eeg_models)) == NULL)
    goto bad;
  if (fwd_setup_eeg_sphere_model(eeg_model,eeg_sphere_rad,TRUE,3) == FAIL)
    goto bad;
  printf("Using EEG sphere model \"%s\" with scalp radius %7.1f mm\n",
      eeg_model->name,1000*eeg_sphere_rad);
  printf("\n");
  FREE(eeg_model_name);
  fwd_free_eeg_sphere_model_set(eeg_models);
  return eeg_model;

  bad : {
    fwd_free_eeg_sphere_model_set(eeg_models);
    fwd_free_eeg_sphere_model(eeg_model);
    FREE(eeg_model_name);
    return NULL;
  }
}


static void regularize_cov(mneCovMatrix c,       /* The matrix to regularize */
               float        *regs,   /* Regularization values to apply (fractions of the
                          * average diagonal values for each class */
               int          *active) /* Which of the channels are 'active' */
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
      if (!active || active[j]) {
    sums[c->ch_class[j]] += c->cov[mne_lt_packed_index(j,j)];
    nn[c->ch_class[j]]++;
      }
    }
  }
  printf("Average noise-covariance matrix diagonals:\n");
  for (j = 0; j < nkind; j++) {
    if (nn[j] > 0) {
      sums[j] = sums[j]/nn[j];
      if (j == MNE_COV_CH_MEG_MAG)
    printf("\tMagnetometers       : %-7.2f fT    reg = %-6.2f\n",1e15*sqrt(sums[j]),regs[j]);
      else if (j == MNE_COV_CH_MEG_GRAD)
    printf("\tPlanar gradiometers : %-7.2f fT/cm reg = %-6.2f\n",1e13*sqrt(sums[j]),regs[j]);
      else
    printf("\tEEG                 : %-7.2f uV    reg = %-6.2f\n",1e6*sqrt(sums[j]),regs[j]);
      sums[j] = regs[j]*sums[j];
    }
  }
  /*
   * Add thee proper amount to the diagonal
   */
  for (j = 0; j < c->ncov; j++)
    if (c->ch_class[j] >= 0)
      c->cov[mne_lt_packed_index(j,j)] += sums[c->ch_class[j]];

  printf("Noise-covariance regularized as requested.\n");
  return;
}



static int scale_dipole_fit_noise_cov(dipoleFitData f,int nave)

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
      if (mne_decompose_eigen_cov(f->noise) == FAIL)
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
    if (mne_add_inv_cov(f->noise) == FAIL)
      goto bad;
  }
  else {
    for (k = 0; k < f->noise->ncov; k++)
      f->noise->cov_diag[k] = nave_ratio*f->noise->cov_diag[k];
    fprintf(stderr,"Decomposition not needed for a diagonal noise covariance matrix.\n");
    if (mne_add_inv_cov(f->noise) == FAIL)
      goto bad;
  }
  fprintf(stderr,"Effective nave is now %d\n",nave);
  f->nave = nave;
  return OK;

 bad :
  return FAIL;
}



int select_dipole_fit_noise_cov(dipoleFitData f, mshMegEegData d)
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
    float  *w    = MALLOC(f->noise_orig->ncov,float);
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
    mne_free_cov(f->noise); f->noise = NULL;
    if (nmeg > 0 && nmeg-nomit_meg > 0 && nmeg-nomit_meg < min_nchan) {
      qCritical("Too few MEG channels remaining");
      return FAIL;
    }
    if (neeg > 0 && neeg-nomit_eeg > 0 && neeg-nomit_eeg < min_nchan) {
      qCritical("Too few EEG channels remaining");
      return FAIL;
    }
    f->noise = mne_dup_cov(f->noise_orig);
    if (nomit_meg+nomit_eeg > 0) {
      if (f->noise->cov) {
    for (j = 0; j < f->noise->ncov; j++)
      for (k = 0; k <= j; k++) {
        val = f->noise->cov+mne_lt_packed_index(j,k);
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
    FREE(w);
  }
  else {
    if (f->noise && f->nave == nave)
      return OK;
    f->noise = mne_dup_cov(f->noise_orig);
  }
  return scale_dipole_fit_noise_cov(f,nave);
}



dipoleFitData setup_dipole_fit_data(char  *mriname,		 /* This gives the MRI/head transform */
                    char  *measname,		 /* This gives the MEG/head transform and
                                  * sensor locations */
                    char  *bemname,		 /* BEM model */
                    float *r0,			 /* Sphere model origin in head coordinates (optional) */
                    fwdEegSphereModel eeg_model, /* EEG sphere model definition */
                    int   accurate_coils,	 /* Use accurate coil definitions? */
                    char  *badname,		 /* Bad channels list */
                    char  *noisename,		 /* Noise covariance matrix */
                    float grad_std,              /* Standard deviations for the ad-hoc noise cov (planar gradiometers) */
                    float mag_std,               /* Ditto for magnetometers */
                    float eeg_std,               /* Ditto for EEG */
                    float mag_reg,               /* Noise-covariance regularization factors */
                    float grad_reg,
                    float eeg_reg,
                    int   diagnoise,		 /* Use only the diagonal elements of the noise-covariance matrix */
                    char  **projnames,           /* SSP file names */
                    int   nproj,                 /* How many of them */
                    int   include_meg,           /* Include MEG in the fitting? */
                    int   include_eeg)           /* Include EEG in the fitting? */
     /*
      * Background work for modelling
      */
{
  dipoleFitData     res = new_dipole_fit_data();
  int               k;
  char              **badlist = NULL;
  int               nbad      = 0;
  char              **file_bads;
  int               file_nbad;
  int               coord_frame = FIFFV_COORD_HEAD;
  mneCovMatrix      cov;
  fwdCoilSet        templates = NULL;
  mneCTFcompDataSet comp_data  = NULL;
  fwdCoilSet        comp_coils = NULL;
  int               dum;
  /*
   * Read the coordinate transformations
   */
  if (mriname) {
    if ((res->mri_head_t = mne_read_mri_transform(mriname)) == NULL)
      goto bad;
  }
  else if (bemname) {
    qCritical("Source of MRI / head transform required for the BEM model is missing");
    goto bad;
  }
  else {
    float move[] = { 0.0, 0.0, 0.0 };
    float rot[3][3] = { { 1.0, 0.0, 0.0 },
            { 0.0, 1.0, 0.0 },
            { 0.0, 0.0, 1.0 } };
    res->mri_head_t = fiff_make_transform(FIFFV_COORD_MRI,FIFFV_COORD_HEAD,rot,move);
  }

//ToDo    //mne_print_coord_transform(stderr,res->mri_head_t);
  if ((res->meg_head_t = mne_read_meas_transform(measname)) == NULL)
    goto bad;
//ToDo    //  mne_print_coord_transform(stderr,res->meg_head_t);
  /*
   * Read the bad channel lists
   */
  if (badname) {
    if (mne_read_bad_channels(badname,&badlist,&nbad) != OK)
      goto bad;
    printf("%d bad channels read from %s.\n",nbad,badname);
  }
  if (mne_read_bad_channel_list(measname,&file_bads,&file_nbad) == OK && file_nbad > 0) {
    if (!badlist)
      nbad = 0;
    badlist = REALLOC(badlist,nbad+file_nbad,char *);
    for (k = 0; k < file_nbad; k++)
      badlist[nbad++] = file_bads[k];
    FREE(file_bads);
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
    printf("Will use %3d MEG channels from %s\n",res->nmeg,measname);
  if (res->neeg > 0)
    printf("Will use %3d EEG channels from %s\n",res->neeg,measname);
  mne_channel_names_to_name_list(res->chs,res->nmeg+res->neeg,&res->ch_names,&dum);
  /*
   * Make coil definitions
   */
  res->coord_frame = coord_frame;
  if (coord_frame == FIFFV_COORD_HEAD) {
#ifdef USE_SHARE_PATH
    char *coilfile = mne_compose_mne_name("share/mne","coil_def.dat");
#else
    const char *path = "setup/mne";
    const char *filename = "coil_def.dat";
    char *coilfile = mne_compose_mne_name(path,filename);
#endif

    if (!coilfile)
      goto bad;
    if ((templates = fwd_read_coil_defs(coilfile)) == NULL) {
      FREE(coilfile);
      goto bad;
    }

    if ((res->meg_coils = fwd_create_meg_coils(templates,res->chs,res->nmeg,
                           accurate_coils ? FWD_COIL_ACCURACY_ACCURATE : FWD_COIL_ACCURACY_NORMAL,
                           res->meg_head_t)) == NULL)
      goto bad;
    if ((res->eeg_els = fwd_create_eeg_els(res->chs+res->nmeg,res->neeg,NULL)) == NULL)
      goto bad;
    printf("Head coordinate coil definitions created.\n");
  }
  else {
    printf("Cannot handle computations in %s coordinates",mne_coord_frame_name(coord_frame));
    goto bad;
  }
  /*
   * Forward model setup
   */
  res->bemname   = mne_strdup(bemname);
  if (r0) {
    res->r0[0]     = r0[0];
    res->r0[1]     = r0[1];
    res->r0[2]     = r0[2];
  }
  res->eeg_model = fwd_dup_eeg_sphere_model(eeg_model);
  /*
   * Compensation data
   */
  if ((comp_data = mne_read_ctf_comp_data(measname)) == NULL)
    goto bad;
  if (comp_data->ncomp > 0) {	/* Compensation channel information may be needed */
    fiffChInfo comp_chs = NULL;
    int        ncomp    = 0;

    printf("%d compensation data sets in %s\n",comp_data->ncomp,measname);
    if (mne_read_meg_comp_eeg_ch_info(measname,NULL,0,&comp_chs,&ncomp,NULL,NULL,NULL,NULL) == FAIL)
      goto bad;
    if (ncomp > 0) {
      if ((comp_coils = fwd_create_meg_coils(templates,comp_chs,ncomp,
                         FWD_COIL_ACCURACY_NORMAL,res->meg_head_t)) == NULL) {
    FREE(comp_chs);
    goto bad;
      }
      printf("%d compensation channels in %s\n",comp_coils->ncoil,measname);
    }
    FREE(comp_chs);
  }
  else {			/* Get rid of the empty data set */
    mne_free_ctf_comp_data_set(comp_data);
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
  if (make_projection(projnames,nproj,res->chs,res->nmeg+res->neeg,&res->proj) == FAIL)
    goto bad;
  if (res->proj && res->proj->nitems > 0) {
    printf("Final projection operator is:\n");
    mne_proj_op_report(stderr,"\t",res->proj);

    if (mne_proj_op_chs(res->proj,res->ch_names,res->nmeg+res->neeg) == FAIL)
      goto bad;
    if (mne_proj_op_make_proj(res->proj) == FAIL)
      goto bad;
  }
  /*
   * Noise covariance
   */
  if (noisename) {
    if ((cov = mne_read_cov(noisename,FIFFV_MNE_SENSOR_COV)) == NULL)
      goto bad;
    printf("Read a %s noise-covariance matrix from %s\n",
        cov->cov_diag ? "diagonal" : "full", noisename);
  }
  else {
    if ((cov = ad_hoc_noise(res->meg_coils,res->eeg_els,grad_std,mag_std,eeg_std)) == NULL)
      goto bad;
  }
  res->noise_orig = mne_pick_chs_cov_omit(cov,res->ch_names,res->nmeg+res->neeg,TRUE,res->chs);
  if (!res->noise_orig) {
    mne_free_cov(cov);
    goto bad;
  }
  printf("Picked appropriate channels from the noise-covariance matrix.\n");
  mne_free_cov(cov);
  /*
   * Apply the projection operator to the noise-covariance matrix
   */
  if (res->proj && res->proj->nitems > 0 && res->proj->nvec > 0) {
    if (mne_proj_op_apply_cov(res->proj,res->noise_orig) == FAIL)
      goto bad;
    printf("Projection applied to the covariance matrix.\n");
  }
  /*
   * Force diagonal noise covariance?
   */
  if (diagnoise) {
    mne_revert_to_diag_cov(res->noise_orig);
    printf("Using only the main diagonal of the noise-covariance matrix.\n");
  }
  /*
   * Classify the channels in the noise covariance
   */
  if (mne_classify_channels_cov(res->noise_orig,res->chs,res->nmeg+res->neeg) == FAIL)
    goto bad;
  /*
   * Regularize the possibly deficient noise-covariance matrix
   */
  if (res->noise_orig->cov) {
    float regs[3];
    int   do_it;

    regs[MNE_COV_CH_MEG_MAG]  = mag_reg;
    regs[MNE_COV_CH_MEG_GRAD] = grad_reg;
    regs[MNE_COV_CH_EEG]      = eeg_reg;
    /*
     * Do we need to do anything?
     */
    for (k = 0, do_it = 0; k < res->noise_orig->ncov; k++) {
      if (res->noise_orig->ch_class[k] != MNE_COV_CH_UNKNOWN &&
      regs[res->noise_orig->ch_class[k]] > 0.0)
    do_it++;
    }
    /*
     * Apply regularization if necessary
     */
    if (do_it > 0)
      regularize_cov(res->noise_orig,regs,NULL);
    else
      printf("No regularization applied to the noise-covariance matrix\n");
  }
  if (select_dipole_fit_noise_cov(res,NULL) == FAIL)
    goto bad;

  mne_free_name_list(badlist,nbad);
  fwd_free_coil_set(templates);
  fwd_free_coil_set(comp_coils);
  mne_free_ctf_comp_data_set(comp_data);
  return res;

  bad : {
    mne_free_name_list(badlist,nbad);
    fwd_free_coil_set(templates);
    fwd_free_coil_set(comp_coils);
    mne_free_ctf_comp_data_set(comp_data);
    free_dipole_fit_data(res);
    return NULL;
  }
}



int compute_guess_fields(guessData guess,
             dipoleFitData f)
     /*
      * Once the guess locations have been set up we can compute the fields
      */
{
  dipoleFitFuncs orig = NULL;
  int k;

  if (!guess || !f) {
    qCritical("Data missing in compute_guess_fields");
    goto bad;
  }
  if (!f->noise) {
    qCritical("Noise covariance missing in compute_guess_fields");
    goto bad;
  }
  printf("Go through all guess source locations...");
  orig = f->funcs;
  if (f->fit_mag_dipoles)
    f->funcs = f->mag_dipole_funcs;
  else
    f->funcs = f->sphere_funcs;
  for (k = 0; k < guess->nguess; k++) {
    if ((guess->guess_fwd[k] = dipole_forward_one(f,guess->rr[k],guess->guess_fwd[k])) == NULL)
      goto bad;
#ifdef DEBUG
    sing = guess->guess_fwd[k]->sing;
    printf("%f %f %f\n",sing[0],sing[1],sing[2]);
#endif
  }
  f->funcs = orig;
  printf("[done %d sources]\n",guess->nguess);
  return OK;

  bad : {
    if (orig)
      f->funcs = orig;
    return FAIL;
  }
}

guessData make_guess_data(char          *guessname,
              char          *guess_surfname,
              float         mindist,
              float         exclude,
              float         grid,
              dipoleFitData f,
              char          *guess_save_name)

{
  mneSourceSpace *sp = NULL;
  int            nsp = 0;
  guessData      res = NULL;
  int            k,p;
  float          guessrad = 0.080;
  mneSourceSpace guesses = NULL;

  if (guessname) {
    /*
     * Read the guesses and transform to the appropriate coordinate frame
     */
    if (mne_read_source_spaces(guessname,&sp,&nsp) == FAIL)
      goto bad;
    if (nsp != 1) {
      qCritical("Incorrect number of source spaces in guess file");
      for (k = 0; k < nsp; k++)
    mne_free_source_space(sp[k]);
      FREE(sp);
      goto bad;
    }
    printf("Read guesses from %s\n",guessname);
    guesses = sp[0]; FREE(sp);
  }
  else {
    mneSurface     inner_skull = NULL;
    int            free_inner_skull = FALSE;
    float          r0[3];

    VEC_COPY(r0,f->r0);
    fiff_coord_trans_inv(r0,f->mri_head_t,TRUE);
    if (f->bem_model) {
      printf("Using inner skull surface from the BEM (%s)...\n",f->bemname);
      if ((inner_skull = fwd_bem_find_surface(f->bem_model,FIFFV_BEM_SURF_ID_BRAIN)) == NULL)
    goto bad;
    }
    else if (guess_surfname) {
      printf("Reading inner skull surface from %s...\n",guess_surfname);
      if ((inner_skull = mne_read_bem_surface(guess_surfname,FIFFV_BEM_SURF_ID_BRAIN,TRUE,NULL)) == NULL)
    goto bad;
      free_inner_skull = TRUE;
    }
    if ((guesses = make_guesses(inner_skull,guessrad,r0,grid,exclude,mindist)) == NULL)
      goto bad;
    if (free_inner_skull)
      mne_free_source_space(inner_skull);
  }
  /*
   * Save the guesses for future use
   */
  if (guesses->nuse == 0) {
    qCritical("No active guess locations remaining.");
    goto bad;
  }
  if (guess_save_name) {
    if (mne_write_source_spaces(guess_save_name,&guesses,1,FALSE) != OK)
      goto bad;
    printf("Wrote guess locations to %s\n",guess_save_name);
  }
  /*
   * Transform the guess locations to the appropriate coordinate frame
   */
  if (mne_transform_source_spaces_to(f->coord_frame,f->mri_head_t,&guesses,1) != OK)
    goto bad;
  printf("Guess locations are now in %s coordinates.\n",mne_coord_frame_name(f->coord_frame));

  res = new_guess_data();
  res->nguess  = guesses->nuse;
  res->rr      = ALLOC_CMATRIX(guesses->nuse,3);
  for (k = 0, p = 0; k < guesses->np; k++)
    if (guesses->inuse[k]) {
      VEC_COPY(res->rr[p],guesses->rr[k]);
      p++;
    }
  mne_free_source_space(guesses); guesses = NULL;

  res->guess_fwd = MALLOC(res->nguess,dipoleForward);
  for (k = 0; k < res->nguess; k++)
    res->guess_fwd[k] = NULL;
  /*
   * Compute the guesses using the sphere model for speed
   */
  if (compute_guess_fields(res,f) == FAIL)
    goto bad;

  return res;

 bad : {
    mne_free_source_space(guesses);
    free_guess_data(res);
    return NULL;
  }
}





//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#ifndef PROGRAM_VERSION
#define PROGRAM_VERSION     "1.12"
#endif


#ifndef FAIL
#define FAIL -1
#endif

#ifndef OK
#define OK 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif



#ifdef _POSIX_SOURCE
extern char *strdup(const char *);
#endif

#define BIG_TIME 1e6

static char  *bemname     = NULL;		 /* Boundary-element model */
static float r0[]         = { 0.0,0.0,0.04 };    /* Sphere model origin  */
static int   accurate     = FALSE;		 /* Use accurate coil definitions? */
static char  *mriname     = NULL;		 /* Gives the MRI <-> head transform */

static char  *guessname   = NULL;		 /* Initial guess grid (if not present, the values below
                          * will be employed to generate the grid) */
static char  *guess_surfname = NULL;		 /* Load the inner skull surface from this BEM file */
static float guess_mindist = 0.010;		 /* Minimum allowed distance to the surface */
static float guess_exclude = 0.020;		 /* Exclude points closer than this to the origin */
static float guess_grid    = 0.010;		 /* Grid spacing */

static char  *noisename   = NULL;		 /* Noise-covariance matrix */
static float grad_std     = 5e-13;               /* Standard deviations to be used if noise covariance is not specified */
static float mag_std      = 20e-15;
static float eeg_std      = 0.2e-6;
static int   diagnoise    = FALSE;		 /* Use only the diagonals of the noise-covariance matrix */

static char  *measname    = NULL;		 /* Data file */
static int   is_raw       = FALSE;		 /* Is this a raw data file */
static char  *badname     = NULL;		 /* Bad channels */
static int   include_meg  = FALSE;		 /* Use MEG? */
static int   include_eeg  = FALSE;		 /* Use EEG? */
static float tmin         = -2*BIG_TIME;	 /* Possibility to set these from the command line */
static float tmax         = 2*BIG_TIME;
static float tstep        = -1.0;		 /* Step between fits */
static float integ        = 0.0;
static float bmin         = BIG_TIME;	         /* Possibility to set these from the command line */
static float bmax         = BIG_TIME;
static int   do_baseline  = FALSE;	         /* Are both baseline limits set? */
static int   setno        = 1;		         /* Which data set */
static int   verbose      = FALSE;
static mneFilterDefRec filter = { TRUE,		 /* Filter on? */
                  4096,		 /* size */
                  2048,		 /* taper_size */
                  0.0, 0.0,	 /* highpass corner and width */
                  40.0, 5.0,	 /* lowpass corner and width */
                  0.0, 0.0,	 /* EOG highpass corner and width */
                  40.0, 5.0 };	 /* EOG Lowpass corner and width */
static char **projnames   = NULL;                /* Projection file names */
static int  nproj         = 0;
static int  omit_data_proj = FALSE;

static char   *eeg_model_file = NULL;            /* File of EEG sphere model specifications */
static char   *eeg_model_name = NULL;		 /* Name of the EEG model to use */
static float  eeg_sphere_rad = 0.09;		 /* Scalp radius to use in EEG sphere model */
static int    scale_eeg_pos  = FALSE;	         /* Scale the electrode locations to scalp in the sphere model */
static float  mag_reg      = 0.1;                /* Noise-covariance matrix regularization for MEG (magnetometers and axial gradiometers)  */
static int   fit_mag_dipoles = FALSE;

static float  grad_reg     = 0.1;               /* Noise-covariance matrix regularization for EEG (planar gradiometers) */
static float  eeg_reg      = 0.1;               /* Noise-covariance matrix regularization for EEG  */
static char   *dipname     = NULL;		/* Output file in dip format */
static char   *bdipname    = NULL;		/* Output file in bdip format */

static void usage(char *name)

{
  printf("usage: %s [options]\n",name);
  printf("This is a program for sequential single dipole fitting.\n");
  printf("\nInput data:\n\n");
  printf("\t--meas name       specify an evoked-response data file\n");
  printf("\t--set   no        evoked data set number to use (default: 1)\n");
  printf("\t--bad name        take bad channel list from here\n");

  printf("\nModality selection:\n\n");
  printf("\t--meg             employ MEG data in fitting\n");
  printf("\t--eeg             employ EEG data in fitting\n");

  printf("\nTime scale selection:\n\n");
  printf("\t--tmin  time/ms   specify the starting analysis time\n");
  printf("\t--tmax  time/ms   specify the ending analysis time\n");
  printf("\t--tstep time/ms   specify the time step between frames (default 1/(sampling frequency))\n");
  printf("\t--integ time/ms   specify the time integration for each frame (default 0)\n");

  printf("\nPreprocessing:\n\n");
  printf("\t--bmin  time/ms   specify the baseline starting time (evoked data only)\n");
  printf("\t--bmax  time/ms   specify the baseline ending time (evoked data only)\n");
  printf("\t--proj name       Load the linear projection from here\n");
  printf("\t                  Multiple projections can be specified.\n");
  printf("\t                  The data file will be automatically included, unless --noproj is present.\n");
  printf("\t--noproj          Do not load the projection from the data file, just those given with the --proj option.\n");
  printf("\n\tFiltering (raw data only):\n\n");
  printf("\t--filtersize size desired filter length (default = %d)\n",filter.size);
  printf("\t--highpass val/Hz highpass corner (default = %6.1f Hz)\n",filter.highpass);
  printf("\t--lowpass  val/Hz lowpass  corner (default = %6.1f Hz)\n",filter.lowpass);
  printf("\t--lowpassw val/Hz lowpass transition width (default = %6.1f Hz)\n",filter.lowpass_width);
  printf("\t--filteroff       do not filter the data\n");

  printf("\nNoise specification:\n\n");
  printf("\t--noise name      take the noise-covariance matrix from here\n");
  printf("\t--gradnoise val   specify a gradiometer noise value in fT/cm\n");
  printf("\t--magnoise val    specify a gradiometer noise value in fT\n");
  printf("\t--eegnoise val    specify an EEG value in uV\n");
  printf("\t                  NOTE: The above will be used only if --noise is missing\n");
  printf("\t--diagnoise       omit off-diagonal terms from the noise-covariance matrix\n");
  printf("\t--reg amount      Apply regularization to the noise-covariance matrix (same fraction for all channels).\n");
  printf("\t--gradreg amount  Apply regularization to the MEG noise-covariance matrix (planar gradiometers, default = %6.2f).\n",grad_reg);
  printf("\t--magreg amount   Apply regularization to the EEG noise-covariance matrix (axial gradiometers and magnetometers, default = %6.2f).\n",mag_reg);
  printf("\t--eegreg amount   Apply regularization to the EEG noise-covariance matrix (default = %6.2f).\n",eeg_reg);


  printf("\nForward model:\n\n");
  printf("\t--mri name        take head/MRI coordinate transform from here (Neuromag MRI description file)\n");
  printf("\t--bem  name       BEM model name\n");
  printf("\t--origin x:y:z/mm use a sphere model with this origin (head coordinates/mm)\n");
  printf("\t--eegscalp        scale the electrode locations to the surface of the scalp when using a sphere model\n");
  printf("\t--eegmodels name  read EEG sphere model specifications from here.\n");
  printf("\t--eegmodel  name  name of the EEG sphere model to use (default : Default)\n");
  printf("\t--eegrad val      radius of the scalp surface to use in EEG sphere model (default : %7.1f mm)\n",1000*eeg_sphere_rad);
  printf("\t--accurate        use accurate coil definitions in MEG forward computation\n");

  printf("\nFitting parameters:\n\n");
  printf("\t--guess name      The source space of initial guesses.\n");
  printf("\t                  If not present, the values below are used to generate the guess grid.\n");
  printf("\t--gsurf   name    Read the inner skull surface from this fif file to generate the guesses.\n");
  printf("\t--exclude dist/mm Exclude points which are closer than this distance from the CM of the inner skull surface (default =  %6.1f mm).\n",1000*guess_exclude);
  printf("\t--mindist dist/mm Exclude points which are closer than this distance from the inner skull surface  (default = %6.1f mm).\n",1000*guess_mindist);
  printf("\t--grid    dist/mm Source space grid size (default = %6.1f mm).\n",1000*guess_grid);
  printf("\t--magdip          Fit magnetic dipoles instead of current dipoles.\n");
  printf("\nOutput:\n\n");
  printf("\t--dip     name    xfit dip format output file name\n");
  printf("\t--bdip    name    xfit bdip format output file name\n");
  printf("\nGeneral:\n\n");
  printf("\t--help            print this info.\n");
  printf("\t--version         print version info.\n\n");
  return;
}

static int check_unrecognized_args(int argc, char **argv)

{
  int k;

  if (argc > 1) {
    printf("Unrecognized arguments : ");
    for (k = 1; k < argc; k++)
      printf("%s ",argv[k]);
    printf("\n");
    qCritical ("Check the command line.");
    return FAIL;
  }
  return OK;
}

static int check_args (int *argc,char **argv)

{
  int k;
  int p;
  int found;
  float fval;
  int   ival,filter_size;

  for (k = 0; k < *argc; k++) {
    found = 0;
    if (strcmp(argv[k],"--version") == 0) {      
      printf("%s version %s compiled at %s %s\n",
          argv[0],PROGRAM_VERSION,__DATE__,__TIME__);

      exit(0);
    }
    else if (strcmp(argv[k],"--help") == 0) {
      usage(argv[0]);
      exit(1);
    }
    else if (strcmp(argv[k],"--guess") == 0) {
        found = 2;
        if (k == *argc - 1) {
            qCritical ("--guess: argument required.");
            return FAIL;
        }
        guessname = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--gsurf") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--gsurf: argument required.");
    return FAIL;
      }
      guess_surfname = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--mindist") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--mindist: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%f",&fval) != 1) {
    qCritical ("Could not interpret the distance.");
    return FAIL;
      }
      guess_mindist = fval/1000.0;
      if (guess_mindist <= 0.0)
    guess_mindist = 0.0;
    }
    else if (strcmp(argv[k],"--exclude") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--exclude: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%f",&fval) != 1) {
    qCritical ("Could not interpret the distance.");
    return FAIL;
      }
      guess_exclude = fval/1000.0;
      if (guess_exclude <= 0.0)
    guess_exclude = 0.0;
    }
    else if (strcmp(argv[k],"--grid") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--grid: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%f",&fval) != 1) {
    qCritical ("Could not interpret the distance.");
    return FAIL;
      }
      if (fval <= 0.0) {
    qCritical ("Grid spacing should be positive");
    return FAIL;
      }
      guess_grid = guess_grid/1000.0;
    }
    else if (strcmp(argv[k],"--mri") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--mri: argument required.");
    return FAIL;
      }
      mriname = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--bem") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--bem: argument required.");
    return FAIL;
      }
      bemname = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--accurate") == 0) {
      found = 1;
      accurate = TRUE;
    }
    else if (strcmp(argv[k],"--meg") == 0) {
      found = 1;
      include_meg = TRUE;
    }
    else if (strcmp(argv[k],"--eeg") == 0) {
      found = 1;
      include_eeg = TRUE;
    }
    else if (strcmp(argv[k],"--origin") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--origin: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%f:%f:%f",r0+X,r0+Y,r0+Z) != 3) {
    qCritical ("Could not interpret the origin.");
    return FAIL;
      }
      r0[X] = r0[X]/1000.0;
      r0[Y] = r0[Y]/1000.0;
      r0[Z] = r0[Z]/1000.0;
    }
    else if (strcmp(argv[k],"--eegrad") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--eegrad: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&eeg_sphere_rad) != 1) {
    qCritical () << "Incomprehensible radius:" << argv[k+1];
    return FAIL;
      }
      if (eeg_sphere_rad <= 0) {
    qCritical ("Radius must be positive");
    return FAIL;
      }
      eeg_sphere_rad = eeg_sphere_rad/1000.0;
    }
    else if (strcmp(argv[k],"--eegmodels") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--eegmodels: argument required.");
    return FAIL;
      }
      eeg_model_file = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--eegmodel") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--eegmodel: argument required.");
    return FAIL;
      }
      eeg_model_name = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--eegscalp") == 0) {
      found         = 1;
      scale_eeg_pos = TRUE;
    }
    else if (strcmp(argv[k],"--meas") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--meas: argument required.");
    return FAIL;
      }
      measname = strdup(argv[k+1]);
      is_raw = FALSE;
    }
    else if (strcmp(argv[k],"--raw") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--raw: argument required.");
    return FAIL;
      }
      measname = strdup(argv[k+1]);
      is_raw = TRUE;
    }
    else if (strcmp(argv[k],"--proj") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--proj: argument required.");
    return FAIL;
      }
      projnames = REALLOC(projnames,nproj+1,char *);
      projnames[nproj++] = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--noproj") == 0) {
      found = 1;
      omit_data_proj = TRUE;
    }
    else if (strcmp(argv[k],"--bad") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--bad: argument required.");
    return FAIL;
      }
      badname = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--noise") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--noise: argument required.");
    return FAIL;
      }
      noisename = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--gradnoise") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--gradnoise: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Incomprehensible value:" << argv[k+1];
    return FAIL;
      }
      if (fval < 0.0) {
    qCritical ("Value should be positive");
    return FAIL;
      }
      grad_std = 1e-13*fval;
    }
    else if (strcmp(argv[k],"--magnoise") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--magnoise: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Incomprehensible value:" << argv[k+1];
    return FAIL;
      }
      if (fval < 0.0) {
    qCritical ("Value should be positive");
    return FAIL;
      }
      mag_std = 1e-15*fval;
    }
    else if (strcmp(argv[k],"--eegnoise") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--eegnoise: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical () << "Incomprehensible value:" << argv[k+1];
    return FAIL;
      }
      if (fval < 0.0) {
    qCritical ("Value should be positive");
    return FAIL;
      }
      eeg_std = 1e-6*fval;
    }
    else if (strcmp(argv[k],"--diagnoise") == 0) {
      found = 1;
      diagnoise = TRUE;
    }
    else if (strcmp(argv[k],"--eegreg") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--eegreg: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical () << "Incomprehensible value:" << argv[k+1];
    return FAIL;
      }
      if (fval < 0 || fval > 1) {
    qCritical ("Regularization value should be positive and smaller than one.");
    return FAIL;
      }
      eeg_reg = fval;
    }
    else if (strcmp(argv[k],"--magreg") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--magreg: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical () << "Incomprehensible value:" << argv[k+1];
    return FAIL;
      }
      if (fval < 0 || fval > 1) {
    qCritical ("Regularization value should be positive and smaller than one.");
    return FAIL;
      }
      mag_reg = fval;
    }
    else if (strcmp(argv[k],"--gradreg") == 0) {
        found = 2;
        if (k == *argc - 1) {
            qCritical ("--gradreg: argument required.");
            return FAIL;
        }
        if (sscanf(argv[k+1],"%g",&fval) != 1) {
            qCritical () << "Incomprehensible value:" << argv[k+1] ;
            return FAIL;
        }
        if (fval < 0 || fval > 1) {
            qCritical ("Regularization value should be positive and smaller than one.");
            return FAIL;
        }
        grad_reg = fval;
    }
    else if (strcmp(argv[k],"--reg") == 0) {
        found = 2;
        if (k == *argc - 1) {
            qCritical ("--reg: argument required.");
            return FAIL;
        }
        if (sscanf(argv[k+1],"%g",&fval) != 1) {
            qCritical () << "Incomprehensible value:" << argv[k+1];
            return FAIL;
        }
        if (fval < 0 || fval > 1) {
            qCritical ("Regularization value should be positive and smaller than one.");
            return FAIL;
        }
        grad_reg = fval;
        mag_reg = fval;
        eeg_reg = fval;
    }
    else if (strcmp(argv[k],"--tstep") == 0) {
        found = 2;
        if (k == *argc - 1) {
            qCritical ("--tstep: argument required.");
            return FAIL;
        }
        if (sscanf(argv[k+1],"%g",&fval) != 1) {
            qCritical() << "Incomprehensible tstep:" << argv[k+1];
            return FAIL;
        }
      if (fval < 0.0) {
    qCritical ("Time step should be positive");
    return FAIL;
      }
      tstep = fval/1000.0;
    }
    else if (strcmp(argv[k],"--integ") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--integ: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Incomprehensible integration time:" << argv[k+1];
    return FAIL;
      }
      if (fval <= 0.0) {
    qCritical ("Integration time should be positive.");
    return FAIL;
      }
      integ = fval/1000.0;
    }
    else if (strcmp(argv[k],"--tmin") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--tmin: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Incomprehensible tmin:" << argv[k+1];
    return FAIL;
      }
      tmin = fval/1000.0;
    }
    else if (strcmp(argv[k],"--tmax") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--tmax: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Incomprehensible tmax:" << argv[k+1];
    return FAIL;
      }
      tmax = fval/1000.0;
    }
    else if (strcmp(argv[k],"--bmin") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--bmin: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Incomprehensible bmin:" << argv[k+1];
    return FAIL;
      }
      bmin = fval/1000.0;
    }
    else if (strcmp(argv[k],"--bmax") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--bmax: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Incomprehensible bmax:" << argv[k+1];
    return FAIL;
      }
      bmax = fval/1000.0;
    }
    else if (strcmp(argv[k],"--set") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--set: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%d",&setno) != 1) {
    qCritical() << "Incomprehensible data set number:" << argv[k+1];
    return FAIL;
      }
      if (setno <= 0) {
    qCritical ("Data set number must be > 0");
    return FAIL;
      }
    }
    else if (strcmp(argv[k],"--filteroff") == 0) {
      found = 1;
      filter.filter_on = FALSE;
    }
    else if (strcmp(argv[k],"--lowpass") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--lowpass: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Illegal number:" << argv[k+1];
    return FAIL;
      }
      if (fval <= 0) {
    qCritical ("Lowpass corner must be positive");
    return FAIL;
      }
      filter.lowpass = fval;
    }
    else if (strcmp(argv[k],"--lowpassw") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--lowpassw: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Illegal number:" << argv[k+1];
    return FAIL;
      }
      if (fval <= 0) {
    qCritical ("Lowpass width must be positive");
    return FAIL;
      }
      filter.lowpass_width = fval;
    }
    else if (strcmp(argv[k],"--highpass") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--highpass: argument required.");
    return FAIL;
      }
      if (sscanf(argv[k+1],"%g",&fval) != 1) {
    qCritical() << "Illegal number:" << argv[k+1];
    return FAIL;
      }
        if (fval <= 0) {
            qCritical ("Highpass corner must be positive");
            return FAIL;
        }
        filter.highpass = fval;
    }
    else if (strcmp(argv[k],"--filtersize") == 0) {
        found = 2;
    if (k == *argc - 1) {
        qCritical ("--filtersize: argument required.");
        return FAIL;
    }
    if (sscanf(argv[k+1],"%d",&ival) != 1) {
        qCritical() << "Illegal number:" << argv[k+1];
        return FAIL;
    }
    if (ival < 1024) {
        qCritical ("Filtersize should be at least 1024.");
        return FAIL;
    }
    for (filter_size = 1024; filter_size < ival; filter_size = 2*filter_size)
    ;
        filter.size       = filter_size;
        filter.taper_size = filter_size/2;
    }
    else if (strcmp(argv[k],"--magdip") == 0) {
        found = 1;
        fit_mag_dipoles = TRUE;
    }
    else if (strcmp(argv[k],"--dip") == 0) {
        found = 2;
        if (k == *argc - 1) {
            qCritical ("--dip: argument required.");
            return FAIL;
    }
    dipname = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--bdip") == 0) {
      found = 2;
      if (k == *argc - 1) {
    qCritical ("--bdip: argument required.");
    return FAIL;
      }
      bdipname = strdup(argv[k+1]);
    }
    else if (strcmp(argv[k],"--verbose") == 0) {
      found = 1;
      verbose = TRUE;
    }
    if (found) {
      for (p = k; p < *argc-found; p++)
    argv[p] = argv[p+found];
      *argc = *argc - found;
      k = k - found;
    }
  }
  return check_unrecognized_args(*argc,argv);
}












//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    int            res      = FAIL;
    dipoleFitData  fit_data = NULL;
    mneMeasData    data     = NULL;
    mneRawData     raw      = NULL;
    guessData      guess    = NULL;
    mneChSelection sel      = NULL;
    ecdSet         set      = NULL;

    fwdEegSphereModel    eeg_model = NULL;
    int                  k;

    if (check_args(&argc,argv) == FAIL)
        goto out;

    do_baseline = (bmin < BIG_TIME && bmax < BIG_TIME);

    if (!measname) {
        qCritical ("Data file name missing. Please specify one using the --meas option.");
        goto out;
    }
    if (!dipname && !bdipname) {
        qCritical ("Output file name missing. Please use the --dip or --bdip options to do this.");
        goto out;
    }
    if (!guessname) {
        if (!bemname && guess_surfname && !mriname) {
            qCritical ("Please specify the MRI/head coordinate transformation with the --mri option");
            goto out;
        }
    }
    if (!include_meg && !include_eeg) {
        qCritical ("Specify one or both of the --eeg and --meg options");
        goto out;
    }
    if (!omit_data_proj) {
        projnames = REALLOC(projnames,nproj+1,char *);
        nproj++;
        for (k = 1; k < nproj; k++)
            projnames[k] = projnames[k-1];
        projnames[0] = mne_strdup(measname);
    }

//    mne_print_version_info(stderr,argv[0],PROGRAM_VERSION,__DATE__,__TIME__);
    printf("%s version %s\n",argv[0],PROGRAM_VERSION);//,__DATE__,__TIME__);

    if (bemname)
        printf("BEM              : %s\n",bemname);
    else {
        printf("Sphere model     : origin at (% 7.2f % 7.2f % 7.2f) mm\n",
        1000*r0[X],1000*r0[Y],1000*r0[Z]);
    }
    printf("Using %s MEG coil definitions.\n",accurate ? "accurate" : "standard");
    if (mriname)
        printf("MRI transform    : %s\n",mriname);
    if (guessname)
        printf("Guesses          : %s\n",guessname);
    else {
        printf("Guess grid       : %6.1f mm\n",1000*guess_grid);
        if (guess_mindist > 0.0)
            printf("Guess mindist    : %6.1f mm\n",1000*guess_mindist);
        if (guess_exclude > 0)
            printf("Guess exclude    : %6.1f mm\n",1000*guess_exclude);
    }
    printf("Data             : %s\n",measname);
    if (nproj > 0) {
        printf("SSP sources      :\n");
    for (k = 0; k < nproj; k++)
        printf("\t%s\n",projnames[k]);
    }
    if (badname)
        printf("Bad channels     : %s\n",badname);
    if (do_baseline)
        printf("Baseline         : %10.2f ... %10.2f ms\n",
    1000*bmin,1000*bmax);
    if (noisename) {
        printf("Noise covariance : %s\n",noisename);
        if (include_meg) {
            if (mag_reg > 0.0)
                printf("\tNoise-covariange regularization (mag)     : %-5.2f\n",mag_reg);
            if (grad_reg > 0.0)
                printf("\tNoise-covariange regularization (grad)    : %-5.2f\n",grad_reg);
        }
        if (include_eeg && eeg_reg > 0.0)
            printf("\tNoise-covariange regularization (EEG)     : %-5.2f\n",eeg_reg);
    }
    if (fit_mag_dipoles)
        printf("Fit data with magnetic dipoles\n");
    if (dipname)
        printf("dip output      : %s\n",dipname);
    if (bdipname)
        printf("bdip output     : %s\n",bdipname);
    printf("\n");
    printf("---- Setting up...\n\n");

    if (include_eeg) {
        if ((eeg_model = setup_eeg_sphere_model(eeg_model_file,eeg_model_name,eeg_sphere_rad)) == NULL)
            goto out;
    }
    if ((fit_data = setup_dipole_fit_data(mriname,measname,bemname,r0,eeg_model,accurate,
        badname,noisename,grad_std,mag_std,eeg_std,
        mag_reg,grad_reg,eeg_reg,
        diagnoise,projnames,nproj,include_meg,include_eeg)) == NULL)
        goto out;
    fit_data->fit_mag_dipoles = fit_mag_dipoles;
    if (is_raw) {
        int c;
        float t1,t2;

        printf("\n---- Opening a raw data file...\n\n");
        if ((raw = mne_raw_open_file(measname,TRUE,FALSE,&filter)) == NULL)
            goto out;
        /*
        * A channel selection is needed to access the data
        */
        sel = mne_ch_selection_these("fit",fit_data->ch_names,fit_data->nmeg+fit_data->neeg);
        mne_ch_selection_assign_chs(sel,raw);
        for (c = 0; c < sel->nchan; c++)
            if (sel->pick[c] < 0) {
                qCritical ("All desired channels were not available");
                goto out;
            }
            printf("\tChannel selection created.\n");
            /*
            * Let's be a little generous here
            */
            t1 = raw->first_samp/raw->info->sfreq;
            t2 = (raw->first_samp+raw->nsamp-1)/raw->info->sfreq;
            if (tmin < t1 + integ)
            tmin = t1 + integ;
            if (tmax > t2 - integ)
            tmax =  t2 - integ;
            if (tstep < 0)
            tstep = 1.0/raw->info->sfreq;

            printf("\tOpened raw data file %s : %d MEG and %d EEG \n",
            measname,fit_data->nmeg,fit_data->neeg);
        }
    else {
        printf("\n---- Reading data...\n\n");
        if ((data = mne_read_meas_data(measname,setno,NULL,NULL,
            fit_data->ch_names,fit_data->nmeg+fit_data->neeg)) == NULL)
            goto out;
        if (do_baseline)
            mne_adjust_baselines(data,bmin,bmax);
        else
            printf("\tNo baseline setting in effect.\n");
        if (tmin < data->current->tmin + integ/2.0)
            tmin = data->current->tmin + integ/2.0;
        if (tmax > data->current->tmin + (data->current->np-1)*data->current->tstep - integ/2.0)
            tmax =  data->current->tmin + (data->current->np-1)*data->current->tstep - integ/2.0;
        if (tstep < 0)
            tstep = data->current->tstep;

        printf("\tRead data set %d from %s : %d MEG and %d EEG \n",
        setno,measname,fit_data->nmeg,fit_data->neeg);
        if (noisename) {
            printf("\nScaling the noise covariance...\n");
            if (scale_noise_cov(fit_data,data->current->nave) == FAIL)
                goto out;
        }
    }
    /*
    * Proceed to computing the fits
    */
    printf("\n---- Computing the forward solution for the guesses...\n\n");
    if ((guess = make_guess_data(guessname,
    guess_surfname,
    guess_mindist,guess_exclude,guess_grid,fit_data)) == NULL)
    goto out;
    fprintf (stderr,"\n---- Fitting : %7.1f ... %7.1f ms (step: %6.1f ms integ: %6.1f ms)\n\n",
    1000*tmin,1000*tmax,1000*tstep,1000*integ);
    if (raw) {
        if (fit_dipoles_raw(measname,raw,sel,fit_data,guess,tmin,tmax,tstep,integ,verbose,NULL) == FAIL)
            goto out;
    }
    else {
        if (fit_dipoles(measname,data,fit_data,guess,tmin,tmax,tstep,integ,verbose,&set) == FAIL)
            goto out;
    }
    printf("%d dipoles fitted\n",set->ndip);
    /*
    * Saving...
    */
    if (save_dipoles_dip(dipname,set) == FAIL)
    goto out;
    if (save_dipoles_bdip(bdipname,set) == FAIL)
    goto out;
    free_ecd_set(set);
    res = OK;

    out : {
        if (res == FAIL) {
//            err_print_error();
            exit(1);
        }
        else
            exit (0);
    }


//    return app.exec();
}

//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
