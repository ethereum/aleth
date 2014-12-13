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
 * @author Christian <c@ethdev.com>
 * @date 2014
 * Formatting functions for errors referencing positions and locations in the source.
 */

#include <libsolidity/SourceReferenceFormatter.h>
#include <libsolidity/Scanner.h>
#include <libsolidity/Exceptions.h>

using namespace std;

namespace dev
{
namespace solidity
{

void SourceReferenceFormatter::printSourceLocation(ostream& _stream,
												   Location const& _location,
												   Scanner const& _scanner)
{
	int startLine;
	int startColumn;
	tie(startLine, startColumn) = _scanner.translatePositionToLineColumn(_location.start);
	_stream << "starting at line " << (startLine + 1) << ", column " << (startColumn + 1) << "\n";
	int endLine;
	int endColumn;
	tie(endLine, endColumn) = _scanner.translatePositionToLineColumn(_location.end);
	if (startLine == endLine)
	{
		_stream << _scanner.getLineAtPosition(_location.start) << endl
				<< string(startColumn, ' ') << "^";
		if (endColumn > startColumn + 2)
			_stream << string(endColumn - startColumn - 2, '-');
		if (endColumn > startColumn + 1)
			_stream << "^";
		_stream << endl;
	}
	else
		_stream << _scanner.getLineAtPosition(_location.start) << endl
				<< string(startColumn, ' ') << "^\n"
				<< "Spanning multiple lines.\n";
}

void SourceReferenceFormatter::printSourcePosition(ostream& _stream,
												   int _position,
												   const Scanner& _scanner)
{
	int line;
	int column;
	tie(line, column) = _scanner.translatePositionToLineColumn(_position);
	_stream << "at line " << (line + 1) << ", column " << (column + 1) << endl
			<< _scanner.getLineAtPosition(_position) << endl
			<< string(column, ' ') << "^" << endl;
}

void SourceReferenceFormatter::printExceptionInformation(ostream& _stream,
														 Exception const& _exception,
														 string const& _name,
														 Scanner const& _scanner)
{
	_stream << _name;
	if (string const* description = boost::get_error_info<errinfo_comment>(_exception))
		_stream << ": " << *description;

	if (int const* position = boost::get_error_info<errinfo_sourcePosition>(_exception))
	{
		_stream << " ";
		printSourcePosition(_stream, *position, _scanner);
	}
	if (Location const* location = boost::get_error_info<errinfo_sourceLocation>(_exception))
	{
		_stream << " ";
		printSourceLocation(_stream, *location, _scanner);
	}
}

}
}
