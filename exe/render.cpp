// render.cpp
#include <windows.h>
#include <gdiplus.h>
#include "chart.h"
#include "game.h"

using namespace Gdiplus;

void render(HDC hdc){

    Graphics g(hdc);
    SolidBrush black(Color(0,0,0));

    g.FillRectangle(&black,0,0,1400,900);

    int laneW = 350/5;
    int laneX = 420-175;

    for(auto& n : g_chart.notes){

        if(n.hit) continue;

        SolidBrush b(Color(0,200,255));

        g.FillRectangle(&b,
            laneX + n.lane*laneW,
            n.y,
            laneW-4,
            18);
    }
}