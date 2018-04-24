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

#include <ctime>
#include <chrono>
#include <string>
#include "CommonIO.h"
#include "FixedHash.h"
#include "Terminal.h"

#include <boost/log/common.hpp>

namespace dev
{

/// A simple log-output function that prints log messages to stdout.
void debugOut(std::string const& _s);

/// The logging system's current verbosity.
extern int g_logVerbosity;

/// Temporary changes system's verbosity for specific function. Restores the old verbosity when function returns.
/// Not thread-safe, use with caution!
struct VerbosityHolder
{
    VerbosityHolder(int _temporaryValue, bool _force = false): oldLogVerbosity(g_logVerbosity) { if (g_logVerbosity >= 0 || _force) g_logVerbosity = _temporaryValue; }
    ~VerbosityHolder() { g_logVerbosity = oldLogVerbosity; }
    int oldLogVerbosity;
};

class ThreadContext
{
public:
    ThreadContext(std::string const& _info) { push(_info); }
    ~ThreadContext() { pop(); }

    static void push(std::string const& _n);
    static void pop();
    static std::string join(std::string const& _prior);
};

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

/// The default logging channels. Each has an associated verbosity and three-letter prefix (name() ).
/// Channels should inherit from LogChannel and define name() and verbosity.
struct LogChannel { static const char* name(); static const int verbosity = 1; static const bool debug = true; };

enum class LogTag
{
    None,
    Url,
    Error,
    Special
};

class LogOutputStreamBase
{
public:
    LogOutputStreamBase(char const* _id, std::type_info const* _info, unsigned _v, bool _autospacing);

    void comment(std::string const& _t)
    {
        switch (m_logTag)
        {
        case LogTag::Url: m_sstr << EthNavyUnder; break;
        case LogTag::Error: m_sstr << EthRedBold; break;
        case LogTag::Special: m_sstr << EthWhiteBold; break;
        default:;
        }
        m_sstr << _t << EthReset;
        m_logTag = LogTag::None;
    }

    void append(unsigned long _t) { m_sstr << EthBlue << _t << EthReset; }
    void append(long _t) { m_sstr << EthBlue << _t << EthReset; }
    void append(unsigned int _t) { m_sstr << EthBlue << _t << EthReset; }
    void append(int _t) { m_sstr << EthBlue << _t << EthReset; }
    void append(bigint const& _t) { m_sstr << EthNavy << _t << EthReset; }
    void append(u256 const& _t) { m_sstr << EthNavy << _t << EthReset; }
    void append(u160 const& _t) { m_sstr << EthNavy << _t << EthReset; }
    void append(double _t) { m_sstr << EthBlue << _t << EthReset; }
    template <unsigned N> void append(FixedHash<N> const& _t) { m_sstr << EthTeal "#" << _t.abridged() << EthReset; }
    void append(h160 const& _t) { m_sstr << EthRed "@" << _t.abridged() << EthReset; }
    void append(h256 const& _t) { m_sstr << EthCyan "#" << _t.abridged() << EthReset; }
    void append(h512 const& _t) { m_sstr << EthTeal "##" << _t.abridged() << EthReset; }
    void append(std::string const& _t) { m_sstr << EthGreen "\"" + _t + "\"" EthReset; }
    void append(bytes const& _t) { m_sstr << EthYellow "%" << toHex(_t) << EthReset; }
    void append(bytesConstRef _t) { m_sstr << EthYellow "%" << toHex(_t) << EthReset; }
    template <class T> void append(std::vector<T> const& _t)
    {
        m_sstr << EthWhite "[" EthReset;
        int n = 0;
        for (T const& i: _t)
        {
            m_sstr << (n++ ? EthWhite ", " EthReset : "");
            append(i);
        }
        m_sstr << EthWhite "]" EthReset;
    }
    template <class T> void append(std::set<T> const& _t)
    {
        m_sstr << EthYellow "{" EthReset;
        int n = 0;
        for (T const& i: _t)
        {
            m_sstr << (n++ ? EthYellow ", " EthReset : "");
            append(i);
        }
        m_sstr << EthYellow "}" EthReset;
    }
    template <class T, class U> void append(std::map<T, U> const& _t)
    {
        m_sstr << EthLime "{" EthReset;
        int n = 0;
        for (auto const& i: _t)
        {
            m_sstr << (n++ ? EthLime ", " EthReset : "");
            append(i.first);
            m_sstr << (n++ ? EthLime ": " EthReset : "");
            append(i.second);
        }
        m_sstr << EthLime "}" EthReset;
    }
    template <class T> void append(std::unordered_set<T> const& _t)
    {
        m_sstr << EthYellow "{" EthReset;
        int n = 0;
        for (T const& i: _t)
        {
            m_sstr << (n++ ? EthYellow ", " EthReset : "");
            append(i);
        }
        m_sstr << EthYellow "}" EthReset;
    }
    template <class T, class U> void append(std::unordered_map<T, U> const& _t)
    {
        m_sstr << EthLime "{" EthReset;
        int n = 0;
        for (auto const& i: _t)
        {
            m_sstr << (n++ ? EthLime ", " EthReset : "");
            append(i.first);
            m_sstr << (n++ ? EthLime ": " EthReset : "");
            append(i.second);
        }
        m_sstr << EthLime "}" EthReset;
    }
    template <class T, class U> void append(std::pair<T, U> const& _t)
    {
        m_sstr << EthPurple "(" EthReset;
        append(_t.first);
        m_sstr << EthPurple ", " EthReset;
        append(_t.second);
        m_sstr << EthPurple ")" EthReset;
    }
    template <class T> void append(T const& _t)
    {
        m_sstr << toString(_t);
    }

protected:
    bool m_autospacing = false;
    unsigned m_verbosity = 0;
    std::stringstream m_sstr;   ///< The accrued log entry.
    LogTag m_logTag = LogTag::None;
};

/// Logging class, iostream-like, that can be shifted to.
template <class Id, bool _AutoSpacing = true>
class LogOutputStream: LogOutputStreamBase
{
public:
    /// Construct a new object.
    /// If _term is true the the prefix info is terminated with a ']' character; if not it ends only with a '|' character.
    LogOutputStream(): LogOutputStreamBase(Id::name(), &typeid(Id), Id::verbosity, _AutoSpacing) {}

    /// Destructor. Posts the accrued log entry to the output stream.
    ~LogOutputStream() { if (Id::verbosity <= g_logVerbosity) debugOut(m_sstr.str()); }

    LogOutputStream& operator<<(std::string const& _t) { if (Id::verbosity <= g_logVerbosity) { if (_AutoSpacing && m_sstr.str().size() && m_sstr.str().back() != ' ') m_sstr << " "; comment(_t); } return *this; }

    LogOutputStream& operator<<(LogTag _t) { m_logTag = _t; return *this; }

    /// Shift arbitrary data to the log. Spaces will be added between items as required.
    template <class T> LogOutputStream& operator<<(T const& _t) { if (Id::verbosity <= g_logVerbosity) { if (_AutoSpacing && m_sstr.str().size() && m_sstr.str().back() != ' ') m_sstr << " "; append(_t); } return *this; }
};

#define clog(X) dev::LogOutputStream<X>()

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
#define clogSimple(SEVERITY, CHANNEL)                      \
    BOOST_LOG_STREAM_WITH_PARAMS(dev::g_clogLogger::get(), \
        (boost::log::keywords::severity = SEVERITY)(boost::log::keywords::channel = CHANNEL))


// Should be called in every executable
void setupLogging(int _verbosity);

// Simple non-thread-safe logger with fixed severity and channel for each message
using Logger = boost::log::sources::severity_channel_logger<>;
inline Logger createLogger(int _severity, std::string const& _channel)
{
    return Logger(
        boost::log::keywords::severity = _severity, boost::log::keywords::channel = _channel);
}


// Log stream output operators to apply special formatting to the values sent to log
inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, unsigned long _value)
{
    _strm.stream() << EthBlue << _value << EthReset;
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, long _value)
{
    _strm.stream() << EthBlue << _value << EthReset;
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, unsigned int _value)
{
    _strm.stream() << EthBlue << _value << EthReset;
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(boost::log::formatting_ostream& _strm, int _value)
{
    _strm.stream() << EthBlue << _value << EthReset;
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, double _value)
{
    _strm.stream() << EthBlue << _value << EthReset;
    return _strm;
}

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
    _strm << const_cast<bigint const&>(_value);
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
    _strm << const_cast<u256 const&>(_value);
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
    _strm << const_cast<u160 const&>(_value);
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
    _strm << const_cast<FixedHash<N> const&>(_value);
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
    _strm << const_cast<h160 const&>(_value);
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
    _strm << const_cast<h256 const&>(_value);
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
    _strm << const_cast<h512 const&>(_value);
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, std::string const& _value)
{
    _strm.stream() << EthGreen "\"" + _value + "\"" EthReset;
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, bytes const& _value)
{
    _strm.stream() << EthYellow "%" << toHex(_value) << EthReset;
    return _strm;
}

inline boost::log::formatting_ostream& operator<<(
    boost::log::formatting_ostream& _strm, bytesConstRef _value)
{
    _strm.stream() << EthYellow "%" << toHex(_value) << EthReset;
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
    boost::log::formatting_ostream& _strm, std::pair<T, U> const& _value)
{
    _strm.stream() << EthPurple "(" EthReset;
    _strm << _value.first;
    _strm.stream() << EthPurple ", " EthReset;
    _strm << _value.second;
    _strm.stream() << EthPurple ")" EthReset;
    return _strm;
}
}
