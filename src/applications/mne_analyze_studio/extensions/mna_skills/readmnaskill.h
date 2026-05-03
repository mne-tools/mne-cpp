//=============================================================================================================
/**
 * @file     readmnaskill.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Workflow skill that loads an MNA project file and reports its
 *           summary into the Studio workflow graph (TASK 5).
 */

#ifndef MNE_ANALYZE_STUDIO_READMNASKILL_H
#define MNE_ANALYZE_STUDIO_READMNASKILL_H

#include <iskilloperator.h>

namespace MNEANALYZESTUDIO
{

class ReadMnaSkill : public ISkillOperator
{
    Q_OBJECT
public:
    explicit ReadMnaSkill(QObject* parent = nullptr);

    QJsonObject getOperatorDefinition() const override;
    QJsonObject executeSkill(const WorkflowNode& nodeState) override;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_READMNASKILL_H
