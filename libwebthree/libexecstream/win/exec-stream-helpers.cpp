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

// os_error_t
os_error_t::os_error_t( std::string const & msg )
{
    compose( msg, GetLastError() );
}

os_error_t::os_error_t( std::string const & msg, exec_stream_t::error_code_t code )
{
    compose( msg, code );
}

void os_error_t::compose( std::string const & msg, exec_stream_t::error_code_t code )
{
    std::string s( msg );
    s+='\n';
    LPVOID buf;
    if( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
        0, 
        code, 
        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), 
        (LPTSTR) &buf,
        0,
        0 
        )==0 ) {
        s+="[unable to retrieve error description]";
    }else {
        // FormatMessage may return \n-terminated string
        LPTSTR str_buf=(LPTSTR)buf;
        std::size_t buf_len=strlen( str_buf );
        while( buf_len>0 && str_buf[buf_len-1]=='\n' ) {
            --buf_len;
            str_buf[buf_len]=0;
        }
        s+=(LPTSTR)buf;
        LocalFree( buf );
    }
    exec_stream_t::error_t::compose( s, code );
}

// pipe_t
pipe_t::pipe_t() 
: m_direction( closed ), m_r( INVALID_HANDLE_VALUE ), m_w( INVALID_HANDLE_VALUE ) 
{
    open();
}

pipe_t::~pipe_t() 
{
    close();
}

void pipe_t::close_r() 
{
    if( m_direction==both || m_direction==read ) {
        if( !CloseHandle( m_r ) ) {
            throw os_error_t( "pipe_t::close_r: CloseHandle failed" );
        }
        m_direction= m_direction==both ? write : closed;
    }
}

void pipe_t::close_w() 
{
    if( m_direction==both || m_direction==write ) {
        if( !CloseHandle( m_w ) ) {
            throw os_error_t( "pipe_t::close_w: CloseHandle failed" );
        }
        m_direction= m_direction==both ? read : closed;
    }
}

void pipe_t::close() 
{
    close_r();
    close_w();
}

void pipe_t::open() 
{
    close();
    SECURITY_ATTRIBUTES sa;
    sa.nLength=sizeof( sa );
    sa.bInheritHandle=true;
    sa.lpSecurityDescriptor=0;
    if( !CreatePipe( &m_r, &m_w, &sa, 0 ) )
        throw os_error_t( "pipe_t::pipe_t: CreatePipe failed" );
    m_direction=both;
}

HANDLE pipe_t::r() const 
{
    return m_r;
}

HANDLE pipe_t::w() const 
{
    return m_w;
}

// set_stdhandle_t
set_stdhandle_t::set_stdhandle_t( DWORD kind, HANDLE handle )
: m_kind( kind ), m_save_handle( GetStdHandle( kind ) ) 
{
    if( m_save_handle==INVALID_HANDLE_VALUE )
        throw os_error_t( "set_stdhandle_t::set_stdhandle_t: GetStdHandle() failed" );
    if( !SetStdHandle( kind, handle ) )
        throw os_error_t( "set_stdhandle_t::set_stdhandle_t: SetStdHandle() failed" );
}

set_stdhandle_t::~set_stdhandle_t() 
{
    SetStdHandle( m_kind, m_save_handle );
}

//wait_result_t
wait_result_t::wait_result_t()
{
    m_signaled_object=INVALID_HANDLE_VALUE;
    m_timed_out=false;
    m_error_code=ERROR_SUCCESS;
    m_error_message="";
}

wait_result_t::wait_result_t( DWORD wait_result, int objects_count, HANDLE const * objects )
{
    m_signaled_object=INVALID_HANDLE_VALUE;
    m_timed_out=false;
    m_error_code=ERROR_SUCCESS;
    m_error_message="";
    if( wait_result>=WAIT_OBJECT_0 && wait_result<WAIT_OBJECT_0+objects_count ) {
        m_signaled_object=objects[wait_result-WAIT_OBJECT_0];
    }else if( wait_result>=WAIT_ABANDONED_0 && wait_result<WAIT_ABANDONED_0+objects_count ) {
        m_error_message="wait_result_t: one of the wait objects was abandoned";
    }else if( wait_result==WAIT_TIMEOUT ) {
        m_timed_out=true;
        m_error_message="wait_result_t: timeout elapsed";
    }else if( wait_result==WAIT_FAILED ) {
        m_error_code=GetLastError();
    }else {
        m_error_message="wait_result_t: weird error: unrecognised WaitForMultipleObjects return value";
        m_error_code=wait_result;
    }
}

bool wait_result_t::ok()
{
    return m_error_code==ERROR_SUCCESS && m_error_message[0]==0;
}

bool wait_result_t::is_signaled( event_t & event )
{
    return m_signaled_object==event.m_handle;
}

bool wait_result_t::timed_out()
{
    return m_timed_out;
}

DWORD wait_result_t::error_code()
{
    return m_error_code;
}

char const * wait_result_t::error_message()
{
    return m_error_message;
}

// event_t
event_t::event_t()
{
    m_handle=CreateEvent( 0, TRUE, FALSE, 0 );
    if( m_handle==0 ) {
        throw os_error_t( "event_t::event_t: create event failed" );
    }
}

event_t::~event_t()
{
    CloseHandle( m_handle );
}

bool event_t::set()
{
    return SetEvent( m_handle )!=0;
}

bool event_t::reset()
{
    return ResetEvent( m_handle )!=0;
}

// wait functions
wait_result_t wait( HANDLE h, DWORD timeout )
{
    return wait_result_t( WaitForSingleObject( h, timeout ), 1, &h );
}

wait_result_t wait( event_t & e, DWORD timeout )
{
    return wait_result_t( WaitForSingleObject( e.m_handle, timeout ), 1, &e.m_handle );
}

wait_result_t wait( event_t & e1, event_t & e2, DWORD timeout )
{
    HANDLE h[2];
    h[0]=e1.m_handle;
    h[1]=e2.m_handle;
    return wait_result_t( WaitForMultipleObjects( 2, h, FALSE, timeout ), 2, h );
}

// mutex_t
mutex_t::mutex_t()
{
    m_handle=CreateMutex( 0, FALSE, 0 );
    if( m_handle==0 ) {
        throw os_error_t( "mutex_t::mutex_t: CreateMutex failed" );
    }
}

mutex_t::~mutex_t()
{
    CloseHandle( m_handle );
}

// grab_mutex_t
grab_mutex_t::grab_mutex_t( mutex_t & mutex, DWORD timeout )
{
    m_mutex=mutex.m_handle;
    m_wait_result=wait( m_mutex, timeout );
}

grab_mutex_t::~grab_mutex_t()
{
    if( m_wait_result.ok() ) {
        ReleaseMutex( m_mutex );
    }
}

bool grab_mutex_t::ok()
{
    return m_wait_result.ok();
}

DWORD grab_mutex_t::error_code()
{
    return m_wait_result.error_code();
}

char const * grab_mutex_t::error_message()
{
    return m_wait_result.error_message();
}

// thread_buffer_t
thread_buffer_t::thread_buffer_t()
{
    m_direction=dir_none;

    m_message_prefix="";
    m_error_code=ERROR_SUCCESS;
    m_error_message="";

    m_wait_timeout=2000;
    m_buffer_limit=0;
    m_read_buffer_size=4096;

    m_thread=0;

    m_thread_termination_timeout=500;
    m_translate_crlf=true;
}

thread_buffer_t::~thread_buffer_t()
{
    bool stopped=false;
    try {
        stopped=stop_thread();
    }catch(...){
    }
    if( !stopped ) {
        // one more time, with attitude
        try {
            stopped=abort_thread();
        }catch(...){
        }
        if( !stopped ) {
            DWORD code=GetLastError();
            // otherwize, the thread will be left running loose stomping on freed memory.
            std::terminate();
        }
    }
}

void thread_buffer_t::set_wait_timeout( DWORD milliseconds )
{
    if( m_direction!=dir_none ) {
        throw exec_stream_t::error_t( "thread_buffer_t::set_wait_timeout: thread already started" );
    }
    m_wait_timeout=milliseconds;
}

// next three set values that are accessed in the same thread only, so they may be called anytime
void thread_buffer_t::set_thread_termination_timeout( DWORD milliseconds ) {
    m_thread_termination_timeout=milliseconds;
}

void thread_buffer_t::set_binary_mode()
{
    m_translate_crlf=false;
}

void thread_buffer_t::set_text_mode()
{
    m_translate_crlf=true;
}

void thread_buffer_t::set_buffer_limit( std::size_t limit )
{
    if( m_direction!=dir_none ) {
        throw exec_stream_t::error_t( "thread_buffer_t::set_buffer_limit: thread already started" );
    }
    m_buffer_limit=limit;
}

void thread_buffer_t::set_read_buffer_size( std::size_t size )
{
    if( m_direction!=dir_none ) {
        throw exec_stream_t::error_t( "thread_buffer_t::set_read_buffer_size: thread already started" );
    }
    m_read_buffer_size=size;
}

void thread_buffer_t::start_reader_thread( HANDLE pipe ) 
{
    start_thread( pipe, dir_read );
}

void thread_buffer_t::start_writer_thread( HANDLE pipe )
{
    start_thread( pipe, dir_write );
}

void thread_buffer_t::start_thread( HANDLE pipe, direction_t direction )
{
    if( m_direction!=dir_none ) {
        throw exec_stream_t::error_t( "thread_buffer_t::start_thread: thread already started" );
    }
    m_buffer_list.clear();
    m_pipe=pipe;
    if( !m_stop_thread.reset() ) {
        throw os_error_t( "thread_buffer_t::start_thread: unable to initialize m_stop_thread event" );
    }
    if( !m_got_data.reset() ) {
        throw os_error_t( "thread_buffer_t::start_thread: unable to initialize m_got_data event" );
    }
    if( !m_want_data.set() ) {
        throw os_error_t( "thread_buffer_t::start_thread: unable to initialize m_want_data event" );
    }
    DWORD thread_id;
    m_thread=CreateThread( 0, 0, direction==dir_read ? reader_thread : writer_thread, this, 0, &thread_id );
    if( m_thread==0 ) {
        throw os_error_t( "thread_buffer_t::start_thread: unable to start thread" );
    }
    m_direction= direction==dir_read ? dir_read : dir_write;
}

bool thread_buffer_t::check_thread_stopped()
{
    wait_result_t wait_result=wait( m_thread, m_thread_termination_timeout );
    if( !wait_result.ok() && !wait_result.timed_out() ) {
        check_error( "thread_buffer_t::check_thread_stopped: wait for thread to finish failed", wait_result.error_code(), wait_result.error_message() );
    }
    if( wait_result.ok() ) {
        CloseHandle( m_thread );
        m_direction=dir_none;
        return true;
    }else {
        return false;
    }
}

bool thread_buffer_t::stop_thread()
{
    if( m_direction!=dir_none ) {
        if( !m_stop_thread.set() ) {
            throw os_error_t( "thread_buffer_t::stop_thread: m_stop_thread.set() failed" );
        }
        bool res=check_thread_stopped();
        if( res ) {
            check_error( m_message_prefix, m_error_code, m_error_message );
        }
        return res;
    }
    return true;
}

bool thread_buffer_t::abort_thread()
{
    if( m_direction!=dir_none ) {
        if( !TerminateThread( m_thread, 0 ) ) {
            throw os_error_t( "exec_steam_t::abort_thread: TerminateThread failed" );
        }
        return check_thread_stopped();
    }
    return true;
}

void thread_buffer_t::get( exec_stream_t::stream_kind_t, char * dst, std::size_t & size, bool & no_more )
{
    if( m_direction!=dir_read ) {
        throw exec_stream_t::error_t( "thread_buffer_t::get: thread was not started or was started for writing" );
    }
    // check thread status
    DWORD thread_exit_code;
    if( !GetExitCodeThread( m_thread, &thread_exit_code ) ) {
        throw os_error_t( "thread_buffer_t::get: GetExitCodeThread failed" );
    }

    if( thread_exit_code!=STILL_ACTIVE ) {
        if( !m_buffer_list.empty() ) {
            // we have data - deliver it first
            // when thread terminated, there is no need to synchronize
            if( m_translate_crlf ) {
                m_buffer_list.get_translate_crlf( dst, size );
            }else {
                m_buffer_list.get( dst, size );
            }
            no_more=false;
        }else {
            // thread terminated and we have no more data to return - report errors, if any
            check_error( m_message_prefix, m_error_code, m_error_message );
            // if terminated without error  - signal eof 
            no_more=true;
            size=0;
        }
    }else {
        no_more=false;
        // thread still running - synchronize
        // wait for both m_got_data, m_mutex
        wait_result_t wait_result=wait( m_got_data, m_wait_timeout );
        if( !wait_result.ok() ) {
            check_error( "thread_buffer_t::get: wait for got_data failed", wait_result.error_code(), wait_result.error_message() );
        }
        grab_mutex_t grab_mutex( m_mutex, m_wait_timeout );
        if( !grab_mutex.ok() ) {
            check_error( "thread_buffer_t::get: wait for mutex failed", grab_mutex.error_code(), grab_mutex.error_message() );
        }

        if( m_translate_crlf ) {
            m_buffer_list.get_translate_crlf( dst, size );
        }else {
            m_buffer_list.get( dst, size );
        }

        // if buffer is not too long tell the thread we want more data
        if( !m_buffer_list.full( m_buffer_limit ) ) {
            if( !m_want_data.set() ) {
                throw os_error_t( "thread_buffer_t::get: unable to set m_want_data event" );
            }
        }
        // if no data left - make the next get() wait until it arrives
        if( m_buffer_list.empty() ) {
            if( !m_got_data.reset() ) {
                throw os_error_t( "thread_buffer_t::get: unable to reset m_got_data event" );
            }
        }
    }
}

DWORD WINAPI thread_buffer_t::reader_thread( LPVOID param )
{
    thread_buffer_t * p=static_cast< thread_buffer_t * >( param );
    // accessing p anywhere here is safe because thread_buffer_t destructor 
    // ensures the thread is terminated before p get destroyed
    char * read_buffer=0;
    try {
        read_buffer=new char[p->m_read_buffer_size];

        while( true ) {
            // see if get() wants more data, or if someone wants to stop the thread
            wait_result_t wait_result=wait( p->m_stop_thread, p->m_want_data, p->m_wait_timeout );
            if( !wait_result.ok() && !wait_result.timed_out() ) {
                p->note_thread_error( "thread_buffer_t::reader_thread: wait for want_data, destruction failed", wait_result.error_code(), wait_result.error_message() );
                break;
            }

            if( wait_result.is_signaled( p->m_stop_thread ) ) {
                // they want us to stop
                break;
            }

            if( wait_result.is_signaled( p->m_want_data ) ) {
                // they want more data - read the file
                DWORD read_size=0;
                DWORD read_status=ERROR_SUCCESS;
                if( !ReadFile( p->m_pipe, read_buffer, p->m_read_buffer_size, &read_size, 0 ) ) {
                    read_status=GetLastError();
                    if( read_status!=ERROR_BROKEN_PIPE ) {
                        p->note_thread_error( "thread_buffer_t::reader_thread: ReadFile failed", read_status, "" );
                        break;
                    }
                }

                // read something - append to p->m_buffers
                if( read_size!=0 ) {
                    grab_mutex_t grab_mutex( p->m_mutex, p->m_wait_timeout );
                    if( !grab_mutex.ok() ) {
                        p->note_thread_error( "thread_buffer_t::reader_thread: wait for mutex failed", grab_mutex.error_code(), grab_mutex.error_message() );
                        break;
                    }

                    p->m_buffer_list.put( read_buffer, read_size );

                    // if buffer is too long - do not read any more until it shrinks
                    if( p->m_buffer_list.full( p->m_buffer_limit ) ) {
                        if( !p->m_want_data.reset() ) {
                            p->note_thread_error( "thread_buffer_t::reader_thread: unable to reset m_want_data event", GetLastError(), "" );
                            break;
                        }
                    }
                    // tell get() we got some data
                    if( !p->m_got_data.set() ) {
                        p->note_thread_error( "thread_buffer_t::reader_thread:  unable to set m_got_data event", GetLastError(), "" );
                        break;
                    }
                }
                // pipe broken - quit thread, which will be seen by get() as eof.
                if( read_status==ERROR_BROKEN_PIPE ) {
                    break;
                }
            }
        }
    }catch( ... ) {
    // might only be std::bad_alloc
        p->note_thread_error( "", ERROR_SUCCESS, "thread_buffer_t::reader_thread: unknown exception caught" );
    }

    delete[] read_buffer;

    // ensure that get() is not left waiting on got_data
    p->m_got_data.set();
    return 0;
}

void thread_buffer_t::put( char * const src, std::size_t & size, bool & no_more )
{
    if( m_direction!=dir_write ) {
        throw exec_stream_t::error_t( "thread_buffer_t::put: thread not started or started for reading" );
    }
    // check thread status
    DWORD thread_exit_code;
    if( !GetExitCodeThread( m_thread, &thread_exit_code ) ) {
        throw os_error_t( "thread_buffer_t::get: GetExitCodeThread failed" );
    }

    if( thread_exit_code!=STILL_ACTIVE ) {
        // thread terminated - check for errors
        check_error( m_message_prefix, m_error_code, m_error_message );
        // if terminated without error  - signal eof, since no one will ever write our data
        size=0;
        no_more=true;
    }else {
        // wait for both m_want_data and m_mutex
        wait_result_t wait_result=wait( m_want_data, m_wait_timeout );
        if( !wait_result.ok() ) {
            check_error( "thread_buffer_t::put: wait for want_data failed", wait_result.error_code(), wait_result.error_message() );
        }
        grab_mutex_t grab_mutex( m_mutex, m_wait_timeout );
        if( !grab_mutex.ok() ) {
            check_error( "thread_buffer_t::put: wait for mutex failed", grab_mutex.error_code(), grab_mutex.error_message() );
        }
        
        // got them - put data
        no_more=false;
        if( m_translate_crlf ) {
            m_buffer_list.put_translate_crlf( src, size );
        }else {
            m_buffer_list.put( src, size );
        }
        
        // if the buffer is too long - make the next put() wait until it shrinks
        if( m_buffer_list.full( m_buffer_limit ) ) {
            if( !m_want_data.reset() ) {
                throw os_error_t( "thread_buffer_t::put: unable to reset m_want_data event" );
            }
        }
        // tell the thread we got data
        if( !m_buffer_list.empty() ) {
            if( !m_got_data.set() ) {
                throw os_error_t( "thread_buffer_t::put: unable to set m_got_data event" );
            }
        }
    }
}

DWORD WINAPI thread_buffer_t::writer_thread( LPVOID param )
{
    thread_buffer_t * p=static_cast< thread_buffer_t * >( param );
    // accessing p anywhere here is safe because thread_buffer_t destructor 
    // ensures the thread is terminated before p get destroyed
    try {
        buffer_list_t::buffer_t buffer;
        buffer.data=0;
        buffer.size=0;
        std::size_t buffer_offset=0;

        while( true ) {
            // wait for got_data or destruction, ignore timeout errors 
            // for destruction the timeout is normally expected,
            // for got data the timeout is not normally expected but tolerable (no one wants to write)
            wait_result_t wait_result=wait( p->m_got_data, p->m_stop_thread, p->m_wait_timeout );

           if( !wait_result.ok() && !wait_result.timed_out() ) {
                p->note_thread_error( "thread_buffer_t::writer_thread: wait for got_data, destruction failed", wait_result.error_code(), wait_result.error_message() );
                break;
            }
            
            // if no data in local buffer to write - get from p->m_buffers
            if( buffer.data==0 && wait_result.is_signaled( p->m_got_data ) ) { 
                grab_mutex_t grab_mutex( p->m_mutex, p->m_wait_timeout );
                if( !grab_mutex.ok() ) {
                    p->note_thread_error( "thread_buffer_t::writer_thread: wait for mutex failed", grab_mutex.error_code(), grab_mutex.error_message() );
                    break;
                }
                if( !p->m_buffer_list.empty() ) {
                    // we've got buffer - detach it
                    buffer=p->m_buffer_list.detach();
                    buffer_offset=0;
                }
                // if no data left in p->m_buffers - wait until it arrives
                if( p->m_buffer_list.empty() ) {
                    if( !p->m_got_data.reset() ) {
                        p->note_thread_error( "thread_buffer_t::writer_thread: unable to reset m_got_data event", GetLastError(), "" );
                        break;
                    }
                }
                // if buffer is not too long - tell put() it can proceed
                if( !p->m_buffer_list.full( p->m_buffer_limit ) ) {
                    if( !p->m_want_data.set() ) {
                        p->note_thread_error( "thread_buffer_t::writer_thread: unable to set m_want_data event", GetLastError(), "" );
                        break;
                    }
                }
            }
            
            // see if they want us to stop, but only when all is written
            if( buffer.data==0 && wait_result.is_signaled( p->m_stop_thread ) ) {
                break;
            }
            
            if( buffer.data!=0 ) {
                // we have buffer - write it
                DWORD written_size;
                if( !WriteFile( p->m_pipe, buffer.data+buffer_offset, buffer.size-buffer_offset, &written_size, 0 ) ) {
                    p->note_thread_error( "thread_buffer_t::writer_thread: WriteFile failed", GetLastError(), "" );
                    break;
                }
                buffer_offset+=written_size;
                if( buffer_offset==buffer.size ) {
                    delete[] buffer.data;
                    buffer.data=0;
                }
            }

        }

        // we won't be writing any more - close child's stdin
        CloseHandle( p->m_pipe );

        // buffer may be left astray - clean up
        delete[] buffer.data;

    }catch( ... ) {
    // unreachable code. really.
        p->note_thread_error( "", ERROR_SUCCESS, "thread_buffer_t::writer_thread: unknown exception caught" );
    }
    // ensure that put() is not left waiting on m_want_data
    p->m_want_data.set();
    return 0;
}

void thread_buffer_t::check_error( std::string const & message_prefix, DWORD error_code, std::string const & error_message )
{
    if( !error_message.empty() ) {
        throw exec_stream_t::error_t( message_prefix+"\n"+error_message, error_code );
    }else if( error_code!=ERROR_SUCCESS ) {
        throw os_error_t( message_prefix, error_code );
    }
}

void thread_buffer_t::note_thread_error( char const * message_prefix, DWORD error_code, char const * error_message )
{
    m_message_prefix=message_prefix;
    m_error_code=error_code;
    m_error_message=error_message;
}

