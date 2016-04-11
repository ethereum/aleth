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


class pipe_t {
public:
    pipe_t();
    ~pipe_t();
    HANDLE r() const;
    HANDLE w() const;
    void close_r();
    void close_w();
    void close();
    void open();
private:
    enum direction_t{ closed, read, write, both };
    direction_t m_direction;
    HANDLE m_r;
    HANDLE m_w;
};


class set_stdhandle_t {
public:
    set_stdhandle_t( DWORD kind, HANDLE handle );
    ~set_stdhandle_t();
private:
    DWORD m_kind;
    HANDLE m_save_handle;
};


class wait_result_t {
public:
    wait_result_t();
    wait_result_t( DWORD wait_result, int objects_count, HANDLE const * objects );

    bool ok();
    bool is_signaled( class event_t & event );
    bool timed_out();
    DWORD error_code();
    char const * error_message();

private:

    HANDLE m_signaled_object;
    bool m_timed_out;
    DWORD m_error_code;
    char const * m_error_message;
};


class event_t {
public:
    event_t();
    ~event_t();
    bool set();
    bool reset();

private:
    HANDLE m_handle;

    friend wait_result_t wait( event_t & e, DWORD timeout );
    friend wait_result_t wait( event_t & e1, event_t & e2, DWORD timeout );
    friend class wait_result_t;
};

wait_result_t wait( HANDLE e, DWORD timeout );
wait_result_t wait( event_t & e, DWORD timeout );
wait_result_t wait( event_t & e1, event_t & e2, DWORD timeout ); // waits for any one of e1, e2

class mutex_t {
public:
    mutex_t();
    ~mutex_t();

private:
    HANDLE m_handle;
    friend class grab_mutex_t;
};

class grab_mutex_t {
public:
    grab_mutex_t( mutex_t & mutex, DWORD timeout );
    ~grab_mutex_t();

    bool ok();
    DWORD error_code();
    char const * error_message();

private:
    HANDLE m_mutex;
    wait_result_t m_wait_result;
};


class thread_buffer_t {
public:
    thread_buffer_t();
    ~thread_buffer_t();

    // those three may be called only before the thread is started
    void set_wait_timeout( DWORD milliseconds );
    void set_thread_termination_timeout( DWORD milliseconds );
    void set_buffer_limit( std::size_t limit );
    void set_read_buffer_size( std::size_t size );
    void set_binary_mode();
    void set_text_mode();

    void start_reader_thread( HANDLE pipe );
    void start_writer_thread( HANDLE pipe);

    void get( exec_stream_t::stream_kind_t kind, char * dst, std::size_t & size, bool & no_more );          // may be called only after start_reader_thread
    void put( char * const src, std::size_t & size, bool & no_more );// may be called only after start_writer_thread
    
    bool stop_thread();
    bool abort_thread();

private:
    enum direction_t { dir_none, dir_read, dir_write };
    direction_t m_direction; // set by start_thread

    buffer_list_t m_buffer_list;
    mutex_t m_mutex; // protecting m_buffer_list

    char const * m_message_prefix; // error occured in the thread, if any
    DWORD m_error_code;                 // they are examined only after the thread has terminated
    char const * m_error_message;  // so setting them anywhere in the thread is safe

    DWORD m_wait_timeout;            // parameters used in thread
    std::size_t m_buffer_limit;           // they are set before the thread is started,
    std::size_t m_read_buffer_size;   // so accessing them anywhere in the thread is safe

    HANDLE m_thread;
    event_t m_want_data; // for synchronisation between get and reader_thread
    event_t m_got_data;   // or between put and writer_thread
    event_t m_stop_thread;
    HANDLE m_pipe;
    DWORD m_thread_termination_timeout;
    bool m_translate_crlf;

    void start_thread( HANDLE pipe, direction_t direction );
    static DWORD WINAPI reader_thread( LPVOID param );
    static DWORD WINAPI writer_thread( LPVOID param );

    void check_error( std::string const & message_prefix, DWORD error_code, std::string const & error_message );
    void note_thread_error( char const * message_prefix, DWORD error_code, char const * error_message );
    bool check_thread_stopped();
};

