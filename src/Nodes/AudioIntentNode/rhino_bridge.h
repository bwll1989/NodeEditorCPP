// rhino_bridge.h
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Rhino初始化结果结构体
 */
typedef struct {
    int status;           // 状态码 (对应 pv_status_t)
    void* handle;         // Rhino实例句柄
    int is_understood;    // 是否理解命令 (0=false, 1=true)
    const char* intent;   // 意图字符串
} rhino_result_t;

/**
 * 初始化Rhino实例
 * @param access_key Picovoice访问密钥
 * @param model_path 模型文件路径
 * @param context_path 上下文文件路径
 * @param sensitivity 敏感度 (0.0-1.0)
 * @param endpoint_duration_sec 端点持续时间（秒）
 * @param require_endpoint 是否需要端点 (0=false, 1=true)
 * @return 初始化结果
 */
rhino_result_t rhino_init_wrapper(
    const char* access_key,
    const char* model_path,
    const char* context_path,
    float sensitivity,
    float endpoint_duration_sec,
    int require_endpoint);

/**
 * 删除Rhino实例
 * @param handle Rhino实例句柄
 */
void rhino_delete_wrapper(void* handle);

/**
 * 处理音频帧
 * @param handle Rhino实例句柄
 * @param pcm 音频数据 (16位PCM)
 * @param is_finalized 输出参数：是否完成推理
 * @return 状态码
 */
int rhino_process_wrapper(void* handle, const short* pcm, int* is_finalized);

/**
 * 检查是否理解命令
 * @param handle Rhino实例句柄
 * @param is_understood 输出参数：是否理解
 * @return 状态码
 */
int rhino_is_understood_wrapper(void* handle, int* is_understood);

/**
 * 获取意图信息
 * @param handle Rhino实例句柄
 * @param intent 输出参数：意图字符串
 * @param num_slots 输出参数：槽位数量
 * @param slots 输出参数：槽位名称数组
 * @param values 输出参数：槽位值数组
 * @return 状态码
 */
int rhino_get_intent_wrapper(
    void* handle,
    const char** intent,
    int* num_slots,
    const char*** slots,
    const char*** values);

/**
 * 释放槽位和值的内存
 * @param handle Rhino实例句柄
 * @param slots 槽位名称数组
 * @param values 槽位值数组
 * @return 状态码
 */
int rhino_free_slots_and_values_wrapper(
    void* handle,
    const char** slots,
    const char** values);

/**
 * 重置Rhino状态
 * @param handle Rhino实例句柄
 * @return 状态码
 */
int rhino_reset_wrapper(void* handle);

/**
 * 获取Rhino帧长度
 * @return 每帧采样数
 */
int rhino_frame_length_wrapper(void);

/**
 * 获取Picovoice采样率
 * @return 采样率
 */
int rhino_sample_rate_wrapper(void);

/**
 * 获取状态码的字符串表示
 * @param status 状态码
 * @return 状态码字符串
 */
const char* rhino_status_to_string_wrapper(int status);

/**
 * 获取Rhino版本
 * @return 版本字符串
 */
const char* rhino_version_wrapper(void);

/**
 * 获取Rhino上下文信息
 * @param handle Rhino实例句柄
 * @param context_info 输出参数：上下文信息字符串
 * @return 状态码
 */
int rhino_context_info_wrapper(void* handle, const char** context_info);

// 状态码常量定义
#define RHINO_STATUS_SUCCESS 0
#define RHINO_STATUS_OUT_OF_MEMORY 1
#define RHINO_STATUS_IO_ERROR 2
#define RHINO_STATUS_INVALID_ARGUMENT 3
#define RHINO_STATUS_STOP_ITERATION 4
#define RHINO_STATUS_KEY_ERROR 5
#define RHINO_STATUS_INVALID_STATE 6
#define RHINO_STATUS_RUNTIME_ERROR 7
#define RHINO_STATUS_ACTIVATION_ERROR 8
#define RHINO_STATUS_ACTIVATION_LIMIT_REACHED 9
#define RHINO_STATUS_ACTIVATION_THROTTLED 10
#define RHINO_STATUS_ACTIVATION_REFUSED 11

#ifdef __cplusplus
}
#endif