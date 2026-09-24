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

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QTextStream>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

struct ChannelAlias {
    QString from;
    QString to;
    int toKind;   // -1 means do not change kind
};

//=============================================================================================================

static QString explainKind(int kind)
{
    switch (kind) {
    case FIFFV_MEG_CH:  return "MEG";
    case FIFFV_MCG_CH:  return "MCG";
    case FIFFV_EEG_CH:  return "EEG";
    case FIFFV_EOG_CH:  return "EOG";
    case FIFFV_EMG_CH:  return "EMG";
    case FIFFV_ECG_CH:  return "ECG";
    case FIFFV_MISC_CH: return "MISC";
    case FIFFV_STIM_CH: return "STI";
    case FIFFV_RESP_CH: return "RESP";
    default:            return "Unknown";
    }
}

//=============================================================================================================

static QList<ChannelAlias> readAliases(const QString &filename, bool reverse)
{
    QList<ChannelAlias> aliases;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("Cannot open alias file: %s", qPrintable(filename));
        return aliases;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        QStringList parts = line.split(':');
        if (parts.size() < 2) {
            qCritical("Alias line should contain a colon: %s", qPrintable(line));
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
                qCritical("Cannot interpret new kind: %s", qPrintable(parts[2]));
                return QList<ChannelAlias>();
            }
        }

        if (a.to.isEmpty()) {
            qCritical("Destination name missing (from = %s)", qPrintable(a.from));
            return QList<ChannelAlias>();
        }

        // Truncate to 15 chars (FIFF channel name limit)
        if (a.to.length() > 15) {
            a.to.truncate(15);
            fprintf(stderr, "Warning: name truncated to <%s>\n", qPrintable(a.to));
        }

        aliases.append(a);
    }
    return aliases;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
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
        qCritical("Both --fif and --alias are required.");
        parser.showHelp(1);
    }

    fprintf(stderr, "fif file   : %s\n", qPrintable(fifName));
    fprintf(stderr, "alias file : %s\n", qPrintable(aliasName));
    if (reverse)
        fprintf(stderr, "Exchange the roles of old and new channel names in the alias file\n");

    QList<ChannelAlias> aliases = readAliases(aliasName, reverse);
    if (aliases.isEmpty()) {
        qCritical("No aliases read.");
        return 1;
    }
    fprintf(stderr, "%lld aliases read from %s\n",
            static_cast<long long>(aliases.size()),
            qPrintable(aliasName));

    // Open FIFF file for in-place update
    QFile file(fifName);
    FiffStream::SPtr stream = FiffStream::open_update(file);
    if (!stream) {
        qCritical("Cannot open file for update: %s", qPrintable(fifName));
        return 1;
    }

    int nMod = 0;
    for (int k = 0; k < stream->nent(); k++) {
        if (stream->dir()[k]->kind == FIFF_CH_INFO) {
            FiffTag::UPtr tag;
            stream->read_tag(tag, stream->dir()[k]->pos);
            FiffChInfo ch = tag->toChInfo();

            for (const ChannelAlias &a : aliases) {
                if (ch.ch_name == a.from) {
                    fprintf(stderr, "%s -> %s", qPrintable(ch.ch_name), qPrintable(a.to));
                    nMod++;
                    ch.ch_name = a.to;
                    if (a.toKind > 0) {
                        fprintf(stderr, " (%s -> %s)\n",
                                qPrintable(explainKind(ch.kind)),
                                qPrintable(explainKind(a.toKind)));
                        ch.kind = a.toKind;
                    } else {
                        fprintf(stderr, "\n");
                    }
                    FiffTag::UPtr dummyTag; // unused
                    stream->device()->seek(stream->dir()[k]->pos);
                    stream->write_ch_info(ch);
                    break;
                }
            }
        }
    }

    stream->close();
    fprintf(stderr, "%d changes done\n", nMod);
    fprintf(stderr, "%s processed.\n", qPrintable(fifName));

    return 0;
}
