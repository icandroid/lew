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

#ifndef LEW_CONNECTION_H
#define LEW_CONNECTION_H

#include    <cstdint>
#include    <string>

#include    <event2/event.h>
#include    <event2/buffer.h>
#include    <event2/bufferevent.h>
#include    <event2/thread.h>
#include    <event2/http.h>

#include    "lew/utildef.h"

NS_LEW_BEGIN();

class   Wrapper;

/**
 *  \note   (TCP|HTTP) x (SERVER|CLIENT) Connection created on libevent.
 *
 * */
class   Connection {
public:
    friend  class           Wrapper;
    enum    Type {
        CONN_TCP_SERVER         = 0,
        CONN_HTTP_SERVER,
        CONN_TCP_CLIENT,
        CONN_HTTP_CLIENT,
    };
    enum    Status {
        DISCONNECTED            = 0,
        CONNECTING,
        CONNECTED,
    };

    /**
     *  \note   general event callback
     * */
    friend  void    _event_cb(
                                struct bufferevent*    bev,
                                short                  evt,
                                void*                  ctx );

    /**
     *  \note   listen event callback.
     * */
    friend  void    _listen_cb(
                                struct evconnlistener*    listener,
                                evutil_socket_t           fd,
                                struct sockaddr*          sock,
                                int                       socklen,
                                void*                     ctx);
    /**
     *  \note   http request event callback.
     * */
    friend  void    _http_req_cb(
                                struct evhttp_request*   req,
                                void*                    ctx);
public:
    Connection( Wrapper*    owner,  //  the wrapper who owns the connection.
                Type        type,   //  category of the connection.
                const char* addr,   //  remote address
                uint16_t    port ); //  remote port
    virtual ~Connection();
public:
    Wrapper*                owner(){    return _owner;};
    Type                    type(){     return _type;};
    Status                  status(){   return _status;};
    std::string             addr(){     return _addr;};
    uint16_t                port(){     return _port;};
    //
    struct bufferevent*     bev(){      return _bev;};
    struct evbuffer*        readBuf(){  return _readBuf;};
    struct evbuffer*        writeBuf(){ return _writeBuf;};
    struct evhttp_request*  httpReq(){  return _httpReq;};
    /**
     *  \note   get retryTimes. if it's zero, the connection will not try
     *          to reconnect to tcp server when connection is lost.
     * */
    int     retryTimes(){   return _retryTimes;};

    /**
     *  \note   set the retry times. if zero is provided, the connection will
     *          not try to reconnect to tcp server when connection is lost.
     * */
    void    setRetryTimes(int retryTimes){ _retryTimes  = retryTimes;};
protected:
    Wrapper*                _owner;
    Type                    _type;
    Status                  _status;
    std::string             _addr;
    int                     _retryTimes;
    uint16_t                _port;
    struct bufferevent*     _bev;
    struct evbuffer*        _readBuf;
    struct evbuffer*        _writeBuf;
    struct evhttp_request*  _httpReq;
protected:
    void                    setBev(     struct bufferevent*     bev);
    void                    setHttpReq( struct evhttp_request*  req);
    //
    //  http client connection
    struct evhttp_connection*   _httpConn;
};  // class Connection


NS_LEW_END();

#endif

