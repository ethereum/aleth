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

#include "Common.h"
#include "Exceptions.h"
#include "Log.h"

#if defined(_WIN32)
#include <windows.h>
#endif

#include <aleth/buildinfo.h>

using namespace std;

namespace dev
{
char const* Version = aleth_get_buildinfo()->project_version;
bytes const NullBytes;
std::string const EmptyString;

void InvariantChecker::checkInvariants(HasInvariants const* _this, char const* _fn, char const* _file, int _line, bool _pre)
{
    if (!_this->invariants())
    {
        cwarn << (_pre ? "Pre" : "Post") << "invariant failed in" << _fn << "at" << _file << ":" << _line;
        ::boost::exception_detail::throw_exception_(FailedInvariant(), _fn, _file, _line);
    }
}

TimerHelper::~TimerHelper()
{
    auto e = std::chrono::high_resolution_clock::now() - m_t;
    if (!m_ms || e > chrono::milliseconds(m_ms))
        clog(VerbosityDebug, "timer")
            << m_id << " " << chrono::duration_cast<chrono::milliseconds>(e).count() << " ms";
}

int64_t utcTime()
{
    // TODO: Fix if possible to not use time(0) and merge only after testing in all platforms
    // time_t t = time(0);
    // return mktime(gmtime(&t));
    return time(0);
}

string inUnits(bigint const& _b, strings const& _units)
{
    ostringstream ret;
    u256 b;
    if (_b < 0)
    {
        ret << "-";
        b = (u256)-_b;
    }
    else
        b = (u256)_b;

    u256 biggest = 1;
    for (unsigned i = _units.size() - 1; !!i; --i)
        biggest *= 1000;

    if (b > biggest * 1000)
    {
        ret << (b / biggest) << " " << _units.back();
        return ret.str();
    }
    ret << setprecision(3);

    u256 unit = biggest;
    for (auto it = _units.rbegin(); it != _units.rend(); ++it)
    {
        auto i = *it;
        if (i != _units.front() && b >= unit)
        {
            ret << (double(b / (unit / 1000)) / 1000.0) << " " << i;
            return ret.str();
        }
        else
            unit /= 1000;
    }
    ret << b << " " << _units.front();
    return ret.str();
}

/*
The equivalent of setlocale(LC_ALL, “C”) is called before any user code is run.
If the user has an invalid environment setting then it is possible for the call
to set locale to fail, so there are only two possible actions, the first is to
throw a runtime exception and cause the program to quit (default behaviour),
or the second is to modify the environment to something sensible (least
surprising behaviour).

The follow code produces the least surprising behaviour. It will use the user
specified default locale if it is valid, and if not then it will modify the
environment the process is running in to use a sensible default. This also means
that users do not need to install language packs for their OS.
*/
void setDefaultOrCLocale()
{
#if __unix__
    if (!std::setlocale(LC_ALL, ""))
    {
        setenv("LC_ALL", "C", 1);
    }
#endif

#if defined(_WIN32)
    // Change the code page from the default OEM code page (437) so that UTF-8 characters are
    // displayed correctly in the console.
    SetConsoleOutputCP(CP_UTF8);
#endif
}

bool ExitHandler::s_shouldExit = false;

bool isTrue(std::string const& _m)
{
    return _m == "on" || _m == "yes" || _m == "true" || _m == "1";
}

bool isFalse(std::string const& _m)
{
    return _m == "off" || _m == "no" || _m == "false" || _m == "0";
}

}  // namespace dev
