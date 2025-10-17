//
// Created by WuBin on 2025/10/5.
//
// porcupine_bridge.c
// 注意：这个文件必须保存为 .c 文件，并用 C 编译器编译！

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef PORCUPINE_DYNAMIC_LOAD
#include <windows.h>

// 动态库句柄
static HMODULE porcupine_dll = NULL;

// 手动定义Picovoice类型
typedef enum {
    PV_STATUS_SUCCESS = 0,
    PV_STATUS_OUT_OF_MEMORY,
    PV_STATUS_IO_ERROR,
    PV_STATUS_INVALID_ARGUMENT,
    PV_STATUS_STOP_ITERATION,
    PV_STATUS_KEY_ERROR,
    PV_STATUS_INVALID_STATE,
    PV_STATUS_RUNTIME_ERROR,
    PV_STATUS_ACTIVATION_ERROR,
    PV_STATUS_ACTIVATION_LIMIT_REACHED,
    PV_STATUS_ACTIVATION_THROTTLED,
    PV_STATUS_ACTIVATION_REFUSED
} pv_status_t;

// 前向声明Porcupine结构体
typedef struct pv_porcupine pv_porcupine_t;

// 函数指针类型定义
typedef int32_t (*pv_sample_rate_func)(void);
typedef const char* (*pv_status_to_string_func)(pv_status_t status);
typedef pv_status_t (*pv_get_error_stack_func)(char ***message_stack, int32_t *message_stack_depth);
typedef void (*pv_free_error_stack_func)(char **message_stack);
typedef pv_status_t (*pv_porcupine_init_func)(
    const char *access_key,
    const char *model_path,
    int32_t num_keywords,
    const char* const *keyword_paths,
    const float *sensitivities,
    pv_porcupine_t **object);
typedef void (*pv_porcupine_delete_func)(pv_porcupine_t *object);
typedef pv_status_t (*pv_porcupine_process_func)(
    pv_porcupine_t *object,
    const int16_t *pcm,
    int32_t *keyword_index);
typedef const char* (*pv_porcupine_version_func)(void);
typedef int32_t (*pv_porcupine_frame_length_func)(void);

// 函数指针变量
static pv_sample_rate_func pv_sample_rate_ptr = NULL;
static pv_status_to_string_func pv_status_to_string_ptr = NULL;
static pv_get_error_stack_func pv_get_error_stack_ptr = NULL;
static pv_free_error_stack_func pv_free_error_stack_ptr = NULL;
static pv_porcupine_init_func pv_porcupine_init_ptr = NULL;
static pv_porcupine_delete_func pv_porcupine_delete_ptr = NULL;
static pv_porcupine_process_func pv_porcupine_process_ptr = NULL;
static pv_porcupine_version_func pv_porcupine_version_ptr = NULL;
static pv_porcupine_frame_length_func pv_porcupine_frame_length_ptr = NULL;

/**
 * 初始化Porcupine动态库
 * @return 成功返回1，失败返回0
 */
static int init_porcupine_dll(void) {
    if (porcupine_dll != NULL) {
        return 1; // 已经初始化
    }
    
    // 加载动态库 - 使用宏定义的路径，与rhino_bridge保持一致
    porcupine_dll = LoadLibraryA(PORCUPINE_DLL_PATH);
    if (porcupine_dll == NULL) {
        printf("Failed to load Porcupine DLL: %s\n", PORCUPINE_DLL_PATH);
        return 0;
    }
    
    // 获取函数地址
    pv_sample_rate_ptr = (pv_sample_rate_func)GetProcAddress(porcupine_dll, "pv_sample_rate");
    pv_status_to_string_ptr = (pv_status_to_string_func)GetProcAddress(porcupine_dll, "pv_status_to_string");
    pv_get_error_stack_ptr = (pv_get_error_stack_func)GetProcAddress(porcupine_dll, "pv_get_error_stack");
    pv_free_error_stack_ptr = (pv_free_error_stack_func)GetProcAddress(porcupine_dll, "pv_free_error_stack");
    pv_porcupine_init_ptr = (pv_porcupine_init_func)GetProcAddress(porcupine_dll, "pv_porcupine_init");
    pv_porcupine_delete_ptr = (pv_porcupine_delete_func)GetProcAddress(porcupine_dll, "pv_porcupine_delete");
    pv_porcupine_process_ptr = (pv_porcupine_process_func)GetProcAddress(porcupine_dll, "pv_porcupine_process");
    pv_porcupine_version_ptr = (pv_porcupine_version_func)GetProcAddress(porcupine_dll, "pv_porcupine_version");
    pv_porcupine_frame_length_ptr = (pv_porcupine_frame_length_func)GetProcAddress(porcupine_dll, "pv_porcupine_frame_length");
    
    // 检查关键函数是否加载成功
    if (!pv_sample_rate_ptr || !pv_status_to_string_ptr || !pv_porcupine_init_ptr || 
        !pv_porcupine_delete_ptr || !pv_porcupine_process_ptr) {
        printf("Failed to get function addresses from Porcupine DLL\n");
        FreeLibrary(porcupine_dll);
        porcupine_dll = NULL;
        return 0;
    }
    
    return 1;
}

/**
 * 清理Porcupine动态库
 */
static void cleanup_porcupine_dll(void) {
    if (porcupine_dll != NULL) {
        FreeLibrary(porcupine_dll);
        porcupine_dll = NULL;
        
        // 重置函数指针
        pv_sample_rate_ptr = NULL;
        pv_status_to_string_ptr = NULL;
        pv_get_error_stack_ptr = NULL;
        pv_free_error_stack_ptr = NULL;
        pv_porcupine_init_ptr = NULL;
        pv_porcupine_delete_ptr = NULL;
        pv_porcupine_process_ptr = NULL;
        pv_porcupine_version_ptr = NULL;
        pv_porcupine_frame_length_ptr = NULL;
    }
}

#else
// 静态链接版本的类型和extern声明
typedef enum {
    PV_STATUS_SUCCESS = 0,
    PV_STATUS_OUT_OF_MEMORY,
    PV_STATUS_IO_ERROR,
    PV_STATUS_INVALID_ARGUMENT,
    PV_STATUS_STOP_ITERATION,
    PV_STATUS_KEY_ERROR,
    PV_STATUS_INVALID_STATE,
    PV_STATUS_RUNTIME_ERROR,
    PV_STATUS_ACTIVATION_ERROR,
    PV_STATUS_ACTIVATION_LIMIT_REACHED,
    PV_STATUS_ACTIVATION_THROTTLED,
    PV_STATUS_ACTIVATION_REFUSED
} pv_status_t;

typedef struct pv_porcupine pv_porcupine_t;

extern int32_t pv_sample_rate(void);
extern const char *pv_status_to_string(pv_status_t status);
extern pv_status_t pv_get_error_stack(char ***message_stack, int32_t *message_stack_depth);
extern void pv_free_error_stack(char **message_stack);
extern pv_status_t pv_porcupine_init(
    const char *access_key,
    const char *model_path,
    int32_t num_keywords,
    const char* const *keyword_paths,
    const float *sensitivities,
    pv_porcupine_t **object);
extern void pv_porcupine_delete(pv_porcupine_t *object);
extern pv_status_t pv_porcupine_process(
    pv_porcupine_t *object,
    const int16_t *pcm,
    int32_t *keyword_index);
extern const char *pv_porcupine_version(void);
extern int32_t pv_porcupine_frame_length(void);
#endif

// 定义桥接结构体
typedef struct {
    pv_status_t status;
    void* handle;
    int keyword_index;
} porcupine_result_t;

/**
 * 桥接函数：初始化 Porcupine
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
    const float* sensitivities) {
    
    porcupine_result_t result = {0};
    result.keyword_index = -1;
    
#ifdef PORCUPINE_DYNAMIC_LOAD
    if (!init_porcupine_dll()) {
        result.status = PV_STATUS_RUNTIME_ERROR;
        return result;
    }
    
    pv_status_t status = pv_porcupine_init_ptr(
        access_key,
        model_path,
        (int32_t)num_keywords,
        keyword_paths,
        sensitivities,
        (pv_porcupine_t**)&result.handle);
#else
    pv_status_t status = pv_porcupine_init(
        access_key,
        model_path,
        (int32_t)num_keywords,
        keyword_paths,
        sensitivities,
        (pv_porcupine_t**)&result.handle);
#endif
    
    result.status = status;
    return result;
}

/**
 * 桥接函数：删除 Porcupine 实例
 * @param handle Porcupine实例句柄
 */
void porcupine_delete_wrapper(void* handle) {
    if (handle) {
#ifdef PORCUPINE_DYNAMIC_LOAD
        if (pv_porcupine_delete_ptr) {
            pv_porcupine_delete_ptr((pv_porcupine_t*)handle);
        }
#else
        pv_porcupine_delete((pv_porcupine_t*)handle);
#endif
    }
}

/**
 * 桥接函数：处理音频数据
 * @param handle Porcupine实例句柄
 * @param pcm 音频数据
 * @param keyword_index 检测到的关键词索引
 * @return 状态码
 */
int porcupine_process_wrapper(void* handle, const short* pcm, int* keyword_index) {
    if (!handle || !pcm || !keyword_index) {
        return PV_STATUS_INVALID_ARGUMENT;
    }
    
    int32_t detected_index = -1;
#ifdef PORCUPINE_DYNAMIC_LOAD
    if (!pv_porcupine_process_ptr) {
        return PV_STATUS_RUNTIME_ERROR;
    }
    pv_status_t status = pv_porcupine_process_ptr((pv_porcupine_t*)handle, (const int16_t*)pcm, &detected_index);
#else
    pv_status_t status = pv_porcupine_process((pv_porcupine_t*)handle, (const int16_t*)pcm, &detected_index);
#endif
    
    *keyword_index = (int)detected_index;
    return (int)status;
}

/**
 * 获取Porcupine帧长度
 * @return 帧长度
 */
int porcupine_frame_length_wrapper(void) {
#ifdef PORCUPINE_DYNAMIC_LOAD
    if (!init_porcupine_dll() || !pv_porcupine_frame_length_ptr) {
        return -1;
    }
    return (int)pv_porcupine_frame_length_ptr();
#else
    return (int)pv_porcupine_frame_length();
#endif
}

/**
 * 获取采样率
 * @return 采样率
 */
int porcupine_sample_rate_wrapper(void) {
#ifdef PORCUPINE_DYNAMIC_LOAD
    if (!init_porcupine_dll() || !pv_sample_rate_ptr) {
        return -1; // 初始化失败
    }
    return (int)pv_sample_rate_ptr();
#else
    return (int)pv_sample_rate();
#endif
}

/**
 * 状态码转字符串
 * @param status 状态码
 * @return 状态描述字符串
 */
const char* porcupine_status_to_string_wrapper(int status) {
#ifdef PORCUPINE_DYNAMIC_LOAD
    if (!init_porcupine_dll() || !pv_status_to_string_ptr) {
        return "Unknown status";
    }
    return pv_status_to_string_ptr((pv_status_t)status);
#else
    return pv_status_to_string((pv_status_t)status);
#endif
}

/**
 * 获取Porcupine版本
 * @return 版本字符串
 */
const char* porcupine_version_wrapper(void) {
#ifdef PORCUPINE_DYNAMIC_LOAD
    if (!init_porcupine_dll() || !pv_porcupine_version_ptr) {
        return "Unknown version";
    }
    return pv_porcupine_version_ptr();
#else
    return pv_porcupine_version();
#endif
}

/**
 * 清理资源（可选调用）
 */
void porcupine_cleanup(void) {
#ifdef PORCUPINE_DYNAMIC_LOAD
    cleanup_porcupine_dll();
#endif
}