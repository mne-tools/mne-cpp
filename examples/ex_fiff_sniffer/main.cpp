
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QCoreApplication>

#include <fiff/fiff.h>
#include <mne/mne.h>

#include <fiff/fiff_io.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineParser>
#include <QBuffer>
#include <QtEndian>

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Disp Example");
    parser.addHelpOption();

    QCommandLineOption inputOption("file", "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    parser.addOption(inputOption);

    parser.process(a);

    QFile t_fileRaw(parser.value(inputOption));

    std::cout << "\n------------ Fiff Sniffer ------------\n";

    if(!t_fileRaw.exists())
    {
        std::cout << "Input file : " << t_fileRaw.fileName().toStdString() << "\n";
        std::cout << "File cannot be found.\n";
        std::cout << "\nBye!\n";

    } else { //file found


        QBuffer buffer;
        buffer.open(QIODevice::ReadWrite);
        t_fileRaw.open(QIODevice::ReadOnly);
        buffer.write(t_fileRaw.readAll());


        qint32_be iIntToChar;
        char cCharFromInt[sizeof (qint32)];

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

        buffer.reset();

        FIFFLIB::FiffStream stream(&buffer);

        std::cout << "Stream created";
        stream.setByteOrder(QDataStream::LittleEndian);

        stream.open();

        FIFFLIB::FiffInfo FifInfo;
        FIFFLIB::FiffDirNode::SPtr DirNode;



        if(!stream.read_meas_info(stream.dirtree(), FifInfo, DirNode)) {
            qCritical() << "Unable to parse fiff data.";
            return 0;
        }

        std::cout << "Sample frequency: " << FifInfo.sfreq << "\n";
        std::cout << "Nubmer of digitizer points: " << FifInfo.dig.size() << "\n";

        for (auto& point : FifInfo.dig){
            if (point.kind == FIFFV_POINT_HPI){
                std::cout << "HPI Point " << point.ident << " - " << point.r[0] << ", " << point.r[1] << ", " << point.r[2] << "\n";
            }
        }

//        FIFFLIB::FiffIO p_fiffIO(t_fileRaw);

//        std::cout << "Num. raw dat sets: " << p_fiffIO.m_qlistRaw.size() << "\n";
//        std::cout << "Num. evoked sets: " << p_fiffIO.m_qlistEvoked.size() << "\n";

//        int count = 0;
//        for (auto& data : p_fiffIO.m_qlistRaw){
//            std::cout << "--- \n";
//            std::cout << "Raw Set " << count << "\n";
//            std::cout << "Sample frequency: " << data->info.sfreq << "\n";
//            std::cout << "LineFreq: " << data->info.linefreq << " | Highpass: " << data->info.highpass << " | Lowpass: " << data->info.lowpass << "\n";
//            std::cout << "First sample: " << data->first_samp << " | Last sample: " << data->last_samp << "\n";
//            std::cout << "Number of samples: " << data->last_samp - data->first_samp << " | Time: " << (data->last_samp - data->first_samp) / data->info.sfreq << " sec. \n";
//            std::cout << "Nubmer of digitizer points: " << data->info.dig.size() << "\n";
//            for (auto& point : data->info.dig){
//                if (point.kind == FIFFV_POINT_HPI){
//                    std::cout << "HPI Point " << point.ident << " - " << point.r[0] << ", " << point.r[1] << ", " << point.r[2] << "\n";
//                }
//            }
//        }
//        count = 0;
//        for (auto& data : p_fiffIO.m_qlistEvoked){
//            std::cout << "--- \n";
//            std::cout << "Evoked Set " << count << "\n";
//            std::cout << "Sample frequency: " << data->info.sfreq << "\n";
//            std::cout << "LineFreq: " << data->info.linefreq << " | Highpass: " << data->info.highpass << " | Lowpass: " << data->info.lowpass << "\n";
//            std::cout << "Num. averaged epochs: " << data->nave << "\n";
//        }
//        std::cout << "--- \n";
//        std::cout << "--------------------------------------\n";
    }

    return 0;
}
