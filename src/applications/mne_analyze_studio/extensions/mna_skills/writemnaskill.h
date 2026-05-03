//=============================================================================================================
/**
 * @file     writemnaskill.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Workflow skill that writes an MNA project file (TASK 5).
 */

#ifndef MNE_ANALYZE_STUDIO_WRITEMNASKILL_H
#define MNE_ANALYZE_STUDIO_WRITEMNASKILL_H

#include <iskilloperator.h>

namespace MNEANALYZESTUDIO
{

class WriteMnaSkill : public ISkillOperator
{
    Q_OBJECT
public:
    explicit WriteMnaSkill(QObject* parent = nullptr);

    QJsonObject getOperatorDefinition() const override;
    QJsonObject executeSkill(const WorkflowNode& nodeState) override;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_WRITEMNASKILL_H
