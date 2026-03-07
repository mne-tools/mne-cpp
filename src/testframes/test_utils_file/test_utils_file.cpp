//=============================================================================================================
/**
 * @file     test_utils_file.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Tests for File utility class — full coverage of exists/create/copy/rename/remove.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/file.h>
#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QObject>
#include <QTemporaryDir>
#include <QDebug>
#include <QtTest>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <fstream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//=============================================================================================================
/**
 * @brief Tests for UTILSLIB::File — covers all static methods with round-trip IO patterns
 *        inspired by mne-python's file-based test strategies.
 */
class TestUtilsFile : public QObject
{
    Q_OBJECT

public:
    TestUtilsFile();

private slots:
    void initTestCase();

    // exists() overloads
    void testExistsCharPtr();
    void testExistsStdString();
    void testExistsQString();
    void testExistsNonExistent();

    // create() overloads
    void testCreateCharPtr();
    void testCreateStdString();
    void testCreateQString();
    void testCreateAlreadyExists();

    // copy() overloads
    void testCopyCharPtr();
    void testCopyStdString();
    void testCopyQString();
    void testCopyNonExistentSource();
    void testCopyDestAlreadyExists();

    // rename() overloads
    void testRenameCharPtr();
    void testRenameStdString();
    void testRenameQString();
    void testRenameNonExistentSource();
    void testRenameDestAlreadyExists();

    // remove() overloads
    void testRemoveCharPtr();
    void testRemoveStdString();
    void testRemoveQString();
    void testRemoveNonExistent();

    // Round-trip: create -> copy -> rename -> remove
    void testFullRoundTrip();

    void cleanupTestCase();

private:
    QTemporaryDir m_tempDir;
    std::string m_basePath;
};

//=============================================================================================================

TestUtilsFile::TestUtilsFile()
{
}

//=============================================================================================================

void TestUtilsFile::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    QVERIFY(m_tempDir.isValid());
    m_basePath = m_tempDir.path().toStdString() + "/";
}

//=============================================================================================================

void TestUtilsFile::testExistsCharPtr()
{
    std::string path = m_basePath + "exists_test_c.txt";
    std::ofstream ofs(path);
    ofs << "content";
    ofs.close();

    QVERIFY(File::exists(path.c_str()));
}

//=============================================================================================================

void TestUtilsFile::testExistsStdString()
{
    std::string path = m_basePath + "exists_test_std.txt";
    std::ofstream ofs(path);
    ofs << "content";
    ofs.close();

    QVERIFY(File::exists(path));
}

//=============================================================================================================

void TestUtilsFile::testExistsQString()
{
    std::string path = m_basePath + "exists_test_qt.txt";
    std::ofstream ofs(path);
    ofs << "content";
    ofs.close();

    QVERIFY(File::exists(QString::fromStdString(path)));
}

//=============================================================================================================

void TestUtilsFile::testExistsNonExistent()
{
    QVERIFY(!File::exists((m_basePath + "does_not_exist.txt").c_str()));
    QVERIFY(!File::exists(m_basePath + "does_not_exist.txt"));
    QVERIFY(!File::exists(QString::fromStdString(m_basePath + "does_not_exist.txt")));
}

//=============================================================================================================

void TestUtilsFile::testCreateCharPtr()
{
    std::string path = m_basePath + "create_c.txt";
    QVERIFY(!File::exists(path.c_str()));
    QVERIFY(File::create(path.c_str()));
    QVERIFY(File::exists(path.c_str()));
    File::remove(path.c_str());
}

//=============================================================================================================

void TestUtilsFile::testCreateStdString()
{
    std::string path = m_basePath + "create_std.txt";
    QVERIFY(!File::exists(path));
    QVERIFY(File::create(path));
    QVERIFY(File::exists(path));
    File::remove(path);
}

//=============================================================================================================

void TestUtilsFile::testCreateQString()
{
    QString path = QString::fromStdString(m_basePath + "create_qt.txt");
    QVERIFY(!File::exists(path));
    QVERIFY(File::create(path));
    QVERIFY(File::exists(path));
    File::remove(path);
}

//=============================================================================================================

void TestUtilsFile::testCreateAlreadyExists()
{
    std::string path = m_basePath + "create_dup.txt";
    QVERIFY(File::create(path));
    // Creating again should fail
    QVERIFY(!File::create(path));
    File::remove(path);
}

//=============================================================================================================

void TestUtilsFile::testCopyCharPtr()
{
    std::string src = m_basePath + "copy_src_c.txt";
    std::string dst = m_basePath + "copy_dst_c.txt";

    {
        std::ofstream ofs(src);
        ofs << "test content for copy";
    }

    QVERIFY(File::copy(src.c_str(), dst.c_str()));
    QVERIFY(File::exists(dst.c_str()));
    QVERIFY(File::exists(src.c_str()));  // source still exists

    File::remove(src.c_str());
    File::remove(dst.c_str());
}

//=============================================================================================================

void TestUtilsFile::testCopyStdString()
{
    std::string src = m_basePath + "copy_src_std.txt";
    std::string dst = m_basePath + "copy_dst_std.txt";

    {
        std::ofstream ofs(src);
        ofs << "test content";
    }

    QVERIFY(File::copy(src, dst));
    QVERIFY(File::exists(dst));

    File::remove(src);
    File::remove(dst);
}

//=============================================================================================================

void TestUtilsFile::testCopyQString()
{
    QString src = QString::fromStdString(m_basePath + "copy_src_qt.txt");
    QString dst = QString::fromStdString(m_basePath + "copy_dst_qt.txt");

    {
        std::ofstream ofs(src.toStdString());
        ofs << "test content";
    }

    QVERIFY(File::copy(src, dst));
    QVERIFY(File::exists(dst));

    File::remove(src);
    File::remove(dst);
}

//=============================================================================================================

void TestUtilsFile::testCopyNonExistentSource()
{
    std::string src = m_basePath + "nonexistent_src.txt";
    std::string dst = m_basePath + "copy_fail_dst.txt";

    QVERIFY(!File::copy(src, dst));
    QVERIFY(!File::exists(dst));
}

//=============================================================================================================

void TestUtilsFile::testCopyDestAlreadyExists()
{
    std::string src = m_basePath + "copy_src_exists.txt";
    std::string dst = m_basePath + "copy_dst_exists.txt";

    File::create(src);
    File::create(dst);

    // Should fail because dest already exists
    QVERIFY(!File::copy(src, dst));

    File::remove(src);
    File::remove(dst);
}

//=============================================================================================================

void TestUtilsFile::testRenameCharPtr()
{
    std::string src = m_basePath + "rename_src_c.txt";
    std::string dst = m_basePath + "rename_dst_c.txt";

    File::create(src);
    QVERIFY(File::rename(src.c_str(), dst.c_str()));
    QVERIFY(!File::exists(src.c_str()));
    QVERIFY(File::exists(dst.c_str()));

    File::remove(dst.c_str());
}

//=============================================================================================================

void TestUtilsFile::testRenameStdString()
{
    std::string src = m_basePath + "rename_src_std.txt";
    std::string dst = m_basePath + "rename_dst_std.txt";

    File::create(src);
    QVERIFY(File::rename(src, dst));
    QVERIFY(!File::exists(src));
    QVERIFY(File::exists(dst));

    File::remove(dst);
}

//=============================================================================================================

void TestUtilsFile::testRenameQString()
{
    QString src = QString::fromStdString(m_basePath + "rename_src_qt.txt");
    QString dst = QString::fromStdString(m_basePath + "rename_dst_qt.txt");

    File::create(src);
    QVERIFY(File::rename(src, dst));
    QVERIFY(!File::exists(src));
    QVERIFY(File::exists(dst));

    File::remove(dst);
}

//=============================================================================================================

void TestUtilsFile::testRenameNonExistentSource()
{
    std::string src = m_basePath + "rename_nonexistent.txt";
    std::string dst = m_basePath + "rename_dst_fail.txt";

    QVERIFY(!File::rename(src, dst));
}

//=============================================================================================================

void TestUtilsFile::testRenameDestAlreadyExists()
{
    std::string src = m_basePath + "rename_src_dup.txt";
    std::string dst = m_basePath + "rename_dst_dup.txt";

    File::create(src);
    File::create(dst);

    QVERIFY(!File::rename(src, dst));

    File::remove(src);
    File::remove(dst);
}

//=============================================================================================================

void TestUtilsFile::testRemoveCharPtr()
{
    std::string path = m_basePath + "remove_c.txt";
    File::create(path);
    QVERIFY(File::remove(path.c_str()));
    QVERIFY(!File::exists(path.c_str()));
}

//=============================================================================================================

void TestUtilsFile::testRemoveStdString()
{
    std::string path = m_basePath + "remove_std.txt";
    File::create(path);
    QVERIFY(File::remove(path));
    QVERIFY(!File::exists(path));
}

//=============================================================================================================

void TestUtilsFile::testRemoveQString()
{
    QString path = QString::fromStdString(m_basePath + "remove_qt.txt");
    File::create(path);
    QVERIFY(File::remove(path));
    QVERIFY(!File::exists(path));
}

//=============================================================================================================

void TestUtilsFile::testRemoveNonExistent()
{
    QVERIFY(!File::remove((m_basePath + "remove_nonexistent.txt").c_str()));
}

//=============================================================================================================

void TestUtilsFile::testFullRoundTrip()
{
    // Inspired by mne-python's IO round-trip pattern: create -> write -> copy -> rename -> verify -> remove
    std::string original = m_basePath + "roundtrip_original.txt";
    std::string copied = m_basePath + "roundtrip_copied.txt";
    std::string renamed = m_basePath + "roundtrip_renamed.txt";

    // Create a file with some content (File::copy uses rdbuf which needs non-empty source)
    {
        std::ofstream ofs(original);
        ofs << "hello round trip";
    }
    QVERIFY(File::exists(original));

    // Copy
    QVERIFY(File::copy(original, copied));
    QVERIFY(File::exists(original));
    QVERIFY(File::exists(copied));

    // Rename the copy
    QVERIFY(File::rename(copied, renamed));
    QVERIFY(!File::exists(copied));
    QVERIFY(File::exists(renamed));

    // Remove both
    QVERIFY(File::remove(original));
    QVERIFY(File::remove(renamed));

    QVERIFY(!File::exists(original));
    QVERIFY(!File::exists(renamed));
}

//=============================================================================================================

void TestUtilsFile::cleanupTestCase()
{
    qInfo() << "TestUtilsFile: All tests completed";
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestUtilsFile)
#include "test_utils_file.moc"
