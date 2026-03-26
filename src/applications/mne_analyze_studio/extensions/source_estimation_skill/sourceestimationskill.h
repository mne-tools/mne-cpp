//=============================================================================================================
/**
 * @file     sourceestimationskill.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Declares the source estimation workflow skill.
 *
 *           Inputs:  a FIFF raw/filtered file and a pre-computed MNE inverse operator (.fif).
 *           Process: Applies minimum-norm (MNE / dSPM / sLORETA) to a configurable time window
 *                    from the raw recording, then writes the resulting source timecourses to an
 *                    STC file beside the raw input.
 *           Outputs: STC file URI + JSON summary (peak source amplitude, n_sources, time range).
 */

#ifndef MNE_ANALYZE_STUDIO_SOURCEESTIMATIONSKILL_H
#define MNE_ANALYZE_STUDIO_SOURCEESTIMATIONSKILL_H

#include <iskilloperator.h>

namespace MNEANALYZESTUDIO
{

/**
 * @brief Workflow skill that applies a pre-computed MNE inverse operator to raw MEG/EEG data.
 */
class SourceEstimationSkill : public ISkillOperator
{
    Q_OBJECT

public:
    explicit SourceEstimationSkill(QObject* parent = nullptr);

    QJsonObject getOperatorDefinition() const override;
    QJsonObject executeSkill(const WorkflowNode& nodeState) override;
};

} // namespace MNEANALYZESTUDIO

#endif // MNE_ANALYZE_STUDIO_SOURCEESTIMATIONSKILL_H
