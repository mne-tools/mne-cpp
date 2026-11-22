//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Rename channels in FIFF files using an alias file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_dir_entry.h>
#include <fiff/fiff_ch_info.h>

#include <base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QTextStream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

struct ChannelAlias
{
    QString from;
    QString to;
    int toKind; // -1 means do not change kind
};

//=============================================================================================================

static QString explainKind(int kind)
{
    switch (kind) {
        case FIFFV_MEG_CH:
            return "MEG";
        case FIFFV_MCG_CH:
            return "MCG";
        case FIFFV_EEG_CH:
            return "EEG";
        case FIFFV_EOG_CH:
            return "EOG";
        case FIFFV_EMG_CH:
            return "EMG";
        case FIFFV_ECG_CH:
            return "ECG";
        case FIFFV_MISC_CH:
            return "MISC";
        case FIFFV_STIM_CH:
            return "STI";
        case FIFFV_RESP_CH:
            return "RESP";
        default:
            return "Unknown";
    }
}

//=============================================================================================================

static QList<ChannelAlias> readAliases(const QString& filename, bool reverse)
{
    QList<ChannelAlias> aliases;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Log::critical("Cannot open alias file: {}", filename.toStdString());
        return aliases;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        QStringList parts = line.split(':');
        if (parts.size() < 2) {
            Log::critical("Alias line should contain a colon: {}", line.toStdString());
            return QList<ChannelAlias>();
        }

        ChannelAlias a;
        if (reverse) {
            a.from = parts[1].trimmed();
            a.to = parts[0].trimmed();
        } else {
            a.from = parts[0].trimmed();
            a.to = parts[1].trimmed();
        }

        a.toKind = -1;
        if (parts.size() >= 3) {
            bool ok;
            a.toKind = parts[2].trimmed().toInt(&ok);
            if (!ok) {
                Log::critical("Cannot interpret new kind: {}", parts[2].toStdString());
                return QList<ChannelAlias>();
            }
        }

        if (a.to.isEmpty()) {
            Log::critical("Destination name missing (from = {})", a.from.toStdString());
            return QList<ChannelAlias>();
        }

        // Truncate to 15 chars (FIFF channel name limit)
        if (a.to.length() > 15) {
            a.to.truncate(15);
            Log::warning("Warning: name truncated to <{}>", a.to.toStdString());
        }

        aliases.append(a);
    }
    return aliases;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_rename_channels");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Give new names to selected channels in a FIFF file.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fifOpt("fif", "FIFF file to modify.", "name");
    parser.addOption(fifOpt);

    QCommandLineOption aliasOpt("alias", "Alias file (<old>:<new>[:<kind>]).", "name");
    parser.addOption(aliasOpt);

    QCommandLineOption revertOpt(QStringList() << "revert" << "reverse", "Swap old/new in the alias file.");
    parser.addOption(revertOpt);

    parser.process(app);

    QString fifName = parser.value(fifOpt);
    QString aliasName = parser.value(aliasOpt);
    bool reverse = parser.isSet(revertOpt);

    if (fifName.isEmpty() || aliasName.isEmpty()) {
        Log::critical("Both --fif and --alias are required.");
        parser.showHelp(1);
    }

    Log::info("fif file   : {}", fifName.toStdString());
    Log::info("alias file : {}", aliasName.toStdString());
    if (reverse)
        Log::info("Exchange the roles of old and new channel names in the alias file");

    QList<ChannelAlias> aliases = readAliases(aliasName, reverse);
    if (aliases.isEmpty()) {
        Log::critical("No aliases read.");
        return 1;
    }
    Log::info("{} aliases read from {}", static_cast<long long>(aliases.size()), aliasName.toStdString());

    // Open FIFF file for in-place update
    QFile file(fifName);
    auto stream = FiffStream::open_update(fifName.toStdU16String());
    if (!stream) {
        Log::critical("Cannot open file for update: {}", fifName.toStdString());
        return 1;
    }

    int nMod = 0;
    for (int k = 0; k < stream->nent(); k++) {
        if (stream->dir()[k].kind == FIFF_CH_INFO) {
            std::optional<FiffTag> tag;
            tag = stream->read_tag(stream->dir()[k].pos);
            FiffChInfo ch = tag->to_ch_info();

            for (const ChannelAlias& a : aliases) {
                if (ch.ch_name == a.from) {
                    QString line = QString("%1 -> %2").arg(ch.ch_name, a.to);
                    nMod++;
                    ch.ch_name = a.to.toStdString();
                    if (a.toKind > 0) {
                        line += QString(" (%1 -> %2)").arg(explainKind(ch.kind), explainKind(a.toKind));
                        ch.kind = a.toKind;
                    }
                    Log::info("{}", line.toStdString());
                    stream->byte_stream().seek(stream->dir()[k].pos);
                    stream->write_ch_info(ch);
                    break;
                }
            }
        }
    }

    stream->close();
    Log::info("{} changes done", nMod);
    Log::info("{} processed.", fifName.toStdString());

    return 0;
}
