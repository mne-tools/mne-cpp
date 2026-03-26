//=============================================================================================================
/**
 * @file     temporalfilterskill.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares a minimal DAG-aware temporal filtering skill.
 */

#ifndef MNE_ANALYZE_STUDIO_TEMPORALFILTERSKILL_H
#define MNE_ANALYZE_STUDIO_TEMPORALFILTERSKILL_H

#include <iskilloperator.h>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Minimal temporal filter skill that consumes WorkflowNode state.
 */
class TemporalFilterSkill : public ISkillOperator
{
    Q_OBJECT

public:
    explicit TemporalFilterSkill(QObject* parent = nullptr);

    QJsonObject getOperatorDefinition() const override;
    QJsonObject executeSkill(const WorkflowNode& nodeState) override;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_TEMPORALFILTERSKILL_H
