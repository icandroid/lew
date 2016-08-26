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
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * */
#ifndef LEW_WRAPPER_H
#define LEW_WRAPPER_H

#include    <cstdint>
#include    <exception>
#include    <vector>
#include    <string>
#include    <unordered_set>

#include    <event2/event.h>
#include    <event2/buffer.h>
#include    <event2/bufferevent.h>
#include    <event2/http.h>
#include    <event2/thread.h>
#include    <event2/util.h>
#include    <event2/listener.h>

#include    "lew/connection.h"

NS_LEW_BEGIN();

class   Wrapper;
class   Timer;

typedef     std::unordered_set<Connection*>     ConnectionSet;
typedef     void(Wrapper::*timer_handler_t)(Timer* timer, void* arg);

class   Timer{
public:
    Timer(){
        owner   = nullptr;
        handler = nullptr;
        args    = nullptr;
        evt     = nullptr;
    }
    ~Timer();
    Wrapper*            owner;      // the wrapper who owns the timer.
    timer_handler_t     handler;    // the handler to be called.
    void*               args;       // optional data.
    struct event*       evt;        // internal event used.
};

class   ConstructException: public std::exception{
public:
    virtual const char* what() const throw(){
        return "fail to constuct libevent wrapper";
    }
};

/**
 *  \note   the wrapper of libevent. <br>
 *          the class simply works on libevent, offering services of <br>
 *              tcp server, tcp client, http server, http client. <br>
 *
 * */
class   Wrapper{
public:
    Wrapper();
    virtual ~Wrapper();

    /**
     * \note    start the wrapper.
     * \return  true on success, or false on failure.
     * */
    bool    start();

    /**
     *  \note   stop the wrapper.
     *  \return true on success, or false on failure.
     * */
    bool    stop();

    /**
     * \note    clear all servers/connections.
     * */
    void    clean();


    /**
     * \note    start a tcp server.
     * \param   listenAddr  the listening address, must be IPv4.
     * \param   port        the listening port.
     * \return  true on success, or false on failure.
     * */
    bool            startTcpServer( std::string     listenAddr,
                                    uint16_t        port );
    /**
     * \note    start a tcp client connection.
     * \param   remoteAddr  the IPv4 address of remote server.
     * \param   port        the port of remote server.
     * \return  the connection on success, or nullptr on failure.
     * */
    Connection*     startTcpClient( std::string     remoteAddr,
                                    uint16_t        port );
    /**
     * \note    start a http server.
     * \param   listenAddr  the listening address, must be IPv4.
     * \param   port        the listening port.
     * \return  true on success, or false on failure
     * */
    bool            startHttpServer(std::string     listenAddr,
                                    uint16_t        port);
    /**
     * \note    start a http connection to remote host.
     * \param   remoteAddr  the IPv4 address of remote http server.
     * \param   port        the port of remote http server.
     * \return  the connection on success, or nullptr on failure.
     * */
    Connection*     startHttpClient(std::string     remoteAddr,
                                    uint16_t        port,
                                    std::string     localAddr);

    void            stopTcpServer();
    void            stopTcpClient();
    void            stopHttpServer();
    void            stopHttpClient();

    /**
     * \note    make a new http request on an http client connection.
     * \param   conn        the connection created by startHttpClient.
     * \param   cmd         the http command type.
     * \param   uri         the URI of the request.
     * \return  0 on success, or others on failure.
     * */
    int             makeHttpRequest(Connection*     conn,
                                    evhttp_cmd_type cmd,
                                    const char*     uri);

    /**
     * \note    callback method called when the wrapper is started.
     * */
    virtual bool    onStart(){ return true; };

    /**
     * \note    callback method called when the wrapper is stopped.
     * */
    virtual void    onStop(){};

    /**
     * \note    callback method called when the program is signaled.
     * */
    virtual void    onSignal(   int     signo){};

    /**
     * \note    callback method called when a new connection is created.
     * */
    virtual void    onNewConnection(    Connection* conn){};

    /**
     * \note    callback method called when a connection has data to read.
     * */
    virtual void    onConnectionRead(   Connection* conn){};

    /**
     * \note    callback method called when a connection is ready to write.
     * */
    virtual void    onConnectionWrite(  Connection* conn){};

    /**
     * \note    callback method called when a connection is closed.<br>
     *          NOTICE that the method may be called more than once.
     * */
    virtual void    onConnectionClose(  Connection* conn){};

    /**
     * \note    callback method called when a http server connection receives
     *          a request.
     * */
    virtual void    onHttpRequest(Connection* conn, struct evhttp_request* req){};

    /**
     * \note    callback method called when a http client connection receives
     *          response from http server.
     * */
    virtual void    onHttpResponse(Connection* conn, struct evhttp_request* req){};

    friend void     _event_cb( struct bufferevent*  bev, short evt, void* ctx);
    friend void     _timer_cb( int  s, short what, void* arg);

public:
    /**
     * \note    create a timer.
     * \param   ms          milliseconds to delay.
     * \param   handler     the method to be called when the timer is triggered.
     * \param   arg         optional data to be transfered to the handler.
     * \return  a new timer on success, or nullptr on failure.
     * */
    Timer*      addTimer(int ms, timer_handler_t handler, void* arg);
    /**
     * \note    remove/cancel a timer.
     * \param   timer       the timer created by 'addTimer'.
     * \return  true on success, or false on failure.
     * */
    bool        delTimer(Timer*             timer);

public:
    ConnectionSet&  tcpServerConnectionSet(){ return _tcpServerConnectionSet; };
    ConnectionSet&  tcpClientConnectionSet(){ return _tcpClientConnectionSet; };
    ConnectionSet&  httpServerConnectionSet(){return _httpServerConnectionSet;};
    ConnectionSet&  httpClientConnectionSet(){return _httpClientConnectionSet;};
    struct event_base*      base(){ return _base; };

protected:
    struct event_base*      _base;
    ConnectionSet           _tcpServerConnectionSet;
    ConnectionSet           _tcpClientConnectionSet;
    ConnectionSet           _httpServerConnectionSet;
    ConnectionSet           _httpClientConnectionSet;
protected:
    typedef std::unordered_set<Timer*>      TimerSet;
    TimerSet                                _timerSet;
    std::vector<struct evconnlistener*>     _lev;
    std::vector<struct evhttp*>             _http;
    bool                                    _started;
    bool                                    _stopped;
    struct event*                           _sig_events[256];
    //
    int             tcpClientReconnect( Connection* conn );

};


NS_LEW_END();

#endif


