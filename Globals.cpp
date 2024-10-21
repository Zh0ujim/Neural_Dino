#include "Globals.h"

// 定义全局变量
bool down, crouch, collision;
int j, life=1;
unsigned long score_m;
unsigned long highestscore;
unsigned int r;
unsigned long r_bird[3];
double rate;
double std_;
char Score[7];
char HI[10] = "HI ";
bool detect[3];

clock_t FDurTime;

SDL_Color Score_Color;
SDL_Color Gameover_Color;

// 游戏状态相关变量
SDL_Rect Dino_menu_Rect;
SDL_Rect Road_Rect[2];
SDL_Rect Cloud_Rect[4];
SDL_Rect TheDINO_Rect[2];
SDL_Rect running_rect[2];
SDL_Rect crouching_rect[2];
SDL_Rect Obstacles_Rect[14];
SDL_Rect Birds_Rect[2];
SDL_Rect birds_rect[2];
SDL_Rect Hit_Rect;
SDL_Rect Restart_Rect;
SDL_Rect Score_Rect;
SDL_Rect HI_Rect;
SDL_Rect Gameover_Rect;

SDL_Surface* Birds_Surface;
SDL_Texture* Birds_Texture;
SDL_Surface* Blinking_Surface;
SDL_Texture* Blinking_Texture;
SDL_Surface* Cloud_Surface;
SDL_Texture* Cloud_Texture;
SDL_Surface* Crouching_Surface;
SDL_Texture* Crouching_Texture;
SDL_Surface* Dino_menu_Surface;
SDL_Texture* Dino_menu_Texture;
SDL_Surface* Hit_Surface;
SDL_Texture* Hit_Texture;
SDL_Surface* Restart_Surface;
SDL_Texture* Restart_Texture;
SDL_Surface* Road_Surface;
SDL_Texture* Road_Texture;
SDL_Surface* Running_Surface;
SDL_Texture* Running_Texture;

SDL_Surface* Obstacle_Surface[7];
SDL_Texture* Obstacle_Texture[7];
SDL_Surface* Score_Surface;
SDL_Texture* Score_Texture;
SDL_Surface* HI_Surface;
SDL_Texture* HI_Texture;
SDL_Surface* Gameover_Surface;
SDL_Texture* Gameover_Texture;

Mix_Music* Bgm;
TTF_Font* Score_Font;
TTF_Font* Gameover_Font;

use Obstacle_Use[3];

std::thread t;
std::atomic<int> spikes_count(0);
std::atomic<bool> stop_thread(false);
std::atomic<bool> jump(false);

