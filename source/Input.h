#pragma once
#include "SDL.h"
#include "imgui.h"

#include "Math.h"

#include <unordered_map>

extern bool g_running;

struct Key {
    bool down;
    bool downPrevFrame;
    bool downThisFrame;
    bool upThisFrame;
};

struct Mouse {
    Vec2Int pos = {};
    Vec2Int pDelta = {};
    Vec2 wheel = {}; //Y for vertical rotations, X for Horizontal rotations/movement
    Vec2 wheelInstant = {};
    bool wheelModifiedLastFrame = false;
    SDL_Cursor* cursors[ImGuiMouseCursor_COUNT] = {};
    float m_sensitivity = 0.4f;
    bool canUseGlobalState = true;
};

struct CommandHandler {
    std::unordered_map<i32, Key> keyStates;
    Mouse mouse = {};
    float m_sensitivity = 0.3f;

    void InputUpdate();
};
