//=============================================================================================================
/**
 * @file     test_report.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for HTML Report generation.
 */

#include <utils/report.h>

#include <QtTest>
#include <QObject>
#include <QTemporaryFile>

using namespace UTILSLIB;

class TestReport : public QObject
{
    Q_OBJECT

private slots:
    void testConstructor()
    {
        Report report("Test Report");
        QCOMPARE(report.title(), QString("Test Report"));
        QCOMPARE(report.sectionCount(), 0);
    }

    void testDefaultTitle()
    {
        Report report;
        QVERIFY(report.title().contains("MNE-CPP"));
    }

    void testAddText()
    {
        Report report("Test");
        report.addText("Info", "Some text content");
        QCOMPARE(report.sectionCount(), 1);

        QString html = report.toHtml();
        QVERIFY(html.contains("Some text content"));
        QVERIFY(html.contains("Info"));
    }

    void testAddTable()
    {
        Report report("Test");
        QStringList headers = {"Name", "Value"};
        QList<QStringList> rows;
        rows.append(QStringList{"Lambda", "0.1"});
        rows.append(QStringList{"SNR", "3.0"});

        report.addTable("Parameters", headers, rows);
        QCOMPARE(report.sectionCount(), 1);

        QString html = report.toHtml();
        QVERIFY(html.contains("<table>"));
        QVERIFY(html.contains("Lambda"));
        QVERIFY(html.contains("3.0"));
    }

    void testAddKeyValue()
    {
        Report report("Test");
        QList<QPair<QString, QString>> pairs;
        pairs.append(QPair<QString, QString>("Subject", "sub-01"));
        pairs.append(QPair<QString, QString>("Channels", "306"));

        report.addKeyValue("Data Info", pairs);

        QString html = report.toHtml();
        QVERIFY(html.contains("<dl>"));
        QVERIFY(html.contains("sub-01"));
        QVERIFY(html.contains("306"));
    }

    void testAddCode()
    {
        Report report("Test");
        report.addCode("Script", "auto x = computeInverse();");

        QString html = report.toHtml();
        QVERIFY(html.contains("<pre>"));
        QVERIFY(html.contains("<code>"));
        QVERIFY(html.contains("computeInverse"));
    }

    void testHtmlStructure()
    {
        Report report("My Report");
        report.addText("Section 1", "Content 1");

        QString html = report.toHtml();
        QVERIFY(html.contains("<!DOCTYPE html>"));
        QVERIFY(html.contains("<html"));
        QVERIFY(html.contains("</html>"));
        QVERIFY(html.contains("<title>My Report</title>"));
        QVERIFY(html.contains("MNE-CPP"));  // footer
    }

    void testSaveToFile()
    {
        Report report("Save Test");
        report.addText("Section", "Test content");

        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        QVERIFY(tmp.open());
        QString path = tmp.fileName();
        tmp.close();

        QVERIFY(report.save(path));

        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QByteArray content = file.readAll();
        QVERIFY(content.contains("<!DOCTYPE html>"));
        QVERIFY(content.contains("Save Test"));
    }

    void testSaveFailsOnBadPath()
    {
        Report report("Test");
        QVERIFY(!report.save("/nonexistent/dir/report.html"));
    }

    void testHtmlEscaping()
    {
        Report report("Test");
        report.addText("XSS Test", "<script>alert('xss')</script>");

        QString html = report.toHtml();
        QVERIFY(!html.contains("<script>"));
        QVERIFY(html.contains("&lt;script&gt;"));
    }

    void testMultipleSections()
    {
        Report report("Multi");
        report.addText("A", "First");
        report.addText("B", "Second");
        report.addCode("C", "Third");

        QCOMPARE(report.sectionCount(), 3);

        QString html = report.toHtml();
        // Sections should appear in order
        int posA = html.indexOf("First");
        int posB = html.indexOf("Second");
        int posC = html.indexOf("Third");
        QVERIFY(posA < posB);
        QVERIFY(posB < posC);
    }
};

QTEST_GUILESS_MAIN(TestReport)
#include "test_report.moc"
