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

#include <json_spirit/JsonSpiritHeaders.h>
#include <libdevcore/JsonUtils.h>
#include <boost/algorithm/string/join.hpp>
#include <ostream>
#include <set>
#include <string>

void dev::validateFieldNames(json_spirit::mObject const& _obj, std::set<std::string> const& _allowedFields)
{
	for (auto const& elm: _obj)
		if (_allowedFields.find(elm.first) == _allowedFields.end())
		{
			std::string const comment = "Unknown field in config: " + elm.first;
			std::cerr << comment << "\n";
			BOOST_THROW_EXCEPTION(UnknownField() << errinfo_comment(comment));
		}
}

std::string dev::jsonTypeAsString(json_spirit::Value_type _type)
{
    switch (_type)
    {
    case json_spirit::obj_type:
        return "json Object";
    case json_spirit::array_type:
        return "json Array";
    case json_spirit::str_type:
        return "json String";
    case json_spirit::bool_type:
        return "json Bool";
    case json_spirit::int_type:
        return "json Int";
    case json_spirit::real_type:
        return "json Real";
    case json_spirit::null_type:
        return "json Null";
    default:
        return "json n/a";
    }
}

void dev::requireJsonFields(json_spirit::mObject const& _o, std::string const& _config,
    std::map<std::string, JsonFieldOptions> const& _validationMap)
{
    // check for unexpected fiedls
    for (auto const& field : _o)
    {
        if (!_validationMap.count(field.first))
        {
            std::string const comment =
                "Unexpected field '" + field.first + "' in config: " + _config;
            std::cerr << comment << "\n"
                      << json_spirit::write_string((json_spirit::mValue)_o, true) << "\n";
            BOOST_THROW_EXCEPTION(UnknownField() << errinfo_comment(comment));
        }
    }

    // check field types with validation map
    for (auto const vmap : _validationMap)
    {
        auto const& expectedFieldName = vmap.first;
        auto const& expectedFieldPresence = vmap.second.second;
        // check that all required fields are in the object
        if (!_o.count(expectedFieldName))
        {
            if (expectedFieldPresence == JsonFieldPresence::Required)
            {
                std::string const comment =
                    "Expected field '" + expectedFieldName + "' not found in config: " + _config;
                std::cerr << comment << "\n"
                          << json_spirit::write_string((json_spirit::mValue)_o, true) << "\n";
                BOOST_THROW_EXCEPTION(MissingField() << errinfo_comment(comment));
            }
            else if (expectedFieldPresence == JsonFieldPresence::Optional)
                continue;
        }

        // check that field type is one of allowed field types
        auto const& expectedFieldTypes = vmap.second.first;
        bool matched = expectedFieldTypes.count(_o.at(expectedFieldName).type());
        if (matched == false)
        {
            std::vector<std::string> types;
            for (auto const& type : expectedFieldTypes)
                types.push_back(jsonTypeAsString(type));
            std::string sTypes = boost::algorithm::join(types, " or ");

            std::string const comment = "Field '" + expectedFieldName + "' is expected to be " +
                                        sTypes + ", but is set to " +
                                        jsonTypeAsString(_o.at(expectedFieldName).type()) + " in " +
                                        _config;
            std::cerr << comment << "\n"
                      << json_spirit::write_string((json_spirit::mValue)_o, true) << "\n";
            BOOST_THROW_EXCEPTION(WrongFieldType() << errinfo_comment(comment));
        }
    }
}
