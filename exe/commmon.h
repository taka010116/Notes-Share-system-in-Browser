#pragma once

#include <string>
#include <vector>

struct Note {
    int lane = 0;
    int row = 0;
    std::string wav;
    bool hit = false;
    float y = 0;
};

struct BGAEvent {
    int row = 0;
    std::string bmp;
    bool played = false;
};

struct Chart {
    std::string title;
    std::string artist;
    std::string genre;
    std::string difficulty;
    int level = 1;
    float bpm = 120.0f;

    std::vector<Note> notes;
    std::vector<BGAEvent> bga;
};