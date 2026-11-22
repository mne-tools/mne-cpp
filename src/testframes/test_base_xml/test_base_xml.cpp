//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_base_xml.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests parse_xml() and read_xml_file() against Qt's QXmlStreamReader.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/byte_stream.h>
#include <mne/base/xml.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTemporaryDir>
#include <QXmlStreamReader>
#include <QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <optional>
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

/** The element tree QXmlStreamReader sees, or nothing if it finds the document malformed. */
std::optional<XmlElement> qtTree(std::string_view document)
{
    QXmlStreamReader xml(QByteArray(document.data(), qsizetype(document.size())));
    std::vector<XmlElement> open;
    std::optional<XmlElement> root;
    while (!xml.atEnd()) {
        switch (xml.readNext()) {
            case QXmlStreamReader::StartElement: {
                XmlElement element;
                element.name = xml.qualifiedName().toString().toStdString();
                // Qt reports namespace declarations apart; the documents here declare them before other attributes
                for (const QXmlStreamNamespaceDeclaration& ns : xml.namespaceDeclarations())
                    element.attributes.emplace_back(ns.prefix().isEmpty() ? std::string("xmlns") : "xmlns:" + ns.prefix().toString().toStdString(), ns.namespaceUri().toString().toStdString());
                for (const QXmlStreamAttribute& attribute : xml.attributes())
                    element.attributes.emplace_back(attribute.qualifiedName().toString().toStdString(), attribute.value().toString().toStdString());
                open.push_back(std::move(element));
                break;
            }
            case QXmlStreamReader::EndElement: {
                XmlElement done = std::move(open.back());
                open.pop_back();
                if (open.empty())
                    root = std::move(done);
                else
                    open.back().children.push_back(std::move(done));
                break;
            }
            case QXmlStreamReader::Characters:
                if (!open.empty())
                    open.back().text += xml.text().toString().toStdString();
                break;
            default:
                break;
        }
    }
    if (xml.hasError())
        return std::nullopt;
    return root;
}

bool same(const XmlElement& a, const XmlElement& b)
{
    if (a.name != b.name || a.attributes != b.attributes || a.text != b.text || a.children.size() != b.children.size())
        return false;
    for (std::size_t i = 0; i < a.children.size(); ++i)
        if (!same(a.children[i], b.children[i]))
            return false;
    return true;
}

} // namespace

//=============================================================================================================
/**
 * @brief Tests the XML reader.
 */
class TestBaseXml : public QObject
{
    Q_OBJECT

private slots:
    void readsWellFormedDocumentsLikeQt_data();
    void readsWellFormedDocumentsLikeQt();
    void rejectsMalformedDocumentsLikeQt_data();
    void rejectsMalformedDocumentsLikeQt();
    void findsAttributesAndChildrenByName();
    void readsDeeplyNestedDocuments();
    void readsFilesAndNamesThemInErrors();
};

//=============================================================================================================

void TestBaseXml::readsWellFormedDocumentsLikeQt_data()
{
    QTest::addColumn<QByteArray>("document");
    QTest::newRow("captrak") << QByteArray(
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<BrainVisionCapTrak>\r\n  <CapTrakElectrodeList>\r\n"
        "    <CapTrakElectrode><Name>Fp1</Name><X>-27.5</X><Y>88.1</Y><Z>-3.2</Z><Channel>1</Channel></CapTrakElectrode>\r\n"
        "    <CapTrakElectrode><Name>Nasion</Name><X>0</X><Y>95</Y><Z>-30</Z><Channel></Channel></CapTrakElectrode>\r\n"
        "  </CapTrakElectrodeList>\r\n</BrainVisionCapTrak>\r\n");
    QTest::newRow("egi namespaces") << QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<coordinates xmlns=\"http://www.egi.com/coordinates_mff\" "
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n<sensorLayout><sensors>\n"
        "<sensor><name></name><number>1</number><type>0</type><x>6.96</x><y>5.38</y><z>-2.22</z></sensor>\n"
        "<xsi:sensor xsi:kind='ref'><number>2</number></xsi:sensor>\n</sensors></sensorLayout></coordinates>");
    QTest::newRow("references and cdata") << QByteArray(
        "<a t='&lt;&gt;&amp;&apos;&quot;&#65;&#x263A;&#x1F600;'>x &amp; y<![CDATA[ <raw> & ]]>z&#10;<b/>tail</a>");
    QTest::newRow("comments pi doctype") << QByteArray(
        "\xEF\xBB\xBF<?xml version='1.0'?><!-- head --><!DOCTYPE a [<!ELEMENT a ANY>]><?style x?>"
        "<a><!-- inside -->1<?pi data?>2</a><!-- tail -->\n");
    QTest::newRow("attribute white space") << QByteArray("<a v=\"one\ttwo\nthree\r\nfour\" w = 'x' />");
    QTest::newRow("latin1") << QByteArray("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><a n=\"M\xFCller\">Gr\xF6\xDF"
                                          "e</a>");
    QTest::newRow("utf8 names") << QByteArray("<\xC3\xA9l\xC3\xA9"
                                              "ment attr\xC3\xAF=\"\xE2\x82\xAC\">\xE2\x82\xAC</\xC3\xA9l\xC3\xA9"
                                              "ment>");
}

void TestBaseXml::readsWellFormedDocumentsLikeQt()
{
    QFETCH(QByteArray, document);
    const std::string_view text(document.constData(), std::size_t(document.size()));
    const std::optional<XmlElement> qt = qtTree(text);
    QVERIFY(qt.has_value());
    const Expected<XmlElement> ours = parse_xml(text);
    QVERIFY2(ours.has_value(), ours ? "" : ours.error().what());
    QVERIFY(same(*ours, *qt));
}

//=============================================================================================================

void TestBaseXml::rejectsMalformedDocumentsLikeQt_data()
{
    QTest::addColumn<QByteArray>("document");
    QTest::newRow("empty") << QByteArray("");
    QTest::newRow("only a comment") << QByteArray("<!-- x -->");
    QTest::newRow("mismatched tags") << QByteArray("<a><b></a></b>");
    QTest::newRow("unclosed") << QByteArray("<a><b></b>");
    QTest::newRow("two roots") << QByteArray("<a/><b/>");
    QTest::newRow("text after root") << QByteArray("<a/>x");
    QTest::newRow("undeclared entity") << QByteArray("<a>&nbsp;</a>");
    QTest::newRow("bare ampersand") << QByteArray("<a>a & b</a>");
    QTest::newRow("lt in attribute") << QByteArray("<a v='<'/>");
    QTest::newRow("unquoted attribute") << QByteArray("<a v=1/>");
    QTest::newRow("duplicate attribute") << QByteArray("<a v='1' v='2'/>");
    QTest::newRow("attributes not separated") << QByteArray("<a v='1'w='2'/>");
    QTest::newRow("unterminated comment") << QByteArray("<a><!-- x </a>");
    QTest::newRow("cdata end in text") << QByteArray("<a>]]></a>");
    QTest::newRow("character reference to nul") << QByteArray("<a>&#0;</a>");
    QTest::newRow("invalid utf8") << QByteArray("<a>\xFF</a>");
}

void TestBaseXml::rejectsMalformedDocumentsLikeQt()
{
    QFETCH(QByteArray, document);
    const std::string_view text(document.constData(), std::size_t(document.size()));
    QVERIFY(!qtTree(text).has_value());
    const Expected<XmlElement> ours = parse_xml(text);
    QVERIFY(!ours.has_value());
    QCOMPARE(ours.error().code, ErrorCode::Format);
}

//=============================================================================================================

void TestBaseXml::findsAttributesAndChildrenByName()
{
    const Expected<XmlElement> root = parse_xml("<egi:sensors xmlns:egi='u'><egi:sensor id='7'><x>1</x><x>2</x></egi:sensor></egi:sensors>");
    QVERIFY(root.has_value());
    QCOMPARE(root->local_name(), std::string_view("sensors"));
    const XmlElement* sensor = root->child("sensor");
    QVERIFY(sensor != nullptr);
    QCOMPARE(sensor->name, std::string("egi:sensor"));
    QCOMPARE(*sensor->attribute("id"), std::string("7"));
    QVERIFY(sensor->attribute("missing") == nullptr);
    QCOMPARE(sensor->child("x")->text, std::string("1"));
    QVERIFY(sensor->child("y") == nullptr);
}

//=============================================================================================================

void TestBaseXml::readsDeeplyNestedDocuments()
{
    // The parser keeps its own stack, so the depth of a document is not bounded by the call stack
    const int depth = 5000;
    std::string document;
    for (int i = 0; i < depth; ++i)
        document += "<a>";
    document += "leaf";
    for (int i = 0; i < depth; ++i)
        document += "</a>";
    const Expected<XmlElement> root = parse_xml(document);
    QVERIFY(root.has_value());
    const XmlElement* element = &*root;
    for (int i = 1; i < depth; ++i)
        element = &element->children.front();
    QCOMPARE(element->text, std::string("leaf"));
}

//=============================================================================================================

void TestBaseXml::readsFilesAndNamesThemInErrors()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const std::filesystem::path good = std::filesystem::path(dir.path().toStdString()) / "good.xml";
    const std::filesystem::path bad = std::filesystem::path(dir.path().toStdString()) / "bad.xml";
    QVERIFY(write_text_file(good, "<a><b>1</b></a>").has_value());
    QVERIFY(write_text_file(bad, "<a>\n<b></a>").has_value());

    const Expected<XmlElement> read = read_xml_file(good);
    QVERIFY(read.has_value());
    QCOMPARE(read->child("b")->text, std::string("1"));

    const Expected<XmlElement> malformed = read_xml_file(bad);
    QVERIFY(!malformed.has_value());
    QCOMPARE(malformed.error().code, ErrorCode::Format);
    const std::string message = malformed.error().what();
    QVERIFY2(message.find("bad.xml") != std::string::npos && message.find("line 2") != std::string::npos, message.c_str());

    const Expected<XmlElement> missing = read_xml_file(std::filesystem::path(dir.path().toStdString()) / "missing.xml");
    QVERIFY(!missing.has_value());
    QCOMPARE(missing.error().code, ErrorCode::Io);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestBaseXml)
#include "test_base_xml.moc"
