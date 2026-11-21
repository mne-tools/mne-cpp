//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     test_fiff_rwr.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     December, 2015
 * @brief    Test for reading writing reading a fiff raw file
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/log.h>
#include <base/log_qt.h>
#include <fiff/fiff.h>
#include <fiff/fiff_dig_point_set.h>
#include <mne/mne_forward_solution.h>
#include <base/byte_stream.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QBuffer>
#include <QDirIterator>
#include <QTemporaryDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <iostream>
#include <stdexcept>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE LOCAL FUNCTIONS
//=============================================================================================================

namespace
{

QString testDataDir()
{
    return QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data";
}

std::uint64_t fnv1a(const QByteArray& bytes)
{
    std::uint64_t hash = 1469598103934665603ULL;
    for (const char c : bytes)
        hash = (hash ^ static_cast<unsigned char>(c)) * 1099511628211ULL;
    return hash;
}

std::unique_ptr<mne::MemoryByteStream> memoryOf(const QByteArray& bytes)
{
    const auto* begin = reinterpret_cast<const std::byte*>(bytes.constData());
    return std::make_unique<mne::MemoryByteStream>(std::vector<std::byte>(begin, begin + bytes.size()));
}

QByteArray bytesOf(const mne::MemoryByteStream& stream)
{
    return QByteArray(reinterpret_cast<const char*>(stream.data().data()), static_cast<qsizetype>(stream.size()));
}

// File bytes with the file and block IDs zeroed, since they carry the time of writing
QByteArray stableBytes(QByteArray bytes)
{
    FiffStream stream(memoryOf(bytes));
    if (!stream.open())
        return {};
    const std::vector<FiffDirEntry> dir = stream.dir();
    stream.close();
    for (const FiffDirEntry& entry : dir) {
        if (entry.pos >= 0 && (entry.kind == FIFF_FILE_ID || entry.kind == FIFF_BLOCK_ID || entry.kind == FIFF_PARENT_FILE_ID || entry.kind == FIFF_PARENT_BLOCK_ID))
            bytes.replace(entry.pos + 16, entry.size, QByteArray(entry.size, '\0'));
    }
    return bytes;
}

// Kind, type and size of every tag in order: computed float data may round differently per compiler
QByteArray tagLayout(const QByteArray& bytes)
{
    FiffStream stream(memoryOf(bytes));
    if (!stream.open())
        return {};
    QByteArray layout;
    for (const FiffDirEntry& entry : stream.dir())
        layout += QByteArray::number(entry.kind) + ':' + QByteArray::number(entry.type) + ':' + QByteArray::number(entry.size) + ';';
    stream.close();
    return layout;
}

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestFiffRWR
 *
 * @brief The TestFiffRWR class provides read write read fiff verification tests
 *
 */
class TestFiffRWR : public QObject
{
    Q_OBJECT

public:
    TestFiffRWR();

private slots:
    void initTestCase();
    void compareData();
    void compareTimes();
    void compareInfo();
    void tagsRewriteByteIdentical_data();
    void tagsRewriteByteIdentical();
    void readsFromAByteStream();
    void opensAndStartsFilesByPath();
    void readsRawByPathAndStream();
    void writersStable_data();
    void writersStable();
    void cleanupTestCase();

private:
    double dEpsilon;
    QTemporaryDir m_tempDir;

    FiffRawData rawFirstInRaw;

    MatrixXd mFirstInData;
    MatrixXd mFirstInTimes;

    FiffRawData rawSecondInRaw;

    MatrixXd mSecondInData;
    MatrixXd mSecondInTimes;
};

//=============================================================================================================

TestFiffRWR::TestFiffRWR()
: dEpsilon(0.000001)
{
}

//=============================================================================================================

void TestFiffRWR::initTestCase()
{
    mne::install_qt_message_handler();
    qDebug() << "dEpsilon" << dEpsilon;

    QVERIFY(m_tempDir.isValid());

    //*********************************************************************************************************
    // First Read & Write
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Read & Write >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    //
    //   Setup for reading the raw data
    //
    rawFirstInRaw = FiffRawData((QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());

    //
    //   Set up pick list: MEG + STI 014 - bad channels
    //
    //
    bool bWantMeg = true;
    bool bWantEeg = false;
    bool bWantStim = false;
    StringList include{"STI 014"};

    MatrixXi vPicks = rawFirstInRaw.info.pick_types(bWantMeg, bWantEeg, bWantStim, include, rawFirstInRaw.info.bads); // prefer member function
    if (vPicks.cols() == 0) {
        include = {"STI 101", "STI 201", "STI 301"};
        vPicks = rawFirstInRaw.info.pick_types(bWantMeg, bWantEeg, bWantStim, include, rawFirstInRaw.info.bads); // prefer member function
        if (vPicks.cols() == 0) {
            printf("channel list may need modification\n");
        }
    }
    //
    RowVectorXd vCals;

    auto outfid = FiffStream::start_writing_raw(m_tempDir.filePath("sample_audvis_trunc_raw_test_rwr_out.fif").toStdString(), rawFirstInRaw.info, vCals /*, vPicks*/);

    //
    //   Set up the reading parameters
    //
    float fQuantumSec = 1.0f; //read and write in 1 sec junks
    fiff_int_t from = rawFirstInRaw.first_samp;
    fiff_int_t to = rawFirstInRaw.last_samp;
    fiff_int_t quantum = ceil(fQuantumSec * rawFirstInRaw.info.sfreq);

    //
    //   To read the whole file at once set
    //
    //quantum     = to - from + 1;
    //
    //
    //   Read and write all the data
    //
    bool bFirstBuffer = true;

    fiff_int_t first, last;

    for (first = from; first < to; first += quantum) {
        last = first + quantum - 1;
        if (last > to) {
            last = to;
        }

        if (!rawFirstInRaw.read_raw_segment(mFirstInData, mFirstInTimes, first, last /*,vPicks*/)) {
            printf("error during read_raw_segment\n");
        }
        //
        //   You can add your own miracle here
        //
        printf("Writing...");
        if (bFirstBuffer) {
            if (first > 0)
                outfid->write_int(FIFF_FIRST_SAMPLE, &first);
            bFirstBuffer = false;
        }
        outfid->write_raw_buffer(mFirstInData, vCals);
        printf("[done]\n");
    }

    outfid->finish_writing_raw();

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Read & Write Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");

    //*********************************************************************************************************
    // Second Read
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Read Again >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    //
    //   Setup for reading the raw data from out file
    //
    rawSecondInRaw = FiffRawData(m_tempDir.filePath("sample_audvis_trunc_raw_test_rwr_out.fif").toStdString());

    //
    //   Set up pick list: MEG + STI 014 - bad channels
    //
    //

    vPicks = rawSecondInRaw.info.pick_types(bWantMeg, bWantEeg, bWantStim, include, rawSecondInRaw.info.bads); // prefer member function
    if (vPicks.cols() == 0) {
        include = {"STI 101", "STI 201", "STI 301"};
        vPicks = rawSecondInRaw.info.pick_types(bWantMeg, bWantEeg, bWantStim, include, rawSecondInRaw.info.bads); // prefer member function
        if (vPicks.cols() == 0) {
            printf("channel list may need modification\n");
        }
    }

    //
    //   Read all the data
    //

    for (first = from; first < to; first += quantum) {
        last = first + quantum - 1;
        if (last > to) {
            last = to;
        }

        if (!rawSecondInRaw.read_raw_segment(mSecondInData, mSecondInTimes, first, last /*,vPicks*/)) {
            printf("error during read_raw_segment\n");
        }
    }

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Read Again Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestFiffRWR::compareData()
{
    MatrixXd mDataDiff = mFirstInData - mSecondInData;
    //    std::cout << "\tCompare data:\n";
    //    std::cout << "\tFirst data\n" << mFirstInData.block(0,0,4,4) << "\n";
    //    std::cout << "\tSecond data\n" << mSecondInData.block(0,0,4,4) << "\n";

    QVERIFY(mDataDiff.sum() < dEpsilon);
}

//=============================================================================================================

void TestFiffRWR::compareTimes()
{
    MatrixXd mTimesDiff = mFirstInTimes - mSecondInTimes;
    //    std::cout << "\tCompare Times:\n";
    //    std::cout << "\tFirst times\n" << mFirstInTimes.block(0,0,1,4) << "\n";
    //    std::cout << "\tSecond times\n" << mSecondInTimes.block(0,0,1,4) << "\n";

    QVERIFY(mTimesDiff.sum() < dEpsilon);
}

//=============================================================================================================

void TestFiffRWR::compareInfo()
{
    //Sampling frequency
    std::cout << "[1] Sampling Frequency Check\n";
    QVERIFY(rawFirstInRaw.info.sfreq == rawSecondInRaw.info.sfreq);

    //Projection
    std::cout << "[2] Projection Check\n";
    QVERIFY(rawFirstInRaw.info.projs.size() == rawSecondInRaw.info.projs.size());

    for (qint32 i = 0; i < std::ssize(rawFirstInRaw.info.projs); ++i) {
        std::cout << "Projector " << i << std::endl;
        MatrixXd mTmp = rawFirstInRaw.info.projs[i].data.data - rawSecondInRaw.info.projs[i].data.data;
        QVERIFY(mTmp.sum() < dEpsilon);
    }

    //Compensators
    std::cout << "[3] Compensator Check\n";
    QVERIFY(rawFirstInRaw.info.comps.size() == rawSecondInRaw.info.comps.size());

    for (qint32 i = 0; i < std::ssize(rawFirstInRaw.info.comps); ++i) {
        std::cout << "Compensator " << i << std::endl;
        MatrixXd mTmp = rawFirstInRaw.info.comps[i].data.data - rawSecondInRaw.info.comps[i].data.data;
        QVERIFY(mTmp.sum() < dEpsilon);
    }
}

//=============================================================================================================

void TestFiffRWR::tagsRewriteByteIdentical_data()
{
    QTest::addColumn<QString>("path");
    QDirIterator it(testDataDir(), {"*.fif"}, QDir::Files, QDirIterator::Subdirectories);
    QStringList paths;
    while (it.hasNext())
        paths << it.next();
    paths.sort();
    for (const QString& path : std::as_const(paths))
        QTest::newRow(qPrintable(QDir(testDataDir()).relativeFilePath(path))) << path;
}

//=============================================================================================================

void TestFiffRWR::tagsRewriteByteIdentical()
{
    QFETCH(QString, path);

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray original = file.readAll();
    file.close();

    FiffStream stream(path.toStdU16String());
    QVERIFY(stream.open());
    QVERIFY(!stream.dir().empty());

    // Every tag read and written again must reproduce the bytes in the file, header and payload
    for (const FiffDirEntry& entry : stream.dir()) {
        // The stored directory ends with a sentinel entry
        if (entry.pos < 0)
            continue;
        const std::optional<FiffTag> tag = stream.read_tag(entry.pos);
        QVERIFY(tag);
        QCOMPARE(tag->kind, entry.kind);
        auto memory = std::make_unique<mne::MemoryByteStream>();
        const mne::MemoryByteStream& written = *memory;
        FiffStream out(std::move(memory));
        out.write_tag(*tag, 0);
        const QByteArray expected = original.mid(entry.pos, 16 + entry.size);
        if (bytesOf(written) != expected)
            QFAIL(qPrintable(QString("tag kind %1 type %2 at %3 differs").arg(entry.kind).arg(entry.type).arg(entry.pos)));
    }
    const FiffDirEntry& first = stream.dirtree()->dir.front();
    const auto found = stream.dirtree()->find_tag(stream, first.kind);
    QVERIFY(found);
    QCOMPARE(found->kind, first.kind);
    QVERIFY(!stream.dirtree()->find_tag(stream, FIFF_MNE_ENV_COMMAND_LINE));
    stream.close();
}

//=============================================================================================================

void TestFiffRWR::readsRawByPathAndStream()
{
    // A path and a stream on bytes in memory give the same raw data
    const QString path = testDataDir() + "/MEG/sample/sample_audvis_trunc_raw.fif";
    const FiffRawData byPath(path.toStdString());
    QFile qtFile(path);
    QVERIFY(qtFile.open(QIODevice::ReadOnly));
    std::vector<std::byte> fileBytes(static_cast<std::size_t>(qtFile.size()));
    QVERIFY(qtFile.read(reinterpret_cast<char*>(fileBytes.data()), qtFile.size()) == qtFile.size());
    qtFile.close();
    const FiffRawData inMemory(std::make_unique<mne::MemoryByteStream>(std::move(fileBytes)));
    const fiff_int_t from = byPath.first_samp + 1000;
    const fiff_int_t to = from + 2 * static_cast<fiff_int_t>(byPath.info.sfreq);
    MatrixXd expected, times, data;
    QVERIFY(byPath.read_raw_segment(expected, times, from, to));
    QVERIFY(byPath.read_raw_segment(data, times, from, to));
    QVERIFY(data == expected);
    QVERIFY(inMemory.read_raw_segment(data, times, from, to));
    QVERIFY(data == expected);
    QCOMPARE(byPath.info.nchan, inMemory.info.nchan);

    QVERIFY_THROWS_EXCEPTION(mne::Error, FiffRawData(m_tempDir.filePath("missing_raw.fif").toStdString()));
    QCOMPARE(FiffRawData::read(m_tempDir.filePath("missing_raw.fif").toStdString()).error().code, mne::ErrorCode::Io);
    QCOMPARE(FiffRawData::read(path.toStdString()).value().last_samp, byPath.last_samp);
    QCOMPARE(FiffRawData::read(std::unique_ptr<mne::ByteStream>()).error().code, mne::ErrorCode::Io);

    // Writing to a file and to bytes in memory gives the same bytes
    const std::string outPath = m_tempDir.filePath("written_raw.fif").toStdString();
    RowVectorXd cals;
    auto toFile = FiffStream::start_writing_raw(outPath, byPath.info, cals);
    QVERIFY(toFile);
    toFile->write_raw_buffer(expected, cals);
    toFile->finish_writing_raw();
    auto memory = std::make_unique<mne::MemoryByteStream>();
    const mne::MemoryByteStream& written = *memory;
    auto toMemory = FiffStream::start_writing_raw(std::move(memory), byPath.info, cals);
    QVERIFY(toMemory);
    toMemory->write_raw_buffer(expected, cals);
    toMemory->finish_writing_raw();
    QFile fileOut(QString::fromStdString(outPath));
    QVERIFY(fileOut.open(QIODevice::ReadOnly));
    QCOMPARE(stableBytes(fileOut.readAll()), stableBytes(QByteArray(reinterpret_cast<const char*>(written.data().data()), static_cast<qsizetype>(written.size()))));
    QVERIFY(!FiffStream::start_writing_raw(std::unique_ptr<mne::ByteStream>(), byPath.info, cals));
    QVERIFY(!FiffStream::start_writing_raw(m_tempDir.filePath("no/dir/raw.fif").toStdString(), byPath.info, cals));

    const std::string savedPath = m_tempDir.filePath("saved_raw.fif").toStdString();
    QVERIFY(byPath.save(savedPath, RowVectorXi(), 1, from, to));
    const FiffRawData back(savedPath);
    QVERIFY(back.read_raw_segment(data, times, from, to));
    QVERIFY(data.isApprox(expected, 1e-6));
}

//=============================================================================================================

void TestFiffRWR::readsFromAByteStream()
{
    // A stream on a file byte stream sees the same directory and tags as one on its path
    const QString path = testDataDir() + "/MEG/sample/sample_audvis-cov.fif";
    FiffStream byPath(path.toStdU16String());
    QVERIFY(byPath.open());
    QVERIFY(byPath.is_open());

    auto bytes = mne::FileByteStream::open(path.toStdString(), mne::FileByteStream::Mode::Read);
    QVERIFY(bytes);
    FiffStream stream(std::move(bytes));
    QVERIFY(stream.open());
    QCOMPARE(stream.nent(), byPath.nent());
    QCOMPARE(stream.id().machid[0], byPath.id().machid[0]);

    for (int i = 0; i < stream.nent(); ++i) {
        const FiffDirEntry& entry = stream.dir()[i];
        if (entry.pos < 0)
            continue;
        const std::optional<FiffTag> tag = stream.read_tag(entry.pos);
        QVERIFY(tag);
        const std::optional<FiffTag> pathTag = byPath.read_tag(entry.pos);
        QVERIFY(pathTag);
        QCOMPARE(tag->kind, pathTag->kind);
        QVERIFY(std::ranges::equal(tag->bytes(), pathTag->bytes()));
    }

    FiffCov cov;
    QVERIFY(stream.read_cov(stream.dirtree(), FIFFV_MNE_NOISE_COV, cov));
    QCOMPARE(cov.dim, 366);
    byPath.close();
}

//=============================================================================================================

void TestFiffRWR::opensAndStartsFilesByPath()
{
    const std::string path = (testDataDir() + "/MEG/sample/all-trans.fif").toStdString();
    auto in = FiffStream::open(path);
    QVERIFY(in);
    QVERIFY(in->is_open());
    QCOMPARE(in->stream_name(), QString::fromStdString(path));
    QVERIFY(in->nent() > 0);
    const auto missing = FiffStream::open(m_tempDir.filePath("missing.fif").toStdString());
    QVERIFY(!missing);
    QCOMPARE(missing.error().code, mne::ErrorCode::Io);
    QFile text(m_tempDir.filePath("text.fif"));
    QVERIFY(text.open(QIODevice::WriteOnly));
    text.write("not a FIFF file");
    text.close();
    const auto notFiff = FiffStream::open(text.fileName().toStdString());
    QVERIFY(!notFiff);
    QCOMPARE(notFiff.error().code, mne::ErrorCode::Format);
    QVERIFY(std::string_view(notFiff.error().what()).contains("text.fif"));

    // The stream is a value: it moves out of the result and keeps reading.
    FiffStream moved = std::move(*FiffStream::open(path));
    QCOMPARE(moved.nent(), in->nent());

    // A closed stream reopens its file and still reads by the directory it read before
    const std::optional<FiffTag> firstTag = in->read_tag(in->dir()[1].pos);
    QVERIFY(firstTag);
    in->close();
    QVERIFY(!in->is_open());
    QVERIFY(in->reopen());
    QVERIFY(in->is_open());
    const std::optional<FiffTag> again = in->read_tag(in->dir()[1].pos);
    QVERIFY(again);
    QCOMPARE(again->kind, firstTag->kind);
    QVERIFY(std::ranges::equal(again->bytes(), firstTag->bytes()));
    QVERIFY(in->reopen());

    // A file started by path is complete on disk once the stream is closed, before the stream goes away
    const std::string outPath = m_tempDir.filePath("by-path.fif").toStdString();
    auto out = FiffStream::start_file(outPath);
    QVERIFY(out);
    for (const FiffDirEntry& entry : in->dir()) {
        if (entry.kind != FIFF_COORD_TRANS)
            continue;
        if (const auto tag = in->read_tag(entry.pos))
            out->write_tag(*tag);
    }
    out->end_file();
    out->close();
    QVERIFY(!out->is_open());
    auto back = FiffStream::open(outPath);
    QVERIFY(back);
    int transforms = 0;
    for (const FiffDirEntry& entry : back->dir())
        transforms += entry.kind == FIFF_COORD_TRANS ? 1 : 0;
    QVERIFY(transforms > 0);
    QVERIFY(!FiffStream::start_file(m_tempDir.filePath("no/dir/out.fif").toStdString()));
}

//=============================================================================================================

void TestFiffRWR::writersStable_data()
{
    QTest::addColumn<QString>("writer");
    QTest::addColumn<qulonglong>("hash");
    QTest::addColumn<qlonglong>("size");
    QTest::addColumn<bool>("layoutOnly");
    QTest::newRow("raw") << "raw" << qulonglong(0x8916c3f09038d573ULL) << qlonglong(12036856) << false;
    QTest::newRow("evoked") << "evoked" << qulonglong(0xf6bbb2083ff32b99ULL) << qlonglong(2581903) << true;
    QTest::newRow("covariance") << "cov" << qulonglong(0x698353a3817538d8ULL) << qlonglong(546446) << false;
    QTest::newRow("transform") << "trans" << qulonglong(0x9199331869f76d3eULL) << qlonglong(212) << false;
    QTest::newRow("digitizer") << "dig" << qulonglong(0xc59bdb7c1024cdc8ULL) << qlonglong(5468) << false;
    QTest::newRow("forward") << "fwd" << qulonglong(0x176c6bb765acd4afULL) << qlonglong(55425088) << true;
}

//=============================================================================================================

void TestFiffRWR::writersStable()
{
    QFETCH(QString, writer);
    QFETCH(qulonglong, hash);
    QFETCH(qlonglong, size);
    QFETCH(bool, layoutOnly);

    // Pins the bytes each writer produces for a fixture, so moving the stream layer cannot change the files
    QByteArray bytes;
    const QString meg = testDataDir() + "/MEG/sample/";
    if (writer == "raw") {
        const FiffRawData raw((meg + "sample_audvis_trunc_raw.fif").toStdString());
        const QString path = m_tempDir.filePath("stable-raw.fif");
        QVERIFY(raw.save(path.toStdString()));
        QFile written(path);
        QVERIFY(written.open(QIODevice::ReadOnly));
        bytes = written.readAll();
    } else if (writer == "evoked") {
        const FiffEvokedSet evoked((meg + "sample_audvis-ave.fif").toStdU16String());
        const QString path = m_tempDir.filePath("stable-ave.fif");
        QVERIFY(evoked.save(path.toStdU16String()));
        QFile written(path);
        QVERIFY(written.open(QIODevice::ReadOnly));
        bytes = written.readAll();
    } else if (writer == "cov") {
        const FiffCov cov((meg + "sample_audvis-cov.fif").toStdString());
        const QString path = m_tempDir.filePath("stable-cov.fif");
        QVERIFY(cov.save(path.toStdString()));
        QFile written(path);
        QVERIFY(written.open(QIODevice::ReadOnly));
        bytes = written.readAll();
    } else if (writer == "trans") {
        const FiffCoordTrans trans((meg + "all-trans.fif").toStdString());
        const QString path = m_tempDir.filePath("stable-trans.fif");
        QVERIFY(trans.write(path.toStdString()));
        QFile written(path);
        QVERIFY(written.open(QIODevice::ReadOnly));
        bytes = written.readAll();
    } else if (writer == "dig") {
        FiffDigPointSet dig((meg + "sample_audvis_trunc_raw.fif").toStdString());
        QVERIFY(dig.size() > 0);
        const QString path = m_tempDir.filePath("stable-dig.fif");
        QVERIFY(dig.write(path.toStdString()));
        QFile written(path);
        QVERIFY(written.open(QIODevice::ReadOnly));
        bytes = written.readAll();
    } else if (writer == "fwd") {
        const mne::MNEForwardSolution fwd((testDataDir() + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif").toStdU16String());
        const QString path = m_tempDir.filePath("stable-fwd.fif");
        QVERIFY(fwd.write(path.toStdU16String()));
        QFile written(path);
        QVERIFY(written.open(QIODevice::ReadOnly));
        bytes = written.readAll();
    }

    const QByteArray stable = stableBytes(bytes);
    QVERIFY(!stable.isEmpty());
    QCOMPARE(qlonglong(stable.size()), size);
    QCOMPARE(qulonglong(fnv1a(layoutOnly ? tagLayout(stable) : stable)), hash);
}

//=============================================================================================================

void TestFiffRWR::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffRWR)
#include "test_fiff_rwr.moc"
