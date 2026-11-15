//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_base_cbor.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Tests the CBOR classes against Qt's: encoded bytes, decoding, JSON conversion and malformed input.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/cbor.h>
#include <base/json.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCborArray>
#include <QCborMap>
#include <QCborValue>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

CborByteArray bytes(const QByteArray& data)
{
    const auto* first = reinterpret_cast<const std::byte*>(data.constData());
    return CborByteArray(first, first + data.size());
}

QByteArray qbytes(const CborByteArray& data)
{
    return QByteArray(reinterpret_cast<const char*>(data.data()), qsizetype(data.size()));
}

/** The same tree built with Qt and with mne_base. */
struct Pair
{
    QCborValue qt;
    CborValue ours;
};

std::vector<Pair> samples()
{
    const QByteArray blob("\x00\x01\xfe\xff", 4);
    QCborMap qMap;
    qMap[QStringLiteral("zeta")] = 1;
    qMap[QStringLiteral("alpha")] = QStringLiteral("\u00b5V");
    qMap[QStringLiteral("nested")] = QCborArray{QCborValue(true), QCborValue(nullptr), QCborValue(2.5)};
    CborMap map;
    map["zeta"] = 1;
    map["alpha"] = "\xC2\xB5V";
    map["nested"] = CborArray{true, nullptr, 2.5};

    std::vector<Pair> pairs{
        {QCborValue(0), 0},
        {QCborValue(23), 23},
        {QCborValue(24), 24},
        {QCborValue(255), 255},
        {QCborValue(256), 256},
        {QCborValue(65535), 65535},
        {QCborValue(65536), 65536},
        {QCborValue(qint64(4294967296LL)), std::int64_t{4294967296LL}},
        {QCborValue(-1), -1},
        {QCborValue(-25), -25},
        {QCborValue(std::numeric_limits<qint64>::min()), std::numeric_limits<std::int64_t>::min()},
        {QCborValue(std::numeric_limits<qint64>::max()), std::numeric_limits<std::int64_t>::max()},
        {QCborValue(0.5), 0.5},
        {QCborValue(-1e300), -1e300},
        {QCborValue(true), true},
        {QCborValue(false), false},
        {QCborValue(nullptr), nullptr},
        {QCborValue(), CborValue()},
        {QCborValue(QString()), ""},
        {QCborValue(QStringLiteral("text with \u65e5\u672c")), "text with \xE6\x97\xA5\xE6\x9C\xAC"},
        {QCborValue(blob), bytes(blob)},
        {QCborValue(QByteArray()), CborByteArray{}},
        {QCborValue(QCborTag(0), QStringLiteral("2026-01-01T12:00:00.000Z")), CborValue::tagged(0, "2026-01-01T12:00:00.000Z")},
        {QCborValue(QCborTag(22), blob), CborValue::tagged(22, bytes(blob))},
        {QCborValue(QCborTag(23), blob), CborValue::tagged(23, bytes(blob))},
        {QCborValue(qMap), map},
        {QCborValue(QCborArray{QCborValue(qMap), QCborValue(blob), QCborArray{}}), CborArray{map, bytes(blob), CborArray{}}},
    };
    return pairs;
}

} // namespace

//=============================================================================================================
/**
 * @brief Tests mne::CborValue, CborArray and CborMap against QCborValue, QCborArray and QCborMap.
 */
class TestBaseCbor : public QObject
{
    Q_OBJECT

private slots:
    void encodingMatchesQt();
    void decodingRoundTripsQtBytes();
    void decodingAcceptsEveryEncodingForm();
    void jsonConversionMatchesQt();
    void mapsKeepInsertionOrderAndLookUpKeys();
    void stringListsAndKeyFiltersMatchQt();
    void malformedInputIsRejected();
};

//=============================================================================================================

void TestBaseCbor::encodingMatchesQt()
{
    for (const auto& [qt, ours] : samples()) {
        QCOMPARE(qbytes(ours.to_cbor()).toHex(), qt.toCbor().toHex());
    }
}

//=============================================================================================================

void TestBaseCbor::decodingRoundTripsQtBytes()
{
    for (const auto& [qt, ours] : samples()) {
        CborParseError error;
        const CborValue decoded = CborValue::from_cbor(bytes(qt.toCbor()), &error);
        QCOMPARE(error.error, CborParseError::NoError);
        QVERIFY2(decoded == ours, qt.toCbor().toHex().constData());
        QCOMPARE(qbytes(decoded.to_cbor()), qt.toCbor());
    }
}

//=============================================================================================================

void TestBaseCbor::decodingAcceptsEveryEncodingForm()
{
    // Indefinite-length strings, arrays and maps, long integer heads and 16- and 32-bit floats, as RFC 8949
    // appendix A; each must decode to the same value as Qt decodes it to
    const QList<QByteArray> inputs{
        QByteArray::fromHex("7f657374726561646d696e67ff"), // (_ "strea", "ming")
        QByteArray::fromHex("5f42010243030405ff"),         // (_ h'0102', h'030405')
        QByteArray::fromHex("9f018202039f0405ffff"),       // [_ 1, [2, 3], [_ 4, 5]]
        QByteArray::fromHex("bf61610161629f0203ffff"),     // {_ "a": 1, "b": [_ 2, 3]}
        QByteArray::fromHex("1b000000e8d4a51000"),         // 1000000000000
        QByteArray::fromHex("f93c00"),                     // 1.0 as half float
        QByteArray::fromHex("f97bff"),                     // 65504.0
        QByteArray::fromHex("f90001"),                     // smallest half subnormal
        QByteArray::fromHex("f97c00"),                     // infinity
        QByteArray::fromHex("fa47c35000"),                 // 100000.0 as single float
        QByteArray::fromHex("f7"),                         // undefined
    };
    for (const QByteArray& input : inputs) {
        CborParseError error;
        const CborValue decoded = CborValue::from_cbor(bytes(input), &error);
        QCOMPARE(error.error, CborParseError::NoError);
        const QCborValue qt = QCborValue::fromCbor(input);
        QCOMPARE(qbytes(decoded.to_cbor()).toHex(), qt.toCbor().toHex());
    }
    // Integer keys become their decimal text, as QCborMap::toJsonObject() writes them
    QCOMPARE(CborValue::from_cbor(bytes(QByteArray::fromHex("a201020304"))).to_map().keys(), (std::vector<std::string>{"1", "3"}));
    // Integers beyond std::int64_t become doubles; Qt wraps -2^64 to 0
    QCOMPARE(CborValue::from_cbor(bytes(QByteArray::fromHex("3bffffffffffffffff"))).to_double(), -18446744073709551616.0);
    QCOMPARE(CborValue::from_cbor(bytes(QByteArray::fromHex("1bffffffffffffffff"))).to_double(), 18446744073709551615.0);
}

//=============================================================================================================

void TestBaseCbor::jsonConversionMatchesQt()
{
    for (const auto& [qt, ours] : samples()) {
        const QByteArray expected = QJsonDocument(QJsonObject{{"v", qt.toJsonValue()}}).toJson(QJsonDocument::Compact);
        const std::string actual = JsonDocument(JsonObject{{"v", ours.to_json_value()}}).to_json(JsonDocument::Compact);
        QCOMPARE(QByteArray::fromStdString(actual), expected);
    }

    const QByteArray json = R"({"b":[1,2.5,"x",null,true],"a":{"n":-7,"big":9007199254740993}})";
    const QCborValue qt = QCborValue::fromJsonValue(QJsonDocument::fromJson(json).object());
    const CborValue ours = CborValue::from_json_value(JsonDocument::from_json(json.toStdString()).object());
    QCOMPARE(qbytes(ours.to_cbor()).toHex(), qt.toCbor().toHex());
    QCOMPARE(ours.to_map().to_json_object(), JsonDocument::from_json(json.toStdString()).object());
}

//=============================================================================================================

void TestBaseCbor::mapsKeepInsertionOrderAndLookUpKeys()
{
    CborMap map{{"b", 1}, {"a", "two"}};
    map.insert("b", 3);
    map["c"] = CborByteArray{std::byte{7}};
    QCOMPARE(map.keys(), (std::vector<std::string>{"b", "a", "c"}));
    QCOMPARE(map.value("b").to_integer(), std::int64_t{3});
    QVERIFY(map.value("missing").is_undefined());
    QCOMPARE(CborValue(map)["a"].to_string(), std::string_view("two"));
    QCOMPARE(map["c"].to_byte_array().size(), std::size_t{1});
    map.remove("a");
    QVERIFY(!map.contains("a") && map.size() == 2);
    const CborValue date = CborValue::tagged(0, "2026-01-01T00:00:00Z");
    QCOMPARE(date.tag(), std::uint64_t{0});
    QCOMPARE(date.tagged_value().to_string(), std::string_view("2026-01-01T00:00:00Z"));
    QCOMPARE(CborValue(2).to_double(), 2.0);
    QVERIFY(CborValue("x").to_array().empty());
}

//=============================================================================================================

void TestBaseCbor::stringListsAndKeyFiltersMatchQt()
{
    const std::vector<std::string> list{"-u", "", "\xC2\xB5V"};
    const QStringList qtList{"-u", "", QString::fromUtf8("\xC2\xB5V")};
    QCOMPARE(CborValue(CborArray::from_string_list(list)).to_cbor(), bytes(QCborValue(QCborArray::fromStringList(qtList)).toCbor()));

    CborMap map{{"id", "run-01"}, {"data", CborByteArray{std::byte{1}, std::byte{2}}}, {"n", 3}};
    const JsonObject json = map.to_json_object({"data"});
    QCOMPARE(json.keys(), (std::vector<std::string>{"id", "n"}));
    QCOMPARE(map.to_json_object(), CborMap(map).to_json_object({}));
    QCOMPARE(map.to_json_object()["data"].to_string(), std::string_view("AQI")); // base64url, like Qt
}

//=============================================================================================================

void TestBaseCbor::malformedInputIsRejected()
{
    // Each input is rejected by Qt too, except trailing bytes, which Qt ignores, and keys Qt accepts but JSON cannot
    // hold; the codes are Qt's where both detect the same problem first
    const QList<std::pair<QByteArray, CborParseError::Code>> inputs{
        {QByteArray(), CborParseError::EndOfFile},
        {QByteArray::fromHex("1a0001"), CborParseError::EndOfFile},  // truncated integer
        {QByteArray::fromHex("63616263"), CborParseError::NoError},  // valid control case
        {QByteArray::fromHex("6361"), CborParseError::DataTooLarge}, // text longer than the input
        {QByteArray::fromHex("0102"), CborParseError::GarbageAtEnd}, // two values
        {QByteArray::fromHex("ff"), CborParseError::UnexpectedBreak},
        {QByteArray::fromHex("1c"), CborParseError::UnknownType},       // reserved additional info
        {QByteArray::fromHex("1f"), CborParseError::IllegalNumber},     // indefinite integer
        {QByteArray::fromHex("7f4100ff"), CborParseError::IllegalType}, // byte chunk in a text string
        {QByteArray::fromHex("62c328"), CborParseError::InvalidUtf8String},
        {QByteArray::fromHex("5bffffffffffffff00"), CborParseError::DataTooLarge}, // length beyond the input
        {QByteArray::fromHex("9bffffffffffffff00"), CborParseError::DataTooLarge}, // element count beyond the input
        {QByteArray::fromHex("f81f"), CborParseError::IllegalSimpleType},          // two-byte simple value below 32
        {QByteArray::fromHex("a18001"), CborParseError::UnsupportedType},          // array as key
        {QByteArray(2000, '\x81') + QByteArray(1, '\x00'), CborParseError::NestingTooDeep},
    };
    for (const auto& [input, code] : inputs) {
        CborParseError error;
        const CborValue decoded = CborValue::from_cbor(bytes(input), &error);
        QVERIFY2(error.error == code, qPrintable(input.left(16).toHex() + " -> " + QString::number(error.error)));
        QCOMPARE(decoded.is_undefined(), code != CborParseError::NoError);
        if (code != CborParseError::NoError && code != CborParseError::UnsupportedType && code != CborParseError::GarbageAtEnd) {
            QCborParserError qtError;
            QCborValue::fromCbor(input, &qtError);
            QVERIFY2(qtError.error != QCborError::NoError, input.left(16).toHex().constData());
        }
        QVERIFY(!error.error_string().empty());
    }
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestBaseCbor)
#include "test_base_cbor.moc"
