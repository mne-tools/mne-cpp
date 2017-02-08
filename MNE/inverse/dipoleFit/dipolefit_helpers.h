#ifndef DIPOLEFITHELPERS_H
#define DIPOLEFITHELPERS_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ecd_set.h"
#include "mne_sss_data.h"
#include "mne_meas_data_set.h"
#include "mne_deriv.h"
#include "mne_deriv_set.h"
#include "mne_surface_or_volume.h"
#include <iostream>
#include <vector>

#define _USE_MATH_DEFINES
#include <math.h>

#include <utils/sphere.h>

#include <fiff/fiff_constants.h>
#include <fiff/fiff_file.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_types.h>

#include <fiff/fiff_dir_node.h>


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <time.h>

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <unsupported/Eigen/FFT>


//ToDo don't use access and unlink -> use Qt stuff instead
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <io.h>
#else
#include <unistd.h>
#endif


#if defined(_WIN32) || defined(_WIN64)
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif


#include "fwd_eeg_sphere_model_set.h"
#include "guess_data.h"
#include "dipole_fit_data.h"
#include "guess_data.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace INVERSELIB;
using namespace FIFFLIB;

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


/* NOTE:
   The architecture is now deduced from the operating system, which is
   a bit stupid way, since the same operating system can me run on various
   architectures. This may need revision later on. */

#if defined(DARWIN)

#if defined(__LITTLE_ENDIAN__)
#define INTEL_X86_ARCH
#else
#define BIG_ENDIAN_ARCH
#endif

#else

#if defined(__hpux) || defined(__Lynx__) || defined(__sun)
#define BIG_ENDIAN_ARCH
#else
#if defined(__linux) || defined(WIN32) || defined(__APPLE__)
#define INTEL_X86_ARCH
#endif

#endif
#endif

#ifdef  INTEL_X86_ARCH
#define NATIVE_ENDIAN    FIFFV_LITTLE_ENDIAN
#endif

#ifdef  BIG_ENDIAN_ARCH
#define NATIVE_ENDIAN    FIFFV_BIG_ENDIAN
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


//============================= allocs.h =============================

/*
 * integer matrices
 */
#define ALLOC_ICMATRIX(x,y) mne_imatrix((x),(y))
#define FREE_ICMATRIX(m) mne_free_icmatrix((m))
#define ICMATRIX ALLOC_ICMATRIX


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

//============================= mne_allocs.c =============================


#include <fiff/fiff_types.h>
#include "mne_types.h"
#include "analyze_types.h"







//============================= mne_filename_util.c =============================


//char *mne_compose_mne_name(const char *path, const char *filename)
///*
//      * Compose a filename under the "$MNE_ROOT" directory
//      */
//{
//    char *res;
//    char *mne_root;

//    if (filename == NULL) {
//        qCritical("No file name specified to mne_compose_mne_name");
//        return NULL;
//    }
//    mne_root = getenv(MNE_ENV_ROOT);
//    if (mne_root == NULL || strlen(mne_root) == 0) {
//        qCritical("Environment variable MNE_ROOT not set");
//        return NULL;
//    }
//    if (path == NULL || strlen(path) == 0) {
//        res = MALLOC(strlen(mne_root)+strlen(filename)+2,char);
//        strcpy(res,mne_root);
//        strcat(res,"/");
//        strcat(res,filename);
//    }
//    else {
//        res = MALLOC(strlen(mne_root)+strlen(filename)+strlen(path)+3,char);
//        strcpy(res,mne_root);
//        strcat(res,"/");
//        strcat(res,path);
//        strcat(res,"/");
//        strcat(res,filename);
//    }
//    return res;
//}





//============================= mne_sparse_matop.c =============================






//============================= mne_named_matrix.c =============================

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
        fprintf(stderr,"%d %s\n",k,list[k]);
#endif
        FREE(list[k]);
    }
    FREE(list);
    return;
}


/*
 * Handle matrices whose rows and/or columns are named with a list
 */







#endif // DIPOLEFITHELPERS_H
