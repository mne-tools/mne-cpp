//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     networktreeitem.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Tree-item identifying a connectivity network for the instanced node / edge renderer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "networktreeitem.h"

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NetworkTreeItem::NetworkTreeItem(const QString &text, const QString &objectKey)
    : AbstractTreeItem(text, AbstractTreeItem::NetworkItem)
    , m_objectKey(objectKey)
{
}
