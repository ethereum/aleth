// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#pragma once

#include <libdevcore/Exceptions.h>
#include <json_spirit/JsonSpiritHeaders.h>
#include <set>
#include <string>

namespace dev
{
// Throws UnknownField() if _obj contains field names not listed in _allowedFields.
void validateFieldNames(json_spirit::mObject const& _obj, std::set<std::string> const& _allowedFields);

// Converts json value type to string
std::string jsonTypeAsString(json_spirit::Value_type _type);

enum class JsonFieldPresence
{
    Required,
    Optional
};
using JsonTypeSet = std::set<json_spirit::Value_type>;
using JsonFieldOptions = std::pair<JsonTypeSet, JsonFieldPresence>;
/// Check the json object with validation map that reuires certain field of certain type to be
/// present in json
/**
  @param _o a json object to check
  @param _configName a string with json object name. Will apper in error message.
  @param _validationMap a map with json objects that would be checked. "objName" -> {js::str_type,
  jsonField::Required}
*/
void requireJsonFields(json_spirit::mObject const& _o, std::string const& _configName,
    std::map<std::string, JsonFieldOptions> const& _validationMap);
}
