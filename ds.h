#pragma once
#include <stdio.h>
#include <stdbool.h>

// API配置结构（最简单版）
typedef struct {
	char api_key[128];
	bool use_api;  // 是否使用真实API
} APIConfig;

// 全局API配置
extern APIConfig g_api_config;

// API相关函数
bool init_api_from_config(const char* config_file);
char* call_qwen_api_simple(const char* user_input, char intents[][50], int intent_count);