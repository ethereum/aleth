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

// exec_stream_t::impl_t
struct exec_stream_t::impl_t {
    impl_t();
    ~impl_t();
    
    void split_args( std::string const & program, std::string const & arguments );
    void split_args( std::string const & program, exec_stream_t::next_arg_t & next_arg );
    void start( std::string const & program );
    
    pid_t m_child_pid;
    int m_exit_code;
    unsigned long m_child_timeout;

    buf_t< char > m_child_args;
    buf_t< char * > m_child_argp;
    
    pipe_t m_in_pipe;
    pipe_t m_out_pipe;
    pipe_t m_err_pipe;

    thread_buffer_t m_thread;
    
    exec_stream_buffer_t m_in_buffer;
    exec_stream_buffer_t m_out_buffer;
    exec_stream_buffer_t m_err_buffer;

    exec_ostream_t m_in;
    exec_istream_t m_out;
    exec_istream_t m_err;

    void (*m_old_sigpipe_handler)(int);
};

exec_stream_t::impl_t::impl_t()
: m_thread( m_in_pipe, m_out_pipe, m_err_pipe, m_in ), /* m_in here is not initialized, but its ok */
  m_in_buffer( exec_stream_t::s_in, m_thread ), m_out_buffer( exec_stream_t::s_out, m_thread ), m_err_buffer( exec_stream_t::s_err, m_thread ),
  m_in( m_in_buffer ), m_out( m_out_buffer ), m_err( m_err_buffer )
{
    m_out.tie( &m_in );
    m_err.tie( &m_in );
    m_child_timeout=1000;
    m_child_pid=-1;
    m_old_sigpipe_handler=signal( SIGPIPE, SIG_IGN );
}

exec_stream_t::impl_t::~impl_t()
{
    signal( SIGPIPE, m_old_sigpipe_handler );
}

void exec_stream_t::impl_t::split_args( std::string const & program, std::string const & arguments )
{
    char * args_end=m_child_args.new_data( program.size()+1+arguments.size()+1 );
    int argc=1;

    std::string::traits_type::copy( args_end, program.data(), program.size() );
    args_end+=program.size();
    *args_end++=0;

    std::string whitespace=" \t\r\n\v";
    
    std::string::size_type arg_start=arguments.find_first_not_of( whitespace );
    while( arg_start!=std::string::npos ) {
        ++argc;
        std::string::size_type arg_stop;
    if( arguments[arg_start]!='"' ) {
            arg_stop=arguments.find_first_of( whitespace, arg_start );
            if( arg_stop==std::string::npos ) {
                arg_stop=arguments.size();
            }
            std::string::traits_type::copy( args_end, arguments.data()+arg_start, arg_stop-arg_start );
            args_end+=arg_stop-arg_start;
        }else {
            std::string::size_type cur=arg_start+1;
            while( true ) {
                std::string::size_type next=arguments.find( '"', cur );
                if( next==std::string::npos || arguments[next-1]!='\\' ) {
                    if( next==std::string::npos ) {
                        next=arguments.size();
                        arg_stop=next;
                    }else {
                        arg_stop=next+1;
                    }
                    std::string::traits_type::copy( args_end, arguments.data()+cur, next-cur );
                    args_end+=next-cur;
                    break;
                }else {
                    std::string::traits_type::copy( args_end, arguments.data()+cur, next-1-cur );
                    args_end+=next-1-cur;
                    *args_end++='"';
                    cur=next+1;
                }
            }
        }
        *args_end++=0;
        arg_start=arguments.find_first_not_of( whitespace, arg_stop );
    }

    char ** argp_end=m_child_argp.new_data( argc+1 );
    char * args=m_child_args.data();
    while( args!=args_end ) {
        *argp_end=args;
        args+=std::string::traits_type::length( args )+1;
        ++argp_end;
    }
    *argp_end=0;
}

void exec_stream_t::impl_t::split_args( std::string const & program, exec_stream_t::next_arg_t & next_arg )
{
    typedef std::vector< std::size_t > arg_sizes_t;
    arg_sizes_t arg_sizes;
    
    m_child_args.new_data( program.size()+1 );
    std::string::traits_type::copy( m_child_args.data(), program.c_str(), program.size()+1 );
    arg_sizes.push_back( program.size()+1 );
    
    while( std::string const * s=next_arg.next() ) {
        m_child_args.append_data( s->c_str(), s->size()+1 );
        arg_sizes.push_back( s->size()+1 );
    }
    
    char ** argp_end=m_child_argp.new_data( arg_sizes.size()+1 );
    char * argp=m_child_args.data();
    for( arg_sizes_t::iterator i=arg_sizes.begin(); i!=arg_sizes.end(); ++i ) {
        *argp_end=argp;
        argp+=*i;
        ++argp_end;
    }
    *argp_end=0;
}

void exec_stream_t::set_buffer_limit( int stream_kind, std::size_t size )
{
    m_impl->m_thread.set_buffer_limit( stream_kind, size );    
}

void exec_stream_t::set_wait_timeout( int stream_kind, timeout_t milliseconds )
{
    m_impl->m_thread.set_wait_timeout( stream_kind, milliseconds );
    if( stream_kind&exec_stream_t::s_child ) {
        m_impl->m_child_timeout=milliseconds;
    }
}

void exec_stream_t::start( std::string const & program, std::string const & arguments )
{
    if( !close() ) {
        throw exec_stream_t::error_t( "exec_stream_t::start: previous child process has not yet terminated" );
    }
    
    m_impl->split_args( program, arguments );
    m_impl->start( program );
}

void exec_stream_t::start( std::string const & program, exec_stream_t::next_arg_t & next_arg )
{
    if( !close() ) {
        throw exec_stream_t::error_t( "exec_stream_t::start: previous child process has not yet terminated" );
    }

    m_impl->split_args( program, next_arg );
    m_impl->start( program );
}

void exec_stream_t::impl_t::start( std::string const & program )
{
    m_in_pipe.open();
    m_out_pipe.open();
    m_err_pipe.open();
    
    pipe_t status_pipe;
    status_pipe.open();
    
    pid_t pid=fork();
    if( pid==-1 ) {
        throw os_error_t( "exec_stream_t::start: fork failed" );
    }else if( pid==0 ) {
        try {
            status_pipe.close_r();
            if( fcntl( status_pipe.w(), F_SETFD, FD_CLOEXEC )==-1 ) {
                throw os_error_t( "exec_stream_t::start: unable to fcnth( status_pipe, F_SETFD, FD_CLOEXEC ) in child process" );
            }
            m_in_pipe.close_w();
            m_out_pipe.close_r();
            m_err_pipe.close_r();
            if( ::close( 0 )==-1 ) {
                throw os_error_t( "exec_stream_t::start: unable to close( 0 ) in child process" );
            }
            if( fcntl( m_in_pipe.r(), F_DUPFD, 0 )==-1 ) {
                throw os_error_t( "exec_stream_t::start: unable to fcntl( .., F_DUPFD, 0 ) in child process" );
            }
            if( ::close( 1 )==-1 ) {
                throw os_error_t( "exec_stream_t::start: unable to close( 1 ) in child process" );
            }
            if( fcntl( m_out_pipe.w(), F_DUPFD, 1 )==-1 ) {
                throw os_error_t( "exec_stream_t::start: unable to fcntl( .., F_DUPFD, 1 ) in child process" );
            }
            if( ::close( 2 )==-1 ) {
                throw os_error_t( "exec_stream_t::start: unable to close( 2 ) in child process" );
            }
            if( fcntl( m_err_pipe.w(), F_DUPFD, 2 )==-1 ) {
                throw os_error_t( "exec_stream_t::start: unable to fcntl( .., F_DUPFD, 2 ) in child process" );
            }
            m_in_pipe.close_r();
            m_out_pipe.close_w();
            m_err_pipe.close_w();
            if( execvp( m_child_args.data(), m_child_argp.data() )==-1 ) {
                throw os_error_t( "exec_stream_t::start: exec in child process failed. "+program );
            }
            throw exec_stream_t::error_t( "exec_stream_t::start: exec in child process returned" );
        }catch( std::exception const & e ) {
            const char * msg=e.what();
            std::size_t len=strlen( msg );
            write( status_pipe.w(), &len, sizeof( len ) );
            write( status_pipe.w(), msg, len );
            _exit( -1 );
        }catch( ... ) {
            char * msg="exec_stream_t::start: unknown exception in child process";
            std::size_t len=strlen( msg );
            write( status_pipe.w(), &len, sizeof( len ) );
            write( status_pipe.w(), msg, len );
            _exit( 1 );
        }
    }else {
        m_child_pid=pid;
        status_pipe.close_w();
        fd_set status_fds;
        FD_ZERO( &status_fds );
        FD_SET( status_pipe.r(), &status_fds );
        struct timeval timeout;
        timeout.tv_sec=3;
        timeout.tv_usec=0;
        if( select( status_pipe.r()+1, &status_fds, 0, 0, &timeout )==-1 ) {
            throw os_error_t( "exec_stream_t::start: select on status_pipe failed" );
        }
        if( !FD_ISSET( status_pipe.r(), &status_fds ) ) {
            throw os_error_t( "exec_stream_t::start: timeout while waiting for child to report via status_pipe" );
        }
        std::size_t status_len;
        int status_nread=read( status_pipe.r(), &status_len, sizeof( status_len ) );
        // when all ok, status_pipe is closed on child's exec, and nothing is written to it
        if( status_nread!=0 ) {
            // otherwize, check what went wrong.
            if( status_nread==-1 ) {
                throw os_error_t( "exec_stream_t::start: read from status pipe failed" );
            }else if( status_nread!=sizeof( status_len ) ) {
                throw os_error_t( "exec_stream_t::start: unable to read length of status message from status_pipe" );
            }
            std::string status_msg;
            if( status_len!=0 ) {
                buf_t< char > status_buf;
                status_buf.new_data( status_len );
                status_nread=read( status_pipe.r(), status_buf.data(), status_len );
                if( status_nread==-1 ) {
                    throw os_error_t( "exec_stream_t::start: readof status message from status pipe failed" );
                }
                status_msg.assign( status_buf.data(), status_len );
            }
            throw exec_stream_t::error_t( "exec_stream_t::start: error in child process."+status_msg );
        }
        status_pipe.close_r();

        m_in_pipe.close_r();
        m_out_pipe.close_w();
        m_err_pipe.close_w();

        if( fcntl( m_in_pipe.w(), F_SETFL, O_NONBLOCK )==-1 ) {
            throw os_error_t( "exec_stream_t::start: fcntl( in_pipe, F_SETFL, O_NONBLOCK ) failed" );
        }

        m_in_buffer.clear();
        m_out_buffer.clear();
        m_err_buffer.clear();

        m_in.clear();
        m_out.clear();
        m_err.clear();
        
        m_thread.set_read_buffer_size( exec_stream_t::s_out, STREAM_BUFFER_SIZE );
        m_thread.set_read_buffer_size( exec_stream_t::s_err, STREAM_BUFFER_SIZE );
        m_thread.start();
    }
}

bool exec_stream_t::close_in()
{
    m_impl->m_thread.close_in();
    return true;
}

bool exec_stream_t::close()
{
    close_in();
    if( !m_impl->m_thread.stop_thread() ) {
        m_impl->m_thread.abort_thread();
    }
    m_impl->m_in_pipe.close();
    m_impl->m_out_pipe.close();
    m_impl->m_err_pipe.close();

    if( m_impl->m_child_pid!=-1 ) {
        pid_t code=waitpid( m_impl->m_child_pid, &m_impl->m_exit_code, WNOHANG );
        if( code==-1 ) {
            throw os_error_t( "exec_stream_t::close: first waitpid failed" );
        }else if( code==0 ) {

            struct timeval select_timeout;
            select_timeout.tv_sec=m_impl->m_child_timeout/1000;
            select_timeout.tv_usec=(m_impl->m_child_timeout%1000)*1000;
            if( (code=select( 0, 0, 0, 0, &select_timeout ))==-1 ) {
                throw os_error_t( "exec_stream_t::close: select failed" );
            }

            code=waitpid( m_impl->m_child_pid, &m_impl->m_exit_code, WNOHANG );
            if( code==-1 ) {
                throw os_error_t( "exec_stream_t::close: second waitpid failed" );
            }else if( code==0 ) {
                return false;
            }else {
                m_impl->m_child_pid=-1;
                return true;
            }
                    
        }else {
            m_impl->m_child_pid=-1;
            return true;
        }
    }
    return true;
}

void exec_stream_t::kill()
{
    if( m_impl->m_child_pid!=-1 ) {
        if( ::kill( m_impl->m_child_pid, SIGKILL )==-1 ) {
            throw os_error_t( "exec_stream_t::kill: kill failed" );
        }
        m_impl->m_child_pid=-1;
        m_impl->m_exit_code=0;
    }
}

int exec_stream_t::exit_code()
{
    if( m_impl->m_child_pid!=-1 ) {
        throw exec_stream_t::error_t( "exec_stream_t::exit_code: child process still running" );
    }
    return WEXITSTATUS( m_impl->m_exit_code );
}

void exec_stream_t::set_binary_mode( int )
{
}

void exec_stream_t::set_text_mode( int )
{
}
