//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_tag.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     August 2012
 * @brief    FIFF tag: the 16-byte tag header (kind, type, size, next) plus its decoded payload.
 *
 * A FIFF tag is the atomic unit of the format: a fixed 16-byte header
 * (@c kind, @c type, @c size, @c next) followed by @c size bytes of
 * payload whose interpretation depends on the type descriptor. The upper
 * byte of @c type selects the fundamental structure — scalar
 * (@c FIFFFS_SCALAR == 0x00) or multidimensional matrix
 * (@c FIFFFS_MATRIX == 0x40) — and the matrix variants additionally carry
 * a coding flag in the low bits selecting dense (@c MATRIX_CODING_DENSE),
 * column-compressed sparse (@c MATRIX_CODING_CCS) or row-compressed
 * sparse (@c MATRIX_CODING_RCS) storage. @ref mne::FiffTag is the C++ wrapper
 * that holds the header fields, the raw payload bytes and a small set of
 * typed accessors (@c to_int, @c to_float, @c to_float_matrix, @c to_coord_trans,
 * @c to_ch_info, ...) that decode the payload on demand into the relevant
 * mne_fiff class.
 *
 * The header also re-publishes the @c IS_MATRIX / @c MATRIX_CODING_*
 * bitmasks and the @c NATIVE_ENDIAN macro driven by the OS-detection
 * ladder above so callers can compare @c FiffTag::type against a
 * canonical mask without duplicating the bit fiddling.
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

#define NATIVE_ENDIAN FIFFV_LITTLE_ENDIAN

#ifdef INTEL_X86_ARCH
#define NATIVE_ENDIAN FIFFV_LITTLE_ENDIAN
#endif

#ifdef BIG_ENDIAN_ARCH
#define NATIVE_ENDIAN FIFFV_BIG_ENDIAN
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
#include "fiff_dig_point.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <string>
#include <Eigen/Core>
#include <Eigen/SparseCore>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>
#include <memory>
#include <vector>
#include <cstdint>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

class FiffStream;
class FiffDirNode;

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================
//
//   The magic hexadecimal values
//
inline constexpr auto IS_MATRIX = 0xFFFF0000;           /**< Is Matrix encoding. ffff0000. */
inline constexpr auto MATRIX_CODING_DENSE = 0x00004000; /**< MATRIX_CODING_DENSE encoding. 4000. */
inline constexpr auto MATRIX_CODING_CCS = 0x00004010;   /**< MATRIX_CODING_CCS encoding. 4010. */
inline constexpr auto MATRIX_CODING_RCS = 0x00004020;   /**< MATRIX_CODING_RCS encoding. 4020. */
inline constexpr auto DATA_TYPE = 0x0000FFFF;           /**< DATA_TYPE encoding 0000ffff. */

//=============================================================================================================
/**
 * @brief FIFF tag: 16-byte header (kind, type, size, next) plus payload, with typed decoders for every FIFFT_* type.
 *
 * Holds the four header fields exactly as stored on disk and the raw
 * payload bytes. Decoders convert the payload to the
 * appropriate C++ representation: scalars via @c to_int / @c to_float /
 * @c to_double, dense and sparse matrices via @c to_float_matrix /
 * @c to_sparse_float_matrix, and the structured FIFF records into their
 * class wrappers (@c to_fiff_id, @c to_coord_trans, @c to_ch_info,
 * @c to_dig_point, @c to_dir_entry, ...). Used by @ref FiffStream both for
 * streaming reads and for assembling tags before write_tag.
 *
 * @snippet ex_fiff_api/main.cpp fiff_stream_usage
 */
class FIFFSHARED_EXPORT FiffTag
{
public:
    //=========================================================================================================
    /**
     * Constructs a default FiffTag.
     */
    FiffTag();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffTag   FiffTag to copy.
     */
    FiffTag(const FiffTag& p_FiffTag) = default;

    //=========================================================================================================
    /**
     * Move constructor.
     *
     * @param[in] other   FiffTag to move from; left in a valid but unspecified state.
     */
    FiffTag(FiffTag&& other) = default;

    //=========================================================================================================
    /**
     * Copy assignment operator.
     *
     * @param[in] other   FiffTag to copy.
     *
     * @return Reference to this object.
     */
    FiffTag& operator=(const FiffTag& other) = default;

    //=========================================================================================================
    /**
     * Move assignment operator.
     *
     * @param[in] other   FiffTag to move from; left in a valid but unspecified state.
     *
     * @return Reference to this object.
     */
    FiffTag& operator=(FiffTag&& other) = default;

    //=========================================================================================================
    /**
     * Destroys the FiffTag.
     */
    ~FiffTag() = default;

    //=========================================================================================================
    /**
     * Provides information about matrix coding
     *
     * @return Matrix coding.
     */
    fiff_int_t matrix_coding() const;

    //=========================================================================================================
    /**
     * Provides information if tag contains a matrix
     *
     * @return true if tag contains a matrix.
     */
    bool is_matrix() const;

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
    bool matrix_dimensions(std::int32_t& p_ndim, std::vector<std::int32_t>& p_Dims) const;

    //=========================================================================================================
    /**
     * Provides the tag type information
     *
     * @return the tag type.
     */
    fiff_int_t data_type() const;

    //=========================================================================================================
    /**
     * Type information as a string
     *
     * @return the tag type information.
     */
    std::string info() const;

    //
    // Simple types
    //
    //=========================================================================================================
    /**
     * to Byte
     * Fast access; Data are deleted if tag gets deleted, and vice versa
     *
     * @return type cast of the tag data pointer.
     */
    inline const std::uint8_t* to_byte() const;

    //=========================================================================================================
    /**
     * to unsigned Short
     * Fast access; Data are deleted if tag gets deleted, and vice versa
     *
     * @return type cast of the tag data pointer.
     */
    inline const std::uint16_t* to_unsigned_short() const;

    //=========================================================================================================
    /**
     * to Short
     * Fast access; Data are deleted if tag gets deleted, and vice versa
     *
     * @return type cast of the tag data pointer.
     */
    inline const std::int16_t* to_short() const;

    //=========================================================================================================
    /**
     * to Int
     * Fast access; Data are deleted if tag gets deleted, and vice versa
     *
     * @return type cast of the tag data pointer.
     */
    inline const std::uint32_t* to_unsigned_int() const;

    //=========================================================================================================
    /**
     * to Int
     * Fast access; Data are deleted if tag gets deleted, and vice versa
     *
     * @return type cast of the tag data pointer.
     */
    const std::int32_t* to_int() const;

    //=========================================================================================================
    /**
    * to Julian
    * Fast access; Data are deleted if tag gets deleted, and vice versa
    *
    * @return type cast of the tag data pointer.
    */
    const std::int32_t* to_julian() const;

    //=========================================================================================================
    /**
    * to Float
    * Fast access; Data are deleted if tag gets deleted, and vice versa
    *
    * @return type cast of the tag data pointer.
    */
    inline const float* to_float() const;

    //=========================================================================================================
    /**
     * to Double
     * Fast access; Data are deleted if tag gets deleted, and vice versa
     *
     * @return type cast of the tag data pointer.
     */
    inline const double* to_double() const;

    //=========================================================================================================
    /**
     * to String
     *
     * @return converts data to a string.
     */
    inline std::string to_string() const;

    //=========================================================================================================
    /**
     * to DauPack16
     * Fast access; Data are deleted if tag gets deleted, and vice versa
     *
     * @return type cast of the tag data pointer.
     */
    inline const std::int16_t* to_dau_pack16() const;

    //
    // Structures
    //
    //=========================================================================================================
    /**
     * to fiff ID
     *
     * @return Fiff universially unique identifier.
     */
    inline FiffId to_fiff_id() const;

    //=========================================================================================================
    /**
     * to fiff DIG POINT
     *
     * @return Fiff point descriptor.
     */
    inline FiffDigPoint to_dig_point() const;

    //=========================================================================================================
    /**
     * to fiff COORD TRANS
     *
     * @return Fiff coordinate transformation descriptor.
     */
    inline FiffCoordTrans to_coord_trans() const;

    //=========================================================================================================
    /**
     * to fiff CH INFO
     *
     * @return Fiff channel info descriptor.
     */
    inline FiffChInfo to_ch_info() const;

    //=========================================================================================================
    /**
     * to fiff DIR ENTRY
     *
     * @return List of directory entry descriptors.
     */
    inline std::vector<FiffDirEntry> to_dir_entry() const;

    //
    // MATRIX
    //
    //=========================================================================================================
    /**
     * to fiff FIFFT INT MATRIX
     *
     * @return Integer matrix.
     */
    Eigen::MatrixXi to_int_matrix() const;

    //=========================================================================================================
    /**
     * to fiff FIFFT FLOAT MATRIX
     *
     * parses a fiff float matrix
     *
     * @return the parsed float matrix.
     */
    Eigen::MatrixXf to_float_matrix() const;

    //=========================================================================================================
    /**
     * to sparse fiff FIFFT FLOAT MATRIX
     *
     * parses a sparse fiff float matrix and returns a double sparse matrix to make sure only double is used
     *
     * @return a sparse double matrix which is newly created from the parsed float; empty if the tag is
     *         no valid sparse float matrix (see FiffSparseMatrix::fiff_get_float_sparse_matrix).
     */
    Eigen::SparseMatrix<double> to_sparse_float_matrix() const;

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
     *
     * @param[in, out] tag   Matrix tag whose dimensions, indices and data are byte-swapped in place; non-matrix tags are left untouched.
     */
    static void convert_matrix_from_file_data(FiffTag& tag);

    //=========================================================================================================
    /**
     * Convert matrix data before writing to a file inside a fiff tag
     *
     * @param[in, out] tag    matrix data to convert.
     */
    static void convert_matrix_to_file_data(FiffTag& tag);

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
    static void convert_tag_data(FiffTag& tag, int from_endian, int to_endian);

    //
    // from fiff_type_spec.c
    //
    //=========================================================================================================
    /**
     * These return information about a fiff type.
     *
     * @param[in] type   The fiff data type to query.
     *
     * @return fiff type.
     */
    static fiff_int_t fiff_type_fundamental(fiff_int_t type);

    //=========================================================================================================
    /**
     * These return information about fiff type base.
     *
     * @param[in] type   The fiff data type to query.
     *
     * @return fiff type.
     */
    static fiff_int_t fiff_type_base(fiff_int_t type);

    //=========================================================================================================
    /**
     * These return information about the matrix coding.
     *
     * @param[in] type   The fiff data type to query.
     *
     * @return matrix coding.
     */
    static fiff_int_t fiff_type_matrix_coding(fiff_int_t type);

    //=========================================================================================================
    /**
     * Size of the on-disk tag info header: kind + type + size + next = 4 x fiff_int_t.
     * This corresponds to the old sizeof(fiffTagRec) - sizeof(fiff_data_t *).
     *
     * @return the byte size of the on-disk tag info header.
     */
    inline static std::int32_t storage_size();

    //
    // Payload
    //
    /** @return The payload bytes. */
    std::byte* data() noexcept
    {
        return m_data.data();
    }
    const std::byte* data() const noexcept
    {
        return m_data.data();
    }

    /** @return The payload size in bytes, the tag's on-disk @c size field. */
    std::size_t size() const noexcept
    {
        return m_data.size();
    }

    /** @return Whether the payload is empty. */
    bool empty() const noexcept
    {
        return m_data.empty();
    }

    /**
     * Resizes the payload; new bytes are zero.
     *
     * @param[in] bytes   The new payload size in bytes.
     */
    void resize(std::size_t bytes)
    {
        m_data.resize(bytes);
    }

    /** @return The payload as a span. */
    std::span<std::byte> bytes() noexcept
    {
        return m_data;
    }
    std::span<const std::byte> bytes() const noexcept
    {
        return m_data;
    }

    /**
     * Replaces the payload with a copy of @p bytes.
     *
     * @param[in] bytes   The new payload.
     */
    void assign(std::span<const std::byte> bytes)
    {
        m_data.assign(bytes.begin(), bytes.end());
    }

public:
    fiff_int_t kind; /**< Tag number.
                             *   This defines the meaning of the item */
    fiff_int_t type; /**< Data type.
                             *   This defines the representation of the data. */
    fiff_int_t next; /**< Pointer to the next object.
                             *   Zero if the object follows
                             *   sequentially in file.
                             *   Negative at the end of file */

private:
    std::vector<std::byte> m_data; /**< The payload. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline std::int32_t FiffTag::storage_size()
{
    // On-disk layout: kind, type, size, next (4 x fiff_int_t)
    // The on-disk size field is the payload size, size()
    return sizeof(FiffTag::kind) + sizeof(FiffTag::type) + sizeof(fiff_int_t) /* size field on disk */
        + sizeof(FiffTag::next);
}

//=============================================================================================================
// Simple types
//=============================================================================================================

inline const std::uint8_t* FiffTag::to_byte() const
{
    if (this->is_matrix() || this->data_type() != FIFFT_BYTE)
        return nullptr;
    else
        return reinterpret_cast<const std::uint8_t*>(this->data());
}

//=============================================================================================================

inline const std::uint16_t* FiffTag::to_unsigned_short() const
{
    if (this->is_matrix() || this->data_type() != FIFFT_USHORT)
        return nullptr;
    else
        return reinterpret_cast<const std::uint16_t*>(this->data());
}

//=============================================================================================================

inline const std::int16_t* FiffTag::to_short() const
{
    if (this->is_matrix() || this->data_type() != FIFFT_SHORT)
        return nullptr;
    else
        return reinterpret_cast<const std::int16_t*>(this->data());
}

//=============================================================================================================

inline const std::uint32_t* FiffTag::to_unsigned_int() const
{
    if (this->is_matrix() || this->data_type() != FIFFT_UINT)
        return nullptr;
    else
        return reinterpret_cast<const std::uint32_t*>(this->data());
}

//=============================================================================================================

inline const float* FiffTag::to_float() const
{
    if (this->is_matrix() || this->data_type() != FIFFT_FLOAT)
        return nullptr;
    else
        return reinterpret_cast<const float*>(this->data());
}

//=============================================================================================================

inline const double* FiffTag::to_double() const
{
    if (this->is_matrix() || this->data_type() != FIFFT_DOUBLE)
        return nullptr;
    else
        return reinterpret_cast<const double*>(this->data());
}

//=============================================================================================================

inline std::string FiffTag::to_string() const
{
    if (this->is_matrix() || this->data_type() != FIFFT_STRING)
        return {};
    return std::string(reinterpret_cast<const char*>(data()), size());
}

//=============================================================================================================

inline const std::int16_t* FiffTag::to_dau_pack16() const
{
    if (this->is_matrix() || this->data_type() != FIFFT_DAU_PACK16)
        return nullptr;
    else
        return reinterpret_cast<const std::int16_t*>(this->data());
}

//=============================================================================================================
// Structures
//=============================================================================================================

inline FiffId FiffTag::to_fiff_id() const
{
    FiffId p_fiffID;
    if (this->is_matrix() || this->data_type() != FIFFT_ID_STRUCT || this->size() < static_cast<std::size_t>(FiffId::storage_size()))
        return p_fiffID;
    else {
        const auto* t_pInt32 = reinterpret_cast<const std::int32_t*>(this->data());

        p_fiffID.version = t_pInt32[0];
        p_fiffID.machid[0] = t_pInt32[1];
        p_fiffID.machid[1] = t_pInt32[2];
        p_fiffID.time.secs = t_pInt32[3];
        p_fiffID.time.usecs = t_pInt32[4];

        return p_fiffID;
    }
}

//=============================================================================================================

inline FiffDigPoint FiffTag::to_dig_point() const
{
    FiffDigPoint t_fiffDigPoint;
    if (this->is_matrix() || this->data_type() != FIFFT_DIG_POINT_STRUCT || this->size() < static_cast<std::size_t>(FiffDigPoint::storage_size()))
        return t_fiffDigPoint;
    else {
        auto* t_pInt32 = reinterpret_cast<const std::int32_t*>(this->data());

        t_fiffDigPoint.kind = t_pInt32[0];
        t_fiffDigPoint.ident = t_pInt32[1];

        auto* t_pFloat = reinterpret_cast<const float*>(this->data());
        t_fiffDigPoint.r[0] = t_pFloat[2];
        t_fiffDigPoint.r[1] = t_pFloat[3];
        t_fiffDigPoint.r[2] = t_pFloat[4];
        t_fiffDigPoint.coord_frame = 0;

        return t_fiffDigPoint;
    }
}

//=============================================================================================================

inline FiffCoordTrans FiffTag::to_coord_trans() const
{
    FiffCoordTrans p_FiffCoordTrans;
    if (this->is_matrix() || this->data_type() != FIFFT_COORD_TRANS_STRUCT || this->size() < static_cast<std::size_t>(FiffCoordTrans::storage_size()))
        return p_FiffCoordTrans;
    else {
        auto* t_pInt32 = reinterpret_cast<const std::int32_t*>(this->data());
        p_FiffCoordTrans.from = t_pInt32[0];
        p_FiffCoordTrans.to = t_pInt32[1];

        p_FiffCoordTrans.trans.setIdentity(4, 4);
        auto* t_pFloat = reinterpret_cast<const float*>(this->data());
        int count = 0;
        int r, c;
        for (r = 0; r < 3; ++r) {
            p_FiffCoordTrans.trans(r, 3) = t_pFloat[11 + r];
            for (c = 0; c < 3; ++c) {
                p_FiffCoordTrans.trans(r, c) = t_pFloat[2 + count];
                ++count;
            }
        }

        p_FiffCoordTrans.invtrans.setIdentity(4, 4);
        count = 0;
        for (r = 0; r < 3; ++r) {
            p_FiffCoordTrans.invtrans(r, 3) = t_pFloat[23 + r];
            for (c = 0; c < 3; ++c) {
                p_FiffCoordTrans.invtrans(r, c) = t_pFloat[14 + count];
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
inline FiffChInfo FiffTag::to_ch_info() const
{
    FiffChInfo p_FiffChInfo;

    if (this->is_matrix() || this->data_type() != FIFFT_CH_INFO_STRUCT || this->size() < static_cast<std::size_t>(FiffChInfo::storage_size()))
        return p_FiffChInfo;
    else {
        auto* t_pInt32 = reinterpret_cast<const std::int32_t*>(this->data());
        p_FiffChInfo.scanNo = t_pInt32[0];
        p_FiffChInfo.logNo = t_pInt32[1];
        p_FiffChInfo.kind = t_pInt32[2];
        auto* t_pFloat = reinterpret_cast<const float*>(this->data());
        p_FiffChInfo.range = t_pFloat[3];
        p_FiffChInfo.cal = t_pFloat[4];

        p_FiffChInfo.chpos.coil_type = t_pInt32[5];

        //
        //   Read the coil coordinate system definition
        //
        std::int32_t r;
        // r0
        for (r = 0; r < 3; ++r)
            p_FiffChInfo.chpos.r0[r] = t_pFloat[6 + r];
        // ex
        for (r = 0; r < 3; ++r)
            p_FiffChInfo.chpos.ex[r] = t_pFloat[6 + 3 + r];
        // ey
        for (r = 0; r < 3; ++r)
            p_FiffChInfo.chpos.ey[r] = t_pFloat[6 + 6 + r];
        // ez
        for (r = 0; r < 3; ++r)
            p_FiffChInfo.chpos.ez[r] = t_pFloat[6 + 9 + r];

        p_FiffChInfo.coord_frame = FIFFV_COORD_UNKNOWN;

        //
        //   Convert loc into a more useful format
        //
        if (p_FiffChInfo.kind == FIFFV_MEG_CH || p_FiffChInfo.kind == FIFFV_REF_MEG_CH) {
            p_FiffChInfo.coil_trans.setIdentity(4, 4);
            // r0
            for (r = 0; r < 3; ++r)
                p_FiffChInfo.coil_trans(r, 3) = p_FiffChInfo.chpos.r0[r];
            // ex
            for (r = 0; r < 3; ++r)
                p_FiffChInfo.coil_trans(r, 0) = p_FiffChInfo.chpos.ex[r];
            // ey
            for (r = 0; r < 3; ++r)
                p_FiffChInfo.coil_trans(r, 1) = p_FiffChInfo.chpos.ey[r];
            // ez
            for (r = 0; r < 3; ++r)
                p_FiffChInfo.coil_trans(r, 2) = p_FiffChInfo.chpos.ez[r];

            p_FiffChInfo.coord_frame = FIFFV_COORD_DEVICE;
        } else if (p_FiffChInfo.kind == FIFFV_EEG_CH) {
            if (p_FiffChInfo.chpos.ex.norm() > 0) {
                p_FiffChInfo.eeg_loc.block(0, 0, 3, 1) = p_FiffChInfo.chpos.r0.block(0, 0, 3, 1);
                p_FiffChInfo.eeg_loc.block(0, 1, 3, 1) = p_FiffChInfo.chpos.ex.block(0, 0, 3, 1);
            } else {
                p_FiffChInfo.eeg_loc.block(0, 0, 3, 1) = p_FiffChInfo.chpos.r0.block(0, 0, 3, 1);
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
        const auto* name = reinterpret_cast<const char*>(data()) + 80;
        p_FiffChInfo.ch_name.assign(name, std::find(name, name + 16, '\0'));

        return p_FiffChInfo;
    }
}

//=============================================================================================================

inline std::vector<FiffDirEntry> FiffTag::to_dir_entry() const
{
    std::vector<FiffDirEntry> entries;
    if (this->is_matrix() || this->data_type() != FIFFT_DIR_ENTRY_STRUCT) {
        return entries;
    }
    const auto* ints = reinterpret_cast<const std::int32_t*>(this->data());
    const auto count = size() / static_cast<std::size_t>(FiffDirEntry::storage_size());
    entries.reserve(count);
    for (std::size_t k = 0; k < count; ++k) {
        entries.push_back({ints[k * 4], ints[k * 4 + 1], ints[k * 4 + 2], ints[k * 4 + 3]});
    }
    return entries;
}

//=============================================================================================================
// MATRIX
//=============================================================================================================

} // NAMESPACE

#endif // FIFF_TAG_H
