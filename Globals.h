#ifndef GLOBALS_H
#define GLOBALS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <ctime>
#include <thread>
#include <atomic>


// 常量定义
constexpr int Width_Window = 1600;
constexpr int Height_Window = 350;
constexpr int MinInterval_Half = 100;
// 添加新的常量来增大障碍物间隔
constexpr int Obstacle_Spacing = 800; // 将值设置为800，可根据需要调整
constexpr int mFPS = 40;
constexpr int V = 6;
constexpr int Tan = 5;

// 声明全局变量
extern bool down, crouch, collision;
extern int j, life;
extern unsigned long score_m;
extern unsigned long highestscore;
extern unsigned int r;
extern unsigned long r_bird[3];
extern double rate;
extern double std_;
extern char Score[7];
extern char HI[10];
extern bool detect[3];

extern clock_t FDurTime;

extern SDL_Color Score_Color;
extern SDL_Color Gameover_Color;

// 游戏状态相关变量
extern SDL_Rect Dino_menu_Rect;
extern SDL_Rect Road_Rect[2];
extern SDL_Rect Cloud_Rect[4];
extern SDL_Rect TheDINO_Rect[2];
extern SDL_Rect running_rect[2];
extern SDL_Rect crouching_rect[2];
extern SDL_Rect Obstacles_Rect[14];
extern SDL_Rect Birds_Rect[2];
extern SDL_Rect birds_rect[2];
extern SDL_Rect Hit_Rect;
extern SDL_Rect Restart_Rect;
extern SDL_Rect Score_Rect;
extern SDL_Rect HI_Rect;
extern SDL_Rect Gameover_Rect;

extern SDL_Surface* Birds_Surface;
extern SDL_Texture* Birds_Texture;
extern SDL_Surface* Blinking_Surface;
extern SDL_Texture* Blinking_Texture;
extern SDL_Surface* Cloud_Surface;
extern SDL_Texture* Cloud_Texture;
extern SDL_Surface* Crouching_Surface;
extern SDL_Texture* Crouching_Texture;
extern SDL_Surface* Dino_menu_Surface;
extern SDL_Texture* Dino_menu_Texture;
extern SDL_Surface* Hit_Surface;
extern SDL_Texture* Hit_Texture;
extern SDL_Surface* Restart_Surface;
extern SDL_Texture* Restart_Texture;
extern SDL_Surface* Road_Surface;
extern SDL_Texture* Road_Texture;
extern SDL_Surface* Running_Surface;
extern SDL_Texture* Running_Texture;

extern SDL_Surface* Obstacle_Surface[7];
extern SDL_Texture* Obstacle_Texture[7];
extern SDL_Surface* Score_Surface;
extern SDL_Texture* Score_Texture;
extern SDL_Surface* HI_Surface;
extern SDL_Texture* HI_Texture;
extern SDL_Surface* Gameover_Surface;
extern SDL_Texture* Gameover_Texture;

extern Mix_Music* Bgm;
extern TTF_Font* Score_Font;
extern TTF_Font* Gameover_Font;

struct use {
    int Obstacle_i;
    int space;
    bool detect;
    SDL_Rect Rect[2];
};

extern use Obstacle_Use[3];

extern std::thread t;
extern std::atomic<int> spikes_count;
extern std::atomic<bool> stop_thread;
extern std::atomic<bool> jump;

extern int calculateDistance(SDL_Rect dino, struct use *obstacles); // 声明 calculateDistance 函数
extern void message_thread();

#endif // GLOBALS_H