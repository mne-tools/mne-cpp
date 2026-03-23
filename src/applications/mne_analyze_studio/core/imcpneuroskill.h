//=============================================================================================================
/**
 * @file     imcpneuroskill.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
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
 * @brief    Declares the MCP-compatible skill interface used by MNE Analyze Studio.
 */

#ifndef MNE_ANALYZE_STUDIO_IMCPNEUROSKILL_H
#define MNE_ANALYZE_STUDIO_IMCPNEUROSKILL_H

#include "studio_core_global.h"

#include <QJsonObject>
#include <QObject>

namespace MNEANALYZESTUDIO
{

class IBuffer;

/**
 * @brief Base class for studio skills that expose MCP tool metadata and execution hooks.
 */
class STUDIOCORESHARED_EXPORT IMcpNeuroSkill : public QObject
{
    Q_OBJECT

public:
    explicit IMcpNeuroSkill(QObject* parent = nullptr);
    ~IMcpNeuroSkill() override;

    virtual QJsonObject getMetadata() const = 0;
    virtual QJsonObject execute(const QJsonObject& arguments) = 0;
    virtual void onBufferAttached(IBuffer* buffer) = 0;
};

} // namespace MNEANALYZESTUDIO

#define IMcpNeuroSkill_iid "org.mnecpp.mne-analyze-studio.IMcpNeuroSkill/1.0"
Q_DECLARE_INTERFACE(MNEANALYZESTUDIO::IMcpNeuroSkill, IMcpNeuroSkill_iid)

#endif // MNE_ANALYZE_STUDIO_IMCPNEUROSKILL_H
