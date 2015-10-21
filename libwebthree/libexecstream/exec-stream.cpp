/*
Copyright (C)  2004 Artem Khodush

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation 
and/or other materials provided with the distribution. 

3. The name of the author may not be used to endorse or promote products 
derived from this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "exec-stream.h"

#include <list>
#include <vector>
#include <algorithm>
#include <exception>

#ifdef _WIN32

#define NOMINMAX
#include <windows.h>

#define HELPERS_H         "win/exec-stream-helpers.h"
#define HELPERS_CPP     "win/exec-stream-helpers.cpp"
#define IMPL_CPP           "win/exec-stream-impl.cpp"

#else

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h> 

#define HELPERS_H         "posix/exec-stream-helpers.h"
#define HELPERS_CPP     "posix/exec-stream-helpers.cpp"
#define IMPL_CPP           "posix/exec-stream-impl.cpp"

#endif

// helper classes
namespace {

class buffer_list_t {
public:
    struct buffer_t {
        std::size_t size;
        char * data;
    };

    buffer_list_t();
    ~buffer_list_t();

    void get( char * dst, std::size_t & size );
    void get_translate_crlf( char * dst, std::size_t & size );
    void put( char * const src, std::size_t size );
    void put_translate_crlf( char * const src, std::size_t size );
    buffer_t detach();

    bool empty();
    bool full( std::size_t limit ); // limit==0 -> no limit

    void clear();
    
private:
    typedef std::list< buffer_t > buffers_t;
    buffers_t m_buffers;
    std::size_t m_read_offset; // offset into the first buffer
    std::size_t m_total_size;
};

buffer_list_t::buffer_list_t()
{
    m_total_size=0;
    m_read_offset=0;
}

buffer_list_t::~buffer_list_t()
{
    clear();
}

void buffer_list_t::get( char * dst, std::size_t & size )
{
    std::size_t written_size=0;
    while( size>0 && m_total_size>0 ) {
        std::size_t portion_size=std::min( size, m_buffers.front().size-m_read_offset );
        std::char_traits< char >::copy( dst, m_buffers.front().data+m_read_offset, portion_size );
        dst+=portion_size;
        size-=portion_size;
        m_total_size-=portion_size;
        m_read_offset+=portion_size;
        written_size+=portion_size;
        if( m_read_offset==m_buffers.front().size ) {
            delete[] m_buffers.front().data;
            m_buffers.pop_front();
            m_read_offset=0;
        }
    }
    size=written_size;
}

void buffer_list_t::get_translate_crlf( char * dst, std::size_t & size )
{
    std::size_t written_size=0;
    while( written_size!=size && m_total_size>0 ) {
        while( written_size!=size && m_read_offset!=m_buffers.front().size ) {
            char c=m_buffers.front().data[m_read_offset];
            if( c!='\r' ) {  // MISFEATURE: single \r in the buffer will cause end of file
                *dst++=c;
                ++written_size;
            }
            --m_total_size;
            ++m_read_offset;
        }
        if( m_read_offset==m_buffers.front().size ) {
            delete[] m_buffers.front().data;
            m_buffers.pop_front();
            m_read_offset=0;
        }
    }
    size=written_size;
}

void buffer_list_t::put( char * const src, std::size_t size )
{
    buffer_t buffer;
    buffer.data=new char[size];
    buffer.size=size;
    std::char_traits< char >::copy( buffer.data, src, size );
    m_buffers.push_back( buffer );
    m_total_size+=buffer.size;
}

void buffer_list_t::put_translate_crlf( char * const src, std::size_t size )
{
    char const * p=src;
    std::size_t lf_count=0;
    while( p!=src+size ) {
        if( *p=='\n' ) {
            ++lf_count;
        }
        ++p;
    }
    buffer_t buffer;
    buffer.data=new char[size+lf_count];
    buffer.size=size+lf_count;
    p=src;
    char * dst=buffer.data;
    while( p!=src+size ) {
        if( *p=='\n' ) {
            *dst++='\r';
        }
        *dst++=*p;
        ++p;
    }
    m_buffers.push_back( buffer );
    m_total_size+=buffer.size;
}

buffer_list_t::buffer_t buffer_list_t::detach()
{
    buffer_t buffer=m_buffers.front();
    m_buffers.pop_front();
    m_total_size-=buffer.size;
    return buffer;
}

bool buffer_list_t::empty()
{
    return m_total_size==0;
}

bool buffer_list_t::full( std::size_t limit )
{
    return limit!=0 && m_total_size>=limit;
}

void buffer_list_t::clear()
{
    for( buffers_t::iterator i=m_buffers.begin(); i!=m_buffers.end(); ++i ) {
        delete[] i->data;
    }
    m_buffers.clear();
    m_read_offset=0;
    m_total_size=0;
}

}

// platform-dependent helpers

namespace {

#include HELPERS_H
#include HELPERS_CPP

}

// stream buffer class
namespace {

class exec_stream_buffer_t : public std::streambuf {
public:
    exec_stream_buffer_t( exec_stream_t::stream_kind_t kind, thread_buffer_t & thread_buffer );
    virtual ~exec_stream_buffer_t();

    void clear();

protected:
    virtual int_type underflow();
    virtual int_type overflow( int_type c );
    virtual int sync();

private:
    bool send_buffer();
    bool send_char( char c );

    exec_stream_t::stream_kind_t m_kind;
    thread_buffer_t & m_thread_buffer;
    char * m_stream_buffer;
};

const std::size_t STREAM_BUFFER_SIZE=4096;

exec_stream_buffer_t::exec_stream_buffer_t( exec_stream_t::stream_kind_t kind, thread_buffer_t & thread_buffer )
: m_kind( kind ), m_thread_buffer( thread_buffer )
{
    m_stream_buffer=new char[STREAM_BUFFER_SIZE];
    clear();
}

exec_stream_buffer_t::~exec_stream_buffer_t()
{
    delete[] m_stream_buffer;
}

void exec_stream_buffer_t::clear()
{
    if( m_kind==exec_stream_t::s_in ) {
        setp( m_stream_buffer, m_stream_buffer+STREAM_BUFFER_SIZE );
    }else {
        setg( m_stream_buffer, m_stream_buffer+STREAM_BUFFER_SIZE, m_stream_buffer+STREAM_BUFFER_SIZE );
    }
}

exec_stream_buffer_t::int_type exec_stream_buffer_t::underflow()
{
    if( gptr()==egptr() ) {
        std::size_t read_size=STREAM_BUFFER_SIZE;
        bool no_more;
        m_thread_buffer.get( m_kind, m_stream_buffer, read_size, no_more );
        if( no_more || read_size==0 ) { // there is no way for underflow to return something other than eof when 0 bytes are read
            return traits_type::eof();
        }else {
            setg( m_stream_buffer, m_stream_buffer, m_stream_buffer+read_size );
        }
    }
    return traits_type::to_int_type( *eback() );
}

bool exec_stream_buffer_t::send_buffer()
{
    if( pbase()!=pptr() ) {
        std::size_t write_size=pptr()-pbase();
        std::size_t n=write_size;
        bool no_more;
        m_thread_buffer.put( pbase(), n, no_more );
        if( no_more || n!=write_size ) {
            return false;
        }else {
            setp( m_stream_buffer, m_stream_buffer+STREAM_BUFFER_SIZE );
        }
    }
    return true;
}

bool exec_stream_buffer_t::send_char( char c ) 
{
    std::size_t write_size=1;
    bool no_more;
    m_thread_buffer.put( &c, write_size, no_more );
    return write_size==1 && !no_more;
}

exec_stream_buffer_t::int_type exec_stream_buffer_t::overflow( exec_stream_buffer_t::int_type c )
{
    if( !send_buffer() ) {
        return traits_type::eof();
    }
    if( c!=traits_type::eof() ) {
        if( pbase()==epptr() ) {
            if( !send_char( c ) ) {
                return traits_type::eof();
            }
        }else {
            sputc( c );
        }
    }
    return traits_type::not_eof( c );
}

int exec_stream_buffer_t::sync()
{
    if( !send_buffer() ) {
        return -1;
    }
    return 0;
}

// stream  classes

class exec_istream_t : public std::istream {
public:
    exec_istream_t( exec_stream_buffer_t & buf )
    : std::istream( &buf ) {
    }
};


class exec_ostream_t : public std::ostream {
public:
    exec_ostream_t( exec_stream_buffer_t & buf )
    : std::ostream( &buf ){
    }
};

}

// platform-dependent implementation
#include IMPL_CPP


//platform-independent exec_stream_t member functions
exec_stream_t::exec_stream_t()
{
    m_impl=new impl_t;
    exceptions( true );
}

exec_stream_t::exec_stream_t( std::string const & program, std::string const & arguments )
{
    m_impl=new impl_t;
    exceptions( true );
    start( program, arguments );
}

void exec_stream_t::new_impl()
{
    m_impl=new impl_t;
}

exec_stream_t::~exec_stream_t()
{
    try {
        close();
    }catch( ... ) {
    }
    delete m_impl;
}

std::ostream & exec_stream_t::in()
{
    return m_impl->m_in;
}

std::istream & exec_stream_t::out()
{
    return m_impl->m_out;
}

std::istream & exec_stream_t::err()
{
    return m_impl->m_err;
}

void exec_stream_t::exceptions( bool enable ) 
{
    if( enable ) {
        // getline sets failbit on eof, so we should enable badbit and badbit _only_ to propagate our exceptions through iostream code.
        m_impl->m_in.exceptions( std::ios_base::badbit );
        m_impl->m_out.exceptions( std::ios_base::badbit );
        m_impl->m_err.exceptions( std::ios_base::badbit );
    }else {
        m_impl->m_in.exceptions( std::ios_base::goodbit );
        m_impl->m_out.exceptions( std::ios_base::goodbit );
        m_impl->m_err.exceptions( std::ios_base::goodbit );
    }
}

// exec_stream_t::error_t
namespace {
    
std::string int2str( unsigned long i, int base, std::size_t width ) 
{
    std::string s;
    s.reserve(4);
    while( i!=0 ) {
        s="0123456789abcdef"[i%base]+s;
        i/=base;
    }
    if( width!=0 ) {
        while( s.size()<width ) {
            s="0"+s;
        }
    }
    return s;
}

}

exec_stream_t::error_t::error_t()
{
}

exec_stream_t::error_t::error_t( std::string const & msg )
{
    m_msg=msg;
}

exec_stream_t::error_t::error_t( std::string const & msg, error_code_t code )
{
    compose( msg, code );
}

exec_stream_t::error_t::~error_t() throw()
{
}

char const * exec_stream_t::error_t::what() const throw()
{
    return m_msg.c_str();
}


void exec_stream_t::error_t::compose( std::string const & msg, error_code_t code )
{
    m_msg=msg;
    m_msg+="\n[code 0x"+int2str( code, 16, 4 )+" ("+int2str( code, 10, 0 )+")]";
}
