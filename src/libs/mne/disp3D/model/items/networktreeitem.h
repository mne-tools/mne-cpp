//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     networktreeitem.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Tree item identifying a connectivity network rendered as instanced nodes and edges.
 *
 * Holds only the unique object key that maps the item back to its
 * @ref mne::Network payload inside @ref mne::disp::NetworkObject. The
 * actual graph (nodes as spheres, edges as cylinders colour-mapped
 * by weight) lives on the GPU as a pair of instanced meshes,
 * regenerated whenever the threshold or colormap changes.
 */

#ifndef NETWORKTREEITEM_H
#define NETWORKTREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"
#include "abstracttreeitem.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>

namespace mne::disp
{

//=============================================================================================================
/**
 * NetworkTreeItem holds metadata about a connectivity network in the tree model.
 *
 * @brief Tree item representing a connectivity network.
 */
class DISP3DSHARED_EXPORT NetworkTreeItem : public AbstractTreeItem
{
public:
    //=========================================================================================================
    /**
     * Constructs a NetworkTreeItem.
     *
     * @param[in] text           Display text.
     * @param[in] objectKey      Unique key (e.g. "net_coherence").
     */
    explicit NetworkTreeItem(const QString& text = "Network",
                             const std::string& objectKey = {});

    //=========================================================================================================
    /**
     * Returns the object key identifying this network.
     *
     * @return The object key.
     */
    std::string objectKey() const
    {
        return m_objectKey;
    }

private:
    std::string m_objectKey; /**< Unique key for this network item. */
};

} // namespace mne::disp

#endif // NETWORKTREEITEM_H
