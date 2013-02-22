/*
 * Copyright 1992
 *
 * Matti Hamalainen
 * Neuromag, Ltd.
 * Espoo, Finland
 *
 * No part of this program may be photocopied, reproduced,
 * or translated to another program language without the
 * prior written consent of the author.
 *
 * $Id: dot.h 2647 2009-05-02 22:52:15Z msh $ 
 *
 * Revision 1.1  2005/04/03 18:15:37  msh
 * Added allocs.h and dot.h so that the include directory is not needed for compilation
 *
 * Revision 2.2  2002/05/17 15:47:09  msh
 * Eliminated macros DOT, DOTP, and LEN
 *
 * Revision 2.1  1996/11/08 17:01:09  msh
 * Check in for software release 2.1
 *
 * 
 * General-purpose 3-vector arithmetics
 *
 */
#ifndef _dot_h
#define _dot_h
#include <math.h>
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
#endif

