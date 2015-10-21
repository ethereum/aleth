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

class os_error_t : public exec_stream_t::error_t {
public:
    os_error_t( std::string const & msg );
    os_error_t( std::string const & msg,  exec_stream_t::error_code_t code );
private:
    void compose( std::string const & msg, exec_stream_t::error_code_t code );
};


template< class T > class buf_t {
public:
    typedef T data_t;

    buf_t()
    {
        m_buf=0;
        m_size=0;
    }
    
    ~buf_t()
    {
        delete [] m_buf;
    }

    data_t * new_data( std::size_t size )
    {
        m_buf=new T[size];
        m_size=size;
        return m_buf;
    }

    void append_data( data_t const * data, std::size_t size )
    {
        buf_t new_buf;
        new_buf.new_data( m_size+size );
        std::char_traits< data_t >::copy( new_buf.m_buf, m_buf, m_size );
        std::char_traits< data_t >::copy( new_buf.m_buf+m_size, data, size );
        std::swap( this->m_buf, new_buf.m_buf );
        std::swap( this->m_size, new_buf.m_size );
    }
        
    data_t * data()
    {
        return m_buf;
    }

    std::size_t size()
    {
        return m_size;
    }
    
private:
    buf_t( buf_t const & );
    buf_t & operator=( buf_t const & );

    data_t * m_buf;
    std::size_t m_size;
};


class pipe_t {
public:
    pipe_t();
    ~pipe_t();
    int r() const;
    int w() const;
    void close_r();
    void close_w();
    void close();
    void open();
private:
    enum direction_t{ closed, read, write, both };
    direction_t m_direction;
    int m_fds[2];
};


class mutex_t {
public:
    mutex_t();
    ~mutex_t();
    
private:
    pthread_mutex_t m_mutex;

    friend class event_t;
    friend class grab_mutex_t;
};


class grab_mutex_t {
public:
    grab_mutex_t( mutex_t & mutex, class mutex_registrator_t * mutex_registrator );
    ~grab_mutex_t();
    
    int release();
    bool ok();
    int error_code();
    
private:
    pthread_mutex_t * m_mutex;
    int m_error_code;
    bool m_grabbed;
    class mutex_registrator_t * m_mutex_registrator;

    friend class mutex_registrator_t;
};

class mutex_registrator_t {
public:
    ~mutex_registrator_t();
    void add( grab_mutex_t * g );
    void remove( grab_mutex_t * g );
    void release_all();
private:
    typedef std::list< grab_mutex_t * > mutexes_t;
    mutexes_t m_mutexes;
};


class wait_result_t {
public:
    wait_result_t( unsigned signaled_state, int error_code, bool timed_out );

    bool ok();
    bool is_signaled( int state );
    int error_code();
    bool timed_out();

private:
    unsigned m_signaled_state;
    int m_error_code;
    bool m_timed_out;
};


class event_t {
public:
    event_t();
    ~event_t();

    int set( unsigned bits, mutex_registrator_t * mutex_registrator );
    int reset( unsigned bits, mutex_registrator_t * mutex_registrator );
    
    wait_result_t wait( unsigned any_bits, unsigned long timeout, mutex_registrator_t * mutex_registrator );

private:
    mutex_t m_mutex;
    pthread_cond_t m_cond;
    unsigned volatile m_state;    
};


class thread_buffer_t {
public:
    thread_buffer_t( pipe_t & in_pipe, pipe_t & out_pipe, pipe_t & err_pipe, std::ostream & in );
    ~thread_buffer_t();

    void set_wait_timeout( int stream_kind, unsigned long milliseconds );
    void set_buffer_limit( int stream_kind, std::size_t limit );
    void set_read_buffer_size( int stream_kind, std::size_t size );

    void start();

    void get( exec_stream_t::stream_kind_t kind, char * dst, std::size_t & size, bool & no_more );
    void put( char * src, std::size_t & size, bool & no_more );

    void close_in();
    bool stop_thread();
    bool abort_thread();

private:
    static void * thread_func( void * param );

    pthread_t m_thread;    
    mutex_t m_mutex; // protecting m_in_buffer, m_out_buffer, m_err_buffer
    
    buffer_list_t m_in_buffer;
    buffer_list_t m_out_buffer;
    buffer_list_t m_err_buffer;

    event_t m_thread_control;  // s_in : in got_data;  s_out: out want data; s_err: err want data; s_child: stop thread
    event_t m_thread_responce; // s_in : in want data; s_out: out got data;  s_err: err got data;  s_child: thread stopped

    char const * m_error_prefix;
    int m_error_code;

    bool m_thread_started; // set in start(), checked in set_xxx(), get() and put()
    bool m_in_closed; // set in close_in(), checked in put()
    
    pipe_t & m_in_pipe;
    pipe_t & m_out_pipe;
    pipe_t & m_err_pipe;
    
    unsigned long m_in_wait_timeout;
    unsigned long m_out_wait_timeout;
    unsigned long m_err_wait_timeout;
    
    unsigned long m_thread_termination_timeout;
    
    std::size_t m_in_buffer_limit;
    std::size_t m_out_buffer_limit;
    std::size_t m_err_buffer_limit;

    std::size_t m_out_read_buffer_size;
    std::size_t m_err_read_buffer_size;

    // workaround for not-quite-conformant libstdc++ (see put())
    std::ostream & m_in;
    bool m_in_bad;
};
