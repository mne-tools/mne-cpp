//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_codec.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Private helpers shared by the JSON and CBOR codecs of the MNA structs.
 *
 * MNA structs convert their members with to_json() / from_json() and to_cbor() / from_cbor(). Apart from
 * embedded file data (byte strings) and the project dates (tag 0), the CBOR form of every MNA struct holds the
 * same keys and values as its JSON form, so those structs encode CBOR through their JSON.
 */

#ifndef MNA_CODEC_H
#define MNA_CODEC_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/cbor.h>
#include <mne/base/json.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

/**
 * @param[in] items   The structs to encode.
 * @return Each item's to_json() in a JSON array.
 */
template<class T>
JsonArray to_json_array(const std::vector<T>& items)
{
    JsonArray array;
    array.reserve(items.size());
    for (const T& item : items) {
        array.push_back(item.to_json());
    }
    return array;
}

/**
 * @param[in] value   A JSON array of objects.
 * @return T::from_json() of each object in @p value, if it is an array.
 */
template<class T>
std::vector<T> from_json_array(const JsonValue& value)
{
    std::vector<T> items;
    items.reserve(value.to_array().size());
    for (const JsonValue& item : value.to_array()) {
        items.push_back(T::from_json(item.to_object()));
    }
    return items;
}

/**
 * @param[in] items   The structs to encode.
 * @return Each item's to_cbor() in a CBOR array.
 */
template<class T>
CborArray to_cbor_array(const std::vector<T>& items)
{
    CborArray array;
    array.reserve(items.size());
    for (const T& item : items) {
        array.append(item.to_cbor());
    }
    return array;
}

/**
 * @param[in] value   A CBOR array of maps.
 * @return T::from_cbor() of each map in @p value, if it is an array.
 */
template<class T>
std::vector<T> from_cbor_array(const CborValue& value)
{
    std::vector<T> items;
    items.reserve(value.to_array().size());
    for (const CborValue& item : value.to_array()) {
        items.push_back(T::from_cbor(item.to_map()));
    }
    return items;
}

/**
 * @param[in] item   A struct whose CBOR holds the same keys and values as its JSON.
 * @return The CBOR form of @p item.
 */
template<class T>
CborMap to_cbor_via_json(const T& item)
{
    return CborMap::from_json_object(item.to_json());
}

} // namespace mne::detail

#endif // MNA_CODEC_H
