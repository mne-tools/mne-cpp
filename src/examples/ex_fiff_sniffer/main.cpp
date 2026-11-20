//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2010-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @brief    Read a FIFF file from a memory buffer with a chosen byte order and summarise its contents
 */


//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <memory>

#include <fiff/fiff.h>
#include <mne/mne.h>

#include <fiff/fiff_io.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_digitizer_data.h>

////////////////////////

#include <connectivity/network/network.h>

#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_digitizer_data.h>

#include <mne/mne_msh_display_surface_set.h>
#include <mne/mne_surface_or_volume.h>
//////////////////////

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineParser>
#include <QFile>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>

//=============================================================================================================
// MAIN
//=============================================================================================================

using namespace mne;

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Fiff Sniffer Example");
    parser.addHelpOption();

    QCommandLineOption inputOption({"file", "f"}, "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    QCommandLineOption endianOption({"endian", "e", "en"}, "The file's endianness. 'little' or 'big'", "endian", "big");
    QCommandLineOption padEndOption("pad", "Adds -1 to end of stream to account for unfinished file (use for neuromag2ft)");

    parser.addOption(inputOption);
    parser.addOption(endianOption);
    parser.addOption(padEndOption);

    parser.process(a);

    bool littleEndian = false;
    if (parser.value(endianOption) == "little") {
        littleEndian = true;
    }

    QFile t_fileRaw(parser.value(inputOption));

    std::cout << "\n------------ Fiff Sniffer ------------\n";
    std::cout << "Input file : " << t_fileRaw.fileName().toStdString() << "\n";

    if (!t_fileRaw.exists()) {
        std::cout << "File cannot be found.\n";
        std::cout << "\nBye!\n";

    } else { //file found
        if (!t_fileRaw.open(QIODevice::ReadOnly)) {
            std::cout << "Input file can't be opened.\n";
            return 1;
        }
        const QByteArray contents = t_fileRaw.readAll();
        t_fileRaw.close();

        const auto* begin = reinterpret_cast<const std::byte*>(contents.constData());
        std::vector<std::byte> bytes(begin, begin + contents.size());
        if (parser.isSet(padEndOption)) {
            bytes.insert(bytes.end(), 4 * sizeof(std::int32_t), std::byte{0xFF});
        }

        mne::FiffStream stream(std::make_unique<mne::MemoryByteStream>(std::move(bytes)));
        stream.set_little_endian(littleEndian);

        if (stream.open()) {
            mne::FiffInfo FifInfo;
            std::shared_ptr<mne::FiffDirNode> DirNode;

            if (stream.read_meas_info(stream.dirtree(), FifInfo, DirNode)) {
                mne::FiffIO p_fiffIO(parser.value(inputOption).toStdU16String());

                std::cout << "Num. raw dat sets: " << p_fiffIO.raw.size() << "\n";
                std::cout << "Num. evoked sets: " << p_fiffIO.evoked.size() << "\n";

                int count = 0;
                for (auto& data : p_fiffIO.raw) {
                    std::cout << "--- \n";
                    std::cout << "Raw Set " << count << "\n";
                    std::cout << "Sample frequency: " << data->info.sfreq << "\n";
                    std::cout << "LineFreq: " << data->info.linefreq << " | Highpass: " << data->info.highpass << " | Lowpass: " << data->info.lowpass << "\n";
                    std::cout << "First sample: " << data->first_samp << " | Last sample: " << data->last_samp << "\n";
                    std::cout << "Number of samples: " << data->last_samp - data->first_samp << " | Time: " << (data->last_samp - data->first_samp) / data->info.sfreq << " sec. \n";
                    std::cout << "Number of digitizer points: " << data->info.dig.size() << "\n";
                    for (auto& point : data->info.dig) {
                        if (point.kind == FIFFV_POINT_HPI) {
                            std::cout << "HPI Point " << point.ident << " - " << point.r[0] << ", " << point.r[1] << ", " << point.r[2] << "\n";
                        }
                    }
                }
                count = 0;
                for (auto& data : p_fiffIO.evoked) {
                    std::cout << "--- \n";
                    std::cout << "Evoked Set " << count << "\n";
                    std::cout << "Sample frequency: " << data->info.sfreq << "\n";
                    std::cout << "LineFreq: " << data->info.linefreq << " | Highpass: " << data->info.highpass << " | Lowpass: " << data->info.lowpass << "\n";
                    std::cout << "Num. averaged epochs: " << data->nave << "\n";
                }
                std::cout << "--- \n";
                std::cout << "--------------------------------------\n";
            }
            if (auto read = stream.read_digitizer_data(stream.dirtree())) {
                const FiffDigitizerData& digData = *read;
                std::cout << "Digitizer data found.\n";
                std::cout << "Number of digitizer points: " << digData.points.size() << "\n";
                for (auto& point : digData.points) {
                    if (point.kind == FIFFV_POINT_HPI) {
                        std::cout << "HPI Point " << point.ident << " - " << point.r[0] << ", " << point.r[1] << ", " << point.r[2] << "\n";
                    }
                }
            }
        }
    }

    return 0;
}
