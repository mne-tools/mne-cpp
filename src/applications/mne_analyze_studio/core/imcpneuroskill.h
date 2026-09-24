//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     imcpneuroskill.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  dev
 * @brief    Backward-compatible wrapper for the legacy skill include.
 */

#ifndef MNE_ANALYZE_STUDIO_IMCPNEUROSKILL_H
#define MNE_ANALYZE_STUDIO_IMCPNEUROSKILL_H

#include "iskilloperator.h"

namespace MNEANALYZESTUDIO
{

using IMcpNeuroSkill = ISkillOperator;

} // namespace MNEANALYZESTUDIO

#define IMcpNeuroSkill_iid ISkillOperator_iid

#endif // MNE_ANALYZE_STUDIO_IMCPNEUROSKILL_H
