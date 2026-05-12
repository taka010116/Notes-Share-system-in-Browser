// chart.cpp
#include <filesystem>
#include <fstream>
#include "json.hpp"
#include "chart.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

Chart g_chart;

std::string song_folder;

void chart_load(const std::string& path){

    song_folder = fs::path(path).parent_path().string();

    std::ifstream ifs(path);
    json j; ifs >> j;

    g_chart.notes.clear();
    g_chart.bga.clear();

    g_chart.bpm = j["meta"]["bpm"];

    for(auto& n : j["notes"]){
        Note note;
        note.lane = n["lane"];
        note.row = n["row"];
        note.wav = n["wav"];
        g_chart.notes.push_back(note);
    }
}