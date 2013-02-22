#ifndef _allocs_h
#define _allocs_h
#ifndef NULL
#define NULL 0
#endif
#include <stdlib.h>

extern float **meg_cmatrix(int nr, int nc);
extern double **meg_dmatrix(int nr, int nc);
extern int **meg_imatrix(int nr, int nc);
extern void meg_free_cmatrix(float **m);
extern void meg_free_icmatrix(int **m);
extern void meg_free_dcmatrix(double **m);

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
#define ALLOC_CMATRIX(x,y) meg_cmatrix((x),(y))
#define FREE_CMATRIX(m) meg_free_cmatrix((m))
#define CMATRIX ALLOC_CMATRIX
/*
 * integer matrices
 */
#define ALLOC_ICMATRIX(x,y) meg_imatrix((x),(y))
#define FREE_ICMATRIX(m) meg_free_icmatrix((m))
#define ICMATRIX ALLOC_ICMATRIX
/*
 * double matrices
 */
#define ALLOC_DCMATRIX(x,y) meg_dmatrix((x),(y))
#define FREE_DCMATRIX(m) meg_free_dcmatrix((m))
/*
 * vectors of pointers
 */
#define ALLOC_FLOATP(x) MALLOC(x,float *)
#define ALLOC_DOUBLEP(x) MALLOC(x,double *)
/*
 * Resolve handle with check
 */
#define RESOLVE_HANDLE(x) (*(x) == NULL ? NULL : *x)
#endif

