#include <stdio.h>
#include "ds.h"
#include <string.h>
#include <stdlib.h>

int main()
{
	//打开配置文件
	FILE* pconfig;
	fopen_s(&pconfig, "config.txt", "r");
	if (!pconfig)
	{
		printf("Can't not open config\n");
		return 0;
	}
	else
	{
		printf("right\n");//调试信息，到时候注释掉
	}

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
	printf("%s\n", file_name_of_test);

	//打开file_name_of_test作为文件名的文件
	FILE* p_test_file;
	fopen_s(&p_test_file, file_name_of_test, "r");
	if (!p_test_file)
	{
		printf("can't open test_file\n");
		return 0;
	}
	else
	{
		printf("right\n");
	}

	char text_in_test[90];//用来读取脚本文件
	char b[20];//辅助的
	int index = 0;//用来标记已读入那行的文件内容处理到了哪里

	//读取第一行
	fgets(text_in_test, sizeof(text_in_test), p_test_file);
	printf("%s", text_in_test);

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

	printf("调试信息，解析出来的condtion：");
	for (int i = 0; i <= index_of_conditions; i++)
	{
		printf("%s ", conditions[i]);
	}
	printf("\n");

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
			for (int i = 0; i <= index_of_words; i++)
			{
				printf("%s ", words[i]);
			}
			printf("\n");

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

					printf("调试信息%d：规则%d的condition和reply：%s %s\n", f, id_of_rule, rules[id_of_rule].condition, rules[id_of_rule].reply);
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

			printf("调试信息，检查读入的规则名称：%s\n", rules[id_of_rule].rule_name);

		}

		f++;
	}

	//开始处理用户的输入内容
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

	return 0;
}
