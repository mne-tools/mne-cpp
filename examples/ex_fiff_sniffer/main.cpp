
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QCoreApplication>

#include <fiff/fiff.h>
#include <mne/mne.h>

#include <fiff/fiff_io.h>
#include <fiff/fiff_stream.h>
#include <fiff/c/fiff_digitizer_data.h>

////////////////////////

#include <connectivity/network/network.h>

#include <fiff/fiff_ch_info.h>
#include <fiff/c/fiff_digitizer_data.h>

#include <mne/c/mne_msh_display_surface_set.h>
#include <mne/c/mne_surface_or_volume.h>
//////////////////////

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineParser>
#include <QBuffer>
#include <QtEndian>

//=============================================================================================================
// MAIN
//=============================================================================================================

using namespace CONNECTIVITYLIB;
using namespace MNELIB;
using namespace FIFFLIB;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Disp Example");
    parser.addHelpOption();

    QCommandLineOption inputOption({"file", "f"}, "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    QCommandLineOption endianOption({"endian", "e", "en"}, "The file's endianness. 'little' or 'big'", "endian", "big");
    QCommandLineOption padEndOption("pad", "Adds -1 to end of stream to account for unfinished file (use for neuromag2ft)");

    parser.addOption(inputOption);
    parser.addOption(endianOption);
    parser.addOption(padEndOption);

    parser.process(a);

    bool littleEndian = false;
    if (parser.value(endianOption) == "little"){
        littleEndian = true;
    }

    QFile t_fileRaw(parser.value(inputOption));

    std::cout << "\n------------ Fiff Sniffer ------------\n";
    std::cout << "Input file : " << t_fileRaw.fileName().toStdString() << "\n";

    if(!t_fileRaw.exists())
    {
        std::cout << "File cannot be found.\n";
        std::cout << "\nBye!\n";

    } else { //file found
        QBuffer buffer;
        if(!buffer.open(QIODevice::ReadWrite))
        {
            std::cout << "Buffer can't be opened.\n";
            return 1;
        }
        if(!t_fileRaw.open(QIODevice::ReadOnly))
        {
            std::cout << "Input file can't be opened.\n";
            return 1;
        }
        buffer.write(t_fileRaw.readAll());

        t_fileRaw.close();

        qint32_le iIntToChar;
        char cCharFromInt[sizeof (qint32)];

        if (parser.isSet(padEndOption)){
            iIntToChar = -1;
            memcpy(cCharFromInt, &iIntToChar, sizeof(qint32));
            buffer.write(cCharFromInt);
            iIntToChar = -1;
            memcpy(cCharFromInt, &iIntToChar, sizeof(qint32));
            buffer.write(cCharFromInt);
            iIntToChar = -1;
            memcpy(cCharFromInt, &iIntToChar, sizeof(qint32));
            buffer.write(cCharFromInt);
            iIntToChar = -1;
            memcpy(cCharFromInt, &iIntToChar, sizeof(qint32));
            buffer.write(cCharFromInt);
        }

        buffer.reset();

        FIFFLIB::FiffStream stream(&buffer);
        if(littleEndian){
            stream.setByteOrder(QDataStream::LittleEndian);
        }else {
            stream.setByteOrder(QDataStream::BigEndian);
        }

        if(stream.open()){
            FIFFLIB::FiffInfo FifInfo;
            FIFFLIB::FiffDirNode::SPtr DirNode;

            FIFFLIB::FiffDigitizerData digData;

            if(stream.read_meas_info(stream.dirtree(), FifInfo, DirNode)) {

                FIFFLIB::FiffIO p_fiffIO(t_fileRaw);

                std::cout << "Num. raw dat sets: " << p_fiffIO.m_qlistRaw.size() << "\n";
                std::cout << "Num. evoked sets: " << p_fiffIO.m_qlistEvoked.size() << "\n";

                int count = 0;
                for (auto& data : p_fiffIO.m_qlistRaw){
                    std::cout << "--- \n";
                    std::cout << "Raw Set " << count << "\n";
                    std::cout << "Sample frequency: " << data->info.sfreq << "\n";
                    std::cout << "LineFreq: " << data->info.linefreq << " | Highpass: " << data->info.highpass << " | Lowpass: " << data->info.lowpass << "\n";
                    std::cout << "First sample: " << data->first_samp << " | Last sample: " << data->last_samp << "\n";
                    std::cout << "Number of samples: " << data->last_samp - data->first_samp << " | Time: " << (data->last_samp - data->first_samp) / data->info.sfreq << " sec. \n";
                    std::cout << "Number of digitizer points: " << data->info.dig.size() << "\n";
                    for (auto& point : data->info.dig){
                        if (point.kind == FIFFV_POINT_HPI){
                            std::cout << "HPI Point " << point.ident << " - " << point.r[0] << ", " << point.r[1] << ", " << point.r[2] << "\n";
                        }
                    }
                }
                count = 0;
                for (auto& data : p_fiffIO.m_qlistEvoked){
                    std::cout << "--- \n";
                    std::cout << "Evoked Set " << count << "\n";
                    std::cout << "Sample frequency: " << data->info.sfreq << "\n";
                    std::cout << "LineFreq: " << data->info.linefreq << " | Highpass: " << data->info.highpass << " | Lowpass: " << data->info.lowpass << "\n";
                    std::cout << "Num. averaged epochs: " << data->nave << "\n";
                }
                std::cout << "--- \n";
                std::cout << "--------------------------------------\n";
            }
            if(stream.read_digitizer_data(stream.dirtree(), digData)) {
                std::cout << "Digitizer data found.\n";
                std::cout << "Number of digitizer points: " << digData.points.size() << "\n";
                for (auto& point : digData.points){
                    if (point.kind == FIFFV_POINT_HPI){
                        std::cout << "HPI Point " << point.ident << " - " << point.r[0] << ", " << point.r[1] << ", " << point.r[2] << "\n";
                    }
                }
            }
        }
    }

    return 0;
}
