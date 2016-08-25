#include    <unistd.h>
#include    <cstdio>
#include    <cstring>

#include    "lew/wrapper.h"
#include    "gtest/gtest.h"

using   namespace   std;
using   namespace   lew;

struct  stat_data_t{
    int     start;
    int     stop;
    int     signal;
    int     new_conn;
    int     conn_close;
    int     conn_connected;
    int     conn_disconnected;
    int     conn_read;
    int     conn_write;
    int     timer;
    int     tcp_client_start;
    int     tcp_client_conn;
    int     tcp_server_conn;
    int     http_client_start;
    int     http_client_conn;
    int     http_server_conn;
}stat_data;

class   TcpServer  : public Wrapper{
public:
    TcpServer(){ memset(&stat_data, 0, sizeof(stat_data)); };
    virtual ~TcpServer(){};

    virtual bool onStart(){
        stat_data.start++;
        printf( "%s\n", __FUNCTION__ );
        return  true;
    };
    virtual void    onStop(){
        stat_data.stop++;
        printf( "%s\n", __FUNCTION__ );
    };
    virtual void    onSignal( int  sig ){
        stat_data.signal++;
        printf( "%s\n", __FUNCTION__ );
        stop();
    }
    virtual void    onNewConnection(Connection*      conn){
        printf( "%s %p\n", __FUNCTION__, conn );
        stat_data.new_conn++;
        if (conn->type() == Connection::CONN_TCP_SERVER){
            assert( conn->status() == Connection::CONNECTED );
            stat_data.tcp_server_conn++;
            evbuffer_add_printf( conn->writeBuf(), "server: hello from %p", conn );
            stat_data.tcp_client_conn++;
        }
    };
    virtual void    onConnectionClose(    Connection*      conn){
        printf( "%s %p\n", __FUNCTION__, conn );
        stat_data.conn_close++;
    };
    virtual void    onConnectionRead(     Connection*      conn){
        printf( "%s %p\n", __FUNCTION__, conn );
        stat_data.conn_read++;
        char        buf[128];
        int         len = evbuffer_get_length( conn->readBuf());
        evbuffer_remove( conn->readBuf(), buf, sizeof(buf));
        buf[len]    = '\0';
        printf("conn %p recv: %s\n", conn, buf);
    };
    virtual void    onConnectionWrite(    Connection*      conn){
        printf( "%s %p\n", __FUNCTION__, conn );
        stat_data.conn_write++;
    };
    void    onHttpResponse(Connection* conn, struct evhttp_request* req){
        printf( "%s %p\n", __FUNCTION__, conn );
        stat_data.http_client_conn++;
        char        buf[128];
        int         len = evbuffer_get_length( conn->readBuf() );
        evbuffer_remove( conn->readBuf(), buf, sizeof(buf));
        buf[len]    = '\0';
        printf( "resp URI=%s\n%s\n", evhttp_request_get_uri(req), buf);
        assert( strcmp(buf, "*^_^*") == 0);
    }
    void    onHttpRequest(Connection* conn, struct evhttp_request* req){
        printf( "%s %p\n", __FUNCTION__, conn );
        stat_data.http_server_conn++;
        printf( "recv  URI=%s\n", evhttp_request_get_uri(req));
        struct evbuffer*    buf = evbuffer_new();
        evbuffer_add_printf(buf, "%s", "*^_^*");
        evhttp_send_reply(conn->httpReq(), 200, "http-server: hello", buf);
        evbuffer_free( buf );
    }
    //
    void    onTimer( Timer* tmr, void * arg){
        printf( "%s\n", __FUNCTION__ );
        assert((uintptr_t)arg   == 0x12345678 );
        stat_data.timer++;
    }
    void    onStopTimer( Timer*  tmr,    void*   arg){
        printf( "%s\n", __FUNCTION__ );
        stop();
    }
    //  start try to connect.
    void    onTryToConnect(Timer* tmr, void* arg){
        TcpServer*     to  = (TcpServer*)arg;
        //
        if (to->startTcpClient("127.0.0.1", 9988 ) != 0){
            stat_data.tcp_client_start++;
        }
        else{
            stat_data.tcp_client_start--;
        }
    }
    //  start try to send HTTP request
    void    onTryToRequest(Timer* tmr, void* arg){
        TcpServer*         to      = (TcpServer*)arg;
        Connection*  conn    = 0;
        printf( "%s\n", __FUNCTION__);
        //
        if ((conn = to->startHttpClient("127.0.0.1", 9988, "127.0.0.1"))){
            stat_data.http_client_start++;
            to->makeHttpRequest(conn, EVHTTP_REQ_GET, "/hello_ip4.html");
        }
        else{
            stat_data.http_client_start--;
        }
    }
};

TEST(TcpServer,  start_stop){
    TcpServer*     to  = new TcpServer();
    ASSERT_TRUE( to != NULL );
    to->addTimer(100, (timer_handler_t)&TcpServer::onStopTimer, 0);
    to->start();
    to->start();
    to->stop();
    to->stop();
    to->clean();
    delete  to;
    //
    ASSERT_EQ( stat_data.start,     1);
    ASSERT_EQ( stat_data.stop,      1);
}

TEST(TcpServer,  add_del_timer){
    TcpServer*     to  = new TcpServer();
    ASSERT_TRUE( to != NULL );
    to->addTimer(1000 * 2, (timer_handler_t)&TcpServer::onStopTimer, 0);
    uintptr_t       arg = 0x12345678;
    Timer*   tmr = 0;
    EXPECT_TRUE( (tmr = to->addTimer(100, (timer_handler_t)&TcpServer::onTimer, (void*)arg)) != nullptr );
    EXPECT_TRUE( to->delTimer( tmr ) );
    EXPECT_TRUE( (tmr = to->addTimer(100, (timer_handler_t)&TcpServer::onTimer, (void*)arg)) != nullptr );
    to->start();
    EXPECT_FALSE( to->delTimer( tmr ) );
    to->stop();
    to->clean();
    delete  to;
    //
    EXPECT_EQ(  stat_data.timer,    1);
}

TEST(TcpServer,  start_server){
    TcpServer*     to  = new TcpServer();
    to->addTimer(2000,(timer_handler_t)&TcpServer::onStopTimer, 0);
    to->addTimer(200 ,(timer_handler_t)&TcpServer::onTryToConnect, to);
    EXPECT_TRUE( to->startTcpServer("127.0.0.1", 9988) );
    to->start();
    to->clean();
    delete  to;
    //
    EXPECT_EQ(  stat_data.tcp_client_start,     1);
    EXPECT_EQ(  stat_data.new_conn,             2);
    EXPECT_EQ(  stat_data.conn_close,           2);
    EXPECT_EQ(  stat_data.tcp_client_conn,      1);
    EXPECT_EQ(  stat_data.tcp_server_conn,      1);
    EXPECT_EQ(  stat_data.conn_write,           1);
}

TEST(HttpServer,   start_http){
    TcpServer*     to  = new TcpServer();
    to->addTimer(2000, (timer_handler_t)&TcpServer::onStopTimer, 0);
    to->addTimer(200 , (timer_handler_t)&TcpServer::onTryToRequest, to);
    EXPECT_TRUE( to->startHttpServer("127.0.0.1", 9988) );
    to->start();
    to->stop();
    to->clean();
    delete      to;
    //
    EXPECT_EQ(  stat_data.http_client_start,    1);
    EXPECT_EQ(  stat_data.http_server_conn,     1);
    EXPECT_EQ(  stat_data.http_client_conn,     1);
}


