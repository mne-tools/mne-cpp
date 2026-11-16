//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_registry_loader.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of the @c mna-registry.json manifest reader, drop-in directory merger and registry-to-manifest serialiser.
 *
 * @ref MnaRegistryLoader::load_file parses one manifest JSON, walks
 * the @c ops array, deserialises each entry into a
 * @ref MnaOpSchema with its port and attribute lists, and calls
 * @ref MnaOpRegistry::register_op; missing or malformed entries
 * are skipped with a warning so a single broken op does not
 * abort the load. @ref load_directory loads @c mna-registry.json
 * first, then every @c *.json file under @c mna-registry.d/ in
 * alphabetical order so later files override earlier definitions,
 * which is how plug-in packages extend or shadow built-in ops.
 *
 * @ref save_file inverts the process: it walks every schema
 * currently registered with @ref MnaOpRegistry, emits the
 * canonical manifest header (@c mna_registry_version, @c provider)
 * and the @c ops array, and writes the file pretty-printed so the
 * output remains diff-friendly under version control.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_registry_loader.h"

#include "mna_op_registry.h"
#include "mna_op_schema.h"

#include <base/byte_stream.h>
#include <base/json.h>
#include <base/log.h>
#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <system_error>
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

MnaAttrType attrTypeFromString(std::string_view text)
{
    if (text == "double")
        return MnaAttrType::Double;
    if (text == "float")
        return MnaAttrType::Float;
    if (text == "int")
        return MnaAttrType::Int;
    if (text == "bool")
        return MnaAttrType::Bool;
    return MnaAttrType::String; // also "string" and "path"
}

std::string_view attrTypeToString(MnaAttrType type)
{
    switch (type) {
        case MnaAttrType::Double:
            return "double";
        case MnaAttrType::Float:
            return "float";
        case MnaAttrType::Int:
            return "int";
        case MnaAttrType::Bool:
            return "bool";
        case MnaAttrType::String:
            break;
    }
    return "string";
}

MnaOpSchemaPort parsePort(const JsonObject& obj)
{
    MnaOpSchemaPort port;
    port.name = obj["name"].to_std_string();
    port.data_kind = data_kind_from_string(obj["kind"].to_string());
    port.required = obj["required"].to_bool(true);
    port.description = obj["description"].to_std_string();
    return port;
}

JsonObject portToJson(const MnaOpSchemaPort& port)
{
    JsonObject obj;
    obj["name"] = port.name;
    obj["kind"] = to_string(port.data_kind);
    obj["description"] = port.description;
    if (!port.required)
        obj["required"] = false;
    return obj;
}

MnaOpSchemaAttr parseAttr(const JsonObject& obj)
{
    MnaOpSchemaAttr attr;
    attr.name = obj["name"].to_std_string();
    attr.type = attrTypeFromString(obj["type"].to_string());
    attr.required = obj["required"].to_bool(false);
    attr.description = obj["description"].to_std_string();
    attr.default_value = obj["default"];
    return attr;
}

JsonObject attrToJson(const MnaOpSchemaAttr& attr)
{
    JsonObject obj;
    obj["name"] = attr.name;
    obj["type"] = attrTypeToString(attr.type);
    if (attr.required)
        obj["required"] = true;
    if (!attr.description.empty())
        obj["description"] = attr.description;
    if (!attr.default_value.is_undefined())
        obj["default"] = attr.default_value;
    return obj;
}

template<class T, class Parse>
std::vector<T> parseList(const JsonValue& value, Parse parse)
{
    std::vector<T> items;
    for (const JsonValue& item : value.to_array()) {
        items.push_back(parse(item.to_object()));
    }
    return items;
}

template<class T, class Write>
JsonArray writeList(const std::vector<T>& items, Write write)
{
    JsonArray array;
    for (const T& item : items) {
        array.push_back(write(item));
    }
    return array;
}

MnaOpSchema parseOp(const JsonObject& obj)
{
    MnaOpSchema schema;
    schema.op_type = obj["type"].to_std_string();
    schema.version = obj["version"].to_std_string();
    schema.binding = obj["binding"].to_string("internal");
    schema.category = obj["category"].to_std_string();
    schema.description = obj["description"].to_std_string();
    schema.library = obj["library"].to_std_string();
    schema.executable = obj["executable"].to_std_string();
    schema.cli_template = obj["cli_template"].to_std_string();
    schema.input_ports = parseList<MnaOpSchemaPort>(obj["inputs"], parsePort);
    schema.output_ports = parseList<MnaOpSchemaPort>(obj["outputs"], parsePort);
    schema.attributes = parseList<MnaOpSchemaAttr>(obj["parameters"], parseAttr);
    return schema;
}

JsonObject opToJson(const MnaOpSchema& schema)
{
    JsonObject obj;
    obj["type"] = schema.op_type;
    obj["description"] = schema.description;
    if (!schema.version.empty())
        obj["version"] = schema.version;
    if (!schema.binding.empty())
        obj["binding"] = schema.binding;
    if (!schema.category.empty())
        obj["category"] = schema.category;
    if (!schema.library.empty())
        obj["library"] = schema.library;
    if (!schema.executable.empty())
        obj["executable"] = schema.executable;
    if (!schema.cli_template.empty())
        obj["cli_template"] = schema.cli_template;
    obj["inputs"] = writeList(schema.input_ports, portToJson);
    obj["outputs"] = writeList(schema.output_ports, portToJson);
    obj["parameters"] = writeList(schema.attributes, attrToJson);
    return obj;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

int MnaRegistryLoader::load_file(const std::filesystem::path& path, MnaOpRegistry& registry)
{
    const Expected<std::string> text = read_text_file(path);
    if (!text) {
        Log::warning("MnaRegistryLoader: Cannot open {}", utf8_from_path(path));
        return -1;
    }
    JsonParseError parseError;
    const JsonDocument doc = JsonDocument::from_json(*text, &parseError);
    if (parseError.error != JsonParseError::NoError) {
        Log::warning("MnaRegistryLoader: JSON parse error in {} : {}", utf8_from_path(path), parseError.error_string());
        return -1;
    }
    const JsonObject& root = doc.object();
    if (!root.contains("mna_registry_version")) {
        Log::warning("MnaRegistryLoader: Missing 'mna_registry_version' in {}", utf8_from_path(path));
        return -1;
    }

    int count = 0;
    for (const JsonValue& op : root["ops"].to_array()) {
        MnaOpSchema schema = parseOp(op.to_object());
        if (!schema.op_type.empty()) {
            registry.register_op(schema);
            ++count;
        }
    }
    Log::info("MnaRegistryLoader: Loaded {} ops from {} (provider: {} )", count, utf8_from_path(path), root["provider"].to_string());
    return count;
}

//=============================================================================================================

int MnaRegistryLoader::load_directory(const std::filesystem::path& dir, MnaOpRegistry& registry)
{
    int total = 0;
    std::error_code ec;
    if (const std::filesystem::path master = dir / "mna-registry.json"; std::filesystem::exists(master, ec)) {
        total += std::max(load_file(master, registry), 0);
    }

    std::vector<std::filesystem::path> dropIns;
    for (const auto& entry : std::filesystem::directory_iterator(dir / "mna-registry.d", ec)) {
        if (entry.is_regular_file(ec) && entry.path().extension() == ".json") {
            dropIns.push_back(entry.path());
        }
    }
    std::ranges::sort(dropIns);
    for (const std::filesystem::path& file : dropIns) {
        total += std::max(load_file(file, registry), 0);
    }
    return total;
}

//=============================================================================================================

bool MnaRegistryLoader::save_file(const std::filesystem::path& path, std::string_view provider, const MnaOpRegistry& registry)
{
    JsonObject root;
    root["mna_registry_version"] = "1.0";
    root["provider"] = provider;
    JsonArray ops;
    for (const std::string& op : registry.registered_ops()) {
        ops.push_back(opToJson(registry.schema(op)));
    }
    root["ops"] = std::move(ops);

    if (!write_text_file(path, JsonDocument(root).to_json(JsonDocument::Indented))) {
        Log::warning("MnaRegistryLoader: Cannot write {}", utf8_from_path(path));
        return false;
    }
    return true;
}
