//
// Created by Jack on 2023/9/24.
//
#include <unistd.h>
#include "protocol.h"
#include "sys/socket.h"
#include <sstream>
#include <arpa/inet.h>
#include <ctime>
#include <cstring>

#define MAXLENGTH 1024

void getTime();
void getName();
void getList();
void sendMessage();
void mark_connected();
void beClosed();
void *receive(void *args);
void printChoices(bool connected);

int clientSocket;
pthread_t receiver;
pthread_mutex_t mutex;
bool waitingStatus = false;
const int waitTime = 1; //200ms=0.2s
const int maxWaitNum = 500; //max wait time=20s

void alterLock(bool type)
{
    pthread_mutex_lock(&mutex);
    waitingStatus = type;
    pthread_mutex_unlock(&mutex);
}

int main(){
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }
    struct sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;

    char opt;
    bool connected = false;
    std::string ip;
    int port, res;

    pthread_mutex_init(&mutex, nullptr);

    while(true){
//        std::cout << "1) 连接功能" << std:: endl;
//        if(connected){
//            std::cout << "2) 断开功能" << std:: endl;
//            std::cout << "3) 获取时间功能" << std:: endl;
//            std::cout << "4) 获取名字功能" << std:: endl;
//            std::cout << "5) 获取客户端列表功能" << std:: endl;
//            std::cout << "6) 发送消息功能" << std:: endl;
//        }
//        std::cout << "q) 退出功能" << std:: endl;
//        std::cout << "请选择功能：";
        printChoices(connected);
        std::cin >> opt;
        if((!connected && opt != '1' && opt != 'q')){
            std::cout << "没有与输入对应的功能，请输入正确的指令。\n" << std::endl;
            continue;
        }

        switch(opt){
            case '1':   //连接功能
                if(connected){
                    printf("已经连接服务端。\n\n");
                    break;
                }
                std::cout << "请输入服务器IP：";
                std::cin >> ip;
                std::cout << "请输入服务器端口：";
                std::cin >> port;
                serverAddr.sin_port = htons(port);
                serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
                if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
                    std::cerr << "Error connecting to server" << std::endl;
                    close(clientSocket);
                    return -1;
                }
                connected = true;

                res = pthread_create(&receiver, nullptr, receive, &clientSocket);
                if (res != 0)
                {
                    printf("ERROR:\tCreate pthread failed; Return code: %d\n", res);
                    return 0;
                }
                puts("");

                mark_connected();
                break;
            case '2':   //断开功能
                beClosed();
                connected = false;
                std::cout << std::endl;
                break;
            case '3':   //获取时间功能
                getTime();
                break;
            case '4':   //获取名字功能
                getName();
                break;
            case '5':   //获取客户端列表功能
                getList();
                break;
            case '6':   //发送消息功能
                sendMessage();
                break;
            case 'q':
                if(connected) beClosed();
                std::cout << "拜拜～" << std::endl;
                return 0;
            default: std::cout << "没有与输入对应的功能，请输入正确的指令。\n" << std::endl;
        }
    }
}

void *receive(void *args){
    int clientSocket2 = *reinterpret_cast<int*>(args);
    long res;

    while(true){
        char buffer[MAXLENGTH] = {0};
        res = recv(clientSocket2, buffer, MAXLENGTH, 0);
        if (res <= 0) break;
        packet receive = analyzePacket(buffer);

        if(receive.getType() == 2) {
            time_t timestamp = std::stoi(receive.getText());
            char time[128]= {0};
            strftime(time, 64, "%Y-%m-%d %H:%M:%S", localtime(&timestamp));
            printf("当前服务器时间为: %s\n\n", time);
            alterLock(false);
        }
        else if(receive.getType() == 3) {
            printf("客户端名称为:\n%s\n\n", receive.getText().c_str());
            alterLock(false);
        }
        else if(receive.getType() == 4) {
            printf("客户端列表为:\n%s\n", receive.getText().c_str());
            alterLock(false);
        }
        else if(receive.getType() == 5) {
            if(waitingStatus){
                std::cout << receive.getText() << "\n" << std::endl;
                alterLock(false);
            }else{
                std::cout << "\n\n收到来自客户端的消息:\n" << receive.getText() << "\n" << std::endl;
                printChoices(true);
//                std::cout << "请选择功能：";
            }
        }
    }
    return nullptr;
}

void mark_connected(){
    //send a package for connecting successfully
    const void *message = "0000|";
    send(clientSocket, message, strlen(reinterpret_cast<const char*>(message)), 0);
}

void beClosed(){
    const void *message = "0010|";
    send(clientSocket, message, strlen(reinterpret_cast<const char*>(message)), 0);
    close(clientSocket);
//    pthread_join(receiver, nullptr);    //关闭子线程
    pthread_detach(receiver);

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }
}

void getTime(){
    const void *message = "0100|";
    send(clientSocket, message, strlen(reinterpret_cast<const char*>(message)), 0);

    alterLock(true);
    int cnt = 0;
    while(true){
        if(cnt++ >= maxWaitNum){
            std::cout << "getTime应答超时" << std::endl;
            break;
        }
        pthread_mutex_lock(&mutex);
        if (!waitingStatus){
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_unlock(&mutex);
        usleep(waitTime);
    }
}

void getName(){
    const void *message = "0110|";
    send(clientSocket, message, strlen(reinterpret_cast<const char*>(message)), 0);

    alterLock(true);
    int cnt = 0;
    while(true){
        if(cnt++ >= maxWaitNum){
            std::cout << "getName应答超时" << std::endl;
            break;
        }
        pthread_mutex_lock(&mutex);
        if (!waitingStatus){
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_unlock(&mutex);
        usleep(waitTime);
    }
}

void getList(){
    const void *message = "1000|";
    send(clientSocket, message, strlen(reinterpret_cast<const char*>(message)), 0);

    alterLock(true);
    int cnt = 0;
    while(true){
        if(cnt++ > maxWaitNum){
            std::cout << "getList应答超时" << std::endl;
            break;
        }
        pthread_mutex_lock(&mutex);
        if (!waitingStatus){
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_unlock(&mutex);
        usleep(waitTime);
    }
}

void sendMessage(){
    int id;
    std::string text, str_id;

    std::cout << "请输入目标客户端的ID：";
    std::cin >> id;
    getchar();
    std::cout << "请输入要传输的内容：";
    std::getline(std::cin, text);

    if(id == 0) str_id = "0|";
    else{
        str_id = "";
        while(id != 0){
            if(id % 2 == 1) str_id = "1" + str_id;
            else str_id = "0" + str_id;
            id /= 2;
        }
        str_id += "|";
    }
    std::string mes = "101" + str_id + text;
    char message[2048] = "";
    strcpy(message, mes.c_str());
    send(clientSocket, (const void*)message, mes.length(), 0);
//    std::stringstream ss;
//    ss << "101" << str_id << text;
//    const void* ptr = static_cast<const void*>(ss.str().c_str());
//    const void *message = ss.str().c_str();
//    std::cout << reinterpret_cast<const char*>(message) << std::endl;
//    send(clientSocket, message, strlen(reinterpret_cast<const char*>(message)), 0);

    alterLock(true);
    int cnt = 0;
    while(true){
        if(cnt++ > maxWaitNum){
            std::cout << "sendMessage应答超时" << std::endl;
            break;
        }
        pthread_mutex_lock(&mutex);
        if (!waitingStatus){
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_unlock(&mutex);
        usleep(waitTime);
    }
}

void printChoices(bool connected){
    std::cout << "1) 连接功能" << std:: endl;
    if(connected){
        std::cout << "2) 断开功能" << std:: endl;
        std::cout << "3) 获取时间功能" << std:: endl;
        std::cout << "4) 获取名字功能" << std:: endl;
        std::cout << "5) 获取客户端列表功能" << std:: endl;
        std::cout << "6) 发送消息功能" << std:: endl;
    }
    std::cout << "q) 退出功能" << std:: endl;
    std::cout << "请选择功能：" << std::flush;
}