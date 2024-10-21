cmake_minimum_required(VERSION 3.28)
project(Dino_1011)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug) # 设置构建类型为Debug
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb3") # 添加调试选项

include_directories("/home/maxwell/ZJM/SDL_all_in_one/include")
include_directories("/home/maxwell/ZJM/Neural-Dino/Neural_Dino/maxlab/include/maxlab")

link_directories("/home/maxwell/ZJM/SDL_all_in_one/lib")
link_directories("/home/maxwell/ZJM/Neural-Dino/Neural_Dino/maxlab/lib")

#虚拟机相关头文件静态库路径
# include_directories("/home/zjm/ZJM/SDL2_all_in_one/_install/include")
# link_directories("/home/zjm/ZJM/SDL2_all_in_one/_install/lib")

add_executable(Dino_1011 main.cpp DinoGame.cpp Renderer.cpp Globals.cpp)

target_link_libraries(Dino_1011 PRIVATE  maxlab pthread  SDL2main SDL2 SDL2_image SDL2_ttf SDL2_mixer)
