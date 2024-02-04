#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <csignal>
#include <cstring>
#include <winsock2.h>
#include <windows.h>
#include <process.h>

#define SOCKET_PORT 2037
#define LOGIN 3210102037
#define PASSWORD 2037

#pragma comment(lib, "ws2_32.lib")

/* Global Variables */
SOCKET serverSocket;
bool exit_requested = false;
std::condition_variable process;

void Start();
void Initialize();
DWORD WINAPI ThreadHandle(LPVOID lpParam);
void GETHandle(SOCKET clientSocket, std::string url);
void POSTHandle(SOCKET clientSocket, std::string url, std::string request);
void UnknownMethodHandle(SOCKET clientSocket);

int main()
{
    Initialize();
    std::cout << "Server listening on port 2037. Press 'Ctrl + C' to shut down the server." << std::endl;

    Start();

    return 0;
}

void Start() {
    /* Process the clients */
    while(!exit_requested) {
        /* Create the Client Socket */
	    sockaddr_in clientAddress{};
        int clientAddrLen = sizeof(clientAddress);
	    SOCKET clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr*>(&clientAddress), &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            perror("Error accepting connection");
            continue;
        }
        CreateThread(NULL, 0, ThreadHandle, reinterpret_cast<LPVOID>(&clientSocket), 0, nullptr);
    }
    if (serverSocket != INVALID_SOCKET) closesocket(serverSocket);
	WSACleanup();
}

void Initialize() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cout << "WSAStartup Error!" << std::endl;
        closesocket(serverSocket);
		std::exit(-1);
	}

	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		std::cout << "Socket Error !" << std::endl;
        closesocket(serverSocket);
		std::exit(-1);
	}

	sockaddr_in sin{};
	sin.sin_family = AF_INET;
	sin.sin_port = htons(SOCKET_PORT);
	sin.sin_addr.s_addr = INADDR_ANY;

	if (bind(serverSocket, reinterpret_cast<struct sockaddr*>(&sin), sizeof(sin)) == SOCKET_ERROR) {
        std::cout << "Bind Error!" << std::endl;
        closesocket(serverSocket);
        std::exit(-1);
    }
 
	if (listen(serverSocket, 5) == SOCKET_ERROR) {
		std::cout << "Listen Error !" << std::endl;
        closesocket(serverSocket);
		std::exit(-1);
	}
}

DWORD WINAPI ThreadHandle(LPVOID lpParam) {
    char buffer[1024 * 1024] = {0};
    SOCKET clientSocket = *(static_cast<SOCKET *>(lpParam));
    struct sockaddr_in HTTPAddress = {0};
    int HTTPAddrLen = sizeof(HTTPAddress);
    // if (getpeername(clientSocket, (struct sockaddr *)&HTTPAddress, &HTTPAddrLen) == SOCKET_ERROR) {
    //     perror("Error getting client address");
    //     closesocket(clientSocket);
    //     return -1;
    // }
    // std::cout << "Client connected: " << inet_ntoa(HTTPAddress.sin_addr) << ":" << HTTPAddress.sin_port << std::endl;
    while (true) {
        if( recv( clientSocket, buffer, sizeof(buffer), 0) < 0 ) {
            printf( "Receive Error from client!\n" );
            closesocket(clientSocket);
            return -1;
        }
        std::string request(buffer);
        std::istringstream ss(request);
        std::string method, file, headers;
        ss >> method >> file >> headers;

        if (method == "GET") {
            GETHandle(clientSocket, file);
        } else if (method == "POST") {
            POSTHandle(clientSocket, file, request);
            break;
        } else {
            UnknownMethodHandle(clientSocket);
        }
    }
    std::cout << "Thread " << clientSocket << " exiting." << std::endl;
    closesocket(clientSocket);
    return 0;
}

void GETHandle(SOCKET clientSocket, std::string url) {
    std::cout << "handle GET request!" << std::endl;
    // analyse the request
    std::string response_type;
    std::string filepath = "..";
    if (url.find(".txt") != std::string::npos) {
        response_type = "text/plain";
        filepath += "/file/txt";
    } else if (url.find(".html") != std::string::npos) {
        response_type = "text/html";
        filepath += "/file/html";
    } else if (url.find(".jpg") != std::string::npos) {
        response_type = "image/jpg";
        filepath += "/file/img";
    } else if (url.find(".ico") != std::string::npos) {
        response_type = "image/ico";
        filepath += "/img";
    }
    filepath += url;

    // read the files
    std::string response;
    std::ifstream fileStream(filepath, std::ios::binary);
    if (fileStream.is_open()) {
        std::cout << "file found" << std::endl;
        response.clear();
        std::ostringstream fileContentStream;
        fileContentStream << fileStream.rdbuf();
        std::string fileContent = fileContentStream.str();

        // 构建HTTP响应
        std::ostringstream responseStream;
        responseStream << "HTTP/1.1 200 OK\r\n";
        responseStream << "Connection: keep-alive\r\n";
        responseStream << "Content-Length: " << fileContent.size() << "\r\n";
        responseStream << "Server: csr_http1.1\n";

        if (url.find(".jpg") != std::string::npos) {
            responseStream << "Accept-Ranges: bytes\r\n";
        }

        responseStream << "Content-Type: " << response_type << "\r\n";
        responseStream << "\r\n";
        responseStream << fileContent;

        // 发送HTTP响应
        response = responseStream.str();
    } else {
        // 文件不存在，返回404 Not Found
        std::cout << "file found failed!!!!!" << std::endl;
        response.clear();
        response = "HTTP/1.1 404 Not Found\r\n";
        response += "Content-Type: text/html;charset=utf-8\r\n";
        response += "Content-Length: 84\r\n\r\n";
        response += "<html><body><h1>404 Not Found</h1><p>From server: URL is wrong.</p></body></html>\r\n";
    }
    send(clientSocket, response.c_str(), response.size(), 0);
    fileStream.close(); // 记得关闭文件
    fileStream.clear();
}

void POSTHandle(SOCKET clientSocket, std::string url, std::string request) {
    std::cout << "handle POST request!" << std::endl;
    std::cout << request << std::endl;
    std::string response;
    std::string content, loginName, passWord;
    if (url.find("dopost") != std::string::npos) {
        response.clear();
        response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: ";
        // get the post content
        content = request.substr(request.find("login=") + 6);
        loginName = content.substr(0, content.find("&"));
        passWord = content.substr(content.find("&") + 1 + 5);
        std::cout << loginName << "  " << passWord << std::endl;
        if (loginName == std::to_string(LOGIN) && passWord == std::to_string(PASSWORD)) {
            std::string SuccessMsg = "<html><body><h1>Login Success</h1></body></html>\r\n";
            response += std::to_string(SuccessMsg.length());
            response += "\r\n\r\n";
            response += SuccessMsg;
        } else {
            std::string FailMsg = "<html><body><h1>Login Fail</h1></body></html>\r\n";
            response += std::to_string(FailMsg.length());
            response += "\r\n\r\n";
            response += FailMsg;
        }
    } else {
        response.clear();
        response = "HTTP/1.1 404 Not Found\r\n";
        response += "Content-Type: text/html;charset=utf-8\r\n";
        response += "Content-Length: 0\r\n\r\n";
    }
    send(clientSocket, response.c_str(), response.size(), 0);
}

void UnknownMethodHandle(SOCKET clientSocket) {
    std::string response;
    response.clear();
    response = "HTTP/1.1 404 Not Found\r\n";
    response += "Content-Type: text/html;charset=utf-8\r\n";
    response += "Content-Length: 0\r\n\r\n";
    send(clientSocket, response.c_str(), response.size(), 0);
}