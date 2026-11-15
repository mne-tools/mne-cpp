//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_tag.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Florian Schlembach <fschlembach@web.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Implementation of @ref FiffTag: the 16-byte FIFF tag header plus payload, with typed decoders for every FIFFT_* type.
 *
 * Decoders convert the payload to the appropriate C++ representation
 * (scalars, dense and sparse matrices, structured FIFF records). Used by
 * @ref FiffStream both for streaming reads and for assembling tags
 * before @ref FiffStream::write_tag.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_tag.h"
#include <memory>
#include <base/log.h>

#include <vector>
#include "fiff_byte_swap.h"
#include "fiff_sparse_matrix.h"

#include <algorithm>
#include <complex>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <cstdint>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

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

fiff_int_t FiffTag::matrix_coding() const
{
    return IS_MATRIX & this->type;
}

//=============================================================================================================

bool FiffTag::is_matrix() const
{
    if (this->matrix_coding() != 0)
        return true;
    else
        return false;
}

//=============================================================================================================

bool FiffTag::matrix_dimensions(std::int32_t& p_ndim, std::vector<std::int32_t>& p_Dims) const
{
    p_Dims.clear();
    p_ndim = 0;
    if (!this->is_matrix() || this->data() == nullptr || this->size() < 4)
        return false;

    //
    // The dimensions trail the data: [nz,] dims..., ndim. A corrupt file must not make the readers
    // index outside the tag, so the trailer has to fit and a dense matrix has to fit before it.
    //
    const std::int32_t* t_pInt32 = reinterpret_cast<const std::int32_t*>(this->data());
    const qsizetype nInts = this->size() / 4;
    const std::int32_t ndim = t_pInt32[nInts - 1];
    const int coding = fiff_type_matrix_coding(this->type);
    const bool sparse = coding == FIFFTS_MC_CCS || coding == FIFFTS_MC_RCS;
    if (coding != FIFFTS_MC_DENSE && !sparse) {
        mne::Log::warning("Cannot handle other than dense or sparse matrices yet.");
        return false;
    }
    const std::int32_t nTrailer = ndim + (sparse ? 2 : 1);
    if (ndim <= 0 || ndim > FIFFC_MATRIX_MAX_DIM || nTrailer > nInts)
        return false;

    std::vector<std::int32_t> dims;
    std::int64_t nValues = 1;
    for (qsizetype i = nInts - nTrailer; i < nInts - 1; ++i) {
        if (t_pInt32[i] < 0)
            return false;
        dims.push_back(t_pInt32[i]);
        nValues = std::min<std::int64_t>(nValues * t_pInt32[i], this->size()); // capped: enough to compare, no overflow
    }
    if (!sparse) {
        const int base = this->data_type();
        const std::int64_t valueSize = (base == FIFFT_DOUBLE || base == FIFFT_COMPLEX_FLOAT) ? 8 : (base == FIFFT_COMPLEX_DOUBLE ? 16 : 4);
        if (nValues * valueSize > static_cast<std::int64_t>(nInts - nTrailer) * 4)
            return false;
    }

    p_ndim = ndim;
    p_Dims = dims;
    return true;
}

//=============================================================================================================

fiff_int_t FiffTag::data_type() const
{
    if (this->is_matrix()) {
        return DATA_TYPE & this->type;
    } else {
        return this->type;
    }
}

//=============================================================================================================

Eigen::SparseMatrix<double> FiffTag::to_sparse_float_matrix() const
{
    const std::unique_ptr<FiffSparseMatrix> mat = FiffSparseMatrix::fiff_get_float_sparse_matrix(*this);
    return mat ? Eigen::SparseMatrix<double>(mat->eigen().cast<double>()) : Eigen::SparseMatrix<double>();
}

//=============================================================================================================

std::string FiffTag::info() const
{
    std::string t_qStringInfo;

    if (this->is_matrix()) {
        switch (this->data_type()) {
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
    } else {
        switch (this->data_type()) {
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
            case FIFFT_JULIAN:
                t_qStringInfo = "Simple type FIFFT_JULIAN";
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
    pos->coil_type = swap_int(pos->coil_type);
    for (k = 0; k < 3; k++) {
        swap_floatp(&pos->r0[k]);
        swap_floatp(&pos->ex[k]);
        swap_floatp(&pos->ey[k]);
        swap_floatp(&pos->ez[k]);
    }
    return;
}

//=============================================================================================================

void FiffTag::convert_matrix_from_file_data(FiffTag& tag)
/*
 * Assumes that the input is in the non-native byte order and needs to be swapped to the other one
 */
{
    int ndim;
    int k;
    int *dimp, *data, kind, np, nz;
    float* fdata;
    double* ddata;
    const std::size_t tsize = tag.size();

    if (fiff_type_fundamental(tag.type) != FIFFTS_FS_MATRIX)
        return;
    if (tag.data() == nullptr)
        return;
    if (tsize < sizeof(fiff_int_t))
        return;

    dimp = (reinterpret_cast<fiff_int_t*>((tag.data()) + tag.size() - sizeof(fiff_int_t)));
    swap_intp(dimp);
    ndim = *dimp;
    if (fiff_type_matrix_coding(tag.type) == FIFFTS_MC_DENSE) {
        if (tsize < (ndim + 1) * sizeof(fiff_int_t))
            return;
        dimp = dimp - ndim;
        for (k = 0, np = 1; k < ndim; k++) {
            swap_intp(dimp + k);
            np = np * dimp[k];
        }
    } else {
        if (tsize < (ndim + 2) * sizeof(fiff_int_t))
            return;
        if (ndim > 2) /* Not quite sure what to do */
            return;
        dimp = dimp - ndim - 1;
        for (k = 0; k < ndim + 1; k++)
            swap_intp(dimp + k);
        nz = dimp[0];
        if (fiff_type_matrix_coding(tag.type) == FIFFTS_MC_CCS)
            np = nz + dimp[2] + 1; /* nz + n + 1 */
        else if (fiff_type_matrix_coding(tag.type) == FIFFTS_MC_RCS)
            np = nz + dimp[1] + 1; /* nz + m + 1 */
        else
            return; /* Don't know what to do */
        /*
         * Take care of the indices
        */
        for (data = reinterpret_cast<int*>(tag.data()) + nz, k = 0; k < np; k++)
            swap_intp(data + k);
        np = nz;
    }
    /*
     * Now convert data...
     */
    kind = fiff_type_base(tag.type);
    if (kind == FIFFT_INT) {
        for (data = reinterpret_cast<int*>(tag.data()), k = 0; k < np; k++)
            swap_intp(data + k);
    } else if (kind == FIFFT_FLOAT) {
        for (fdata = reinterpret_cast<float*>(tag.data()), k = 0; k < np; k++)
            swap_floatp(fdata + k);
    } else if (kind == FIFFT_DOUBLE) {
        for (ddata = reinterpret_cast<double*>(tag.data()), k = 0; k < np; k++)
            swap_doublep(ddata + k);
    } else if (kind == FIFFT_COMPLEX_FLOAT) {
        for (fdata = reinterpret_cast<float*>(tag.data()), k = 0; k < 2 * np; k++)
            swap_floatp(fdata + k);
    } else if (kind == FIFFT_COMPLEX_DOUBLE) {
        for (ddata = reinterpret_cast<double*>(tag.data()), k = 0; k < 2 * np; k++)
            swap_doublep(ddata + k);
    }
    return;
}

//=============================================================================================================

void FiffTag::convert_matrix_to_file_data(FiffTag& tag)
/*
 * Assumes that the input is in the NATIVE_ENDIAN byte order and needs to be swapped to the other one
 */
{
    int ndim;
    int k;
    int *dimp, *data, kind, np;
    float* fdata;
    double* ddata;
    const std::size_t tsize = tag.size();

    if (fiff_type_fundamental(tag.type) != FIFFTS_FS_MATRIX)
        return;
    if (tag.data() == nullptr)
        return;
    if (tsize < sizeof(fiff_int_t))
        return;

    dimp = (reinterpret_cast<fiff_int_t*>((tag.data()) + tag.size() - sizeof(fiff_int_t)));
    ndim = *dimp;
    swap_intp(dimp);

    if (fiff_type_matrix_coding(tag.type) == FIFFTS_MC_DENSE) {
        if (tsize < (ndim + 1) * sizeof(fiff_int_t))
            return;
        dimp = dimp - ndim;
        for (k = 0, np = 1; k < ndim; k++) {
            np = np * dimp[k];
            swap_intp(dimp + k);
        }
    } else {
        if (tsize < (ndim + 2) * sizeof(fiff_int_t))
            return;
        if (ndim > 2) /* Not quite sure what to do */
            return;
        dimp = dimp - ndim - 1;
        const int nz = dimp[0];
        if (fiff_type_matrix_coding(tag.type) == FIFFTS_MC_CCS)
            np = nz + dimp[2] + 1; /* nz + n + 1 */
        else if (fiff_type_matrix_coding(tag.type) == FIFFTS_MC_RCS)
            np = nz + dimp[1] + 1; /* nz + m + 1 */
        else
            return; /* Don't know what to do */
        for (k = 0; k < ndim + 1; k++)
            swap_intp(dimp + k);
        for (data = reinterpret_cast<int*>(tag.data()) + nz, k = 0; k < np; k++)
            swap_intp(data + k);
        np = nz;
    }
    /*
     * Now convert data...
     */
    kind = fiff_type_base(tag.type);
    if (kind == FIFFT_INT) {
        for (data = reinterpret_cast<int*>(tag.data()), k = 0; k < np; k++)
            swap_intp(data + k);
    } else if (kind == FIFFT_FLOAT) {
        for (fdata = reinterpret_cast<float*>(tag.data()), k = 0; k < np; k++)
            swap_floatp(fdata + k);
    } else if (kind == FIFFT_DOUBLE) {
        for (ddata = reinterpret_cast<double*>(tag.data()), k = 0; k < np; k++)
            swap_doublep(ddata + k);
    } else if (kind == FIFFT_COMPLEX_FLOAT) {
        for (fdata = reinterpret_cast<float*>(tag.data()), k = 0; k < 2 * np; k++)
            swap_floatp(fdata + k);
    } else if (kind == FIFFT_COMPLEX_DOUBLE) {
        for (ddata = reinterpret_cast<double*>(tag.data()), k = 0; k < 2 * np; k++)
            swap_doublep(ddata + k);
    }
    return;
}

//=============================================================================================================
void FiffTag::convert_tag_data(FiffTag& tag, int from_endian, int to_endian)
{
    // Element counts derived from tag->size(), which is a size type. A FIFF tag
    // never holds more than INT_MAX elements, so narrowing is safe, but it has
    // to be explicit: on Win64 size_t is 64-bit while int stays 32-bit.
    int np;
    int k, r;
    std::byte* offset;
    fiff_int_t* ithis;
    fiff_short_t* sthis;
    fiff_long_t* lthis;
    float* fthis;
    double* dthis;
    FiffDataRef* drthis;

    if (tag.data() == nullptr || tag.size() == 0)
        return;

    if (from_endian == FIFFV_NATIVE_ENDIAN)
        from_endian = NATIVE_ENDIAN;
    if (to_endian == FIFFV_NATIVE_ENDIAN)
        to_endian = NATIVE_ENDIAN;

    if (from_endian == to_endian)
        return;

    if (fiff_type_fundamental(tag.type) == FIFFTS_FS_MATRIX) {
        if (from_endian == NATIVE_ENDIAN)
            convert_matrix_to_file_data(tag);
        else
            convert_matrix_from_file_data(tag);
        return;
    }

    switch (tag.type) {
        case FIFFT_INT:
        case FIFFT_UINT:
        case FIFFT_JULIAN:
            np = static_cast<int>(tag.size() / sizeof(fiff_int_t));
            for (ithis = reinterpret_cast<fiff_int_t*>(tag.data()), k = 0; k < np; k++, ithis++)
                swap_intp(ithis);
            break;

        case FIFFT_LONG:
        case FIFFT_ULONG:
            np = static_cast<int>(tag.size() / sizeof(fiff_long_t));
            for (lthis = reinterpret_cast<fiff_long_t*>(tag.data()), k = 0; k < np; k++, lthis++)
                swap_longp(lthis);
            break;

        case FIFFT_SHORT:
        case FIFFT_DAU_PACK16:
        case FIFFT_USHORT:
            np = static_cast<int>(tag.size() / sizeof(fiff_short_t));
            for (sthis = reinterpret_cast<fiff_short_t*>(tag.data()), k = 0; k < np; k++, sthis++)
                *sthis = swap_short(*sthis);
            break;

        case FIFFT_FLOAT:
        case FIFFT_COMPLEX_FLOAT:
            np = static_cast<int>(tag.size() / sizeof(fiff_float_t));
            for (fthis = reinterpret_cast<fiff_float_t*>(tag.data()), k = 0; k < np; k++, fthis++)
                swap_floatp(fthis);
            break;

        case FIFFT_DOUBLE:
        case FIFFT_COMPLEX_DOUBLE:
            np = static_cast<int>(tag.size() / sizeof(fiff_double_t));
            for (dthis = reinterpret_cast<fiff_double_t*>(tag.data()), k = 0; k < np; k++, dthis++)
                swap_doublep(dthis);
            break;

        case FIFFT_OLD_PACK:
            fthis = reinterpret_cast<float*>(tag.data());
            /*
     * Offset and scale...
     */
            swap_floatp(fthis + 0);
            swap_floatp(fthis + 1);
            sthis = reinterpret_cast<short*>(fthis + 2);
            np = static_cast<int>((tag.size() - 2 * sizeof(float)) / sizeof(short));
            for (k = 0; k < np; k++, sthis++)
                *sthis = swap_short(*sthis);
            break;

        case FIFFT_DIR_ENTRY_STRUCT:
            np = static_cast<int>(tag.size() / FiffDirEntry::storage_size());
            for (k = 0; k < np; k++) {
                offset = tag.data() + k * FiffDirEntry::storage_size();
                ithis = reinterpret_cast<fiff_int_t*>(offset);
                ithis[0] = swap_int(ithis[0]); //kind
                ithis[1] = swap_int(ithis[1]); //type
                ithis[2] = swap_int(ithis[2]); //size
                ithis[3] = swap_int(ithis[3]); //pos
            }
            break;

        case FIFFT_ID_STRUCT:
            np = static_cast<int>(tag.size() / FiffId::storage_size());
            for (k = 0; k < np; k++) {
                offset = tag.data() + k * FiffId::storage_size();
                ithis = reinterpret_cast<fiff_int_t*>(offset);
                ithis[0] = swap_int(ithis[0]); //version
                ithis[1] = swap_int(ithis[1]); //machid[0]
                ithis[2] = swap_int(ithis[2]); //machid[1]
                ithis[3] = swap_int(ithis[3]); //time.secs
                ithis[4] = swap_int(ithis[4]); //time.usecs
            }
            break;

        case FIFFT_CH_INFO_STRUCT:

            np = static_cast<int>(tag.size() / FiffChInfo::storage_size());
            for (k = 0; k < np; k++) {
                offset = tag.data() + k * FiffChInfo::storage_size();
                ithis = reinterpret_cast<fiff_int_t*>(offset);
                fthis = reinterpret_cast<float*>(offset);

                ithis[0] = swap_int(ithis[0]); //scanno
                ithis[1] = swap_int(ithis[1]); //logno
                ithis[2] = swap_int(ithis[2]); //kind
                swap_floatp(&fthis[3]);        //range
                swap_floatp(&fthis[4]);        //cal
                ithis[5] = swap_int(ithis[5]); //coil_type
                for (r = 0; r < 12; ++r)
                    swap_floatp(&fthis[6 + r]);  //loc
                ithis[18] = swap_int(ithis[18]); //unit
                ithis[19] = swap_int(ithis[19]); //unit_mul
            }

            break;

        case FIFFT_CH_POS_STRUCT:

            np = static_cast<int>(tag.size() / FiffChPos::storage_size());
            for (k = 0; k < np; ++k) {
                offset = tag.data() + k * FiffChPos::storage_size();
                ithis = reinterpret_cast<fiff_int_t*>(offset);
                fthis = reinterpret_cast<float*>(offset);

                ithis[0] = swap_int(ithis[0]); //coil_type
                for (r = 0; r < 12; r++)
                    swap_floatp(&fthis[1 + r]); //r0, ex, ey, ez
            }

            break;

        case FIFFT_DIG_POINT_STRUCT:

            np = static_cast<int>(tag.size() / FiffDigPoint::storage_size());

            for (k = 0; k < np; k++) {
                offset = tag.data() + k * FiffDigPoint::storage_size();
                ithis = reinterpret_cast<fiff_int_t*>(offset);
                fthis = reinterpret_cast<float*>(offset);

                ithis[0] = swap_int(ithis[0]); //kind
                ithis[1] = swap_int(ithis[1]); //ident

                for (r = 0; r < 3; ++r)
                    swap_floatp(&fthis[2 + r]); //r
            }
            break;

        case FIFFT_COORD_TRANS_STRUCT:

            np = static_cast<int>(tag.size() / FiffCoordTrans::storage_size());

            for (k = 0; k < np; ++k) {
                offset = tag.data() + k * FiffCoordTrans::storage_size();
                ithis = reinterpret_cast<fiff_int_t*>(offset);
                fthis = reinterpret_cast<float*>(offset);

                ithis[0] = swap_int(ithis[0]);
                ithis[1] = swap_int(ithis[1]);

                for (r = 0; r < 24; ++r)
                    swap_floatp(&fthis[2 + r]);
            }
            break;

        case FIFFT_DATA_REF_STRUCT:
            np = static_cast<int>(tag.size() / FiffDataRef::storage_size());
            for (drthis = reinterpret_cast<FiffDataRef*>(tag.data()), k = 0; k < np; k++, drthis++) {
                drthis->type = swap_int(drthis->type);
                drthis->endian = swap_int(drthis->endian);
                drthis->size = swap_long(drthis->size);
                drthis->offset = swap_long(drthis->offset);
            }
            break;

        default:
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

//=============================================================================================================

const std::int32_t* FiffTag::to_int() const
{
    if (this->is_matrix() || this->data_type() != FIFFT_INT) {
        mne::Log::warning("Expected an integer tag : {} (found data type {} instead)\n", this->kind, this->data_type());
        return nullptr;
    } else
        return reinterpret_cast<const std::int32_t*>(this->data());
}

//=============================================================================================================

const std::int32_t* FiffTag::to_julian() const
{
    if (this->is_matrix() || this->data_type() != FIFFT_JULIAN) {
        mne::Log::warning("Expected a julian tag : {} (found data type {} instead)\n", this->kind, this->data_type());
        return nullptr;
    } else
        return reinterpret_cast<const std::int32_t*>(this->data());
}

//=============================================================================================================

Eigen::MatrixXi FiffTag::to_int_matrix() const
{
    if (!this->is_matrix() || this->data_type() != FIFFT_INT || this->data() == nullptr)
        return Eigen::MatrixXi();

    std::int32_t ndim;
    std::vector<std::int32_t> dims;
    this->matrix_dimensions(ndim, dims);

    if (ndim != 2) {
        mne::Log::warning("Only two-dimensional matrices are supported at this time");
        return Eigen::MatrixXi();
    }

    // Use copy constructor instead --> slower performance but higher memory management reliability
    Eigen::MatrixXi p_Matrix(Eigen::Map<const Eigen::MatrixXi>(reinterpret_cast<const int*>(this->data()), dims[0], dims[1]));

    return p_Matrix;
}

//=============================================================================================================

Eigen::MatrixXf FiffTag::to_float_matrix() const
{
    if (!this->is_matrix() || this->data_type() != FIFFT_FLOAT || this->data() == nullptr)
        return Eigen::MatrixXf(); //NULL;

    if (fiff_type_matrix_coding(this->type) != FIFFTS_MC_DENSE) {
        mne::Log::warning("Error in FiffTag::to_float_matrix(): Matrix is not dense!");
        return Eigen::MatrixXf();
    }

    std::int32_t ndim;
    std::vector<std::int32_t> dims;
    this->matrix_dimensions(ndim, dims);

    if (ndim != 2) {
        mne::Log::warning("Only two-dimensional matrices are supported at this time");
        return Eigen::MatrixXf();
    }

    // Use copy constructor instead --> slower performance but higher memory management reliability
    Eigen::MatrixXf p_Matrix(Eigen::Map<const Eigen::MatrixXf>(reinterpret_cast<const float*>(this->data()), dims[0], dims[1]));

    return p_Matrix;
}
