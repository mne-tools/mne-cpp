//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_param_binding.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    JSON and CBOR codecs for @ref MnaParamBinding — target path, expression, trigger, period and dependency list.
 *
 * The implementation is a straightforward field-for-field codec
 * pair that keeps the expression body unchanged so future
 * evaluators with extended grammar can still consume bindings
 * written by this build. The @c dependencies list is written
 * explicitly (rather than re-derived from @c expression) so the
 * @ref MnaParamTree triggering logic can avoid parsing formulas
 * at every check, and unknown fields round-trip through the
 * standard @c extras pattern.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_param_binding.h"

#include "mna_codec.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

JsonObject MnaParamBinding::to_json() const
{
    JsonObject json;
    json["target"] = target_path;
    json["expression"] = expression;
    json["trigger"] = trigger;
    if (trigger == "periodic" && period_ms > 0) {
        json["period_ms"] = period_ms;
    }
    if (!dependencies.empty()) {
        json["dependencies"] = JsonArray::from_string_list(dependencies);
    }
    return json;
}

//=============================================================================================================

MnaParamBinding MnaParamBinding::from_json(const JsonObject& json)
{
    MnaParamBinding b;
    b.target_path = json["target"].to_std_string();
    b.expression = json["expression"].to_std_string();
    b.trigger = json["trigger"].to_std_string();
    b.period_ms = json["period_ms"].to_int(0);
    b.dependencies = json["dependencies"].to_array().to_string_list();
    return b;
}

//=============================================================================================================

CborMap MnaParamBinding::to_cbor() const
{
    return detail::to_cbor_via_json(*this);
}

//=============================================================================================================

MnaParamBinding MnaParamBinding::from_cbor(const CborMap& cbor)
{
    return from_json(cbor.to_json_object());
}
