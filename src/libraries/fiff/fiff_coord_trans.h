//=============================================================================================================
/**
 * @file     fiff_coord_trans.h
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
 * @brief    FiffCoordTrans class declaration.
 *
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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QIODevice>
#include <QSharedPointer>

#include <memory>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

// Forward declaration
class FiffTag;

//=============================================================================================================
/**
 * Coordinate transformation description.
 *
 * @brief Coordinate transformation description.
 */
class FIFFSHARED_EXPORT FiffCoordTrans
{
public:
    typedef QSharedPointer<FiffCoordTrans> SPtr;            /**< Shared pointer type for FiffCoordTrans. */
    typedef QSharedPointer<const FiffCoordTrans> ConstSPtr; /**< Const shared pointer type for FiffCoordTrans. */
    typedef std::unique_ptr<FiffCoordTrans> UPtr;           /**< Unique pointer type for FiffCoordTrans. */
    typedef std::unique_ptr<const FiffCoordTrans> ConstUPtr;/**< Const unique pointer type for FiffCoordTrans. */

    //=========================================================================================================
    /**
     * Constructs the coordinate transformation descriptor.
     */
    FiffCoordTrans();

    //=========================================================================================================
    /**
     * Constructs a coordinate transformation, by reading from a IO device.
     *
     * @param[in] p_IODevice     IO device to read from the coordinate transform.
     */
    FiffCoordTrans(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffCoordTrans   Coordinate transformation description which should be copied.
     */
    FiffCoordTrans(const FiffCoordTrans &p_FiffCoordTrans);

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
    inline bool isEmpty() const
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

    /** Forward rotation (3×3 block of trans). */
    auto rot() { return trans.block<3,3>(0,0); }
    /** @overload */
    auto rot() const { return trans.block<3,3>(0,0); }

    /** Forward translation (column 3 of trans). */
    auto move() { return trans.block<3,1>(0,3); }
    /** @overload */
    auto move() const { return trans.block<3,1>(0,3); }

    /** Inverse rotation (3×3 block of invtrans). */
    auto invrot() { return invtrans.block<3,3>(0,0); }
    /** @overload */
    auto invrot() const { return invtrans.block<3,3>(0,0); }

    /** Inverse translation (column 3 of invtrans). */
    auto invmove() { return invtrans.block<3,1>(0,3); }
    /** @overload */
    auto invmove() const { return invtrans.block<3,1>(0,3); }
    /** @} */

    //=========================================================================================================
    /**
     * Reads a coordinate transform from a fif file
     *
     * @param[in] p_IODevice    A fiff IO device like a fiff QFile or QTCPSocket.
     * @param[out] p_Trans      A coordinate transform from a fif file.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool read(QIODevice& p_IODevice, FiffCoordTrans& p_Trans);

    //=========================================================================================================
    /**
     * Reads a specified coordinate transform from a FIFF file.
     *
     * @param[in] name   FIFF file path.
     * @param[in] from   Expected source coordinate system.
     * @param[in] to     Expected destination coordinate system.
     *
     * @return The coordinate transform, or an empty transform if not found.
     */
    static FiffCoordTrans readTransform(const QString& name, int from, int to);

    //=========================================================================================================
    /**
     * Reads the MRI → head coordinate transform from a FIFF file.
     *
     * @param[in] name   FIFF file path.
     *
     * @return The MRI→head transform, or an empty transform if not found.
     */
    static FiffCoordTrans readMriTransform(const QString& name);

    //=========================================================================================================
    /**
     * Reads the MEG device → head coordinate transform from a FIFF file.
     *
     * @param[in] name   FIFF file path.
     *
     * @return The device→head transform, or an empty transform if not found.
     */
    static FiffCoordTrans readMeasTransform(const QString& name);

    //=========================================================================================================
    /**
     * Reads a coordinate transform from an ASCII 4×3 matrix file (mm-to-m conversion applied).
     *
     * @param[in] name   ASCII file path.
     * @param[in] from   Source coordinate system.
     * @param[in] to     Destination coordinate system.
     *
     * @return The coordinate transform, or an empty transform on failure.
     */
    static FiffCoordTrans readTransformAscii(const QString& name, int from, int to);

    //=========================================================================================================
    /**
     * Reads the FreeSurfer head → MRI transform from an ASCII file.
     *
     * @param[in] name   ASCII file path.
     *
     * @return The head→MRI transform, or an empty transform on failure.
     */
    static FiffCoordTrans readFShead2mriTransform(const QString& name);

    //=========================================================================================================
    /**
     * TODO: dest       - The id of the destination coordinate system (FIFFV_COORD_...)
     *
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
    static QString frame_name (int frame);

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
    static FiffCoordTrans fromCardinalPoints(int from, int to, const float* rL, const float* rN, const float* rR);

    //=========================================================================================================
    /**
     * @param[in] t      Fiff coordinate transform to which the inverse should be added.
     *
     * @return True when successful.
     */
    static bool addInverse(FiffCoordTrans& t);

    //=========================================================================================================
    /**
     * Prints the coordinate transform.
     */
    void print() const;

    //=========================================================================================================
    /**
     * @brief Writes the transformation to file.
     *
     * @param[in] p_IODevice   IO device to write the transformation to.
     */
    void write(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * @brief Writes the transformation to a FIFF stream.
     *
     * @param[in] p_pStream   Pointer to the FIFF stream to write to.
     */
    void writeToStream(FiffStream* p_pStream);

    //=========================================================================================================
    /**
     * Size of the old struct (fiffCoordTransRec) 26*int = 26*4 = 104
     *
     * @return the size of the old struct fiffCoordTransRec.
     */
    inline static qint32 storageSize();

    //========================================================================================================
    /**
     * Overloaded == operator to compare an object to this instance.
     *
     * @param[in] object    The object which should be compared to.
     *
     * @return true if equal, false otherwise.
     */
    friend bool operator== (const FiffCoordTrans &a, const FiffCoordTrans &b);

    //========================================================================================================
    /**
     * Calculate rotation as angle between two rotation matrices
     *
     * @param[in]  mTransDest  The destination transformation matrix.
     *
     * @return The angle of rotation between two rotation matrices in degrees.
     */
    float angleTo(Eigen::MatrixX4f mTransDest);

    //========================================================================================================
    /**
     * Calculate translation between two rotation matrices in meter
     *
     * @param[in]  mTransDest   The destination transformation matrix.
     *
     * @return The translation between two rotation matrices in m.
     */
    float translationTo(Eigen::MatrixX4f mTransDest);

    //========================================================================================================
    /**
     * Reads a coordinate transform from a FIFF tag.
     *
     * @param[in] tag   The FIFF tag to read from.
     *
     * @return The coordinate transform, or an empty transform if the tag is invalid.
     */
    static FiffCoordTrans readFromTag(const QSharedPointer<FiffTag>& tag);

    //========================================================================================================
    /**
     * Reads a coordinate transform from a FIFF stream node.
     *
     * @param[in] stream   The FIFF stream.
     * @param[in] node     The directory node to search.
     * @param[in] from     Expected source coordinate system.
     * @param[in] to       Expected destination coordinate system.
     *
     * @return The coordinate transform, or an empty transform if not found.
     */
    static FiffCoordTrans readTransformFromNode(FiffStream::SPtr& stream,
                                                const FiffDirNode::SPtr& node,
                                                int from, int to);

    //========================================================================================================
    /**
     * Performs a Procrustes alignment between two sets of corresponding 3D points.
     *
     * @param[in] from_frame   Source coordinate frame ID.
     * @param[in] to_frame     Destination coordinate frame ID.
     * @param[in] fromp        Point locations in the source frame (np×3 C matrix).
     * @param[in] top          Point locations in the destination frame (np×3 C matrix).
     * @param[in] w            Optional weights (np elements), or nullptr for equal weight.
     * @param[in] np           Number of points.
     * @param[in] max_diff     Maximum allowed difference between transformed and target.
     *
     * @return The alignment transform, or an empty transform on failure.
     */
    static FiffCoordTrans procrustesAlign(int from_frame, int to_frame,
                                          float **fromp, float **top,
                                          float *w, int np,
                                          float max_diff);

public:
    fiff_int_t  from;   /**< Source coordinate system. */
    fiff_int_t  to;     /**< Destination coordinate system. */
    Eigen::Matrix<float, 4,4, Eigen::DontAlign>   trans;      /**< The forward transform. */
    Eigen::Matrix<float, 4,4, Eigen::DontAlign>   invtrans;   /**< The inverse transform. */

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 FiffCoordTrans::storageSize()
{
    // On-disk layout: from, to, rot[3][3], move[3], invrot[3][3], invmove[3]
    // (C++ class uses Matrix4f but on-disk stores 3x3 rot + 3-vec separately)
    return 2 * sizeof(fiff_int_t) + 2 * 12 * sizeof(fiff_float_t);
}

//=============================================================================================================

inline bool operator== (const FiffCoordTrans &a, const FiffCoordTrans &b)
{
    return (a.from == b.from &&
            a.to == b.to &&
            a.trans.isApprox(b.trans, 0.0001f) &&
            a.invtrans.isApprox(b.invtrans, 0.0001f));
}
} // NAMESPACE

#ifndef metatype_fiffcoordtrans
#define metatype_fiffcoordtrans
Q_DECLARE_METATYPE(FIFFLIB::FiffCoordTrans); /**< Provides QT META type declaration of the FIFFLIB::FiffCoordTrans type. For signal/slot usage.*/
#endif

#ifndef metatype_fiffcoordtrans_sptr
#define metatype_fiffcoordtrans_sptr
Q_DECLARE_METATYPE(FIFFLIB::FiffCoordTrans::SPtr); /**< Provides QT META type declaration of the FIFFLIB::FiffCoordTrans::SPtr type. For signal/slot usage.*/
#endif

#endif // FIFF_COORD_TRANS_H
