//=============================================================================================================
/**
 * @file     networktreeitem.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
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
 * @brief    NetworkTreeItem class declaration.
 *
 */

#ifndef NETWORKTREEITEM_H
#define NETWORKTREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "abstracttreeitem.h"

//=============================================================================================================
/**
 * NetworkTreeItem holds metadata about a connectivity network in the tree model.
 *
 * @brief Tree item representing a connectivity network.
 */
class NetworkTreeItem : public AbstractTreeItem
{
public:
    //=========================================================================================================
    /**
     * Constructs a NetworkTreeItem.
     *
     * @param[in] text           Display text.
     * @param[in] objectKey      Unique key (e.g. "net_coherence").
     */
    explicit NetworkTreeItem(const QString &text = "Network",
                             const QString &objectKey = QString());

    //=========================================================================================================
    /**
     * Returns the object key identifying this network.
     *
     * @return The object key.
     */
    QString objectKey() const { return m_objectKey; }

private:
    QString m_objectKey;    /**< Unique key for this network item. */
};

#endif // NETWORKTREEITEM_H
