// gamer.cpp
// BM26 COMPLETE VERSION
// register.exe から jsonPath を受け取ってゲーム開始
// JSON譜面読み込み
// WAV再生
// BGA表示
// レーン発光
// ダブルバッファリング
// コンボ
// FULLCOMBO
// 判定
// ノーツアニメーション
// BPM対応
// 低遅延化
// 軽量化
// 演奏ゲーム仕様

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <gdiplus.h>
#include <mmsystem.h>

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <cmath>
#include <filesystem>
#include <mmreg.h>
#include <dsound.h>
#pragma comment(lib, "winmm.lib")



#include "json.hpp"

#pragma comment(lib,"winmm.lib")

using namespace Gdiplus;
using namespace std;
using json = nlohmann::json;

namespace fs = std::filesystem;

/* =========================================
   定数
========================================= */

const int WINDOW_W = 1400;
const int WINDOW_H = 900;

const int GAME_W = 840;

const int LANE_COUNT = 5;

const int LANE_AREA_W = 350;

const int NOTE_H = 18;

const int JUDGE_LINE_Y = 700;

const float GREAT_TIMING = 0.06f;
const float GOOD_TIMING  = 0.010f;
const float BAD_TIMING   = 0.15f;

/* =========================================
   構造体
========================================= */

struct Note {

    int lane = 0;

    int row = 0;

    string wav;

    bool hit = false;

    float y = 0;
};

struct BGAEvent {

    int row = 0;

    string bmp;

    bool played = false;
};

#include <unordered_map>

struct WavData {
    char* data = nullptr;
    DWORD size = 0;
};

HWAVEOUT hWaveOut = NULL;


unordered_map<string, WavData> wavCache;

struct Chart {

    string title;

    string artist;

    string genre;

    string difficulty;

    int level = 1;

    float bpm = 120.0f;

    vector<Note> notes;

    vector<BGAEvent> bga;
};

/* =========================================
   グローバル
========================================= */

HWND hwnd;

ULONG_PTR gdiplusToken;

Chart chart;

vector<Note> notes;

vector<BGAEvent> bgaEvents;

vector<int> laneQueue[LANE_COUNT];
int lanePlayIndex[LANE_COUNT] = {0};


bool isPlaying = false;

DWORD startTick = 0;
const float GAME_START_DELAY = 2.0f;

float bpm = 120.0f;

int score = 0;

int combo = 0;

bool fullCombo = false;

wstring judgeText = L"";

DWORD judgeTimer = 0;

wstring currentBGA;

Image* currentBGAImage = nullptr;

float noteSpeed = 280.0f;

bool laneFlash[LANE_COUNT] = {};

DWORD laneFlashTime[LANE_COUNT] = {};

string currentSongFolder;

vector<int> laneIndex[LANE_COUNT];


/* =========================================
   UTF変換
========================================= */

wstring s2ws(const string& s) {

    int sizeNeeded =
        MultiByteToWideChar(
            CP_UTF8,
            0,
            s.c_str(),
            -1,
            NULL,
            0
        );

    wstring result(
        sizeNeeded,
        0
    );

    MultiByteToWideChar(
        CP_UTF8,
        0,
        s.c_str(),
        -1,
        &result[0],
        sizeNeeded
    );

    result.pop_back();

    return result;
}

/* =========================================
   時間
========================================= */


float getNowTime() {

    return
        (GetTickCount() - startTick)
        / 1000.0f;
}
float getSongTime() {

    float t = getNowTime() - GAME_START_DELAY;

    if (t < 0.0f)
        return -1.0f;   // まだ開始前

    return t;
}

/* =========================================
   WAV再生
========================================= */
#include <mmeapi.h>
#pragma comment(lib, "winmm.lib")

void playSoundFile(const string& file) {

    string fullPath =
        currentSongFolder + "/" + file;

    auto it = wavCache.find(fullPath);

    if (it == wavCache.end())
        return;

    WavData& w = it->second;

    if (!w.data || w.size == 0)
        return;

    WAVEHDR hdr = {};
    hdr.lpData = (LPSTR)w.data;
    hdr.dwBufferLength = w.size;
    hdr.dwFlags = 0;

    waveOutPrepareHeader(hWaveOut, &hdr, sizeof(WAVEHDR));
    waveOutWrite(hWaveOut, &hdr, sizeof(WAVEHDR));

    // 非同期なので少し後で解除（重要）
    waveOutUnprepareHeader(hWaveOut, &hdr, sizeof(WAVEHDR));
}
/* =========================================
   JSON読み込み
========================================= */

bool initAudio() {

    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    return waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL)
        == MMSYSERR_NOERROR;
}


bool loadChart(const string& jsonPath) {

    ifstream ifs(jsonPath);

    if (!ifs.is_open()) {

        MessageBoxA(
            NULL,
            ("json open failed:\n" + jsonPath).c_str(),
            "ERROR",
            MB_OK
        );

        return false;
    }

    currentSongFolder =
        fs::path(jsonPath)
        .parent_path()
        .string();

    json j;

    try {
        ifs >> j;
    }
    catch (...) {

        MessageBoxA(
            NULL,
            "json parse failed",
            "ERROR",
            MB_OK
        );

        return false;
    }

    // =========================
    // 初期化
    // =========================
    chart.notes.clear();
    chart.bga.clear();
    wavCache.clear();   // ★重要：古い音データ破棄

    // =========================
    // meta
    // =========================
    if (j.contains("meta")) {

        auto meta = j["meta"];

        chart.title =
            meta.value("title", "Unknown");

        chart.artist =
            meta.value("artist", "Unknown");

        chart.genre =
            meta.value("genre", "Unknown");

        chart.difficulty =
            meta.value("difficulty", "NORMAL");

        chart.level =
            meta.value("level", 1);

        chart.bpm =
            meta.value("bpm", 120.0f);
    }

    bpm = chart.bpm;

    // =========================
    // notes
    // =========================
    if (j.contains("notes")) {

        for (auto& n : j["notes"]) {

            int lane =
                n.value("lane", 0);

            if (lane < 0 || lane >= LANE_COUNT)
                continue;

            Note note;

            note.lane = lane;

            note.row =
                n.value("row", 0);

            note.wav =
                n.value("wav", "");

            chart.notes.push_back(note);
        }
    }

    // =========================
    // bga
    // =========================
    if (j.contains("bga")) {

        for (auto& b : j["bga"]) {

            BGAEvent e;

            e.row =
                b.value("row", 0);

            e.bmp =
                b.value("bmp", "");

            chart.bga.push_back(e);
        }
    }

    // =========================
    // WAVキャッシュ生成（ここが最重要）
    // =========================
    for (auto& n : chart.notes) {

        if (n.wav.empty())
            continue;

        string path =
            currentSongFolder + "/" + n.wav;

        // すでにロード済みならスキップ
        if (wavCache.find(path) != wavCache.end())
            continue;

        ifstream wf(path, ios::binary | ios::ate);

        if (!wf.is_open())
            continue;

        streamsize size = wf.tellg();

        if (size <= 0)
            continue;

        wf.seekg(0, ios::beg);

        char* buffer = new char[(size_t)size];

        if (!wf.read(buffer, size)) {
            delete[] buffer;
            continue;
        }

        // ★重要：そのまま生WAVを保持
        WavData w;
        w.data = buffer;
        w.size = (DWORD)size;

        wavCache[path] = w;
    }

    return true;
}


/* =========================================
   判定
========================================= */

wstring judge(float diff) {

    float ad =
        abs(diff);

    if (ad <= GREAT_TIMING)
        return L"GREAT";

    if (ad <= GOOD_TIMING)
        return L"GOOD";

    if (ad <= BAD_TIMING)
        return L"BAD";

    return L"";
}

/* =========================================
   判定表示
========================================= */

void showJudge(
    const wstring& text
) {

    judgeText = text;

    judgeTimer =
        GetTickCount();
}

/* =========================================
   FULL COMBO確認
========================================= */

void checkFullCombo() {

    bool remain = false;

    for (auto& n : notes) {

        if (!n.hit) {

            remain = true;
            break;
        }
    }

    if (
        !remain
        &&
        combo == (int)notes.size()
    ) {

        fullCombo = true;
    }
}

/* =========================================
   ゲーム開始
========================================= */

void startGame() {

    notes.clear();
    bgaEvents.clear();
    currentBGA = L"";

    for (int i = 0; i < LANE_COUNT; i++) {
        laneQueue[i].clear();
        lanePlayIndex[i] = 0;
    }

    for (auto n : chart.notes) {
        notes.push_back(n);
    }

    for (auto b : chart.bga) {
        bgaEvents.push_back(b);
    }

    // ★ laneごとにインデックス構築
    for (int i = 0; i < (int)notes.size(); i++) {
        laneQueue[notes[i].lane].push_back(i);
    }

    score = 0;
    combo = 0;
    fullCombo = false;
    judgeText = L"";

    startTick = GetTickCount();
    isPlaying = true;
}
/* =========================================
   レーン発光
========================================= */

void flashLane(int lane) {

    if (
        lane < 0
        ||
        lane >= LANE_COUNT
    )
        return;

    laneFlash[lane] = true;

    laneFlashTime[lane] =
        GetTickCount();
}

/* =========================================
   レーン押下
========================================= */
void pressLane(int lane) {

    if (!isPlaying)
        return;

    flashLane(lane);

    float songTime = getSongTime();
    if (songTime < 0.0f)
        return;

    float beat = 60.0f / bpm;

    // ===================================
    // ① 判定処理（そのまま）
    // ===================================
    for (auto& n : notes) {

        if (n.hit)
            continue;

        if (n.lane != lane)
            continue;

        float t = n.row * (beat / 4.0f);
        float diff = songTime - t;

        wstring j = judge(diff);

        if (j != L"") {

            n.hit = true;

            if (!n.wav.empty())
                playSoundFile(n.wav);

            showJudge(j);

            if (j == L"GREAT" || j == L"GOOD")
                combo++;
            else
                combo = 0;

            score += 100;

            checkFullCombo();
            return;
        }
    }

    // ===================================
    // ② ★音保証システム（修正版）
    // ===================================

    auto& q = laneQueue[lane];

    int& idx = lanePlayIndex[lane];

    float BAD_RANGE = 0.15f;

    // 初期保証：最初は必ず次のノーツ
    if (idx >= (int)q.size())
        idx = (int)q.size() - 1;

    for (int i = idx; i < (int)q.size(); i++) {

        Note& n = notes[q[i]];

        float t = n.row * (beat / 4.0f);
        float diff = songTime - t;

        // ★重要：BAD範囲
        if (fabs(diff) <= BAD_RANGE) {

            if (!n.wav.empty())
                playSoundFile(n.wav);

            idx = i + 1;
            return;
        }

        // 未来ノーツならそれを「次候補」
        if (diff < -BAD_RANGE) {

            if (!n.wav.empty())
                playSoundFile(n.wav);

            idx = i;
            return;
        }
    }

    // ===================================
    // ③ 最後の保険（最後のノーツ）
    // ===================================
    if (!q.empty()) {

        Note& n = notes[q.back()];

        if (!n.wav.empty())
            playSoundFile(n.wav);
    }
}




bool keyState[256] = {};
bool prevKeyState[256] = {};

void handleInput() {

    const int keys[5] = { 'F','G','H','J','K' };

    vector<int> pressed;

    // ① まず「押されたレーンを全部集める」
    for (int i = 0; i < 5; i++) {

        int k = keys[i];

        if (keyState[k] && !prevKeyState[k]) {
            pressed.push_back(i);
        }

        prevKeyState[k] = keyState[k];
    }

    // ② 同フレーム内でまとめて処理
    for (int lane : pressed) {
        pressLane(lane);
    }

    // ③ 長押しエフェクト（これは分離）
    for (int i = 0; i < 5; i++) {

        int k = keys[i];

        if (keyState[k]) {
            laneFlash[i] = true;
            laneFlashTime[i] = GetTickCount();
        }
    }
}
/* =========================================
   更新
========================================= */

void updateGame() {

    handleInput();
    if (!isPlaying)
        return;

    float songTime = getSongTime();
    if (songTime < 0.0f)
        return;

    float beat = 60.0f / bpm;

    // ノーツ更新
    for (auto& n : notes) {

        if (n.hit)
            continue;

        float t = n.row * (beat / 4.0f);
        float diff = t - songTime;

        n.y = JUDGE_LINE_Y - (diff * noteSpeed);

        if (diff < -BAD_TIMING) {

            n.hit = true;
            combo = 0;

            showJudge(L"MISS");
        }
    }

    // BGA更新（ここも songTime に統一）
    for (auto& b : bgaEvents) {

        if (b.played)
            continue;

        float t = b.row * (beat / 4.0f);

        if (songTime >= t) {

            string fullPath =
                currentSongFolder + "/" + b.bmp;

            if (fs::exists(fullPath)) {

                currentBGA = s2ws(fullPath);

                if (currentBGAImage) {
                    delete currentBGAImage;
                    currentBGAImage = nullptr;
                }

                currentBGAImage =
                    new Image(currentBGA.c_str());
            }

            b.played = true;
        }
    }

    // レーン発光
    for (int i = 0; i < LANE_COUNT; i++) {

        if (laneFlash[i]) {

            DWORD elapsed =
                GetTickCount() - laneFlashTime[i];

            if (elapsed > 120) {
                laneFlash[i] = false;
            }
        }
    }

    // 判定表示消去
    if (!judgeText.empty() &&
        GetTickCount() - judgeTimer > 500) {
        judgeText = L"";
    }
}

/* =========================================
   テキスト描画
========================================= */

void drawTextCenter(
    Graphics& g,
    const wstring& text,
    int x,
    int y,
    int size,
    Color color
) {

    FontFamily ff(
        L"Arial"
    );

    Font font(
        &ff,
        size,
        FontStyleBold,
        UnitPixel
    );

    SolidBrush brush(
        color
    );

    RectF rect(
        (REAL)x,
        (REAL)y,
        600,
        120
    );

    StringFormat sf;

    sf.SetAlignment(
        StringAlignmentCenter
    );

    g.DrawString(
        text.c_str(),
        -1,
        &font,
        rect,
        &sf,
        &brush
    );
}

/* =========================================
   描画
========================================= */

void render(HDC hdc) {

    HDC memDC =
        CreateCompatibleDC(hdc);

    HBITMAP memBitmap =
        CreateCompatibleBitmap(
            hdc,
            WINDOW_W,
            WINDOW_H
        );

    HBITMAP oldBitmap =
        (HBITMAP)
        SelectObject(
            memDC,
            memBitmap
        );

    Graphics g(memDC);

    g.SetSmoothingMode(
        SmoothingModeHighSpeed
    );

    SolidBrush black(
        Color(0,0,0)
    );

    SolidBrush noteBrush(
        Color(0,220,255)
    );

    g.FillRectangle(
        &black,
        0,
        0,
        WINDOW_W,
        WINDOW_H
    );

    /* BGA */

    if (currentBGAImage) {

        g.DrawImage(
            currentBGAImage,
            GAME_W,
            0,
            WINDOW_W - GAME_W,
            WINDOW_H
        );
    }

    int laneW =
        LANE_AREA_W
        / LANE_COUNT;

    int laneX =
        (GAME_W / 2)
        - (LANE_AREA_W / 2);

    /* lane */

    for (int i=0;i<LANE_COUNT;i++) {

        int x =
            laneX
            + laneW*i;

        Color laneColor(
            10,
            10,
            30
        );

        if (laneFlash[i]) {

            DWORD elapsed =
                GetTickCount()
                - laneFlashTime[i];

            int alpha =
                max(
                    0,
                    255 - (int)(elapsed * 2)
                );

            laneColor =
                Color(
                    alpha,
                    180,
                    220,
                    255
                );
        }

        SolidBrush laneBrush(
            laneColor
        );

        g.FillRectangle(
            &laneBrush,
            x,
            0,
            laneW,
            WINDOW_H
        );

        Pen border(
            Color(
                0,
                180,
                255
            ),
            2
        );

        g.DrawRectangle(
            &border,
            x,
            0,
            laneW,
            WINDOW_H
        );
    }

    /* judge line */

    SolidBrush judgeLine(
        Color(
            255,
            255,
            0
        )
    );

    g.FillRectangle(
        &judgeLine,
        laneX,
        JUDGE_LINE_Y,
        LANE_AREA_W,
        4
    );

    /* notes */

    for (auto& n : notes) {

        if (n.hit)
            continue;

        int x =
            laneX
            + laneW*n.lane
            + 3;

        g.FillRectangle(
            &noteBrush,
            x,
            (int)n.y,
            laneW - 6,
            NOTE_H
        );
    }

    /* title */

    drawTextCenter(
        g,
        s2ws(chart.title),
        20,
        30,
        34,
        Color(
            255,
            255,
            255
        )
    );

    drawTextCenter(
        g,
        s2ws(chart.artist),
        20,
        75,
        20,
        Color(
            180,
            180,
            180
        )
    );

    /* score */

    drawTextCenter(
        g,
        L"SCORE : "
        + to_wstring(score),
        10,
        10,
        26,
        Color(
            255,
            255,
            255
        )
    );

    /* combo */

    if (combo > 0) {

        drawTextCenter(
            g,
            to_wstring(combo),
            240,
            180,
            80,
            Color(
                255,
                255,
                255
            )
        );
    }

    /* judge */

    if (!judgeText.empty()) {

        Color c(
            255,
            255,
            255
        );

        if (judgeText == L"GREAT")
            c = Color(0,255,255);

        if (judgeText == L"GOOD")
            c = Color(0,255,0);

        if (judgeText == L"BAD")
            c = Color(255,128,0);

        if (judgeText == L"MISS")
            c = Color(255,0,0);

        drawTextCenter(
            g,
            judgeText,
            160,
            300,
            52,
            c
        );
    }

    if (isPlaying && getNowTime() < GAME_START_DELAY) {

        drawTextCenter(
            g,
            L"GAME START",
            200,
            350,
            64,
            Color(255, 255, 255)
        );
    }

    /* FULL COMBO */

    if (fullCombo) {

        drawTextCenter(
            g,
            L"FULL COMBO",
            80,
            100,
            62,
            Color(
                255,
                255,
                0
            )
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
        oldBitmap
    );

    DeleteObject(
        memBitmap
    );

    DeleteDC(
        memDC
    );
}

/* =========================================
   WindowProc
========================================= */

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
            8,
            NULL
        );

        return 0;

    case WM_ERASEBKGND:

        return 1;

    case WM_TIMER:

        updateGame();

        InvalidateRect(
            hWnd,
            NULL,
            FALSE
        );

        return 0;

    case WM_KEYDOWN:
        keyState[wp] = true;
        return 0;

    case WM_KEYUP:
        keyState[wp] = false;
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

        if (currentBGAImage) {

            delete currentBGAImage;
            currentBGAImage = nullptr;
        }

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


/* =========================================
   WinMain
========================================= */


int WINAPI WinMain(
    HINSTANCE hInst,
    HINSTANCE,
    LPSTR lpCmdLine,
    int nCmd
) {

    GdiplusStartupInput gdiplusStartupInput;

    GdiplusStartup(
        &gdiplusToken,
        &gdiplusStartupInput,
        NULL
    );
    
    string jsonPath =
        lpCmdLine;

    if (
        jsonPath.size() >= 2
        &&
        jsonPath.front() == '"'
        &&
        jsonPath.back() == '"'
    ) {

        jsonPath =
            jsonPath.substr(
                1,
                jsonPath.size() - 2
            );
    }

    if (jsonPath.empty()) {

        MessageBoxA(
            NULL,
            "json path missing",
            "ERROR",
            MB_OK
        );

        return 0;
    }
    initAudio();
    if (!loadChart(jsonPath)) {

        return 0;
    }

    WNDCLASSW wc = {};

    wc.lpfnWndProc =
        WndProc;

    wc.hInstance =
        hInst;

    wc.lpszClassName =
        L"BM26";

    wc.hCursor =
        LoadCursor(
            NULL,
            IDC_ARROW
        );

    RegisterClassW(&wc);

    hwnd =
        CreateWindowExW(
            0,
            L"BM26",
            L"BM26",
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

    startGame();

// 高精度タイマー
timeBeginPeriod(1);

MSG msg;
ZeroMemory(&msg, sizeof(msg));

LARGE_INTEGER freq;
QueryPerformanceFrequency(&freq);

LARGE_INTEGER prev;
QueryPerformanceCounter(&prev);

while (msg.message != WM_QUIT) {

    // メッセージ処理（ノンブロッキング）
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 時間計算
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    double delta =
        (double)(now.QuadPart - prev.QuadPart)
        / (double)freq.QuadPart;

    prev = now;

    // 60fps制御
    static double acc = 0.0;
    acc += delta;

    while (acc >= 1.0 / 60.0) {

        updateGame();
        acc -= 1.0 / 60.0;
    }

    // 描画（毎フレーム）
    InvalidateRect(hwnd, NULL, FALSE);
}

timeEndPeriod(1);

    return 0;
}


