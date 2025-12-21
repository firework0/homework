// api.cpp - 添加编码转换功能
#include "ds.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <windows.h>
#include <vector>
#include <string>

#ifdef _WIN32
#pragma comment(lib, "libcurl.lib")
#endif

APIConfig g_api_config = { 0 };

// UTF-8 转 GBK 编码函数
std::string utf8_to_gbk(const char* utf8_str) {
    if (!utf8_str || utf8_str[0] == '\0') {
        return "";
    }

    int utf8_len = (int)strlen(utf8_str);

    // UTF-8 → UTF-16
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8_str, utf8_len, NULL, 0);
    if (wlen <= 0) return "";

    std::vector<wchar_t> wbuffer(wlen + 1);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str, utf8_len, wbuffer.data(), wlen);
    wbuffer[wlen] = L'\0';

    // UTF-16 → GBK
    int gbk_len = WideCharToMultiByte(CP_ACP, 0, wbuffer.data(), wlen, NULL, 0, NULL, NULL);
    if (gbk_len <= 0) return "";

    std::vector<char> gbuffer(gbk_len + 1);
    WideCharToMultiByte(CP_ACP, 0, wbuffer.data(), wlen, gbuffer.data(), gbk_len, NULL, NULL);
    gbuffer[gbk_len] = '\0';

    return std::string(gbuffer.data());
}

// C风格接口
char* utf8_to_gbk_c(const char* utf8_str) {
    std::string gbk_str = utf8_to_gbk(utf8_str);
    if (gbk_str.empty()) {
        return _strdup("");
    }
    return _strdup(gbk_str.c_str());
}

// 添加一个新的本地匹配函数避免递归
char* call_local_intent_matching(const char* user_input) {
    printf("[本地] 使用关键词匹配\n");

    if (strstr(user_input, "你好") || strstr(user_input, "您好")) {
        return _strdup("问候");
    }
    else if (strstr(user_input, "订单") || strstr(user_input, "查询")) {
        return _strdup("订单查询");
    }
    else if (strstr(user_input, "退款") || strstr(user_input, "退货")) {
        return _strdup("退款");
    }
    else if (strstr(user_input, "退出") || strstr(user_input, "再见")) {
        return _strdup("退出");
    }
    else if (strstr(user_input, "价格") || strstr(user_input, "多少钱")) {
        return _strdup("价格咨询");
    }

    return _strdup("未知");
}
// 从配置文件读取API密钥
bool init_api_from_config(const char* config_file) {
    FILE* file ;
    fopen_s(&file, config_file, "r");
    if (!file) {
        printf("警告: 配置文件不存在，使用本地意图识别\n");
        g_api_config.use_api = false;
        return false;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';

        if (strncmp(line, "api_key=", 8) == 0) {
            strcpy_s(g_api_config.api_key, line + 8);
            g_api_config.use_api = true;
            printf("API密钥已加载\n");
            break;
        }
    }

    fclose(file);
    return g_api_config.use_api;
}

// CURL回调函数
static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    char** response_ptr = (char**)userp;

    char* ptr = (char*)realloc(*response_ptr, realsize + 1);
    if (!ptr) return 0;

    *response_ptr = ptr;
    memcpy(*response_ptr, contents, realsize);
    (*response_ptr)[realsize] = '\0';

    return realsize;
}

// 解析API响应
char* parse_api_response(const char* response) {
    // 查找content字段
    const char* content_start = strstr(response, "\"content\":\"");
    if (!content_start) {
        return _strdup("未知");
    }

    content_start += 11; // 跳过 "\"content\":\""

    // 找到content结束的双引号
    const char* content_end = strchr(content_start, '"');
    if (!content_end) {
        return _strdup("未知");
    }

    // 计算内容长度
    size_t content_len = content_end - content_start;

    // 分配内存并复制UTF-8字符串
    char* utf8_content = (char*)malloc(content_len + 1);
    if (!utf8_content) {
        return _strdup("未知");
    }

    strncpy_s(utf8_content, content_len + 1, content_start, content_len);
    utf8_content[content_len] = '\0';

    // 将UTF-8转换为GBK
    char* gbk_content = utf8_to_gbk_c(utf8_content);
    free(utf8_content);

    if (!gbk_content || strlen(gbk_content) == 0) {
        if (gbk_content) free(gbk_content);
        return _strdup("未知");
    }

    return gbk_content;  // 注意：调用者需要释放这个内存
}


// JSON转义函数
void escape_for_json(const char* input, char* output, size_t output_size) {
    size_t j = 0;
    for (size_t i = 0; input[i] != '\0' && j < output_size - 1; i++) {
        switch (input[i]) {
        case '\\':
            if (j + 2 < output_size) { output[j++] = '\\'; output[j++] = '\\'; }
            break;
        case '"':
            if (j + 2 < output_size) { output[j++] = '\\'; output[j++] = '"'; }
            break;
        case '\n':
            if (j + 2 < output_size) { output[j++] = '\\'; output[j++] = 'n'; }
            break;
        case '\r':
            if (j + 2 < output_size) { output[j++] = '\\'; output[j++] = 'r'; }
            break;
        case '\t':
            if (j + 2 < output_size) { output[j++] = '\\'; output[j++] = 't'; }
            break;
        default:
            output[j++] = input[i];
        }
    }
    output[j] = '\0';
}

// 使用curl命令调用API（返回GBK编码的意图）
char* call_qwen_via_system(const char* user_input, char intents[][50], int intent_count) {
    printf("[SYSTEM] 使用curl命令调用API\n");

    // 构建系统提示词
    char prompt[1024] = "分析用户意图，只返回以下之一：";
    for (int i = 0; i < intent_count; i++) {
        if (intents[i] && strlen(intents[i]) > 0) {
            // 清理意图字符串
            char clean[50];
            strcpy_s(clean, sizeof(clean), intents[i]);

            // 移除换行符
            char* nl = strchr(clean, '\n');
            if (nl) *nl = '\0';

            if (strlen(clean) > 0) {
                strcat_s(prompt, sizeof(prompt), clean);
                if (i < intent_count - 1) strcat_s(prompt, sizeof(prompt), "、");
            }
        }
    }
    strcat_s(prompt, sizeof(prompt), "。不要其他内容。");

    printf("[SYSTEM] 提示词: %s\n", prompt);
    printf("[SYSTEM] 用户输入: %s\n", user_input);

    // 转义JSON字符串（简单的转义处理）
    char escaped_prompt[2048];
    char escaped_input[512];

    // 简单的转义：只处理双引号和反斜杠
    char* p1 = escaped_prompt;
    for (int i = 0; prompt[i] != '\0' && p1 < escaped_prompt + sizeof(escaped_prompt) - 2; i++) {
        if (prompt[i] == '"' || prompt[i] == '\\') {
            *p1++ = '\\';
        }
        *p1++ = prompt[i];
    }
    *p1 = '\0';

    char* p2 = escaped_input;
    for (int i = 0; user_input[i] != '\0' && p2 < escaped_input + sizeof(escaped_input) - 2; i++) {
        if (user_input[i] == '"' || user_input[i] == '\\') {
            *p2++ = '\\';
        }
        *p2++ = user_input[i];
    }
    *p2 = '\0';

    // 构建curl命令
    char command[4096];
    snprintf(command, sizeof(command),
        "curl -s -X POST https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions "
        "-H \"Authorization: Bearer %s\" "
        "-H \"Content-Type: application/json\" "
        "-d \"{\\\"model\\\":\\\"qwen-turbo\\\",\\\"messages\\\":["
        "{\\\"role\\\":\\\"system\\\",\\\"content\\\":\\\"%s\\\"},"
        "{\\\"role\\\":\\\"user\\\",\\\"content\\\":\\\"%s\\\"}"
        "]}\"",
        g_api_config.api_key, escaped_prompt, escaped_input);

    // 执行命令 - 二进制模式读取
    FILE* pipe = _popen(command, "rb");
    if (!pipe) {
        printf("[ERROR] 执行命令失败\n");
        return _strdup("未知");
    }

    // 读取响应
    std::vector<unsigned char> buffer;
    unsigned char chunk[1024];
    size_t bytes_read;

    while ((bytes_read = fread(chunk, 1, sizeof(chunk), pipe)) > 0) {
        buffer.insert(buffer.end(), chunk, chunk + bytes_read);
    }
    _pclose(pipe);

    // 添加null终止符
    buffer.push_back('\0');

    // 将响应视为UTF-8字符串
    const char* utf8_response = (const char*)buffer.data();

    // 解析响应并提取意图（会自动转换为GBK）
    char* intent = parse_api_response(utf8_response);
    printf("[SYSTEM] 识别意图: %s\n", intent);

    return intent;
}

// 极简API调用函数
// 主API调用函数
char* call_qwen_api_simple(const char* user_input, char intents[][50], int intent_count) {
    // 如果没有API密钥，使用本地匹配
    if (!g_api_config.use_api || strlen(g_api_config.api_key) < 5) {
        printf("[本地] 使用关键词匹配\n");
        return call_local_intent_matching(user_input);
    }

    // 使用curl命令行调用API（确保编码正确）
    return call_qwen_via_system(user_input, intents, intent_count);
}