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
#include "cxxopts.hpp"
#include "lew/wrapper.h"

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

    cxxopts::Options    opts("c10kclient",
                             "c10k client, which tests the capability of "
                             "concurrent connections.");
    int     port        = DEFAULT_PORT;
    int     count       = DEFAULT_COUNT;
    string  host_addr   = DEFAULT_HOST;
    opts.add_options()
        ("h,host", "host address, default to " DEFAULT_HOST,
            cxxopts::value<string>(host_addr))
        ("p,port", "remote port, default to 7000",
            cxxopts::value<int>(port) )
        ("c,connections", "concurrent connections to remote server",
            cxxopts::value<int>(count) );
    try{
        opts.parse(argc, argv);
    }
    catch(exception& e){
        string  help_info   = opts.help( opts.groups() );
        cout << help_info << endl;
        return 1;
    }
    if (opts.count("host"))     host_addr   = opts["host"].as<string>();
    if (opts.count("port"))     port        = opts["port"].as<int>();
    if (opts.count("c"))        count       = opts["c"].as<int>();

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

