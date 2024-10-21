#ifndef DINO_GAME_H
#define DINO_GAME_H

#include "Globals.h"
#include "Renderer.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <atomic>
#include <ctime>
#include <thread>


class DinoGame {
public:
    DinoGame();
    ~DinoGame();

    void Load();
    void PrepareAll();
    void Jump();
    void Play();
    void Set();
    void ControlFPS(clock_t FStartTime);
    void CD();
    void QUIT();

    SDL_Event MainEvent;

    Renderer renderer; // 渲染器对象
    
};

#endif
