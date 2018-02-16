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
/** @file Log.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "Log.h"

#include <string>
#include <iostream>
#include <thread>
#ifdef __APPLE__
#include <pthread.h>
#endif
#include "Guards.h"

#include <boost/core/null_deleter.hpp>
#include <boost/log/attributes/clock.hpp>
#include <boost/log/attributes/function.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/support/date_time.hpp>

using namespace std;

//⊳⊲◀▶■▣▢□▷◁▧▨▩▲◆◉◈◇◎●◍◌○◼☑☒☎☢☣☰☀♽♥♠✩✭❓✔✓✖✕✘✓✔✅⚒⚡⦸⬌∅⁕«««»»»⚙

namespace dev
{
// Logging
int g_logVerbosity = 5;
mutex x_logOverride;

/// Map of Log Channel types to bool, false forces the channel to be disabled, true forces it to be enabled.
/// If a channel has no entry, then it will output as long as its verbosity (LogChannel::verbosity) is less than
/// or equal to the currently output verbosity (g_logVerbosity).
static map<type_info const*, bool> s_logOverride;

bool isChannelVisible(std::type_info const* _ch, bool _default)
{
    Guard l(x_logOverride);
    if (s_logOverride.count(_ch))
        return s_logOverride[_ch];
    return _default;
}

LogOverrideAux::LogOverrideAux(std::type_info const* _ch, bool _value):
    m_ch(_ch)
{
    Guard l(x_logOverride);
    m_old = s_logOverride.count(_ch) ? (int)s_logOverride[_ch] : c_null;
    s_logOverride[m_ch] = _value;
}

LogOverrideAux::~LogOverrideAux()
{
    Guard l(x_logOverride);
    if (m_old == c_null)
        s_logOverride.erase(m_ch);
    else
        s_logOverride[m_ch] = (bool)m_old;
}

#if defined(_WIN32)
const char* LogChannel::name() { return EthGray "..."; }
const char* LeftChannel::name() { return EthNavy "<--"; }
const char* RightChannel::name() { return EthGreen "-->"; }
const char* WarnChannel::name() { return EthOnRed EthBlackBold "  X"; }
const char* NoteChannel::name() { return EthBlue "  i"; }
const char* DebugChannel::name() { return EthWhite "  D"; }

#else
const char* LogChannel::name() { return EthGray "···"; }
const char* LeftChannel::name() { return EthNavy "◀▬▬"; }
const char* RightChannel::name() { return EthGreen "▬▬▶"; }
const char* WarnChannel::name() { return EthOnRed EthBlackBold "  ✘"; }
const char* NoteChannel::name() { return EthBlue "  ℹ"; }
const char* DebugChannel::name() { return EthWhite "  ◇"; }
#endif

LogOutputStreamBase::LogOutputStreamBase(char const* _id, std::type_info const* _info, unsigned _v, bool _autospacing):
    m_autospacing(_autospacing),
    m_verbosity(_v)
{
    Guard l(x_logOverride);
    auto it = s_logOverride.find(_info);
    if ((it != s_logOverride.end() && it->second == true) || (it == s_logOverride.end() && (int)_v <= g_logVerbosity))
    {
        time_t rawTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        unsigned ms = chrono::duration_cast<chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() % 1000;
        char buf[24];
        if (strftime(buf, 24, "%X", localtime(&rawTime)) == 0)
            buf[0] = '\0'; // empty if case strftime fails
        static char const* c_begin = "  " EthViolet;
        static char const* c_sep1 = EthReset EthBlack "|" EthNavy;
        static char const* c_sep2 = EthReset EthBlack "|" EthTeal;
        static char const* c_end = EthReset "  ";
        m_sstr << _id << c_begin << buf << "." << setw(3) << setfill('0') << ms;
        m_sstr << c_sep1 << getThreadName() << ThreadContext::join(c_sep2) << c_end;
    }
}

/// Associate a name with each thread for nice logging.
struct ThreadLocalLogName
{
    ThreadLocalLogName(std::string const& _name) { m_name.reset(new string(_name)); }
    boost::thread_specific_ptr<std::string> m_name;
};

/// Associate a name with each thread for nice logging.
struct ThreadLocalLogContext
{
    ThreadLocalLogContext() = default;

    void push(std::string const& _name)
    {
        if (!m_contexts.get())
            m_contexts.reset(new vector<string>);
        m_contexts->push_back(_name);
    }

    void pop()
    {
        m_contexts->pop_back();
    }

    string join(string const& _prior)
    {
        string ret;
        if (m_contexts.get())
            for (auto const& i: *m_contexts)
                ret += _prior + i;
        return ret;
    }

    boost::thread_specific_ptr<std::vector<std::string>> m_contexts;
};

ThreadLocalLogContext g_logThreadContext;

ThreadLocalLogName g_logThreadName("main");

void ThreadContext::push(string const& _n)
{
    g_logThreadContext.push(_n);
}

void ThreadContext::pop()
{
    g_logThreadContext.pop();
}

string ThreadContext::join(string const& _prior)
{
    return g_logThreadContext.join(_prior);
}

string getThreadName()
{
#if defined(__GLIBC__) || defined(__APPLE__)
    char buffer[128];
    pthread_getname_np(pthread_self(), buffer, 127);
    buffer[127] = 0;
    return buffer;
#else
    return g_logThreadName.m_name.get() ? *g_logThreadName.m_name.get() : "<unknown>";
#endif
}

void setThreadName(string const& _n)
{
#if defined(__GLIBC__)
    pthread_setname_np(pthread_self(), _n.c_str());
#elif defined(__APPLE__)
    pthread_setname_np(_n.c_str());
#else
    g_logThreadName.m_name.reset(new std::string(_n));
#endif
}

void debugOut(std::string const& _s)
{
    cerr << _s << '\n';
}

BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(threadName, "ThreadName", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

void setupLogging(int _verbosity)
{
    auto sink = boost::make_shared<
        boost::log::sinks::asynchronous_sink<boost::log::sinks::text_ostream_backend>>();

    boost::shared_ptr<std::ostream> stream{&std::cout, boost::null_deleter{}};
    sink->locked_backend()->add_stream(stream);
    sink->set_filter([_verbosity](boost::log::attribute_value_set const& _set) {
        return _set["Severity"].extract<int>() <= _verbosity;
    });

    namespace expr = boost::log::expressions;
    sink->set_formatter(expr::stream
                        << EthViolet << expr::format_date_time(timestamp, "%Y-%m-%d %H:%M:%S")
                        << EthReset " " EthNavy << threadName << EthReset " " << channel << " "
                        << expr::smessage);

    boost::log::core::get()->add_sink(sink);
    boost::log::core::get()->add_global_attribute(
        "ThreadName", boost::log::attributes::make_function(&getThreadName));

    boost::log::core::get()->add_global_attribute(
        "TimeStamp", boost::log::attributes::local_clock());
}

}  // namespace dev
