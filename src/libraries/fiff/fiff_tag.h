//=============================================================================================================
/**
 * @file     fiff_tag.h
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
 * @brief    FiffTag class declaration, which provides fiff tag I/O and processing methods.
 *
 */

#ifndef FIFF_TAG_H
#define FIFF_TAG_H

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
#if defined(__linux) || defined(WIN32) || defined(__APPLE__)
#define INTEL_X86_ARCH
#endif

#endif
#endif

#define NATIVE_ENDIAN    FIFFV_LITTLE_ENDIAN

#ifdef  INTEL_X86_ARCH
#define NATIVE_ENDIAN    FIFFV_LITTLE_ENDIAN
#endif

#ifdef  BIG_ENDIAN_ARCH
#define NATIVE_ENDIAN    FIFFV_BIG_ENDIAN
#endif

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_constants.h"
#include "fiff_file.h"
#include "fiff_types.h"
#include "fiff_id.h"
#include "fiff_coord_trans.h"
#include "fiff_ch_info.h"
#include "fiff_ch_pos.h"
#include "fiff_dir_entry.h"
#include "fiff_tag.h"
#include "fiff_dig_point.h"

#include <iostream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QByteArray>
#include <QList>
#include <QSharedPointer>
#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

class FiffStream;
class FiffDirNode;

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================
//
//   The magic hexadecimal values
//
#define IS_MATRIX               0xFFFF0000          /**< Is Matrix encoding. ffff0000. */
#define MATRIX_CODING_DENSE     0x00004000          /**< MATRIX_CODING_DENSE encoding. 4000. */
#define MATRIX_CODING_CCS       0x00004010          /**< MATRIX_CODING_CCS encoding. 4010. */
#define MATRIX_CODING_RCS       0x00004020          /**< MATRIX_CODING_RCS encoding. 4020. */
#define DATA_TYPE               0x0000FFFF          /**< DATA_TYPE encoding 0000ffff. */

//const fiff_int_t IS_MATRIX           = 4294901760; /**< Is Matrix encoding. ffff0000. */
//const fiff_int_t MATRIX_CODING_DENSE = 16384;       /**< MATRIX_CODING_DENSE encoding. 4000. */
//const fiff_int_t MATRIX_CODING_CCS   = 16400;       /**< MATRIX_CODING_CCS encoding. 4010. */
//const fiff_int_t MATRIX_CODING_RCS   = 16416;       /**< MATRIX_CODING_RCS encoding. 4020. */
//const fiff_int_t DATA_TYPE           = 65535;       /**< DATA_TYPE encoding. ffff. */

//=============================================================================================================
/**
 * Tags are used in front of data items to tell what they are.
 *
 * @brief FIFF data tag
 */
class FIFFSHARED_EXPORT FiffTag : public QByteArray
{

public:
    typedef QSharedPointer<FiffTag> SPtr;            /**< Shared pointer type for FiffTag. */
    typedef QSharedPointer<const FiffTag> ConstSPtr; /**< Const shared pointer type for FiffTag. */

    //=========================================================================================================
    /**
     * ctor //ToDo add FiffStream to constructor and remove static implementations --> make them members
     */
    FiffTag();

    //=========================================================================================================
    /**
     * copy ctor //ToDo add FiffStream to constructor and remove static implementations --> make them members
     */
    FiffTag(const FiffTag* p_pFiffTag);

    //=========================================================================================================
    /**
     * Destroys the FiffTag.
     */
    virtual ~FiffTag();

    //=========================================================================================================
    /**
     * Provides information about matrix coding
     *
     * @return Matrix coding.
     */
    fiff_int_t getMatrixCoding() const;

    //=========================================================================================================
    /**
     * Provides information if tag contains a matrix
     *
     * @return true if tag contains a matrix.
     */
    bool isMatrix() const;

    //=========================================================================================================
    /**
     * Returns matrix dimensions
     * Refactors: fiff_get_matrix_dims (fiff_matrix.c)
     *
     * @param[out] p_ndim    number of dimensions.
     * @param[out] p_Dims    vector containing the size of each dimension.
     *
     * @return true if dimensions are available.
     */
    bool getMatrixDimensions(qint32& p_ndim, QVector<qint32>& p_Dims) const;

    //=========================================================================================================
    /**
     * Provides the tag type information
     *
     * @return the tag type.
     */
    fiff_int_t getType() const;

    //=========================================================================================================
    /**
     * Type information as a string
     *
     * @return the tag type information.
     */
    QString getInfo() const;

    //
    // Simple types
    //
    //=========================================================================================================
    /**
     * to Byte
     * Fast access; Data are deleted if tag gets deleted, and wise versa
     *
     * @return type cast of the tag data pointer.
     */
    inline quint8* toByte() const;

    //=========================================================================================================
    /**
     * to unsigned Short
     * Fast access; Data are deleted if tag gets deleted, and wise versa
     *
     * @return type cast of the tag data pointer.
     */
    inline quint16* toUnsignedShort() const;

    //=========================================================================================================
    /**
     * to Short
     * Fast access; Data are deleted if tag gets deleted, and wise versa
     *
     * @return type cast of the tag data pointer.
     */
    inline qint16* toShort() const;

    //=========================================================================================================
    /**
     * to Int
     * Fast access; Data are deleted if tag gets deleted, and wise versa
     *
     * @return type cast of the tag data pointer.
     */
    inline quint32* toUnsignedInt() const;

    //=========================================================================================================
    /**
     * to Int
     * Fast access; Data are deleted if tag gets deleted, and wise versa
     *
     * @return type cast of the tag data pointer.
     */
    inline qint32* toInt() const;

    //=========================================================================================================
    /**
    * to Julian
    * Fast access; Data are deleted if tag gets deleted, and wise versa
    *
    * @return type cast of the tag data pointer.
    */
    inline qint32* toJulian() const;

    //=========================================================================================================
    /**
    * to Float
    * Fast access; Data are deleted if tag gets deleted, and wise versa
    *
    * @return type cast of the tag data pointer.
    */
    inline float* toFloat() const;

    //=========================================================================================================
    /**
     * to Double
     * Fast access; Data are deleted if tag gets deleted, and wise versa
     *
     * @return type cast of the tag data pointer.
     */
    inline double* toDouble() const;

    //=========================================================================================================
    /**
     * to String
     *
     * @return converts data to a string.
     */
    inline QString toString() const;

    //=========================================================================================================
    /**
     * to DauPack16
     * Fast access; Data are deleted if tag gets deleted, and wise versa
     *
     * @return type cast of the tag data pointer.
     */
    inline qint16* toDauPack16() const;

    //=========================================================================================================
    /**
     * to complex float
     * Allocates new memory - pointer has to be deleted ater use
     *
     * @return complex float array.
     */
//    inline std::complex<float>* toComplexFloat();

    //=========================================================================================================
    /**
     * to complex double
     * Allocates new memory - pointer has to be deleted ater use
     *
     * @return complex double array.
     */
//    inline std::complex<double>* toComplexDouble();

    //
    // Structures
    //
    //=========================================================================================================
    /**
     * to fiff ID
     *
     * @return Fiff universially unique identifier.
     */
    inline FiffId toFiffID() const;

    //=========================================================================================================
    /**
     * to fiff DIG POINT
     *
     * @return Fiff point descriptor.
     */
    inline FiffDigPoint toDigPoint() const;

    //=========================================================================================================
    /**
     * to fiff COORD TRANS
     *
     * @return Fiff coordinate transformation descriptor.
     */
    inline FiffCoordTrans toCoordTrans() const;

    //=========================================================================================================
    /**
     * to fiff CH INFO
     *
     * @return Fiff channel info descriptor.
     */
    inline FiffChInfo toChInfo() const;

//    //=========================================================================================================
//    /**
//    * to fiff OLD PACK
//    */
//    inline fiff_coord_trans_t toCoordTrans() const;

    //=========================================================================================================
    /**
     * to fiff DIR ENTRY
     *
     * @return List of directory entry descriptors.
     */
    inline QList< QSharedPointer<FiffDirEntry> > toDirEntry() const;

    //
    // MATRIX
    //
    //=========================================================================================================
    /**
     * to fiff FIFFT INT MATRIX
     *
     * @return Integer matrix.
     */
    inline Eigen::MatrixXi toIntMatrix() const;

    //=========================================================================================================
    /**
     * to fiff FIFFT FLOAT MATRIX
     *
     * parses a fiff float matrix
     *
     * @return the parsed float matrix.
     */
    inline Eigen::MatrixXf toFloatMatrix() const;

    //=========================================================================================================
    /**
     * to sparse fiff FIFFT FLOAT MATRIX
     *
     * parses a sparse fiff float matrix and returns a double sparse matrix to make sure only double is used
     *
     * @return a sparse double matrix wich is newly created from the parsed float.
     */
    inline Eigen::SparseMatrix<double> toSparseFloatMatrix() const;

    //
    //from fiff_combat.c
    //

    //=========================================================================================================
    /**
     * Convert coil position descriptor
     *
     * @param[in, out] pos    coil position descriptor to convert.
     */
    static void convert_ch_pos(FiffChPos* pos);

    //TODO: Check if this is of interest
//    static void fiff_convert_tag_info(FiffTag*& tag);

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
    //TODO: Check if this is of interest
//    void fiff_convert_tag_data(fiffTag tag, int from_endian, int to_endian)

    //=========================================================================================================
    /**
     * Convert matrix data read from a file inside a fiff tag
     *
     * @param[in, out] tag    matrix data to convert.
     */
    static void convert_matrix_from_file_data(FiffTag::SPtr tag);

    //=========================================================================================================
    /**
     * Convert matrix data before writing to a file inside a fiff tag
     *
     * @param[in, out] tag    matrix data to convert.
     */
    static void convert_matrix_to_file_data(FiffTag::SPtr tag);

    //
    // Data type conversions for the little endian systems.
    //
    //=========================================================================================================
    /**
     * Machine dependent data type conversions (tag info only)
     *
     * from_endian defines the byte order of the input
     * to_endian   defines the byte order of the output
     *
     * Either of these may be specified as FIFFV_LITTLE_ENDIAN, FIFFV_BIG_ENDIAN, or FIFFV_NATIVE_ENDIAN.
     * The last choice means that the native byte order value will be substituted here before proceeding
     *
     * @param[in, out] tag       matrix data to convert.
     * @param[in] from_endian    from endian encoding.
     * @param[in] to_endian      to endian encoding.
     */
    static void convert_tag_data(FiffTag::SPtr tag, int from_endian, int to_endian);

    //
    // from fiff_type_spec.c
    //
    //=========================================================================================================
    /**
     * These return information about a fiff type.
     *
     * @return fiff type.
     */
    static fiff_int_t fiff_type_fundamental(fiff_int_t type);

    //=========================================================================================================
    /**
     * These return information about fiff type base.
     *
     * @return fiff type.
     */
    static fiff_int_t fiff_type_base(fiff_int_t type);

    //=========================================================================================================
    /**
     * These return information about the matrix coding.
     *
     * @return matrix coding.
     */
    static fiff_int_t fiff_type_matrix_coding(fiff_int_t type);

public:
    fiff_int_t  kind;       /**< Tag number.
                             *   This defines the meaning of the item */
    fiff_int_t  type;       /**< Data type.
                             *   This defines the representation of the data. */
//    fiff_int_t  size;       /**< Size of the data.
//                             *   The size is given in bytes and defines the
//                             *   total size of the data. */
    fiff_int_t  next;       /**< Pointer to the next object.
                             *   Zero if the object follows
                             *   sequentially in file.
                             *   Negative at the end of file */
//    QByteArray* data;       /**< Pointer to the data.
//                             *   This point to the data read or to be written. */
private:
//    std::complex<float>* m_pComplexFloatData;

//    std::complex<double>* m_pComplexDoubleData;

// ### OLD STRUCT ###
//typedef struct _fiffTagRec {
//    fiff_int_t  kind;		/**< Tag number.
//                 *   This defines the meaning of the item */
//    fiff_int_t  type;		/**< Data type.
//                 *   This defines the reperentation of the data. */
//    fiff_int_t  size;		/**< Size of the data.
//                 *   The size is given in bytes and defines the
//                 *   total size of the data. */
//    fiff_int_t  next;		/**< Pointer to the next object.
//                 *   Zero if the object follows
//                 *   sequentially in file.
//                 *   Negative at the end of file */
//    fiff_data_t *data;		/**< Pointer to the data.
//                 *   This point to the data read or to be written. */
//} *fiffTag,fiffTagRec;   /**< FIFF data tag. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

//=============================================================================================================
// Simple types
//=============================================================================================================

inline quint8* FiffTag::toByte() const
{
    if(this->isMatrix() || this->getType() != FIFFT_BYTE)
        return NULL;
    else
        return (quint8*)this->data();
}

//=============================================================================================================

inline quint16* FiffTag::toUnsignedShort() const
{
    if(this->isMatrix() || this->getType() != FIFFT_USHORT)
        return NULL;
    else
        return (quint16*)this->data();
}

//=============================================================================================================

inline qint16* FiffTag::toShort() const
{
    if(this->isMatrix() || this->getType() != FIFFT_SHORT)
        return NULL;
    else
        return (qint16*)this->data();
}

//=============================================================================================================

inline quint32* FiffTag::toUnsignedInt() const
{
    if(this->isMatrix() || this->getType() != FIFFT_UINT)
        return NULL;
    else
        return (quint32*)this->data();
}

//=============================================================================================================

inline qint32* FiffTag::toInt() const
{
    if(this->isMatrix() || this->getType() != FIFFT_INT) {
        printf("Expected an integer tag : %d (found data type %d instead)\n",this->kind,this->getType());
        return NULL;
    }
    else
        return (qint32*)this->data();
}

//=============================================================================================================

inline qint32* FiffTag::toJulian() const
{
    if(this->isMatrix() || this->getType() != FIFFT_JULIAN) {
        printf("Expected a julian tag : %d (found data type %d instead)\n",this->kind,this->getType());
        return NULL;
    }
    else
        return (qint32*)this->data();
}

//=============================================================================================================

inline float* FiffTag::toFloat() const
{
    if(this->isMatrix() || this->getType() != FIFFT_FLOAT)
        return NULL;
    else
        return (float*)this->data();
}

//=============================================================================================================

inline double* FiffTag::toDouble() const
{
    if(this->isMatrix() || this->getType() != FIFFT_DOUBLE)
        return NULL;
    else
        return (double*)this->data();
}

//=============================================================================================================

inline QString FiffTag::toString() const
{
    if(this->isMatrix() || this->getType() != FIFFT_STRING)
        return NULL;
    else
        return *this;
}

//=============================================================================================================

inline qint16* FiffTag::toDauPack16() const
{
    if(this->isMatrix() || this->getType() != FIFFT_DAU_PACK16)
        return NULL;
    else
        return (qint16*)this->data();
}

//=============================================================================================================

//inline std::complex<float>* FiffTag::toComplexFloat()
//{
//    if(this->isMatrix() || this->getType() != FIFFT_COMPLEX_FLOAT)
//        return NULL;
//    else if(this->m_pComplexFloatData == NULL)
//    {
//        float* t_pFloat = (float*)this->data();
//        qDebug() << "ToDo toComplexFloat";
//        //ToDo check this for arrays which contains more than one value
//        this->m_pComplexFloatData = new std::complex<float>(t_pFloat[0],t_pFloat[1]);
//    }
//    return m_pComplexFloatData;
//}

//=============================================================================================================

//inline std::complex<double>* FiffTag::toComplexDouble()
//{
//    if(this->isMatrix() || this->getType() != FIFFT_COMPLEX_DOUBLE)
//        return NULL;
//    else if(this->m_pComplexDoubleData == NULL)
//    {
//        double* t_pDouble = (double*)this->data();
//        qDebug() << "ToDo toComplexDouble";
//        //ToDo check this for arrays which contains more than one value
//        this->m_pComplexDoubleData = new std::complex<double>(t_pDouble[0],t_pDouble[1]);
//    }
//    return m_pComplexDoubleData;
//}

//=============================================================================================================
// Structures
//=============================================================================================================

inline FiffId FiffTag::toFiffID() const
{
    FiffId p_fiffID;
    if(this->isMatrix() || this->getType() != FIFFT_ID_STRUCT || this->data() == NULL)
        return p_fiffID;
    else
    {
        const qint32* t_pInt32 = (qint32*)this->data();

        p_fiffID.version = t_pInt32[0];
        p_fiffID.machid[0] = t_pInt32[1];
        p_fiffID.machid[1] = t_pInt32[2];
        p_fiffID.time.secs = t_pInt32[3];
        p_fiffID.time.usecs = t_pInt32[4];

        return p_fiffID;
    }
}

//=============================================================================================================

inline FiffDigPoint FiffTag::toDigPoint() const
{

    FiffDigPoint t_fiffDigPoint;
    if(this->isMatrix() || this->getType() != FIFFT_DIG_POINT_STRUCT || this->data() == NULL)
        return t_fiffDigPoint;
    else
    {
        qint32* t_pInt32 = (qint32*)this->data();
//            memcpy (&t_fiffDigPoint,this->data,this->size);

        t_fiffDigPoint.kind = t_pInt32[0];
        t_fiffDigPoint.ident = t_pInt32[1];

        float* t_pFloat = (float*)this->data();
        t_fiffDigPoint.r[0] = t_pFloat[2];
        t_fiffDigPoint.r[1] = t_pFloat[3];
        t_fiffDigPoint.r[2] = t_pFloat[4];
        t_fiffDigPoint.coord_frame = 0;

        return t_fiffDigPoint;
    }
}

//=============================================================================================================

inline FiffCoordTrans FiffTag::toCoordTrans() const
{

    FiffCoordTrans p_FiffCoordTrans;
    if(this->isMatrix() || this->getType() != FIFFT_COORD_TRANS_STRUCT || this->data() == NULL)
        return p_FiffCoordTrans;
    else
    {
        qint32* t_pInt32 = (qint32*)this->data();
        p_FiffCoordTrans.from = t_pInt32[0];
        p_FiffCoordTrans.to = t_pInt32[1];

        p_FiffCoordTrans.trans.setIdentity(4,4);
        float* t_pFloat = (float*)this->data();
        int count = 0;
        int r, c;
        for (r = 0; r < 3; ++r) {
            p_FiffCoordTrans.trans(r,3) = t_pFloat[11+r];
            for (c = 0; c < 3; ++c) {
                p_FiffCoordTrans.trans(r,c) = t_pFloat[2+count];
                ++count;
            }
        }

        p_FiffCoordTrans.invtrans.setIdentity(4,4);
        count = 0;
        for (r = 0; r < 3; ++r) {
            p_FiffCoordTrans.invtrans(r,3) = t_pFloat[23+r];
            for (c = 0; c < 3; ++c) {
                p_FiffCoordTrans.invtrans(r,c) = t_pFloat[14+count];
                ++count;
            }
        }

        return p_FiffCoordTrans;
    }
}

//=========================================================================================================
/**
 * to fiff CH INFO STRUCT
 */
inline FiffChInfo FiffTag::toChInfo() const
{
    FiffChInfo p_FiffChInfo;

    if(this->isMatrix() || this->getType() != FIFFT_CH_INFO_STRUCT || this->data() == NULL)
        return p_FiffChInfo;
    else
    {
        qint32* t_pInt32 = (qint32*)this->data();
        p_FiffChInfo.scanNo = t_pInt32[0];
        p_FiffChInfo.logNo = t_pInt32[1];
        p_FiffChInfo.kind = t_pInt32[2];
        float* t_pFloat = (float*)this->data();
        p_FiffChInfo.range = t_pFloat[3];
        p_FiffChInfo.cal = t_pFloat[4];
        p_FiffChInfo.chpos.coil_type = t_pInt32[5];

        //
        //   Read the coil coordinate system definition
        //
        qint32 r;
        // r0
        for (r = 0; r < 3; ++r)
            p_FiffChInfo.chpos.r0[r] = t_pFloat[6+r];
        // ex
        for (r = 0; r < 3; ++r)
            p_FiffChInfo.chpos.ex[r] = t_pFloat[6+3+r];
        // ey
        for (r = 0; r < 3; ++r)
            p_FiffChInfo.chpos.ey[r] = t_pFloat[6+6+r];
        // ez
        for (r = 0; r < 3; ++r)
            p_FiffChInfo.chpos.ez[r] = t_pFloat[6+9+r];

        p_FiffChInfo.coord_frame = FIFFV_COORD_UNKNOWN;

        //
        //   Convert loc into a more useful format
        //
        if (p_FiffChInfo.kind == FIFFV_MEG_CH || p_FiffChInfo.kind == FIFFV_REF_MEG_CH)
        {
            p_FiffChInfo.coil_trans.setIdentity(4,4);
            // r0
            for (r = 0; r < 3; ++r)
                p_FiffChInfo.coil_trans(r,3) = p_FiffChInfo.chpos.r0[r];
            // ex
            for (r = 0; r < 3; ++r)
                p_FiffChInfo.coil_trans(r,0) = p_FiffChInfo.chpos.ex[r];
            // ey
            for (r = 0; r < 3; ++r)
                p_FiffChInfo.coil_trans(r,1) = p_FiffChInfo.chpos.ey[r];
            // ez
            for (r = 0; r < 3; ++r)
                p_FiffChInfo.coil_trans(r,2) = p_FiffChInfo.chpos.ez[r];

            p_FiffChInfo.coord_frame = FIFFV_COORD_DEVICE;
        }
        else if (p_FiffChInfo.kind == FIFFV_EEG_CH)
        {
            if (p_FiffChInfo.chpos.ex.norm() > 0) {
                p_FiffChInfo.eeg_loc.block(0,0,3,1) = p_FiffChInfo.chpos.r0.block(0,0,3,1);
                p_FiffChInfo.eeg_loc.block(0,1,3,1) = p_FiffChInfo.chpos.ex.block(0,0,3,1);
            }
            else {
                p_FiffChInfo.eeg_loc.block(0,0,3,1) = p_FiffChInfo.chpos.r0.block(0,0,3,1);
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
        char* orig = (char*)this->data();
        p_FiffChInfo.ch_name = QString::fromUtf8(orig + 80).replace(" ","");

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

//=============================================================================================================

inline QList< QSharedPointer<FiffDirEntry> > FiffTag::toDirEntry() const
{
//         tag.data = struct('kind',{},'type',{},'size',{},'pos',{});
    QList< QSharedPointer<FiffDirEntry> > p_ListFiffDir;
    if(this->isMatrix() || this->getType() != FIFFT_DIR_ENTRY_STRUCT || this->data() == NULL)
        return p_ListFiffDir;
    else
    {
        QSharedPointer<FiffDirEntry> t_pFiffDirEntry;
        qint32* t_pInt32 = (qint32*)this->data();
        for (int k = 0; k < this->size()/16; ++k)
        {
            t_pFiffDirEntry = QSharedPointer<FiffDirEntry>(new FiffDirEntry);
            t_pFiffDirEntry->kind = t_pInt32[k*4];//fread(fid,1,'int32');
            t_pFiffDirEntry->type = t_pInt32[k*4+1];//fread(fid,1,'uint32');
            t_pFiffDirEntry->size = t_pInt32[k*4+2];//fread(fid,1,'int32');
            t_pFiffDirEntry->pos  = t_pInt32[k*4+3];//fread(fid,1,'int32');
            p_ListFiffDir.append(t_pFiffDirEntry);
        }
    }
    return p_ListFiffDir;
}

//=============================================================================================================
// MATRIX
//=============================================================================================================

//=============================================================================================================

inline Eigen::MatrixXi FiffTag::toIntMatrix() const
{
    if(!this->isMatrix() || this->getType() != FIFFT_INT || this->data() == NULL)
        return Eigen::MatrixXi();

    qint32 ndim;
    QVector<qint32> dims;
    this->getMatrixDimensions(ndim, dims);

    if (ndim != 2)
    {
        printf("Only two-dimensional matrices are supported at this time");
        return Eigen::MatrixXi();
    }

    //MatrixXd p_Matrix = Map<MatrixXd>( (float*)this->data,p_dims[0], p_dims[1]);
    // --> Use copy constructor instead --> slower performance but higher memory management reliability
    Eigen::MatrixXi p_Matrix(Eigen::Map<Eigen::MatrixXi>( (int*)this->data(),dims[0], dims[1]));

    return p_Matrix;
}

//=============================================================================================================

inline Eigen::MatrixXf FiffTag::toFloatMatrix() const
{
    if(!this->isMatrix() || this->getType() != FIFFT_FLOAT || this->data() == NULL)
        return Eigen::MatrixXf();//NULL;

    if (fiff_type_matrix_coding(this->type) != FIFFTS_MC_DENSE)
    {
        printf("Error in FiffTag::toFloatMatrix(): Matrix is not dense!\n");
        return Eigen::MatrixXf();//NULL;
    }

    qint32 ndim;
    QVector<qint32> dims;
    this->getMatrixDimensions(ndim, dims);

    if (ndim != 2)
    {
        printf("Only two-dimensional matrices are supported at this time");
        return Eigen::MatrixXf();//NULL;
    }

    // --> Use copy constructor instead --> slower performance but higher memory management reliability
    Eigen::MatrixXf p_Matrix((Eigen::Map<Eigen::MatrixXf>( (float*)this->data(),dims[0], dims[1])));

    return p_Matrix;
}

//=============================================================================================================

inline Eigen::SparseMatrix<double> FiffTag::toSparseFloatMatrix() const
{
    if(!this->isMatrix() || this->getType() != FIFFT_FLOAT || this->data() == NULL)
        return Eigen::SparseMatrix<double>();//NULL;

    if (fiff_type_matrix_coding(this->type) != FIFFTS_MC_CCS && fiff_type_matrix_coding(this->type) != FIFFTS_MC_RCS)
    {
        printf("Error in FiffTag::toSparseFloatMatrix(): Matrix is not sparse!\n");
        return Eigen::SparseMatrix<double>();//NULL;
    }

    qint32 ndim;
    QVector<qint32> dims;
    this->getMatrixDimensions(ndim, dims);

    if (ndim != 2)
    {
        printf("Only two-dimensional matrices are supported at this time");
        return Eigen::SparseMatrix<double>();//NULL;
    }

    qint32 nnz = dims[0];
    qint32 nrow = dims[1];
    qint32 ncol = dims[2];

    typedef Eigen::Triplet<double> T;
    std::vector<T> tripletList;
    tripletList.reserve(nnz);

    float *t_pFloat = (float*)this->data();
    int *t_pInt = (int*)this->data();
    qint32 offset1 = nnz;
    qint32 offset2 = 2*nnz;
    if (fiff_type_matrix_coding(this->type) == FIFFTS_MC_CCS)
    {
        //
        //    CCS
        //
        qWarning("Warning in FiffTag::toSparseFloatMatrix(): CCS has to be debugged - never done before.");
        qint32 p = 0;
        for(qint32 j = 0; j < ncol; ++j)
        {
            while( p < t_pInt[offset2+j+1])
            {
//                tripletList[p] = T(tripletList[p].row(), j, tripletList[p].value());
                tripletList.push_back(T(t_pInt[offset1+p], j, (double)(t_pFloat[p])));
                ++p;
            }
        }
    }
    else
    {
        //
        //    RCS
        //
        qint32 p = 0;
        for(qint32 j = 0; j < nrow; ++j)
        {
            while( p < t_pInt[offset2+j+1])
            {
//                tripletList[p] = T(j, tripletList[p].col(), tripletList[p].value());
                tripletList.push_back(T(j, t_pInt[offset1+p], (double)(t_pFloat[p])));
                ++p;
            }
        }
    }

//    std::cout << "Size: " << tripletList.size() << std::endl;
//    qint32 offsetTest = tripletList.size() - 10;
//    for(qint32 i = 0; i < 10; ++i)
//        std::cout << std::endl << tripletList[offsetTest + i].row() << " " << tripletList[offsetTest + i].col() << " " << tripletList[offsetTest + i].value();

    Eigen::SparseMatrix<double> p_Matrix(nrow, ncol);
    p_Matrix.setFromTriplets(tripletList.begin(), tripletList.end());

    return p_Matrix;
}
} // NAMESPACE

#endif // FIFF_TAG_H
