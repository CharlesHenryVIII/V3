#include "Misc.h"
#include "SDL.h"
#include "Math.h"

bool g_running = true;

u64 f = SDL_GetPerformanceFrequency();
u64 ct = SDL_GetPerformanceCounter();

float GetTimer()
{
    u64 c = SDL_GetPerformanceCounter();
    return float(((c - ct) * 1000) / (double)f);
}

u64 GetCurrentTime()
{
    return ((SDL_GetPerformanceCounter() - ct) * 1000 * 1000 * 1000) / f; //nano seconds
}
