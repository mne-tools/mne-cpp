//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_base_json.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Tests the JSON classes against Qt's and the stream parser on input split at every byte.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/json.h>
#include <base/json_stream_parser.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <limits>
#include <string>
#include <string_view>
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

std::string qt_compact(const QByteArray& json)
{
    return QJsonDocument::fromJson(json).toJson(QJsonDocument::Compact).toStdString();
}

/** Records the tokens of a stream as text, and whether strings were handed out without a copy. */
class Recorder : public JsonHandler
{
public:
    void on_start_object() override
    {
        m_events += '{';
    }
    void on_end_object() override
    {
        m_events += '}';
    }
    void on_start_array() override
    {
        m_events += '[';
    }
    void on_end_array() override
    {
        m_events += ']';
    }
    void on_key(std::string_view key) override
    {
        m_events += "k:" + std::string(key) + ' ';
    }
    void on_string(std::string_view value) override
    {
        m_events += "s:" + std::string(value) + ' ';
        m_lastString = value.data();
    }
    void on_integer(std::int64_t value) override
    {
        m_events += "i:" + std::to_string(value) + ' ';
    }
    void on_number(double value) override
    {
        m_events += "d:" + std::to_string(value) + ' ';
    }
    void on_bool(bool value) override
    {
        m_events += value ? "true " : "false ";
    }
    void on_null() override
    {
        m_events += "null ";
    }
    void on_end_document() override
    {
        m_events += '|';
    }

    std::string m_events;
    const char* m_lastString = nullptr;
};

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestBaseJson
 *
 * @brief Tests the classes in base/json.h and base/json_stream_parser.h.
 */
class TestBaseJson : public QObject
{
    Q_OBJECT

private slots:
    void round_trip_matches_qt_data();
    void round_trip_matches_qt();
    void errors_match_qt_data();
    void errors_match_qt();
    void strict_where_qt_is_lenient_data();
    void strict_where_qt_is_lenient();
    void writer_matches_qt();
    void unicode_escapes_become_utf8();
    void integers_keep_all_digits();
    void getters_fall_back_like_qt();
    void missing_data_is_undefined();
    void objects_stay_sorted_and_unique();
    void arrays_edit_safely();
    void string_lists_and_key_filters_match_qt();
    void stream_yields_the_same_at_every_split();
    void stream_takes_one_byte_at_a_time();
    void handler_gets_tokens_without_copies();
    void callback_receives_documents();
    void depth_is_limited();
    void buffers_are_limited();
    void reset_recovers_from_errors();
    void finish_reports_truncation();
};

//=============================================================================================================

void TestBaseJson::round_trip_matches_qt_data()
{
    QTest::addColumn<QByteArray>("json");
    QTest::newRow("empty object") << QByteArray("{}");
    QTest::newRow("empty array") << QByteArray(" [ ] ");
    QTest::newRow("scalars") << QByteArray(R"([null, true, false, 0, -1, 42, 1.5, -0.25, 3.0, 1e-7, 0.1, 123456789012])");
    QTest::newRow("nested") << QByteArray(R"({"b": {"y": [1, {"z": []}], "x": {}}, "a": [[[]]]})");
    QTest::newRow("strings") << QByteArray(R"(["", "plain", "esc\"aped\\", "\/\b\f\n\r\t", "\u0041\u00e9\u20ac", "caf\u00e9 ✓"])");
    QTest::newRow("surrogates") << QByteArray(R"({"emoji": "\ud83d\ude00", "raw": "😀"})");
    QTest::newRow("controls") << QByteArray(R"(["\u0001\u001f"])");
    QTest::newRow("duplicate keys") << QByteArray(R"({"a": 1, "a": 2, "b": 3})");
    QTest::newRow("unsorted keys") << QByteArray(R"({"zeta": 1, "Alpha": 2, "alpha": 3, "": 4})");
    QTest::newRow("whitespace") << QByteArray("\t\r\n{ \"a\" :\n[ 1 ,2 ] }\n\n");
    QTest::newRow("byte order mark") << QByteArray("\xEF\xBB\xBF{\"a\":1}");
}

void TestBaseJson::round_trip_matches_qt()
{
    QFETCH(QByteArray, json);
    JsonParseError error;
    const JsonDocument document = JsonDocument::from_json(std::string_view(json.constData(), json.size()), &error);
    QCOMPARE(error.error, JsonParseError::NoError);
    QCOMPARE(error.error_string(), std::string_view("no error occurred"));
    QVERIFY(!document.is_null());
    QCOMPARE(document.to_json(JsonDocument::Compact), qt_compact(json));
    QCOMPARE(document.to_json(), QJsonDocument::fromJson(json).toJson().toStdString());
    QCOMPARE(JsonDocument::from_json(document.to_json()), document);
}

//=============================================================================================================

void TestBaseJson::errors_match_qt_data()
{
    QTest::addColumn<QByteArray>("json");
    for (const char* json : {"", "  ", "42", "\"s\"", "{", "[", "{\"a\"", "{\"a\":", "{\"a\":1", "{\"a\" 1}", "{\"a\":1 \"b\":2}",
                             "[1 2]", "{\"a\":1,}", "[1,]", "[-]", "[1e]", "[tru]", "[nul]", "[\"\xff\"]", "[\"\xc0\xaf\"]", "{} x",
                             "{}{}", "[1e400]", "[1e-400]", "[\"abc", "{1:2}", "[}", "{]", "[1", "[,1]"}) {
        QTest::newRow(json) << QByteArray(json);
    }
}

void TestBaseJson::errors_match_qt()
{
    QFETCH(QByteArray, json);
    QJsonParseError qtError;
    QJsonDocument::fromJson(json, &qtError);
    JsonParseError error;
    const JsonDocument document = JsonDocument::from_json(std::string_view(json.constData(), json.size()), &error);
    QVERIFY(document.is_null());
    QCOMPARE(int(error.error), int(qtError.error));
    QCOMPARE(QString::fromUtf8(error.error_string().data(), qsizetype(error.error_string().size())), qtError.errorString());
}

//=============================================================================================================

void TestBaseJson::strict_where_qt_is_lenient_data()
{
    QTest::addColumn<QByteArray>("json");
    QTest::addColumn<int>("error");
    QTest::newRow("leading zero") << QByteArray("[01]") << int(JsonParseError::IllegalNumber);
    QTest::newRow("bare point") << QByteArray("[1.]") << int(JsonParseError::IllegalNumber);
    QTest::newRow("unknown escape") << QByteArray("[\"\\x\"]") << int(JsonParseError::IllegalEscapeSequence);
    QTest::newRow("short unicode escape") << QByteArray("[\"\\u12\"]") << int(JsonParseError::IllegalEscapeSequence);
    QTest::newRow("truncated UTF-8") << QByteArray("[\"\xc3\"]") << int(JsonParseError::IllegalUTF8String);
    QTest::newRow("encoded surrogate") << QByteArray("[\"\xed\xa0\x80\"]") << int(JsonParseError::IllegalUTF8String);
}

void TestBaseJson::strict_where_qt_is_lenient()
{
    QFETCH(QByteArray, json);
    QFETCH(int, error);
    JsonParseError result;
    QVERIFY(JsonDocument::from_json(std::string_view(json.constData(), json.size()), &result).is_null());
    QCOMPARE(int(result.error), error);
}

//=============================================================================================================

void TestBaseJson::writer_matches_qt()
{
    const std::vector<double> numbers = {1.5, 100.0, 1e20, 1e21, 0.0001, 0.00012, 1e-5, 123456789.0, 1e-7, 3.0e15, 1.0 / 3, -0.0,
                                         1e16, 1e16 + 2, 1.5e17, 1234567890123456800.0, 0.001, 5e-324, 1e300};
    JsonArray array;
    QJsonArray qtArray;
    for (const double number : numbers) {
        array.append(number);
        qtArray.append(number);
    }
    array << JsonValue() << true << "x\n\"\x01/\xc3\xa9" << std::numeric_limits<double>::quiet_NaN();
    qtArray << QJsonValue() << true << QString::fromUtf8("x\n\"\x01/\xc3\xa9") << std::numeric_limits<double>::quiet_NaN();

    JsonObject object{{"b", 1}, {"a", array}, {"e", JsonObject()}, {"f", JsonArray()}, {"n", JsonObject{{"k", JsonArray{JsonObject{{"z", 1}}}}}}};
    QJsonObject qtObject{{"b", 1}, {"a", qtArray}, {"e", QJsonObject()}, {"f", QJsonArray()}, {"n", QJsonObject{{"k", QJsonArray{QJsonObject{{"z", 1}}}}}}};

    QCOMPARE(JsonDocument(object).to_json(), QJsonDocument(qtObject).toJson().toStdString());
    QCOMPARE(JsonDocument(object).to_json(JsonDocument::Compact), QJsonDocument(qtObject).toJson(QJsonDocument::Compact).toStdString());
    QCOMPARE(JsonDocument(JsonArray()).to_json(), std::string("[\n]\n"));
    QCOMPARE(JsonDocument().to_json(), std::string());
}

//=============================================================================================================

void TestBaseJson::unicode_escapes_become_utf8()
{
    const JsonDocument document = JsonDocument::from_json(R"(["\u00e9", "\ud83d\ude00", "\ud800", "\ud800x", "\udc00", "\ud800\u0041", "\ud800\n", "a\u0000b"])");
    QCOMPARE(document[0].to_string(), std::string_view("\xc3\xa9"));
    QCOMPARE(document[1].to_string(), std::string_view("\xf0\x9f\x98\x80"));
    // Lone surrogates have no UTF-8 form and become U+FFFD.
    QCOMPARE(document[2].to_string(), std::string_view("\xef\xbf\xbd"));
    QCOMPARE(document[3].to_string(), std::string_view("\xef\xbf\xbdx"));
    QCOMPARE(document[4].to_string(), std::string_view("\xef\xbf\xbd"));
    QCOMPARE(document[5].to_string(), std::string_view("\xef\xbf\xbd"
                                                       "A"));
    QCOMPARE(document[6].to_string(), std::string_view("\xef\xbf\xbd\n"));
    QCOMPARE(document[7].to_string(), std::string_view("a\0b", 3));
}

//=============================================================================================================

void TestBaseJson::integers_keep_all_digits()
{
    const JsonDocument document = JsonDocument::from_json("[9007199254740993, -9223372036854775808, 9223372036854775807, 9223372036854775808, 2.0]");
    QVERIFY(document[0].is_integer() && document[0].is_double());
    QCOMPARE(document[0].type(), JsonValue::Double);
    QCOMPARE(document[0].to_integer(), std::int64_t(9007199254740993));
    QCOMPARE(document[1].to_integer(), std::numeric_limits<std::int64_t>::min());
    QCOMPARE(document[2].to_integer(), std::numeric_limits<std::int64_t>::max());
    QVERIFY(!document[3].is_integer());
    QCOMPARE(document[3].to_double(), 9223372036854775808.0);
    QCOMPARE(document[3].to_integer(-1), std::int64_t(-1));
    QVERIFY(!document[4].is_integer());
    QCOMPARE(document[4].to_integer(), std::int64_t(2));
    QCOMPARE(document.to_json(JsonDocument::Compact), std::string("[9007199254740993,-9223372036854775808,9223372036854775807,9223372036854776000,2]"));
    QCOMPARE(JsonValue(std::numeric_limits<std::uint64_t>::max()).to_double(), 18446744073709551615.0);
}

//=============================================================================================================

void TestBaseJson::getters_fall_back_like_qt()
{
    const JsonDocument document = JsonDocument::from_json(R"({"b": true, "i": 3, "big": 3000000000, "d": 2.5, "w": 3.0, "s": "text", "n": null, "a": [1], "o": {"k": 1}})");
    const QJsonDocument qt = QJsonDocument::fromJson(R"({"b": true, "i": 3, "big": 3000000000, "d": 2.5, "w": 3.0, "s": "text", "n": null, "a": [1], "o": {"k": 1}})");
    for (const char* key : {"b", "i", "big", "d", "w", "s", "n", "a", "o", "missing"}) {
        const JsonValue& value = document[key];
        const QJsonValue qtValue = qt[QLatin1String(key)];
        QCOMPARE(int(value.type()), int(qtValue.type()));
        QCOMPARE(value.to_bool(), qtValue.toBool());
        QCOMPARE(value.to_bool(true), qtValue.toBool(true));
        QCOMPARE(value.to_int(-7), qtValue.toInt(-7));
        QCOMPARE(value.to_integer(-7), qtValue.toInteger(-7));
        QCOMPARE(value.to_double(-7.0), qtValue.toDouble(-7.0));
        QCOMPARE(QString::fromStdString(std::string(value.to_string("fallback"))), qtValue.toString("fallback"));
        QCOMPARE(value.to_array().size(), std::size_t(qtValue.toArray().size()));
        QCOMPARE(value.to_object().size(), std::size_t(qtValue.toObject().size()));
    }
    QCOMPARE(JsonValue(1), JsonValue(1.0));
    QVERIFY(!(JsonValue(1) == JsonValue(true)));
    QVERIFY(!(JsonValue() == JsonValue(JsonValue::Undefined)));
}

//=============================================================================================================

void TestBaseJson::missing_data_is_undefined()
{
    const JsonDocument document = JsonDocument::from_json(R"({"list": [1, 2], "value": 5})");
    QVERIFY(document["missing"].is_undefined());
    QVERIFY(!document["missing"].is_null());
    QVERIFY(document["list"][2].is_undefined());
    QVERIFY(document["list"]["key"].is_undefined());
    QVERIFY(document["value"][0].is_undefined());
    QVERIFY(document[0].is_undefined());
    QVERIFY(document["missing"]["deeper"][3].is_undefined());
    QCOMPARE(document["list"][1].to_int(), 2);
    QVERIFY(JsonArray().first().is_undefined() && JsonArray().last().is_undefined());
}

//=============================================================================================================

void TestBaseJson::objects_stay_sorted_and_unique()
{
    JsonObject object{{"c", 3}, {"a", 1}, {"b", 2}, {"a", 10}};
    QCOMPARE(object.keys(), std::vector<std::string>({"a", "b", "c"}));
    QCOMPARE(object["a"].to_int(), 10);

    QCOMPARE(object.insert("b", "two")->value().to_string(), std::string_view("two"));
    object["aa"] = JsonArray{1, 2};
    object["d"];
    QCOMPARE(object.keys(), std::vector<std::string>({"a", "aa", "b", "c", "d"}));
    QVERIFY(object.value("d").is_null());
    QCOMPARE(object.take("c").to_int(), 3);
    object.remove("a");
    object.remove("nothing");
    QVERIFY(object.take("nothing").is_undefined());
    QCOMPARE(object.size(), std::size_t(3));
    QVERIFY(object.contains("aa") && !object.contains("a"));

    std::string keys;
    for (auto& [key, value] : object) {
        keys += key;
        value = JsonValue(1);
    }
    QCOMPARE(keys, std::string("aabd"));
    for (const auto& member : object) {
        QCOMPARE(member.value().to_int(), 1);
    }
    object.erase(object.find("b"));
    QCOMPARE(object, (JsonObject{{"d", 1}, {"aa", 1}}));
}

//=============================================================================================================

void TestBaseJson::arrays_edit_safely()
{
    JsonArray array{1, "two"};
    array.prepend(0);
    array.append(JsonObject{{"k", true}});
    array.insert(99, 4);
    array.insert(1, 0.5);
    QCOMPARE(JsonDocument(array).to_json(JsonDocument::Compact), std::string(R"([0,0.5,1,"two",{"k":true},4])"));
    array.remove_at(1);
    array.remove_at(99);
    array.remove_first();
    array.remove_last();
    QCOMPARE(array.take_at(0).to_int(), 1);
    QVERIFY(array.take_at(5).is_undefined());
    array.replace(0, "2");
    array.replace(9, "ignored");
    QVERIFY(array.contains(JsonValue("2")));
    array[7] = JsonValue(1); // dropped, not a crash
    array[0] = JsonValue(nullptr);
    QCOMPARE(JsonDocument(array).to_json(JsonDocument::Compact), std::string(R"([null,{"k":true}])"));
    int count = 0;
    for (const JsonValue& value : array) {
        count += value.is_null() || value.is_object();
    }
    QCOMPARE(count, 2);
}

//=============================================================================================================

void TestBaseJson::string_lists_and_key_filters_match_qt()
{
    const std::vector<std::string> list{"MEG 0113", "", "\xC2\xB5V"};
    const QStringList qtList{"MEG 0113", "", QString::fromUtf8("\xC2\xB5V")};
    const JsonArray array = JsonArray::from_string_list(list);
    QCOMPARE(JsonDocument(array).to_json(JsonDocument::Compact), QJsonDocument(QJsonArray::fromStringList(qtList)).toJson(QJsonDocument::Compact).toStdString());
    QCOMPARE(array.to_string_list(), list);
    QCOMPARE((JsonArray{"a", 1, nullptr}.to_string_list()), (std::vector<std::string>{"a", "", ""}));

    const JsonObject object{{"a", 1}, {"b", 2}, {"c", JsonArray{3}}};
    QCOMPARE(JsonDocument(object.without({"b", "missing"})).to_json(JsonDocument::Compact), std::string(R"({"a":1,"c":[3]})"));
    QCOMPARE(object.without({}), object);
    QVERIFY(object.without({"a", "b", "c"}).empty());
}

//=============================================================================================================

void TestBaseJson::stream_yields_the_same_at_every_split()
{
    const std::string stream = "{\"event\":1,\"name\":\"caf\xc3\xa9 \\u00e9\\ud83d\\ude00\",\"v\":[-1.25e-3,true,null]}\n"
                               "[{\"a\":{}},[],\"x\"]{\"event\":2,\"big\":12345678901234}  \r\n{}";
    std::vector<std::string> expected;
    {
        JsonStreamParser parser;
        QVERIFY(parser.feed(stream) && parser.finish());
        while (parser.has_document()) {
            expected.push_back(parser.take_document().to_json(JsonDocument::Compact));
        }
    }
    QCOMPARE(expected.size(), std::size_t(4));
    QCOMPARE(expected[2], std::string(R"({"big":12345678901234,"event":2})"));

    for (std::size_t first = 0; first <= stream.size(); ++first) {
        for (std::size_t second = first; second <= stream.size(); second += 7) {
            JsonStreamParser parser;
            QVERIFY(parser.feed(std::string_view(stream).substr(0, first)));
            QVERIFY(parser.feed(std::string_view(stream).substr(first, second - first)));
            QVERIFY(parser.feed(std::string_view(stream).substr(second)));
            QVERIFY(parser.finish());
            QCOMPARE(parser.document_count(), expected.size());
            for (const std::string& json : expected) {
                QCOMPARE(parser.take_document().to_json(JsonDocument::Compact), json);
            }
        }
    }
}

//=============================================================================================================

void TestBaseJson::stream_takes_one_byte_at_a_time()
{
    const std::string stream = "{\"s\":\"a\\\"b\\\\c\\u20ac\\ud834\\udd1e\",\"n\":-0.5e+2}\n[123456789,false]";
    Recorder whole;
    JsonStreamParser parser;
    parser.set_handler(&whole);
    QVERIFY(parser.feed(stream));

    Recorder bytes;
    JsonStreamParser byteParser;
    byteParser.set_handler(&bytes);
    for (const char c : stream) {
        QVERIFY(byteParser.feed(std::span<const std::byte>(reinterpret_cast<const std::byte*>(&c), 1)));
    }
    QVERIFY(byteParser.finish());
    QCOMPARE(bytes.m_events, whole.m_events);
    QCOMPARE(bytes.m_events, std::string("{k:s s:a\"b\\c\xe2\x82\xac\xf0\x9d\x84\x9e k:n d:-50.000000 }|[i:123456789 false ]|"));
    QCOMPARE(byteParser.bytes_fed(), stream.size());
    QVERIFY(!byteParser.has_document());
}

//=============================================================================================================

void TestBaseJson::handler_gets_tokens_without_copies()
{
    Recorder recorder;
    JsonStreamParser parser;
    parser.set_handler(&recorder);
    const std::string chunk = R"({"telemetry":[1,2.5,"plain"]})";
    QVERIFY(parser.feed(chunk));
    // An unescaped string within one chunk is a view into that chunk.
    QCOMPARE(recorder.m_lastString, chunk.data() + chunk.find("plain"));
    QCOMPARE(recorder.m_events, std::string("{k:telemetry [i:1 d:2.500000 s:plain ]}|"));
    QCOMPARE(parser.depth(), std::size_t(0));
    QVERIFY(parser.is_idle());

    parser.set_handler(nullptr);
    QVERIFY(parser.feed(chunk));
    QCOMPARE(parser.take_document()["telemetry"][2].to_string(), std::string_view("plain"));
}

//=============================================================================================================

void TestBaseJson::callback_receives_documents()
{
    std::vector<int> events;
    JsonStreamParser parser;
    parser.set_document_callback([&](JsonDocument&& document) { events.push_back(document["event"].to_int()); });
    QVERIFY(parser.feed(R"({"event":1}{"ev)"));
    QCOMPARE(events, std::vector<int>({1}));
    QVERIFY(!parser.is_idle());
    QVERIFY(parser.feed(R"(ent":2}   {"event":3})"));
    QCOMPARE(events, std::vector<int>({1, 2, 3}));
    QVERIFY(!parser.has_document());

    JsonStreamParser::Options options;
    options.singleDocument = true;
    JsonStreamParser single(options);
    QVERIFY(single.feed("{}  "));
    QVERIFY(!single.feed("{}"));
    QCOMPARE(single.error().error, JsonParseError::GarbageAtEnd);
    QCOMPARE(single.error().offset, std::size_t(4));
    QVERIFY(single.has_document());
}

//=============================================================================================================

void TestBaseJson::depth_is_limited()
{
    JsonStreamParser::Options options;
    options.maxDepth = 3;
    JsonStreamParser parser(options);
    QVERIFY(parser.feed("[[[]]]"));
    QVERIFY(!parser.feed("[[[{"));
    QCOMPARE(parser.error().error, JsonParseError::DeepNesting);
    QCOMPARE(parser.error().offset, std::size_t(9));
    QVERIFY(!parser.feed("}]]]")); // ignored until reset()

    QVERIFY(!JsonStreamParser().feed(std::string(65, '[')));
    QVERIFY(JsonStreamParser().feed(std::string(64, '[')));

    JsonParseError error;
    QVERIFY(JsonDocument::from_json(std::string(1025, '[') + std::string(1025, ']'), &error).is_null());
    QCOMPARE(error.error, JsonParseError::DeepNesting);
    QVERIFY(!JsonDocument::from_json(std::string(1024, '[') + std::string(1024, ']')).is_null());
}

//=============================================================================================================

void TestBaseJson::buffers_are_limited()
{
    JsonStreamParser::Options options;
    options.maxBufferSize = 8;
    options.maxDocumentSize = 40;

    JsonStreamParser split(options);
    QVERIFY(split.feed(R"(["12345)"));
    QVERIFY(split.feed(R"(678"])")); // eight bytes buffered across chunks
    QVERIFY(split.feed(R"(["1234)"));
    QVERIFY(!split.feed(R"(56789"])"));
    QCOMPARE(split.error().error, JsonParseError::DocumentTooLarge);

    JsonStreamParser whole(options);
    QVERIFY(whole.feed(R"(["a string longer than the buffer"])")); // a view of the chunk needs no buffer
    QVERIFY(!JsonStreamParser(options).feed(R"(["escaped\n string"])"));

    JsonStreamParser large(options);
    QVERIFY(large.feed(R"([1,2,3,4,5,6,7,8,9,10,)"));
    QVERIFY(!large.feed(R"(11,12,13,14,15,16,17,18,19)"));
    QCOMPARE(large.error().error, JsonParseError::DocumentTooLarge);
}

//=============================================================================================================

void TestBaseJson::reset_recovers_from_errors()
{
    JsonStreamParser parser;
    QVERIFY(parser.feed("{\"ok\":1}\n"));
    QVERIFY(!parser.feed("{\"broken\":tru}\n"));
    QCOMPARE(parser.error().error, JsonParseError::IllegalValue);
    QCOMPARE(parser.error().offset, std::size_t(22)); // the "}" after "tru"
    QVERIFY(parser.has_error());
    parser.reset();
    QVERIFY(!parser.has_error());
    QCOMPARE(parser.bytes_fed(), std::size_t(0));
    QVERIFY(parser.feed("{\"ok\":2}"));
    QCOMPARE(parser.document_count(), std::size_t(2));
    QCOMPARE(parser.take_document()["ok"].to_int(), 1);
    QCOMPARE(parser.take_document()["ok"].to_int(), 2);
    QVERIFY(parser.take_document().is_null());
}

//=============================================================================================================

void TestBaseJson::finish_reports_truncation()
{
    const std::vector<std::pair<std::string, JsonParseError::ParseError>> cases = {
        {"", JsonParseError::NoError},
        {" \n", JsonParseError::NoError},
        {"{\"a\":1}", JsonParseError::NoError},
        {"{\"a\":\"x", JsonParseError::UnterminatedString},
        {"{\"a\":\"\\u12", JsonParseError::UnterminatedString},
        {"{\"a\"", JsonParseError::MissingNameSeparator},
        {"{\"a\":tr", JsonParseError::IllegalValue},
        {"{\"a\":[1", JsonParseError::UnterminatedArray},
        {"{\"a\":[1],", JsonParseError::UnterminatedObject},
    };
    for (const auto& [json, error] : cases) {
        JsonStreamParser parser;
        QVERIFY(parser.feed(json));
        QCOMPARE(parser.finish(), error == JsonParseError::NoError);
        QCOMPARE(parser.error().error, error);
        if (error != JsonParseError::NoError) {
            QCOMPARE(parser.error().offset, json.size());
        }
    }
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestBaseJson)
#include "test_base_json.moc"
