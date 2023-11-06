#include "Timers.h"
#include "SDL.h"

uint64_t f = SDL_GetPerformanceFrequency();
uint64_t ct = SDL_GetPerformanceCounter();

float GetTimer()
{
    uint64_t c = SDL_GetPerformanceCounter();
    return float(((c - ct) * 1000) / (double)f);
}

uint64_t GetCurrentTime()
{
    return ((SDL_GetPerformanceCounter() - ct) * 1000 * 1000 * 1000) / f; //nano seconds
}
