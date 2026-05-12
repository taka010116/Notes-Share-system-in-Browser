#pragma once
#include <string>
#include <vector>

struct Note;
struct BGAEvent;
struct Chart;

bool loadChart(const std::string& path);

extern Chart chart;
extern std::string currentSongFolder;