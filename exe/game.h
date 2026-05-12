#pragma once
#include <windows.h>
#include <vector>
#include <string>

struct Note;
struct Chart;

extern std::vector<Note> notes;
extern int score;
extern int combo;
extern bool isPlaying;

void startGame();
void updateGame();
void pressLane(int lane);
void showJudge(const std::wstring& text);