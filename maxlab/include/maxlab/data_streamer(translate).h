/// @file data_streamer.h
/// @brief 数据流处理模块头文件

#pragma once  // 头文件保护指令

#include <stdint.h>      // 标准整型类型
#include "errors.h"      // 错误状态定义

/**
 * @namespace maxlab
 * @brief MaxLab C++ API 主命名空间
 */
namespace maxlab
{
struct SpikeEvent;  // 前向声明，尖峰事件数据结构

/**
 * @enum FilterType
 * @brief 滤波器类型枚举
 * 
 * 用于配置数据流的滤波方式
 */
enum class FilterType
{
    FIR = 0,    ///< 有限冲激响应滤波器，延迟约 2048/采样率(ms)
    IIR = 1     ///< 无限冲激响应滤波器，延迟约 10/采样率(ms)
};

/**
 * @struct FrameInfo
 * @brief 数据帧元信息
 * 
 * 包含数据帧的基础状态信息
 */
struct FrameInfo
{
    uint64_t frame_number = 0; ///< 递增帧号（64位无符号整型）
    uint8_t well_id = 0;       ///< 多孔板设备上的孔位ID（0-255）
    bool corrupted = false;    ///< 数据损坏标志（true表示数据不可靠）
};

/**
 * @struct FilteredFrameData
 * @brief 滤波后数据帧结构
 * 
 * 包含经过滤波处理后的尖峰事件数据
 * 
 * @warning 尖峰事件指针的有效范围是[0, spikeCount-1]
 *          访问超出范围会导致未定义行为
 */
struct FilteredFrameData
{
    uint64_t spikeCount;            ///< 检测到的尖峰数量（最大1024）
    const SpikeEvent* spikeEvents;   ///< 尖峰事件数组指针（只读）
};

/**
 * @struct RawFrameData
 * @brief 原始数据帧结构
 * 
 * 包含直接从设备读取的原始振幅数据
 */
struct RawFrameData
{
    FrameInfo frameInfo;            ///< 帧元信息 @see FrameInfo
    const static uint64_t amplitudeCount = 1024;  ///< 固定振幅数据长度（对应读入通道数）
    const float* amplitudes;       ///< 振幅数据数组指针（只读）
};

/* 滤波数据流操作接口 ------------------------------------------------------ */

/**
 * @brief 打开滤波数据流
 * @param filterType 滤波器类型 @see FilterType
 * @return 操作状态码：
 *         - MAXLAB_OK: 成功
 *         - MAXLAB_LICENSE_INVALID: 无效许可证
 *         - MAXLAB_STREAM_ALREADY_OPENED: 流已打开
 *         - MAXLAB_INCOMPATIBLE_FILTERING: 滤波不兼容
 *         - MAXLAB_NO_SERVER_CONNECTION: 服务器连接失败
 * @note 建议使用 verifyStatus 验证返回值
 */
Status DataStreamerFiltered_open(FilterType filterType);

/**
 * @brief 关闭滤波数据流
 * @return 操作状态码：
 *         - MAXLAB_OK: 成功
 *         - MAXLAB_NO_SERVER_CONNECTION: 服务器连接失败
 *         - MAXLAB_API_NO_RESPONSE: API无响应
 * @warning 实验结束后必须调用，失败需重启mxwserver
 */
Status DataStreamerFiltered_close();

/**
 * @brief 获取下一帧滤波数据
 * @param frameData 输出参数，用于接收帧数据
 * @return 操作状态码：
 *         - MAXLAB_OK: 成功获取数据
 *         - MAXLAB_INVALID_INPUT: 无效输入参数
 *         - MAXLAB_NO_SPIKES: 无尖峰数据
 * @note 非阻塞调用，需检查spikeCount > 0 后再访问数据
 */
Status DataStreamerFiltered_receiveNextFrame(FilteredFrameData* frameData);

/**
 * @brief 动态切换滤波器类型
 * @param filterType 新滤波器类型 @see FilterType
 * @return 操作状态码：
 *         - MAXLAB_OK: 成功
 *         - MAXLAB_STREAM_NOT_OPENED: 流未打开
 *         - MAXLAB_INCOMPATIBLE_FILTERING: 滤波不兼容
 */
Status DataStreamerFiltered_setFilterType(FilterType filterType);

/**
 * @brief 获取当前滤波器类型
 * @return 当前激活的滤波器类型 @see FilterType
 */
FilterType DataStreamerFiltered_getFilterType();

/* 原始数据流操作接口 ------------------------------------------------------ */

/**
 * @brief 打开原始数据流
 * @return 操作状态码 @see DataStreamerFiltered_open
 */
Status DataStreamerRaw_open();

/**
 * @brief 关闭原始数据流
 * @return 操作状态码 @see DataStreamerFiltered_close
 */
Status DataStreamerRaw_close();

/**
 * @brief 获取下一帧原始数据
 * @param frameData 输出参数，用于接收帧数据
 * @return 操作状态码：
 *         - MAXLAB_OK: 成功获取数据
 *         - MAXLAB_INVALID_INPUT: 无效输入参数
 *         - MAXLAB_STREAM_NOT_OPENED: 流未打开
 *         - MAXLAB_NO_FRAME: 无可用数据帧
 * @note 非阻塞调用，返回MAXLAB_NO_FRAME表示当前无数据
 */
Status DataStreamerRaw_receiveNextFrame(RawFrameData* frameData);

} // namespace maxlab