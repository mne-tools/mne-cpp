//=============================================================================================================
/**
 * @file     temporalfilterskill.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements a minimal DAG-aware temporal filtering skill.
 */

#include "temporalfilterskill.h"

#include <workflowgraph.h>

#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_stream.h>
#include <dsp/firfilter.h>

#include <Eigen/Core>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>

using namespace MNEANALYZESTUDIO;
using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

namespace
{

QJsonObject objectSchema(const QJsonObject& properties, const QJsonArray& required = QJsonArray())
{
    return QJsonObject{
        {"type", "object"},
        {"properties", properties},
        {"required", required}
    };
}

// Resolve a URI or bare path to an absolute filesystem path.
// Handles file:// URIs and absolute paths only; returns empty string for
// unresolvable mne://workspace/ or relative URIs.
QString resolveInputUri(const QString& uri)
{
    if(uri.startsWith(QLatin1String("file://"))) {
        return uri.mid(7);
    }
    if(QFileInfo(uri).isAbsolute()) {
        return uri;
    }
    return {};
}

// Derive the output FIFF file path from the input path and filter parameters.
QString deriveOutputPath(const QString& inputPath, double highpass, double lowpass)
{
    const QFileInfo fi(inputPath);
    const QString suffix = QString("_hp%1Hz_lp%2Hz")
        .arg(highpass, 0, 'f', 1)
        .arg(lowpass, 0, 'f', 1);
    return fi.dir().filePath(fi.completeBaseName() + suffix + QStringLiteral(".fif"));
}

} // namespace

TemporalFilterSkill::TemporalFilterSkill(QObject* parent)
: ISkillOperator(parent)
{
}

QJsonObject TemporalFilterSkill::getOperatorDefinition() const
{
    return QJsonObject{
        {"skill_id", "mne.skills.temporal_filter"},
        {"tool_name", "apply_filter"},
        {"display_name", "Temporal Filter"},
        {"description", "Append a temporal-filter workflow node and derive a filtered FIFF artifact URI."},
        {"extension_id", "temporal-filter-skill"},
        {"extension_display_name", "Temporal Filter Skill"},
        {"inputs_schema", objectSchema(QJsonObject{
             {"raw_data", QJsonObject{
                  {"type", "string"},
                  {"title", "Raw Data UID"},
                  {"description", "UID of the upstream raw or filtered FIFF resource."},
                  {"resource_type", "fiff_raw"}
              }}
         }, QJsonArray{"raw_data"})},
        {"parameters_schema", objectSchema(QJsonObject{
             {"highpass", QJsonObject{
                  {"type", "number"},
                  {"title", "Highpass"},
                  {"minimum", 0.0},
                  {"maximum", 2000.0},
                  {"default", 1.0},
                  {"description", "High-pass cutoff frequency in Hz."}
              }},
             {"lowpass", QJsonObject{
                  {"type", "number"},
                  {"title", "Lowpass"},
                  {"minimum", 0.0},
                  {"maximum", 2000.0},
                  {"default", 40.0},
                  {"description", "Low-pass cutoff frequency in Hz."}
              }}
         }, QJsonArray{"highpass"})},
        {"outputs_schema", objectSchema(QJsonObject{
             {"filtered_data", QJsonObject{
                  {"type", "string"},
                  {"title", "Filtered Data UID"},
                  {"description", "UID for the derived filtered FIFF resource."},
                  {"resource_type", "fiff_raw"}
              }}
         }, QJsonArray{"filtered_data"})}
    };
}

QJsonObject TemporalFilterSkill::executeSkill(const WorkflowNode& nodeState)
{
    const QJsonObject rawInput = nodeState.resolvedInputs.value("raw_data").toObject();
    const QString inputUri = rawInput.value("uri").toString().trimmed();
    const QString outputUid = nodeState.outputs.value("filtered_data").toString().trimmed();

    if(inputUri.isEmpty()) {
        return QJsonObject{
            {"status", "error"},
            {"message", QString("Temporal filter node `%1` is missing a resolved raw input URI.").arg(nodeState.uid)}
        };
    }

    const double highpass = nodeState.parameters.value("highpass").toDouble(1.0);
    const double lowpass = nodeState.parameters.value("lowpass").toDouble(40.0);

    if(highpass > 0.0 && lowpass > 0.0 && highpass >= lowpass) {
        return QJsonObject{
            {"status", "error"},
            {"message", QString("Temporal filter node `%1`: highpass (%2 Hz) must be less than lowpass (%3 Hz).")
                            .arg(nodeState.uid)
                            .arg(highpass)
                            .arg(lowpass)}
        };
    }

    // Resolve the input URI to a local filesystem path.
    const QString inputPath = resolveInputUri(inputUri);
    if(inputPath.isEmpty()) {
        return QJsonObject{
            {"status", "error"},
            {"message", QString("Temporal filter node `%1`: input URI `%2` cannot be resolved to a file path. "
                                "Use a file:// URI or an absolute path.")
                            .arg(nodeState.uid, inputUri)}
        };
    }

    if(!QFileInfo::exists(inputPath)) {
        return QJsonObject{
            {"status", "error"},
            {"message", QString("Temporal filter node `%1`: input file `%2` does not exist.")
                            .arg(nodeState.uid, inputPath)}
        };
    }

    // Open FIFF raw data.
    QFile inputFile(inputPath);
    FiffRawData raw(inputFile);
    if(raw.isEmpty()) {
        return QJsonObject{
            {"status", "error"},
            {"message", QString("Temporal filter node `%1`: could not parse FIFF file `%2`.")
                            .arg(nodeState.uid, inputPath)}
        };
    }

    // Read the full raw segment (calibrated physical units).
    MatrixXd data;
    MatrixXd times;
    if(!raw.read_raw_segment(data, times, raw.first_samp, raw.last_samp)) {
        return QJsonObject{
            {"status", "error"},
            {"message", QString("Temporal filter node `%1`: failed to read raw segment from `%2`.")
                            .arg(nodeState.uid, inputPath)}
        };
    }

    // Choose a filter order that is at most 256 taps but never larger than a
    // quarter of the available samples, so the zero-phase pass always has
    // enough data to work with.
    const int filterOrder = std::max(10, std::min(256, static_cast<int>(data.cols()) / 4));

    // Design FIR filter.  FirFilter::design() maps Hz values to the normed
    // FilterKernel representation internally.
    FilterKernel kernel;
    QString filterTypeName;
    if(highpass > 0.0 && lowpass > 0.0) {
        kernel = FirFilter::design(filterOrder, FirFilter::BandPass, highpass, lowpass, raw.info.sfreq);
        filterTypeName = QStringLiteral("bandpass");
    } else if(highpass > 0.0) {
        kernel = FirFilter::design(filterOrder, FirFilter::HighPass, highpass, highpass, raw.info.sfreq);
        filterTypeName = QStringLiteral("highpass");
    } else {
        kernel = FirFilter::design(filterOrder, FirFilter::LowPass, lowpass, lowpass, raw.info.sfreq);
        filterTypeName = QStringLiteral("lowpass");
    }

    // Apply zero-phase forward/backward FIR filter to all channels.
    const MatrixXd filteredData = FirFilter::applyZeroPhaseMatrix(data, kernel);

    // Derive output file path next to the input file.
    const QString outputPath = deriveOutputPath(inputPath, highpass, lowpass);

    // Write filtered FIFF.  start_writing_raw fills `cals` with the per-channel
    // calibration scalings; write_raw_buffer divides the calibrated data by cals
    // before writing so the stored samples are in raw ADC units.
    QFile outputFile(outputPath);
    RowVectorXd cals;
    FiffStream::SPtr outStream = FiffStream::start_writing_raw(outputFile, raw.info, cals);
    if(!outStream) {
        return QJsonObject{
            {"status", "error"},
            {"message", QString("Temporal filter node `%1`: could not open output file `%2` for writing.")
                            .arg(nodeState.uid, outputPath)}
        };
    }

    if(!outStream->write_raw_buffer(filteredData, cals)) {
        return QJsonObject{
            {"status", "error"},
            {"message", QString("Temporal filter node `%1`: failed to write filtered buffer to `%2`.")
                            .arg(nodeState.uid, outputPath)}
        };
    }

    outStream->finish_writing_raw();

    const QString outputUri = QString("file://%1").arg(outputPath);

    return QJsonObject{
        {"status", "completed"},
        {"message", QString("Applied %1 filter to `%2` (highpass=%3 Hz, lowpass=%4 Hz) → `%5`.")
                        .arg(filterTypeName,
                             QFileInfo(inputPath).fileName(),
                             QString::number(highpass, 'f', 2),
                             QString::number(lowpass, 'f', 2),
                             QFileInfo(outputPath).fileName())},
        {"outputs", QJsonObject{
             {"filtered_data", outputUri}
         }},
        {"parameters_used", QJsonObject{
             {"highpass", highpass},
             {"lowpass", lowpass},
             {"filter_order", filterOrder},
             {"filter_type", filterTypeName},
             {"sampling_frequency", raw.info.sfreq}
         }},
        {"source_resource", rawInput},
        {"output_path", outputPath}
    };
}
