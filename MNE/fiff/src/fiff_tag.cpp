//=============================================================================================================
/**
* @file     fiff_tag.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the FiffTag Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../include/fiff_tag.h"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffTag::FiffTag()
: data(NULL)
, m_pComplexFloatData(NULL)
, m_pComplexDoubleData(NULL)
{

}


//*************************************************************************************************************

FiffTag::~FiffTag()
{
    if(this->data)
        free(this->data);
    if(this->m_pComplexFloatData)
        delete this->m_pComplexFloatData;
    if(this->m_pComplexDoubleData)
        delete this->m_pComplexDoubleData;
}


//*************************************************************************************************************

bool FiffTag::read_tag_info(QFile* p_pFile, FiffTag*& p_pTag)
{
    QDataStream t_DataStream(p_pFile);

    if (p_pTag != NULL)
        delete p_pTag;
    p_pTag = new FiffTag();

    //Option 1
//    t_DataStream.readRawData((char *)p_pTag, FIFFC_TAG_INFO_SIZE);
//    p_pTag->kind = Fiff::swap_int(p_pTag->kind);
//    p_pTag->type = Fiff::swap_int(p_pTag->type);
//    p_pTag->size = Fiff::swap_int(p_pTag->size);
//    p_pTag->next = Fiff::swap_int(p_pTag->next);

    //Option 2
    t_DataStream >> p_pTag->kind;
    t_DataStream >> p_pTag->type;
    t_DataStream >> p_pTag->size;
    t_DataStream >> p_pTag->next;

//    qDebug() << "read_tag_info" << "  Kind:" << p_pTag->kind << "  Type:" << p_pTag->type << "  Size:" << p_pTag->size << "  Next:" << p_pTag->next;

    if (p_pTag->next == FIFFV_NEXT_SEQ)
    {
        p_pFile->seek(p_pFile->pos()+p_pTag->size); //fseek(fid,tag.size,'cof');
    }
    else if (p_pTag->next > 0)
    {
        p_pFile->seek(p_pTag->next); //fseek(fid,tag.next,'bof');
    }

    return true;
}


//*************************************************************************************************************

bool FiffTag::read_tag(QFile* p_pFile, FiffTag*& p_pTag, qint64 pos)
{
    if (pos > 0)
    {
        p_pFile->seek(pos);
    }

    QDataStream t_DataStream(p_pFile);

    if (p_pTag != NULL)
        delete p_pTag;
    p_pTag = new FiffTag();

    //Option 2
    t_DataStream >> p_pTag->kind;
    t_DataStream >> p_pTag->type;
    t_DataStream >> p_pTag->size;
    t_DataStream >> p_pTag->next;

//    qDebug() << "read_tag" << "  Kind:" << p_pTag->kind << "  Type:" << p_pTag->type << "  Size:" << p_pTag->size << "  Next:" << p_pTag->next;

    //
    if (p_pTag->size > 0)
    {
        if (p_pTag->data == NULL)
            p_pTag->data = malloc(p_pTag->size + ((p_pTag->type == FIFFT_STRING) ? 1 : 0));
        else
            p_pTag->data = realloc(p_pTag->data,p_pTag->size + ((p_pTag->type == FIFFT_STRING) ? 1 : 0));

        if (p_pTag->data == NULL) {
            printf("fiff_read_tag: memory allocation failed.\n");//consider throw
            delete p_pTag;
            p_pTag = NULL;
            return false;
        }
        char *t_pCharData = static_cast< char* >(p_pTag->data);
        t_DataStream.readRawData(t_pCharData, p_pTag->size);
        if (p_pTag->type == FIFFT_STRING)
            t_pCharData[p_pTag->size] = NULL;//make sure that char ends with NULL
        FiffTag::fiff_convert_tag_data(p_pTag,FIFFV_BIG_ENDIAN,FIFFV_NATIVE_ENDIAN);
    }

    if (p_pTag->next != FIFFV_NEXT_SEQ)
        p_pFile->seek(p_pTag->next);//fseek(fid,tag.next,'bof');

    return true;
}


//*************************************************************************************************************

fiff_int_t FiffTag::getMatrixCoding() const
{
   return IS_MATRIX & this->type;
}


//*************************************************************************************************************

bool FiffTag::isMatrix() const
{
    if (this->getMatrixCoding() != 0)
        return true;
    else
        return false;
}


//*************************************************************************************************************

bool FiffTag::getMatrixDimensions(qint32& p_ndim, qint32*& p_pDims) const
{
    if(!this->isMatrix() || this->data == NULL)
    {
        if (p_pDims)
            delete p_pDims;
        p_pDims = NULL;
        p_ndim = 0;
        return false;
    }

    //
    // Find dimensions and return to the beginning of tag data
    //
    qint32* t_pInt32 = static_cast< qint32* >(this->data);

    p_ndim = t_pInt32[(this->size-4)/4];


    if (p_pDims)
        delete p_pDims;
    p_pDims = new qint32[p_ndim];

    int j = 0;
    for(int i = p_ndim+1; i > 1; --i)
    {
        p_pDims[j] = t_pInt32[(this->size-(i*4))/4];
        ++j;
    }
    return true;
}


//*************************************************************************************************************

fiff_int_t FiffTag::getType() const
{
    if (this->isMatrix())
    {
        return DATA_TYPE & this->type;
    }
    else
    {
        return this->type;
    }
}


//*************************************************************************************************************

QString FiffTag::getInfo() const
{
    QString t_qStringInfo;

    if (this->isMatrix())
    {
        switch(this->getType())
        {
        case FIFFT_INT:
            t_qStringInfo = "Matrix of type FIFFT_INT";
            break;
        case FIFFT_JULIAN:
            t_qStringInfo = "Matrix of type FIFFT_JULIAN";
            break;
        case FIFFT_FLOAT:
            t_qStringInfo = "Matrix of type FIFFT_FLOAT";
            break;
        case FIFFT_DOUBLE:
            t_qStringInfo = "Matrix of type FIFFT_DOUBLE";
            break;
        case FIFFT_COMPLEX_FLOAT:
            t_qStringInfo = "Matrix of type FIFFT_COMPLEX_FLOAT";
            break;
        case FIFFT_COMPLEX_DOUBLE:
            t_qStringInfo = "Matrix of type FIFFT_COMPLEX_DOUBLE";
            break;
        default:
            t_qStringInfo = "Matrix of unknown type";
        }
    }
    else
    {
        switch(this->getType())
        {
        //
        // Simple types
        //
        case FIFFT_BYTE:
            t_qStringInfo = "Simple type FIFFT_BYTE";
            break;
        case FIFFT_SHORT:
            t_qStringInfo = "Simple type FIFFT_SHORT";
            break;
        case FIFFT_INT:
            t_qStringInfo = "Simple type FIFFT_INT";
            break;
        case FIFFT_USHORT:
            t_qStringInfo = "Simple type FIFFT_USHORT";
            break;
        case FIFFT_UINT:
            t_qStringInfo = "Simple type FIFFT_UINT";
            break;
        case FIFFT_FLOAT:
            t_qStringInfo = "Simple type FIFFT_FLOAT";
            break;
        case FIFFT_DOUBLE:
            t_qStringInfo = "Simple type FIFFT_DOUBLE";
            break;
        case FIFFT_STRING:
            t_qStringInfo = "Simple type FIFFT_STRING";
            break;
        case FIFFT_DAU_PACK16:
            t_qStringInfo = "Simple type FIFFT_DAU_PACK16";
            break;
        case FIFFT_COMPLEX_FLOAT:
            t_qStringInfo = "Simple type FIFFT_COMPLEX_FLOAT";
            break;
        case FIFFT_COMPLEX_DOUBLE:
            t_qStringInfo = "Simple type FIFFT_COMPLEX_DOUBLE";
            break;
        //
        //   Structures
        //
        case FIFFT_ID_STRUCT:
            t_qStringInfo = "Structure FIFFT_ID_STRUCT";
            break;
        case FIFFT_DIG_POINT_STRUCT:
            t_qStringInfo = "Structure FIFFT_DIG_POINT_STRUCT";
            break;
        case FIFFT_COORD_TRANS_STRUCT:
            t_qStringInfo = "Structure FIFFT_COORD_TRANS_STRUCT";
            break;
        case FIFFT_CH_INFO_STRUCT:
            t_qStringInfo = "Structure FIFFT_CH_INFO_STRUCT";
            break;
        case FIFFT_OLD_PACK:
            t_qStringInfo = "Structure FIFFT_OLD_PACK";
            break;
        case FIFFT_DIR_ENTRY_STRUCT:
            t_qStringInfo = "Structure FIFFT_DIR_ENTRY_STRUCT";
            break;
        default:
            t_qStringInfo = "Structure unknown";
        }
    }
    return t_qStringInfo;
}


//*************************************************************************************************************
//fiff_combat
short FiffTag::swap_short (fiff_short_t source)

{
    unsigned char *csource = (unsigned char *)(&source);
    fiff_short_t result;
    unsigned char *cresult =  (unsigned char *)(&result);

    cresult[0] = csource[1];
    cresult[1] = csource[0];
    return (result);
}


//*************************************************************************************************************

fiff_int_t FiffTag::swap_int (fiff_int_t source)
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

fiff_long_t FiffTag::swap_long (fiff_long_t source)

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

void FiffTag::swap_longp (fiff_long_t *source)

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

void FiffTag::swap_intp (fiff_int_t *source)

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

void FiffTag::swap_floatp (float *source)

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

void FiffTag::swap_doublep(double *source)

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

//#ifdef BIG_ENDIAN_ARCH


//void FiffTag::fiff_convert_tag_info(FiffTag*& tag)
//{
//    return;
//}

//#endif

//#ifdef INTEL_X86_ARCH
///*! Machine dependent data type conversions (tag info only)
// */

//void FiffTag::fiff_convert_tag_info(FiffTag*& tag)

//{
//    tag->kind = swap_int(tag->kind);
//    tag->type = swap_int(tag->type);
//    tag->size = swap_int(tag->size);
//    tag->next = swap_int(tag->next);
//    return;
//}

//#endif /* INTEL_X86_ARCH */


//*************************************************************************************************************

void FiffTag::convert_ch_pos(fiffChPos pos)
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

void FiffTag::convert_matrix_from_file_data(FiffTag* tag)
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

void FiffTag::convert_matrix_to_file_data(FiffTag* tag)
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

void FiffTag::fiff_convert_tag_data(FiffTag* tag, int from_endian, int to_endian)

{
    int            np;
    int            k,r;//,c;
    int            offset = 0;
    fiff_int_t     *ithis;
    fiff_short_t   *sthis;
    fiff_long_t    *lthis;
    float          *fthis;
    double         *dthis;
    fiffDirEntry   dethis;
    fiffId         idthis;
//    fiffChInfoRec* chthis;//FiffChInfo*     chthis;//ToDo adapt parsing to the new class
    fiffChPos      cpthis;
//    fiffCoordTrans ctthis;
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
//        np = tag->size/sizeof(fiffChInfoRec);
//        for (chthis = (fiffChInfoRec*)tag->data, k = 0; k < np; k++, chthis++) {
//            chthis->scanNo    = swap_int(chthis->scanNo);
//            chthis->logNo     = swap_int(chthis->logNo);
//            chthis->kind      = swap_int(chthis->kind);
//            swap_floatp(&chthis->range);
//            swap_floatp(&chthis->cal);
//            chthis->unit      = swap_int(chthis->unit);
//            chthis->unit_mul  = swap_int(chthis->unit_mul);
//            convert_ch_pos(&(chthis->chpos));
//        }

        ithis = static_cast< fiff_int_t* >(tag->data);
        fthis = static_cast< float* >(tag->data);
        np = tag->size/FiffChInfo::size();
        for (k = 0; k < np; k++) {
            offset = k*FiffChInfo::size();

            ithis[0+offset] = swap_int(ithis[0+offset]);//scanno
            ithis[1+offset] = swap_int(ithis[1+offset]);//logno
            ithis[2+offset] = swap_int(ithis[2+offset]); //kind
            swap_floatp(&fthis[3+offset]); //range
            swap_floatp(&fthis[4+offset]); //cal
            ithis[5+offset] = swap_int(ithis[5+offset]); //coil_type
            for (r = 0; r < 12; ++r)
                swap_floatp(&fthis[6+r+offset]); //loc
            ithis[18+offset] = swap_int(ithis[18+offset]); //unit
            ithis[19+offset] = swap_int(ithis[19+offset]); //unit_mul
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
//        np = tag->size/sizeof(fiffCoordTransRec);
//        for (ctthis = (fiffCoordTrans)tag->data, k = 0; k < np; k++, ctthis++) {
//            ctthis->from = swap_int(ctthis->from);
//            ctthis->to   = swap_int(ctthis->to);
//        for (r = 0; r < 3; r++) {
//             swap_floatp(&ctthis->move[r]);
//             swap_floatp(&ctthis->invmove[r]);
//            for (c = 0; c < 3; c++) {
//                swap_floatp(&ctthis->rot[r][c]);
//                swap_floatp(&ctthis->invrot[r][c]);
//            }
//        }
//    }

        ithis = static_cast< fiff_int_t* >(tag->data);
        fthis = static_cast< float* >(tag->data);

        np = tag->size/FiffCoordTrans::size();

        for( k = 0; k < np; ++k)
        {
            ithis = ithis + FiffCoordTrans::size()*k;
            fthis = fthis + FiffCoordTrans::size()*k;

            ithis[0] = swap_int(ithis[0]);
            ithis[1] = swap_int(ithis[1]);

            for (r = 0; r < 24; ++r)
                swap_floatp(&fthis[2+r]);
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

fiff_int_t FiffTag::fiff_type_fundamental(fiff_int_t type)
{
    return type & FIFFTS_FS_MASK;
}


//*************************************************************************************************************

fiff_int_t FiffTag::fiff_type_base(fiff_int_t type)
{
    return type & FIFFTS_BASE_MASK;
}


//*************************************************************************************************************

fiff_int_t FiffTag::fiff_type_matrix_coding(fiff_int_t type)
{
    return type & FIFFTS_MC_MASK;
}





