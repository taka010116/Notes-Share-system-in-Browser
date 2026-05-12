// main.cpp
#include <windows.h>
#include <gdiplus.h>
#include "game.h"
#include "audio.h"
#include "chart.h"
#include "render.h"

#pragma comment(lib,"gdiplus.lib")

ULONG_PTR gdiplusToken;

LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);

HWND hwnd;

int WINAPI WinMain(HINSTANCE h,HINSTANCE,LPSTR cmd,int n){

    GdiplusStartupInput g;
    GdiplusStartup(&gdiplusToken,&g,NULL);

    chart_load(cmd);      // Songs読み込み
    audio_init();         // 音初期化
    game_init();          // ゲーム初期化

    WNDCLASS wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = h;
    wc.lpszClassName = L"BM26";

    RegisterClass(&wc);

    hwnd = CreateWindow(L"BM26",L"BM26",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,0,1400,900,
        NULL,NULL,h,NULL);

    ShowWindow(hwnd,n);

    MSG msg;
    while(GetMessage(&msg,NULL,0,0)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    audio_shutdown();
    GdiplusShutdown(gdiplusToken);
}

LRESULT CALLBACK WndProc(HWND h,UINT m,WPARAM w,LPARAM l){

    switch(m){

    case WM_TIMER:
        game_update();
        InvalidateRect(h,NULL,FALSE);
        return 0;

    case WM_KEYDOWN:
        if(w=='F') game_hit(0,get_time());
        if(w=='G') game_hit(1,get_time());
        if(w=='H') game_hit(2,get_time());
        if(w=='J') game_hit(3,get_time());
        if(w=='K') game_hit(4,get_time());
        return 0;

    case WM_PAINT:{
        PAINTSTRUCT ps;
        HDC hdc=BeginPaint(h,&ps);
        render(hdc);
        EndPaint(h,&ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(h,m,w,l);
}