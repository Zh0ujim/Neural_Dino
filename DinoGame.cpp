#include "DinoGame.h"
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <cmath>
#include <climits> // 添加这个头文件以确保 INT_MAX 被正确定义
#include <unistd.h>
#include "maxlab/include/maxlab/maxlab.h"
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <random>
#include <atomic>
#include <unordered_set>
#include <algorithm>
#include <iomanip>
#include <fstream>

// 指定的通道列表
const std::vector<int> specified_channels = {635, 849, 445, 569, 81, 163, 273, 971};

// 通道数量，即指定的通道数
const int num_channels = specified_channels.size();

// 用于存储刺激后100ms内的数据样本
struct DataSample {
    std::vector<double> spike_rates; // 指定通道的尖峰放电率
    int label;                       // 刺激序列编号（0或1）
};

// 共享队列，用于存储待训练的数据样本
std::queue<DataSample> sampleQueue;
std::mutex queueMutex;
std::condition_variable dataCondition;

// 控制程序运���状态的标志
std::atomic<bool> isRunning(true);

// 添加 LogisticRegression 类的定义
class LogisticRegression {
public:
    LogisticRegression(int num_features, double learning_rate)
        : w(num_features, 0.0), b(0.0), lr(learning_rate) {}

    // Sigmoid 函数
    double sigmoid(double z) {
        return 1.0 / (1.0 + std::exp(-z));
    }

    // 预测概率
    double predict_proba(const std::vector<double>& x) {
        double z = b;
        for (size_t i = 0; i < w.size(); ++i) {
            z += w[i] * x[i];
        }
        return sigmoid(z);
    }

    // 对输入样本进行分类，返回 0 或 1
    int predict(const std::vector<double>& x) {
        return predict_proba(x) >= 0.5 ? 1 : 0;
    }

    // 使用 SGD 更新参数
    void update(const std::vector<double>& x, int y) {
        double y_pred = predict_proba(x);
        double error = y_pred - y;

        // 更新权重
        for (size_t i = 0; i < w.size(); ++i) {
            w[i] -= lr * error * x[i];
        }
        // 更新偏置
        b -= lr * error;
    }

    // 计算损失
    double compute_loss(const std::vector<double>& x, int y) {
        double y_pred = predict_proba(x);
        return -y * std::log(y_pred + 1e-10) - (1 - y) * std::log(1 - y_pred + 1e-10);
    }

    // 获取权重
    const std::vector<double>& get_weights() const {
        return w;
    }

    // 获取偏置
    double get_bias() const {
        return b;
    }

private:
    std::vector<double> w; // 权重向量
    double b;              // 偏置
    double lr;             // 学习率
};

// 模型参数（逻辑回归模型）
double learning_rate = 0.001; // 根据需要调整学习率
LogisticRegression model(num_channels, learning_rate);

// 用于同步刺激发送和数据收集的条件变量和互斥锁
std::mutex stimulationMutex;
std::condition_variable stimulationCondition;
std::atomic<bool> stimulationTriggered(false);
std::atomic<int> currentLabel(-1); // 存储当前刺激的标签

// 训练次数计数
std::atomic<int> iteration_count(0);

// 定义用于归一化的全局变量
std::vector<double> feature_means(num_channels, 0.0);
std::vector<double> feature_stds(num_channels, 1.0); // 初始标准差为1，避免除以零
std::vector<double> M2(num_channels, 0.0);
int sample_count = 0;

// 数据收集函数，收集刺激后100ms内的数据
void collectDataAfterStimulation(int label) {
    // 定义一个容器，存储指定通道的尖峰计数
    std::vector<int> spike_counts(num_channels, 0);

    // 创建一个集合，方便快速判断通道是否在指定列表中
    std::unordered_set<int> channel_set(specified_channels.begin(), specified_channels.end());

    // 记录开始时间
    auto start_time = std::chrono::steady_clock::now();
    
    // 先等待120ms
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    
    // 更新开始时间
    start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + std::chrono::milliseconds(100); // 100ms窗口

    while (isRunning && std::chrono::steady_clock::now() < end_time) {
        maxlab::FilteredFrameData frameData;
        maxlab::Status status = maxlab::DataStreamerFiltered_receiveNextFrame(&frameData);

        if (status != maxlab::Status::MAXLAB_OK) {
            maxlab::Response response = maxlab::sendRaw("get_errors");
            fprintf(stderr, "An error occured: %s\n", response.content);
            maxlab::freeResponse(&response);
        }

        if (status == maxlab::Status::MAXLAB_NO_FRAME)
            continue;

        // 遍历尖峰事件，统计指定通道的尖峰计数
        for (int i = 0; i < frameData.spikeCount; ++i) {
            const maxlab::SpikeEvent &spike = frameData.spikeEvents[i];
            int channel = spike.channel;
            if (channel_set.find(channel) != channel_set.end()) {
                // 获取指定通道的索引
                auto it = std::find(specified_channels.begin(), specified_channels.end(), channel);
                int index = std::distance(specified_channels.begin(), it);
                spike_counts[index]++;
            }
        }
    }

    // 计算放电率（可根据需要进行归一化）
    std::vector<double> spike_rates;
    for (int count : spike_counts) {
        spike_rates.push_back(static_cast<double>(count)); // 放电次数，即尖峰计数
    }

    // 增量更新均值和标准差
    sample_count++;
    for (int i = 0; i < num_channels; ++i) {
        double x = spike_rates[i];
        // 更新均值
        double delta = x - feature_means[i];
        feature_means[i] += delta / sample_count;
        // 更新 M2
        double delta2 = x - feature_means[i];
        M2[i] += delta * delta2;
        // 计算标准差
        if (sample_count > 1) {
            double variance = M2[i] / (sample_count - 1);
            feature_stds[i] = std::sqrt(variance);
            if (feature_stds[i] == 0.0) {
                feature_stds[i] = 1.0; // 防止除以零
            }
        }
        // 对特征进行归一化
        spike_rates[i] = (x - feature_means[i]) / feature_stds[i];
    }

    // 将数据样本加入队列，等待模型训练
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        sampleQueue.push({spike_rates, label});
    }
    dataCondition.notify_one();
}

// 引入头文件
#include <fstream>
#include <iomanip>

// 定义全局变量
std::ofstream log_file;
std::ofstream csv_file;
std::atomic<int> correct_predictions(0);
std::atomic<double> total_loss(0.0);

// 初始化日志文件和 CSV 文件
void initializeLogs() {
    log_file.open("training_log.txt");
    csv_file.open("training_metrics.csv");
    if (!log_file.is_open() || !csv_file.is_open()) {
        std::cerr << "无法打开日志文件或 CSV 文件" << std::endl;
        return;
    }
    csv_file << "iteration,accuracy,avg_loss,current_loss,prediction,true_label,prediction_prob\n";
}

// 修改模型训练线程
void modelTrainingThread() {
    initializeLogs();
    while (isRunning) {
        std::unique_lock<std::mutex> lock(queueMutex);
        dataCondition.wait(lock, []{ return !sampleQueue.empty() || !isRunning.load(); });

        if (!isRunning && sampleQueue.empty())
            break;

        // 获取数据样本
        DataSample sample = sampleQueue.front();
        sampleQueue.pop();
        lock.unlock();

        iteration_count++;

        // 预测和计算损失
        double y_pred = model.predict_proba(sample.spike_rates);
        int prediction = model.predict(sample.spike_rates);
        double loss = model.compute_loss(sample.spike_rates, sample.label);

        // 更新统计信息
        total_loss += loss;
        if (prediction == sample.label) {
            correct_predictions++;
        }
        double accuracy = static_cast<double>(correct_predictions.load()) / iteration_count.load();
        double avg_loss = total_loss.load() / iteration_count.load();

        // 将关键信息写入 CSV 文件
        csv_file << iteration_count.load() << ","
                 << accuracy << ","
                 << avg_loss << ","
                 << loss << ","
                 << prediction << ","
                 << sample.label << ","
                 << y_pred << "\n";

        // 每10次迭代记录日志
        if (iteration_count.load() % 1 == 0) {
            log_file << "\n第 " << iteration_count.load() << " 次迭代：\n";
            log_file << "----------------------------------------\n";
            log_file << "累计准确率: " << accuracy * 100 << "%\n";
            log_file << "平均损失: " << avg_loss << "\n";
            // 打印当前权重
            log_file << "当前权重: [";
            const auto& weights = model.get_weights();
            for (size_t i = 0; i < weights.size(); ++i) {
                log_file << std::fixed << std::setprecision(6) << weights[i];
                if (i != weights.size() - 1) log_file << ", ";
            }
            log_file << "]\n";
            log_file << "当前偏置: " << model.get_bias() << "\n";
            log_file << "----------------------------------------\n";

            // 在控制台显示进度
            std::cout << "进度: " << iteration_count.load()
                      << ", 准确率: " << accuracy * 100 << "%, "
                      << "平均损失: " << avg_loss << std::endl;
        }

        // 使用模型更新参数
        model.update(sample.spike_rates, sample.label);
    }

    std::cout << "模型训练线程结束" << std::endl;

    // 关闭日志文件
    if (log_file.is_open()) {
        log_file.close();
    }
    if (csv_file.is_open()) {
        csv_file.close();
    }
}

// 数据收集线程
void dataCollectionThread() {
    while (isRunning) {
        std::unique_lock<std::mutex> lock(stimulationMutex);
        stimulationCondition.wait(lock, []{ return stimulationTriggered.load() || !isRunning.load(); });

        if (!isRunning)
            break;

        int label = currentLabel.load();
        stimulationTriggered = false;
        lock.unlock();

        collectDataAfterStimulation(label);
    }
    std::cout << "数据收集线程结束" << std::endl;
}

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

void message_thread() {
    maxlab::checkVersions();
    maxlab::verifyStatus(maxlab::DataStreamerFiltered_open(maxlab::FilterType::IIR));
    printf("thread\n");

    uint64_t isi = 0;
    
    maxlab::FilteredFrameData frameData;
    int distance;
    
    bool reset_isi = true;
    bool reset_sti = true;
    auto last_stimulus_time = std::chrono::steady_clock::now();
    while (isRunning) {
        distance = calculateDistance(TheDINO_Rect[0], Obstacle_Use);
        //printf("distance(thread): %d\n", distance);

        // maxlab::Status status = maxlab::DataStreamerFiltered_receiveNextFrame(&frameData);
        // if (status == maxlab::Status::MAXLAB_NO_FRAME)
        //     continue;

        // 获取当前时间
        auto now = std::chrono::steady_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_stimulus_time).count();

        if (distance > 1500 && elapsed_time >= 4000) {
            const maxlab::Status status = maxlab::sendSequence("close_loop1");
            last_stimulus_time = now;
            {
                std::lock_guard<std::mutex> lock(stimulationMutex);
                stimulationTriggered = true;
                currentLabel = 1; // 设置标签
            }
            stimulationCondition.notify_one();
        } else if (distance <= 1500 && distance > 200 && elapsed_time >= 2 * (distance * 20) / 20) {
            const maxlab::Status status = maxlab::sendSequence("close_loop1");
            last_stimulus_time = now;
            {
                std::lock_guard<std::mutex> lock(stimulationMutex);
                stimulationTriggered = true;
                currentLabel = 1; // 设置标签
            }
            stimulationCondition.notify_one();
        } 
        else if (distance <= 200 && distance > 120 ) {
            if (reset_sti) {
                const maxlab::Status status = maxlab::sendSequence("trigger");
                reset_sti = false;
                last_stimulus_time = now;
                {
                    std::lock_guard<std::mutex> lock(stimulationMutex);
                    stimulationTriggered = true;
                    currentLabel = 0; // 设置标签
                }
                stimulationCondition.notify_one();
            }
        } 
        // else if (distance <= 120 && distance > 100) {
        //     if (reset_sti) {
        //         const maxlab::Status status = maxlab::sendSequence("close_loop2");
        //         reset_sti = false;
        //         last_stimulus_time = now;
        //         {
        //             std::lock_guard<std::mutex> lock(stimulationMutex);
        //             stimulationTriggered = true;
        //             currentLabel = 0; // 设置标签
        //         }
        //         stimulationCondition.notify_one();
        //     }
        // } 
        else {
            reset_sti = true;
        }

        // if (status != maxlab::Status::MAXLAB_OK) {
        //     maxlab::Response response = maxlab::sendRaw("get_errors");
        //     fprintf(stderr, "An error occured: %s\n", response.content);
        //     maxlab::freeResponse(&response);
        // }

        // spikes_count = frameData.spikeCount;

        // if (spikes_count >= 20) {
        //     printf("spike count(thread): %d\n", spikes_count.load());
        //     jump = 1;
        // }

        // printf("jump(thread)=%d\n", jump.load());
    }
    std::cout << "消息线程结束" << std::endl;
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
        Obstacle_Use[i] = { -1, Obstacle_Spacing * (i + 2), false };
    }

    // 更新最高分数显��
    unsigned long temp = highestscore % 1000000;
    for (int i = 5; i >= 0; i--) {
        Score[i] = '0';
        HI[i + 3] = temp % 10 + '0';
        temp /= 10;
    }

    std::cout << "Set Parameter" <<std::endl;
}

void DinoGame::Jump() {
    // 记录恐龙初始的���坐标
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
    std::thread dataCollectorThread(dataCollectionThread);
    std::thread trainingThread(modelTrainingThread);
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
                    std::cout << "quit" << std::endl;
                    stop_thread = true;
                    isRunning = false;
                    // 通知所有等待的条件变量
                    stimulationCondition.notify_all();
                    dataCondition.notify_all();
                    t.join();
                    dataCollectorThread.join();
                    trainingThread.join();
                    return;
                    break;

                case SDL_KEYDOWN:
                    switch (MainEvent.key.keysym.sym)
                    {
                        case SDLK_ESCAPE:
                            std::cout << "esc" << std::endl;
                            stop_thread = true;
                            isRunning = false;
                            // 通知所有等待的条件变量
                            stimulationCondition.notify_all();
                            dataCondition.notify_all();
                            t.join();
                            dataCollectorThread.join();
                            trainingThread.join();
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
            rate = 1;
        }
        else if (score_m >= 10000)
        {
            rate = 1;
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

        if (life < 0) {
            // 调用新的 RenderGameover 函数
            renderer.RenderGameover(Hit_Texture, Gameover_Texture, Restart_Texture, Hit_Rect, Gameover_Rect, Restart_Rect, crouch, TheDINO_Rect, Hit_Surface);
            std::cout << "Game Over" << std::endl;
            // 等一秒钟
            SDL_Delay(1000);

            // 自动重新开始游戏
            if (score_m / 5 > highestscore) {
                highestscore = score_m / 5;
            }

            SDL_FreeSurface(HI_Surface);
            SDL_DestroyTexture(HI_Texture);
            Set();
            std::cout << "Set" << std::endl;
            SDL_Delay(100);
        } else {
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
                        isRunning = false;
                        t.join();
                        dataCollectorThread.join();
                        trainingThread.join();
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
    dataCollectorThread.join();
    trainingThread.join();
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


