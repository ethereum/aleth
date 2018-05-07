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
/** @file Log.h
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 *
 * The logging subsystem.
 */

#pragma once

#include "CommonIO.h"
#include "FixedHash.h"
#include "Terminal.h"
#include <string>
#include <vector>

#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>

namespace dev
{

/// Set the current thread's log name.
///
/// It appears that there is not currently any cross-platform way of setting
/// thread names either in Boost or in the C++11 runtime libraries.   What is
/// more, the API for 'pthread_setname_np' is not even consistent across
/// platforms which implement it.
///
/// A proposal to add such functionality on the Boost mailing list, which
/// I assume never happened, but which I should follow-up and ask about.
/// http://boost.2283326.n4.nabble.com/Adding-an-option-to-set-the-name-of-a-boost-thread-td4638283.html
///
/// man page for 'pthread_setname_np', including this crucial snippet of
/// information ... "These functions are nonstandard GNU extensions."
/// http://man7.org/linux/man-pages/man3/pthread_setname_np.3.html
///
/// Stack Overflow "Can I set the name of a thread in pthreads / linux?"
/// which includes useful information on the minor API differences between
/// Linux, BSD and OS X.
/// http://stackoverflow.com/questions/2369738/can-i-set-the-name-of-a-thread-in-pthreads-linux/7989973#7989973
///
/// musl mailng list posting "pthread set name on MIPs" which includes the
/// information that musl doesn't currently implement 'pthread_setname_np'
/// https://marc.info/?l=musl&m=146171729013062&w=1
void setThreadName(std::string const& _n);

/// Set the current thread's log name.
std::string getThreadName();

#define LOG BOOST_LOG

// Simple cout-like stream objects for accessing common log channels.
// Thread-safe
BOOST_LOG_INLINE_GLOBAL_LOGGER_CTOR_ARGS(g_debugLogger,
    boost::log::sources::severity_channel_logger_mt<>,
    (boost::log::keywords::severity = 0)(boost::log::keywords::channel = "debug"))
#define cdebug LOG(dev::g_debugLogger::get())

BOOST_LOG_INLINE_GLOBAL_LOGGER_CTOR_ARGS(g_noteLogger,
    boost::log::sources::severity_channel_logger_mt<>,
    (boost::log::keywords::severity = 1)(boost::log::keywords::channel = "note"))
#define cnote LOG(dev::g_noteLogger::get())

BOOST_LOG_INLINE_GLOBAL_LOGGER_CTOR_ARGS(g_warnLogger,
    boost::log::sources::severity_channel_logger_mt<>,
    (boost::log::keywords::severity = 0)(boost::log::keywords::channel = "warn"))
#define cwarn LOG(dev::g_warnLogger::get())

BOOST_LOG_INLINE_GLOBAL_LOGGER_CTOR_ARGS(g_traceLogger,
    boost::log::sources::severity_channel_logger_mt<>,
    (boost::log::keywords::severity = 4)(boost::log::keywords::channel = "trace"))
#define ctrace LOG(dev::g_traceLogger::get())

// Simple macro to log to any channel a message without creating a logger object
// e.g. clog(0, "channel") << "message";
// Thread-safe
BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(
    g_clogLogger, boost::log::sources::severity_channel_logger_mt<>);
#define clog(SEVERITY, CHANNEL)                            \
    BOOST_LOG_STREAM_WITH_PARAMS(dev::g_clogLogger::get(), \
        (boost::log::keywords::severity = SEVERITY)(boost::log::keywords::channel = CHANNEL))


// Should be called in every executable
void setupLogging(int _verbosity, std::vector<std::string> const& _includeChannels = {},
    std::vector<std::string> const& _excludeChannels = {});

// Simple non-thread-safe logger with fixed severity and channel for each message
using Logger = boost::log::sources::severity_channel_logger<>;
inline Logger createLogger(int _severity, std::string const& _channel)
{
    return Logger(
        boost::log::keywords::severity = _severity, boost::log::keywords::channel = _channel);
}

// Adds the context string to all log messages in the scope
#define LOG_SCOPED_CONTEXT(context) \
    BOOST_LOG_SCOPED_THREAD_ATTR("Context", boost::log::attributes::constant<std::string>(context));


// Below overloads for both const and non-const references are needed, because without overload for
// non-const reference generic operator<<(formatting_ostream& _strm, T& _value) will be preferred by
// overload resolution.
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, bigint const& _value)
{
    _strm.stream() << EthNavy << _value << EthReset;
    return _strm;
}
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, bigint& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, u256 const& _value)
{
    _strm.stream() << EthNavy << _value << EthReset;
    return _strm;
}
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, u256& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, u160 const& _value)
{
    _strm.stream() << EthNavy << _value << EthReset;
    return _strm;
}
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, u160& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

template <unsigned N>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, FixedHash<N> const& _value)
{
    _strm.stream() << EthTeal "#" << _value.abridged() << EthReset;
    return _strm;
}
template <unsigned N>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, FixedHash<N>& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, h160 const& _value)
{
    _strm.stream() << EthRed "@" << _value.abridged() << EthReset;
    return _strm;
}
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, h160& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, h256 const& _value)
{
    _strm.stream() << EthCyan "#" << _value.abridged() << EthReset;
    return _strm;
}
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, h256& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, h512 const& _value)
{
    _strm.stream() << EthTeal "##" << _value.abridged() << EthReset;
    return _strm;
}
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, h512& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, bytesConstRef _value)
{
    _strm.stream() << EthYellow "%" << toHex(_value) << EthReset;
    return _strm;
}
}  // namespace dev

// Overloads for types of std namespace can't be defined in dev namespace, because they won't be
// found due to Argument-Dependent Lookup Placing anything into std is not allowed, but we can put
// them into boost::log
namespace boost
{
namespace log
{
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, dev::bytes const& _value)
{
    _strm.stream() << EthYellow "%" << dev::toHex(_value) << EthReset;
    return _strm;
}
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, dev::bytes& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

template <typename T>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::vector<T> const& _value)
{
    _strm.stream() << EthWhite "[" EthReset;
    int n = 0;
    for (T const& i : _value)
    {
        _strm.stream() << (n++ ? EthWhite ", " EthReset : "");
        _strm << i;
    }
    _strm.stream() << EthWhite "]" EthReset;
    return _strm;
}
template <typename T>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::vector<T>& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

template <typename T>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::set<T> const& _value)
{
    _strm.stream() << EthYellow "{" EthReset;
    int n = 0;
    for (T const& i : _value)
    {
        _strm.stream() << (n++ ? EthYellow ", " EthReset : "");
        _strm << i;
    }
    _strm.stream() << EthYellow "}" EthReset;
    return _strm;
}
template <typename T>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::set<T>& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

template <typename T>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::unordered_set<T> const& _value)
{
    _strm.stream() << EthYellow "{" EthReset;
    int n = 0;
    for (T const& i : _value)
    {
        _strm.stream() << (n++ ? EthYellow ", " EthReset : "");
        _strm << i;
    }
    _strm.stream() << EthYellow "}" EthReset;
    return _strm;
}
template <typename T>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::unordered_set<T>& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

template <typename T, typename U>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::map<T, U> const& _value)
{
    _strm.stream() << EthLime "{" EthReset;
    int n = 0;
    for (auto const& i : _value)
    {
        _strm << (n++ ? EthLime ", " EthReset : "");
        _strm << i.first;
        _strm << (n++ ? EthLime ": " EthReset : "");
        _strm << i.second;
    }
    _strm.stream() << EthLime "}" EthReset;
    return _strm;
}
template <typename T, typename U>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::map<T, U>& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

template <typename T, typename U>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::unordered_map<T, U> const& _value)
{
    _strm << EthLime "{" EthReset;
    int n = 0;
    for (auto const& i : _value)
    {
        _strm.stream() << (n++ ? EthLime ", " EthReset : "");
        _strm << i.first;
        _strm.stream() << (n++ ? EthLime ": " EthReset : "");
        _strm << i.second;
    }
    _strm << EthLime "}" EthReset;
    return _strm;
}
template <typename T, typename U>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::unordered_map<T, U>& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}

template <typename T, typename U>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::pair<T, U> const& _value)
{
    _strm.stream() << EthPurple "(" EthReset;
    _strm << _value.first;
    _strm.stream() << EthPurple ", " EthReset;
    _strm << _value.second;
    _strm.stream() << EthPurple ")" EthReset;
    return _strm;
}
template <typename T, typename U>
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::pair<T, U>& _value)
{
    auto const& constValue = _value;
    _strm << constValue;
    return _strm;
}
}
}  // namespace boost