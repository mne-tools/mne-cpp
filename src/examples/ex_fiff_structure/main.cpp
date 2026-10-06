//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    FIFF library: fixed-length epochs, CTF compensation, the file directory and raw buffers, and handing
 *           recordings to another application.
 *
 * Compares every value with MNE-Python 1.11 (make_fixed_length_epochs, make_compensator, fiff_open,
 * read_raw_fif) on the sample recording and a CTF test file. Exits non-zero on any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_constants.h>
#include <fiff/fiff_ctf_comp.h>
#include <fiff/fiff_data_ref.h>
#include <fiff/fiff_dir_entry.h>
#include <fiff/fiff_epochs.h>
#include <fiff/fiff_file_sharer.h>
#include <fiff/fiff_id.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_raw_dir.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_time.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QEventLoop>
#include <QTimer>
#include <QTemporaryDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdlib>
#include <limits>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

bool expect(bool condition, const QString& what)
{
    qInfo().noquote() << (condition ? "  ok  " : "  FAIL") << what;
    return condition;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    QCommandLineOption dataOption("data", "MNE-CPP test data <dir>.", "dir",
                                  QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data");
    QCommandLineOption ctfOption("ctf", "CTF raw file with compensation matrices.", "file", QString(MNE_CTF_FILE));
    parser.addOption(dataOption);
    parser.addOption(ctfOption);
    parser.process(app);
    const QString rawPath = parser.value(dataOption) + "/MEG/sample/sample_audvis_trunc_raw.fif";
    QTemporaryDir tmp;
    bool ok = tmp.isValid();

    //! [fiff_epochs_usage]
    QFile rawFile(rawPath);
    FiffRawData raw(rawFile);
    MatrixXd data;
    MatrixXd times;
    ok &= raw.read_raw_segment(data, times); // all 6007 samples
    const double sfreq = raw.info.sfreq;
    const QList<FiffEpochData> epochs = FiffEpochs::makeFixedLengthEpochs(data, sfreq, 2.0, 0.5); // 2 s, 0.5 s overlap
    const FiffEvoked average = FiffEpochs::averageEpochs(epochs, sfreq);
    //! [fiff_epochs_usage]
    // mne.make_fixed_length_epochs(raw, duration=2, overlap=0.5): 13 epochs of 601 samples, 450 samples apart
    ok &= expect(epochs.size() == 13 && epochs[0].data.cols() == 601 && std::lround((epochs[1].tmin - epochs[0].tmin) * sfreq) == 450 && average.nave == 13,
                 "FiffEpochs/FiffEpochData: 13 fixed-length epochs like mne.make_fixed_length_epochs, averaged over 13");

    //! [fiff_raw_dir_usage]
    // The raw data directory: one FiffRawDir per data buffer, each pointing at its FiffDirEntry in the file.
    int samples = 0;
    for (const FiffRawDir& buffer : raw.rawdir)
        samples += buffer.nsamp;
    const FiffDirEntry::SPtr firstBuffer = raw.rawdir.first().ent;
    //! [fiff_raw_dir_usage]
    // mne.io.read_raw_fif: 2 buffers, 6007 samples from first_samp 12900; fiff_open lists 640 directory entries
    ok &= expect(raw.rawdir.size() == 2 && samples == 6007 && raw.rawdir.first().first == 12900 && firstBuffer->kind == FIFF_DATA_BUFFER,
                 "FiffRawDir/FiffDirEntry: 2 data buffers cover the 6007 samples from sample 12900");

    //! [fiff_dir_entry_usage]
    QFile structureFile(rawPath);
    FiffStream::SPtr stream(new FiffStream(&structureFile));
    ok &= stream->open();
    const QList<FiffDirEntry::SPtr>& directory = stream->dir(); // every tag (kind, type, size, position) + a kind -1 end
    const FiffTime created = stream->id().time;                 // seconds and microseconds since 1970
    stream->close();
    //! [fiff_dir_entry_usage]
    // mne fiff_open: 640 entries; file_id secs 1411146352, usecs 0
    ok &= expect(directory.size() == 641 && directory.last()->kind == -1 && created.secs == 1411146352 && created.usecs == 0 && FiffTime::storageSize() == 8 && FiffDirEntry::storageSize() == 16,
                 "FiffDirEntry/FiffTime: the 640 tags of mne fiff_open plus the end entry; the file id was written at 1411146352 s");

    //! [fiff_data_ref_usage]
    // A reference to data in an external file: 2 x 32-bit type and byte order, 2 x 64-bit size and offset.
    FiffDataRef reference;
    reference.size = 3LL << 31; // beyond 2 GiB
    reference.offset = 1024;
    //! [fiff_data_ref_usage]
    ok &= expect(FiffDataRef::storageSize() == 24 && reference.size > std::numeric_limits<qint32>::max(), "FiffDataRef: 24-byte record with 64-bit size and offset");

    //! [fiff_ctf_comp_usage]
    // CTF third-order gradient compensation: info.comps holds one FiffCtfComp per grade.
    QFile ctfFile(parser.value(ctfOption));
    const FiffRawData ctf(ctfFile);
    FiffCtfComp toGrade1;
    ok &= ctf.info.make_compensator(0, 1, toGrade1); // uncompensated -> grade 1
    //! [fiff_ctf_comp_usage]
    // mne._fiff.compensator.make_compensator(info, 0, 1) on ctf_grade0_raw.fif: 36 x 36, ||C - I||_F = 0.1947596520139745
    const MatrixXd& compensator = toGrade1.data->data;
    ok &= expect(ctf.info.comps.size() == 5 && compensator.rows() == 36 && std::abs((compensator - MatrixXd::Identity(36, 36)).norm() - 0.1947596520139745) < 1e-6,
                 "FiffCtfComp: 5 compensation grades; the grade-1 compensator matches mne make_compensator");

    //! [fiff_file_sharer_usage]
    // Hand a finished recording to another application through a watched directory.
    QDir::setCurrent(tmp.path());
    FiffFileSharer consumer("shared");
    QStringList arrived;
    QObject::connect(&consumer, &FiffFileSharer::newFileAtPath, [&](const QString& path) { arrived << path; });
    consumer.initWatcher();
    FiffFileSharer producer("shared");
    producer.copyRealtimeFile(rawPath); // shared/realtime_file0_raw.fif
    //! [fiff_file_sharer_usage]
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    QObject::connect(&consumer, &FiffFileSharer::newFileAtPath, &loop, &QEventLoop::quit);
    if (arrived.isEmpty())
        loop.exec();
    QFile shared(tmp.filePath("shared/realtime_file0_raw.fif"));
    const FiffRawData sharedRaw(shared);
    ok &= expect(!arrived.isEmpty() && sharedRaw.last_samp - sharedRaw.first_samp + 1 == 6007,
                 "FiffFileSharer: the copied recording arrives at the watcher and reads back with all 6007 samples");

    qInfo().noquote() << (ok ? "All fiff structure checks passed." : "fiff structure checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
