//=============================================================================================================
/**
 * @file     neurokernelservice.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the local Neuro-Kernel JSON-RPC service for studio analysis tools.
 */

#include "neurokernelservice.h"

#include <Eigen/Core>

#include <fiff/fiff_raw_data.h>

#include <jsonrpcmessage.h>
#include <capabilityutils.h>

#include <dsp/welch_psd.h>

#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QLocalSocket>

#include <algorithm>
#include <cmath>

using namespace MNEANALYZESTUDIO;

namespace
{

QJsonObject objectSchema(const QJsonObject& properties,
                         const QJsonArray& required = QJsonArray())
{
    return QJsonObject{
        {"type", "object"},
        {"properties", properties},
        {"required", required}
    };
}

QJsonObject numberSchema(const QString& title,
                         const QString& description = QString())
{
    QJsonObject schema{
        {"type", "number"},
        {"title", title}
    };

    if(!description.isEmpty()) {
        schema.insert("description", description);
    }

    return schema;
}

QJsonObject integerSchema(const QString& title,
                          int minimum,
                          int maximum,
                          int defaultValue,
                          const QString& description = QString())
{
    QJsonObject schema{
        {"type", "integer"},
        {"title", title},
        {"minimum", minimum},
        {"maximum", maximum},
        {"default", defaultValue}
    };

    if(!description.isEmpty()) {
        schema.insert("description", description);
    }

    return schema;
}

QJsonObject stringSchema(const QString& title,
                         const QJsonArray& values = QJsonArray(),
                         const QString& defaultValue = QString(),
                         const QString& description = QString())
{
    QJsonObject schema{
        {"type", "string"},
        {"title", title},
        {"default", defaultValue}
    };

    if(!values.isEmpty()) {
        schema.insert("enum", values);
    }

    if(!description.isEmpty()) {
        schema.insert("description", description);
    }

    return schema;
}

QJsonObject arraySchema(const QString& title,
                        const QJsonObject& itemSchema,
                        const QString& description = QString())
{
    QJsonObject schema{
        {"type", "array"},
        {"title", title},
        {"items", itemSchema}
    };

    if(!description.isEmpty()) {
        schema.insert("description", description);
    }

    return schema;
}

}

NeuroKernelService::NeuroKernelService(QObject* parent)
: QObject(parent)
, m_router(this)
{
    m_router.registerMethod("tools/call", [this](const QJsonObject& params) {
        return handleToolCall(params);
    });
    m_router.registerMethod("tools/list", [this](const QJsonObject&) {
        return handleToolsList();
    });
}

bool NeuroKernelService::start(const QString& socketName)
{
    QLocalServer::removeServer(socketName);
    if(!m_server.listen(socketName)) {
        return false;
    }

    connect(&m_server, &QLocalServer::newConnection, this, [this]() {
        while(QLocalSocket* socket = m_server.nextPendingConnection()) {
            connect(socket, &QLocalSocket::readyRead, socket, [this, socket]() {
                while(socket->canReadLine()) {
                    const QByteArray payload = socket->readLine();
                    QJsonObject request;
                    QString errorString;
                    QJsonObject response;

                    if(JsonRpcMessage::deserialize(payload, request, errorString)) {
                        response = m_router.route(request);
                    } else {
                        response = JsonRpcMessage::createError(QJsonValue(), -32700, errorString);
                    }

                    socket->write(JsonRpcMessage::serialize(response));
                    socket->flush();
                }
            });
        }
    });

    return true;
}

QJsonObject NeuroKernelService::handleToolCall(const QJsonObject& params) const
{
    const QString toolName = params.value("name").toString();
    const QJsonObject arguments = params.value("arguments").toObject();

    if(toolName == "neurokernel.raw_stats") {
        const QString filePath = arguments.value("file").toString();
        if(!QFileInfo::exists(filePath)) {
            return QJsonObject{
                {"status", "error"},
                {"message", QString("Neuro-Kernel could not find raw file: %1").arg(filePath)}
            };
        }

        QFile file(filePath);
        FIFFLIB::FiffRawData raw(file);
        if(raw.isEmpty()) {
            return QJsonObject{
                {"status", "error"},
                {"message", QString("Neuro-Kernel could not parse raw file: %1").arg(filePath)}
            };
        }
        const int requestedFrom = arguments.value("from_sample").toInt(raw.first_samp);
        const int requestedTo = arguments.value("to_sample").toInt(raw.last_samp);
        const int fromSample = std::max<int>(raw.first_samp, requestedFrom);
        const int toSample = std::min<int>(raw.last_samp, requestedTo);

        Eigen::MatrixXd data;
        Eigen::MatrixXd times;
        if(!raw.read_raw_segment(data, times, fromSample, toSample)) {
            return QJsonObject{
                {"status", "error"},
                {"message", QString("Neuro-Kernel could not read raw segment for %1").arg(filePath)}
            };
        }

        if(data.size() == 0) {
            return QJsonObject{
                {"status", "error"},
                {"message", QString("Neuro-Kernel read an empty data segment for %1").arg(filePath)}
            };
        }

        const double rms = std::sqrt(data.array().square().mean());
        const double meanAbs = data.array().abs().mean();
        const double peakAbs = data.array().abs().maxCoeff();

        struct ChannelStat {
            QString name;
            double rms;
        };

        QVector<ChannelStat> channelStats;
        channelStats.reserve(static_cast<int>(data.rows()));
        for(int row = 0; row < data.rows(); ++row) {
            channelStats.push_back(ChannelStat{
                raw.info.ch_names.value(row),
                std::sqrt(data.row(row).array().square().mean())
            });
        }

        std::sort(channelStats.begin(), channelStats.end(), [](const ChannelStat& left, const ChannelStat& right) {
            return left.rms > right.rms;
        });

        QJsonArray topChannels;
        const int topCount = std::min<int>(3, channelStats.size());
        QStringList topChannelTexts;
        for(int i = 0; i < topCount; ++i) {
            topChannels.append(QJsonObject{
                {"name", channelStats.at(i).name},
                {"rms", channelStats.at(i).rms}
            });
            topChannelTexts << QString("%1 (%2)")
                                   .arg(channelStats.at(i).name)
                                   .arg(channelStats.at(i).rms, 0, 'g', 4);
        }

        return QJsonObject{
            {"status", "ok"},
            {"tool_name", toolName},
            {"message", QString("Raw stats for %1 samples %2-%3: RMS=%4, mean|x|=%5, peak|x|=%6, top channels=%7")
                            .arg(QFileInfo(filePath).fileName())
                            .arg(fromSample)
                            .arg(toSample)
                            .arg(rms, 0, 'g', 4)
                            .arg(meanAbs, 0, 'g', 4)
                            .arg(peakAbs, 0, 'g', 4)
                            .arg(topChannelTexts.join(", "))},
            {"file", filePath},
            {"from_sample", fromSample},
            {"to_sample", toSample},
            {"channel_count", static_cast<int>(data.rows())},
            {"sample_count", static_cast<int>(data.cols())},
            {"rms", rms},
            {"mean_abs", meanAbs},
            {"peak_abs", peakAbs},
            {"top_channels", topChannels},
            {"plane", "data"},
            {"transport", "local_socket"},
            {"protocol", "mcp-over-json-rpc-2.0"}
        };
    }

    if(toolName == "neurokernel.channel_stats") {
        const QString filePath = arguments.value("file").toString();
        if(!QFileInfo::exists(filePath)) {
            return QJsonObject{
                {"status", "error"},
                {"message", QString("Neuro-Kernel could not find raw file: %1").arg(filePath)}
            };
        }

        QFile file(filePath);
        FIFFLIB::FiffRawData raw(file);
        if(raw.isEmpty()) {
            return QJsonObject{
                {"status", "error"},
                {"message", QString("Neuro-Kernel could not parse raw file: %1").arg(filePath)}
            };
        }

        const int requestedFrom = arguments.value("from_sample").toInt(raw.first_samp);
        const int requestedTo = arguments.value("to_sample").toInt(raw.last_samp);
        const int fromSample = std::max<int>(raw.first_samp, requestedFrom);
        const int toSample = std::min<int>(raw.last_samp, requestedTo);
        const int limit = std::max(1, arguments.value("limit").toInt(10));
        const QString match = arguments.value("match").toString().trimmed();

        Eigen::MatrixXd data;
        Eigen::MatrixXd times;
        if(!raw.read_raw_segment(data, times, fromSample, toSample)) {
            return QJsonObject{
                {"status", "error"},
                {"message", QString("Neuro-Kernel could not read raw segment for %1").arg(filePath)}
            };
        }

        struct ChannelStat {
            QString name;
            double rms;
            double meanAbs;
            double peakAbs;
        };

        QVector<ChannelStat> channelStats;
        channelStats.reserve(static_cast<int>(data.rows()));
        for(int row = 0; row < data.rows(); ++row) {
            const QString channelName = raw.info.ch_names.value(row);
            if(!match.isEmpty() && !channelName.contains(match, Qt::CaseInsensitive)) {
                continue;
            }

            channelStats.push_back(ChannelStat{
                channelName,
                std::sqrt(data.row(row).array().square().mean()),
                data.row(row).array().abs().mean(),
                data.row(row).array().abs().maxCoeff()
            });
        }

        std::sort(channelStats.begin(), channelStats.end(), [](const ChannelStat& left, const ChannelStat& right) {
            return left.rms > right.rms;
        });

        QJsonArray channels;
        QStringList channelTexts;
        const int resultCount = std::min(limit, static_cast<int>(channelStats.size()));
        for(int i = 0; i < resultCount; ++i) {
            const ChannelStat& stat = channelStats.at(i);
            channels.append(QJsonObject{
                {"name", stat.name},
                {"rms", stat.rms},
                {"mean_abs", stat.meanAbs},
                {"peak_abs", stat.peakAbs}
            });
            channelTexts << QString("%1 (rms=%2)").arg(stat.name).arg(stat.rms, 0, 'g', 4);
        }

        return QJsonObject{
            {"status", "ok"},
            {"tool_name", toolName},
            {"message", QString("Channel stats for %1 samples %2-%3: %4")
                            .arg(QFileInfo(filePath).fileName())
                            .arg(fromSample)
                            .arg(toSample)
                            .arg(channelTexts.isEmpty() ? QString("no channels matched") : channelTexts.join(", "))},
            {"file", filePath},
            {"from_sample", fromSample},
            {"to_sample", toSample},
            {"match", match},
            {"channel_count", resultCount},
            {"channels", channels},
            {"plane", "data"},
            {"transport", "local_socket"},
            {"protocol", "mcp-over-json-rpc-2.0"}
        };
    }

    if(toolName == "neurokernel.find_peak_window") {
        const QString filePath = arguments.value("file").toString();
        if(!QFileInfo::exists(filePath)) {
            return QJsonObject{
                {"status", "error"},
                {"message", QString("Neuro-Kernel could not find raw file: %1").arg(filePath)}
            };
        }

        QFile file(filePath);
        FIFFLIB::FiffRawData raw(file);
        if(raw.isEmpty()) {
            return QJsonObject{
                {"status", "error"},
                {"message", QString("Neuro-Kernel could not parse raw file: %1").arg(filePath)}
            };
        }

        const int requestedFrom = arguments.value("from_sample").toInt(raw.first_samp);
        const int requestedTo = arguments.value("to_sample").toInt(raw.last_samp);
        const int fromSample = std::max<int>(raw.first_samp, requestedFrom);
        const int toSample = std::min<int>(raw.last_samp, requestedTo);
        const QString match = arguments.value("match").toString().trimmed();

        Eigen::MatrixXd data;
        Eigen::MatrixXd times;
        if(!raw.read_raw_segment(data, times, fromSample, toSample)) {
            return QJsonObject{
                {"status", "error"},
                {"message", QString("Neuro-Kernel could not read raw segment for %1").arg(filePath)}
            };
        }

        double peakValue = -1.0;
        int peakRow = -1;
        int peakColumn = -1;
        for(int row = 0; row < data.rows(); ++row) {
            const QString channelName = raw.info.ch_names.value(row);
            if(!match.isEmpty() && !channelName.contains(match, Qt::CaseInsensitive)) {
                continue;
            }

            Eigen::Index localColumn = 0;
            const double localPeak = data.row(row).array().abs().maxCoeff(&localColumn);
            if(localPeak > peakValue) {
                peakValue = localPeak;
                peakRow = row;
                peakColumn = static_cast<int>(localColumn);
            }
        }

        if(peakRow < 0 || peakColumn < 0) {
            return QJsonObject{
                {"status", "error"},
                {"message", QString("Neuro-Kernel could not find a matching peak window for %1").arg(filePath)}
            };
        }

        const int peakSample = fromSample + peakColumn;
        const QString peakChannel = raw.info.ch_names.value(peakRow);
        return QJsonObject{
            {"status", "ok"},
            {"tool_name", toolName},
            {"message", QString("Peak window for %1: sample %2 on %3 with |x|=%4")
                            .arg(QFileInfo(filePath).fileName())
                            .arg(peakSample)
                            .arg(peakChannel)
                            .arg(peakValue, 0, 'g', 4)},
            {"file", filePath},
            {"from_sample", fromSample},
            {"to_sample", toSample},
            {"match", match},
            {"peak_sample", peakSample},
            {"peak_channel", peakChannel},
            {"peak_abs", peakValue},
            {"plane", "data"},
            {"transport", "local_socket"},
            {"protocol", "mcp-over-json-rpc-2.0"}
        };
    }

    if(toolName == "neurokernel.psd_summary") {
        const QString filePath = arguments.value("file").toString();
        if(!QFileInfo::exists(filePath)) {
            return QJsonObject{
                {"status", "error"},
                {"message", QString("Neuro-Kernel could not find raw file: %1").arg(filePath)}
            };
        }

        QFile file(filePath);
        FIFFLIB::FiffRawData raw(file);
        if(raw.isEmpty()) {
            return QJsonObject{
                {"status", "error"},
                {"message", QString("Neuro-Kernel could not parse raw file: %1").arg(filePath)}
            };
        }

        const int requestedFrom = arguments.value("from_sample").toInt(raw.first_samp);
        const int requestedTo = arguments.value("to_sample").toInt(raw.last_samp);
        const int fromSample = std::max<int>(raw.first_samp, requestedFrom);
        const int toSample = std::min<int>(raw.last_samp, requestedTo);
        const QString match = arguments.value("match").toString().trimmed();
        const int nfft = std::max(32, arguments.value("nfft").toInt(256));

        Eigen::MatrixXd data;
        Eigen::MatrixXd times;
        if(!raw.read_raw_segment(data, times, fromSample, toSample)) {
            return QJsonObject{
                {"status", "error"},
                {"message", QString("Neuro-Kernel could not read raw segment for %1").arg(filePath)}
            };
        }

        Eigen::RowVectorXi picks(data.rows());
        int pickCount = 0;
        QStringList matchedChannels;
        for(int row = 0; row < data.rows(); ++row) {
            const QString channelName = raw.info.ch_names.value(row);
            if(!match.isEmpty() && !channelName.contains(match, Qt::CaseInsensitive)) {
                continue;
            }

            picks[pickCount++] = row;
            matchedChannels << channelName;
        }

        if(pickCount == 0) {
            return QJsonObject{
                {"status", "error"},
                {"message", QString("Neuro-Kernel could not find matching channels for PSD in %1").arg(filePath)}
            };
        }

        const Eigen::RowVectorXi selectedPicks = picks.head(pickCount);
        const UTILSLIB::WelchPsdResult psdResult =
            UTILSLIB::WelchPsd::compute(data,
                                        raw.info.sfreq,
                                        std::min(nfft, static_cast<int>(data.cols())),
                                        0.5,
                                        UTILSLIB::WelchPsd::Hann,
                                        selectedPicks);

        const Eigen::RowVectorXd meanPsd = psdResult.matPsd.colwise().mean();
        QJsonArray frequencies;
        QJsonArray psdValues;
        for(int i = 0; i < psdResult.vecFreqs.size(); ++i) {
            frequencies.append(psdResult.vecFreqs[i]);
            psdValues.append(meanPsd[i]);
        }

        return QJsonObject{
            {"status", "ok"},
            {"tool_name", toolName},
            {"message", QString("PSD summary for %1 using %2 channels.")
                            .arg(QFileInfo(filePath).fileName())
                            .arg(pickCount)},
            {"file", filePath},
            {"from_sample", fromSample},
            {"to_sample", toSample},
            {"match", match},
            {"nfft", std::min(nfft, static_cast<int>(data.cols()))},
            {"channel_count", pickCount},
            {"channels", QJsonArray::fromStringList(matchedChannels)},
            {"frequencies", frequencies},
            {"psd", psdValues},
            {"plane", "data"},
            {"transport", "local_socket"},
            {"protocol", "mcp-over-json-rpc-2.0"}
        };
    }

    if(toolName == "neurokernel.execute") {
        const QString command = arguments.value("command").toString().trimmed();

        if(command.isEmpty() || command.compare(QLatin1String("help"), Qt::CaseInsensitive) == 0) {
            QJsonArray toolNames;
            for(const QJsonValue& tool : toolDefinitions()) {
                toolNames.append(tool.toObject().value("name").toString());
            }
            return QJsonObject{
                {"status", "ok"},
                {"tool_name", toolName},
                {"command", command.isEmpty() ? "help" : command},
                {"message", QString("Neuro-Kernel exposes %1 tools. Use tools/list for full definitions.")
                                .arg(toolNames.size())},
                {"available_tools", toolNames},
                {"plane", "data"},
                {"transport", "local_socket"},
                {"protocol", "mcp-over-json-rpc-2.0"}
            };
        }

        if(command.compare(QLatin1String("status"), Qt::CaseInsensitive) == 0) {
            return QJsonObject{
                {"status", "ok"},
                {"tool_name", toolName},
                {"command", command},
                {"message", "Neuro-Kernel is running."},
                {"kernel_version", "2.0.0"},
                {"tool_count", static_cast<int>(toolDefinitions().size())},
                {"plane", "data"},
                {"transport", "local_socket"},
                {"protocol", "mcp-over-json-rpc-2.0"}
            };
        }

        if(command.compare(QLatin1String("version"), Qt::CaseInsensitive) == 0) {
            return QJsonObject{
                {"status", "ok"},
                {"tool_name", toolName},
                {"command", command},
                {"message", "Neuro-Kernel version 2.0.0."},
                {"version", "2.0.0"},
                {"plane", "data"},
                {"transport", "local_socket"},
                {"protocol", "mcp-over-json-rpc-2.0"}
            };
        }

        return QJsonObject{
            {"status", "error"},
            {"tool_name", toolName},
            {"command", command},
            {"message", QString("Unknown Neuro-Kernel command `%1`. Run `help` for available commands.").arg(command)},
            {"plane", "data"},
            {"transport", "local_socket"},
            {"protocol", "mcp-over-json-rpc-2.0"}
        };
    }

    return QJsonObject{
        {"status", "ignored"},
        {"message", QString("No Neuro-Kernel tool registered for %1").arg(toolName)}
    };
}

QJsonObject NeuroKernelService::handleToolsList() const
{
    return QJsonObject{
        {"status", "ok"},
        {"tool_name", "tools/list"},
        {"message", QString("Kernel tools: %1").arg(toolDefinitions().size())},
        {"tools", toolDefinitions()},
        {"plane", "data"},
        {"transport", "local_socket"},
        {"protocol", "mcp-over-json-rpc-2.0"}
    };
}

QJsonArray NeuroKernelService::toolDefinitions() const
{
    const QJsonArray rawTools{
        QJsonObject{
            {"name", "neurokernel.execute"},
            {"description", "Execute a generic neuro-kernel command string."},
            {"input_schema", objectSchema(QJsonObject{
                 {"command", stringSchema("Command",
                                          QJsonArray(),
                                          "help",
                                          "Generic Neuro-Kernel command string for debugging or fallback execution.")}
             }, QJsonArray{"command"})},
            {"result_schema", objectSchema(QJsonObject{
                 {"status", stringSchema("Status", QJsonArray{"ok", "error", "ignored"})},
                 {"tool_name", stringSchema("Tool Name")},
                 {"message", stringSchema("Message")}
             }, QJsonArray{"status", "tool_name", "message"})}
        },
        QJsonObject{
            {"name", "neurokernel.raw_stats"},
            {"description", "Compute RMS, mean absolute value, peak absolute value, and top channels for a raw sample window."},
            {"input_schema", objectSchema(QJsonObject{
                 {"window_samples", integerSchema("Window Samples",
                                                  1,
                                                  1000000,
                                                  600,
                                                  "Number of samples around the current cursor to include in the analysis window.")}
             }, QJsonArray{"window_samples"})},
            {"result_schema", objectSchema(QJsonObject{
                 {"status", stringSchema("Status", QJsonArray{"ok", "error"})},
                 {"tool_name", stringSchema("Tool Name")},
                 {"message", stringSchema("Message")},
                 {"file", stringSchema("File")},
                 {"from_sample", integerSchema("From Sample", 0, 1000000000, 0)},
                 {"to_sample", integerSchema("To Sample", 0, 1000000000, 0)},
                 {"channel_count", integerSchema("Channel Count", 0, 1000000, 0)},
                 {"sample_count", integerSchema("Sample Count", 0, 1000000000, 0)},
                 {"rms", numberSchema("RMS", "Root mean square value over the selected raw window.")},
                 {"mean_abs", numberSchema("Mean Absolute", "Mean absolute amplitude over the selected raw window.")},
                 {"peak_abs", numberSchema("Peak Absolute", "Peak absolute amplitude over the selected raw window.")},
                 {"top_channels", arraySchema("Top Channels",
                                              objectSchema(QJsonObject{
                                                  {"name", stringSchema("Channel Name")},
                                                  {"rms", numberSchema("Channel RMS")}
                                              }, QJsonArray{"name", "rms"}))}
             }, QJsonArray{"status", "tool_name", "message", "rms", "mean_abs", "peak_abs"})}
        },
        QJsonObject{
            {"name", "neurokernel.channel_stats"},
            {"description", "Compute per-channel RMS, mean absolute value, and peak absolute value for a raw sample window."},
            {"input_schema", objectSchema(QJsonObject{
                 {"window_samples", integerSchema("Window Samples",
                                                  1,
                                                  1000000,
                                                  600,
                                                  "Number of samples around the current cursor to analyze.")},
                 {"limit", integerSchema("Channel Limit",
                                         1,
                                         512,
                                         5,
                                         "Maximum number of channels to return in the result.")},
                 {"match", stringSchema("Channel Match",
                                        QJsonArray{"", "EEG", "MEG", "EOG"},
                                        "EEG",
                                        "Optional channel-name filter applied before ranking channels.")}
             }, QJsonArray{"window_samples"})},
            {"result_schema", objectSchema(QJsonObject{
                 {"status", stringSchema("Status", QJsonArray{"ok", "error"})},
                 {"tool_name", stringSchema("Tool Name")},
                 {"message", stringSchema("Message")},
                 {"file", stringSchema("File")},
                 {"from_sample", integerSchema("From Sample", 0, 1000000000, 0)},
                 {"to_sample", integerSchema("To Sample", 0, 1000000000, 0)},
                 {"match", stringSchema("Channel Match")},
                 {"channel_count", integerSchema("Channel Count", 0, 1000000, 0)},
                 {"channels", arraySchema("Channels",
                                          objectSchema(QJsonObject{
                                              {"name", stringSchema("Channel Name")},
                                              {"rms", numberSchema("Channel RMS")},
                                              {"mean_abs", numberSchema("Mean Absolute")},
                                              {"peak_abs", numberSchema("Peak Absolute")}
                                          }, QJsonArray{"name", "rms", "mean_abs", "peak_abs"}))}
             }, QJsonArray{"status", "tool_name", "message", "channels"})}
        },
        QJsonObject{
            {"name", "neurokernel.psd_summary"},
            {"description", "Compute a Welch PSD summary for the active raw sample window and optional channel match."},
            {"input_schema", objectSchema(QJsonObject{
                 {"window_samples", integerSchema("Window Samples",
                                                  32,
                                                  1000000,
                                                  1200,
                                                  "Number of samples around the current cursor to include in the PSD window.")},
                 {"nfft", integerSchema("FFT Size",
                                        32,
                                        8192,
                                        256,
                                        "FFT length used for the Welch PSD estimate.")},
                 {"match", stringSchema("Channel Match",
                                        QJsonArray{"", "EEG", "MEG", "EOG"},
                                        "EEG",
                                        "Optional channel-name filter applied before averaging PSDs.")}
             }, QJsonArray{"window_samples"})},
            {"result_schema", objectSchema(QJsonObject{
                 {"status", stringSchema("Status", QJsonArray{"ok", "error"})},
                 {"tool_name", stringSchema("Tool Name")},
                 {"message", stringSchema("Message")},
                 {"file", stringSchema("File")},
                 {"channel_count", integerSchema("Channel Count", 0, 1000000, 0)},
                 {"channels", arraySchema("Channels", stringSchema("Channel Name"))},
                 {"frequencies", arraySchema("Frequencies", numberSchema("Frequency"))},
                 {"psd", arraySchema("PSD", numberSchema("Power Spectral Density"))}
             }, QJsonArray{"status", "tool_name", "message", "frequencies", "psd"})}
        },
        QJsonObject{
            {"name", "neurokernel.find_peak_window"},
            {"description", "Find the strongest absolute-amplitude sample inside a raw window, optionally filtered by channel name match."},
            {"input_schema", objectSchema(QJsonObject{
                 {"window_samples", integerSchema("Window Samples",
                                                  1,
                                                  1000000,
                                                  4000,
                                                  "Number of samples to search for the strongest absolute-amplitude event.")},
                 {"match", stringSchema("Channel Match",
                                        QJsonArray{"", "EEG", "MEG", "EOG"},
                                        "EEG",
                                        "Optional channel-name filter used while searching for the peak window.")}
             }, QJsonArray{"window_samples"})},
            {"result_schema", objectSchema(QJsonObject{
                 {"status", stringSchema("Status", QJsonArray{"ok", "error"})},
                 {"tool_name", stringSchema("Tool Name")},
                 {"message", stringSchema("Message")},
                 {"file", stringSchema("File")},
                 {"from_sample", integerSchema("From Sample", 0, 1000000000, 0)},
                 {"to_sample", integerSchema("To Sample", 0, 1000000000, 0)},
                 {"match", stringSchema("Channel Match")},
                 {"peak_sample", integerSchema("Peak Sample", 0, 1000000000, 0)},
                 {"peak_channel", stringSchema("Peak Channel")},
                {"peak_abs", numberSchema("Peak Absolute")}
             }, QJsonArray{"status", "tool_name", "message", "peak_sample", "peak_channel", "peak_abs"})}
        }
    };

    QJsonArray tools;
    for(const QJsonValue& value : rawTools) {
        tools.append(annotateCapabilityMetadata(value.toObject()));
    }

    return tools;
}
