/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <stdio.h>
#include <stdlib.h>
#define max(a, b) ((a) > (b) ? (a) : (b))
bool division_zero = false;


/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256,   // 空格
  TK_DECIMAL = 257,  // 十进制整数
  RESGISTER = 258,   // 寄存器
  HEX = 259,      	 // 16进制数
  TK_EQ = 1 ,        // ==
  TK_LEQ = 2,      	 // 小于等于
  TK_NOTEQ = 3, 	//  不等于
  TK_OR = 4,
  TK_AND = 5,
  
  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"==", TK_EQ},        // equal

  {"0[xX][0-9a-fA-F]+", HEX},          //16进制
  {"[0-9]+", TK_DECIMAL},   // 十进制整数
  
  {"\\+", '+'},             // plus
  {"-", '-'},               // 减法
  {"\\*", '*'},             // 乘法
  {"/", '/'},               // 除法
  
  {"\\(", '('},             // 左括号
  {"\\)", ')'},             // 右括号
  
  {"\\<\\=",TK_LEQ},        // 小于等于
  {"\\!\\=", TK_NOTEQ},      // 不等于
  
  {"\\|\\|", TK_OR},           // 或
  {"\\&\\&", TK_AND},          // 与
  {"\\!", '!'},             // 非
  
  {"\\$[a-zA-Z]*[0-9]*", RESGISTER},   //寄存器
  
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[1024] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

int char_int(char s[]){
    int s_size = strlen(s);
    int res = 0 ;
    for(int i = 0 ; i < s_size ; i ++)
    {
	res += s[i] - '0';
	res *= 10;
    }
    res /= 10;
    return res;
}

void int_char(int x, char str[]){
    int len = strlen(str);
    memset(str, 0, len);
    int tmp_index = 0;
    int tmp_x = x;
    int x_size = 0;
    long flag = 1;
    while(tmp_x){
	tmp_x /= 10;
	x_size ++;
	flag *= 10;
    }
    flag /= 10;
    while(x)
    {
	long a = x / flag; 
	x %= flag;
	flag /= 10;
	str[tmp_index ++] = a + '0';
    }
}

void unsigned_long_char(unsigned long x, char str[]) {  
    int len = strlen(str);  
    memset(str, 0, len);  
    int tmp_index = 0;  
    unsigned long tmp_x = x;  
    int x_size = 0;  
    unsigned long flag = 1;  
    
    // 特殊情况：处理 x 为 0 的情况  
    if (x == 0) {  
        str[tmp_index++] = '0';  
        str[tmp_index] = '\0'; // 确保字符串终止  
        return;  
    } 
    
    // 计算 x 的位数  
    while (tmp_x) {  
        tmp_x /= 10;  
        x_size++;  
        flag *= 10; 
    }  
    flag /= 10;  

    // 将 unsigned long 型 x 转换为字符串  
    while (flag) {  
        unsigned long a = x / flag;
        x %= flag;  
        flag /= 10;  
        str[tmp_index++] = a + '0';  
    }
    str[tmp_index] = '\0';
    //printf("str:%s\n", str);  
}  


static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
	  case '+':
	    // 处理加法
	    tokens[nr_token].type = '+';
	    nr_token++;
	    break;
	  case '-':
	    // 处理减法
	    tokens[nr_token].type = '-';
	    nr_token++;
	    break;
	  case '*':
	    // 处理乘法
	    tokens[nr_token].type = '*';
	    nr_token++;
	    break;
	  case '/':
	    // 处理除法
	    tokens[nr_token].type = '/';
	    nr_token++;
	    break;
	  case '(':
	    // 处理左括号
	    tokens[nr_token].type = '(';
	    nr_token++;
	    break;
	  case ')':
	    // 处理右括号
	    tokens[nr_token].type = ')';
	    nr_token++;
	    break;
	  case TK_NOTYPE:
	    // 忽略空格串
	    break;
	  case '!':
	    tokens[nr_token].type = '!';
	    nr_token++;
	    break;
	    
	  case 257:
            tokens[nr_token].type = 257;
            strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
            nr_token ++;
            break;
	  case RESGISTER:
            tokens[nr_token].type = RESGISTER;
            strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
            nr_token ++;
            break;
	  case HEX:
            tokens[nr_token].type = HEX;
            strncpy(tokens[nr_token].str, &e[position - substr_len], substr_len);
            nr_token ++;
            break;
            
	  case TK_EQ:
	    tokens[nr_token].type = 1;
	    stpcpy(tokens[nr_token].str, "==");
	    nr_token++;
	    break;
	  case TK_LEQ:
	    tokens[nr_token].type = 2;
	    stpcpy(tokens[nr_token].str, "<=");
	    nr_token++;
	    break;
	  case TK_NOTEQ:
	    tokens[nr_token].type = 3;
	    stpcpy(tokens[nr_token].str, "!=");
	    nr_token++;
	    break;
	  case TK_OR:
	    tokens[nr_token].type = 4;
	    stpcpy(tokens[nr_token].str, "||");
	    nr_token++;
	    break;
	  case TK_AND:
	    tokens[nr_token].type = 5;
	    stpcpy(tokens[nr_token].str, "&&");
	    nr_token++;
	    break; 
          default: printf("wo hai mei xie");
        }

        break;
      }
}

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
}
    //获取tokens长度
    int tokens_len = 0;
    for(int i = 0 ; i < 30 ; i ++)
    {
	if(tokens[i].type == 0)
	    break;
	tokens_len ++;
    }
    
    
    //初始化tokens寄存器 
    for(int i = 0 ; i < tokens_len ; i ++)
    {
	if(tokens[i].type == 258)
	{
	    bool flag = true;
	    unsigned long tmp = isa_reg_str2val(tokens[i].str, &flag);
	    if(flag){
		unsigned_long_char(tmp, tokens[i].str); // transfrom the str --> $egx
	    }else{
		printf("Transfrom error. \n");
		assert(0);
	    }
	}
    }
    //初始化tokens 16进制
    for(int i = 0 ; i < tokens_len ; i ++)
    {
        if(tokens[i].type == 259)// Hex num
        {
            //printf("Value : %s\n",tokens[i].str);
            unsigned long value = strtol(tokens[i].str, NULL, 16);

    	    //printf("Value : %lld\n",value);
    	    unsigned_long_char(value, tokens[i].str);
    	    //printf("Value : %s\n",tokens[i].str);           
        }
    }
    //对-1进行处理
    for(int i = 0 ; i < tokens_len ; i ++)
    {
		if (tokens[i].type == '-' && (i == 0 || (i > 0 && tokens[i-1].type != TK_DECIMAL)) && (i + 1 < tokens_len && tokens[i + 1].type == TK_DECIMAL))		
		{
			//printf("处理负数\n");
            tokens[i].type = TK_NOTYPE;  
            // 将下一个数字前添加负号  
            for (int j = strlen(tokens[i + 1].str); j >= 0; j--) {  
                tokens[i + 1].str[j + 1] = tokens[i + 1].str[j]; // 向后移一位  
            }  
            tokens[i + 1].str[0] = '-'; // 添加负号  
            tokens[i + 1].type = TK_DECIMAL; // 设置为负数  
            
            //tokens_len--; // 更新 tokens 的数量 
            
            // 更新 tokens 的数量 
			for(int j = 0;j < tokens_len; j ++){ 
				if(tokens[j].type == TK_NOTYPE)
				{
				  for(int k = j+1; k < tokens_len;k ++){
					tokens[k - 1] = tokens[k];
				  }
				  tokens_len --;
				  nr_token--;   //全局变量也要减
				}
			}
            
            //输出token长度和token
			//printf("Tokens length: %d\n", tokens_len);  
			//for (int i = 0; i < tokens_len; i++) {  
			//	printf("Token Type: %d, Token String: %s\n", tokens[i].type, tokens[i].str);  
			//}
		}
    }
    //对*指针进行预处理
    
	for (int i = 0; i < tokens_len; i++) {
		// 判断当前的 '*' 是指针解引用
		if ((tokens[i].type == '*' && i > 0 &&
		     tokens[i-1].type != TK_DECIMAL && tokens[i-1].type != HEX && tokens[i-1].type != RESGISTER && tokens[i-1].type != ')' &&
		     (tokens[i+1].type == TK_DECIMAL || tokens[i+1].type == HEX)) 
		    || 
		    (tokens[i].type == '*' && i == 0)) {
		    
		    // 将 '*' 标记为不需要的 token，准备移除
		    tokens[i].type = TK_NOTYPE;

		    // 获取 '*' 后的数字或地址值，并进行解引用
		    int tmp = char_int(tokens[i+1].str); // 将数字或地址字符串转为整数
		    uintptr_t addr = (uintptr_t)tmp;     // 将整数转换为指针地址
		    unsigned long value = *((unsigned long*)addr); // 解引用该地址

		    // 将解引用后的值转换为字符串，存储在 tokens[i+1]
		    unsigned_long_char(value, tokens[i+1].str); // 将值写回 tokens[i+1]

		    // 删除 `*` 这个 token，并调整数组长度
		    for (int j = i; j < tokens_len - 1; j++) {
		        tokens[j] = tokens[j + 1];  // 将后续 token 向前移动
		    }
		    tokens_len--;   // 更新 token 长度
		    nr_token--;     // 更新 token 总数

		    i--;  // 回退索引以重新检查移动后的 token
		}
	}

  return true;
}


bool check_parentheses(int p, int q) {
    // 首先检查开头和结尾的括号是否为一对
    if (tokens[p].type != '(' || tokens[q].type != ')') {
        return false;
    }

    // 计数器用来记录括号的匹配情况
    int parentheses_count = 0;

    // 遍历 p+1 到 q-1，检查括号是否成对匹配
    for (int i = p; i <= q; i++) {
        if (tokens[i].type == '(') {
            parentheses_count++; // 遇到左括号，计数器加一
        } else if (tokens[i].type == ')') {
            parentheses_count--; // 遇到右括号，计数器减一
        }

        // 如果在遍历过程中，右括号比左括号多，说明不匹配
        if (parentheses_count < 0) {
            return false;
        }
    }

    // 最终计数器应回到0，表示括号匹配
    return parentheses_count == 0;
}


//获取优先级
int get_operator_priority(int type) {
    switch (type) {
        case TK_OR:
            return 1; // OR 优先级最低
        case TK_AND:
            return 2;
        case TK_LEQ:
        case TK_NOTEQ:
            return 3;
        case '+':
        case '-':
            return 4;
        case '*':
        case '/':
            return 5; // 乘除优先级最高
        default:
            return 100; // 非运算符
    }
}


uint32_t eval(int p, int q) {
	printf("Entering eval with p=%d, q=%d\n", p, q);
    if (p > q) {
        /* Bad expression */
        assert(0);
        return -1;
    }
    else if (p == q) {
        /* Single token.
         * For now this token should be a number.
         * Return the value of the number.
         */
        
        return atoi(tokens[p].str);
    }
    else if (check_parentheses(p, q)) {
        /* The expression is surrounded by a matched pair of parentheses.
         * If that is the case, just throw away the parentheses.
         */
         printf("check p = %d, q = %d\n",p , q);
        return eval(p + 1, q - 1);
    }
    else if(check_parentheses(p, q) == false){
       printf("Unique\n");
       return -1;
       }
    else {
        int op = -1; // op = the position of 主运算符 in the token expression;
        int min_priority = 1000; // 初始化为一个较大的值
        int parentheses_level = 0; // 记录当前括号的层级

        for (int i = p; i <= q; i++) {
            if (tokens[i].type == '(') {
                parentheses_level++;
            } else if (tokens[i].type == ')') {
                parentheses_level--;
            }

            if (parentheses_level == 0) {
                int current_priority = get_operator_priority(tokens[i].type);
                if (current_priority <= min_priority) {
                    min_priority = current_priority;
                    op = i;
                }
            }
        }

        if (op == -1) {
            printf("No operator found.\n");
            assert(0);
        }
        printf("op position is %d\n", op);
        int  op_type = tokens[op].type;
        // 递归处理剩余的部分
        uint32_t  val1 = eval(p, op - 1);
        uint32_t  val2 = eval(op + 1, q);
        printf("val1 = %d, val2 = %d \n", val1, val2);

        switch (op_type) {
            case '+':
                return val1 + val2;
            case '-':
                return val1 - val2;
            case '*':
            	printf("使用乘法\n");
                return val1 * val2;
            case '/':
                if(val2 == 0){
                	printf("division can't zero;\n");
                    division_zero = true;
                    return 0;
                }
                return val1 / val2;
            case 1:
                return val1 == val2;
            case 3:
                return val1 != val2;
            case 4:
                return val1 || val2;
            case 5:
                return val1 && val2;
            default:
                printf("No Op type.");
                assert(0);
        }
    }
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
      // 调用eval函数计算表达式的值
    word_t result = eval(0, nr_token - 1);
    
    memset(tokens, 0, sizeof(tokens)); // 清空 tokens 数组 
    
    if (division_zero) {
        // 处理除以0的情况
        *success = false;
        return 0;
    }
    
    *success = true;
    return result;
}
 
