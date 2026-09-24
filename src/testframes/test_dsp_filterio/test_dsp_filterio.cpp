//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_dsp_filterio.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.10
 * @date     March, 2026
 * @brief    Unit tests for FilterIO.
 */

#include <QtTest/QtTest>

#include <dsp/filterio.h>

using namespace UTILSLIB;

class TestDspFilterIo : public QObject
{
    Q_OBJECT

private slots:
    void writeAndReadRoundTrip()
    {
        FilterKernel original("RoundTrip Filter",
                              2,
                              33,
                              25.0,
                              10.0,
                              5.0,
                              1000.0,
                              1);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = dir.path() + "/roundtrip_filter.txt";
        QVERIFY(FilterIO::writeFilter(path, original));

        FilterKernel restored;
        QVERIFY(FilterIO::readFilter(path, restored));

        QCOMPARE(restored.getName(), QString(original.getName()).remove(' '));
        QCOMPARE(restored.getFilterOrder(), original.getFilterOrder());
        QCOMPARE(restored.getDesignMethod().getName(), original.getDesignMethod().getName());
        QCOMPARE(restored.getFilterType().getName(), original.getFilterType().getName());
        QCOMPARE(restored.getSamplingFrequency(), original.getSamplingFrequency());
        QCOMPARE(restored.getCoefficients().size(), original.getCoefficients().size());

        for(int i = 0; i < restored.getCoefficients().size(); ++i) {
            QVERIFY(qAbs(restored.getCoefficients()(i) - original.getCoefficients()(i)) < 1e-9);
        }
    }

    void readRejectsNonTxtPath()
    {
        FilterKernel filter;
        QVERIFY(!FilterIO::readFilter("not_a_filter.csv", filter));
    }

    void writeRejectsEmptyPath()
    {
        FilterKernel filter("EmptyPath", 0, 17, 40.0, 0.0, 5.0, 1000.0, 0);
        QVERIFY(!FilterIO::writeFilter(QString(), filter));
    }

    void readAdjustsOrderToCoefficientCount()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString path = dir.path() + "/manual_filter.txt";
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));

        QTextStream out(&file);
        out << "#sFreq 512\n";
        out << "#name Manual Filter\n";
        out << "#type LPF\n";
        out << "#order 99\n";
        out << "#HPFreq 0\n";
        out << "#LPFreq 40\n";
        out << "#CenterFreq 20\n";
        out << "#DesignMethod Cosine\n";
        out << "0.25\n";
        out << "0.50\n";
        out << "0.25\n";
        file.close();

        FilterKernel filter;
        QVERIFY(FilterIO::readFilter(path, filter));

        QCOMPARE(filter.getSamplingFrequency(), 512.0);
        QCOMPARE(filter.getName(), QString("ManualFilter"));
        QCOMPARE(filter.getFilterOrder(), 3);
        QCOMPARE(filter.getCoefficients().size(), 3);
        QVERIFY(qAbs(filter.getCoefficients()(0) - 0.25) < 1e-12);
        QVERIFY(qAbs(filter.getCoefficients()(1) - 0.50) < 1e-12);
        QVERIFY(qAbs(filter.getCoefficients()(2) - 0.25) < 1e-12);
    }
};

QTEST_GUILESS_MAIN(TestDspFilterIo)
#include "test_dsp_filterio.moc"
