//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_param_tree.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of the hierarchical parameter store: path-keyed value access, binding management, formula evaluation and JSON round-trip.
 *
 * The implementation keeps two parallel @c QMap members — one
 * mapping path strings to literal @c QVariant values, the other
 * mapping target paths to their @ref MnaParamBinding definitions.
 * @ref MnaParamTree::evaluate iterates over registered bindings,
 * fires those whose trigger condition is met for the supplied
 * execution-context @c results map, calls
 * @ref MnaParamTree::evaluate_expression to compute the new value
 * via the embedded expression evaluator (with @c ref("…") look-ups
 * resolved against both the param store and the execution results),
 * writes the result back into @c m_params and returns the list of
 * paths whose values actually changed so the executor can mark
 * downstream nodes dirty.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_param_tree.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <charconv>
#include <cmath>
#include <optional>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

/** @return The value as a number: numbers as they are, booleans as 0 or 1, numeric text parsed. */
std::optional<double> toNumber(const JsonValue& value)
{
    if (value.is_number()) {
        return value.to_double();
    }
    if (value.is_bool()) {
        return value.to_bool() ? 1.0 : 0.0;
    }
    if (value.is_string()) {
        return mne::to_double(value.to_string());
    }
    return std::nullopt;
}

/** Recursive-descent evaluator of one formula; any failure yields an undefined value. */
class Evaluator
{
public:
    using Lookup = std::function<JsonValue(std::string_view)>;

    Evaluator(std::string_view text, Lookup lookup)
    : m_text(text)
    , m_lookup(std::move(lookup))
    {
    }

    JsonValue run()
    {
        JsonValue value = comparison();
        skipSpace();
        return m_pos == m_text.size() ? value : JsonValue(JsonValue::Undefined);
    }

private:
    static bool isNameStart(char c)
    {
        return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_';
    }
    static bool isNameChar(char c)
    {
        return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '.' || c == ':' || c == '/';
    }

    void skipSpace()
    {
        while (m_pos < m_text.size() && std::isspace(static_cast<unsigned char>(m_text[m_pos])) != 0) {
            ++m_pos;
        }
    }

    bool accept(std::string_view token)
    {
        skipSpace();
        if (m_text.substr(m_pos).starts_with(token)) {
            m_pos += token.size();
            return true;
        }
        return false;
    }

    JsonValue comparison()
    {
        JsonValue left = sum();
        for (const std::string_view op : {">=", "<=", "==", "!=", ">", "<"}) {
            if (!accept(op)) {
                continue;
            }
            const JsonValue right = sum();
            if (left.is_undefined() || right.is_undefined()) {
                return JsonValue(JsonValue::Undefined);
            }
            if (left.is_string() && right.is_string() && (op == "==" || op == "!=")) {
                return (left.to_string() == right.to_string()) == (op == "==");
            }
            const std::optional<double> a = toNumber(left);
            const std::optional<double> b = toNumber(right);
            if (!a || !b) {
                return JsonValue(JsonValue::Undefined);
            }
            return op == ">=" ? *a >= *b : op == "<=" ? *a <= *b
                : op == "=="                          ? *a == *b
                : op == "!="                          ? *a != *b
                : op == ">"                           ? *a > *b
                                                      : *a < *b;
        }
        return left;
    }

    JsonValue sum()
    {
        JsonValue value = product();
        for (;;) {
            const bool plus = accept("+");
            if (!plus && !accept("-")) {
                return value;
            }
            value = arithmetic(value, product(), plus ? '+' : '-');
        }
    }

    JsonValue product()
    {
        JsonValue value = unary();
        for (;;) {
            const bool times = accept("*");
            if (!times && !accept("/")) {
                return value;
            }
            value = arithmetic(value, unary(), times ? '*' : '/');
        }
    }

    JsonValue unary()
    {
        if (accept("-")) {
            const std::optional<double> v = toNumber(unary());
            return v ? JsonValue(-*v) : JsonValue(JsonValue::Undefined);
        }
        return primary();
    }

    static JsonValue arithmetic(const JsonValue& left, const JsonValue& right, char op)
    {
        const std::optional<double> a = toNumber(left);
        const std::optional<double> b = toNumber(right);
        if (left.is_undefined() || right.is_undefined() || !a || !b) {
            return JsonValue(JsonValue::Undefined);
        }
        switch (op) {
            case '+':
                return *a + *b;
            case '-':
                return *a - *b;
            case '*':
                return *a * *b;
            default:
                return std::abs(*b) < 1e-15 ? JsonValue(JsonValue::Undefined) : JsonValue(*a / *b);
        }
    }

    JsonValue primary()
    {
        skipSpace();
        if (m_pos >= m_text.size()) {
            return JsonValue(JsonValue::Undefined);
        }
        const char c = m_text[m_pos];
        if (c == '(') {
            ++m_pos;
            JsonValue value = comparison();
            return accept(")") ? value : JsonValue(JsonValue::Undefined);
        }
        if (c == '\'') {
            const std::size_t end = m_text.find('\'', m_pos + 1);
            if (end == std::string_view::npos) {
                return JsonValue(JsonValue::Undefined);
            }
            JsonValue text(m_text.substr(m_pos + 1, end - m_pos - 1));
            m_pos = end + 1;
            return text;
        }
        if (std::isdigit(static_cast<unsigned char>(c)) != 0 || c == '.') {
            double number = 0.0;
            const auto [next, ec] = std::from_chars(m_text.data() + m_pos, m_text.data() + m_text.size(), number);
            if (ec != std::errc{}) {
                return JsonValue(JsonValue::Undefined);
            }
            m_pos = static_cast<std::size_t>(next - m_text.data());
            return number;
        }
        if (!isNameStart(c)) {
            return JsonValue(JsonValue::Undefined);
        }
        const std::size_t start = m_pos;
        while (m_pos < m_text.size() && isNameChar(m_text[m_pos])) {
            ++m_pos;
        }
        const std::string_view name = m_text.substr(start, m_pos - start);
        if (!accept("(")) {
            if (name == "true" || name == "false") {
                return name == "true";
            }
            return m_lookup(name);
        }
        std::vector<JsonValue> args;
        if (!accept(")")) {
            do {
                args.push_back(comparison());
            } while (accept(","));
            if (!accept(")")) {
                return JsonValue(JsonValue::Undefined);
            }
        }
        return call(name, args);
    }

    JsonValue call(std::string_view name, const std::vector<JsonValue>& args) const
    {
        if (name == "ref") {
            return args.size() == 1 && args[0].is_string() ? m_lookup(args[0].to_string()) : JsonValue(JsonValue::Undefined);
        }
        std::vector<double> x;
        for (const JsonValue& arg : args) {
            const std::optional<double> v = toNumber(arg);
            if (arg.is_undefined() || !v) {
                return JsonValue(JsonValue::Undefined);
            }
            x.push_back(*v);
        }
        if (name == "clamp" && x.size() == 3) {
            return std::clamp(x[0], x[1], x[2]);
        }
        if (name == "scale" && x.size() == 2) {
            return x[0] * x[1];
        }
        if (name == "threshold" && x.size() == 4) {
            return x[0] > x[1] ? x[2] : x[3];
        }
        if (name == "lerp" && x.size() == 3) {
            return x[0] + ((x[1] - x[0]) * x[2]);
        }
        return JsonValue(JsonValue::Undefined);
    }

    std::string_view m_text;
    std::size_t m_pos = 0;
    Lookup m_lookup;
};

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void MnaParamTree::set_param(std::string_view path, JsonValue value)
{
    m_params.insert_or_assign(std::string(path), std::move(value));
}

//=============================================================================================================

const JsonValue& MnaParamTree::param(std::string_view path) const
{
    const auto it = m_params.find(path);
    return it != m_params.end() ? it->second : JsonValue::undefined_value();
}

//=============================================================================================================

bool MnaParamTree::has_param(std::string_view path) const
{
    return m_params.contains(path);
}

//=============================================================================================================

StringList MnaParamTree::all_paths() const
{
    StringList paths;
    paths.reserve(m_params.size());
    for (const auto& [path, value] : m_params) {
        paths.push_back(path);
    }
    return paths;
}

//=============================================================================================================

void MnaParamTree::add_binding(const MnaParamBinding& binding)
{
    m_bindings.insert_or_assign(binding.target_path, binding);
}

//=============================================================================================================

void MnaParamTree::remove_binding(std::string_view target_path)
{
    if (const auto it = m_bindings.find(target_path); it != m_bindings.end()) {
        m_bindings.erase(it);
    }
}

//=============================================================================================================

std::vector<MnaParamBinding> MnaParamTree::bindings() const
{
    std::vector<MnaParamBinding> list;
    list.reserve(m_bindings.size());
    for (const auto& [path, binding] : m_bindings) {
        list.push_back(binding);
    }
    return list;
}

//=============================================================================================================

bool MnaParamTree::has_binding(std::string_view target_path) const
{
    return m_bindings.contains(target_path);
}

//=============================================================================================================

StringList MnaParamTree::evaluate(const JsonObject& results)
{
    StringList changed;
    for (const auto& [path, binding] : m_bindings) {
        if (binding.trigger == "manual") { // manual bindings need explicit invocation
            continue;
        }
        JsonValue value = evaluate_expression(binding.expression, results);
        if (value.is_undefined() || param(path) == value) {
            continue;
        }
        m_params.insert_or_assign(path, std::move(value));
        changed.push_back(path);
    }
    return changed;
}

//=============================================================================================================

JsonValue MnaParamTree::evaluate_expression(std::string_view expression, const JsonObject& results) const
{
    return Evaluator(expression, [&](std::string_view name) {
               const auto it = m_params.find(name);
               return it != m_params.end() ? it->second : results.value(name);
           })
        .run();
}

//=============================================================================================================

JsonObject MnaParamTree::to_json() const
{
    JsonObject json;
    JsonArray bindingList;
    for (const auto& [path, binding] : m_bindings) {
        bindingList.push_back(binding.to_json());
    }
    json["bindings"] = std::move(bindingList);

    JsonObject params; // only parameters without a binding; bound ones are recomputed
    for (const auto& [path, value] : m_params) {
        if (!m_bindings.contains(path)) {
            params.insert(path, value);
        }
    }
    if (!params.empty()) {
        json["parameters"] = std::move(params);
    }
    return json;
}

//=============================================================================================================

MnaParamTree MnaParamTree::from_json(const JsonObject& json)
{
    MnaParamTree tree;
    for (const JsonValue& binding : json["bindings"].to_array()) {
        tree.add_binding(MnaParamBinding::from_json(binding.to_object()));
    }
    for (const auto& [path, value] : json["parameters"].to_object()) {
        tree.set_param(path, value);
    }
    return tree;
}
