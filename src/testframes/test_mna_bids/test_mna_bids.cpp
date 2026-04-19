//=============================================================================================================
/**
 * @file     test_mna_bids.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Integration tests for MNA/MNX ↔ BIDS operations:
 *           - Extract embedded .mnx to BIDS directory tree
 *           - Pack BIDS directory into .mnx/.mna
 *           - Convert between .mna and .mnx
 *           - mne_show_mna CLI output
 *           - mne_mna_bids_converter CLI subcommands
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mna/mna_io.h>
#include <mna/mna_project.h>
#include <mna/mna_subject.h>
#include <mna/mna_session.h>
#include <mna/mna_recording.h>
#include <mna/mna_file_ref.h>
#include <mna/mna_types.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QTemporaryDir>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestMnaBids
 *
 * @brief Integration tests for MNA/MNX ↔ BIDS operations and CLI tools.
 */
class TestMnaBids : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // ── BIDS-structured MNX round-trip ──────────────────────────────
    void testCreateBidsStructuredMnx();
    void testExtractMnxToBidsDir();
    void testExtractPreservesDirStructure();
    void testExtractedDataIntegrity();

    // ── Pack BIDS → MNX ─────────────────────────────────────────────
    void testPackBidsDirToMnx();
    void testPackBidsDirToMna();
    void testPackWithSidecar();

    // ── Convert .mna ↔ .mnx ─────────────────────────────────────────
    void testConvertMnaToMnx();
    void testConvertMnxToMna();
    void testConvertMnaToMnxEmbedsData();

    // ── CLI: mne_show_mna ───────────────────────────────────────────
    void testShowMnaHelp();
    void testShowMnaDisplaysProject();
    void testShowMnaVerbose();
    void testShowMnaJsonOutput();
    void testShowMnaInvalidFile();

    // ── CLI: mne_mna_bids_converter ─────────────────────────────────
    void testConverterHelp();
    void testConverterExtract();
    void testConverterPack();
    void testConverterConvert();
    void testConverterInvalidCommand();

    // ── Edge cases ──────────────────────────────────────────────────
    void testExtractEmptyProject();
    void testPackEmptyDirectory();
    void testMultiSubjectMultiSession();

    void cleanupTestCase();

private:
    MnaProject createTestProject(bool embed = true);
    void createBidsTree(const QString &rootDir);
    QString findTool(const QString &name);

    QTemporaryDir m_tempDir;
    QString m_showMnaPath;
    QString m_converterPath;
};

//=============================================================================================================
// INIT / CLEANUP
//=============================================================================================================

void TestMnaBids::initTestCase()
{
    QVERIFY(m_tempDir.isValid());

    // Locate CLI tools relative to test binary
    m_showMnaPath = findTool("mne_show_mna");
    m_converterPath = findTool("mne_mna_bids_converter");

    qInfo() << "mne_show_mna:" << m_showMnaPath;
    qInfo() << "mne_mna_bids_converter:" << m_converterPath;
}

void TestMnaBids::cleanupTestCase()
{
}

QString TestMnaBids::findTool(const QString &name)
{
    // Look in same directory as the test binary
    const QString appDir = QCoreApplication::applicationDirPath();
    QString path = appDir + "/" + name;
    if (QFile::exists(path))
        return path;

    // Look in sibling bin/ directory (tests/ → bin/)
    path = appDir + "/../bin/" + name;
    if (QFile::exists(path))
        return QFileInfo(path).absoluteFilePath();

    // macOS app bundle: go up from .app/Contents/MacOS
    path = appDir + "/../../../" + name;
    if (QFile::exists(path))
        return QFileInfo(path).absoluteFilePath();

    return QString();
}

//=============================================================================================================
// HELPER: create test project with embedded BIDS-structured file refs
//=============================================================================================================

MnaProject TestMnaBids::createTestProject(bool embed)
{
    MnaProject proj;
    proj.name = "BIDS Test Project";
    proj.description = "Test project for BIDS integration tests";
    proj.mnaVersion = MnaProject::CURRENT_SCHEMA_VERSION;
    proj.modified = QDateTime::currentDateTimeUtc();

    MnaSubject subj;
    subj.id = "sample";

    MnaSession sess;
    sess.id = "01";

    MnaRecording rec;
    rec.id = "recording-01";

    // Surface
    {
        MnaFileRef ref;
        ref.role = MnaFileRole::Surface;
        ref.path = "sub-sample/ses-01/anat/lh.pial";
        ref.format = "pial";
        ref.embedded = embed;
        if (embed) {
            ref.data = QByteArray(256, '\x11');
            ref.sizeBytes = ref.data.size();
        }
        rec.files.append(ref);
    }

    // Annotation
    {
        MnaFileRef ref;
        ref.role = MnaFileRole::Annotation;
        ref.path = "sub-sample/ses-01/anat/lh.aparc.annot";
        ref.format = "annot";
        ref.embedded = embed;
        if (embed) {
            ref.data = QByteArray(128, '\x22');
            ref.sizeBytes = ref.data.size();
        }
        rec.files.append(ref);
    }

    // BEM
    {
        MnaFileRef ref;
        ref.role = MnaFileRole::Bem;
        ref.path = "sub-sample/ses-01/bem/sample-bem.fif";
        ref.format = "fif";
        ref.embedded = embed;
        if (embed) {
            ref.data = QByteArray(512, '\x33');
            ref.sizeBytes = ref.data.size();
        }
        rec.files.append(ref);
    }

    // Digitizer
    {
        MnaFileRef ref;
        ref.role = MnaFileRole::Digitizer;
        ref.path = "sub-sample/ses-01/meg/sample_raw.fif";
        ref.format = "fif";
        ref.embedded = embed;
        if (embed) {
            ref.data = QByteArray(64, '\x44');
            ref.sizeBytes = ref.data.size();
        }
        rec.files.append(ref);
    }

    // Source estimate
    {
        MnaFileRef ref;
        ref.role = MnaFileRole::SourceEstimate;
        ref.path = "sub-sample/ses-01/source/result-lh.stc";
        ref.format = "stc";
        ref.embedded = embed;
        if (embed) {
            ref.data = QByteArray(1024, '\x55');
            ref.sizeBytes = ref.data.size();
        }
        rec.files.append(ref);
    }

    sess.recordings.append(rec);
    subj.sessions.append(sess);
    proj.subjects.append(subj);
    return proj;
}

//=============================================================================================================
// HELPER: create a BIDS directory tree with sample files
//=============================================================================================================

void TestMnaBids::createBidsTree(const QString &rootDir)
{
    QDir root(rootDir);

    // sub-sample/ses-01/anat/
    root.mkpath("sub-sample/ses-01/anat");
    root.mkpath("sub-sample/ses-01/bem");
    root.mkpath("sub-sample/ses-01/meg");
    root.mkpath("sub-sample/ses-01/source");

    // Write sample files
    auto writeFile = [&](const QString &relPath, const QByteArray &data) {
        QFile f(root.filePath(relPath));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(data);
        f.close();
    };

    writeFile("sub-sample/ses-01/anat/lh.pial", QByteArray(256, '\x11'));
    writeFile("sub-sample/ses-01/anat/lh.aparc.annot", QByteArray(128, '\x22'));
    writeFile("sub-sample/ses-01/bem/sample-bem.fif", QByteArray(512, '\x33'));
    writeFile("sub-sample/ses-01/meg/sample_raw.fif", QByteArray(64, '\x44'));
    writeFile("sub-sample/ses-01/source/result-lh.stc", QByteArray(1024, '\x55'));
}

//=============================================================================================================
// BIDS-structured MNX round-trip
//=============================================================================================================

void TestMnaBids::testCreateBidsStructuredMnx()
{
    MnaProject proj = createTestProject(true);
    const QString path = m_tempDir.filePath("bids_test.mnx");
    QVERIFY(MnaIO::write(proj, path));
    QVERIFY(QFile::exists(path));
    QVERIFY(QFileInfo(path).size() > 0);
}

void TestMnaBids::testExtractMnxToBidsDir()
{
    MnaProject proj = createTestProject(true);
    const QString mnxPath = m_tempDir.filePath("extract_test.mnx");
    QVERIFY(MnaIO::write(proj, mnxPath));

    // Read and extract
    MnaProject loaded = MnaIO::read(mnxPath);
    const QString extractDir = m_tempDir.filePath("extracted");
    QDir().mkpath(extractDir);

    for (const MnaSubject &subj : loaded.subjects) {
        for (const MnaSession &sess : subj.sessions) {
            for (const MnaRecording &rec : sess.recordings) {
                for (const MnaFileRef &ref : rec.files) {
                    if (ref.embedded && !ref.data.isEmpty()) {
                        const QString fullPath = QDir(extractDir).filePath(ref.path);
                        QDir().mkpath(QFileInfo(fullPath).absolutePath());
                        QFile f(fullPath);
                        QVERIFY(f.open(QIODevice::WriteOnly));
                        f.write(ref.data);
                        f.close();
                    }
                }
            }
        }
    }

    // Verify files exist
    QVERIFY(QFile::exists(extractDir + "/sub-sample/ses-01/anat/lh.pial"));
    QVERIFY(QFile::exists(extractDir + "/sub-sample/ses-01/anat/lh.aparc.annot"));
    QVERIFY(QFile::exists(extractDir + "/sub-sample/ses-01/bem/sample-bem.fif"));
    QVERIFY(QFile::exists(extractDir + "/sub-sample/ses-01/meg/sample_raw.fif"));
    QVERIFY(QFile::exists(extractDir + "/sub-sample/ses-01/source/result-lh.stc"));
}

void TestMnaBids::testExtractPreservesDirStructure()
{
    MnaProject proj = createTestProject(true);
    const QString mnxPath = m_tempDir.filePath("dir_structure_test.mnx");
    QVERIFY(MnaIO::write(proj, mnxPath));

    MnaProject loaded = MnaIO::read(mnxPath);
    const QString extractDir = m_tempDir.filePath("dir_structure");
    QDir().mkpath(extractDir);

    for (const auto &subj : loaded.subjects) {
        for (const auto &sess : subj.sessions) {
            for (const auto &rec : sess.recordings) {
                for (const auto &ref : rec.files) {
                    if (ref.embedded) {
                        const QString fullPath = QDir(extractDir).filePath(ref.path);
                        QDir().mkpath(QFileInfo(fullPath).absolutePath());
                        QFile f(fullPath);
                        QVERIFY(f.open(QIODevice::WriteOnly));
                        f.write(ref.data);
                        f.close();
                    }
                }
            }
        }
    }

    // Verify directory structure
    QDir extractRoot(extractDir);
    QVERIFY(extractRoot.exists("sub-sample"));
    QVERIFY(extractRoot.exists("sub-sample/ses-01"));
    QVERIFY(extractRoot.exists("sub-sample/ses-01/anat"));
    QVERIFY(extractRoot.exists("sub-sample/ses-01/bem"));
    QVERIFY(extractRoot.exists("sub-sample/ses-01/meg"));
    QVERIFY(extractRoot.exists("sub-sample/ses-01/source"));
}

void TestMnaBids::testExtractedDataIntegrity()
{
    MnaProject proj = createTestProject(true);
    const QString mnxPath = m_tempDir.filePath("integrity_test.mnx");
    QVERIFY(MnaIO::write(proj, mnxPath));

    MnaProject loaded = MnaIO::read(mnxPath);
    const QString extractDir = m_tempDir.filePath("integrity");
    QDir().mkpath(extractDir);

    for (const auto &subj : loaded.subjects) {
        for (const auto &sess : subj.sessions) {
            for (const auto &rec : sess.recordings) {
                for (const auto &ref : rec.files) {
                    if (ref.embedded) {
                        const QString fullPath = QDir(extractDir).filePath(ref.path);
                        QDir().mkpath(QFileInfo(fullPath).absolutePath());
                        QFile f(fullPath);
                        QVERIFY(f.open(QIODevice::WriteOnly));
                        f.write(ref.data);
                        f.close();
                    }
                }
            }
        }
    }

    // Read extracted files and verify data matches
    QFile surface(extractDir + "/sub-sample/ses-01/anat/lh.pial");
    QVERIFY(surface.open(QIODevice::ReadOnly));
    QCOMPARE(surface.readAll(), QByteArray(256, '\x11'));

    QFile bem(extractDir + "/sub-sample/ses-01/bem/sample-bem.fif");
    QVERIFY(bem.open(QIODevice::ReadOnly));
    QCOMPARE(bem.readAll(), QByteArray(512, '\x33'));

    QFile stc(extractDir + "/sub-sample/ses-01/source/result-lh.stc");
    QVERIFY(stc.open(QIODevice::ReadOnly));
    QCOMPARE(stc.readAll(), QByteArray(1024, '\x55'));
}

//=============================================================================================================
// Pack BIDS → MNX/MNA
//=============================================================================================================

void TestMnaBids::testPackBidsDirToMnx()
{
    const QString bidsDir = m_tempDir.filePath("bids_pack_mnx");
    createBidsTree(bidsDir);

    // Create a sidecar .mna from the test project (non-embedded references)
    MnaProject refProj = createTestProject(false);
    QVERIFY(MnaIO::write(refProj, bidsDir + "/project.mna"));

    // Read sidecar and embed
    MnaProject loaded = MnaIO::read(bidsDir + "/project.mna");
    for (MnaSubject &subj : loaded.subjects) {
        for (MnaSession &sess : subj.sessions) {
            for (MnaRecording &rec : sess.recordings) {
                for (MnaFileRef &ref : rec.files) {
                    if (!ref.embedded) {
                        QString filePath = QDir(bidsDir).filePath(ref.path);
                        QFile f(filePath);
                        if (f.open(QIODevice::ReadOnly)) {
                            ref.data = f.readAll();
                            ref.sizeBytes = ref.data.size();
                            ref.embedded = true;
                            f.close();
                        }
                    }
                }
            }
        }
    }

    const QString mnxPath = m_tempDir.filePath("packed.mnx");
    QVERIFY(MnaIO::write(loaded, mnxPath));
    QVERIFY(QFileInfo(mnxPath).size() > 0);

    // Verify round-trip
    MnaProject verify = MnaIO::read(mnxPath);
    QCOMPARE(verify.subjects.size(), 1);
    int totalFiles = 0;
    for (const auto &s : verify.subjects)
        for (const auto &se : s.sessions)
            for (const auto &r : se.recordings)
                totalFiles += r.files.size();
    QCOMPARE(totalFiles, 5);
}

void TestMnaBids::testPackBidsDirToMna()
{
    const QString bidsDir = m_tempDir.filePath("bids_pack_mna");
    createBidsTree(bidsDir);

    MnaProject refProj = createTestProject(false);
    const QString mnaPath = bidsDir + "/project.mna";
    QVERIFY(MnaIO::write(refProj, mnaPath));

    // Read back and verify it's non-embedded
    MnaProject loaded = MnaIO::read(mnaPath);
    for (const auto &subj : loaded.subjects)
        for (const auto &sess : subj.sessions)
            for (const auto &rec : sess.recordings)
                for (const auto &ref : rec.files)
                    QCOMPARE(ref.embedded, false);
}

void TestMnaBids::testPackWithSidecar()
{
    const QString bidsDir = m_tempDir.filePath("bids_sidecar");
    createBidsTree(bidsDir);

    MnaProject proj = createTestProject(false);
    const QString sidecarPath = bidsDir + "/project.mna";
    QVERIFY(MnaIO::write(proj, sidecarPath));

    // Verify sidecar has same structure
    MnaProject loaded = MnaIO::read(sidecarPath);
    QCOMPARE(loaded.name, "BIDS Test Project");
    QCOMPARE(loaded.subjects[0].id, "sample");
    QCOMPARE(loaded.subjects[0].sessions[0].recordings[0].files.size(), 5);
}

//=============================================================================================================
// Convert .mna ↔ .mnx
//=============================================================================================================

void TestMnaBids::testConvertMnaToMnx()
{
    MnaProject proj = createTestProject(false);
    const QString mnaPath = m_tempDir.filePath("convert_src.mna");
    QVERIFY(MnaIO::write(proj, mnaPath));

    MnaProject loaded = MnaIO::read(mnaPath);
    const QString mnxPath = m_tempDir.filePath("convert_dst.mnx");
    QVERIFY(MnaIO::write(loaded, mnxPath));

    MnaProject verify = MnaIO::read(mnxPath);
    QCOMPARE(verify.name, proj.name);
    QCOMPARE(verify.subjects.size(), 1);
}

void TestMnaBids::testConvertMnxToMna()
{
    MnaProject proj = createTestProject(true);
    const QString mnxPath = m_tempDir.filePath("convert_src.mnx");
    QVERIFY(MnaIO::write(proj, mnxPath));

    MnaProject loaded = MnaIO::read(mnxPath);
    const QString mnaPath = m_tempDir.filePath("convert_dst.mna");
    QVERIFY(MnaIO::write(loaded, mnaPath));

    MnaProject verify = MnaIO::read(mnaPath);
    QCOMPARE(verify.name, proj.name);
    // Embedded data preserved in JSON (base64)
    QCOMPARE(verify.subjects[0].sessions[0].recordings[0].files[0].embedded, true);
}

void TestMnaBids::testConvertMnaToMnxEmbedsData()
{
    // Create .mna with non-embedded refs, then verify converting to .mnx
    // doesn't lose structure
    MnaProject proj = createTestProject(false);
    const QString mnaPath = m_tempDir.filePath("embed_convert.mna");
    QVERIFY(MnaIO::write(proj, mnaPath));

    MnaProject loaded = MnaIO::read(mnaPath);
    QCOMPARE(loaded.subjects[0].sessions[0].recordings[0].files[0].embedded, false);

    // Write as .mnx (non-embedded refs pass through)
    const QString mnxPath = m_tempDir.filePath("embed_convert.mnx");
    QVERIFY(MnaIO::write(loaded, mnxPath));

    MnaProject verify = MnaIO::read(mnxPath);
    QCOMPARE(verify.subjects[0].sessions[0].recordings[0].files[0].embedded, false);
}

//=============================================================================================================
// CLI: mne_show_mna
//=============================================================================================================

void TestMnaBids::testShowMnaHelp()
{
    if (m_showMnaPath.isEmpty())
        QSKIP("mne_show_mna not found");

    QProcess proc;
    proc.start(m_showMnaPath, QStringList() << "--help");
    QVERIFY(proc.waitForFinished(5000));
    const QString output = proc.readAllStandardOutput();
    QVERIFY(output.contains("Inspect MNA"));
    QVERIFY(output.contains("--verbose"));
    QVERIFY(output.contains("--json"));
}

void TestMnaBids::testShowMnaDisplaysProject()
{
    if (m_showMnaPath.isEmpty())
        QSKIP("mne_show_mna not found");

    MnaProject proj = createTestProject(true);
    const QString mnxPath = m_tempDir.filePath("show_test.mnx");
    MnaIO::write(proj, mnxPath);

    QProcess proc;
    proc.start(m_showMnaPath, QStringList() << mnxPath);
    QVERIFY(proc.waitForFinished(5000));

    const QString output = proc.readAllStandardOutput();
    QVERIFY(output.contains("BIDS Test Project"));
    QVERIFY(output.contains("surface"));
    QVERIFY(output.contains("sample"));
    QVERIFY(output.contains("[EMBEDDED]"));
}

void TestMnaBids::testShowMnaVerbose()
{
    if (m_showMnaPath.isEmpty())
        QSKIP("mne_show_mna not found");

    MnaProject proj = createTestProject(true);
    proj.subjects[0].sessions[0].recordings[0].files[0].sha256 = "abcdef1234567890";
    const QString mnxPath = m_tempDir.filePath("show_verbose.mnx");
    MnaIO::write(proj, mnxPath);

    QProcess proc;
    proc.start(m_showMnaPath, QStringList() << "-V" << mnxPath);
    QVERIFY(proc.waitForFinished(5000));

    const QString output = proc.readAllStandardOutput();
    QVERIFY(output.contains("format:"));
    QVERIFY(output.contains("size:"));
}

void TestMnaBids::testShowMnaJsonOutput()
{
    if (m_showMnaPath.isEmpty())
        QSKIP("mne_show_mna not found");

    MnaProject proj = createTestProject(false);
    const QString mnaPath = m_tempDir.filePath("show_json.mna");
    MnaIO::write(proj, mnaPath);

    QProcess proc;
    proc.start(m_showMnaPath, QStringList() << "--json" << mnaPath);
    QVERIFY(proc.waitForFinished(5000));

    const QString output = proc.readAllStandardOutput();
    // Should be valid JSON
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8(), &err);
    QCOMPARE(err.error, QJsonParseError::NoError);
    QVERIFY(doc.isObject());
    QVERIFY(doc.object().contains("name"));
}

void TestMnaBids::testShowMnaInvalidFile()
{
    if (m_showMnaPath.isEmpty())
        QSKIP("mne_show_mna not found");

    QProcess proc;
    proc.start(m_showMnaPath, QStringList() << "/nonexistent/file.mnx");
    QVERIFY(proc.waitForFinished(5000));
    QVERIFY(proc.exitCode() != 0);
}

//=============================================================================================================
// CLI: mne_mna_bids_converter
//=============================================================================================================

void TestMnaBids::testConverterHelp()
{
    if (m_converterPath.isEmpty())
        QSKIP("mne_mna_bids_converter not found");

    QProcess proc;
    proc.start(m_converterPath, QStringList() << "--help");
    QVERIFY(proc.waitForFinished(5000));

    const QString output = proc.readAllStandardOutput();
    QVERIFY(output.contains("extract"));
    QVERIFY(output.contains("pack"));
    QVERIFY(output.contains("convert"));
}

void TestMnaBids::testConverterExtract()
{
    if (m_converterPath.isEmpty())
        QSKIP("mne_mna_bids_converter not found");

    MnaProject proj = createTestProject(true);
    const QString mnxPath = m_tempDir.filePath("cli_extract.mnx");
    MnaIO::write(proj, mnxPath);

    const QString outputDir = m_tempDir.filePath("cli_extracted");

    QProcess proc;
    proc.start(m_converterPath, QStringList() << "extract" << mnxPath << outputDir);
    QVERIFY(proc.waitForFinished(10000));
    QCOMPARE(proc.exitCode(), 0);

    // Verify extracted files
    QVERIFY(QFile::exists(outputDir + "/sub-sample/ses-01/anat/lh.pial"));
    QVERIFY(QFile::exists(outputDir + "/sub-sample/ses-01/bem/sample-bem.fif"));
    QVERIFY(QFile::exists(outputDir + "/sub-sample/ses-01/source/result-lh.stc"));

    // Verify sidecar .mna was written
    QVERIFY(QFile::exists(outputDir + "/project.mna"));

    // Verify data integrity
    QFile extracted(outputDir + "/sub-sample/ses-01/anat/lh.pial");
    QVERIFY(extracted.open(QIODevice::ReadOnly));
    QCOMPARE(extracted.readAll(), QByteArray(256, '\x11'));
}

void TestMnaBids::testConverterPack()
{
    if (m_converterPath.isEmpty())
        QSKIP("mne_mna_bids_converter not found");

    // Create a BIDS tree with sidecar
    const QString bidsDir = m_tempDir.filePath("cli_pack_bids");
    createBidsTree(bidsDir);
    MnaProject refProj = createTestProject(false);
    MnaIO::write(refProj, bidsDir + "/project.mna");

    const QString mnxPath = m_tempDir.filePath("cli_packed.mnx");

    QProcess proc;
    proc.start(m_converterPath, QStringList() << "pack" << bidsDir << mnxPath);
    QVERIFY(proc.waitForFinished(10000));
    QCOMPARE(proc.exitCode(), 0);

    // Verify packed file
    QVERIFY(QFile::exists(mnxPath));
    MnaProject verify = MnaIO::read(mnxPath);
    QCOMPARE(verify.subjects.size(), 1);

    // All files should be embedded (default for .mnx)
    for (const auto &subj : verify.subjects)
        for (const auto &sess : subj.sessions)
            for (const auto &rec : sess.recordings)
                for (const auto &ref : rec.files)
                    QCOMPARE(ref.embedded, true);
}

void TestMnaBids::testConverterConvert()
{
    if (m_converterPath.isEmpty())
        QSKIP("mne_mna_bids_converter not found");

    MnaProject proj = createTestProject(false);
    const QString mnaPath = m_tempDir.filePath("cli_convert_src.mna");
    MnaIO::write(proj, mnaPath);

    const QString mnxPath = m_tempDir.filePath("cli_convert_dst.mnx");

    QProcess proc;
    proc.start(m_converterPath, QStringList() << "convert" << mnaPath << mnxPath);
    QVERIFY(proc.waitForFinished(10000));
    QCOMPARE(proc.exitCode(), 0);

    QVERIFY(QFile::exists(mnxPath));
    MnaProject verify = MnaIO::read(mnxPath);
    QCOMPARE(verify.name, "BIDS Test Project");
}

void TestMnaBids::testConverterInvalidCommand()
{
    if (m_converterPath.isEmpty())
        QSKIP("mne_mna_bids_converter not found");

    QProcess proc;
    proc.start(m_converterPath, QStringList() << "foobar");
    QVERIFY(proc.waitForFinished(5000));
    QVERIFY(proc.exitCode() != 0);
}

//=============================================================================================================
// Edge cases
//=============================================================================================================

void TestMnaBids::testExtractEmptyProject()
{
    MnaProject empty;
    empty.name = "Empty";
    empty.mnaVersion = MnaProject::CURRENT_SCHEMA_VERSION;

    const QString mnxPath = m_tempDir.filePath("empty_extract.mnx");
    QVERIFY(MnaIO::write(empty, mnxPath));

    MnaProject loaded = MnaIO::read(mnxPath);
    QCOMPARE(loaded.subjects.size(), 0);
}

void TestMnaBids::testPackEmptyDirectory()
{
    // Pack an empty directory — should produce a project with no files
    const QString emptyDir = m_tempDir.filePath("empty_bids");
    QDir().mkpath(emptyDir);

    MnaProject proj;
    proj.name = "Empty Pack";
    proj.mnaVersion = MnaProject::CURRENT_SCHEMA_VERSION;

    const QString mnxPath = m_tempDir.filePath("empty_packed.mnx");
    QVERIFY(MnaIO::write(proj, mnxPath));

    MnaProject verify = MnaIO::read(mnxPath);
    QCOMPARE(verify.name, "Empty Pack");
    QCOMPARE(verify.subjects.size(), 0);
}

void TestMnaBids::testMultiSubjectMultiSession()
{
    MnaProject proj;
    proj.name = "Multi-BIDS";
    proj.mnaVersion = MnaProject::CURRENT_SCHEMA_VERSION;
    proj.modified = QDateTime::currentDateTimeUtc();

    const QList<MnaFileRole> roles = {
        MnaFileRole::Surface, MnaFileRole::Annotation, MnaFileRole::Bem,
        MnaFileRole::Digitizer, MnaFileRole::SourceEstimate
    };

    for (int si = 0; si < 3; ++si) {
        MnaSubject subj;
        subj.id = QString("sub%1").arg(si + 1);

        for (int sei = 0; sei < 2; ++sei) {
            MnaSession sess;
            sess.id = QString("ses%1").arg(sei + 1);

            MnaRecording rec;
            rec.id = "rec01";

            for (int fi = 0; fi < roles.size(); ++fi) {
                MnaFileRef ref;
                ref.role = roles[fi];
                ref.path = QString("sub-%1/ses-%2/%3/file_%4")
                    .arg(subj.id, sess.id, mnaFileRoleToString(ref.role), QString::number(fi));
                ref.embedded = true;
                ref.data = QByteArray(32 * (fi + 1), static_cast<char>(si * 20 + sei * 10 + fi));
                ref.sizeBytes = ref.data.size();
                rec.files.append(ref);
            }

            sess.recordings.append(rec);
            subj.sessions.append(sess);
        }
        proj.subjects.append(subj);
    }

    // MNX round-trip
    const QString mnxPath = m_tempDir.filePath("multi_bids.mnx");
    QVERIFY(MnaIO::write(proj, mnxPath));

    MnaProject verify = MnaIO::read(mnxPath);
    QCOMPARE(verify.subjects.size(), 3);

    int totalFiles = 0;
    for (const auto &s : verify.subjects)
        for (const auto &se : s.sessions)
            for (const auto &r : se.recordings)
                totalFiles += r.files.size();

    QCOMPARE(totalFiles, 3 * 2 * 5); // 3 subjects × 2 sessions × 5 files

    // Verify data integrity of first and last file
    const auto &firstFile = verify.subjects[0].sessions[0].recordings[0].files[0];
    QCOMPARE(firstFile.data.size(), 32);
    QCOMPARE(firstFile.data[0], '\x00');

    const auto &lastFile = verify.subjects[2].sessions[1].recordings[0].files[4];
    QCOMPARE(lastFile.data.size(), 160);
    QCOMPARE(lastFile.data[0], static_cast<char>(2 * 20 + 1 * 10 + 4));
}

//=============================================================================================================
// TEST MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMnaBids)
#include "test_mna_bids.moc"
