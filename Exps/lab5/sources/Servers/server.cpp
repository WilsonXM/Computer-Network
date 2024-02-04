#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <winsock2.h>
#include <mutex>
#include "protocol.h"
using namespace std;

#define SER_PORT 2037
#define MAXNUM 1000

typedef struct clientTag {
    //unsigned short num;
    SOCKET clisock;
    struct sockaddr_in cliAddr;
} clientINFO;

//int client_num = 0;
vector<clientINFO> clients;
vector<thread> threads;
mutex mtx;

void thread_handle ( clientINFO client )
{
    // send a "Hello"
    // send( client.clisock, sendData.data(), sendData.length(), 0 );
    // 准备接收各种信息
    char rev[2048];
    int Flag = 1;//判断是否断开连接
    while( Flag )
    {
        if( recv( client.clisock, rev, sizeof(rev), 0) < 0 ) //接收失败
            printf( "accept failed!\n" );
        string revv(rev);
        packet receive = analyzePacket(revv);
        mtx.lock();
        switch ( (int)receive.getType() )
        {
            case 0:{
                printf( "client %d has connected\n", client.clisock );
                break;
            }
            //disconnect
            case 1:{
                for( int i = 0; i < clients.size(); i++ ) 
                    if( clients[i].clisock == client.clisock ) {
                        clients.erase( clients.begin() + i );
                        break;
                    }
                // 关闭该链接
                Flag = 0;
                printf( "Client Closed!\n" );
                break;
            }
            //get time
            case 2:{
                time_t seconds;
                time( &seconds );
                packet mes( 2, client.clisock, to_string(seconds) );
                if( send( client.clisock, pac2str(mes).data(), pac2str(mes).length(), 0 ) < 0 ) {
                    printf( "Time error!\n" );
                }
                printf( "Time sent success!\n" );
                break;
                
            }
            //get name
            case 3:{
                char name[256];
                gethostname( name, sizeof(name) );
                packet mes( 3, client.clisock, name );
                if( send( client.clisock, pac2str(mes).data(), pac2str(mes).length(), 0 ) < 0) {
                    printf( "Name error!\n" );
                }
                printf( "Name sent success!\n" );
                break;
                
            }
            //get cli_list
            case 4:{
                string lists;
                for( int i = 0; i < clients.size(); i++ ) {
                    int sock = clients[i].clisock;
                    int port = clients[i].cliAddr.sin_port;
                    string addr = inet_ntoa( clients[i].cliAddr.sin_addr );
                    string ans = to_string(sock) +", " + to_string(port) + ", " + addr + ";\n";
                    lists += ans;
                }
                packet mes( 4, client.clisock, lists );
                if( send( client.clisock, pac2str(mes).data(), pac2str(mes).length(), 0 ) < 0 ) {
                    printf( "Client lists error!\n" );
                }
                printf( "Client lists sent success!\n" );
                break;
                
            }
            //send message
            case 5:{
                int flag = 0 ;
                for( int i = 0; i < clients.size(); i++ ) {
                    if( clients[i].clisock == receive.getId() ) {
                        if( client.clisock == receive.getId() ) {
                            packet mes3( 5, client.clisock, "\n你成功给自己发了一条信息：" + receive.getText() );
                            if( send( client.clisock, pac2str(mes3).data(), pac2str(mes3).length(), 0 ) < 0 ) {
                                printf( "Sending error!\n" );
                            }
                        } else {
                            packet mes( 5, receive.getId(), receive.getText() );
                            if( send( clients[i].clisock, pac2str(mes).data(), pac2str(mes).length(), 0 ) < 0 ) {
                                printf( "Sending error!\n" );
                            }
                            packet mes2( 5, client.clisock, "Sending success!");
                            if( send( client.clisock, pac2str(mes2).data(), pac2str(mes2).length(), 0 ) < 0 ) {
                                printf( "Sending error!\n" );
                            }
                        }
                        printf( "Sending success!\n" );   // need ip and port
                        flag = 1;
                        break;
                    }
                }
                if( !flag ){
                    packet mes( 5, client.clisock, "No this client!" );
                        if( send( client.clisock, pac2str(mes).data(), pac2str(mes).length(), 0 ) < 0 ) {
                            printf( "Sending error!\n" );
                        }
                        printf( "Sending success!\nNOT found the destination!\n" );
                }
            }
            default:
                break;
        }
        memset( &rev, 0, sizeof(rev) );//清空rec，准备接受下一次消息
        mtx.unlock();
    }
}

int main()
{
    // initialize WSA
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;    // store the address of WSADATA
    if( WSAStartup(sockVersion, &wsaData) != 0) {   // success:0   failed:!0
        printf( "WSAStartup() error!\n" );
        return 0;
    }

    // create the server socket
    SOCKET server_sock = socket( AF_INET, SOCK_STREAM,  IPPROTO_TCP);
    if( server_sock == INVALID_SOCKET ) {
        printf( "socket error!\n" );
        return 0;
    }

    // bind server IP and PORT
    //      初始化服务器地址
    struct sockaddr_in serverAddr;  // IPV4 server address ip
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SER_PORT);
    serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;

    //      allocate a specific addr to the socket
    if( bind(server_sock, (LPSOCKADDR)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR ) 
        printf( "bind error!\n" );

    // begin listening
    if( listen(server_sock, MAXNUM) == SOCKET_ERROR ) {
        printf( "listen error!\n" );
        return -1;
    }

    // 循环接收数据
    while(1) {
        printf( "waiting for connecting...\n" );
        // prepare the client address/IP/PORT
        struct sockaddr_in clientAddr;
        int clientAddrlen = sizeof( clientAddr );
        SOCKET client_sock = accept( server_sock, (struct sockaddr *)&clientAddr, &clientAddrlen );
        if( client_sock == INVALID_SOCKET ) {
            printf( "accept error!\n" );
            continue;
        }
        clientINFO cliinfo;
        cliinfo.clisock = client_sock;
        cliinfo.cliAddr = clientAddr;
        clients.push_back( cliinfo );
        printf( "receive a connect: %s\n", inet_ntoa(clientAddr.sin_addr) );
        thread ithread = thread( thread_handle, cliinfo );
        ithread.detach();
    }
    WSACleanup();
    system( "pause" );
    return 0;
}