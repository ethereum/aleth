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
/**
 * @author Lefteris Karapetsas <lefteris@ethdev.com>
 * @date 2015
 * Represents a location in a source file
 */

#pragma once

#include <memory>
#include <string>
#include <ostream>
#include <tuple>

namespace dev
{

/**
 * Representation of an interval of source positions.
 * The interval includes start and excludes end.
 */
struct SourceLocation
{
	SourceLocation(int _start, int _end, std::shared_ptr<std::string const> _sourceName):
		start(_start), end(_end), sourceName(_sourceName) { }
	SourceLocation(): start(-1), end(-1) { }

	SourceLocation(SourceLocation const& _other):
		start(_other.start), end(_other.end), sourceName(_other.sourceName) {}
	SourceLocation& operator=(SourceLocation const& _other) { start = _other.start; end = _other.end; sourceName = _other.sourceName; return *this;}

	bool operator==(SourceLocation const& _other) const { return start == _other.start && end == _other.end;}
	bool operator!=(SourceLocation const& _other) const { return !operator==(_other); }
	inline bool operator<(SourceLocation const& _other) const;
	inline bool contains(SourceLocation const& _other) const;
	inline bool intersects(SourceLocation const& _other) const;

	bool isEmpty() const { return start == -1 && end == -1; }

	int start;
	int end;
	std::shared_ptr<std::string const> sourceName;
};

/// Stream output for Location (used e.g. in boost exceptions).
inline std::ostream& operator<<(std::ostream& _out, SourceLocation const& _location)
{
	if (_location.isEmpty())
		return _out << "NO_LOCATION_SPECIFIED";
	return _out << *_location.sourceName << "[" << _location.start << "," << _location.end << ")";
}

bool SourceLocation::operator<(SourceLocation const& _other) const
{
	if (!sourceName || !_other.sourceName)
		return int(!!sourceName) < int(!!_other.sourceName);
	return make_tuple(*sourceName, start, end) < make_tuple(*_other.sourceName, _other.start, _other.end);
}

bool SourceLocation::contains(SourceLocation const& _other) const
{
	if (isEmpty() || _other.isEmpty() || !sourceName || !_other.sourceName || *sourceName != *_other.sourceName)
		return false;
	return start <= _other.start && _other.end <= end;
}

bool SourceLocation::intersects(SourceLocation const& _other) const
{
	if (isEmpty() || _other.isEmpty() || !sourceName || !_other.sourceName || *sourceName != *_other.sourceName)
		return false;
	return _other.start < end && start < _other.end;
}

}
