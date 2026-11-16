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

#include <base/byte_stream.h>
#include <dsp/filterio.h>

#include <filesystem>
#include <string>

using namespace mne;

class TestDspFilterIo : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        m_path = m_dir.filePath(QStringLiteral("filter.txt")).toStdString();
    }

    void writeAndReadRoundTrip()
    {
        const FilterKernel original("RoundTrip Filter", 2, 33, 25.0, 10.0, 5.0, 1000.0, 1);
        QVERIFY(FilterIO::write_filter(m_path, original));

        const auto restored = FilterIO::read_filter(m_path);
        QVERIFY(restored);
        QCOMPARE(restored->name(), original.name());
        QCOMPARE(restored->filter_order(), original.filter_order());
        QCOMPARE(restored->design_method().name(), original.design_method().name());
        QCOMPARE(restored->filter_type().name(), original.filter_type().name());
        QCOMPARE(restored->sampling_frequency(), original.sampling_frequency());
        QCOMPARE(restored->parks_width(), original.parks_width());
        QCOMPARE(restored->highpass_freq(), original.highpass_freq());
        QCOMPARE(restored->lowpass_freq(), original.lowpass_freq());
        QCOMPARE(restored->center_frequency(), original.center_frequency());
        QVERIFY(restored->coefficients() == original.coefficients());
    }

    void readRejectsMissingFileAndBadValues()
    {
        QVERIFY(!FilterIO::read_filter("/nonexistent/filter.txt"));
        QVERIFY(!FilterIO::write_filter("/nonexistent/dir/filter.txt", FilterKernel()));
        QVERIFY(write_text_file(m_path, "#sFreq 512\n0.25\nnot a number\n"));
        QVERIFY(!FilterIO::read_filter(m_path));
        QVERIFY(write_text_file(m_path, "#sFreq fast\n0.25\n"));
        QVERIFY(!FilterIO::read_filter(m_path));
    }

    void readAdjustsOrderToCoefficientCount()
    {
        QVERIFY(write_text_file(m_path, "#sFreq 512\n"
                                        "#name Manual type filter\n"
                                        "#type LPF\n"
                                        "#order 99\n"
                                        "#HPFreq 0\n"
                                        "#LPFreq 40\n"
                                        "#CenterFreq 20\n"
                                        "#DesignMethod Cosine\n"
                                        "0.25\n"
                                        "0.50\n"
                                        "0.25\n"));
        const auto filter = FilterIO::read_filter(m_path);
        QVERIFY(filter);
        QCOMPARE(filter->sampling_frequency(), 512.0);
        QCOMPARE(filter->name(), std::string("Manual type filter"));
        QCOMPARE(filter->filter_type().name(), std::string("LPF"));
        QCOMPARE(filter->lowpass_freq(), 40.0);
        QCOMPARE(filter->filter_order(), 3);
        QVERIFY(filter->coefficients() == Eigen::RowVector3d(0.25, 0.5, 0.25));
    }

private:
    QTemporaryDir m_dir;
    std::filesystem::path m_path;
};

QTEST_GUILESS_MAIN(TestDspFilterIo)
#include "test_dsp_filterio.moc"
