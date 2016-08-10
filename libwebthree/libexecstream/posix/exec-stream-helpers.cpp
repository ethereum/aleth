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
    compose( msg, errno );
}

os_error_t::os_error_t( std::string const & msg, exec_stream_t::error_code_t code )
{
    compose( msg, code );
}

void os_error_t::compose( std::string const & msg, exec_stream_t::error_code_t code )
{
    std::string s( msg );
    s+='\n';
    errno=0;
    char const * x=strerror( code );
    if( errno!=0 ) {
        s+="[unable to retrieve error description]";
    }else {
        s+=x;
    }
    exec_stream_t::error_t::compose( s, code );
}

// pipe_t
pipe_t::pipe_t()
: m_direction( closed )
{
    m_fds[0]=-1;
    m_fds[1]=-1;
}

pipe_t::~pipe_t()
{
    try {
        close();
    }catch(...) {
    }
}

int pipe_t::r() const
{
    return m_fds[0];
}

int pipe_t::w() const
{
    return m_fds[1];
}

void pipe_t::close_r()
{
    if( m_direction==both || m_direction==read ) {
        if( ::close( m_fds[0] )==-1 ) {
            throw os_error_t( "pipe_t::close_r: close failed" );
        }
        m_direction= m_direction==both ? write : closed;
    }
}

void pipe_t::close_w()
{
    if( m_direction==both || m_direction==write ) {
        if( ::close( m_fds[1] )==-1 ) {
            throw os_error_t( "pipe_t::close_w: close failed" );
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
    if( pipe( m_fds )==-1 ) {
        throw os_error_t( "pipe_t::open(): pipe() failed" );
    }
    m_direction=both;
}

    
// mutex_t
mutex_t::mutex_t()
{
    if( int code=pthread_mutex_init( &m_mutex, 0 ) ) {
        throw os_error_t( "mutex_t::mutex_t: pthread_mutex_init failed", code );
    }    
}

mutex_t::~mutex_t()
{
    pthread_mutex_destroy( &m_mutex );
}


// grab_mutex_t
grab_mutex_t::grab_mutex_t( mutex_t & mutex, mutex_registrator_t * mutex_registrator )
{
    m_mutex=&mutex.m_mutex;
    m_error_code=pthread_mutex_lock( m_mutex );
    m_grabbed=ok();
    m_mutex_registrator=mutex_registrator;
    if( m_mutex_registrator ) {
        m_mutex_registrator->add( this );
    }
}

grab_mutex_t::~grab_mutex_t()
{
    release();
    if( m_mutex_registrator ) {
        m_mutex_registrator->remove( this );
    }
}

int grab_mutex_t::release()
{
    int code=0;
    if( m_grabbed ) {
        code=pthread_mutex_unlock( m_mutex );
        m_grabbed=false;
    }
    return code;
}

bool grab_mutex_t::ok()
{
    return m_error_code==0;
}

int grab_mutex_t::error_code()
{
    return m_error_code;
}

// mutex_registrator_t
mutex_registrator_t::~mutex_registrator_t()
{
    for( mutexes_t::iterator i=m_mutexes.begin(); i!=m_mutexes.end(); ++i ) {
        (*i)->m_mutex_registrator=0;
    }
}

void mutex_registrator_t::add( grab_mutex_t * g )
{
    m_mutexes.insert( m_mutexes.end(), g );
}

void mutex_registrator_t::remove( grab_mutex_t * g )
{
    m_mutexes.erase( std::find( m_mutexes.begin(), m_mutexes.end(), g ) );
}

void mutex_registrator_t::release_all()
{
    for( mutexes_t::iterator i=m_mutexes.begin(); i!=m_mutexes.end(); ++i ) {
        (*i)->release();
    }
}
    
// wait_result_t
wait_result_t::wait_result_t( unsigned signaled_state, int error_code, bool timed_out )
{
    m_timed_out=timed_out;
    m_error_code=error_code;
    m_signaled_state= error_code==0 ? signaled_state : 0;
}

bool wait_result_t::ok()
{
    return m_error_code==0;
}

bool wait_result_t::is_signaled( int state )
{
    return m_signaled_state&state;
}

int wait_result_t::error_code()
{
    return m_error_code;
}

bool wait_result_t::timed_out()
{
    return m_timed_out;
}


// event_t
event_t::event_t()
{
    if( int code=pthread_cond_init( &m_cond, 0 ) ) {
        throw os_error_t( "event_t::event_t: pthread_cond_init failed", code );
    }
    m_state=0;
}

event_t::~event_t()
{
    pthread_cond_destroy( &m_cond );
}

int event_t::set( unsigned bits, mutex_registrator_t * mutex_registrator )
{
    grab_mutex_t grab_mutex( m_mutex, mutex_registrator );
    if( !grab_mutex.ok() ) {
        return grab_mutex.error_code();
    }

    int code=0;
    if( bits&~m_state ) {
        m_state|=bits;
        code=pthread_cond_broadcast( &m_cond );
    }
    
    int release_code=grab_mutex.release();
    if( code==0  ) {
        code=release_code;
    }
    return code;
}

int event_t::reset( unsigned bits, mutex_registrator_t * mutex_registrator )
{
    grab_mutex_t grab_mutex( m_mutex, mutex_registrator );
    if( !grab_mutex.ok() ) {
        return grab_mutex.error_code();
    }
    m_state&=~bits;    
    return grab_mutex.release();
}

wait_result_t event_t::wait( unsigned any_bits, unsigned long timeout, mutex_registrator_t * mutex_registrator )
{
    if( any_bits==0 ) {
        // we ain't waiting for anything
        return wait_result_t( 0, 0, false );
    }

    grab_mutex_t grab_mutex( m_mutex, mutex_registrator );
    if( !grab_mutex.ok() ) {
        return wait_result_t( 0, grab_mutex.error_code(), false );
    }
    
    struct timeval time_val_limit;
    gettimeofday( &time_val_limit, 0 );
    struct timespec time_limit;
    time_limit.tv_sec=time_val_limit.tv_sec+timeout/1000;
    time_limit.tv_nsec=1000*(time_val_limit.tv_usec+1000*(timeout%1000));
    int code=0;
    while( code==0 && (m_state&any_bits)==0 ) {
        code=pthread_cond_timedwait( &m_cond, &m_mutex.m_mutex, &time_limit );
    }
    
    unsigned state=m_state;
    int release_code=grab_mutex.release();
    if( code==0 ) {
        code=release_code;
    }
    return wait_result_t( state, code, code==ETIMEDOUT );
}

// thread_buffer_t
thread_buffer_t::thread_buffer_t( pipe_t & in_pipe, pipe_t & out_pipe, pipe_t & err_pipe, std::ostream & in )
: m_in_pipe( in_pipe ), m_out_pipe( out_pipe ), m_err_pipe( err_pipe ), m_in( in )
{
    m_in_bad=false;
    m_error_prefix="";
    m_error_code=0;
    m_in_wait_timeout=2000;
    m_out_wait_timeout=2000;
    m_err_wait_timeout=2000;
    m_thread_termination_timeout=1000;
    m_in_buffer_limit=0;
    m_out_buffer_limit=0;
    m_err_buffer_limit=0;
    m_out_read_buffer_size=4096;
    m_err_read_buffer_size=4096;
    m_thread_started=false;
    m_in_closed=false;
}

thread_buffer_t::~thread_buffer_t()
{
    bool stopped=false;
    try {
        stopped=stop_thread();
    }catch( ... ) {
    }
    if( !stopped ) {
        try {
            stopped=abort_thread();
        }catch( ... ) {
        }
    }
    if( !stopped ) {
        std::terminate();
    }
}

void thread_buffer_t::set_wait_timeout( int stream_kind, unsigned long milliseconds )
{
    if( m_thread_started ) {
        throw exec_stream_t::error_t( "thread_buffer_t::set_wait_timeout: thread already started" );
    }
    if( stream_kind&exec_stream_t::s_in ) {
        m_in_wait_timeout=milliseconds;
    }
    if( stream_kind&exec_stream_t::s_out ) {
        m_out_wait_timeout=milliseconds;
    }
    if( stream_kind&exec_stream_t::s_err ) {
        m_err_wait_timeout=milliseconds;
    }
    if( stream_kind&exec_stream_t::s_child ) {
        m_thread_termination_timeout=milliseconds;
    }
}

void thread_buffer_t::set_buffer_limit( int stream_kind, std::size_t limit )
{
    if( m_thread_started ) {
        throw exec_stream_t::error_t( "thread_buffer_t::set_buffer_limit: thread already started" );
    }
    if( stream_kind&exec_stream_t::s_in ) {
        m_in_buffer_limit=limit;
    }
    if( stream_kind&exec_stream_t::s_out ) {
        m_out_buffer_limit=limit;
    }
    if( stream_kind&exec_stream_t::s_err ) {
        m_err_buffer_limit=limit;
    }
}

void thread_buffer_t::set_read_buffer_size( int stream_kind, std::size_t size )
{
    if( m_thread_started ) {
        throw exec_stream_t::error_t( "thread_buffer_t::set_read_buffer_size: thread already started" );
    }
    if( stream_kind&exec_stream_t::s_out ) {
        m_out_read_buffer_size=size;
    }
    if( stream_kind&exec_stream_t::s_err ) {
        m_err_read_buffer_size=size;
    }
}

void thread_buffer_t::start()
{
    if( m_thread_started ) {
        throw exec_stream_t::error_t( "thread_buffer_t::start: thread already started" );
    }
    m_in_buffer.clear();
    m_out_buffer.clear();
    m_err_buffer.clear();

    int code;
    if( (code=m_thread_control.reset( ~0u, 0 )) || (code=m_thread_control.set( exec_stream_t::s_out|exec_stream_t::s_err, 0 ) ) ) {
        throw os_error_t( "thread_buffer_t::start: unable to initialize m_thread_control event", code );
    }
    if( (code=m_thread_responce.reset( ~0u, 0 )) || (code=m_thread_responce.set( exec_stream_t::s_in, 0 )) ) {
        throw os_error_t( "thread_buffer_t::start: unable to initialize m_thread_responce event", code );
    }
    
    m_error_prefix="";
    m_error_code=0;

    if( int code=pthread_create( &m_thread, 0, &thread_func, this ) ) {
        throw os_error_t( "exec_stream_therad_t::start: pthread_create failed", code );
    }
    m_thread_started=true;
    m_in_closed=false;
    m_in_bad=false;
}

bool thread_buffer_t::stop_thread()
{
    if( m_thread_started ) {
        if( int code=m_thread_control.set( exec_stream_t::s_child, 0 ) ) {
            throw os_error_t( "thread_buffer_t::stop_thread: unable to set thread termination event", code );
        }
        wait_result_t wait_result=m_thread_responce.wait( exec_stream_t::s_child, m_thread_termination_timeout, 0 );
        if( !wait_result.ok() && !wait_result.timed_out() ) {
            throw os_error_t( "thread_buffer_t::stop_thread: wait for m_thread_stopped failed", wait_result.error_code() );
        }
        if( wait_result.ok() ) {
            void * thread_result;
            if( int code=pthread_join( m_thread, &thread_result ) ) {
                throw os_error_t( "thread_buffer_t::stop_thread: pthread_join failed", code );
            }
            m_thread_started=false;
            // check for any errors encountered in the thread
            if( m_error_code!=0 ) {
                throw os_error_t( m_error_prefix, m_error_code );
            }
            return true;
        }else {
            return false;
        }
    }
    return true;
}

bool thread_buffer_t::abort_thread()
{
    if( m_thread_started ) {
        if( int code=pthread_cancel( m_thread ) ) {
            throw os_error_t( "thread_buffer_t::abort_thread: pthread_cancel failed", code );
        }
        void * thread_result;
        if( int code=pthread_join( m_thread, &thread_result ) ) {
            throw os_error_t( "thread_buffer_t::stop_thread: pthread_join failed", code );
        }
        m_thread_started=false;
    }
    return true;
}

const int s_in_eof=16;
const int s_out_eof=32;
const int s_err_eof=64;

void thread_buffer_t::get( exec_stream_t::stream_kind_t kind, char * dst, std::size_t & size, bool & no_more )
{
    if( !m_thread_started ) {
        throw exec_stream_t::error_t( "thread_buffer_t::get: thread was not started" );
    }
    unsigned long timeout= kind==exec_stream_t::s_out ? m_out_wait_timeout : m_err_wait_timeout;
    int eof_kind= kind==exec_stream_t::s_out ? s_out_eof : s_err_eof;
    buffer_list_t & buffer= kind==exec_stream_t::s_out ? m_out_buffer : m_err_buffer;

    wait_result_t wait_result=m_thread_responce.wait( kind|exec_stream_t::s_child|eof_kind, timeout, 0 );
    if( !wait_result.ok() ) {
        throw os_error_t( "thread_buffer_t::get: wait for got_data failed", wait_result.error_code() );
    }
    
    if( wait_result.is_signaled( exec_stream_t::s_child ) ) {
        // thread stopped - no need to synchronize
        if( !buffer.empty() ) {
            // we have data - deliver it first
            // when thread terminated, there is no need to synchronize
            buffer.get( dst, size );
            no_more=false;
        }else {
            // thread terminated and we have no more data to return - report errors, if any
            if( m_error_code!=0 ) {
                throw os_error_t( m_error_prefix, m_error_code );
            }
            // if terminated without error  - signal eof 
            size=0;
            no_more=true;
        }        
    }else if( wait_result.is_signaled( kind|eof_kind ) ) {
        // thread got some data for us - grab them
        grab_mutex_t grab_mutex( m_mutex, 0 );
        if( !grab_mutex.ok() ) {
            throw os_error_t( "thread_buffer_t::get: wait for mutex failed", grab_mutex.error_code() );
        }

        if( !buffer.empty() ) {
            buffer.get( dst, size );
            no_more=false;
        }else {
            size=0;
            no_more=wait_result.is_signaled( eof_kind );
        }
        // if no data left - make the next get() wait until it arrives
        if( buffer.empty() ) {
            if( int code=m_thread_responce.reset( kind, 0 )  ) {
                throw os_error_t( "thread_buffer_t::get: unable to reset got_data event", code );
            }
        }        
        // if buffer is not too long tell the thread we want more data
        std::size_t buffer_limit= kind==exec_stream_t::s_out ? m_out_buffer_limit : m_err_buffer_limit;
        if( !buffer.full( buffer_limit ) ) {
            if( int code=m_thread_control.set( kind, 0 ) ) {
                throw os_error_t( "thread_buffer_t::get: unable to set want_data event", code );
            }
        }
    }
}

void thread_buffer_t::put( char * src, std::size_t & size, bool & no_more )
{
    if( !m_thread_started ) {
        throw exec_stream_t::error_t( "thread_buffer_t::put: thread was not started" );
    }
    if( m_in_closed || m_in_bad ) { 
        size=0;
        no_more=true;
        return;
    }
    // wait for both m_want_data and m_mutex
    wait_result_t wait_result=m_thread_responce.wait( exec_stream_t::s_in|exec_stream_t::s_child, m_in_wait_timeout, 0 );
    if( !wait_result.ok() ) {
        // workaround for versions of libstdc++ (at least in gcc 3.1 pre) that do not intercept exceptions in operator<<( std::ostream, std::string )
        m_in_bad=true;
        if( m_in.exceptions()&std::ios_base::badbit ) {
            throw os_error_t( "thread_buffer_t::put: wait for want_data failed", wait_result.error_code() );
        }else {
            m_in.setstate( std::ios_base::badbit );
            size=0;
            no_more=true;
            return;
        }
    }
    if( wait_result.is_signaled( exec_stream_t::s_child ) ) {
        // thread stopped - check for errors
        if( m_error_code!=0 ) {
            throw os_error_t( m_error_prefix, m_error_code );
        }        
        // if terminated without error  - signal eof, since no one will ever write our data
        size=0;
        no_more=true;
    }else if( wait_result.is_signaled( exec_stream_t::s_in ) ) {
        // thread wants some data from us - stuff them
        grab_mutex_t grab_mutex( m_mutex, 0 );
        if( !grab_mutex.ok() ) {
            throw os_error_t( "thread_buffer_t::put: wait for mutex failed", grab_mutex.error_code() );
        }

        no_more=false;
        m_in_buffer.put( src, size );
        
        // if the buffer is too long - make the next put() wait until it shrinks
        if( m_in_buffer.full( m_in_buffer_limit ) ) {
            if( int code=m_thread_responce.reset( exec_stream_t::s_in, 0 ) ) {
                throw os_error_t( "thread_buffer_t::put: unable to reset want_data event", code );
            }
        }
        // tell the thread we got data
        if( !m_in_buffer.empty() ) {
            if( int code=m_thread_control.set( exec_stream_t::s_in, 0 ) ) {
                throw os_error_t( "thread_buffer_t::put: unable to set got_data event", code );
            }
        }
    }
}

void thread_buffer_t::close_in()
{
    if( !m_in_bad ) {
        m_in.flush();
    }
    if( m_thread_started ) {
        if( int code=m_thread_control.set( s_in_eof, 0 ) ) {
            throw os_error_t( "thread_buffer_t::close_in: unable to set in_got_data event", code );
        }
        m_in_closed=true;
    }
}

void mutex_cleanup( void * p )
{
    static_cast< mutex_registrator_t * >( p )->release_all();
}

void * thread_buffer_t::thread_func( void * param )
{
    thread_buffer_t * p=static_cast< thread_buffer_t * >( param );
    // accessing p anywhere here is safe because thread_buffer_t destructor 
    // ensures the thread is terminated before p get destroyed
    char * out_read_buffer=0;
    char * err_read_buffer=0;
    bool in_eof=false;
    bool in_closed=false;
    bool out_eof=false;
    bool err_eof=false;

    mutex_registrator_t mutex_registrator;
    pthread_cleanup_push( mutex_cleanup, &mutex_registrator );
        
    try {
        out_read_buffer=new char[p->m_out_read_buffer_size];
        err_read_buffer=new char[p->m_err_read_buffer_size];
        
        buffer_list_t::buffer_t write_buffer;
        write_buffer.data=0;
        write_buffer.size=0;
        std::size_t write_buffer_offset=0;

        unsigned long timeout=std::max( p->m_in_wait_timeout, std::max( p->m_out_wait_timeout, p->m_err_wait_timeout ) );

        fd_set read_fds;
        FD_ZERO( &read_fds );
        fd_set write_fds;
        FD_ZERO( &write_fds );

        while( true ) {
            unsigned wait_for=exec_stream_t::s_child;
            if( !in_eof && write_buffer.data==0 ) {
                wait_for|=exec_stream_t::s_in|s_in_eof;
            }
            if( !out_eof ) {
                wait_for|=exec_stream_t::s_out;
            }
            if( !err_eof ) {
                wait_for|=exec_stream_t::s_err;
            }

            wait_result_t wait_result=p->m_thread_control.wait( wait_for, timeout, &mutex_registrator );
            if( !wait_result.ok() && !wait_result.timed_out() ) {
                p->m_error_code=wait_result.error_code();
                p->m_error_prefix="thread_buffer_t::thread_func: wait for thread_event failed";
                break;
            }

            // we need more data - get from p->m_buffers
            if( write_buffer.data==0 && wait_result.is_signaled( exec_stream_t::s_in|s_in_eof ) ) {
                grab_mutex_t grab_mutex( p->m_mutex, &mutex_registrator );
                if( !grab_mutex.ok() ) {
                    p->m_error_code=grab_mutex.error_code();
                    p->m_error_prefix="thread_buffer_t::thread_func: wait for mutex failed";
                    break;
                }

                if( p->m_in_buffer.empty() ) {
                    // we have empty write_buffer, empty p->m_in_buffer and we are told it will stay so - time to close child's stdin
                    if( wait_result.is_signaled( s_in_eof ) ) {
                        in_eof=true;
                    }
                }
                if( !p->m_in_buffer.empty() ) {
                    // we've got buffer - detach it
                    write_buffer=p->m_in_buffer.detach();
                    write_buffer_offset=0;
                }
                // if no data left in p->m_in_buffer - wait until it arrives
                if( p->m_in_buffer.empty() ) {
                    // if no data for us - stop trying to get it until we are told it arrived
                    if( int code=p->m_thread_control.reset( exec_stream_t::s_in, &mutex_registrator ) ) {
                        p->m_error_code=code;
                        p->m_error_prefix="thread_buffer_t::thread_func: unable to reset thread_event (s_in)";
                        break;
                    }
                }

                // if buffer is not too long - tell put() it can proceed
                if( !p->m_in_buffer.full( p->m_in_buffer_limit ) ) {
                    if( int code=p->m_thread_responce.set( exec_stream_t::s_in, &mutex_registrator ) ) {
                        p->m_error_code=code;
                        p->m_error_prefix="thread_buffer_t::thread_func: unable to set in_want_data event";
                        break;
                    }
                }
            }

            if( in_eof && write_buffer.data==0 ) {
                p->m_in_pipe.close();
                in_closed=true;
            }

            // see if they want us to stop, but only when there is nothing more to write
            if( write_buffer.data==0 && wait_result.is_signaled( exec_stream_t::s_child ) ) {
                break;
            }
            
            // determine whether we want something
            if( write_buffer.data!=0 ) {
                FD_SET( p->m_in_pipe.w(), &write_fds );
            }else {
                FD_CLR( p->m_in_pipe.w(), &write_fds );
            }
            if( !out_eof && wait_result.is_signaled( exec_stream_t::s_out ) ) {
                FD_SET( p->m_out_pipe.r(), &read_fds );
            }else {
                FD_CLR( p->m_out_pipe.r(), &read_fds );
            }
            if( !err_eof && wait_result.is_signaled( exec_stream_t::s_err ) ) {
                FD_SET( p->m_err_pipe.r(), &read_fds );
            }else {
                FD_CLR( p->m_err_pipe.r(), &read_fds );
            }

            if( FD_ISSET( p->m_in_pipe.w(), &write_fds ) || FD_ISSET( p->m_out_pipe.r(), &read_fds ) || FD_ISSET( p->m_err_pipe.r(), &read_fds ) ) {
                // we want something - get it
                struct timeval select_timeout;
                select_timeout.tv_sec=0;
                select_timeout.tv_usec=100000;
                int nfds=std::max( p->m_in_pipe.w(), std::max( p->m_out_pipe.r(), p->m_err_pipe.r() ) )+1;
                if( select( nfds, &read_fds, &write_fds, 0, &select_timeout )==-1 ) {
                    p->m_error_code=errno;
                    p->m_error_prefix="thread_buffer_t::thread_func: select failed";
                    break;
                }
            }
            
            // determine what we got

            if( FD_ISSET( p->m_in_pipe.w(), &write_fds ) ) {
                // it seems we may write to child's stdin
                int n_written=write( p->m_in_pipe.w(), write_buffer.data+write_buffer_offset, write_buffer.size-write_buffer_offset );
                if( n_written==-1 ) {
                    if( errno!=EAGAIN ) {
                        p->m_error_code=errno;
                        p->m_error_prefix="thread_buffer_t::thread_func: write to child stdin failed";
                        break;
                    }
                }else {
                    write_buffer_offset+=n_written;
                    if( write_buffer_offset==write_buffer.size ) {
                        delete[] write_buffer.data;
                        write_buffer.data=0;
                        write_buffer.size=0;
                    }
                }
            }

            if( FD_ISSET( p->m_out_pipe.r(), &read_fds ) ) {
                // it seems we may read child's stdout
                int n_out_read=read( p->m_out_pipe.r(), out_read_buffer, p->m_out_read_buffer_size );
                if( n_out_read==-1 ) {
                    if( errno!=EAGAIN ) {
                        p->m_error_code=errno;
                        p->m_error_prefix="exec_stream_t::thread_func: read from child stdout failed";
                        break;
                    }
                }else {
                    grab_mutex_t grab_mutex( p->m_mutex, &mutex_registrator );
                    if( n_out_read!=0 ) {
                        p->m_out_buffer.put( out_read_buffer, n_out_read );
                        // if buffer is full - stop reading
                        if( p->m_out_buffer.full( p->m_out_buffer_limit ) ) {
                            if( int code=p->m_thread_control.reset( exec_stream_t::s_out, &mutex_registrator ) ) {
                                p->m_error_code=code;
                                p->m_error_prefix="exec_stream_t::thread_func: unable to reset m_out_want_data event";
                                break;
                            }
                        }
                    }
                    unsigned responce=exec_stream_t::s_out;
                    if( n_out_read==0 ) { // EOF when read 0 bytes while select told that it's ready
                        out_eof=true;
                        responce|=s_out_eof;
                    }
                    // we got either data or eof - tell always
                    if( int code=p->m_thread_responce.set( responce, &mutex_registrator ) ) {
                        p->m_error_code=code;
                        p->m_error_prefix="exec_stream_t::thread_func: unable to set out_got_data event";
                        break;
                    }
                }
            }
            
            if( FD_ISSET( p->m_err_pipe.r(), &read_fds ) )  {
                // it seemds we may read child's stderr
                int n_err_read=read( p->m_err_pipe.r(), err_read_buffer, p->m_err_read_buffer_size );
                if( n_err_read==-1 ) {
                    if( errno!=EAGAIN ) {
                        p->m_error_code=errno;
                        p->m_error_prefix="exec_stream_t::thread_func: read from child stderr failed";
                        break;
                    }
                }else {
                    grab_mutex_t grab_mutex( p->m_mutex, &mutex_registrator );
                    if( n_err_read!=0 ) {
                        p->m_err_buffer.put( err_read_buffer, n_err_read );
                        // if buffer is full - stop reading
                        if( p->m_err_buffer.full( p->m_err_buffer_limit ) ) {
                            if( int code=p->m_thread_control.reset( exec_stream_t::s_err, &mutex_registrator ) ) {
                                p->m_error_code=code;
                                p->m_error_prefix="exec_stream_t::thread_func: unable to reset m_err_want_data event";
                                break;
                            }
                        }
                    }
                    unsigned responce=exec_stream_t::s_err;
                    if( n_err_read==0 ) {
                        err_eof=true;
                        responce|=s_err_eof;
                    }
                    // we got either data or eof - tell always
                    if( int code=p->m_thread_responce.set( responce, &mutex_registrator ) ) {
                        p->m_error_code=code;
                        p->m_error_prefix="exec_stream_t::thread_func: unable to set err_got_data event";
                        break;
                    }
                }
            }
            
            if( in_closed && out_eof && err_eof ) {
                // have nothing more to do
                break;
            }
        }
        
        delete[] write_buffer.data;
        
    }catch( ... ) {
        // might only be std::bad_alloc
        p->m_error_code=0;
        p->m_error_prefix="thread_buffer_t::writer_thread: exception caught";
    }

    delete[] out_read_buffer;
    delete[] err_read_buffer;

    // tell everyone that we've stopped, so that get() and put() will be unblocked
    if( int code=p->m_thread_responce.set( exec_stream_t::s_child, &mutex_registrator ) ) {
        p->m_error_code=code;
        p->m_error_prefix="exec_stream_t::thread_func: unable to set thread_stopped event";
    }
    
    pthread_cleanup_pop( 0 );
    return 0;
}
