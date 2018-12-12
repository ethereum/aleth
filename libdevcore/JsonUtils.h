/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/

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
