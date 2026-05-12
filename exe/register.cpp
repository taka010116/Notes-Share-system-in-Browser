// register.cpp
// BM26 SONG SELECT
// Songs自動探索
// data.json自動登録
// gamer.exe起動
// 角丸カードUI
// スクロール
// ダブルバッファリング

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <gdiplus.h>

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

#include "json.hpp"

#pragma comment(lib,"gdiplus.lib")

using namespace Gdiplus;
using namespace std;
using json = nlohmann::json;

namespace fs = std::filesystem;

/*=================================================
  定数
=================================================*/

const int WINDOW_W = 1400;
const int WINDOW_H = 900;

const int CARD_W = 1150;
const int CARD_H = 220;

const int CARD_GAP = 25;

/*=================================================
  Song
=================================================*/

struct Song {

    string title;
    string artist;
    string genre;
    string difficulty;

    int level = 1;

    float bpm = 120;

    string jsonPath;

    string folderPath;
};

/*=================================================
  グローバル
=================================================*/

HWND hwnd;

ULONG_PTR gdiplusToken;

vector<Song> songs;

int selectedIndex = 0;

float scrollY = 0;

float targetScrollY = 0;

/*=================================================
  UTF
=================================================*/

wstring s2ws(const string& s) {

    return wstring(s.begin(), s.end());
}

/*=================================================
  角丸矩形
=================================================*/

void AddRoundRect(
    GraphicsPath& path,
    int x,
    int y,
    int w,
    int h,
    int r
) {

    path.AddArc(
        x,
        y,
        r,
        r,
        180,
        90
    );

    path.AddArc(
        x + w - r,
        y,
        r,
        r,
        270,
        90
    );

    path.AddArc(
        x + w - r,
        y + h - r,
        r,
        r,
        0,
        90
    );

    path.AddArc(
        x,
        y + h - r,
        r,
        r,
        90,
        90
    );

    path.CloseFigure();
}

/*=================================================
  DrawText
=================================================*/

void drawText(
    Graphics& g,
    const wstring& text,
    int x,
    int y,
    int size,
    Color color,
    bool bold = false
) {

    FontFamily ff(L"Segoe UI");

    Font font(
        &ff,
        size,
        bold
        ?
        FontStyleBold
        :
        FontStyleRegular,
        UnitPixel
    );

    SolidBrush brush(color);

    g.DrawString(
        text.c_str(),
        -1,
        &font,
        PointF(
            (REAL)x,
            (REAL)y
        ),
        &brush
    );
}

/*=================================================
  JSONロード
=================================================*/

bool loadSong(
    const string& jsonPath,
    Song& s
) {

    ifstream ifs(jsonPath);

    if (!ifs.is_open())
        return false;

    json j;

    ifs >> j;

    auto meta = j["meta"];

    s.title =
        meta.value(
            "title",
            "Unknown"
        );

    s.artist =
        meta.value(
            "artist",
            "Unknown"
        );

    s.genre =
        meta.value(
            "genre",
            "Unknown"
        );

    s.difficulty =
        meta.value(
            "difficulty",
            "NORMAL"
        );

    s.level =
        meta.value(
            "level",
            1
        );

    s.bpm =
        meta.value(
            "bpm",
            120.0f
        );

    s.jsonPath =
        jsonPath;

    s.folderPath =
        fs::path(jsonPath)
        .parent_path()
        .string();

    return true;
}

/*=================================================
  Songs探索
=================================================*/

void scanSongs() {

    songs.clear();

    string root = "Songs";

    if (!fs::exists(root)) {

        MessageBoxA(
            NULL,
            "Songs folder not found",
            "ERROR",
            MB_OK
        );

        return;
    }

    for (
        auto& dir :
        fs::recursive_directory_iterator(root)
    ) {

        if (!dir.is_regular_file())
            continue;

        if (
            dir.path().filename()
            != "data.json"
        )
            continue;

        Song s;

        if (
            loadSong(
                dir.path().string(),
                s
            )
        ) {

            songs.push_back(s);
        }
    }
}

/*=================================================
  曲カード
=================================================*/

void drawSongCard(
    Graphics& g,
    Song& s,
    int x,
    int y,
    bool selected
) {

    int w = CARD_W;
    int h = CARD_H;

    Color c1(
        20,
        20,
        40
    );

    Color c2(
        10,
        10,
        20
    );

    if (selected) {

        c1 = Color(
            40,
            120,
            255
        );

        c2 = Color(
            20,
            60,
            180
        );
    }

    GraphicsPath path;

    AddRoundRect(
        path,
        x,
        y,
        w,
        h,
        25
    );

    LinearGradientBrush bg(
        Point(x,y),
        Point(x+w,y+h),
        c1,
        c2
    );

    g.FillPath(
        &bg,
        &path
    );

    Pen border(
        selected
        ?
        Color(
            255,
            255,
            255
        )
        :
        Color(
            80,
            120,
            255
        ),
        selected ? 4 : 2
    );

    g.DrawPath(
        &border,
        &path
    );

    drawText(
        g,
        s2ws(s.title),
        x + 35,
        y + 25,
        38,
        Color(
            255,
            255,
            255
        ),
        true
    );

    drawText(
        g,
        L"ARTIST : "
        + s2ws(s.artist),
        x + 40,
        y + 85,
        22,
        Color(
            220,
            220,
            255
        )
    );

    drawText(
        g,
        L"GENRE : "
        + s2ws(s.genre),
        x + 40,
        y + 120,
        20,
        Color(
            180,
            180,
            220
        )
    );

    drawText(
        g,
        L"BPM : "
        + to_wstring(
            (int)s.bpm
        ),
        x + 40,
        y + 160,
        22,
        Color(
            0,
            255,
            255
        ),
        true
    );

    drawText(
        g,
        L"LEVEL : "
        + to_wstring(
            s.level
        ),
        x + 260,
        y + 160,
        22,
        Color(
            255,
            255,
            0
        ),
        true
    );

    drawText(
        g,
        L"DIFFICULTY : "
        + s2ws(
            s.difficulty
        ),
        x + 500,
        y + 160,
        22,
        Color(
            255,
            120,
            120
        ),
        true
    );

    if (selected) {

        drawText(
            g,
            L"PRESS ENTER TO START",
            x + 760,
            y + 155,
            24,
            Color(
                255,
                255,
                255
            ),
            true
        );
    }
}

/*=================================================
  起動
=================================================*/

void launchGame() {

    if (songs.empty())
        return;

    string cmd =
        "gamer.exe \""
        + songs[selectedIndex].jsonPath
        + "\"";

    STARTUPINFOA si = {};

    PROCESS_INFORMATION pi = {};

    si.cb = sizeof(si);

    CreateProcessA(
        NULL,
        cmd.data(),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    CloseHandle(
        pi.hProcess
    );

    CloseHandle(
        pi.hThread
    );
}

/*=================================================
  描画
=================================================*/

void render(HDC hdc) {

    HDC memDC =
        CreateCompatibleDC(hdc);

    HBITMAP bmp =
        CreateCompatibleBitmap(
            hdc,
            WINDOW_W,
            WINDOW_H
        );

    HBITMAP oldBmp =
        (HBITMAP)
        SelectObject(
            memDC,
            bmp
        );

    Graphics g(memDC);

    g.SetSmoothingMode(
        SmoothingModeAntiAlias
    );

    /* 背景 */

    LinearGradientBrush bg(
        Point(0,0),
        Point(
            WINDOW_W,
            WINDOW_H
        ),
        Color(
            0,
            0,
            20
        ),
        Color(
            0,
            0,
            0
        )
    );

    g.FillRectangle(
        &bg,
        0,
        0,
        WINDOW_W,
        WINDOW_H
    );

    /* タイトル */

    drawText(
        g,
        L"BM26 SONG SELECT",
        40,
        25,
        54,
        Color(
            255,
            255,
            255
        ),
        true
    );

    drawText(
        g,
        L"↑ ↓ SELECT    ENTER START",
        45,
        95,
        22,
        Color(
            180,
            180,
            255
        )
    );

    drawText(
        g,
        L"SONGS : "
        + to_wstring(
            songs.size()
        ),
        1100,
        35,
        28,
        Color(
            255,
            255,
            0
        ),
        true
    );

    scrollY +=
        (
            targetScrollY
            - scrollY
        )
        * 0.15f;

    int startX = 90;

    int startY = 160;

    for (
        int i=0;
        i<songs.size();
        i++
    ) {

        int y =
            startY
            + i
            * (
                CARD_H
                + CARD_GAP
            )
            - (int)scrollY;

        if (y < -250)
            continue;

        if (y > WINDOW_H + 50)
            continue;

        drawSongCard(
            g,
            songs[i],
            startX,
            y,
            i == selectedIndex
        );
    }

    BitBlt(
        hdc,
        0,
        0,
        WINDOW_W,
        WINDOW_H,
        memDC,
        0,
        0,
        SRCCOPY
    );

    SelectObject(
        memDC,
        oldBmp
    );

    DeleteObject(bmp);

    DeleteDC(memDC);
}

/*=================================================
  WindowProc
=================================================*/

LRESULT CALLBACK WndProc(
    HWND hWnd,
    UINT msg,
    WPARAM wp,
    LPARAM lp
) {

    switch(msg) {

    case WM_CREATE:

        SetTimer(
            hWnd,
            1,
            16,
            NULL
        );

        return 0;

    case WM_ERASEBKGND:

        return 1;

    case WM_TIMER:

        targetScrollY =
            selectedIndex
            * (
                CARD_H
                + CARD_GAP
            );

        InvalidateRect(
            hWnd,
            NULL,
            FALSE
        );

        return 0;

    case WM_KEYDOWN:

        switch(wp) {

        case VK_UP:

            selectedIndex--;

            if (
                selectedIndex < 0
            )
                selectedIndex = 0;

            break;

        case VK_DOWN:

            selectedIndex++;

            if (
                selectedIndex
                >= songs.size()
            )
                selectedIndex =
                    songs.size()-1;

            break;

        case VK_RETURN:

            launchGame();

            break;
        }

        return 0;

    case WM_PAINT: {

        PAINTSTRUCT ps;

        HDC hdc =
            BeginPaint(
                hWnd,
                &ps
            );

        render(hdc);

        EndPaint(
            hWnd,
            &ps
        );

        return 0;
    }

    case WM_DESTROY:

        PostQuitMessage(0);

        return 0;
    }

    return DefWindowProc(
        hWnd,
        msg,
        wp,
        lp
    );
}

/*=================================================
  WinMain
=================================================*/

int WINAPI WinMain(
    HINSTANCE hInst,
    HINSTANCE,
    LPSTR,
    int nCmd
) {

    GdiplusStartupInput gdiplusStartupInput;

    GdiplusStartup(
        &gdiplusToken,
        &gdiplusStartupInput,
        NULL
    );

    scanSongs();

    WNDCLASS wc = {};

    wc.lpfnWndProc =
        WndProc;

    wc.hInstance =
        hInst;

    wc.lpszClassName =
        L"BM26_REGISTER";

    wc.hCursor =
        LoadCursor(
            NULL,
            IDC_ARROW
        );

    RegisterClass(&wc);

    hwnd =
        CreateWindowEx(
            0,
            L"BM26_REGISTER",
            L"BM26 SONG SELECT",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            WINDOW_W,
            WINDOW_H,
            NULL,
            NULL,
            hInst,
            NULL
        );

    ShowWindow(
        hwnd,
        nCmd
    );

    UpdateWindow(hwnd);

    MSG msg;

    while (
        GetMessage(
            &msg,
            NULL,
            0,
            0
        )
    ) {

        TranslateMessage(
            &msg
        );

        DispatchMessage(
            &msg
        );
    }

    GdiplusShutdown(
        gdiplusToken
    );

    return 0;
}