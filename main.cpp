#include <stdio.h>
#include "ds.h"
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <windows.h>

char scene_names[20][20];

typedef char Condition[50];//意图，以字符串形式储存

typedef char Keyword[10];//关键字，以字符串形式储存

//规则定义，包含规则的id，名称，这个规则对哪个意图产生反应，回复内容，以及该规则如果有跳转的话跳转到哪个规则(一样用id标识)
//规则以数组的形式储存，用数组下标作为标识
//通过意图的比对来确定规则，意图都是汉字，如果是null则表示该规则无条件输出内容
typedef struct rule {
	int idex;
	char rule_name[20];
	Condition condition;
	char reply[90];
	int jump_to_next_rule;
}Rule;

//规则数组
Rule rules[20];

//当前规则，用下标标识，为-1表示结束，用户需求处理完了或者未匹配到规则（不过应该不会匹配不到吧，应该）
int current_rule;

//意图，从脚本中读取,在初始化函数中会将第一个元素初始化为"null"
char conditions[50][50];

//关键字,规则以rule开头，后跟规则名，end_rule表示规则的结束,if后跟意图,下一行跟then＋行动（回复或跳转）。
//condition后跟意图，如果没有定义意图则输出No define of conditions
Keyword keywords[] = { "rule","if","rhen","jumpto","end_rule","condition" };

// 添加测试函数
void test_api_directly() {
	printf("\n=== API直接测试 ===\n");

	if (!g_api_config.use_api) {
		printf("API未启用\n");
		return;
	}

	// 创建一个简单的意图数组
	char test_intents[50][50] = { "问候", "订单查询", "退款", "退出" };
	int intent_count = 4;

	// 测试各种输入
	const char* test_inputs[] = { "你好", "订单", "退款", "退出", "价格" };
	int test_count = 5;

	for (int i = 0; i < test_count; i++) {
		printf("\n测试 %d: 输入='%s'\n", i + 1, test_inputs[i]);
		char* intent = call_qwen_api_simple(test_inputs[i], test_intents, intent_count);
		printf("识别结果: %s\n", intent);
		free(intent);
	}

	printf("\n=== API测试结束 ===\n");
}

// 在你的main.cpp中添加这个函数
char** extract_intents_from_dsl(const char* filename, int* count) {
	static char intents[20][50];  // 最多20个意图，每个最多50字符
	*count = 0;

	FILE* file ;
	fopen_s(&file,filename, "r");
	if (!file) return NULL;

	char line[256];
	while (fgets(line, sizeof(line), file)) {
		// 找condition行
		if (strncmp(line, "condition ", 10) == 0) {
			char* token = line + 10;
			char* end = strchr(token, '\n');
			if (end) *end = '\0';

			// 分割意图
			char* ptr = NULL;
			token = strtok_s(token, " ",&ptr);
			while (token && *count < 20) {
				strcpy_s(intents[*count], token);
				(*count)++;
				token = strtok_s(NULL, " ",&ptr);
			}
			break;
		}
	}

	fclose(file);
	return (char**)intents;
}

int main()
{
	// 初始化API（从api_config.txt读取密钥）
	init_api_from_config("api_config.txt");

	//打开配置文件
	FILE* pconfig;
	fopen_s(&pconfig, "config.txt", "r");
	if (!pconfig)
	{
		printf("Can't not open config\n");
		return 0;
	}
	/*else
	{
		printf("right\n");//调试信息，到时候注释掉
	}*/

	//读取配置
	char config[10];
	fgets(config, sizeof(config), pconfig);

	//a是辅助的
	char a[7];
	for (int i = 0; i < 6; i++)
	{
		a[i] = config[i];
	}
	a[6] = '\0';
	if (strcmp(a, "scene="))
	{
		printf("Config Error\n");
		return 0;
	}

	//通过配置文件打开对应的测试文件
	char file_name_of_test[] = "test0.txt";
	file_name_of_test[4] = config[6];
	//printf("%s\n", file_name_of_test);

	//打开file_name_of_test作为文件名的文件
	FILE* p_test_file;
	fopen_s(&p_test_file, file_name_of_test, "r");
	if (!p_test_file)
	{
		printf("can't open test_file\n");
		return 0;
	}
	/*else
	{
		printf("right\n");
	}
	*/
	char text_in_test[90];//用来读取脚本文件
	char b[20];//辅助的
	int index = 0;//用来标记已读入那行的文件内容处理到了哪里

	//读取第一行
	fgets(text_in_test, sizeof(text_in_test), p_test_file);
	//printf("%s", text_in_test);

	for (int i = 0; i < 10; i++)
	{
		b[i] = text_in_test[i];
	}
	b[10] = '\0';
	//如果是condition开头，读取意图
	int index_of_conditions = 0;
	if (!strcmp(b, "condition "))
	{
		
		int index_of_condition = 0;
		for (int i=10;; i++)
		{
			if (text_in_test[i] != ' ')
			{
				conditions[index_of_conditions][index_of_condition] = text_in_test[i];
				index_of_condition++;
			}
			else if (text_in_test[i] == ' ')
			{
				index_of_conditions++;
				index_of_condition = 0;
			}
			if (text_in_test[i] == '\n')
			{
				break;
			}
		}
	}
	else
	{
		printf("condition error");
	}

	/*printf("调试信息，解析出来的condtion：");
	for (int i = 0; i <= index_of_conditions; i++)
	{
		printf("%s ", conditions[i]);
	}
	printf("\n");
	*/

	//开始读取规则
	int index_of_text_input=0;
	int if_in_rule = 0;//标记状态，是读取规则名称还是规则动作
	int under_if = 0;
	int id_of_rule = 0;//记录规则的id，直接读取的，不一定按顺序
	int f = 0;
	int num_of_rule = 0;//记录有多少个规则
	int in_reply = 0;//用来记录是否在引号里
	while (!feof(p_test_file))
	{
		//标记已经处理到读入的字符的下标
		index_of_text_input = 0;

		//逐行读文件
		fgets(text_in_test, sizeof(text_in_test), p_test_file);

		//跳过空行
		if (text_in_test[0] == '\n')
		{
			continue;
		}

		//打算跟新一下读取方式，先把词拆出来再strcmp就方便多了，不然好麻烦。。。
		if (if_in_rule == 1)
		{
			//分割出读入的单词
			char words[90][10];
			index_of_text_input = 0;
			int index_of_words = 0;
			int char_index = 0;
			for (;; index_of_text_input++)
			{
				if (text_in_test[index_of_text_input] == '\"')//对于双引号，单开一个循环来读取
				{
					if (in_reply == 0)
						in_reply = 1;
					else if (in_reply == 1)
						in_reply = 0;
				}
				else if (text_in_test[index_of_text_input] == ' '&&char_index!=0&&in_reply==0)
				{
					words[index_of_words][char_index ] = '\0';
					index_of_words++;
					char_index = 0;
					continue;
				}
				else if (text_in_test[index_of_text_input] == ' ' && char_index == 0&&in_reply==0)
				{
					 continue;
				}
				else if (text_in_test[index_of_text_input] == '\n')
				{
					 words[index_of_words][char_index] = '\0';
					 if (in_reply == 1)
					 {
						 printf("reply error");
						 return 0;
					 }
					 break;
				}
				else {
					 words[index_of_words][char_index] = text_in_test[index_of_text_input];
					 char_index++;
				}
			}
			/*for (int i = 0; i <= index_of_words; i++)
			{
				printf("%s ", words[i]);
			}
			printf("\n");*/

			for (int i = 0; i <= index_of_words; )
			{
				if (under_if == 1 && !strcmp(words[i], "then"))//有if配对的then正常处理
				{
					if (!strcmp(words[i + 1], "reply"))
					{
						strcpy_s(rules[id_of_rule].reply, words[i + 2]);
					}
					i = i + 3;

					//暂且先定成处理完then就结束这个规则
					if_in_rule = 0;
					//这个也是要归零的
					under_if = 0;

					//printf("调试信息%d：规则%d的condition和reply：%s %s\n", f, id_of_rule, rules[id_of_rule].condition, rules[id_of_rule].reply);
					//这个也要归零
					id_of_rule = 0;
				}
				else if(under_if == 0 && !strcmp(words[i], "then"))//无if配对的then就报错
				{
					printf("error%d:no \"then\" after \"if\".",f);
					return 0;
				}
				if (!strcmp(words[i],"if"))
				{
					strcpy_s(rules[id_of_rule].condition, words[i + 1]);
					i = i + 2;
					under_if = 1;
				}

			}
		}
		else if (if_in_rule == 0)
		{	
			//如果是end，直接结束循环
			if (!strcmp(text_in_test, "end") || !strcmp(text_in_test,"end\n"))
			{
				break;
			}
			//把"rule"拆出来
			for (; index_of_text_input <= 3; index_of_text_input++)
			{
				b[index_of_text_input] = text_in_test[index_of_text_input];
			}
			b[4] = '\0';
			if (strcmp(b, "rule") && if_in_rule == 0)
			{
				printf("rule define error\n");
			}
			else {
				if_in_rule = 1;
				num_of_rule++;
			}

			//拆出rule后面的序号
			for (;; index_of_text_input++)
			{
				if (text_in_test[index_of_text_input] >= '0' && text_in_test[index_of_text_input] <= '9')
				{
					id_of_rule = id_of_rule * 10 + (text_in_test[index_of_text_input] - '0');
				}
				else if (text_in_test[index_of_text_input] == ' ')
				{
					index_of_text_input++;
					break;
				}
			}
			
			//读取规则名称
			int i = 0;
			for (;; index_of_text_input++)
			{
				if (text_in_test[index_of_text_input] == '\n')
				{
					break;
					index_of_text_input = 0;
				}
				if (text_in_test[index_of_text_input] == ' ')
				{
					continue;
				}
				rules[id_of_rule].rule_name[i] = text_in_test[index_of_text_input];
				i++;
			}
			rules[id_of_rule].rule_name[i + 1] = '\0';

			//printf("调试信息，检查读入的规则名称：%s\n", rules[id_of_rule].rule_name);

		}

		f++;
	}

	/*//开始处理用户的输入内容
	char input[30];
	while (1)
	{
		scanf_s("%s", input,(unsigned int)sizeof(input));
		int i = 1;
		for (;i<=num_of_rule;i++)
		{
			if (!strcmp(rules[i].condition, input))
			{
				printf("%s\n", rules[i].reply);
				if (!strcmp(rules[i].rule_name, "quit"))
				{
					return 0;
				}
				break;
			}

		}
		if (i > num_of_rule)
		{
			printf("抱歉，我听不懂您在说什么");
		}
	}
	*/

	test_api_directly();

	// 改为：
	while (1) {
		char input[256];
		printf("> ");

		// 使用fgets读取整行（支持空格）
		if (fgets(input, sizeof(input), stdin) == NULL) {
			break;
		}

		// 移除换行符
		input[strcspn(input, "\n")] = '\0';

		if (strlen(input) == 0) {
			continue;
		}

		// 调用API识别意图
		char* intent = call_qwen_api_simple(input,conditions, index_of_conditions +1);

		// 匹配规则
		int matched = 0;
		for (int i = 1; i <= num_of_rule; i++) {
			if (!strcmp(rules[i].condition, intent)) {
				printf("%s\n", rules[i].reply);
				matched = 1;

				// 如果是退出意图，结束程序
				if (!strcmp(intent, "退出")) {
					free(intent);
					return 0;
				}
				break;
			}
		}

		if (!matched) {
			printf("抱歉，我听不懂您在说什么\n");
		}

		free(intent);
	}
	return 0;
}
