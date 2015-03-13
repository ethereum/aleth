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
/** @file CommonTime.h
 * @author Lefteris Karapetsas <lefteris@ethdev.com>
 * @date 2015
 *
 * Helper functions having to do with time
 */
#pragma once
#include <chrono>

struct tm;

namespace dev
{

/// Cross platform wrapper for thread safe gmtime()
/// @param _timeInput        A chrono time_point to convert to UTC
/// @param _result           Pass in a struct tm inside which to return the resulting UTC timestamp
/// @return                  A pointer to the passed in @a _result if succesfull or NULL otherwise
tm *timeToUTC(std::chrono::system_clock::time_point const& _timeInput, struct tm *_result);

}
