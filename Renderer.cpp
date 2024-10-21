#include "Renderer.h"
#include <iostream>


Renderer::Renderer() : Window(nullptr), Renderer_(nullptr) {}

Renderer::~Renderer() {
    if (Renderer_) {
        SDL_DestroyRenderer(Renderer_);
    }
    if (Window) {
        SDL_DestroyWindow(Window);
    }
    SDL_Quit();
}

bool Renderer::Initialize(const std::string& title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
        std::cout << "Init Failed" <<std::endl;
    }
    std::cout << "Init Successful" <<std::endl;
    SDL_StopTextInput();
    TTF_Init();
    Mix_Init(MIX_INIT_MP3);
    
    Window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    if (!Window) {
        return false;
    }
    Renderer_ = SDL_CreateRenderer(Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!Renderer_) {
        return false;
    }
    return true;
}

void Renderer::Clear() {
    SDL_SetRenderDrawColor(Renderer_, 255, 255, 255, 255);
    SDL_RenderClear(Renderer_);
}

void Renderer::Present() {
    SDL_RenderPresent(Renderer_);
}

void Renderer::LoadTexture(const std::string& filePath, SDL_Texture*& texture, SDL_Rect& rect) {
    SDL_Surface* surface = IMG_Load(filePath.c_str());
    texture = SDL_CreateTextureFromSurface(Renderer_, surface);
    rect = { 0, 0, surface->w, surface->h };
    SDL_FreeSurface(surface);
}

void Renderer::RenderTexture(SDL_Texture* texture, SDL_Rect* srcRect, SDL_Rect* dstRect) {
    SDL_RenderCopy(Renderer_, texture, srcRect, dstRect);
}

void Renderer::RenderBackground(SDL_Texture* texture, SDL_Rect* rects) {
    for (int i = 0; i < 2; i++)
    {
        Road_Rect[i].x -= V;
        if (Road_Rect[i].x < -Road_Surface->w)
        {
            Road_Rect[i].x += Road_Surface->w * 2;
        }
        SDL_RenderCopy(Renderer_, Road_Texture, NULL, Road_Rect + i);
    }


    for (int i = 0; i < 4; i++)
    {
        Cloud_Rect[i].x -= V / 4;
        if (Cloud_Rect[i].x < -Cloud_Surface->w)
        {
            Cloud_Rect[i].x = Width_Window;
        }
        SDL_RenderCopy(Renderer_, Cloud_Texture, NULL, Cloud_Rect + i);
    }

}

void Renderer::RenderObstacle(SDL_Surface* surface[], SDL_Rect rects[], int count) {
    //Select Obstacle Randomly
    if (Obstacle_Use[0].space == Width_Window || Obstacle_Use[1].space == Width_Window || Obstacle_Use[2].space == Width_Window)
    {
        for (int i = 0; i < 3; i++)
        {
            if (Obstacle_Use[i].space == Width_Window)
            {
                Obstacle_Use[i].detect = false;
                int t = score_m > 500 ? 9 : 5;
                Obstacle_Use[i].Obstacle_i = rand() % t;

                if (Obstacle_Use[i].Obstacle_i >= 7)//Bird作为Obstacle
                {
                    int pre_x = Width_Window + MinInterval_Half + rand() % (Width_Window / 2 - MinInterval_Half * 2 - Birds_Rect[i].w);
                    //鸟的扇动翅膀动画
                    for (int h = 0; h < 2; h++)
                    {
                        Obstacle_Use[i].Rect[h] = Birds_Rect[h];
                        Obstacle_Use[i].Rect[h].x = pre_x;
                    }
                    break;
                }

                Obstacle_Use[i].Rect[0] = Obstacles_Rect[Obstacle_Use[i].Obstacle_i];
                Obstacle_Use[i].Rect->x = Width_Window + MinInterval_Half + rand() % (Width_Window / 2 - MinInterval_Half * 2 - Obstacle_Use[i].Rect->w);

                break;
            }
        }
    }
    for (int i = 0; i < 3; i++)
    {
        Obstacle_Use[i].space -= V;

        if (Obstacle_Use[i].space < -Width_Window / 2)
        {
            Obstacle_Use[i].space = Width_Window;
        }

        if (Obstacle_Use[i].Obstacle_i > -1)
        {
            if (Obstacle_Use[i].Obstacle_i >= 7)
            {
                r_bird[i] >>= 1;
                if (r_bird[i] == 0)
                {
                    r_bird[i] = 2147516415;
                }
                Obstacle_Use[i].Rect[0].x -= V;
                Obstacle_Use[i].Rect[1].x -= V;

                SDL_RenderCopy(Renderer_, Birds_Texture, birds_rect + r_bird[i] % 2, Obstacle_Use[i].Rect + r_bird[i] % 2);
            }
            else
            {
                Obstacle_Use[i].Rect->x -= V;

                SDL_RenderCopy(Renderer_, Obstacle_Texture[Obstacle_Use[i].Obstacle_i], NULL, Obstacle_Use[i].Rect);
            }
        }
    }
}

void Renderer::RenderDino(SDL_Texture* texture, SDL_Rect* rect) {
    if (down)
    {
        if (jump && TheDINO_Rect[0].y != Dino_menu_Rect.y)//Rapidly Drop
        {
            j = j <= V * Tan ? 2 * V * Tan + 1 - j : j;
            for (int i = 0; i < 3 && jump; i++)
            {
                std_ += (j - V * Tan) * 2 * Height_Window / (double)(3 * V * Tan * V * Tan);
                TheDINO_Rect[0].y = std_;
                j++;
                if (j == 2 * V * Tan + 1)
                {
                    j = 0;
                    TheDINO_Rect->y = Dino_menu_Rect.y;//Reset TheDINO_Rect[0].y(bug来自于std取整)
                    jump = false;
                }
            }
            SDL_RenderCopy(Renderer_, Blinking_Texture, NULL, TheDINO_Rect);
        }
        else//Crouch
        {
            SDL_RenderCopy(Renderer_, Crouching_Texture, crouching_rect + r % 2, TheDINO_Rect + 1);
            r >>= 1;
            if (r == 16)
            {
                r = 4111;
            }
            jump = false;
            crouch = true;
        }
    }
    else if (jump)
    {
        std_ += (j - V * Tan) * 2 * Height_Window / (double)(3 * V * Tan * V * Tan);
        TheDINO_Rect[0].y = std_;
        j++;
        SDL_RenderCopy(Renderer_, Blinking_Texture, NULL, TheDINO_Rect);
        if (j == 2 * V * Tan + 1)
        {
            j = 0;
            TheDINO_Rect->y = Dino_menu_Rect.y;//Reset TheDINO_Rect[0].y(bug来自于std取整)
            jump = false;
        }
    }
    else//Running
    {
        SDL_RenderCopy(Renderer_, Running_Texture, running_rect + r % 2, TheDINO_Rect);
        r >>= 1;
        if (r == 16)
        {
            r = 4111;
        }
    }
}

void Renderer::RenderScore(unsigned long score, TTF_Font* font, SDL_Rect& Score_Rect) {
    std::string Score(7,'0') ;
    for (int i = 5; i >= 0 && score != 0; i--)
    {
        Score[i] = score % 10 + '0';
        score /= 10;
    }

    SDL_Surface* Score_Surface = TTF_RenderUTF8_Blended(font, Score.c_str(), Score_Color);
    if (Score_Surface == nullptr) {
        std::cerr << "Failed to render score surface: " << TTF_GetError() << std::endl;
        return;
    }
    SDL_Texture* Score_Texture = SDL_CreateTextureFromSurface(Renderer_, Score_Surface);
    Score_Rect = SDL_Rect{ Width_Window - Score_Surface->w - 20,20,Score_Surface->w,Score_Surface->h };
    

    SDL_Surface* HI_Surface = TTF_RenderUTF8_Blended(Score_Font, HI, Score_Color);
    if (HI_Surface == nullptr) {
        std::cerr << "Failed to render HI surface: " << TTF_GetError() << std::endl;
        return;
    }
    SDL_Texture* HI_Texture = SDL_CreateTextureFromSurface(Renderer_, HI_Surface);
    HI_Rect = SDL_Rect{Width_Window * 0.8 - HI_Surface->w -20 ,20,HI_Surface->w,HI_Surface->h };
    

    SDL_RenderCopy(Renderer_, Score_Texture, nullptr, &Score_Rect);
    SDL_RenderCopy(Renderer_, HI_Texture, NULL, &HI_Rect);

    SDL_FreeSurface(Score_Surface);
    SDL_DestroyTexture(Score_Texture);
    
}

void Renderer::RenderGameover(SDL_Texture* hitTexture, SDL_Texture* gameoverTexture, SDL_Texture* restartTexture, SDL_Rect& hitRect, SDL_Rect& gameoverRect, SDL_Rect& restartRect, bool crouch, SDL_Rect* theDinoRect, SDL_Surface* hitSurface) {
    if (crouch) {
        hitRect = { theDinoRect[1].x + theDinoRect[1].w - hitSurface->w, theDinoRect[1].y + 2, hitSurface->w, hitSurface->h };
    } else {
        hitRect = { theDinoRect[0].x + theDinoRect[0].w - hitSurface->w, theDinoRect[0].y, hitSurface->w, hitSurface->h };
    }
    SDL_RenderCopy(Renderer_, hitTexture, NULL, &hitRect);
    SDL_RenderCopy(Renderer_, gameoverTexture, NULL, &gameoverRect);
    SDL_RenderCopy(Renderer_, restartTexture, NULL, &restartRect);
    SDL_RenderPresent(Renderer_);
}

void Renderer::DestroyTexture(SDL_Texture*& texture) {
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}

SDL_Renderer* Renderer::GetRenderer() const {
    return Renderer_;
}

SDL_Window* Renderer::GetWindow() const {
    return Window;
}
