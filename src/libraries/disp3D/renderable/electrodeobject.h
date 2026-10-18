//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     electrodeobject.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Renderable view of an sEEG / ECoG implantation: contact spheres and lead bodies.
 *
 * ElectrodeObject draws an @ref IEEGLIB::IeegImplantation. It provides two instanced streams for
 * the renderer: one sphere per contact, and the lead bodies (one cylinder along each depth lead,
 * thin ties between neighbouring strip and grid contacts). Depth leads and subdural arrays thus
 * differ by shape, while colour tells one lead from another.
 *
 * The implantation owns the contacts and leads; this object only adds display state: per-contact
 * colours (e.g. from setContactValues()), the selected and the hovered contact.
 *
 * Positions are in metres, in the frame of the brain surfaces (FreeSurfer surface RAS).
 */

#ifndef ELECTRODEOBJECT_H
#define ELECTRODEOBJECT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"
#include "../core/rendertypes.h"

#include <ieeg/ieeg_implantation.h>

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
 * @brief Renderable sEEG / ECoG electrodes: contact spheres plus lead bodies.
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
     * Draw an implantation. Clears the selection, hover and contact colours.
     *
     * @param[in] implantation  Contacts and leads, positions in the frame of the surfaces.
     */
    void setImplantation(const IEEGLIB::IeegImplantation& implantation);

    /**
     * @return The drawn implantation.
     */
    const IEEGLIB::IeegImplantation& implantation() const
    {
        return m_implantation;
    }

    /**
     * @return Total number of contacts.
     */
    int totalContactCount() const;

    //=========================================================================================================
    /**
     * Colour contacts by value, mapped linearly between the smallest and largest given value.
     * Contacts without a value keep the colour of their lead.
     *
     * @param[in] values    Map of contact name -> scalar value.
     * @param[in] minColor  Color for the minimum value (default: blue).
     * @param[in] maxColor  Color for the maximum value (default: red).
     */
    void setContactValues(const QMap<QString, float>& values,
                          const QColor& minColor = Qt::blue,
                          const QColor& maxColor = Qt::red);

    /**
     * Restore the lead colours of all contacts.
     */
    void clearContactValues();

    //=========================================================================================================
    /**
     * Select a contact by name. Clears the previous selection.
     *
     * @param[in] name  Contact name to select.
     */
    void selectContact(const QString& name);

    /**
     * Clear the contact selection.
     */
    void clearSelection();

    /**
     * @return Name of the currently selected contact, or empty string if none.
     */
    QString selectedContact() const;

    //=========================================================================================================
    /**
     * Highlight one contact while the pointer is over it, without changing the selection.
     *
     * @param[in] index  Contact index (as returned by @ref intersect), or -1 for none.
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
     * @return Index of the nearest contact hit, or -1.
     */
    int intersect(const QVector3D& rayOrigin, const QVector3D& rayDir, float& dist) const;

    /**
     * @param[in] index  Contact index.
     * @return The contact, or nullptr when out of range.
     */
    const IEEGLIB::IeegContact* contactAt(int index) const;

    /**
     * @param[in] index  Contact index.
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
    float contactRadius(int contact) const;

    IEEGLIB::IeegImplantation m_implantation;
    QVector<int> m_leadOfContact;    /**< Lead index per contact. */
    QVector<QColor> m_contactColors; /**< Per contact; invalid uses the lead colour. */
    int m_selectedContact = -1;
    int m_hoveredContact = -1;
    QVector3D m_bbMin;
    QVector3D m_bbMax;
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

    void computeBoundingBox();

    static QColor interpolateColor(float value, float minVal, float maxVal,
                                   const QColor& minColor, const QColor& maxColor);
};

} // namespace DISP3DLIB

#endif // ELECTRODEOBJECT_H
