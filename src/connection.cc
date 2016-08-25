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
#include    "lew/connection.h"

NS_LEW_BEGIN();

Connection::Connection(
                       Wrapper*     owner,
                       Type         type,
                       const char*  addr,
                       uint16_t     port )
    : _owner(owner), _type(type), _addr(addr), _port(port){
    _bev        = nullptr;
    _readBuf    = nullptr;
    _writeBuf   = nullptr;
    _httpConn   = nullptr;
    _retryTimes = 0;

    _status     = (CONN_TCP_CLIENT == type) ? DISCONNECTED : CONNECTED;
}

Connection::~Connection(){
    if (_bev){
        bufferevent_free( _bev );
        _bev    = nullptr;
    }
    if (_httpConn){
        evhttp_connection_free( _httpConn );
        _httpConn = nullptr;
    }
}

void
Connection::setBev( struct bufferevent*     bev){
    if (  bev){
        _bev        = bev;
        _readBuf    = bufferevent_get_input( _bev );
        _writeBuf   = bufferevent_get_output(_bev);
    }
    else{
        _bev        = nullptr;
        _readBuf    = nullptr;
        _writeBuf   = nullptr;
    }
}

void
Connection::setHttpReq( struct evhttp_request*  req){
    if ( req){
        _httpReq    = req;
        _readBuf    = evhttp_request_get_input_buffer( req );
        _writeBuf   = evhttp_request_get_output_buffer(req );
    }
    else{
        _httpReq    = nullptr;
        _readBuf    = nullptr;
        _writeBuf   = nullptr;
    }
}

NS_LEW_END();

