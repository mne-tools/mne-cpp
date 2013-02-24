//=============================================================================================================
/**
* @file     fiffparser.cpp
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffparser.h"

#include "fiff_file.h"

#include "dot.h"

#include "allocs.h"

#include <stdio.h>
#include <stdlib.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QByteArray>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFF;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

fiffParser::fiffParser()
{

}


//*************************************************************************************************************

fiffParser::~fiffParser()
{
    //ToDo cleanup work
}


//*************************************************************************************************************

//int fiff_read_tag (FILE  *in,
//		   fiffTag tag)	/* Note: data member must be initialized
//				 * to NULL on first call.
//				 * This routine automatically reallocs
//				 * the needed space */
//     /*
//      * Read next tag including its data from file
//      */
//{


int fiffParser::fiff_read_tag (QByteArray* in,
			fiffTag* tag)	/* Note: data member must be initialized
				 * to NULL on first call.
				 * This routine automatically reallocs
				 * the needed space */
     /*
      * Read next tag including its data from file
      */
{
//    long pos = ftell(in);


//    if (fread (tag,FIFFC_TAG_INFO_SIZE,1,in) != 1) {
//        err_printf_set_error("Failed to read tag info (pos = %d)",pos);
//        return (-1);
//    }
    memcpy ( tag, in->data(), FIFFC_TAG_INFO_SIZE);//ToDo need to delete copied data?
    in->remove(0, FIFFC_TAG_INFO_SIZE);

    fiff_convert_tag_info(tag);

    if (tag->size > 0) {
        if (tag->data == NULL)
            tag->data = malloc(tag->size + ((tag->type == FIFFT_STRING) ? 1 : 0));
        else
            tag->data = realloc(tag->data,tag->size + ((tag->type == FIFFT_STRING) ? 1 : 0));

        if (tag->data == NULL) {
            //err_set_error("fiff_read_tag: memory allocation failed.");
            qDebug() << "fiff_read_tag: memory allocation failed.";
            return -1;
        }

//        //ToDo Maybe a need to delete data before copy
//        if (fread (tag->data,tag->size,1,in) != 1) {
//            err_printf_set_error("Failed to read tag data (pos = %d kind = %d size = %d)",pos,tag->kind,tag->size);
//            return(-1);
//        }
        memcpy ( tag->data, in->data(), tag->size);//ToDo need to delete copied data?
        in->remove(0, tag->size);

        if (tag->type == FIFFT_STRING)  /* Null-terminated strings */
            ((char *)tag->data)[tag->size] = '\0';
        else if (tag->type == FIFFT_CH_INFO_STRUCT)
            fix_ch_info (tag);
    }

    if (tag->next > 0)
    {
        qDebug() << "Warning: Next tag is not next!!!";

//        if (fseek(in,tag->next,SEEK_SET) == -1) {
//            err_set_sys_error ("fseek");
//            pos = -1;
//        }
    }
    fiff_convert_tag_data(tag,FIFFV_BIG_ENDIAN,FIFFV_NATIVE_ENDIAN);
    return 0;
}


//*************************************************************************************************************

void fiffParser::convert_loc (float oldloc[9], /*!< These are the old magic numbers */
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


//*************************************************************************************************************

void fiffParser::fix_ch_info (fiffTag* tag)
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
            tag->data = help;
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

                #ifdef _WIN32
                    sprintf_s(ch->ch_name,"MEG %03d",ch->logNo % 1000);
                #elif defined _WIN64
                    sprintf_s(ch->ch_name,"MEG %03d",ch->logNo % 1000);
                #else
                    sprintf(ch->ch_name,"MEG %03d",ch->logNo % 1000);
                #endif

                ch->unit = FIFF_UNIT_T_M;
            }
            else if (ch->kind == FIFFV_EL_CH) {
                pos->coil_type = FIFFV_COIL_EEG;
                pos->r0[X] = old->loc[X];
                pos->r0[Y] = old->loc[Y];
                pos->r0[Z] = old->loc[Z];
                #ifdef _WIN32
                    sprintf_s(ch->ch_name,"EEG %03d",ch->logNo);
                #elif defined _WIN64
                    sprintf_s(ch->ch_name,"EEG %03d",ch->logNo);
                #else
                    sprintf(ch->ch_name,"EEG %03d",ch->logNo);
                #endif
                ch->unit = FIFF_UNIT_V;
            }
            else {
                pos->coil_type = FIFFV_COIL_NONE;
                #ifdef _WIN32
                    sprintf_s(ch->ch_name,"STI %03d",ch->logNo);
                #elif defined _WIN64
                    sprintf_s(ch->ch_name,"STI %03d",ch->logNo);
                #else
                    sprintf(ch->ch_name,"STI %03d",ch->logNo);
                #endif
                ch->unit = FIFF_UNIT_V;
            }
            FREE(old);
            ch->unit_mul = FIFF_UNITM_NONE;
        }
    }
}


//*************************************************************************************************************
//fiff_combat
short fiffParser::swap_short (fiff_short_t source)

{
    unsigned char *csource = (unsigned char *)(&source);
    fiff_short_t result;
    unsigned char *cresult =  (unsigned char *)(&result);

    cresult[0] = csource[1];
    cresult[1] = csource[0];
    return (result);
}


//*************************************************************************************************************

fiff_int_t fiffParser::swap_int (fiff_int_t source)
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


//*************************************************************************************************************

fiff_long_t fiffParser::swap_long (fiff_long_t source)

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


//*************************************************************************************************************

void fiffParser::swap_longp (fiff_long_t *source)

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


//*************************************************************************************************************

void fiffParser::swap_intp (fiff_int_t *source)

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


//*************************************************************************************************************

void fiffParser::swap_floatp (float *source)

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


//*************************************************************************************************************

void fiffParser::swap_doublep(double *source)

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


//*************************************************************************************************************

/*---------------------------------------------------------------------------
 *
 * Motorola like Architectures
 *
 * Fif file bit and byte orderings are defined to be as in HP-UX.
 *
 *--------------------------------------------------------------------------*/

#ifdef BIG_ENDIAN_ARCH


void fiffParser::fiff_convert_tag_info(fiffTag* tag)
{
    return;
}

#endif

#ifdef INTEL_X86_ARCH
/*! Machine dependent data type conversions (tag info only)
 */

void fiffParser::fiff_convert_tag_info(fiffTag* tag)

{
    tag->kind = swap_int(tag->kind);
    tag->type = swap_int(tag->type);
    tag->size = swap_int(tag->size);
    tag->next = swap_int(tag->next);
    return;
}

#endif /* INTEL_X86_ARCH */


//*************************************************************************************************************

void fiffParser::convert_ch_pos(fiffChPos pos)

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


//*************************************************************************************************************

void fiffParser::convert_matrix_from_file_data(fiffTag* tag)
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


//*************************************************************************************************************

void fiffParser::convert_matrix_to_file_data(fiffTag* tag)
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





//*************************************************************************************************************

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

void fiffParser::fiff_convert_tag_data(fiffTag* tag, int from_endian, int to_endian)

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
    fiffChInfoRec* chthis;//fiffChInfo     chthis;
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
        for (chthis = (fiffChInfoRec*)tag->data, k = 0; k < np; k++, chthis++) {
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


//*************************************************************************************************************
//fiff_type_spec
/*
 * These return information about a fiff type.
 */

fiff_int_t fiffParser::fiff_type_fundamental(fiff_int_t type)
{
    return type & FIFFTS_FS_MASK;
}


//*************************************************************************************************************

fiff_int_t fiffParser::fiff_type_base(fiff_int_t type)
{
    return type & FIFFTS_BASE_MASK;
}


//*************************************************************************************************************

fiff_int_t fiffParser::fiff_type_matrix_coding(fiff_int_t type)
{
    return type & FIFFTS_MC_MASK;
}
