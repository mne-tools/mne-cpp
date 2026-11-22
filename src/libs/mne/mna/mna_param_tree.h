//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_param_tree.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Hierarchical parameter store with formula-driven dynamic bindings shared across an MNA graph.
 *
 * @ref mne::MnaParamTree is the second authoritative source of node
 * attribute values — the first being a node's own @c attributes
 * map. Where the latter holds the literal value typed by the user
 * for a single operation, the param tree expresses values that are
 * either reused across many nodes (e.g. @c subject/fsdir) or
 * computed from other values via @ref mne::MnaParamBinding formulas
 * resolved at execution time.
 *
 * Paths are slash-separated and conventionally namespaced by node
 * id (@c bandpass_01/lowcut, @c bandpass_01/highcut), letting the
 * executor merge static defaults from the op schema, static
 * overrides from the project, and dynamically bound values into
 * the final @c resolved_attributes captured in the
 * @ref mne::MnaProvenance record. @ref mne::MnaParamTree::evaluate "evaluate" walks all registered
 * bindings whose triggers have fired and returns the list of paths
 * whose values changed, which the executor uses to mark downstream
 * nodes dirty.
 */

#ifndef MNA_PARAM_TREE_H
#define MNA_PARAM_TREE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_param_binding.h"

#include <mne/base/json.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Hierarchical parameter store keyed by "nodeId/attrKey" paths, with bindings that recompute parameters from
 * formulas.
 *
 * Formulas combine numbers, true/false, 'text', names (a parameter, else a result), ref('path'), the functions
 * clamp(v, lo, hi), scale(v, f), threshold(v, t, above, below) and lerp(a, b, t), the operators + - * / with
 * parentheses, and one comparison (>= <= == != > <). Names may contain '.', ':' and '/', so put spaces around a
 * division that follows a name.
 *
 * @brief Path-keyed parameter store with formula-driven dynamic bindings.
 *
 * @snippet ex_mna/main.cpp mna_param_tree_binding
 */
class MNASHARED_EXPORT MnaParamTree
{
public:
    /** @param[in] path   "nodeId/attrKey". @param[in] value   The value. */
    void set_param(std::string_view path, JsonValue value);

    /** @param[in] path   "nodeId/attrKey". @return The value, or an undefined value if there is none. */
    const JsonValue& param(std::string_view path) const;

    /** @param[in] path   "nodeId/attrKey". @return Whether the parameter is set. */
    bool has_param(std::string_view path) const;

    /** @return The paths of all parameters in ascending order. */
    StringList all_paths() const;

    /** @param[in] binding   A binding; replaces the binding of the same target. */
    void add_binding(const MnaParamBinding& binding);

    /** @param[in] target_path   Target of the binding to remove. */
    void remove_binding(std::string_view target_path);

    /** @return The bindings, ordered by target path. */
    std::vector<MnaParamBinding> bindings() const;

    /** @param[in] target_path   A parameter path. @return Whether a binding controls it. */
    bool has_binding(std::string_view target_path) const;

    //=========================================================================================================
    /**
     * Evaluates every binding that is not "manual" and stores the new values.
     *
     * @param[in] results   Values that names and ref() fall back to, e.g. "nodeId::portName" results.
     *
     * @return The paths whose value changed.
     */
    StringList evaluate(const JsonObject& results);

    //=========================================================================================================
    /**
     * @param[in] expression   A formula.
     * @param[in] results      Values that names and ref() fall back to after the parameters.
     *
     * @return The value of @p expression, or an undefined value if it cannot be evaluated (unknown name or
     *         function, syntax error, division by zero).
     */
    JsonValue evaluate_expression(std::string_view expression, const JsonObject& results) const;

    /** @return The bindings and the parameters without a binding as a JSON object. */
    JsonObject to_json() const;

    /** @param[in] json   A tree as written by to_json(). @return The tree. */
    static MnaParamTree from_json(const JsonObject& json);

private:
    std::map<std::string, JsonValue, std::less<>> m_params;
    std::map<std::string, MnaParamBinding, std::less<>> m_bindings;
};

} // namespace mne

#endif // MNA_PARAM_TREE_H
