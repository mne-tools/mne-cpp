//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     iskilloperator.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     March, 2026
 * @version  dev
 * @brief    Declares the declarative DAG skill operator interface used by MNE Analyze Studio.
 */

#ifndef MNE_ANALYZE_STUDIO_ISKILLOPERATOR_H
#define MNE_ANALYZE_STUDIO_ISKILLOPERATOR_H

#include "studio_core_global.h"

#include <QJsonObject>
#include <QObject>

namespace MNEANALYZESTUDIO
{

struct WorkflowNode;

/**
 * @brief Base class for skill operators that execute workflow DAG nodes.
 */
class STUDIOCORESHARED_EXPORT ISkillOperator : public QObject
{
    Q_OBJECT

public:
    explicit ISkillOperator(QObject* parent = nullptr);
    ~ISkillOperator() override;

    virtual QJsonObject getOperatorDefinition() const = 0;
    virtual QJsonObject executeSkill(const WorkflowNode& nodeState) = 0;
};

} // namespace MNEANALYZESTUDIO

#define ISkillOperator_iid "org.mnecpp.mne-analyze-studio.ISkillOperator/2.0"
Q_DECLARE_INTERFACE(MNEANALYZESTUDIO::ISkillOperator, ISkillOperator_iid)

#endif // MNE_ANALYZE_STUDIO_ISKILLOPERATOR_H
