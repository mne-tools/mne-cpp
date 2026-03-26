//=============================================================================================================
/**
 * @file     sourceestimationskill.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the source estimation workflow skill.
 *
 *           Pipeline:
 *             1. Read FIFF raw data from `raw_data` input.
 *             2. Read a pre-computed MNE inverse operator from `inverse_operator` input.
 *             3. Prepare the inverse operator (regularisation, noise-normalisation).
 *             4. Read the requested time window from the raw recording.
 *             5. Apply InvMinimumNorm::calculateInverse() to obtain source timecourses.
 *             6. Write the STC file beside the raw input.
 *             7. Return a JSON result with the STC URI and summary statistics.
 */

#include "sourceestimationskill.h"

#include <workflowgraph.h>

#include <fiff/fiff_raw_data.h>
#include <mne/mne_inverse_operator.h>
#include <inv/minimum_norm/inv_minimum_norm.h>
#include <inv/inv_source_estimate.h>

#include <Eigen/Core>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QtMath>

using namespace MNEANALYZESTUDIO;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace INVLIB;
using namespace Eigen;

namespace
{

// Resolve a file:// URI or bare absolute path to a filesystem path.
QString resolveUri(const QString& uri)
{
    if(uri.startsWith(QLatin1String("file://"))) {
        return uri.mid(7);
    }
    if(QFileInfo(uri).isAbsolute()) {
        return uri;
    }
    return {};
}

// Build the STC output path next to the raw file.
QString deriveStcPath(const QString& rawPath, const QString& method)
{
    const QFileInfo fi(rawPath);
    const QString suffix = QString("_%1").arg(method.toLower());
    return fi.dir().filePath(fi.completeBaseName() + suffix + QStringLiteral(".stc"));
}

QJsonObject objectSchema(const QJsonObject& properties, const QJsonArray& required = QJsonArray())
{
    return QJsonObject{
        {"type", "object"},
        {"properties", properties},
        {"required", required}
    };
}

} // namespace

SourceEstimationSkill::SourceEstimationSkill(QObject* parent)
: ISkillOperator(parent)
{
}

QJsonObject SourceEstimationSkill::getOperatorDefinition() const
{
    return QJsonObject{
        {"skill_id",              "mne.skills.source_estimation"},
        {"tool_name",             "source_estimation"},
        {"display_name",          "Source Estimation"},
        {"description",           "Apply a pre-computed MNE inverse operator to a FIFF raw recording "
                                  "and write the distributed source timecourses to an STC file."},
        {"extension_id",          "source-estimation-skill"},
        {"extension_display_name","Source Estimation Skill"},

        // ── Inputs ────────────────────────────────────────────────────────────
        {"inputs_schema", objectSchema(QJsonObject{
             {"raw_data", QJsonObject{
                  {"type",          "string"},
                  {"title",         "Raw / Filtered FIFF UID"},
                  {"description",   "UID of the upstream raw or temporally-filtered FIFF resource."},
                  {"resource_type", "fiff_raw"}
              }},
             {"inverse_operator", QJsonObject{
                  {"type",          "string"},
                  {"title",         "Inverse Operator UID"},
                  {"description",   "UID of the pre-computed MNE inverse operator resource (.fif)."},
                  {"resource_type", "fiff_inv"}
              }}
         }, QJsonArray{"raw_data", "inverse_operator"})},

        // ── Parameters ────────────────────────────────────────────────────────
        {"parameters_schema", objectSchema(QJsonObject{
             {"method", QJsonObject{
                  {"type",        "string"},
                  {"title",       "Inverse Method"},
                  {"description", "Distributed inverse method: \"MNE\", \"dSPM\", or \"sLORETA\"."},
                  {"enum",        QJsonArray{"MNE", "dSPM", "sLORETA"}},
                  {"default",     "dSPM"}
              }},
             {"snr", QJsonObject{
                  {"type",        "number"},
                  {"title",       "SNR"},
                  {"description", "Assumed signal-to-noise ratio. Regularisation λ² = 1/SNR²."},
                  {"minimum",     0.1},
                  {"maximum",     100.0},
                  {"default",     3.0}
              }},
             {"tmin", QJsonObject{
                  {"type",        "number"},
                  {"title",       "Window Start (s)"},
                  {"description", "Start of the analysis window in seconds from the recording start. "
                                  "Negative values are clamped to the first sample."},
                  {"default",     0.0}
              }},
             {"tmax", QJsonObject{
                  {"type",        "number"},
                  {"title",       "Window End (s)"},
                  {"description", "End of the analysis window in seconds. "
                                  "Values beyond the last sample are clamped automatically."},
                  {"default",     1.0}
              }}
         }, QJsonArray{"method"})},

        // ── Outputs ───────────────────────────────────────────────────────────
        {"outputs_schema", objectSchema(QJsonObject{
             {"source_estimate", QJsonObject{
                  {"type",          "string"},
                  {"title",         "Source Estimate UID"},
                  {"description",   "UID for the derived STC artifact."},
                  {"resource_type", "stc"}
              }}
         }, QJsonArray{"source_estimate"})}
    };
}

QJsonObject SourceEstimationSkill::executeSkill(const WorkflowNode& nodeState)
{
    // ── Resolve inputs ────────────────────────────────────────────────────────
    const QJsonObject rawInput  = nodeState.resolvedInputs.value("raw_data").toObject();
    const QJsonObject invInput  = nodeState.resolvedInputs.value("inverse_operator").toObject();
    const QString     rawUri    = rawInput.value("uri").toString().trimmed();
    const QString     invUri    = invInput.value("uri").toString().trimmed();

    if(rawUri.isEmpty()) {
        return QJsonObject{
            {"status",  "error"},
            {"message", QString("Source estimation node `%1`: missing resolved raw input URI.").arg(nodeState.uid)}
        };
    }
    if(invUri.isEmpty()) {
        return QJsonObject{
            {"status",  "error"},
            {"message", QString("Source estimation node `%1`: missing resolved inverse operator URI.").arg(nodeState.uid)}
        };
    }

    const QString rawPath = resolveUri(rawUri);
    const QString invPath = resolveUri(invUri);

    if(rawPath.isEmpty() || !QFileInfo::exists(rawPath)) {
        return QJsonObject{
            {"status",  "error"},
            {"message", QString("Source estimation node `%1`: raw file `%2` not found.").arg(nodeState.uid, rawPath)}
        };
    }
    if(invPath.isEmpty() || !QFileInfo::exists(invPath)) {
        return QJsonObject{
            {"status",  "error"},
            {"message", QString("Source estimation node `%1`: inverse operator file `%2` not found.").arg(nodeState.uid, invPath)}
        };
    }

    // ── Parameters ────────────────────────────────────────────────────────────
    const QString method  = nodeState.parameters.value("method").toString(QStringLiteral("dSPM")).trimmed();
    const double  snr     = nodeState.parameters.value("snr").toDouble(3.0);
    const double  tminSec = nodeState.parameters.value("tmin").toDouble(0.0);
    const double  tmaxSec = nodeState.parameters.value("tmax").toDouble(1.0);

    if(snr <= 0.0) {
        return QJsonObject{
            {"status",  "error"},
            {"message", QString("Source estimation node `%1`: SNR must be positive (got %2).").arg(nodeState.uid).arg(snr)}
        };
    }
    if(tmaxSec <= tminSec) {
        return QJsonObject{
            {"status",  "error"},
            {"message", QString("Source estimation node `%1`: tmax (%2 s) must be greater than tmin (%3 s).")
                            .arg(nodeState.uid).arg(tmaxSec).arg(tminSec)}
        };
    }

    const bool isDSPM    = (method.compare(QStringLiteral("dSPM"),    Qt::CaseInsensitive) == 0);
    const bool isSLORETA = (method.compare(QStringLiteral("sLORETA"), Qt::CaseInsensitive) == 0);
    const float lambda2  = static_cast<float>(1.0 / (snr * snr));

    // ── Load raw FIFF ─────────────────────────────────────────────────────────
    QFile rawFile(rawPath);
    FiffRawData raw(rawFile);
    if(raw.isEmpty()) {
        return QJsonObject{
            {"status",  "error"},
            {"message", QString("Source estimation node `%1`: could not parse FIFF file `%2`.").arg(nodeState.uid, rawPath)}
        };
    }

    const float sfreq      = static_cast<float>(raw.info.sfreq);
    const int firstSamp    = raw.first_samp;
    const int lastSamp     = raw.last_samp;

    const int winFirst = qBound(firstSamp,
                                static_cast<int>(firstSamp + tminSec * sfreq),
                                lastSamp);
    const int winLast  = qBound(firstSamp,
                                static_cast<int>(firstSamp + tmaxSec * sfreq),
                                lastSamp);

    if(winFirst >= winLast) {
        return QJsonObject{
            {"status",  "error"},
            {"message", QString("Source estimation node `%1`: requested window [%2 s, %3 s] "
                                "falls outside the recording range.")
                            .arg(nodeState.uid).arg(tminSec).arg(tmaxSec)}
        };
    }

    // ── Load inverse operator ─────────────────────────────────────────────────
    QFile invFile(invPath);
    MNEInverseOperator invOp;
    if(!MNEInverseOperator::read_inverse_operator(invFile, invOp)) {
        return QJsonObject{
            {"status",  "error"},
            {"message", QString("Source estimation node `%1`: failed to read inverse operator from `%2`.")
                            .arg(nodeState.uid, invPath)}
        };
    }

    // ── Prepare the inverse operator ──────────────────────────────────────────
    const MNEInverseOperator prepInv = invOp.prepare_inverse_operator(1, lambda2, isDSPM, isSLORETA);
    InvMinimumNorm mnNorm(prepInv, lambda2, method);

    // ── Read the requested window ─────────────────────────────────────────────
    MatrixXd data;
    MatrixXd times;
    if(!raw.read_raw_segment(data, times, winFirst, winLast)) {
        return QJsonObject{
            {"status",  "error"},
            {"message", QString("Source estimation node `%1`: failed to read raw segment [%2, %3] samples.")
                            .arg(nodeState.uid).arg(winFirst).arg(winLast)}
        };
    }

    // ── Apply inverse ─────────────────────────────────────────────────────────
    const float tminSample = static_cast<float>(winFirst - firstSamp) / sfreq;
    const float tstep      = 1.0f / sfreq;

    InvSourceEstimate stc = mnNorm.calculateInverse(data, tminSample, tstep);

    if(stc.isEmpty()) {
        return QJsonObject{
            {"status",  "error"},
            {"message", QString("Source estimation node `%1`: inverse calculation returned an empty estimate.")
                            .arg(nodeState.uid)}
        };
    }

    // ── Write STC file ────────────────────────────────────────────────────────
    const QString stcPath = deriveStcPath(rawPath, method);
    QFile stcFile(stcPath);
    if(!stc.write(stcFile)) {
        return QJsonObject{
            {"status",  "error"},
            {"message", QString("Source estimation node `%1`: failed to write STC file `%2`.")
                            .arg(nodeState.uid, stcPath)}
        };
    }

    // ── Summary statistics ────────────────────────────────────────────────────
    const int nSources  = static_cast<int>(stc.data.rows());
    const int nTimes    = static_cast<int>(stc.data.cols());
    double    peakAmp   = 0.0;
    int       peakSrc   = 0;
    int       peakTime  = 0;
    for(int r = 0; r < nSources; ++r) {
        for(int c = 0; c < nTimes; ++c) {
            const double v = std::abs(stc.data(r, c));
            if(v > peakAmp) {
                peakAmp  = v;
                peakSrc  = r;
                peakTime = c;
            }
        }
    }

    const QString outputUri = QString("file://%1").arg(stcPath);

    return QJsonObject{
        {"status",  "completed"},
        {"message", QString("Applied %1 inverse to `%2` (SNR=%3, window=[%4–%5 s]) → `%6`.")
                        .arg(method,
                             QFileInfo(rawPath).fileName(),
                             QString::number(snr, 'f', 1),
                             QString::number(tminSec, 'f', 2),
                             QString::number(tmaxSec, 'f', 2),
                             QFileInfo(stcPath).fileName())},
        {"outputs", QJsonObject{
             {"source_estimate", outputUri}
         }},
        {"summary", QJsonObject{
             {"method",         method},
             {"n_sources",      nSources},
             {"n_times",        nTimes},
             {"tmin_s",         tminSample},
             {"tstep_s",        tstep},
             {"snr",            snr},
             {"lambda2",        lambda2},
             {"peak_amplitude", peakAmp},
             {"peak_source",    peakSrc},
             {"peak_time_s",    tminSample + peakTime * tstep},
             {"stc_path",       stcPath}
         }},
        {"output_path", stcPath}
    };
}
