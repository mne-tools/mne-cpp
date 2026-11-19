//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     xml.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Reads XML documents into an element tree in standard C++, the counterpart of QXmlStreamReader.
 *
 * parse_xml() checks that the document is well formed and returns its root element. Every element keeps its
 * name as written, its attributes in document order, the character data directly inside it and its child
 * elements. Comments, processing instructions and the document type declaration are skipped; CDATA sections and
 * the predefined and numeric character references become text. Documents are UTF-8 or, if their declaration
 * says so, ISO-8859-1. Namespaces are not resolved: declarations stay attributes and local_name() drops the
 * prefix like QXmlStreamReader::name().
 */

#ifndef BASE_XML_H
#define BASE_XML_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "base_global.h"
#include "error.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief One element of an XML document with its attributes, text and child elements.
 *
 * @snippet ex_base/main.cpp xml_usage
 */
struct BASESHARED_EXPORT XmlElement
{
    std::string name;                                            /**< Qualified name as written, e.g. "egi:sensor". */
    std::vector<std::pair<std::string, std::string>> attributes; /**< Attributes in document order, values decoded. */
    std::string text;                                            /**< Character data directly inside, child elements left out. */
    std::vector<XmlElement> children;                            /**< Child elements in document order. */

    //=========================================================================================================
    /**
     * @return The name without its namespace prefix, like QXmlStreamReader::name().
     */
    std::string_view local_name() const;

    //=========================================================================================================
    /**
     * @param[in] name   Qualified name of the attribute.
     * @return The value of the attribute, nullptr if the element has none of that name.
     */
    const std::string* attribute(std::string_view name) const;

    //=========================================================================================================
    /**
     * @param[in] local   Local name of the child.
     * @return The first child element with that local name, nullptr if there is none.
     */
    const XmlElement* child(std::string_view local) const;
};

//=============================================================================================================
/**
 * Parses an XML document.
 *
 * @param[in] document   The document, UTF-8 or ISO-8859-1 as its declaration says.
 * @return The root element, or a Format error naming the line where the document is not well formed.
 */
BASESHARED_EXPORT Expected<XmlElement> parse_xml(std::string_view document);

//=============================================================================================================
/**
 * Reads and parses an XML file.
 *
 * @param[in] path   The file.
 * @return The root element, an Io error if the file cannot be read or a Format error naming file and line.
 */
BASESHARED_EXPORT Expected<XmlElement> read_xml_file(const std::filesystem::path& path);

} // namespace mne

#endif // BASE_XML_H
