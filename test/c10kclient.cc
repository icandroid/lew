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

class   C10KClient  : public lew::Wrapper {
public:
    C10KClient() {
        count_connect   = 0;
        count_connected = 0;
        count_read      = 0;
        done            = 0;
    };
    virtual ~C10KClient(){};
    virtual void onConnectionRead( lew::Connection*  conn);
    virtual void onSignal( int signo );
    //
    void        makeConnections(lew::Timer* timer, void *args);
public:
    int         count_connect;
    string      remote_host;
    int         remote_port;
    int         done;
    int         count_read;
    int         count_connected;
};

void
C10KClient::onConnectionRead( lew::Connection*   conn ){
    evutil_socket_t fd  = bufferevent_getfd( conn->bev() );
    int     buf_size    = 0;
    socklen_t   socklen;
    if ( getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&buf_size, &socklen)!=0){
        perror("fail to get socket buffer length\n");
    }
    count_read++;
    //
    evbuffer_add_printf(conn->writeBuf(), "pong #%d", count_read);
}

void
C10KClient::onSignal( int signo ){
    if (signo == SIGTERM || signo == SIGINT || signo == SIGKILL){
        count_connected = this->tcpClientConnectionSet().size();
        this->stopTcpServer();
        this->stop();
        this->clean();
    }
}

void
C10KClient::makeConnections(lew::Timer* timer, void* args){
    int         i       = 0;
    for( ; done < count_connect && i < 1000; done++, i++){
        startTcpClient( remote_host.c_str(), (unsigned short)remote_port);
    }
    cout << done << " connections done" << endl;
    if (done < count_connect){
        addTimer(100, (lew::timer_handler_t)&C10KClient::makeConnections, nullptr);
    }
    else{
        cout << "press Ctrl-C to exit" << endl;
    }
}



int main(int argc, char* argv[]){
#define     DEFAULT_HOST        "127.0.0.1"
#define     DEFAULT_PORT        7000
#define     DEFAULT_COUNT       1

    int     port        = DEFAULT_PORT;
    int     count       = DEFAULT_COUNT;
    string  host_addr   = DEFAULT_HOST;

    Flags   opts;

    opts.Var(host_addr, 'h', "host", string(DEFAULT_HOST),
             "remote address, default to " DEFAULT_HOST);
    opts.Var(port,      'p', "port", int(port),
             "remote port, default to 7000");
    opts.Var(count,     'c', "count", int(count),
             "count of connections to remote, default to 1");
    //
    if (!opts.Parse(argc, argv) ){
        opts.PrintHelp(argv[0]);
        return 1;
    };

    unique_ptr<C10KClient>  client( new C10KClient() );
    client->remote_host     = host_addr;
    client->remote_port     = port;
    client->count_connect   = count;
    cout << "try to make " << client->count_connect << " connections" << endl;
    client->addTimer(100, (lew::timer_handler_t)&C10KClient::makeConnections, nullptr);
    client->start();
    cout << "read count " << client->count_read << endl;
    cout << "connected # " << client->count_connected << endl;
    //
    return 0;
}

