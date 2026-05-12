// audio.cpp
#include <windows.h>
#include <mmsystem.h>
#include <string>
#include "audio.h"

#pragma comment(lib,"winmm.lib")

void audio_init(){}

void audio_shutdown(){
    mciSendString(L"close all",NULL,0,NULL);
}

void audio_play(const std::wstring& path){

    std::wstring cmd =
        L"open \"" + path + L"\" type waveaudio alias snd";

    mciSendString(cmd.c_str(),NULL,0,NULL);
    mciSendString(L"play snd from 0",NULL,0,NULL);
}