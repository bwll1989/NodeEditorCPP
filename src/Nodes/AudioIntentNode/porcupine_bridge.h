// porcupine_bridge.h
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Porcupine初始化结果结构体
 */
typedef struct {
    int status;           // 状态码 (对应 pv_status_t)
    void* handle;         // Porcupine实例句柄
    int keyword_index;    // 检测到的关键词索引 (-1表示未检测到)
} porcupine_result_t;

/**
 * 初始化Porcupine实例
 * @param access_key Picovoice访问密钥
 * @param model_path 模型文件路径
 * @param num_keywords 关键词数量
 * @param keyword_paths 关键词文件路径数组
 * @param sensitivities 敏感度数组 (0.0-1.0)
 * @return 初始化结果
 */
porcupine_result_t porcupine_init_wrapper(
    const char* access_key,
    const char* model_path,
    int num_keywords,
    const char* const* keyword_paths,
    const float* sensitivities);

/**
 * 删除Porcupine实例
 * @param handle Porcupine实例句柄
 */
void porcupine_delete_wrapper(void* handle);

/**
 * 处理音频帧
 * @param handle Porcupine实例句柄
 * @param pcm 音频数据 (16位PCM)
 * @param keyword_index 输出参数：检测到的关键词索引 (-1表示未检测到)
 * @return 状态码
 */
int porcupine_process_wrapper(void* handle, const short* pcm, int* keyword_index);

/**
 * 获取Porcupine帧长度
 * @return 每帧采样数
 */
int porcupine_frame_length_wrapper(void);

/**
 * 获取Picovoice采样率
 * @return 采样率
 */
int porcupine_sample_rate_wrapper(void);

/**
 * 获取状态码的字符串表示
 * @param status 状态码
 * @return 状态码字符串
 */
const char* porcupine_status_to_string_wrapper(int status);

/**
 * 获取Porcupine版本
 * @return 版本字符串
 */
const char* porcupine_version_wrapper(void);

/**
 * 清理资源（可选调用）
 */
void porcupine_cleanup(void);

// 状态码常量定义
#define PORCUPINE_STATUS_SUCCESS 0
#define PORCUPINE_STATUS_OUT_OF_MEMORY 1
#define PORCUPINE_STATUS_IO_ERROR 2
#define PORCUPINE_STATUS_INVALID_ARGUMENT 3
#define PORCUPINE_STATUS_STOP_ITERATION 4
#define PORCUPINE_STATUS_KEY_ERROR 5
#define PORCUPINE_STATUS_INVALID_STATE 6
#define PORCUPINE_STATUS_RUNTIME_ERROR 7
#define PORCUPINE_STATUS_ACTIVATION_ERROR 8
#define PORCUPINE_STATUS_ACTIVATION_LIMIT_REACHED 9
#define PORCUPINE_STATUS_ACTIVATION_THROTTLED 10
#define PORCUPINE_STATUS_ACTIVATION_REFUSED 11

#ifdef __cplusplus
}
#endif