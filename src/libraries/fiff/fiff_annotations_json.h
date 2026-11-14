//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_annotations_json.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    JSON form of annotation channel names and extras, shared by the FIFF and JSON annotation files (internal).
 */

#ifndef FIFF_ANNOTATIONS_JSON_H
#define FIFF_ANNOTATIONS_JSON_H

#include "fiff_annotations.h"

#include <base/json.h>

#include <string>
#include <vector>

namespace FIFFLIB
{

/**
 * @param[in] names   Channel names.
 *
 * @return The names as a JSON array of strings.
 */
BASELIB::JsonArray names_to_json(const std::vector<std::string>& names);

/**
 * @param[in] array   A JSON array of strings.
 *
 * @return The strings.
 */
std::vector<std::string> names_from_json(const BASELIB::JsonArray& array);

/**
 * @param[in] extras   Annotation extras.
 *
 * @return The extras as a JSON object.
 */
BASELIB::JsonObject extras_to_json(const FiffAnnotation::Extras& extras);

/**
 * @param[in] object   A JSON object.
 *
 * @return Its values as extras; values other than null, bool, number or string are skipped.
 */
FiffAnnotation::Extras extras_from_json(const BASELIB::JsonObject& object);

} // namespace FIFFLIB

#endif // FIFF_ANNOTATIONS_JSON_H
