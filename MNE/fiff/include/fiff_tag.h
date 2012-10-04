//=============================================================================================================
/**
* @file     fiff_tag.h
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
* @brief    Contains the FiffTag class declaration, which provides fiff tag I/O and processing methods.
*
*/

#ifndef FIFF_TAG_H
#define FIFF_TAG_H

//*************************************************************************************************************
//=============================================================================================================
// DEFINES
//=============================================================================================================

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

#include "../fiff_global.h"
#include "../../3rdParty/Eigen/Core"
#include "fiff_constants.h"
#include "fiff_types.h"
#include "fiff_id.h"
#include "fiff_coord_trans.h"
#include "fiff_ch_info.h"
#include "fiff_ch_pos.h"
#include "fiff_file.h"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <complex>
#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDataStream>
#include <QList>
#include <QFile>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

class FiffFile;

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

//using namespace SOURCELAB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
* DECLARE CLASS Fiff
*
* @brief The Fiff class provides...
*/


//
//   The magic hexadecimal values
//
const fiff_int_t IS_MATRIX           = 4294901760; // ffff0000
const fiff_int_t MATRIX_CODING_DENSE = 16384;      // 4000
const fiff_int_t MATRIX_CODING_CCS   = 16400;      // 4010
const fiff_int_t MATRIX_CODING_RCS   = 16416;      // 4020
const fiff_int_t DATA_TYPE           = 65535;      // ffff
//


//=============================================================================================================
/**
* Tags are used in front of data items to tell what they are.
*
* @brief FIFF data tag
*/
class FIFFSHARED_EXPORT FiffTag {

public:
    //=========================================================================================================
    /**
    * ctor
    */
    FiffTag();
    //=========================================================================================================
    /**
    * Destroys the FiffTag.
    */
    ~FiffTag();

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the fiff_read_tag_info function
    *
    * Read tag information of one tag from a fif file.
    * if pos is not provided, reading starts from the current file position
    *
    * @param[in] p_pFile opened fif file
    * @param[out] p_pTag the read tag info
    *
    * @return true if succeeded, false otherwise
    */
    static bool read_tag_info(FiffFile* p_pFile, FiffTag*& p_pTag);

    //=========================================================================================================
    /**
    * ### MNE toolbox root function ###: Implementation of the fiff_read_tag function
    *
    * Read one tag from a fif file.
    * if pos is not provided, reading starts from the current file position
    *
    * @param[in] p_pFile opened fif file
    * @param[out] p_pTag the read tag
    * @param[in] pos position of the tag inside the fif file
    *
    * @return true if succeeded, false otherwise
    */
    static bool read_tag(FiffFile* p_pFile, FiffTag*& p_pTag, qint64 pos = -1);

    //=========================================================================================================
    /**
    *
    */
    fiff_int_t getMatrixCoding() const;

    //=========================================================================================================
    /**
    *
    */
    bool isMatrix() const;

    //=========================================================================================================
    /**
    *
    */
    bool getMatrixDimensions(qint32& p_ndim, qint32*& p_pDims) const;

    //=========================================================================================================
    /**
    *
    */
    fiff_int_t getType() const;

    //=========================================================================================================
    /**
    *
    */
    QString getInfo() const;

    //
    // Simple types
    //
    //=========================================================================================================
    /**
    * to Byte
    */
    inline quint8* toByte();

    //=========================================================================================================
    /**
    * to unsigned Short
    */
    inline quint16* toUnsignedShort();

    //=========================================================================================================
    /**
    * to Short
    */
    inline qint16* toShort();

    //=========================================================================================================
    /**
    * to Int
    */
    inline quint32* toUnsignedInt();

    //=========================================================================================================
    /**
    * to Int
    */
    inline qint32* toInt();

    //=========================================================================================================
    /**
    * to Float
    */
    inline float* toFloat();

    //=========================================================================================================
    /**
    * to Double
    */
    inline double* toDouble();

    //=========================================================================================================
    /**
    * to String
    */
    inline QString toString();

    //=========================================================================================================
    /**
    * to DauPack16
    */
    inline qint16* toDauPack16();

    //=========================================================================================================
    /**
    * to complex float - pointer has to be deleted ater use
    */
    inline std::complex<float>* toComplexFloat();

    //=========================================================================================================
    /**
    * to complex double - pointer has to be deleted ater use
    */
    inline std::complex<double>* toComplexDouble();

    //
    // Structures
    //
    //=========================================================================================================
    /**
    * to fiff ID Struct
    */
    inline FiffId toFiffID() const;

    //=========================================================================================================
    /**
    * to fiff DIG POINT STRUCT
    */
    inline FiffDigPoint toDigPoint() const;

    //=========================================================================================================
    /**
    * to fiff COORD TRANS STRUCT
    */
    inline FiffCoordTrans toCoordTrans() const;


    //=========================================================================================================
    /**
    * to fiff CH INFO STRUCT
    */
    inline FiffChInfo toChInfo() const;


//    //=========================================================================================================
//    /**
//    * to fiff OLD PACK
//    */
//    inline fiff_coord_trans_t toCoordTrans() const;


    //=========================================================================================================
    /**
    * to fiff DIR ENTRY STRUCT
    */
    inline QList<FiffDirEntry> toDirEntry() const;


//    if (this->isMatrix())
//    {
//        switch(this->getType())
//        {
//        case FIFFT_INT:
//            t_qStringInfo = "Matrix of type FIFFT_INT";
//            break;
//        case FIFFT_JULIAN:
//            t_qStringInfo = "Matrix of type FIFFT_JULIAN";
//            break;
//        case FIFFT_FLOAT:
//            t_qStringInfo = "Matrix of type FIFFT_FLOAT";
//            break;
//        case FIFFT_DOUBLE:
//            t_qStringInfo = "Matrix of type FIFFT_DOUBLE";
//            break;
//        case FIFFT_COMPLEX_FLOAT:
//            t_qStringInfo = "Matrix of type FIFFT_COMPLEX_FLOAT";
//            break;
//        case FIFFT_COMPLEX_DOUBLE:
//            t_qStringInfo = "Matrix of type FIFFT_COMPLEX_DOUBLE";
//            break;
//        default:
//            t_qStringInfo = "Matrix of unknown type";
//        }
//    }


    //
    // MATRIX
    //
    //=========================================================================================================
    /**
    * to fiff FIFFT INT MATRIX
    */
    inline MatrixXi toIntMatrix() const;


    //=========================================================================================================
    /**
    * to fiff FIFFT FLOAT MATRIX
    */
    inline MatrixXf toFloatMatrix() const;








    //from fiff_combat.c

    static short swap_short (fiff_short_t source);

    static fiff_int_t swap_int (fiff_int_t source);

    static fiff_long_t swap_long (fiff_long_t source);

    static void swap_longp (fiff_long_t *source);

    static void swap_intp (fiff_int_t *source);

    static void swap_floatp (float *source);

    static void swap_doublep(double *source);

    static void convert_ch_pos(FiffChPos* pos);

//    static void fiff_convert_tag_info(FiffTag*& tag);

    static void convert_matrix_from_file_data(FiffTag* tag);

    static void convert_matrix_to_file_data(FiffTag* tag);

    static void convert_tag_data(FiffTag* tag, int from_endian, int to_endian);

    //from fiff_type_spec.c

    static fiff_int_t fiff_type_fundamental(fiff_int_t type);

    static fiff_int_t fiff_type_base(fiff_int_t type);

    static fiff_int_t fiff_type_matrix_coding(fiff_int_t type);


public:
    fiff_int_t  kind;		/**< Tag number.
                 *   This defines the meaning of the item */
    fiff_int_t  type;		/**< Data type.
                 *   This defines the reperentation of the data. */
    fiff_int_t  size;		/**< Size of the data.
                 *   The size is given in bytes and defines the
                 *   total size of the data. */
    fiff_int_t  next;		/**< Pointer to the next object.
                 *   Zero if the object follows
                 *   sequentially in file.
                 *   Negative at the end of file */
    fiff_data_t* data;		/**< Pointer to the data.
                 *   This point to the data read or to be written. */


private:
    std::complex<float>* m_pComplexFloatData;

    std::complex<double>* m_pComplexDoubleData;

};   /**< FIFF data tag */


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

//*************************************************************************************************************
//=============================================================================================================
// Simple types
//=============================================================================================================

inline quint8* FiffTag::toByte()
{
    if(this->isMatrix() || this->getType() != FIFFT_BYTE)
        return NULL;
    else
        return static_cast< quint8* >(this->data);
}


//*************************************************************************************************************

inline quint16* FiffTag::toUnsignedShort()
{
    if(this->isMatrix() || this->getType() != FIFFT_USHORT)
        return NULL;
    else
        return static_cast< quint16* >(this->data);
}


//*************************************************************************************************************

inline qint16* FiffTag::toShort()
{
    if(this->isMatrix() || this->getType() != FIFFT_SHORT)
        return NULL;
    else
        return static_cast< qint16* >(this->data);
}


//*************************************************************************************************************

inline quint32* FiffTag::toUnsignedInt()
{
    if(this->isMatrix() || this->getType() != FIFFT_UINT)
        return NULL;
    else
        return static_cast< quint32* >(this->data);
}


//*************************************************************************************************************

inline qint32* FiffTag::toInt()
{
    if(this->isMatrix() || this->getType() != FIFFT_INT)
        return NULL;
    else
        return static_cast< qint32* >(this->data);
}


//*************************************************************************************************************

inline float* FiffTag::toFloat()
{
    if(this->isMatrix() || this->getType() != FIFFT_FLOAT)
        return NULL;
    else
        return static_cast< float* >(this->data);
}


//*************************************************************************************************************

inline double* FiffTag::toDouble()
{
    if(this->isMatrix() || this->getType() != FIFFT_DOUBLE)
        return NULL;
    else
        return static_cast< double* >(this->data);
}


//*************************************************************************************************************

inline QString FiffTag::toString()
{
    if(this->isMatrix() || this->getType() != FIFFT_STRING)
        return NULL;
    else
        return QString::fromAscii(static_cast< char* >(this->data));
}


//*************************************************************************************************************

inline qint16* FiffTag::toDauPack16()
{
    if(this->isMatrix() || this->getType() != FIFFT_DAU_PACK16)
        return NULL;
    else
        return static_cast< qint16* >(this->data);
}


//*************************************************************************************************************

inline std::complex<float>* FiffTag::toComplexFloat()
{
    if(this->isMatrix() || this->getType() != FIFFT_COMPLEX_FLOAT)
        return NULL;
    else if(this->m_pComplexFloatData == NULL)
    {
        float* t_pFloat = static_cast< float* >(this->data);
        this->m_pComplexFloatData = new std::complex<float>(t_pFloat[0],t_pFloat[1]);
    }
    return m_pComplexFloatData;
}


//*************************************************************************************************************

inline std::complex<double>* FiffTag::toComplexDouble()
{
    if(this->isMatrix() || this->getType() != FIFFT_COMPLEX_DOUBLE)
        return NULL;
    else if(this->m_pComplexDoubleData == NULL)
    {
        double* t_pDouble = static_cast< double* >(this->data);
        this->m_pComplexDoubleData = new std::complex<double>(t_pDouble[0],t_pDouble[1]);
    }
    return m_pComplexDoubleData;
}

//*************************************************************************************************************
//=============================================================================================================
// Structures
//=============================================================================================================

inline FiffId FiffTag::toFiffID() const
{
    FiffId p_fiffID;
    if(this->isMatrix() || this->getType() != FIFFT_ID_STRUCT || this->data == NULL)
        return p_fiffID;
    else
    {
        qint32* t_pInt32 = static_cast< qint32* >(this->data);
//            memcpy (&t_fiffID,this->data,this->size);

        p_fiffID.version = t_pInt32[0];
        p_fiffID.machid[0] = t_pInt32[1];
        p_fiffID.machid[1] = t_pInt32[2];
        p_fiffID.time.secs = t_pInt32[3];
        p_fiffID.time.usecs = t_pInt32[4];

        return p_fiffID;
    }
}


//*************************************************************************************************************

inline FiffDigPoint FiffTag::toDigPoint() const
{

    FiffDigPoint t_fiffDigPoint;
    if(this->isMatrix() || this->getType() != FIFFT_DIG_POINT_STRUCT || this->data == NULL)
        return t_fiffDigPoint;
    else
    {
        qint32* t_pInt32 = static_cast< qint32* >(this->data);
//            memcpy (&t_fiffDigPoint,this->data,this->size);

        t_fiffDigPoint.kind = t_pInt32[0];
        t_fiffDigPoint.ident = t_pInt32[1];

        float* t_pFloat = static_cast< float* >(this->data);
        t_fiffDigPoint.r[0] = t_pFloat[2];
        t_fiffDigPoint.r[1] = t_pFloat[3];
        t_fiffDigPoint.r[2] = t_pFloat[4];
        t_fiffDigPoint.coord_frame = 0;

        return t_fiffDigPoint;
    }
}


//*************************************************************************************************************

inline FiffCoordTrans FiffTag::toCoordTrans() const
{

    FiffCoordTrans t_fiffCoordTrans;
    if(this->isMatrix() || this->getType() != FIFFT_COORD_TRANS_STRUCT || this->data == NULL)
        return t_fiffCoordTrans;
    else
    {
        qint32* t_pInt32 = static_cast< qint32* >(this->data);
        t_fiffCoordTrans.from = t_pInt32[0];
        t_fiffCoordTrans.to = t_pInt32[1];

        t_fiffCoordTrans.trans.setIdentity(4,4);
        float* t_pFloat = static_cast< float* >(this->data);
        int count = 0;
        int r, c;
        for (r = 0; r < 3; ++r) {
            t_fiffCoordTrans.trans(r,3) = t_pFloat[11+r];
            for (c = 0; c < 3; ++c) {
                t_fiffCoordTrans.trans(r,c) = t_pFloat[2+count];
                ++count;
            }
        }

        t_fiffCoordTrans.invtrans.setIdentity(4,4);
        count = 0;
        for (r = 0; r < 3; ++r) {
            t_fiffCoordTrans.invtrans(r,3) = t_pFloat[23+r];
            for (c = 0; c < 3; ++c) {
                t_fiffCoordTrans.invtrans(r,c) = t_pFloat[14+count];
                ++count;
            }
        }

        return t_fiffCoordTrans;
    }
}


//=========================================================================================================
/**
* to fiff CH INFO STRUCT
*/
inline FiffChInfo FiffTag::toChInfo() const
{
    FiffChInfo p_FiffChInfo;

    if(this->isMatrix() || this->getType() != FIFFT_CH_INFO_STRUCT || this->data == NULL)
        return p_FiffChInfo;
    else
    {
        qint32* t_pInt32 = static_cast< qint32* >(this->data);
        p_FiffChInfo.scanno = t_pInt32[0];
        p_FiffChInfo.logno = t_pInt32[1];
        p_FiffChInfo.kind = t_pInt32[2];
        float* t_pFloat = static_cast< float* >(this->data);
        p_FiffChInfo.range = t_pFloat[3];
        p_FiffChInfo.cal = t_pFloat[4];
        p_FiffChInfo.coil_type = t_pInt32[5];

        //
        //   Read the coil coordinate system definition
        //
        qint32 count = 0;
        qint32 r, c;
        for (r = 0; r < 12; ++r) {
            p_FiffChInfo.loc(r,0) = t_pFloat[6+r];
        }

        p_FiffChInfo.coord_frame = FIFFV_COORD_UNKNOWN;

        //
        //   Convert loc into a more useful format
        //
        if (p_FiffChInfo.kind == FIFFV_MEG_CH || p_FiffChInfo.kind == FIFFV_REF_MEG_CH)
        {
            p_FiffChInfo.coil_trans.setIdentity(4,4);
            for (r = 0; r < 3; ++r) {
                p_FiffChInfo.coil_trans(r,3) = p_FiffChInfo.loc(r,0);
                for (c = 0; c < 3; ++c) {
                    p_FiffChInfo.coil_trans(c,r) = p_FiffChInfo.loc(3+count,0);//its transposed stored (r and c are exchanged)
                    ++count;
                }
            }
            p_FiffChInfo.coord_frame = FIFFV_COORD_DEVICE;
        }
        else if (p_FiffChInfo.kind == FIFFV_EEG_CH)
        {
            if (p_FiffChInfo.loc.block(3,0,3,1).norm() > 0)
            {
                p_FiffChInfo.eeg_loc.block(0,0,3,1) = p_FiffChInfo.loc.block(0,0,3,1);
                p_FiffChInfo.eeg_loc.block(0,1,3,1) = p_FiffChInfo.loc.block(3,0,3,1);
            }
            else
            {
                p_FiffChInfo.eeg_loc.block(0,0,3,1) = p_FiffChInfo.loc.block(0,0,3,1);
            }
            p_FiffChInfo.coord_frame = FIFFV_COORD_HEAD;
        }
        //
        //   Unit and exponent
        //
        p_FiffChInfo.unit = t_pInt32[18];
        p_FiffChInfo.unit_mul = t_pInt32[19];

        //
        //   Handle the channel name
        //
        char* orig = static_cast< char* >(this->data);
        p_FiffChInfo.ch_name = QString::fromAscii(orig + 80);

        return p_FiffChInfo;
    }
}


//    //=========================================================================================================
//    /**
//    * to fiff OLD PACK
//    */
//    inline fiff_coord_trans_t toCoordTrans() const
//    {


//    }



//*************************************************************************************************************

inline QList<FiffDirEntry> FiffTag::toDirEntry() const
{
//         tag.data = struct('kind',{},'type',{},'size',{},'pos',{});
    QList<FiffDirEntry> p_ListFiffDir;
    if(this->isMatrix() || this->getType() != FIFFT_DIR_ENTRY_STRUCT || this->data == NULL)
        return p_ListFiffDir;
    else
    {
        FiffDirEntry t_fiffDirEntry;
        qint32* t_pInt32 = static_cast< qint32* >(this->data);
        for (int k = 0; k < this->size/16; ++k)
        {
            t_fiffDirEntry.kind = t_pInt32[k*4];//fread(fid,1,'int32');
            t_fiffDirEntry.type = t_pInt32[k*4+1];//fread(fid,1,'uint32');
            t_fiffDirEntry.size = t_pInt32[k*4+2];//fread(fid,1,'int32');
            t_fiffDirEntry.pos  = t_pInt32[k*4+3];//fread(fid,1,'int32');
            p_ListFiffDir.append(t_fiffDirEntry);
        }
    }
    return p_ListFiffDir;
}


//*************************************************************************************************************
//=============================================================================================================
// MATRIX
//=============================================================================================================

//*************************************************************************************************************

inline MatrixXi FiffTag::toIntMatrix() const
{
//        qDebug() << "toIntMatrix";

    MatrixXi p_defaultMatrix(0, 0);

    if(!this->isMatrix() || this->getType() != FIFFT_INT || this->data == NULL)
        return p_defaultMatrix;

    qint32 ndim;
    qint32* pDims = NULL;
    this->getMatrixDimensions(ndim, pDims);

    if (ndim != 2)
    {
        delete pDims;
        printf("Only two-dimensional matrices are supported at this time");
        return p_defaultMatrix;
    }

    //MatrixXf p_Matrix = Map<MatrixXf>( static_cast< float* >(this->data),p_dims[0], p_dims[1]);
    // --> Use copy constructor instead --> slower performance but higher memory management reliability
    MatrixXi p_Matrix(Map<MatrixXi>( static_cast< int* >(this->data),pDims[0], pDims[1]));

    delete pDims;

    return p_Matrix;
}


//*************************************************************************************************************

inline MatrixXf FiffTag::toFloatMatrix() const
{
//        qDebug() << "toFloatMatrix";

    MatrixXf p_defaultMatrix(0, 0);

    if(!this->isMatrix() || this->getType() != FIFFT_FLOAT || this->data == NULL)
        return p_defaultMatrix;


    qint32 ndim;
    qint32* pDims = NULL;
    this->getMatrixDimensions(ndim, pDims);

//        qDebug() << "toFloatMatrix" << ndim << pDims[0] << pDims[1];


    if (ndim != 2)
    {
        delete pDims;
        printf("Only two-dimensional matrices are supported at this time");
        return p_defaultMatrix;
    }

    //MatrixXf p_Matrix = Map<MatrixXf>( static_cast< float* >(this->data),pDims[0], pDims[1]);
    // --> Use copy constructor instead --> slower performance but higher memory management reliability
    MatrixXf p_Matrix(Map<MatrixXf>( static_cast< float* >(this->data),pDims[0], pDims[1]));

    delete pDims;

    return p_Matrix;
}

} // NAMESPACE

#endif // FIFF_TAG_H
