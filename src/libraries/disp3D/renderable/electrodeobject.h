//=============================================================================================================
/**
 * @file     electrodeobject.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    ElectrodeObject class declaration.
 *
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
 * @brief One shaft of a stereotactic depth electrode (sEEG).
 */
struct DISP3DSHARED_EXPORT ElectrodeShaft
{
    QString                     label;          /**< Shaft label: "LH", "RA", etc. */
    QVector<ElectrodeContact>   contacts;       /**< Contacts ordered tip-to-tail. */
    float                       shaftRadius = 0.4f; /**< Cylinder radius in mm. */
    QColor                      shaftColor = Qt::gray;  /**< Shaft body color. */
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

    //=========================================================================================================
    /**
     * Set electrode data from shaft definitions.
     *
     * @param[in] shafts    Vector of electrode shafts.
     */
    void setShafts(const QVector<ElectrodeShaft>& shafts);

    //=========================================================================================================
    /**
     * @return Const reference to the current shafts.
     */
    const QVector<ElectrodeShaft>& shafts() const;

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

private:
    QVector<ElectrodeShaft> m_shafts;           /**< All electrode shafts. */
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
};

} // namespace DISP3DLIB

#endif // ELECTRODEOBJECT_H
