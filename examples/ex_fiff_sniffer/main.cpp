
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
    QCommandLineOption endianOption({"endian", "e", "en"}, "The file's endianness. 'little' or 'big'", "endian", "little");
    QCommandLineOption padEndOption("pad", "Adds -1 to end of stream to account for unfinished file (use for neuromag2ft)");

    parser.addOption(inputOption);
    parser.addOption(endianOption);

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
        buffer.open(QIODevice::ReadWrite);
        t_fileRaw.open(QIODevice::ReadOnly);
        buffer.write(t_fileRaw.readAll());


//        qint32_be iIntToChar;
//        char cCharFromInt[sizeof (qint32)];

//        iIntToChar = -1;
//        memcpy(cCharFromInt, &iIntToChar, sizeof(qint32));
//        buffer.write(cCharFromInt);
//        iIntToChar = -1;
//        memcpy(cCharFromInt, &iIntToChar, sizeof(qint32));
//        buffer.write(cCharFromInt);
//        iIntToChar = -1;
//        memcpy(cCharFromInt, &iIntToChar, sizeof(qint32));
//        buffer.write(cCharFromInt);
//        iIntToChar = -1;
//        memcpy(cCharFromInt, &iIntToChar, sizeof(qint32));
//        buffer.write(cCharFromInt);

        buffer.reset();

        FIFFLIB::FiffStream stream(&buffer);

        std::cout << "Stream created";
//        stream.setByteOrder(QDataStream::LittleEndian);

        stream.open();

        FIFFLIB::FiffInfo FifInfo;
        FIFFLIB::FiffDirNode::SPtr DirNode;

        if(stream.read_meas_info(stream.dirtree(), FifInfo, DirNode)) {
            std::cout << "Sample frequency: " << FifInfo.sfreq << "\n";
            std::cout << "Nubmer of digitizer points: " << FifInfo.dig.size() << "\n";

            for (auto& point : FifInfo.dig){
                if (point.kind == FIFFV_POINT_HPI){
                    std::cout << "HPI Point " << point.ident << " - " << point.r[0] << ", " << point.r[1] << ", " << point.r[2] << "\n";
                }
            }
        } else {
            FIFFLIB::FiffDigitizerData digData;

            if(stream.read_digitizer_data(stream.dirtree(), digData)) {
                std::cout << "GOT DIGITIZER DATA!!!\n";


                std::cout << "DIG POINTS:" << digData.points.size() << "\n";
                for (auto& point : digData.points){
                    if (point.kind == FIFFV_POINT_HPI){
                        std::cout << "HPI Point " << point.ident << " - " << point.r[0] << ", " << point.r[1] << ", " << point.r[2] << "\n";
                    }
                }
                std::cout << "mri_fids: " << digData.mri_fids;
            }

            ///////////////////////////////////////////////////////
            MneMshDisplaySurfaceSet* pMneMshDisplaySurfaceSet = new MneMshDisplaySurfaceSet();

            MneMshDisplaySurfaceSet::add_bem_surface(pMneMshDisplaySurfaceSet,
                                                     QCoreApplication::applicationDirPath() + "/resources/general/hpiAlignment/fsaverage-head.fif",
                                                     FIFFV_BEM_SURF_ID_HEAD,
                                                     "head",
                                                     1,
                                                     1);

            MneMshDisplaySurface* surface = pMneMshDisplaySurfaceSet->surfs[0];

            QFile t_fileDigDataReference(QCoreApplication::applicationDirPath() + "/resources/general/hpiAlignment/fsaverage-fiducials.fif");

            float scales[3];
            QScopedPointer<FiffDigitizerData> t_digDataReference(new FiffDigitizerData(t_fileDigDataReference));
            MneSurfaceOrVolume::align_fiducials(&digData,
                                                t_digDataReference.data(),
                                                surface,
                                                10,
                                                1,
                                                0,
                                                scales);


            std::cout << "\n";

            std::cout << digData.head_mri_t_adj->invmove(0) << "\n";
            std::cout << digData.head_mri_t_adj->invmove(1) << "\n";
            std::cout << digData.head_mri_t_adj->invmove(2) << "\n";

            ///////////////////////////////////////////////////////
        }

//        QBuffer buffer;
//        buffer.open(QIODevice::ReadWrite);
//        t_fileRaw.open(QIODevice::ReadOnly);
//        buffer.write(t_fileRaw.readAll());

//        t_fileRaw.close();

//        qint32_le iIntToChar;
//        char cCharFromInt[sizeof (qint32)];

//        iIntToChar = -1;
//        memcpy(cCharFromInt, &iIntToChar, sizeof(qint32));
//        buffer.write(cCharFromInt);
//        iIntToChar = -1;
//        memcpy(cCharFromInt, &iIntToChar, sizeof(qint32));
//        buffer.write(cCharFromInt);
//        iIntToChar = -1;
//        memcpy(cCharFromInt, &iIntToChar, sizeof(qint32));
//        buffer.write(cCharFromInt);
//        iIntToChar = -1;
//        memcpy(cCharFromInt, &iIntToChar, sizeof(qint32));
//        buffer.write(cCharFromInt);

//        buffer.reset();

//        FIFFLIB::FiffStream stream(&buffer);
//        if(littleEndian){
//            stream.setByteOrder(QDataStream::LittleEndian);
//        }else {
//            stream.setByteOrder(QDataStream::BigEndian);
//        }

//        stream.open();

//        FIFFLIB::FiffInfo FifInfo;
//        FIFFLIB::FiffDirNode::SPtr DirNode;

//        FIFFLIB::FiffDigitizerData digData;

//        if(stream.read_meas_info(stream.dirtree(), FifInfo, DirNode)) {

//            FIFFLIB::FiffIO p_fiffIO(t_fileRaw);

//            std::cout << "Num. raw dat sets: " << p_fiffIO.m_qlistRaw.size() << "\n";
//            std::cout << "Num. evoked sets: " << p_fiffIO.m_qlistEvoked.size() << "\n";

//            int count = 0;
//            for (auto& data : p_fiffIO.m_qlistRaw){
//                std::cout << "--- \n";
//                std::cout << "Raw Set " << count << "\n";
//                std::cout << "Sample frequency: " << data->info.sfreq << "\n";
//                std::cout << "LineFreq: " << data->info.linefreq << " | Highpass: " << data->info.highpass << " | Lowpass: " << data->info.lowpass << "\n";
//                std::cout << "First sample: " << data->first_samp << " | Last sample: " << data->last_samp << "\n";
//                std::cout << "Number of samples: " << data->last_samp - data->first_samp << " | Time: " << (data->last_samp - data->first_samp) / data->info.sfreq << " sec. \n";
//                std::cout << "Nubmer of digitizer points: " << data->info.dig.size() << "\n";
//                for (auto& point : data->info.dig){
//                    if (point.kind == FIFFV_POINT_HPI){
//                        std::cout << "HPI Point " << point.ident << " - " << point.r[0] << ", " << point.r[1] << ", " << point.r[2] << "\n";
//                    }
//                }
//            }
//            count = 0;
//            for (auto& data : p_fiffIO.m_qlistEvoked){
//                std::cout << "--- \n";
//                std::cout << "Evoked Set " << count << "\n";
//                std::cout << "Sample frequency: " << data->info.sfreq << "\n";
//                std::cout << "LineFreq: " << data->info.linefreq << " | Highpass: " << data->info.highpass << " | Lowpass: " << data->info.lowpass << "\n";
//                std::cout << "Num. averaged epochs: " << data->nave << "\n";
//            }
//            std::cout << "--- \n";
//            std::cout << "--------------------------------------\n";
//        }
//        if(stream.read_digitizer_data(stream.dirtree(), digData)) {
//            std::cout << "GOT DIGITIZER DATA!!!\n";
//            std::cout << "DIG POINTS:" << digData.points.size() << "\n";
//            for (auto& point : digData.points){
//                if (point.kind == FIFFV_POINT_HPI){
//                    std::cout << "HPI Point " << point.ident << " - " << point.r[0] << ", " << point.r[1] << ", " << point.r[2] << "\n";
//                }
//            }
//        }
    }

    return 0;
}
