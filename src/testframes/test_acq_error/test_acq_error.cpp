//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_acq_error.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests for the mne_acq error catalogue and AcqExpected.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <acq/acq_error.h>
#include <acq/acq_global.h>
#include <acq/acq_sample_format.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstring>
#include <memory>
#include <set>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================

class TestAcqError : public QObject
{
    Q_OBJECT

private slots:
    void testCatalogueIsComplete();
    void testUnknownCodeMapsToInternal();
    void testContextAndText();
    void testExpectedValueAndError();
    void testExpectedMoveOnly();
    void testSampleFormatNamesAndBuildInfo();
};

//=============================================================================================================

void TestAcqError::testCatalogueIsComplete()
{
    std::set<std::string_view> names;
    std::size_t entries = 0;
    for (int v = 0; v <= 0xFFFF; ++v) {
        const auto code = static_cast<AcqErrorCode>(v);
        const AcqErrorInfo& info = acqErrorInfo(code);
        if (info.code != code) {
            continue;
        }
        ++entries;
        QVERIFY2(!info.name.empty(), qPrintable(QString("code %1 has no name").arg(v)));
        QVERIFY2(!info.message.empty(), info.name.data());
        QVERIFY2(code == AcqErrorCode::None || !info.fix.empty(), info.name.data());
        QVERIFY2(!info.docAnchor.empty(), info.name.data());
        QVERIFY2(names.insert(info.name).second, info.name.data());
    }
    // None, 8 data-path codes and 20 control-path codes; update when codes are added.
    QCOMPARE(entries, std::size_t(29));
}

//=============================================================================================================

void TestAcqError::testUnknownCodeMapsToInternal()
{
    const AcqErrorInfo& info = acqErrorInfo(static_cast<AcqErrorCode>(4242));
    QCOMPARE(info.code, AcqErrorCode::Internal);
}

//=============================================================================================================

void TestAcqError::testContextAndText()
{
    const AcqError plain(AcqErrorCode::Timeout);
    QVERIFY(!plain.ok());
    QVERIFY(plain.context().empty());
    QVERIFY(plain == AcqErrorCode::Timeout);

    const AcqError withContext(AcqErrorCode::UnknownScheme, "foo://bar");
    QVERIFY(withContext.context() == "foo://bar");
    const std::string text = withContext.text();
    QVERIFY(text.find("foo://bar") != std::string::npos);
    QVERIFY(text.find(std::string(withContext.info().message)) != std::string::npos);
    QVERIFY(text.find(std::string(withContext.info().fix)) != std::string::npos);

    AcqError copy = withContext;
    QVERIFY(copy.context() == withContext.context());
    copy = AcqError();
    QVERIFY(copy.ok());
    QVERIFY(withContext.context() == "foo://bar");
}

//=============================================================================================================

void TestAcqError::testExpectedValueAndError()
{
    AcqExpected<int> value(42);
    QVERIFY(value.hasValue());
    QCOMPARE(*value, 42);
    QVERIFY(value.error().ok());

    AcqExpected<int> error(AcqErrorCode::Overrun);
    QVERIFY(!error);
    QVERIFY(error.error() == AcqErrorCode::Overrun);

    AcqExpected<int> copy = value;
    copy = error;
    QVERIFY(!copy.hasValue());
    copy = value;
    QCOMPARE(*copy, 42);

    AcqExpected<std::string> text(std::string("abc"));
    QCOMPARE(text->size(), std::size_t(3));
}

//=============================================================================================================

void TestAcqError::testExpectedMoveOnly()
{
    AcqExpected<std::unique_ptr<int>> owned(std::make_unique<int>(7));
    AcqExpected<std::unique_ptr<int>> moved(std::move(owned));
    QVERIFY(moved.hasValue());
    QCOMPARE(**moved, 7);
    std::unique_ptr<int> out = std::move(moved).value();
    QCOMPARE(*out, 7);

    // A move-only value is taken straight out of a temporary result, as in *stream.reader(mode).
    auto make = [] {
        return AcqExpected<std::unique_ptr<int>>(std::make_unique<int>(9));
    };
    std::unique_ptr<int> direct = *make();
    QCOMPARE(*direct, 9);

    // A named value is copied in once, not twice.
    const std::string name = "abc";
    AcqExpected<std::string> copied(name);
    QCOMPARE(*copied, name);
}

//=============================================================================================================

void TestAcqError::testSampleFormatNamesAndBuildInfo()
{
    QVERIFY(sampleFormatName(AcqSampleFormat::Int16) == "int16");
    QVERIFY(sampleFormatName(AcqSampleFormat::Int24Packed) == "int24");
    QVERIFY(sampleFormatName(AcqSampleFormat::Int32) == "int32");
    QVERIFY(sampleFormatName(AcqSampleFormat::Float32) == "float32");
    QVERIFY(sampleFormatName(AcqSampleFormat::Float64) == "float64");
    QVERIFY(sampleFormatName(AcqSampleFormat::ComplexInt16) == "cint16");
    QVERIFY(sampleFormatName(AcqSampleFormat::ComplexFloat32) == "cfloat32");
    QVERIFY(sampleFormatName(static_cast<AcqSampleFormat>(0)) == "unknown");
    QCOMPARE(bytesPerSample(static_cast<AcqSampleFormat>(0)), std::size_t(0));
    QVERIFY(isComplex(AcqSampleFormat::ComplexInt16) && !isComplex(AcqSampleFormat::Float64));
    AcqInt24 negative{{0xFF, 0xFF, 0xFF}};
    QCOMPARE(negative.value(), -1);
    QVERIFY(std::strlen(acq_build_date_time()) > 0 && std::strlen(acq_build_hash()) > 0 && std::strlen(acq_build_hash_long()) > 0);
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestAcqError)
#include "test_acq_error.moc"
