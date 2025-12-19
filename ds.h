#pragma once
#include <stdio.h>

char scene_names[20][20];

typedef char Condition[10];//意图，以字符串形式储存

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
Condition conditions[10];

//关键字,规则以rule开头，后跟规则名，end_rule表示规则的结束,if后跟意图,下一行跟then＋行动（回复或跳转）。
//condition后跟意图，如果没有定义意图则输出No define of conditions
Keyword keywords[] = { "rule","if","rhen","jumpto","end_rule","condition" };