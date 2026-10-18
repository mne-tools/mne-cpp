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
 * topologies (@c ElectrodeLayout::Depth stereotactic shaft,
 * @c ElectrodeLayout::Strip 1xN ECoG, @c ElectrodeLayout::Grid
 * rows x cols ECoG) and provides two instanced streams for the renderer:
 * one sphere per contact, and the lead bodies (one cylinder along each
 * depth shaft, thin ties between neighbouring strip / grid contacts).
 * Depth leads and subdural arrays thus differ by shape, while colour tells
 * one lead from another.
 *
 * Positions and radii are in metres, in the frame of the brain surfaces
 * (FreeSurfer surface RAS).
 *
 * Per-contact scalar values (e.g. spectral power, evoked amplitude)
 * are mapped to a min / max colour gradient by
 * @ref DISP3DLIB::ElectrodeObject::setContactValues "ElectrodeObject::setContactValues" so the same primitive can
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
#include "../core/rendertypes.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QByteArray>
#include <QColor>
#include <QMap>
#include <QString>
#include <QVector>
#include <QVector3D>

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
/**
 * @brief Single contact on a depth electrode shaft.
 *
 * @snippet ex_disp3d_scene/main.cpp electrode_object_usage
 */
struct DISP3DSHARED_EXPORT ElectrodeContact
{
    QString name;          /**< Contact label, e.g. "LH1", "LH2". */
    QVector3D position;    /**< Position in metres, MRI surface RAS. */
    float radius = 0.0f;   /**< Drawn radius in metres; 0 uses the default of the array's layout. */
    QColor color;          /**< Display colour; invalid uses the colour of the array. */
    bool selected = false; /**< Whether the contact is selected/highlighted. */
    float value = 0.0f;    /**< Optional scalar for colormap overlay. */
};

//=============================================================================================================
/**
 * @brief Geometry layout for an electrode array.
 *
 * Added in v2.3.0 to extend the original sEEG-only ElectrodeObject
 * with ECoG strip and grid topologies. The renderer interprets the layout
 * field on @ref DISP3DLIB::ElectrodeArray "ElectrodeArray":
 *
 *   - Depth — render the cylindrical shaft + sphere instances per contact
 *             (the only mode supported in v2.2.0).
 *   - Strip — render sphere instances only, no shaft (a 1×N ECoG strip).
 *   - Grid  — render sphere instances on a regular @ref DISP3DLIB::ElectrodeArray::gridRows "gridRows" × @ref
 *             DISP3DLIB::ElectrodeArray::gridCols "gridCols" lattice, optionally with a translucent quad mesh
 *             linking the contacts as a visual reference.
 */
enum class ElectrodeLayout
{
    Depth = 0, /**< Stereotactic depth electrode (cylinder + contacts). */
    Strip,     /**< 1×N ECoG strip — spheres only. */
    Grid       /**< gridRows × gridCols ECoG grid — spheres + optional mesh. */
};

//=============================================================================================================
/**
 * @brief One electrode array — sEEG depth shaft, ECoG strip, or ECoG grid.
 *
 * The Depth-only fields (@ref shaftRadius, @ref shaftColor) are ignored
 * for Strip and Grid layouts. The Grid-only fields (@ref gridRows,
 * @ref DISP3DLIB::ElectrodeArray::gridCols "gridCols") default to 1 and so describe a degenerate single
 * contact for the other layouts.
 *
 * @snippet ex_disp3d_scene/main.cpp electrode_object_usage
 */
struct DISP3DSHARED_EXPORT ElectrodeArray
{
    QString label;                                   /**< Array label: "LH", "GridA", etc. */
    ElectrodeLayout layout = ElectrodeLayout::Depth; /**< Geometry kind. */
    int gridRows = 1;                                /**< Grid only: number of rows. */
    int gridCols = 1;                                /**< Grid only: number of cols (Strip uses Cols = N, Rows = 1). */
    QVector<ElectrodeContact> contacts;              /**< Contacts in order along the lead (contact 1 first). */
    float shaftRadius = 0.0f;                        /**< Depth only: shaft radius in metres; 0 uses the default. */
    QColor shaftColor;                               /**< Lead colour; invalid picks one from the lead palette. */
};

//=============================================================================================================
/**
 * @brief Renderable sEEG / ECoG electrodes: contact spheres plus lead bodies.
 *
 * Holds the arrays and their contacts and provides the instanced streams the renderer draws.
 *
 * @snippet ex_disp3d_scene/main.cpp electrode_object_usage
 */
class DISP3DSHARED_EXPORT ElectrodeObject
{
public:
    // Drawn sizes in metres. Real contacts are about 0.8 mm (depth) and 4 mm (subdural) across;
    // both are enlarged to stay visible and pickable, keeping their ratio.
    static constexpr float kDepthContactRadius = 0.0011f;
    static constexpr float kSubduralContactRadius = 0.0019f;
    static constexpr float kShaftRadius = 0.00045f;
    static constexpr float kShaftOvershoot = 0.004f; /**< Shaft extension past the outer contacts. */
    static constexpr float kTieRadius = 0.00035f;    /**< Ties between neighbouring strip / grid contacts. */
    static constexpr float kMaxTieLength = 0.015f;   /**< Longer gaps are not tied: not one array. */

    ElectrodeObject();
    ~ElectrodeObject();

    /**
     * @param[in] index  Lead index.
     * @return Lead colour: golden-angle hues, so neighbouring leads differ clearly.
     */
    static QColor leadColor(int index);

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
     * Highlight one contact while the pointer is over it, without changing the selection.
     *
     * @param[in] index  Contact index in array order (as returned by @ref intersect), or -1 for none.
     */
    void setHoveredContact(int index);

    /**
     * @return Hovered contact index, or -1.
     */
    int hoveredContact() const
    {
        return m_hoveredContact;
    }

    //=========================================================================================================
    /**
     * Intersect a ray with the contact spheres.
     *
     * @param[in]  rayOrigin  Ray origin.
     * @param[in]  rayDir     Normalised ray direction.
     * @param[out] dist       Distance to the nearest hit.
     * @return Index of the nearest contact hit (in array order), or -1.
     */
    int intersect(const QVector3D& rayOrigin, const QVector3D& rayDir, float& dist) const;

    /**
     * @param[in] index  Contact index in array order.
     * @return The contact, or nullptr when out of range.
     */
    const ElectrodeContact* contactAt(int index) const;

    /**
     * @param[in] index  Contact index in array order.
     * @return Hover text, e.g. "LA3 (depth lead LA)", or an empty string when out of range.
     */
    QString contactLabel(int index) const;

    //=========================================================================================================
    /**
     * @return Instanced streams of the contact spheres (one instance per contact).
     */
    InstancedStreams contactStreams() const;

    /**
     * @return Instanced streams of the lead bodies (unit cylinders: shafts and ties).
     */
    InstancedStreams bodyStreams() const;

    /** @return Identity of the contact mesh in the renderer's GPU cache. */
    const RenderKey& contactRenderKey() const
    {
        return m_contactRenderKey;
    }

    /** @return Identity of the body mesh in the renderer's GPU cache. */
    const RenderKey& bodyRenderKey() const
    {
        return m_bodyRenderKey;
    }

    bool isVisible() const
    {
        return m_visible;
    }
    void setVisible(bool visible)
    {
        m_visible = visible;
    }

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

private:
    /** Per-instance layout shared with the renderer's instanced pipelines. */
    struct InstanceData
    {
        float model[16];
        float color[4];
        float isSelected;
    };

    void buildBodies();
    void buildContactInstances();
    float contactRadius(const ElectrodeArray& array, const ElectrodeContact& contact) const;

    QVector<ElectrodeArray> m_arrays; /**< All electrode arrays. */
    QString m_selectedContact;        /**< Currently selected contact name. */
    int m_hoveredContact = -1;
    QVector3D m_bbMin; /**< Cached bounding box min. */
    QVector3D m_bbMax; /**< Cached bounding box max. */
    bool m_visible = true;

    // Unit sphere and unit cylinder (along +y, centred) shared by all instances
    QByteArray m_sphereVertices;
    QByteArray m_sphereIndices;
    QByteArray m_cylinderVertices;
    QByteArray m_cylinderIndices;
    int m_sphereIndexCount = 0;
    int m_cylinderIndexCount = 0;

    QByteArray m_contactInstances;
    QByteArray m_bodyInstances;
    int m_bodyCount = 0;
    quint64 m_contactGeneration = 0;
    quint64 m_bodyGeneration = 0;
    RenderKey m_contactRenderKey;
    RenderKey m_bodyRenderKey;

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
};

} // namespace DISP3DLIB

#endif // ELECTRODEOBJECT_H
