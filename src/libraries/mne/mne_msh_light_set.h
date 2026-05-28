//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_msh_light_set.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Collection of @ref MNELIB::MNEMshLight sources making up the viewer lighting rig.
 *
 * @ref MNELIB::MNEMshLightSet aggregates the lights (typically a key /
 * fill / rim trio) and is consumed by @ref MNEMshDisplaySurfaceSet when
 * saving or restoring a viewer scene.
 */

#ifndef MNEMSHLIGHTSET_H
#define MNEMSHLIGHTSET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_msh_light.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * @brief Collection of lights defining the lighting setup for 3-D rendering.
 */
class MNESHARED_EXPORT MNEMshLightSet
{
public:
    typedef QSharedPointer<MNEMshLightSet> SPtr;              /**< Shared pointer type for MNEMshLightSet. */
    typedef QSharedPointer<const MNEMshLightSet> ConstSPtr;   /**< Const shared pointer type for MNEMshLightSet. */

    //=========================================================================================================
    /**
     * Constructs an empty MNEMshLightSet.
     */
    MNEMshLightSet() = default;

    MNEMshLightSet(const MNEMshLightSet& other)
        : name(other.name)
    {
        lights.reserve(other.lights.size());
        for (const auto& l : other.lights)
            lights.push_back(std::make_unique<MNEMshLight>(*l));
    }

    MNEMshLightSet& operator=(const MNEMshLightSet& other)
    {
        if (this != &other) {
            name = other.name;
            lights.clear();
            lights.reserve(other.lights.size());
            for (const auto& l : other.lights)
                lights.push_back(std::make_unique<MNEMshLight>(*l));
        }
        return *this;
    }

    MNEMshLightSet(MNEMshLightSet&&) = default;
    MNEMshLightSet& operator=(MNEMshLightSet&&) = default;

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~MNEMshLightSet() = default;

public:
    QString name;                                        /**< Name of this light set. */
    std::vector<std::unique_ptr<MNEMshLight>> lights;    /**< Owned light objects. */

    /**
     * @brief Returns the number of lights in the set.
     */
    int nlight() const { return static_cast<int>(lights.size()); }
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEMSHLIGHTSET_H
