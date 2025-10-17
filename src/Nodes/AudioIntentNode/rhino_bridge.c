//
// Created by WuBin on 2025/10/5.
//
// rhino_bridge.c
// 注意：这个文件必须保存为 .c 文件，并用 C 编译器编译！

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef RHINO_DYNAMIC_LOAD
#include <windows.h>

// 动态库句柄
static HMODULE rhino_dll = NULL;

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

// 前向声明Rhino结构体
typedef struct pv_rhino pv_rhino_t;

// 函数指针类型定义
typedef int32_t (*pv_sample_rate_func)(void);
typedef const char* (*pv_status_to_string_func)(pv_status_t status);
typedef pv_status_t (*pv_get_error_stack_func)(char ***message_stack, int32_t *message_stack_depth);
typedef void (*pv_free_error_stack_func)(char **message_stack);
typedef pv_status_t (*pv_rhino_init_func)(
    const char *access_key,
    const char *model_path,
    const char *context_path,
    float sensitivity,
    float endpoint_duration_sec,
    bool require_endpoint,
    pv_rhino_t **object);
typedef void (*pv_rhino_delete_func)(pv_rhino_t *object);
typedef pv_status_t (*pv_rhino_process_func)(
    pv_rhino_t *object,
    const int16_t *pcm,
    bool *is_finalized);
typedef pv_status_t (*pv_rhino_is_understood_func)(
    const pv_rhino_t *object,
    bool *is_understood);
typedef pv_status_t (*pv_rhino_get_intent_func)(
    const pv_rhino_t *object,
    const char **intent,
    int32_t *num_slots,
    const char ***slots,
    const char ***values);
typedef pv_status_t (*pv_rhino_free_slots_and_values_func)(
    const pv_rhino_t *object,
    const char **slots,
    const char **values);
typedef pv_status_t (*pv_rhino_reset_func)(pv_rhino_t *object);
typedef pv_status_t (*pv_rhino_context_info_func)(
    const pv_rhino_t *object,
    const char **context_info);
typedef const char* (*pv_rhino_version_func)(void);
typedef int32_t (*pv_rhino_frame_length_func)(void);

// 函数指针变量
static pv_sample_rate_func pv_sample_rate_ptr = NULL;
static pv_status_to_string_func pv_status_to_string_ptr = NULL;
static pv_get_error_stack_func pv_get_error_stack_ptr = NULL;
static pv_free_error_stack_func pv_free_error_stack_ptr = NULL;
static pv_rhino_init_func pv_rhino_init_ptr = NULL;
static pv_rhino_delete_func pv_rhino_delete_ptr = NULL;
static pv_rhino_process_func pv_rhino_process_ptr = NULL;
static pv_rhino_is_understood_func pv_rhino_is_understood_ptr = NULL;
static pv_rhino_get_intent_func pv_rhino_get_intent_ptr = NULL;
static pv_rhino_free_slots_and_values_func pv_rhino_free_slots_and_values_ptr = NULL;
static pv_rhino_reset_func pv_rhino_reset_ptr = NULL;
static pv_rhino_context_info_func pv_rhino_context_info_ptr = NULL;
static pv_rhino_version_func pv_rhino_version_ptr = NULL;
static pv_rhino_frame_length_func pv_rhino_frame_length_ptr = NULL;

/**
 * 初始化动态库
 * @return 1表示成功，0表示失败
 */
static int init_rhino_dll(void) {
    if (rhino_dll != NULL) {
        return 1; // 已经初始化
    }
    
    // 加载动态库
    rhino_dll = LoadLibraryA(RHINO_DLL_PATH);
    if (rhino_dll == NULL) {
        printf("Failed to load Rhino DLL: %s\n", RHINO_DLL_PATH);
        return 0;
    }
    
    // 获取函数地址
    pv_sample_rate_ptr = (pv_sample_rate_func)GetProcAddress(rhino_dll, "pv_sample_rate");
    pv_status_to_string_ptr = (pv_status_to_string_func)GetProcAddress(rhino_dll, "pv_status_to_string");
    pv_get_error_stack_ptr = (pv_get_error_stack_func)GetProcAddress(rhino_dll, "pv_get_error_stack");
    pv_free_error_stack_ptr = (pv_free_error_stack_func)GetProcAddress(rhino_dll, "pv_free_error_stack");
    pv_rhino_init_ptr = (pv_rhino_init_func)GetProcAddress(rhino_dll, "pv_rhino_init");
    pv_rhino_delete_ptr = (pv_rhino_delete_func)GetProcAddress(rhino_dll, "pv_rhino_delete");
    pv_rhino_process_ptr = (pv_rhino_process_func)GetProcAddress(rhino_dll, "pv_rhino_process");
    pv_rhino_is_understood_ptr = (pv_rhino_is_understood_func)GetProcAddress(rhino_dll, "pv_rhino_is_understood");
    pv_rhino_get_intent_ptr = (pv_rhino_get_intent_func)GetProcAddress(rhino_dll, "pv_rhino_get_intent");
    pv_rhino_free_slots_and_values_ptr = (pv_rhino_free_slots_and_values_func)GetProcAddress(rhino_dll, "pv_rhino_free_slots_and_values");
    pv_rhino_reset_ptr = (pv_rhino_reset_func)GetProcAddress(rhino_dll, "pv_rhino_reset");
    pv_rhino_context_info_ptr = (pv_rhino_context_info_func)GetProcAddress(rhino_dll, "pv_rhino_context_info");
    pv_rhino_version_ptr = (pv_rhino_version_func)GetProcAddress(rhino_dll, "pv_rhino_version");
    pv_rhino_frame_length_ptr = (pv_rhino_frame_length_func)GetProcAddress(rhino_dll, "pv_rhino_frame_length");
    
    // 检查关键函数是否加载成功
    if (!pv_sample_rate_ptr || !pv_status_to_string_ptr || !pv_rhino_init_ptr || 
        !pv_rhino_delete_ptr || !pv_rhino_process_ptr) {
        printf("Failed to get function addresses from Rhino DLL\n");
        FreeLibrary(rhino_dll);
        rhino_dll = NULL;
        return 0;
    }
    
    return 1;
}

/**
 * 清理动态库
 */
static void cleanup_rhino_dll(void) {
    if (rhino_dll != NULL) {
        FreeLibrary(rhino_dll);
        rhino_dll = NULL;
        // 重置所有函数指针
        pv_sample_rate_ptr = NULL;
        pv_status_to_string_ptr = NULL;
        pv_get_error_stack_ptr = NULL;
        pv_free_error_stack_ptr = NULL;
        pv_rhino_init_ptr = NULL;
        pv_rhino_delete_ptr = NULL;
        pv_rhino_process_ptr = NULL;
        pv_rhino_is_understood_ptr = NULL;
        pv_rhino_get_intent_ptr = NULL;
        pv_rhino_free_slots_and_values_ptr = NULL;
        pv_rhino_reset_ptr = NULL;
        pv_rhino_context_info_ptr = NULL;
        pv_rhino_version_ptr = NULL;
        pv_rhino_frame_length_ptr = NULL;
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

typedef struct pv_rhino pv_rhino_t;

extern int32_t pv_sample_rate(void);
extern const char *pv_status_to_string(pv_status_t status);
extern pv_status_t pv_get_error_stack(char ***message_stack, int32_t *message_stack_depth);
extern void pv_free_error_stack(char **message_stack);
extern pv_status_t pv_rhino_init(
    const char *access_key,
    const char *model_path,
    const char *context_path,
    float sensitivity,
    float endpoint_duration_sec,
    bool require_endpoint,
    pv_rhino_t **object);
extern void pv_rhino_delete(pv_rhino_t *object);
extern pv_status_t pv_rhino_process(
    pv_rhino_t *object,
    const int16_t *pcm,
    bool *is_finalized);
extern pv_status_t pv_rhino_is_understood(
    const pv_rhino_t *object,
    bool *is_understood);
extern pv_status_t pv_rhino_get_intent(
    const pv_rhino_t *object,
    const char **intent,
    int32_t *num_slots,
    const char ***slots,
    const char ***values);
extern pv_status_t pv_rhino_free_slots_and_values(
    const pv_rhino_t *object,
    const char **slots,
    const char **values);
extern pv_status_t pv_rhino_reset(pv_rhino_t *object);
extern pv_status_t pv_rhino_context_info(
    const pv_rhino_t *object,
    const char **context_info);
extern const char *pv_rhino_version(void);
extern int32_t pv_rhino_frame_length(void);
#endif

// 定义桥接结构体
typedef struct {
    pv_status_t status;
    void* handle;
    bool is_understood;
    const char* intent;
} rhino_result_t;

/**
 * 桥接函数：初始化 Rhino
 * @param access_key Picovoice访问密钥
 * @param model_path 模型文件路径
 * @param context_path 上下文文件路径
 * @param sensitivity 敏感度 (0.0-1.0)
 * @param endpoint_duration_sec 端点持续时间（秒）
 * @param require_endpoint 是否需要端点
 * @return 初始化结果
 */
rhino_result_t rhino_init_wrapper(
    const char* access_key,
    const char* model_path,
    const char* context_path,
    float sensitivity,
    float endpoint_duration_sec,
    int require_endpoint) {
    
    rhino_result_t result = {0};
    
#ifdef RHINO_DYNAMIC_LOAD
    if (!init_rhino_dll()) {
        result.status = PV_STATUS_RUNTIME_ERROR;
        return result;
    }
    
    pv_status_t status = pv_rhino_init_ptr(
        access_key,
        model_path,
        context_path,
        sensitivity,
        endpoint_duration_sec,
        (bool)require_endpoint,
        (pv_rhino_t**)&result.handle);
#else
    pv_status_t status = pv_rhino_init(
        access_key,
        model_path,
        context_path,
        sensitivity,
        endpoint_duration_sec,
        (bool)require_endpoint,
        (pv_rhino_t**)&result.handle);
#endif
    
    result.status = status;
    return result;
}

/**
 * 桥接函数：删除 Rhino 实例
 * @param handle Rhino实例句柄
 */
void rhino_delete_wrapper(void* handle) {
    if (handle) {
#ifdef RHINO_DYNAMIC_LOAD
        if (pv_rhino_delete_ptr) {
            pv_rhino_delete_ptr((pv_rhino_t*)handle);
        }
#else
        pv_rhino_delete((pv_rhino_t*)handle);
#endif
    }
}

/**
 * 桥接函数：处理音频数据
 * @param handle Rhino实例句柄
 * @param pcm 音频数据
 * @param is_finalized 是否完成处理
 * @return 状态码
 */
int rhino_process_wrapper(void* handle, const short* pcm, int* is_finalized) {
    if (!handle || !pcm || !is_finalized) {
        return PV_STATUS_INVALID_ARGUMENT;
    }
    
    bool finalized = false;
#ifdef RHINO_DYNAMIC_LOAD
    if (!pv_rhino_process_ptr) {
        return PV_STATUS_RUNTIME_ERROR;
    }
    pv_status_t status = pv_rhino_process_ptr((pv_rhino_t*)handle, (const int16_t*)pcm, &finalized);
#else
    pv_status_t status = pv_rhino_process((pv_rhino_t*)handle, (const int16_t*)pcm, &finalized);
#endif
    
    *is_finalized = finalized ? 1 : 0;
    return (int)status;
}

/**
 * 桥接函数：检查是否理解了意图
 * @param handle Rhino实例句柄
 * @param is_understood 是否理解
 * @return 状态码
 */
int rhino_is_understood_wrapper(void* handle, int* is_understood) {
    if (!handle || !is_understood) {
        return PV_STATUS_INVALID_ARGUMENT;
    }
    
    bool understood = false;
#ifdef RHINO_DYNAMIC_LOAD
    if (!pv_rhino_is_understood_ptr) {
        return PV_STATUS_RUNTIME_ERROR;
    }
    pv_status_t status = pv_rhino_is_understood_ptr((pv_rhino_t*)handle, &understood);
#else
    pv_status_t status = pv_rhino_is_understood((pv_rhino_t*)handle, &understood);
#endif
    
    *is_understood = understood ? 1 : 0;
    return (int)status;
}

/**
 * 桥接函数：获取意图信息
 * @param handle Rhino实例句柄
 * @param intent 意图名称
 * @param num_slots 槽位数量
 * @param slots 槽位名称数组
 * @param values 槽位值数组
 * @return 状态码
 */
int rhino_get_intent_wrapper(
    void* handle,
    const char** intent,
    int* num_slots,
    const char*** slots,
    const char*** values) {
    
    if (!handle) {
        return PV_STATUS_INVALID_ARGUMENT;
    }
    
    int32_t slot_count = 0;
#ifdef RHINO_DYNAMIC_LOAD
    if (!pv_rhino_get_intent_ptr) {
        return PV_STATUS_RUNTIME_ERROR;
    }
    pv_status_t status = pv_rhino_get_intent_ptr(
        (pv_rhino_t*)handle,
        intent,
        &slot_count,
        slots,
        values);
#else
    pv_status_t status = pv_rhino_get_intent(
        (pv_rhino_t*)handle,
        intent,
        &slot_count,
        slots,
        values);
#endif
    
    if (num_slots) {
        *num_slots = (int)slot_count;
    }
    
    return (int)status;
}

/**
 * 桥接函数：释放槽位和值
 * @param handle Rhino实例句柄
 * @param slots 槽位名称数组
 * @param values 槽位值数组
 * @return 状态码
 */
int rhino_free_slots_and_values_wrapper(
    void* handle,
    const char** slots,
    const char** values) {
    
    if (!handle) {
        return PV_STATUS_INVALID_ARGUMENT;
    }
    
#ifdef RHINO_DYNAMIC_LOAD
    if (!pv_rhino_free_slots_and_values_ptr) {
        return PV_STATUS_RUNTIME_ERROR;
    }
    pv_status_t status = pv_rhino_free_slots_and_values_ptr(
        (pv_rhino_t*)handle,
        slots,
        values);
#else
    pv_status_t status = pv_rhino_free_slots_and_values(
        (pv_rhino_t*)handle,
        slots,
        values);
#endif
    
    return (int)status;
}

/**
 * 桥接函数：重置Rhino状态
 * @param handle Rhino实例句柄
 * @return 状态码
 */
int rhino_reset_wrapper(void* handle) {
    if (!handle) {
        return PV_STATUS_INVALID_ARGUMENT;
    }
    
#ifdef RHINO_DYNAMIC_LOAD
    if (!pv_rhino_reset_ptr) {
        return PV_STATUS_RUNTIME_ERROR;
    }
    pv_status_t status = pv_rhino_reset_ptr((pv_rhino_t*)handle);
#else
    pv_status_t status = pv_rhino_reset((pv_rhino_t*)handle);
#endif
    
    return (int)status;
}

/**
 * 获取Rhino帧长度
 * @return 帧长度
 */
int rhino_frame_length_wrapper(void) {
#ifdef RHINO_DYNAMIC_LOAD
    if (!init_rhino_dll() || !pv_rhino_frame_length_ptr) {
        return -1;
    }
    return (int)pv_rhino_frame_length_ptr();
#else
    return (int)pv_rhino_frame_length();
#endif
}

/**
 * 获取采样率
 * @return 采样率
 */
int rhino_sample_rate_wrapper(void) {
#ifdef RHINO_DYNAMIC_LOAD
    if (!init_rhino_dll() || !pv_sample_rate_ptr) {
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
const char* rhino_status_to_string_wrapper(int status) {
#ifdef RHINO_DYNAMIC_LOAD
    if (!init_rhino_dll() || !pv_status_to_string_ptr) {
        return "Unknown status";
    }
    return pv_status_to_string_ptr((pv_status_t)status);
#else
    return pv_status_to_string((pv_status_t)status);
#endif
}

/**
 * 获取Rhino版本
 * @return 版本字符串
 */
const char* rhino_version_wrapper(void) {
#ifdef RHINO_DYNAMIC_LOAD
    if (!init_rhino_dll() || !pv_rhino_version_ptr) {
        return "Unknown version";
    }
    return pv_rhino_version_ptr();
#else
    return pv_rhino_version();
#endif
}

/**
 * 获取Rhino上下文信息
 * @param handle Rhino实例句柄
 * @param context_info 输出参数：上下文信息字符串
 * @return 状态码
 */
int rhino_context_info_wrapper(void* handle, const char** context_info) {
    if (!handle || !context_info) {
        return PV_STATUS_INVALID_ARGUMENT;
    }
    
#ifdef RHINO_DYNAMIC_LOAD
    if (!pv_rhino_context_info_ptr) {
        return PV_STATUS_RUNTIME_ERROR;
    }
    pv_status_t status = pv_rhino_context_info_ptr((pv_rhino_t*)handle, context_info);
#else
    pv_status_t status = pv_rhino_context_info((pv_rhino_t*)handle, context_info);
#endif
    
    return (int)status;
}

/**
 * 清理资源（可选调用）
 */
void rhino_cleanup(void) {
#ifdef RHINO_DYNAMIC_LOAD
    cleanup_rhino_dll();
#endif
}