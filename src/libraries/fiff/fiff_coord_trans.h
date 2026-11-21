//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_coord_trans.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Jana Kiesel <jana.kiesel@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     August 2012
 * @brief    4x4 affine FIFF coordinate transform (FIFF_COORD_TRANS) annotated with source/destination coordinate-frame IDs.
 *
 * The FIFF coordinate-frame world is a small zoo (@c FIFFV_COORD_DEVICE,
 * @c FIFFV_COORD_HEAD, @c FIFFV_COORD_MRI, @c FIFFV_COORD_HPI,
 * @c FIFFV_COORD_RAS, ...) and any forward / inverse computation must
 * move points and orientations between them. @ref mne::FiffCoordTrans wraps one
 * labelled affine: the 3x3 rotation @c rot, the 3-vector translation
 * @c move, the precomputed inverse, and the integer @c from / @c to frame
 * identifiers from @ref fiff_constants.h.
 *
 * The class also publishes the small algebra needed at the call sites:
 * multiplication, inversion and applying the transform to point sets.
 * Surface-compatible with the @c mne.transforms.Transform object
 * in MNE-Python.
 */

#ifndef FIFF_COORD_TRANS_H
#define FIFF_COORD_TRANS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"
#include "fiff_stream.h"
#include "fiff_dir_node.h"
#include <base/error.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <expected>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <cstdint>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

// Forward declaration
class FiffTag;

//=============================================================================================================
/**
 * @brief Labelled 4x4 FIFF affine: source frame, destination frame, rotation, translation and cached inverse.
 *
 * Each transform carries its source (@c from) and destination (@c to)
 * @c FIFFV_COORD_* frame identifiers so the inverse and any chained
 * composition can be type-checked at runtime. The inverse is stored
 * alongside the forward transform because both directions are routinely
 * needed (device→head for head-position tracking, head→MRI for source
 * space registration, etc.).
 *
 * @snippet ex_fiff_api/main.cpp fiff_coord_trans_usage
 */
class FIFFSHARED_EXPORT FiffCoordTrans
{
public:
    //=========================================================================================================
    /**
     * Constructs the coordinate transformation descriptor.
     */
    FiffCoordTrans();

    //=========================================================================================================
    /**
     * Reads the first coordinate transform of a FIFF file, like read(); throws mne::Error if it cannot.
     *
     * @param[in] path   The file.
     */
    explicit FiffCoordTrans(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads the transform between two frames, like read(path, from, to); throws mne::Error if there is none.
     *
     * @param[in] path   The file.
     * @param[in] from   Source coordinate frame.
     * @param[in] to     Destination coordinate frame.
     */
    FiffCoordTrans(const std::filesystem::path& path, int from, int to);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffCoordTrans   Coordinate transformation description which should be copied.
     */
    FiffCoordTrans(const FiffCoordTrans& p_FiffCoordTrans);

    //=========================================================================================================
    /**
     * Copy assignment. All members are values, so the compiler-generated
     * member-wise assignment is correct. It has to be declared explicitly
     * because declaring the copy constructor and destructor above deprecates
     * the implicit one.
     *
     * @param[in] p_FiffCoordTrans   Coordinate transformation description which should be assigned.
     *
     * @return reference to this object.
     */
    FiffCoordTrans& operator=(const FiffCoordTrans& p_FiffCoordTrans) = default;

    //=========================================================================================================
    /**
     * Constructs a coordinate transformation from a rotation matrix and translation vector.
     *
     * @param[in] from   Source coordinate system.
     * @param[in] to     Destination coordinate system.
     * @param[in] rot    The forward transform (3×3 rotation matrix).
     * @param[in] move   The forward transform (3×1 translation vector).
     */
    FiffCoordTrans(int from, int to, const Eigen::Matrix3f& rot, const Eigen::Vector3f& move);

    //=========================================================================================================
    /**
     * Constructs a coordinate transformation from a 4×4 homogeneous matrix.
     *
     * @param[in] from      Source coordinate system.
     * @param[in] to        Destination coordinate system.
     * @param[in] matTrans  The 4×4 forward transform.
     * @param[in] bStandard   When true, enforce standard transform (last row = [0,0,0,1]). Defaults to false.
     */
    FiffCoordTrans(int from, int to, const Eigen::Matrix4f& matTrans, bool bStandard = false);

    //=========================================================================================================
    /**
     * Destroys the coordinate transformation descriptor.
     */
    ~FiffCoordTrans();

    //=========================================================================================================
    /**
     * Initializes the coordinate transformation descriptor.
     */
    void clear();

    //=========================================================================================================
    /**
     * Invert a coordinate transformation
     * (actual obsolete - cause trans and inverse are both stored)
     *
     * @return true if succeeded, false otherwise.
     */
    bool invert_transform();

    //=========================================================================================================
    /**
     * Returns true if coordinate transform contains no data.
     *
     * @return true if coordinate transform is empty.
     */
    inline bool empty() const
    {
        return this->from < 0;
    }

    //=========================================================================================================
    /**
     * @name Rotation/Translation Accessors
     * Provide Matrix3f/Vector3f views into the 4×4 homogeneous matrices,
     * matching the legacy FiffCoordTransOld member layout (rot, move, invrot, invmove).
     * Mutable overloads allow direct modification of the underlying matrix blocks.
     * @{
     */

    /**
     * Forward rotation (3×3 block of trans).
     *
     * @return Writable 3×3 block view of the upper-left rotation part of trans.
     */
    auto rot()
    {
        return trans.block<3, 3>(0, 0);
    }
    /**
     * @overload
     *
     * @return Read-only 3×3 block view of the upper-left rotation part of trans.
     */
    auto rot() const
    {
        return trans.block<3, 3>(0, 0);
    }

    /**
     * Forward translation (column 3 of trans).
     *
     * @return Writable 3×1 block view of the translation column of trans (meters).
     */
    auto move()
    {
        return trans.block<3, 1>(0, 3);
    }
    /**
     * @overload
     *
     * @return Read-only 3×1 block view of the translation column of trans (meters).
     */
    auto move() const
    {
        return trans.block<3, 1>(0, 3);
    }

    /**
     * Inverse rotation (3×3 block of invtrans).
     *
     * @return Writable 3×3 block view of the upper-left rotation part of invtrans.
     */
    auto invrot()
    {
        return invtrans.block<3, 3>(0, 0);
    }
    /**
     * @overload
     *
     * @return Read-only 3×3 block view of the upper-left rotation part of invtrans.
     */
    auto invrot() const
    {
        return invtrans.block<3, 3>(0, 0);
    }

    /**
     * Inverse translation (column 3 of invtrans).
     *
     * @return Writable 3×1 block view of the translation column of invtrans (meters).
     */
    auto invmove()
    {
        return invtrans.block<3, 1>(0, 3);
    }
    /**
     * @overload
     *
     * @return Read-only 3×1 block view of the translation column of invtrans (meters).
     */
    auto invmove() const
    {
        return invtrans.block<3, 1>(0, 3);
    }
    /** @} */

    //=========================================================================================================
    /**
     * Reads the first coordinate transform of a FIFF file, as mne.read_trans.
     *
     * @param[in] path   FIFF file holding the coordinate transform.
     *
     * @return The transform, or why the file holds none.
     */
    [[nodiscard]] static mne::Expected<FiffCoordTrans> read(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads the transform between two coordinate frames from a FIFF file; one stored the other way round is
     * returned inverted.
     *
     * @param[in] path   FIFF file holding the coordinate transform.
     * @param[in] from   Source coordinate frame.
     * @param[in] to     Destination coordinate frame.
     *
     * @return The transform, or why the file holds none between these frames.
     */
    [[nodiscard]] static mne::Expected<FiffCoordTrans> read(const std::filesystem::path& path, int from, int to);

    //=========================================================================================================
    /**
     * Reads a transform from a text file of four rows, rotation and translation in mm, as MNE-C writes it.
     *
     * @param[in] path   Text file; '#' starts a comment.
     * @param[in] from   Source coordinate frame.
     * @param[in] to     Destination coordinate frame.
     *
     * @return The transform with its translation in m, or why the file cannot be read.
     */
    [[nodiscard]] static mne::Expected<FiffCoordTrans> read_ascii(const std::filesystem::path& path, int from, int to);

    //=========================================================================================================
    /**
     * Applies the coordinate transform to given coordinates and returns the transformed coordinates
     *
     * @param[in] rr         The coordinates.
     * @param[in] do_move    Perform translation next to rotation yes/no.
     *
     * @return Transformed coordinates.
     */
    Eigen::MatrixX3f apply_trans(const Eigen::MatrixX3f& rr, bool do_move = true) const;

    //=========================================================================================================
    /**
     * Applies the inverse coordinate transform to given coordinates and returns the transformed coordinates
     *
     * @param[in] rr         The coordinates.
     * @param[in] do_move    Perform translation next to rotation yes/no.
     *
     * @return Transformed coordinates.
     */
    Eigen::MatrixX3f apply_inverse_trans(const Eigen::MatrixX3f& rr, bool do_move = true) const;

    //=========================================================================================================
    /**
     * Applies the coordinate transform to a single 3D point in place.
     *
     * @param[in,out] r      3-element float array to transform in place.
     * @param[in] t          The coordinate transformation to apply.
     * @param[in] do_move    If true, apply translation; if false, rotation only.
     */
    static void apply_trans(float r[3], const FiffCoordTrans& t, bool do_move);

    //=========================================================================================================
    /**
     * Applies the inverse coordinate transform to a single 3D point in place.
     *
     * @param[in,out] r      3-element float array to transform in place.
     * @param[in] t          The coordinate transformation whose inverse to apply.
     * @param[in] do_move    If true, apply translation; if false, rotation only.
     */
    static void apply_inverse_trans(float r[3], const FiffCoordTrans& t, bool do_move);

    //=========================================================================================================
    /**
     * Map coordinate frame integers to human-readable names
     *
     * @param[in] frame  The coordinate frame integer.
     *
     * @return Human readable form of the coordinate frame.
     */
    static std::string_view frame_name(int frame);

    //=========================================================================================================
    /**
     * Creates an identity transformation between two coordinate systems.
     *
     * @param[in] from   Source coordinate system.
     * @param[in] to     Destination coordinate system.
     *
     * @return An identity coordinate transformation.
     */
    static FiffCoordTrans identity(int from, int to);

    //=========================================================================================================
    /**
     * Returns a new inverted copy of this transformation (swaps from/to and trans/invtrans).
     *
     * @return The inverted transformation.
     */
    FiffCoordTrans inverted() const;

    //=========================================================================================================
    /**
     * Combines two coordinate transformations to yield a transform from @p from to @p to.
     * Automatically handles inversion and ordering of the input transforms.
     *
     * @param[in] from   Desired source coordinate system.
     * @param[in] to     Desired destination coordinate system.
     * @param[in] t1     First transformation.
     * @param[in] t2     Second transformation.
     *
     * @return The combined transformation, or an empty transform on failure.
     */
    static FiffCoordTrans combine(int from, int to, const FiffCoordTrans& t1, const FiffCoordTrans& t2);

    //=========================================================================================================
    /**
     * Constructs a coordinate transformation from cardinal points (LPA, Nasion, RPA).
     *
     * @param[in] from   Source coordinate system.
     * @param[in] to     Destination coordinate system (in which the points are expressed).
     * @param[in] rL     Left auricular point (3 floats).
     * @param[in] rN     Nasion point (3 floats).
     * @param[in] rR     Right auricular point (3 floats).
     *
     * @return The coordinate transformation.
     */
    static FiffCoordTrans from_cardinal_points(int from, int to, const float* rL, const float* rN, const float* rR);

    //=========================================================================================================
    /**
     * @param[in] t      Fiff coordinate transform to which the inverse should be added.
     *
     * @return True when successful.
     */
    static bool add_inverse(FiffCoordTrans& t);

    //=========================================================================================================
    /**
     * Prints the coordinate transform.
     */
    void print() const;

    //=========================================================================================================
    /**
     * Writes the transform to a new FIFF file, as mne.write_trans.
     *
     * @param[in] path   FIFF file to create.
     *
     * @return Nothing, or why the file cannot be written.
     */
    [[nodiscard]] mne::Expected<void> write(const std::filesystem::path& path) const;

    //=========================================================================================================
    /**
     * @brief Writes the transformation to a FIFF stream.
     *
     * @param[in] stream   Pointer to the FIFF stream to write to.
     */
    void write_to_stream(FiffStream& stream) const;

    //=========================================================================================================
    /**
     * Size of the old struct (fiffCoordTransRec) 26*int = 26*4 = 104
     *
     * @return the size of the old struct fiffCoordTransRec.
     */
    inline static std::int32_t storage_size();

    //========================================================================================================
    /**
     * Overloaded == operator to compare an object to this instance.
     *
     * @param[in] a    The object which should be compared to.
     * @param[in] b Right-hand transform.
     *
     * @return true if equal, false otherwise.
     */
    friend bool operator==(const FiffCoordTrans& a, const FiffCoordTrans& b);

    //========================================================================================================
    /**
     * Calculate rotation as angle between two rotation matrices
     *
     * @param[in]  mTransDest  The destination transformation matrix.
     *
     * @return The angle of rotation between two rotation matrices in degrees.
     */
    float angle_to(const Eigen::MatrixX4f& mTransDest) const;

    //========================================================================================================
    /**
     * Calculate translation between two rotation matrices in meter
     *
     * @param[in]  mTransDest   The destination transformation matrix.
     *
     * @return The translation between two rotation matrices in m.
     */
    float translation_to(const Eigen::MatrixX4f& mTransDest) const;

    //========================================================================================================
    /**
     * Reads a coordinate transform from a FIFF tag.
     *
     * @param[in] tag   The FIFF tag to read from.
     *
     * @return The coordinate transform, or an empty transform if the tag is invalid.
     */
    static FiffCoordTrans from_tag(const FiffTag& tag);

    //========================================================================================================
    /**
     * Reads a coordinate transform from a FIFF stream node.
     *
     * @param[in] stream   The FIFF stream.
     * @param[in] node     The directory node to search.
     * @param[in] from     Expected source coordinate system.
     * @param[in] to       Expected destination coordinate system.
     *
     * @return The coordinate transform, inverted if stored the other way round, or NotFound.
     */
    [[nodiscard]] static mne::Expected<FiffCoordTrans> read_from_node(FiffStream& stream,
                                                                      const std::shared_ptr<FiffDirNode>& node,
                                                                      int from, int to);

    //========================================================================================================
    /**
     * Performs a Procrustes alignment between two sets of corresponding 3D points.
     *
     * @param[in] from_frame   Source coordinate frame ID.
     * @param[in] to_frame     Destination coordinate frame ID.
     * @param[in] fromp        Point locations in the source frame (np×3 matrix).
     * @param[in] top          Point locations in the destination frame (np×3 matrix).
     * @param[in] w            Weights (np elements), or empty vector for equal weight.
     * @param[in] max_diff     Maximum allowed difference between transformed and target.
     *
     * @return The alignment transform, or an empty transform on failure.
     */
    static FiffCoordTrans procrustes_align(int from_frame, int to_frame,
                                           const Eigen::MatrixXf& fromp,
                                           const Eigen::MatrixXf& top,
                                           const Eigen::VectorXf& w,
                                           float max_diff);

public:
    fiff_int_t from;                                       /**< Source coordinate system. */
    fiff_int_t to;                                         /**< Destination coordinate system. */
    Eigen::Matrix<float, 4, 4, Eigen::DontAlign> trans;    /**< The forward transform. */
    Eigen::Matrix<float, 4, 4, Eigen::DontAlign> invtrans; /**< The inverse transform. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline std::int32_t FiffCoordTrans::storage_size()
{
    // On-disk layout: from, to, rot[3][3], move[3], invrot[3][3], invmove[3]
    // (C++ class uses Matrix4f but on-disk stores 3x3 rot + 3-vec separately)
    return 2 * sizeof(fiff_int_t) + 2 * 12 * sizeof(fiff_float_t);
}

//=============================================================================================================

inline bool operator==(const FiffCoordTrans& a, const FiffCoordTrans& b)
{
    return (a.from == b.from &&
            a.to == b.to &&
            a.trans.isApprox(b.trans, 0.0001f) &&
            a.invtrans.isApprox(b.invtrans, 0.0001f));
}
} // NAMESPACE

#endif // FIFF_COORD_TRANS_H
