#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <string>
#include "Globals.h"

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Initialize(const std::string& title, int width, int height);
    void Clear();
    void Present();
    void LoadTexture(const std::string& filePath, SDL_Texture*& texture, SDL_Rect& rect);
    void RenderTexture(SDL_Texture* texture, SDL_Rect* srcRect, SDL_Rect* dstRect);
    void RenderBackground(SDL_Texture* texture, SDL_Rect* rects);
    void RenderObstacle(SDL_Surface* surface[], SDL_Rect rects[], int count);
    void RenderDino(SDL_Texture* texture, SDL_Rect* rect);
    void RenderScore(unsigned long score, TTF_Font* font, SDL_Rect& rect);
    void RenderGameover(SDL_Texture* hitTexture, SDL_Texture* gameoverTexture, SDL_Texture* restartTexture, SDL_Rect& hitRect, SDL_Rect& gameoverRect, SDL_Rect& restartRect, bool crouch, SDL_Rect* theDinoRect, SDL_Surface* hitSurface);
    void DestroyTexture(SDL_Texture*& texture);

    SDL_Renderer* GetRenderer() const;
    SDL_Window* GetWindow() const;




private:
    SDL_Window* Window;
    SDL_Renderer* Renderer_;
};

#endif
