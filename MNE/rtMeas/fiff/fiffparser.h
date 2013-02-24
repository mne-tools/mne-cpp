//=============================================================================================================
/**
* @file     fiffparser.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    ToDo...
*
*/

#ifndef FIFF_PARSER_H
#define FIFF_PARSER_H

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
#if defined(__linux) || defined(WIN32)
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


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../rtmeas_global.h"
#include "fiff_types.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QByteArray;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FIFF
//=============================================================================================================

namespace FIFF
{


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


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS fiffParser
*
* @brief The fiffParser class provides parser methods.
*/
class RTMEASSHARED_EXPORT fiffParser
{

public:

    //=========================================================================================================
    /**
    * Constructs a fiff parser..
    */
	fiffParser();

    //=========================================================================================================
    /**
    * Destroys the ServerThread.
    */
    ~fiffParser();

    //from fiff_io.c
    static int fiff_read_tag(QByteArray* in, fiffTag* tag);

    static void fix_ch_info (fiffTag* tag);

    static void convert_loc (float oldloc[9], /*!< These are the old magic numbers */
                             float r0[3],     /*!< Coil coordinate system origin */
                             float *ex,       /*!< Coil coordinate system unit x-vector */
                             float *ey,       /*!< Coil coordinate system unit y-vector */
                             float *ez);       /*!< Coil coordinate system unit z-vector */
         /*
          * Convert the traditional location
          * information to new format...
          */


    //from fiff_combat.c

    static short swap_short (fiff_short_t source);

    static fiff_int_t swap_int (fiff_int_t source);

    static fiff_long_t swap_long (fiff_long_t source);

    static void swap_longp (fiff_long_t *source);

    static void swap_intp (fiff_int_t *source);

    static void swap_floatp (float *source);

    static void swap_doublep(double *source);

    static void convert_ch_pos(fiffChPos pos);

    static void fiff_convert_tag_info(fiffTag* tag);

    static void convert_matrix_from_file_data(fiffTag* tag);

    static void convert_matrix_to_file_data(fiffTag* tag);

    static void fiff_convert_tag_data(fiffTag* tag, int from_endian, int to_endian);


    //from fiff_type_spec.c

    static fiff_int_t fiff_type_fundamental(fiff_int_t type);

    static fiff_int_t fiff_type_base(fiff_int_t type);

    static fiff_int_t fiff_type_matrix_coding(fiff_int_t type);


};

}//NAMESPACE


#endif // FIFF_PARSER_H
