/**
 * Copyright (c) 2016, Peixu Zhu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTR/ACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * */

#include    <arpa/inet.h>
#include    <cerrno>
#include    <cassert>
#include    <csignal>
#include    <cstring>
#include    "lew/wrapper.h"

using namespace std;
NS_LEW_BEGIN();


static  bool    _init_lib   = false;

static  bool
init_lib(){
    int     ret = 0;
    if (! _init_lib ){
        _init_lib   = true;
        ret         = evthread_use_pthreads();
    }
    return (0 == ret);
}


static  int
get_host_port(int fd, std::string&  remote, uint16_t&   port){
    union{
        struct  sockaddr_in     addr;
        struct  sockaddr_in6    addr6;
    }uaddr;
    socklen_t       len     = sizeof(uaddr);
    int             ret     = getpeername(fd, (struct sockaddr*)&uaddr, &len);
    if ( 0 == ret ){
        char        ip[64];
        ip[0]   = '\0';
        if ( sizeof(struct sockaddr_in)){
            port    = (uint16_t)ntohs(uaddr.addr.sin_port);
            inet_ntop( AF_INET, &uaddr.addr.sin_addr.s_addr, ip, len);
        }
        else{
            port    = (uint16_t)ntohs(uaddr.addr6.sin6_port);
            inet_ntop( AF_INET6, &uaddr.addr6.sin6_addr, ip, len);
        }
        remote  = ip;
    }
    return  ret;
}

static void
_signal_cb(evutil_socket_t  fd, short  what, void* arg){
    Wrapper*    wrapper     = (Wrapper*)arg;
    wrapper->onSignal( fd );
}

void
_timer_cb(int   s,  short what,  void* arg){
    Timer*      t   = (Timer*)arg;
    if (t){
        Wrapper*            owner       = (Wrapper*)t->owner;
        timer_handler_t     handler     = t->handler;
        if (owner && handler){
            (owner->*handler)(t, t->args);
        }
        owner->_timerSet.erase( t );
        delete  t;
    }
}

void
_event_cb(struct bufferevent*   bev, short  evt, void* ctx ){
    Connection*             conn    = (Connection*)ctx;
    Wrapper*                wrapper = conn->owner();
    bool                    is_live = true;
    ConnectionSet::iterator it      =
        wrapper->tcpServerConnectionSet().find( conn );
    if (it == wrapper->tcpServerConnectionSet().end() ){
        it = wrapper->tcpClientConnectionSet().find( conn );
        if ( it == wrapper->tcpClientConnectionSet().end() ){
            is_live = false;
        }
    }
    if ( is_live ){
        if (evt & (BEV_EVENT_EOF | BEV_EVENT_ERROR) ){
            bool    remove_conn = true;
            bool    reconnect   =
                (conn->retryTimes() &&
                 conn->type() ==Connection::CONN_TCP_CLIENT);
            if (reconnect){
                if (conn->bev() ){
                    bufferevent_free(conn->bev() );
                    conn->setBev( nullptr );
                }
                conn->_status   = Connection::DISCONNECTED;
                remove_conn     = (wrapper->tcpClientReconnect( conn ) != 0 );
            }
            if (remove_conn){
                if (conn->type() == Connection::CONN_TCP_CLIENT ){
                    wrapper->tcpClientConnectionSet().erase( conn );
                }
                else{
                    wrapper->tcpServerConnectionSet().erase( conn );
                }
                delete  conn;
            }
        }
        if (evt & BEV_EVENT_CONNECTED){
            conn->_status   = Connection::CONNECTED;
        }
    }
}

static void
_read_cb( struct bufferevent*   bev, void* ctx){
    Connection* conn    = (Connection*)ctx;
    Wrapper*    wrapper = conn->owner();
    wrapper->onConnectionRead( conn );
}

static void
_write_cb(struct bufferevent*   bev, void* ctx){
    Connection* conn    = (Connection*)ctx;
    Wrapper*    wrapper = conn->owner();
    wrapper->onConnectionWrite( conn );
}

void
_listen_cb( struct evconnlistener*      listener,
            evutil_socket_t             fd,
            struct sockaddr*            sock,
            int                         socklen,
            void*                       ctx) {
    Wrapper*    wrapper     = (Wrapper*)ctx;
    int         flag        =
        BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE;
    struct event_base*  base= wrapper->base();
    struct bufferevent* bev = bufferevent_socket_new( base, fd, flag );
    //
    assert( bev );
    uint16_t        port;
    string          remote_ip;
    get_host_port(fd, remote_ip, port);
    Connection*     conn    = new Connection(
        wrapper, Connection::CONN_TCP_SERVER, remote_ip.c_str(), port);
    bufferevent_setcb(bev, _read_cb, _write_cb, _event_cb, conn);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
    conn->setBev( bev );
    wrapper->tcpServerConnectionSet().insert( conn );
    wrapper->onNewConnection( conn );
}

static void
_listen_error_cb(struct evconnlistener* listener, void* ctx){
    fprintf(stderr, "%s listener %p\n", __FUNCTION__, listener);
}

static void
_http_client_close_cb( struct evhttp_connection*    evconn, void*  ctx){
    Connection*     conn    = (Connection*)ctx;
    Wrapper*        wrapper = conn->owner();
    ConnectionSet&  cs      = wrapper->httpServerConnectionSet();
    if (cs.find( conn ) != cs.end() ){
        delete conn;
        wrapper->httpServerConnectionSet().erase( conn );
    }
}

static void
_http_req_close_cb( struct evhttp_request* req, void* ctx){
    Connection*     conn    = (Connection*)ctx;
    Wrapper*        wrapper = conn->owner();
    wrapper->onHttpResponse(conn, conn->httpReq() );
}

void
_http_req_cb( struct evhttp_request*  req, void * ctx){
    Wrapper*        wrapper = (Wrapper*)ctx;
    struct evhttp_connection*   http_conn =
        evhttp_request_get_connection( req );
    string          ip_addr = "";
    uint16_t        port    = 0;
    struct bufferevent* bev = nullptr;
    if (http_conn){
        char*   addr    = 0;
        evhttp_connection_get_peer(http_conn, &addr, &port);
        if (addr) ip_addr   = addr;
        bev     = evhttp_connection_get_bufferevent( http_conn );
        bufferevent_enable(bev, EV_READ);
    }
    Connection* conn = new Connection( wrapper, Connection::CONN_HTTP_SERVER,
                                       ip_addr.c_str(), port);
    conn->setHttpReq( req );
    if (http_conn){
        evhttp_connection_set_closecb(http_conn, _http_client_close_cb, conn );
    }
    wrapper->httpServerConnectionSet().insert( conn );
    wrapper->onHttpRequest(conn, req);
}

///////////////////////////////////
Timer::~Timer(){
    if (evt){
        event_del( evt );
        event_free( evt );
    }
}


///////////////////////////////////
static ConstructException   _constructException;
Wrapper::Wrapper(){
    if (! init_lib() ){
        throw _constructException;
    };
    _base       = event_base_new();
    if (! _base)    throw _constructException;
    _started    = false;
    _stopped    = false;
    memset(_sig_events, 0, sizeof(_sig_events) );
}

Wrapper::~Wrapper(){
    for( auto t : _timerSet ){
        delete t;
    }
    if (_base){
        event_base_free( _base );
        _base   = nullptr;
    }
}

Timer*
Wrapper::addTimer(int   ms, timer_handler_t handler, void* arg){
    Timer*  newTimer    = new Timer();
    newTimer->args      = arg;
    newTimer->handler   = handler;
    newTimer->owner     = this;
    newTimer->evt       = evtimer_new( _base, _timer_cb, newTimer);
    if ( ! newTimer->evt ){
        delete  newTimer;
        newTimer    = nullptr;
    }
    else{
        struct timeval  tv;
        tv.tv_sec       = ms / 1000;
        tv.tv_usec      = (ms % 1000) * 1000;
        if (evtimer_add( newTimer->evt, &tv) != 0 ){
            delete newTimer;
            newTimer    = nullptr;
        }
        else{
            _timerSet.insert( newTimer );
        }
    }
    return newTimer;
}

bool
Wrapper::delTimer(Timer* timer){
    int         ret = 0;
    TimerSet::iterator  it  = _timerSet.find( timer );
    if ( it != _timerSet.end() ){
        delete  *it;
        _timerSet.erase( it );
    }
    else{
        ret--;
        errno   = ENOENT;
    }
    return ( 0 == ret);
}

bool
Wrapper::start(){
    int         ret = false;
    if ( ! _started ){
        ret     = onStart();
    }
    else{
        errno   = EEXIST;
    }
    if ( ret  && _base ){
        _started    = true;
        _sig_events[ SIGTERM ] =evsignal_new(_base, SIGTERM, _signal_cb, this);
        _sig_events[ SIGQUIT ] =evsignal_new(_base, SIGQUIT, _signal_cb, this);
        _sig_events[ SIGUSR1 ] =evsignal_new(_base, SIGUSR1, _signal_cb, this);
        _sig_events[ SIGUSR2 ] =evsignal_new(_base, SIGUSR2, _signal_cb, this);
        _sig_events[ SIGINT  ] =evsignal_new(_base, SIGINT,  _signal_cb, this);

        evsignal_add( _sig_events[ SIGTERM ], NULL);
        evsignal_add( _sig_events[ SIGQUIT ], NULL);
        evsignal_add( _sig_events[ SIGUSR1 ], NULL);
        evsignal_add( _sig_events[ SIGUSR2 ], NULL);
        evsignal_add( _sig_events[ SIGINT  ], NULL);
        event_base_dispatch( _base );
    }

    return ret;
}

bool
Wrapper::stop(){
    int     ret     = 0;
    if ( ! _stopped ){
        _stopped    = true;
        onStop();
        ret     = event_base_loopbreak( _base );
    }
    return ( 0 == ret);
}

void
Wrapper::clean(){
    if ( _started && ! _stopped){
        stop();
    }
    for( auto& listener : _lev ){
        evconnlistener_free( listener );
    }
    _lev.resize( 0 );
    //
    for( size_t  i = 0; i < sizeof(_sig_events)/sizeof(_sig_events[0]); i++){
        if ( _sig_events[i] ){
            evsignal_del( _sig_events[i] );
            event_free( _sig_events[i] );
            _sig_events[i]  = NULL;
        }
    }
    //
    for( auto& h : _http){
        evhttp_free( h );
    }
    _http.resize( 0 );
    //
    stopTcpServer();
    stopTcpClient();
    stopHttpServer();
    stopHttpClient();
}

bool
Wrapper::startTcpServer( string  listenAddr, uint16_t    port){
    bool                ret     = -1;
    unsigned            flag    =
        LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE | LEV_OPT_THREADSAFE;
    int                 socklen;
    struct sockaddr*    addr    = NULL;
    struct sockaddr_in  sock;
    //
    memset(&sock, 0, sizeof(sock) );
    sock.sin_family     = AF_INET;
    sock.sin_port       = htons( port );
    if (inet_pton(AF_INET, listenAddr.c_str(), &sock.sin_addr.s_addr) > 0){
        socklen = sizeof(sock);
        addr    = (struct sockaddr*)&sock;
    }
    struct evconnlistener*  lev     = NULL;
    if (addr){
        lev     = evconnlistener_new_bind(
                    _base, _listen_cb, this, flag, -1, addr, socklen);
    }
    if (lev){
        ret     = true;
        evconnlistener_set_error_cb(lev, _listen_error_cb );
        _lev.push_back( lev );
    }
    return ret;
}

Connection*
Wrapper::startTcpClient( string remoteAddr, uint16_t port ){
    int                 ret;
    int                 options = BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE;
    struct bufferevent* bev     = bufferevent_socket_new(_base, -1, options);
    Connection*         conn    = nullptr;
    if ( bev ){
        ret     = bufferevent_socket_connect_hostname(
                      bev, NULL, AF_INET, remoteAddr.c_str(), port);
        if (ret < 0){
            bufferevent_free(bev);
            bev     = nullptr;
        }
        else{
            conn    = new Connection(this, Connection::CONN_TCP_CLIENT,
                                     remoteAddr.c_str(), port);
            conn->setBev( bev );
            bufferevent_enable( bev, EV_READ | EV_WRITE );
            bufferevent_setcb( bev, _read_cb, _write_cb, _event_cb, conn);
            _tcpClientConnectionSet.insert( conn );
            onNewConnection( conn );
        }
    }
    return conn;
}
int
Wrapper::tcpClientReconnect( Connection* conn ){
    int     ret     = -1;
    int     options = BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE;
    conn->setRetryTimes( conn->retryTimes() - 1);
    conn->_status   = Connection::CONNECTING;
    if(conn->bev() ){
        bufferevent_free( conn->bev() );
        conn->setBev( nullptr );
    }
    struct bufferevent*     bev = bufferevent_socket_new(_base, -1, options);
    if (bev){
        ret     = bufferevent_socket_connect_hostname(conn->bev(), NULL,
                     AF_INET, conn->_addr.c_str(), conn->_port );
        if ( ret < 0 ){
            if ( conn->bev() ){
                bufferevent_free( conn->bev() );
            }
            conn->setBev( nullptr );
        }
        else{
            bufferevent_setcb( bev,
                              _read_cb, _write_cb, _event_cb, conn);
            conn->setBev( bev );
        }
    }
    return ret;
}

bool
Wrapper::startHttpServer( string listenAddr, uint16_t port){
    int             ret     = -1;
    struct evhttp*  http    = evhttp_new( _base );
    if (http ){
        evhttp_set_gencb( http, _http_req_cb, this);
        ret     = evhttp_bind_socket( http, listenAddr.c_str(), port);
        _http.push_back( http );
    }
    return ( 0 == ret );
}

Connection*
Wrapper::startHttpClient( string remoteAddr, uint16_t port, string localAddr){
    Connection* conn = nullptr;
    struct evhttp_connection*   evhc =
        evhttp_connection_base_new( _base, NULL, remoteAddr.c_str(), port);
    if (evhc){
        evhttp_connection_set_local_address(evhc, localAddr.c_str() );
        conn    = new Connection( this, Connection::CONN_HTTP_CLIENT,
                                  remoteAddr.c_str(), port );
        conn->_httpConn = evhc;
        evhttp_connection_set_closecb( evhc, _http_client_close_cb, conn);
        evhttp_connection_set_timeout( evhc, 10 );
        struct evhttp_request*  req =
            evhttp_request_new( _http_req_close_cb, conn );
        conn->setHttpReq( req );
        _httpClientConnectionSet.insert( conn );
        onNewConnection( conn );
    }
    return conn;
}

#define     _CLEAN_CONNECTION_SET( cs )                             \
    for( auto& c : cs ){                                            \
        delete  c;                                                  \
    }                                                               \
    cs.clear();

void
Wrapper::stopTcpServer(){
    for( auto& listener : _lev ){
        evconnlistener_free( listener );
    }
    _lev.resize( 0 );
    _CLEAN_CONNECTION_SET( _tcpServerConnectionSet );
}

void
Wrapper::stopTcpClient(){
    _CLEAN_CONNECTION_SET( _tcpClientConnectionSet );
}

void
Wrapper::stopHttpServer(){
    for( auto& h : _http ){
        evhttp_free( h );
    }
    _http.resize( 0 );
    _CLEAN_CONNECTION_SET( _httpServerConnectionSet );
};

void
Wrapper::stopHttpClient(){
    _CLEAN_CONNECTION_SET( _httpClientConnectionSet );
}

int
Wrapper::makeHttpRequest(   Connection*     conn,
                            evhttp_cmd_type cmd,
                            const char*     uri){
    int     ret =
        evhttp_make_request( conn->_httpConn, conn->_httpReq, cmd, uri );
    if (ret < 0){
        conn->setHttpReq( nullptr );
    }
    return  ret;
}

NS_LEW_END();

