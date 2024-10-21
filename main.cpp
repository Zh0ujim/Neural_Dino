#include "DinoGame.h"
#include <iostream>

int main(int argc, char* argv[]) {
    DinoGame game; // 创建游戏对象
    game.PrepareAll(); // 准备所有的资源和窗口

    // 渲染游戏的主菜单
    game.renderer.Clear();
    game.renderer.RenderTexture(Dino_menu_Texture, nullptr, &Dino_menu_Rect);
    game.renderer.Present();

    bool gameRunning = true;

    while (SDL_WaitEvent(&game.MainEvent)) {
        switch (game.MainEvent.type) {
            case SDL_QUIT: // 退出事件
                game.QUIT();
                gameRunning = false;
                break;

            case SDL_KEYDOWN: // 按键按下事件
                switch (game.MainEvent.key.keysym.sym) {
                    case SDLK_ESCAPE: // 按下ESC退出游戏
                        game.QUIT();
                        gameRunning = false;
                        break;

                    case SDLK_RETURN:
                    case SDLK_SPACE: // 按下回车或空格键开始游戏
                        // 清屏并渲染游戏开始界面
                        std::cout << "SPACE PRESSED" << std::endl;
                        game.renderer.Clear();
                        game.renderer.RenderTexture(Blinking_Texture, nullptr, TheDINO_Rect);
                        game.renderer.RenderTexture(Road_Texture, nullptr, Road_Rect);
                        game.renderer.Present();

                        // 执行跳跃逻辑并进入游戏主循环
                        game.Jump();
                        game.Play();
                        game.QUIT();
                        gameRunning = false;
                        break;

                    default:
                        break;
                }
                break;

            default:
                break;
        }

        // 如果游戏不再运行，跳出事件循环
        if (!gameRunning) {
            break;
        }
    }

    return 0;
}
