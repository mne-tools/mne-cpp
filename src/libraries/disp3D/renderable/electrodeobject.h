//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     electrodeobject.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    ECoG / sEEG electrode model: shaft cylinders, contact spheres and grid layouts mapped to a colour-bar overlay.
 *
 * ElectrodeObject describes one or more electrode arrays in three
 * topologies (@ref ElectrodeLayout::Depth stereotactic shaft,
 * @ref ElectrodeLayout::Strip 1xN ECoG, @ref ElectrodeLayout::Grid
 * rows x cols ECoG) and generates CPU-side geometry that the
 * renderer uploads as a cylinder mesh (shafts) plus a per-contact
 * instance buffer (spheres with position, radius, RGBA colour,
 * selected flag).
 *
 * Per-contact scalar values (e.g. spectral power, evoked amplitude)
 * are mapped to a min / max colour gradient by
 * @ref ElectrodeObject::setContactValues so the same primitive can
 * show static placement and live activity. Bounding-box accessors
 * include the contact radius so the camera framing keeps every
 * contact visible.
 */

#ifndef ELECTRODEOBJECT_H
#define ELECTRODEOBJECT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColor>
#include <QMap>
#include <QString>
#include <QVector>
#include <QVector3D>

#include <memory>

// Forward-declare QRhi types so that this header stays QRhi-free
class QRhi;
class QRhiBuffer;
class QRhiResourceUpdateBatch;

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
/**
 * @brief Single contact on a depth electrode shaft.
 */
struct DISP3DSHARED_EXPORT ElectrodeContact
{
    QString     name;           /**< Contact label, e.g. "LH1", "LH2". */
    QVector3D   position;       /**< 3-D position in MRI (surface RAS) coords. */
    float       radius = 0.5f;  /**< Contact radius in mm. */
    QColor      color = Qt::yellow; /**< Display color. */
    bool        selected = false;   /**< Whether the contact is selected/highlighted. */
    float       value = 0.0f;   /**< Optional scalar for colormap overlay. */
};

//=============================================================================================================
/**
 * @brief Geometry layout for an electrode array.
 *
 * Added in v2.3.0 to extend the original sEEG-only ElectrodeObject
 * with ECoG strip and grid topologies. The renderer interprets the layout
 * field on @ref ElectrodeArray:
 *
 *   - Depth — render the cylindrical shaft + sphere instances per contact
 *             (the only mode supported in v2.2.0).
 *   - Strip — render sphere instances only, no shaft (a 1×N ECoG strip).
 *   - Grid  — render sphere instances on a regular @ref gridRows × @ref
 *             gridCols lattice, optionally with a translucent quad mesh
 *             linking the contacts as a visual reference.
 */
enum class ElectrodeLayout {
    Depth = 0,  /**< Stereotactic depth electrode (cylinder + contacts). */
    Strip,       /**< 1×N ECoG strip — spheres only. */
    Grid         /**< gridRows × gridCols ECoG grid — spheres + optional mesh. */
};

//=============================================================================================================
/**
 * @brief One electrode array — sEEG depth shaft, ECoG strip, or ECoG grid.
 *
 * The Depth-only fields (@ref shaftRadius, @ref shaftColor) are ignored
 * for Strip and Grid layouts. The Grid-only fields (@ref gridRows,
 * @ref gridCols) default to 1 and so describe a degenerate single
 * contact for the other layouts.
 */
struct DISP3DSHARED_EXPORT ElectrodeArray
{
    QString                     label;                  /**< Array label: "LH", "GridA", etc. */
    ElectrodeLayout             layout = ElectrodeLayout::Depth; /**< Geometry kind. */
    int                         gridRows = 1;           /**< Grid only: number of rows. */
    int                         gridCols = 1;           /**< Grid only: number of cols (Strip uses Cols = N, Rows = 1). */
    QVector<ElectrodeContact>   contacts;               /**< Contacts in array-local order. */
    float                       shaftRadius = 0.4f;     /**< Depth only: cylinder radius in mm. */
    QColor                      shaftColor = Qt::gray;  /**< Depth only: shaft body color. */
};

//=============================================================================================================
/**
 * @brief Data model for stereotactic depth electrode (sEEG) visualization.
 *
 * Holds electrode shaft definitions and contact metadata, and generates CPU-side
 * geometry (vertices/indices) that a QRhi-based renderer can upload to the GPU.
 */
class DISP3DSHARED_EXPORT ElectrodeObject
{
public:
    ElectrodeObject();
    ~ElectrodeObject();

    //=========================================================================================================
    /**
     * Set electrode data from array definitions.
     *
     * @param[in] arrays  Vector of electrode arrays.
     */
    void setArrays(const QVector<ElectrodeArray>& arrays);

    //=========================================================================================================
    /**
     * @return Const reference to the current arrays.
     */
    const QVector<ElectrodeArray>& arrays() const;

    //=========================================================================================================
    /**
     * @return Total number of contacts across all shafts.
     */
    int totalContactCount() const;

    //=========================================================================================================
    /**
     * Apply per-contact scalar overlay values and map them to a color gradient.
     *
     * @param[in] values    Map of contact name -> scalar value.
     * @param[in] minColor  Color for the minimum value (default: blue).
     * @param[in] maxColor  Color for the maximum value (default: red).
     */
    void setContactValues(const QMap<QString, float>& values,
                          const QColor& minColor = Qt::blue,
                          const QColor& maxColor = Qt::red);

    //=========================================================================================================
    /**
     * Select a contact by name. Clears the previous selection.
     *
     * @param[in] name  Contact name to select.
     */
    void selectContact(const QString& name);

    //=========================================================================================================
    /**
     * Clear all contact selections.
     */
    void clearSelection();

    //=========================================================================================================
    /**
     * @return Name of the currently selected contact, or empty string if none.
     */
    QString selectedContact() const;

    //=========================================================================================================
    /**
     * Generate interleaved vertex + index data for all shaft cylinders.
     * Each vertex: position (3 floats) + normal (3 floats) = 6 floats.
     *
     * @param[out] vertices         Vertex buffer (position + normal interleaved).
     * @param[out] indices          Index buffer (triangles).
     * @param[in]  cylinderSides    Number of sides for the cylinder cross-section.
     */
    void generateShaftGeometry(QVector<float>& vertices,
                               QVector<unsigned int>& indices,
                               int cylinderSides = 16) const;

    //=========================================================================================================
    /**
     * Generate per-instance data for contact spheres.
     * Per instance: position (3) + radius (1) + color RGBA (4) + selected flag (1) = 9 floats.
     *
     * @param[out] instanceData     Flat float buffer.
     */
    void generateContactInstances(QVector<float>& instanceData) const;

    //=========================================================================================================
    /**
     * @return Axis-aligned bounding box minimum corner (with contact-radius padding).
     */
    QVector3D boundingBoxMin() const;

    //=========================================================================================================
    /**
     * @return Axis-aligned bounding box maximum corner (with contact-radius padding).
     */
    QVector3D boundingBoxMax() const;

    //=========================================================================================================
    /**
     * Update GPU buffers (shaft vertex/index, contact instance) via QRhi.
     *
     * @param[in] rhi        Pointer to QRhi instance.
     * @param[in] u          Resource update batch.
     */
    void updateBuffers(QRhi *rhi, QRhiResourceUpdateBatch *u);

    //=========================================================================================================
    /**
     * @return Shaft vertex buffer (position + normal interleaved, 6 floats/vertex).
     */
    QRhiBuffer* vertexBuffer() const;

    //=========================================================================================================
    /**
     * @return Shaft index buffer.
     */
    QRhiBuffer* indexBuffer() const;

    //=========================================================================================================
    /**
     * @return Contact instance buffer (9 floats/instance).
     */
    QRhiBuffer* instanceBuffer() const;

    //=========================================================================================================
    /**
     * @return Number of shaft indices for drawIndexed.
     */
    uint32_t shaftIndexCount() const;

    //=========================================================================================================
    /**
     * @return Number of contact instances for instanced draw.
     */
    uint32_t contactInstanceCount() const;

private:
    QVector<ElectrodeArray> m_arrays;            /**< All electrode arrays. */
    QString                 m_selectedContact;   /**< Currently selected contact name. */
    QVector3D               m_bbMin;             /**< Cached bounding box min. */
    QVector3D               m_bbMax;             /**< Cached bounding box max. */

    //=========================================================================================================
    /**
     * Recompute the axis-aligned bounding box from all contact positions.
     */
    void computeBoundingBox();

    //=========================================================================================================
    /**
     * Linearly interpolate between two colors in RGB space.
     *
     * @param[in] value     Scalar value to map.
     * @param[in] minVal    Lower bound of the scalar range.
     * @param[in] maxVal    Upper bound of the scalar range.
     * @param[in] minColor  Color at minVal.
     * @param[in] maxColor  Color at maxVal.
     * @return Interpolated QColor.
     */
    static QColor interpolateColor(float value, float minVal, float maxVal,
                                   const QColor& minColor, const QColor& maxColor);

    /** @brief QRhi vertex, index, and instance buffers for electrode GPU rendering. */
    struct GpuBuffers;
    std::unique_ptr<GpuBuffers> m_gpu;
};

} // namespace DISP3DLIB

#endif // ELECTRODEOBJECT_H
