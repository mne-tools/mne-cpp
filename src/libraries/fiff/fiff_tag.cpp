//=============================================================================================================
/**
 * @file     fiff_tag.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the FiffTag Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_tag.h"
#include <utils/ioutils.h>

#include <complex>
#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTcpSocket>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffTag::FiffTag()
: kind(0)
, type(0)
, next(0)
//, m_pComplexFloatData(NULL)
//, m_pComplexDoubleData(NULL)
{
}

//=============================================================================================================

FiffTag::FiffTag(const FiffTag* p_pFiffTag)
: QByteArray( p_pFiffTag->data(), p_pFiffTag->size())
, kind(p_pFiffTag->kind)
, type(p_pFiffTag->type)
, next(p_pFiffTag->next)
{
//    if(p_pFiffTag->m_pComplexFloatData)
//        this->toComplexFloat();
//    else
//        m_pComplexFloatData = NULL;

//    if(p_pFiffTag->m_pComplexDoubleData)
//        this->toComplexDouble();
//    else
//        m_pComplexDoubleData = NULL;
}

//=============================================================================================================

FiffTag::~FiffTag()
{
//    if(this->m_pComplexFloatData)
//        delete this->m_pComplexFloatData;
//    if(this->m_pComplexDoubleData)
//        delete this->m_pComplexDoubleData;
}

//=============================================================================================================

fiff_int_t FiffTag::getMatrixCoding() const
{
   return IS_MATRIX & this->type;
}

//=============================================================================================================

bool FiffTag::isMatrix() const
{
    if (this->getMatrixCoding() != 0)
        return true;
    else
        return false;
}

//=============================================================================================================

bool FiffTag::getMatrixDimensions(qint32& p_ndim, QVector<qint32>& p_Dims) const
{
    p_Dims.clear();
    if(!this->isMatrix() || this->data() == NULL)
    {
        p_ndim = 0;
        return false;
    }

    //
    // Find dimensions and return to the beginning of tag data
    //
    qint32* t_pInt32 = (qint32*)this->data();

    p_ndim = t_pInt32[(this->size()-4)/4];

    if (fiff_type_matrix_coding(this->type) == FIFFTS_MC_DENSE)
        for(int i = p_ndim+1; i > 1; --i)
            p_Dims.append(t_pInt32[(this->size()-(i*4))/4]);
    else if(fiff_type_matrix_coding(this->type) == FIFFTS_MC_CCS || fiff_type_matrix_coding(this->type) == FIFFTS_MC_RCS)
        for(int i = p_ndim+2; i > 1; --i)
            p_Dims.append(t_pInt32[(this->size()-(i*4))/4]);
    else
    {
        printf("Error: Cannot handle other than dense or sparse matrices yet.\n");//ToDo Throw
        return false;
    }

    return true;
}

//=============================================================================================================

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

//=============================================================================================================

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

//=============================================================================================================

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

//=============================================================================================================

void FiffTag::convert_ch_pos(FiffChPos* pos)
{
    int k;
    pos->coil_type  = IOUtils::swap_int(pos->coil_type);
    for (k = 0; k < 3; k++) {
        IOUtils::swap_floatp(&pos->r0[k]);
        IOUtils::swap_floatp(&pos->ex[k]);
        IOUtils::swap_floatp(&pos->ey[k]);
        IOUtils::swap_floatp(&pos->ez[k]);
    }
    return;
}

//=============================================================================================================

void FiffTag::convert_matrix_from_file_data(FiffTag::SPtr tag)
/*
 * Assumes that the input is in the non-native byte order and needs to be swapped to the other one
 */
{
    int ndim;
    int k;
    int *dimp,*data,kind,np,nz;
    float *fdata;
    double *ddata;
    unsigned int tsize = tag->size();

    if (fiff_type_fundamental(tag->type) != FIFFTS_FS_MATRIX)
        return;
    if (tag->data() == NULL)
        return;
    if (tsize < sizeof(fiff_int_t))
        return;

    dimp = ((fiff_int_t *)((tag->data())+tag->size()-sizeof(fiff_int_t)));
    IOUtils::swap_intp(dimp);
    ndim = *dimp;
    if (fiff_type_matrix_coding(tag->type) == FIFFTS_MC_DENSE) {
        if (tsize < (ndim+1)*sizeof(fiff_int_t))
            return;
        dimp = dimp - ndim;
        for (k = 0, np = 1; k < ndim; k++) {
            IOUtils::swap_intp(dimp+k);
            np = np*dimp[k];
        }
    }
    else {
        if (tsize < (ndim+2)*sizeof(fiff_int_t))
            return;
        if (ndim > 2)       /* Not quite sure what to do */
            return;
        dimp = dimp - ndim - 1;
        for (k = 0; k < ndim+1; k++)
            IOUtils::swap_intp(dimp+k);
        nz = dimp[0];
        if (fiff_type_matrix_coding(tag->type) == FIFFTS_MC_CCS)
            np = nz + dimp[2] + 1; /* nz + n + 1 */
        else if (fiff_type_matrix_coding(tag->type) == FIFFTS_MC_RCS)
            np = nz + dimp[1] + 1; /* nz + m + 1 */
        else
            return;     /* Don't know what to do */
        /*
         * Take care of the indices
        */
        for (data = (int *)(tag->data())+nz, k = 0; k < np; k++)
            IOUtils::swap_intp(data+k);
        np = nz;
    }
    /*
     * Now convert data...
     */
    kind = fiff_type_base(tag->type);
    if (kind == FIFFT_INT) {
        for (data = (int *)(tag->data()), k = 0; k < np; k++)
            IOUtils::swap_intp(data+k);
    }
    else if (kind == FIFFT_FLOAT) {
        for (fdata = (float *)(tag->data()), k = 0; k < np; k++)
            IOUtils::swap_floatp(fdata+k);
    }
    else if (kind == FIFFT_DOUBLE) {
        for (ddata = (double *)(tag->data()), k = 0; k < np; k++)
            IOUtils::swap_doublep(ddata+k);
    }
    return;
}

//=============================================================================================================

void FiffTag::convert_matrix_to_file_data(FiffTag::SPtr tag)
/*
 * Assumes that the input is in the NATIVE_ENDIAN byte order and needs to be swapped to the other one
 */
{
    int ndim;
    int k;
    int *dimp,*data,kind,np;
    float *fdata;
    double *ddata;
    unsigned int tsize = tag->size();

    if (fiff_type_fundamental(tag->type) != FIFFTS_FS_MATRIX)
        return;
    if (tag->data() == NULL)
        return;
    if (tsize < sizeof(fiff_int_t))
        return;

    dimp = ((fiff_int_t *)(((char *)tag->data())+tag->size()-sizeof(fiff_int_t)));
    ndim = *dimp;
    IOUtils::swap_intp(dimp);

    if (fiff_type_matrix_coding(tag->type) == FIFFTS_MC_DENSE) {
        if (tsize < (ndim+1)*sizeof(fiff_int_t))
            return;
        dimp = dimp - ndim;
        for (k = 0, np = 1; k < ndim; k++) {
            np = np*dimp[k];
            IOUtils::swap_intp(dimp+k);
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
            IOUtils::swap_intp(dimp+k);
    }
    /*
     * Now convert data...
     */
    kind = fiff_type_base(tag->type);
    if (kind == FIFFT_INT) {
        for (data = (int *)(tag->data()), k = 0; k < np; k++)
            IOUtils::swap_intp(data+k);
    }
    else if (kind == FIFFT_FLOAT) {
        for (fdata = (float *)(tag->data()), k = 0; k < np; k++)
            IOUtils::swap_floatp(fdata+k);
    }
    else if (kind == FIFFT_DOUBLE) {
        for (ddata = (double *)(tag->data()), k = 0; k < np; k++)
            IOUtils::swap_doublep(ddata+k);
    }
    else if (kind == FIFFT_COMPLEX_FLOAT) {
        for (fdata = (float *)(tag->data()), k = 0; k < 2*np; k++)
            IOUtils::swap_floatp(fdata+k);
    }
    else if (kind == FIFFT_COMPLEX_DOUBLE) {
        for (ddata = (double *)(tag->data()), k = 0; k < 2*np; k++)
            IOUtils::swap_doublep(ddata+k);
    }
    return;
}

//=============================================================================================================
//ToDo remove this function by swapping -> define little endian big endian, QByteArray
void FiffTag::convert_tag_data(FiffTag::SPtr tag, int from_endian, int to_endian)
{
    int            np;
    int            k,r;//,c;
    char           *offset;
    fiff_int_t     *ithis;
    fiff_short_t   *sthis;
    fiff_long_t    *lthis;
    float          *fthis;
    double         *dthis;
//    fiffDirEntry   dethis;
//    fiffId         idthis;
//    fiffChInfoRec* chthis;//FiffChInfo*     chthis;//ToDo adapt parsing to the new class
//    fiffChPos      cpthis;
//    fiffCoordTrans ctthis;
//    fiffDigPoint   dpthis;
    fiffDataRef    drthis;

    if (tag->data() == NULL || tag->size() == 0)
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
    case FIFFT_UINT :
    case FIFFT_JULIAN :
        np = tag->size()/sizeof(fiff_int_t);
        for (ithis = (fiff_int_t *)tag->data(), k = 0; k < np; k++, ithis++)
            IOUtils::swap_intp(ithis);
        break;

    case FIFFT_LONG :
    case FIFFT_ULONG :
        np = tag->size()/sizeof(fiff_long_t);
        for (lthis = (fiff_long_t *)tag->data(), k = 0; k < np; k++, lthis++)
            IOUtils::swap_longp(lthis);
        break;

    case FIFFT_SHORT :
    case FIFFT_DAU_PACK16 :
    case FIFFT_USHORT :
        np = tag->size()/sizeof(fiff_short_t);
        for (sthis = (fiff_short_t *)tag->data(), k = 0; k < np; k++, sthis++)
            *sthis = IOUtils::swap_short(*sthis);
        break;

    case FIFFT_FLOAT :
    case FIFFT_COMPLEX_FLOAT :
        np = tag->size()/sizeof(fiff_float_t);
        for (fthis = (fiff_float_t *)tag->data(), k = 0; k < np; k++, fthis++)
            IOUtils::swap_floatp(fthis);
        break;

    case FIFFT_DOUBLE :
    case FIFFT_COMPLEX_DOUBLE :
        np = tag->size()/sizeof(fiff_double_t);
        for (dthis = (fiff_double_t *)tag->data(), k = 0; k < np; k++, dthis++)
            IOUtils::swap_doublep(dthis);
        break;

    case FIFFT_OLD_PACK :
        fthis = (float *)tag->data();
    /*
     * Offset and scale...
     */
        IOUtils::swap_floatp(fthis+0);
        IOUtils::swap_floatp(fthis+1);
        sthis = (short *)(fthis+2);
        np = (tag->size() - 2*sizeof(float))/sizeof(short);
        for (k = 0; k < np; k++,sthis++)
            *sthis = IOUtils::swap_short(*sthis);
        break;

    case FIFFT_DIR_ENTRY_STRUCT :
//        np = tag->size/sizeof(fiffDirEntryRec);
//        for (dethis = (fiffDirEntry)tag->data->data(), k = 0; k < np; k++, dethis++) {
//            dethis->kind = swap_int(dethis->kind);
//            dethis->type = swap_int(dethis->type);
//            dethis->size = swap_int(dethis->size);
//            dethis->pos  = swap_int(dethis->pos);
//        }
        np = tag->size()/FiffDirEntry::storageSize();
        for (k = 0; k < np; k++) {
            offset = (char*)tag->data() + k*FiffDirEntry::storageSize();
            ithis = (fiff_int_t*) offset;
            ithis[0] = IOUtils::swap_int(ithis[0]);//kind
            ithis[1] = IOUtils::swap_int(ithis[1]);//type
            ithis[2] = IOUtils::swap_int(ithis[2]);//size
            ithis[3] = IOUtils::swap_int(ithis[3]);//pos
        }
        break;

    case FIFFT_ID_STRUCT :
//        np = tag->size/sizeof(fiffIdRec);
//        for (idthis = (fiffId)tag->data->data(), k = 0; k < np; k++, idthis++) {
//            idthis->version = swap_int(idthis->version);
//            idthis->machid[0] = swap_int(idthis->machid[0]);
//            idthis->machid[1] = swap_int(idthis->machid[1]);
//            idthis->time.secs  = swap_int(idthis->time.secs);
//            idthis->time.usecs = swap_int(idthis->time.usecs);
//        }
        np = tag->size()/FiffId::storageSize();
        for (k = 0; k < np; k++) {
            offset = (char*)tag->data() + k*FiffId::storageSize();
            ithis = (fiff_int_t*) offset;
            ithis[0] = IOUtils::swap_int(ithis[0]);//version
            ithis[1] = IOUtils::swap_int(ithis[1]);//machid[0]
            ithis[2] = IOUtils::swap_int(ithis[2]);//machid[1]
            ithis[3] = IOUtils::swap_int(ithis[3]);//time.secs
            ithis[4] = IOUtils::swap_int(ithis[4]);//time.usecs
        }
        break;

    case FIFFT_CH_INFO_STRUCT :
//        np = tag->size/sizeof(fiffChInfoRec);
//        for (chthis = (fiffChInfoRec*)tag->data->data(), k = 0; k < np; k++, chthis++) {
//            chthis->scanNo    = swap_int(chthis->scanNo);
//            chthis->logNo     = swap_int(chthis->logNo);
//            chthis->kind      = swap_int(chthis->kind);
//            swap_floatp(&chthis->range);
//            swap_floatp(&chthis->cal);
//            chthis->unit      = swap_int(chthis->unit);
//            chthis->unit_mul  = swap_int(chthis->unit_mul);
//            convert_ch_pos(&(chthis->chpos));
//        }

        np = tag->size()/FiffChInfo::storageSize();
        for (k = 0; k < np; k++) {
            offset = (char*)tag->data() + k*FiffChInfo::storageSize();
            ithis = (fiff_int_t*) offset;
            fthis = (float*) offset;

            ithis[0] = IOUtils::swap_int(ithis[0]);  //scanno
            ithis[1] = IOUtils::swap_int(ithis[1]);  //logno
            ithis[2] = IOUtils::swap_int(ithis[2]);  //kind
            IOUtils::swap_floatp(&fthis[3]);         //range
            IOUtils::swap_floatp(&fthis[4]);         //cal
            ithis[5] = IOUtils::swap_int(ithis[5]);  //coil_type
            for (r = 0; r < 12; ++r)
                IOUtils::swap_floatp(&fthis[6+r]);   //loc
            ithis[18] = IOUtils::swap_int(ithis[18]);//unit
            ithis[19] = IOUtils::swap_int(ithis[19]);//unit_mul
        }

        break;

    case FIFFT_CH_POS_STRUCT :
//        np = tag->size/sizeof(fiffChPosRec);
//        for (cpthis = (fiffChPos)tag->data->data(), k = 0; k < np; k++, cpthis++)
//            convert_ch_pos(cpthis);

        np = tag->size()/FiffChPos::storageSize();
        for (k = 0; k < np; ++k)
        {
            offset = (char*)tag->data() + k*FiffChPos::storageSize();
            ithis = (fiff_int_t*) offset;
            fthis = (float*) offset;

            ithis[0] = IOUtils::swap_int(ithis[0]);    //coil_type
            for (r = 0; r < 12; r++)
                IOUtils::swap_floatp(&fthis[1+r]);    //r0, ex, ey, ez
        }

        break;

    case FIFFT_DIG_POINT_STRUCT :
//        np = tag->size/sizeof(fiffDigPointRec);
//        for (dpthis = (fiffDigPoint)tag->data->data(), k = 0; k < np; k++, dpthis++) {
//            dpthis->kind = swap_int(dpthis->kind);
//            dpthis->ident = swap_int(dpthis->ident);
//            for (r = 0; r < 3; r++)
//                swap_floatp(&dpthis->r[r]);
//        }

        np = tag->size()/FiffDigPoint::storageSize();

        for (k = 0; k < np; k++) {
            offset = tag->data() + k*FiffDigPoint::storageSize();
            ithis = (fiff_int_t*) offset;
            fthis = (float*) offset;

            ithis[0] = IOUtils::swap_int(ithis[0]);//kind
            ithis[1] = IOUtils::swap_int(ithis[1]);//ident

            for (r = 0; r < 3; ++r)
                IOUtils::swap_floatp(&fthis[2+r]);        //r
        }
        break;

    case FIFFT_COORD_TRANS_STRUCT :
//        np = tag->size/sizeof(fiffCoordTransRec);
//        for (ctthis = (fiffCoordTrans)tag->data->data(), k = 0; k < np; k++, ctthis++) {
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

        np = tag->size()/FiffCoordTrans::storageSize();

        for( k = 0; k < np; ++k)
        {
            offset = tag->data() + k*FiffCoordTrans::storageSize();
            ithis = (fiff_int_t*)offset;
            fthis = (float*)offset;

            ithis[0] = IOUtils::swap_int(ithis[0]);
            ithis[1] = IOUtils::swap_int(ithis[1]);

            for (r = 0; r < 24; ++r)
                IOUtils::swap_floatp(&fthis[2+r]);
        }
        break;

    case FIFFT_DATA_REF_STRUCT :
        np = tag->size()/sizeof(fiffDataRefRec);
        for (drthis = (fiffDataRef)tag->data(), k = 0; k < np; k++, drthis++) {
            drthis->type   = IOUtils::swap_int(drthis->type);
            drthis->endian = IOUtils::swap_int(drthis->endian);
            drthis->size   = IOUtils::swap_long(drthis->size);
            drthis->offset = IOUtils::swap_long(drthis->offset);
        }
        break;

    default :
        break;
    }
    return;
}

//=============================================================================================================
//fiff_type_spec

fiff_int_t FiffTag::fiff_type_fundamental(fiff_int_t type)
{
    return type & FIFFTS_FS_MASK;
}

//=============================================================================================================

fiff_int_t FiffTag::fiff_type_base(fiff_int_t type)
{
    return type & FIFFTS_BASE_MASK;
}

//=============================================================================================================

fiff_int_t FiffTag::fiff_type_matrix_coding(fiff_int_t type)
{
    return type & FIFFTS_MC_MASK;
}
