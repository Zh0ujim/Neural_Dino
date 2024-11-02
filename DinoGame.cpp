#include "DinoGame.h"
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <cmath>

#include <climits> // 添加这个头文件以确保 INT_MAX 被正确定义

#include <unistd.h>
//#include "events.hpp"
#include "maxlab/include/maxlab/maxlab.h"
#include <chrono>

int calculateDistance(SDL_Rect dino, struct use *obstacles) {
    int min_distance = INT_MAX;
    for (int i = 0; i < 3; i++) {
        if (obstacles[i].Obstacle_i > -1) { // if the obstacle is valid
            int distance = obstacles[i].Rect->x - (dino.x + dino.w);
            if (distance < min_distance&&distance>0) {
                min_distance = distance;
            }
        }
    }
    return min_distance;
}

void message_thread(){
//    if (argc < 2) {
//        fprintf(stderr, "Call with: %s [detection_channel]", argv[0]);
//        exit(1);
//    }
//    const int detection_channel = atoi(argv[1]);

    maxlab::checkVersions();
    maxlab::verifyStatus(maxlab::DataStreamerFiltered_open(maxlab::FilterType::IIR));
    printf("thread\n");

    uint64_t isi = 0;
    
    maxlab::FilteredFrameData frameData;
    int distance;
    

    bool reset_isi=true;
    bool reset_sti=true;
    auto last_stimulus_time = std::chrono::steady_clock::now();
    //printf("stop_thread=%d\n",stop_thread.load());
    while (!stop_thread) {

        distance = calculateDistance(TheDINO_Rect[0], Obstacle_Use);
        printf("distance(thread): %d\n", distance);

        maxlab::Status status = maxlab::DataStreamerFiltered_receiveNextFrame(&frameData);
        if (status == maxlab::Status::MAXLAB_NO_FRAME)
            continue;

        // 获取当前时间
        auto now = std::chrono::steady_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_stimulus_time).count();

        if (distance > 1500 && elapsed_time >= 4000) {
            const maxlab::Status status = maxlab::sendSequence("close_loop1");
            last_stimulus_time = now;
        } else if (distance <= 1500 && distance > 200 && elapsed_time >= 2*(distance * 20)/20) {
            const maxlab::Status status = maxlab::sendSequence("close_loop1");
            last_stimulus_time = now;
        } else if (distance <= 200 && distance > 120 && elapsed_time >= (120 * 20)/20) {
            const maxlab::Status status = maxlab::sendSequence("close_loop1");
            last_stimulus_time = now;
        } else if (distance <= 120 && distance > 100 ) {
            if(reset_sti){    
                const maxlab::Status status = maxlab::sendSequence("close_loop2");
                reset_sti = false;
                last_stimulus_time = now;
            }
        } else {
            reset_sti = true;
        }

        if (status != maxlab::Status::MAXLAB_OK) {
            maxlab::Response response = maxlab::sendRaw("get_errors");
            fprintf(stderr, "An error occured: %s\n", response.content);
            maxlab::freeResponse(&response);
        }

        spikes_count = frameData.spikeCount;

        if (spikes_count >= 20) {
            printf("spike count(thread): %d\n", spikes_count.load());
            jump = 1;
        }

        printf("jump(thread)=%d\n", jump.load());

    //        for (int i = 0; i < frameData.spikeCount; ++i) {
    //            const maxlab::SpikeEvent &spike = frameData.spikeEvents[i];
    //            if (spike.channel == detection_channel) {
    //                const maxlab::Status status = maxlab::sendSequence("stimulation_sequence");
    //
    //                blanking = 8000;
    //
    //                if (status != maxlab::Status::MAXLAB_OK) {
    //                    maxlab::Response response = maxlab::sendRaw("get_errors");
    //                    fprintf(stderr, "An error occured: %s\n", response.content);
    //                    maxlab::freeResponse(&response);
    //                }
    //            }
    //        }
    }
    maxlab::verifyStatus(maxlab::DataStreamerFiltered_close());


}





DinoGame::DinoGame() {
    renderer.Initialize("MY DINO", Width_Window, Height_Window);
    
    Load();  // 加载资源
    PrepareAll();  // 准备所有必要资源
    //Set();  // 初始化游戏状态
}

DinoGame::~DinoGame() {
    QUIT(); // 清理所有资源
}

void DinoGame::Load() {
    // 加载字体
    Score_Font = TTF_OpenFont("fonts/KodeMono-VariableFont_wght.ttf", 32);
    if(Score_Font==nullptr) std::cout << "Score_Font Failed"<<std::endl;    
    Gameover_Font = TTF_OpenFont("fonts/CHILLER.TTF", 90);
    if(Gameover_Font==nullptr) std::cout << "Gameover_Font Failed"<<std::endl;


    // 加载图片资源
    Blinking_Surface = IMG_Load("images/Blinking.png");
    Birds_Surface = IMG_Load("images/Birds.png");
    Cloud_Surface = IMG_Load("images/Cloud.png");
    Crouching_Surface = IMG_Load("images/Crouching.png");
    Dino_menu_Surface = IMG_Load("images/Dino_menu.png");
    Hit_Surface = IMG_Load("images/Hit.png");
    Restart_Surface = IMG_Load("images/Restart.png");
    Road_Surface = IMG_Load("images/Road.png");
    Running_Surface = IMG_Load("images/Running.png"); 

    if(Running_Surface==nullptr) std::cout << "Running_Surface Failed"<<std::endl;

    // 加载障碍物资源
    for (int i = 0; i < 7; ++i) {
        Obstacle_Surface[i] = IMG_Load(("images/Obstacle_" + std::string(1, 'a' + i) + ".png").c_str());
    }

    // // 加载背景音乐
    // Bgm = Mix_LoadMUS("sounds/background.mp3");
    // if (Bgm == nullptr) {
    //     std::cerr << "Failed to load background music: " << Mix_GetError() << std::endl;
    // }
}

void DinoGame::PrepareAll() {
    // 创建窗口和渲染器，并准备纹理资源
    renderer.LoadTexture("images/Blinking.png", Blinking_Texture, TheDINO_Rect[0]);
    renderer.LoadTexture("images/Birds.png", Birds_Texture, Birds_Rect[0]);
    renderer.LoadTexture("images/Cloud.png", Cloud_Texture, Cloud_Rect[0]);
    renderer.LoadTexture("images/Crouching.png", Crouching_Texture, TheDINO_Rect[1]);
    renderer.LoadTexture("images/Dino_menu.png", Dino_menu_Texture, Dino_menu_Rect);
    renderer.LoadTexture("images/Hit.png", Hit_Texture, Hit_Rect);
    renderer.LoadTexture("images/Restart.png", Restart_Texture, Restart_Rect);
    renderer.LoadTexture("images/Road.png", Road_Texture, Road_Rect[0]);
    renderer.LoadTexture("images/Running.png", Running_Texture, running_rect[0]);

    

    for (int i = 0; i < 7; ++i) {
        renderer.LoadTexture("images/Obstacle_" + std::string(1, 'a' + i) + ".png", Obstacle_Texture[i], Obstacles_Rect[i]);
    }

    // 渲染“Game Over”字体
    Gameover_Surface = TTF_RenderUTF8_Blended(Gameover_Font, "G A M E  O V E R", Gameover_Color);
    if(Gameover_Surface==nullptr) std::cout << "Gameover_Surface Failed"<<std::endl;
    Gameover_Texture = SDL_CreateTextureFromSurface(renderer.GetRenderer(), Gameover_Surface);
    
    // 确定 Dino 矩形区域
    Dino_menu_Rect = {50, Height_Window - 120, Dino_menu_Surface->w, Dino_menu_Surface->h};
    TheDINO_Rect[0] = {Dino_menu_Rect.x + 4, Dino_menu_Rect.y, Blinking_Surface->w, Blinking_Surface->h};
    TheDINO_Rect[1] = {Dino_menu_Rect.x + 4, Dino_menu_Rect.y + 34, Crouching_Surface->w, Crouching_Surface->h / 2};

    // 设置跑道
    Road_Rect[0] = {0, Dino_menu_Rect.y + Dino_menu_Rect.h - 24, Road_Surface->w, Road_Surface->h};
    Road_Rect[1] = {Road_Surface->w, Dino_menu_Rect.y + Dino_menu_Rect.h - 24, Road_Surface->w, Road_Surface->h};
    
    // 云的矩形区域
    Cloud_Rect[0] = {static_cast<int>(0.11 * Width_Window), static_cast<int>(0.29 * Height_Window), Cloud_Surface->w, Cloud_Surface->h};
    Cloud_Rect[1] = {static_cast<int>(0.36 * Width_Window), static_cast<int>(0.23 * Height_Window), Cloud_Surface->w, Cloud_Surface->h};
    Cloud_Rect[2] = {static_cast<int>(0.61 * Width_Window), static_cast<int>(0.37 * Height_Window), Cloud_Surface->w, Cloud_Surface->h};
    Cloud_Rect[3] = {static_cast<int>(0.89 * Width_Window), static_cast<int>(0.21 * Height_Window), Cloud_Surface->w, Cloud_Surface->h};

    // 设置障碍物矩形区域
    for (int i = 0; i < 7; ++i) {
        Obstacles_Rect[i] = {0, Road_Rect[0].y - Obstacle_Surface[i]->h + 22, Obstacle_Surface[i]->w, Obstacle_Surface[i]->h};
    }

    for (int i = 0; i < 2; i++)
    {
        running_rect[i] = (SDL_Rect){ 0,Running_Surface->h / 2 * i,Running_Surface->w ,Running_Surface->h / 2 };
        crouching_rect[i] = (SDL_Rect){ 0,Crouching_Surface->h / 2 * i,Crouching_Surface->w,Crouching_Surface->h / 2 };
    }

    // 设置“Game Over”和“Restart”按钮的矩形区域
    Gameover_Rect = {(Width_Window - Gameover_Surface->w) / 2, Height_Window / 4, Gameover_Surface->w, Gameover_Surface->h};
    Restart_Rect = {(Width_Window - Restart_Surface->w) / 2, Gameover_Rect.y + Gameover_Surface->h + 5, Restart_Surface->w, Restart_Surface->h};
    
}

void DinoGame::Set() {
    // 初始化游戏状态和随机数
    srand(static_cast<unsigned int>(time(nullptr)));

    jump = false;
    down = false;
    crouch = false;
    collision = false;
    j = 0;
    life = 4;
    rate = 1.0;
    score_m = 0;
    r = 4111;
    
    for (int i = 0; i < 3; ++i) {
        r_bird[i] = 2147516415;
    }

    TheDINO_Rect[0].y = Dino_menu_Rect.y;
    std_ = Dino_menu_Rect.y;

    for (int i = 0; i < 3; ++i) {
        Obstacle_Use[i] = { -1, Width_Window / 2 * (i + 2), false };
    }

    // 更新最高分数显示
    unsigned long temp = highestscore % 1000000;
    for (int i = 5; i >= 0; i--) {
        Score[i] = '0';
        HI[i + 3] = temp % 10 + '0';
        temp /= 10;
    }

    std::cout << "Set Parameter" <<std::endl;
}

void DinoGame::Jump() {
    // 记录恐龙初始的纵坐标
    double t = TheDINO_Rect[0].y;

    // 进行跳跃动画的循环
    for (int i = 0; i < 2 * V * Tan + 1; i++)
    {
        // 计算跳跃位置的变化
        t += (i - V * Tan) * 2 * Height_Window / (double)(3 * V * Tan * V * Tan);

        // 更新 TheDINO_Rect[0] 的纵坐标
        TheDINO_Rect[0].y = t;

        // 清空渲染目标
        renderer.Clear();

        // 绘制恐龙纹理
        renderer.RenderTexture(Blinking_Texture, NULL, TheDINO_Rect);

        // 绘制道路纹理
        renderer.RenderTexture(Road_Texture, NULL, Road_Rect);

        // 更新渲染目标
        renderer.Present();

        // 延迟，控制动画速度
        SDL_Delay(1000 / mFPS);
    }
    TheDINO_Rect->y = Dino_menu_Rect.y;
}

void DinoGame::Play() {

    std::thread t(message_thread);
    //t.detach();
    printf("start thread\n");
    sleep(1);

    Set();

    std::cout << "start game" << std::endl;

    bool pause;
    int blanking = 0;

    while (true)
    {
        clock_t FStartTime = clock();
        if (SDL_PollEvent(&MainEvent))
        {
            switch (MainEvent.type)
            {
                case SDL_QUIT:
                    stop_thread = true;
                    t.join();
                    return;
                    break;

                case SDL_KEYDOWN:
                    switch (MainEvent.key.keysym.sym)
                    {
                        case SDLK_ESCAPE:
                            stop_thread = true;
                            t.join();
                            return;
                            break;

                        case SDLK_DOWN:
                            down = true;
                            break;

                        case SDLK_UP:
                        case SDLK_SPACE:
                            std::cout << "space press" << std::endl;
                            jump = true;
                            break;

                        case SDLK_p:
                            pause = true;
                            break;

                        default:
                            break;
                    }
                    break;

                case SDL_KEYUP:
                    switch (MainEvent.key.keysym.sym)
                    {
                        case SDLK_DOWN:
                            down = false;
                            crouch = false;
                            break;

                        default:
                            break;
                    }
                    break;

                default:
                    break;
            }
        }

        score_m++;
        //Select Speed
        if (score_m >= 2500)
        {
            rate = 1.25;
        }
        else if (score_m >= 10000)
        {
            rate = 1.5;
        }


        // 渲染场景
        renderer.Clear();
        renderer.RenderBackground(Road_Texture, Road_Rect);
        renderer.RenderObstacle(Obstacle_Surface, Obstacles_Rect, 7);       
        renderer.RenderDino(Blinking_Texture, TheDINO_Rect);
        renderer.RenderScore(score_m / 5 % 1000000, Score_Font, Score_Rect);        
        renderer.Present();

        // 碰撞检测
        CD();

        if (life < 0)
        {
            // 调用新的 RenderGameover 函数
            renderer.RenderGameover(Hit_Texture, Gameover_Texture, Restart_Texture, Hit_Rect, Gameover_Rect, Restart_Rect, crouch, TheDINO_Rect, Hit_Surface);

            while (SDL_WaitEvent(&MainEvent))
            {
                bool Replay = false;
                switch (MainEvent.type)
                {
                    case SDL_QUIT:
                        stop_thread = true;
                        t.join();
                        return;
                        break;

                    case SDL_KEYDOWN:
                        switch (MainEvent.key.keysym.sym)
                        {
                            case SDLK_ESCAPE:
                                stop_thread = true;
                                t.join();
                                return;
                                break;

                            case SDLK_RETURN:
                                Replay = true;
                                break;
                            
                            case SDLK_p:
                                pause = true;
                                break;

                            default:
                                break;
                        }
                        break;

                    case SDL_MOUSEBUTTONDOWN:
                        if (MainEvent.button.x > Restart_Rect.x && MainEvent.button.y > Restart_Rect.y && MainEvent.button.x < Restart_Rect.x + Restart_Rect.w && MainEvent.button.y < Restart_Rect.y + Restart_Rect.h)
                        {
                            Replay = true;
                        }
                        break;

                    default:
                        break;
                }

                if (Replay)
                {
                    Mix_ResumeMusic();

                    if (score_m / 5 > highestscore)
                    {
                        highestscore = score_m / 5;
                    }

                    SDL_FreeSurface(HI_Surface);
                    SDL_DestroyTexture(HI_Texture);
                    Set();

                    break;
                }
            }
            SDL_Delay(100);
        }
        else
        {
            ControlFPS(FStartTime);
        }

        if ( pause) //暂停
        {
            // 调用新的 RenderGameover 函数
            renderer.RenderPause(Hit_Texture,  Hit_Rect, crouch, TheDINO_Rect, Hit_Surface);

            while (SDL_WaitEvent(&MainEvent))
            {
                bool Replay = false;
                switch (MainEvent.type)
                {
                    case SDL_QUIT:
                        stop_thread = true;
                        t.join();
                        return;
                        break;

                    case SDL_KEYDOWN:
                        switch (MainEvent.key.keysym.sym)
                        {
                            case SDLK_ESCAPE:
                                return;
                                break;

                            case SDLK_RETURN:
                                Replay = true;
                                break;

                            case SDLK_p:
                                pause = true;
                                break;

                            default:
                                break;
                        }
                        break;

                    default:
                        break;
                }

                if (Replay)
                {
                    pause = false;
                    Mix_ResumeMusic();

                    if (score_m / 5 > highestscore)
                    {
                        highestscore = score_m / 5;
                    }

//                    SDL_FreeSurface(HI_Surface);
//                    SDL_DestroyTexture(HI_Texture);
//                    Set();

                    break;
                }
            }
            SDL_Delay(100);
        }
        else
        {
            ControlFPS(FStartTime);
        }
    }

    t.join();
}

void DinoGame::ControlFPS(clock_t FStartTime) {
    FDurTime = clock() - FStartTime;
    if (FDurTime <= 1000 / mFPS / rate) {
        SDL_Delay(1000 / mFPS / rate - FDurTime);
    }
}

void DinoGame::CD() {
    for (int i = 0; i < 3; ++i) {
        if (SDL_HasIntersection(&TheDINO_Rect[0], &Obstacle_Use[i].Rect[0])) {
            collision = true;
            std::cout << "collision" << collision << std::endl;
            life--;
            return;
        }
    }
}

void DinoGame::QUIT() {
    //析构渲染器
    SDL_Quit();
}


