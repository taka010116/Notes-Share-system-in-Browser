// game.cpp
#include "game.h"
#include "chart.h"
#include "audio.h"

GameState g_state;

void game_init(){
    g_state.score = 0;
    g_state.combo = 0;
}

void game_hit(int lane, double now){

    for(auto& n : g_chart.notes){

        if(n.hit || n.lane != lane) continue;

        double t = n.row * (60.0/g_chart.bpm/4.0);
        double diff = now - t;

        if(fabs(diff) < 0.08){

            n.hit = true;

            audio_play(song_folder + "/" + n.wav);

            g_state.combo++;
            g_state.score += 100;
            return;
        }
    }
}