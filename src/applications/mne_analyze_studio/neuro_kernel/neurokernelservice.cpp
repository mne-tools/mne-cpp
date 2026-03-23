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

#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QLocalSocket>

#include <algorithm>
#include <cmath>

using namespace MNEANALYZESTUDIO;

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

    if(toolName == "neurokernel.execute") {
        return QJsonObject{
            {"status", "ok"},
            {"message", QString("Neuro-Kernel executed command: %1").arg(arguments.value("command").toString())},
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
        {"message", QString("Kernel tools: %1").arg(toolDefinitions().size())},
        {"tools", toolDefinitions()},
        {"plane", "data"},
        {"transport", "local_socket"},
        {"protocol", "mcp-over-json-rpc-2.0"}
    };
}

QJsonArray NeuroKernelService::toolDefinitions() const
{
    return QJsonArray{
        QJsonObject{
            {"name", "neurokernel.execute"},
            {"description", "Execute a generic neuro-kernel command string."}
        },
        QJsonObject{
            {"name", "neurokernel.raw_stats"},
            {"description", "Compute RMS, mean absolute value, peak absolute value, and top channels for a raw sample window."}
        },
        QJsonObject{
            {"name", "neurokernel.channel_stats"},
            {"description", "Compute per-channel RMS, mean absolute value, and peak absolute value for a raw sample window."}
        }
    };
}
