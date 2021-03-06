/**
 *  \note   C10K test.
 * */
#include <sys/socket.h>
#include <cstdio>
#include <csignal>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <memory>
#include <exception>
#include <string>
#include "lew/wrapper.h"
#include "Flags.hpp"

using   namespace   std;

class   C10KServer  : public lew::Wrapper {
public:
    C10KServer() {
        count_connect   = 0;
        count_read      = 0;
    };
    virtual ~C10KServer(){};
    virtual void onNewConnection( lew::Connection*  conn);
    virtual void onConnectionRead( lew::Connection* conn);
    virtual void onSignal( int signo );
public:
    int         count_connect;
    int         count_read;
};

void
C10KServer::onNewConnection( lew::Connection*   conn ){
    evutil_socket_t fd  = bufferevent_getfd( conn->bev() );
    int     buf_size    = 0;
    socklen_t   socklen;
    if ( getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&buf_size, &socklen)!=0){
        perror("fail to get socket buffer length\n");
    }
    evbuffer_add_printf( conn->writeBuf(), "ping client %d", count_connect);
    count_connect++;
}

void
C10KServer::onSignal( int signo ){
    if (signo == SIGTERM || signo == SIGINT || signo == SIGKILL){
        this->stopTcpServer();
        this->stop();
        this->clean();
    }
}

void
C10KServer::onConnectionRead( lew::Connection*  conn){
#define BUF_SIZE    1024
    char    buf[BUF_SIZE];
    evbuffer_remove(conn->readBuf(), buf, BUF_SIZE);
    if (strstr(buf, "pong") == buf){
        count_read++;
    }
}

int main(int argc, char* argv[]){
#define     DEFAULT_HOST        "127.0.0.1"
#define     DEFAULT_PORT        7000

    int     port        = DEFAULT_PORT;
    string  listen_addr = DEFAULT_HOST;

    Flags   opts;

    opts.Var(listen_addr, 'l', "listen", string(DEFAULT_HOST),
             "listen address, default to " DEFAULT_HOST);
    opts.Var(port, 'p', "port", int(port), "listen port, default to 7000");
    //
    if (!opts.Parse(argc, argv) ){
        opts.PrintHelp(argv[0]);
        return 1;
    };

    cout << "listen on " << listen_addr << ":" << port << endl;
    cout << "press Ctrl-C to exit" << endl;
    //
    unique_ptr<C10KServer>  server( new C10KServer() );
    server->startTcpServer( listen_addr.c_str(), (unsigned short)port);
    server->start();
    cout << "total # of connection is " << server->count_connect << endl;
    cout << "total # of reading is " << server->count_read << endl;
    //
    return 0;
}

