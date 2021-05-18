
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <mne/mne.h>

//=============================================================================================================
// Eigen
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QCommandLineParser>

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("TF Example");
    parser.addHelpOption();

    QCommandLineOption inputOption("fileIn", "The input file <in>.", "in", QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
    parser.addOption(inputOption);

    QCommandLineOption fromOption("from", "Read data from <from> (in seconds).", "from", "42.956");
    parser.addOption(fromOption);

    QCommandLineOption toOption("to", "Read data from <to> (in seconds).", "to", "320.670");
    parser.addOption(toOption);

    parser.process(app);

    QFile t_fileRaw(parser.value(inputOption));

    float from = parser.value(fromOption).toFloat();
    float to = parser.value(toOption).toFloat();

    bool in_samples = false;
    bool keep_comp = false;


    FIFFLIB::FiffRawData raw(t_fileRaw);

    //
    //   Set up pick list: MEG + STI 014 - bad channels
    //
    //
    QStringList include;
    include << "STI 014";
    bool want_meg   = true;
    bool want_eeg   = false;
    bool want_stim  = false;

    Eigen::RowVectorXi picks = raw.info.pick_types(want_meg, want_eeg, want_stim, include, raw.info.bads);

    //
    //   Set up projection
    //
    qint32 k = 0;
    if (raw.info.projs.size() == 0)
        qInfo("No projector specified for these data\n");
    else
    {
        //
        //   Activate the projection items
        //
        for (k = 0; k < raw.info.projs.size(); ++k)
            raw.info.projs[k].active = true;

        qInfo("%d projection items activated\n",raw.info.projs.size());
        //
        //   Create the projector
        //
        FIFFLIB::fiff_int_t nproj = raw.info.make_projector(raw.proj);

        if (nproj == 0)
            qWarning("The projection vectors do not apply to these channels\n");
        else
            qInfo("Created an SSP operator (subspace dimension = %d)\n",nproj);
    }

    //
    //   Set up the CTF compensator
    //
    qint32 current_comp = raw.info.get_current_comp();
    qint32 dest_comp = 0;

    if (current_comp > 0)
        qInfo("Current compensation grade : %d\n",current_comp);

    if (keep_comp)
        dest_comp = current_comp;

    if (current_comp != dest_comp)
    {
        if(MNELIB::MNE::make_compensator(raw.info, current_comp, dest_comp, raw.comp))
        {
            raw.info.set_current_comp(dest_comp);
            qInfo("Appropriate compensator added to change to grade %d.\n",dest_comp);
        }
        else
        {
            qWarning("Could not make the compensator\n");
            return -1;
        }
    }
    //
    //   Read a data segment
    //   times output argument is optional
    //
    bool readSuccessful = false;
    Eigen::MatrixXd data;
    Eigen::MatrixXd times;
    if (in_samples)
        readSuccessful = raw.read_raw_segment(data, times, (qint32)from, (qint32)to, picks);
    else
        readSuccessful = raw.read_raw_segment_times(data, times, from, to, picks);

    if (!readSuccessful)
    {
        qWarning("Could not read raw segment.\n");
        return -1;
    }

    //data.

    return app.exec();
}
