//=============================================================================================================
/**
 * @file     runmnagraphskill.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Workflow skill that executes an MNA pipeline.
 */

#ifndef MNE_ANALYZE_STUDIO_RUNMNAGRAPHSKILL_H
#define MNE_ANALYZE_STUDIO_RUNMNAGRAPHSKILL_H

#include <iskilloperator.h>

namespace MNEANALYZESTUDIO
{

class RunMnaGraphSkill : public ISkillOperator
{
    Q_OBJECT
public:
    explicit RunMnaGraphSkill(QObject* parent = nullptr);

    QJsonObject getOperatorDefinition() const override;
    QJsonObject executeSkill(const WorkflowNode& nodeState) override;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_RUNMNAGRAPHSKILL_H
