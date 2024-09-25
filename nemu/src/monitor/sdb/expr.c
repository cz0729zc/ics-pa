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
  TK_NOTYPE = 256, 
  TK_DECIMAL = 257,  // 十进制整数
  RESGISTER = 258,
  HEX = 259,
  TK_EQ = 1 ,
  TK_LEQ = 2,     //261
  TK_NOTEQ = 3,
  OR = 4,
  AND = 5,

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
  
  {"\\|\\|", OR},           // 与
  {"\\&\\&", AND},          // 或
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

static Token tokens[32] __attribute__((used)) = {};
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
    int x_size = 0, flag = 1;
    while(tmp_x){
	tmp_x /= 10;
	x_size ++;
	flag *= 10;
    }
    flag /= 10;
    while(x)
    {
	int a = x / flag; 
	x %= flag;
	flag /= 10;
	str[tmp_index ++] = a + '0';
    }
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
	  case OR:
	    tokens[nr_token].type = 4;
	    stpcpy(tokens[nr_token].str, "||");
	    nr_token++;
	    break;
	  case AND:
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
	    int tmp = isa_reg_str2val(tokens[i].str, &flag);
	    if(flag){
		int_char(tmp, tokens[i].str); // transfrom the str --> $egx
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
            int value = strtol(tokens[i].str, NULL, 16);
            printf("Value : %d",value);
            int_char(value, tokens[i].str);
        }
    }
    //对-1进行处理
    for(int i = 0 ; i < tokens_len ; i ++)
    {
	if((tokens[i].type == '-' && i > 0 && tokens[i-1].type != TK_DECIMAL && tokens[i+1].type == TK_DECIMAL)
		||
		(tokens[i].type == '-' && i == 0)
	  )
	{
	    //printf("%s\n", tokens[i+1].str);
	    tokens[i].type = TK_NOTYPE;
	    //tokens[i].str = tmp;
	    for(int j = 31 ; j >= 0 ; j --){
		tokens[i+1].str[j] = tokens[i+1].str[j-1];//向后移一位
	    }
	    tokens[i+1].str[0] = '-';
	    // printf("%s\n", tokens[i+1].str);
	    for(int j = 0 ; j < tokens_len ; j ++){
		if(tokens[j].type == TK_NOTYPE)
		{
		    for(int k = j +1 ; k < tokens_len ; k ++){
			tokens[k - 1] = tokens[k];
		    }
		    tokens_len -- ;
		}
	    }
	}
    }
    //对*指针进行预处理
    for(int i = 0 ; i < tokens_len ; i ++)
    {
	if(	(tokens[i].type == '*' && i > 0 
		    && tokens[i-1].type != TK_DECIMAL && tokens[i-1].type != HEX && tokens[i-1].type != RESGISTER
		    && tokens[i+1].type == TK_DECIMAL 
		    )
                ||
		(tokens[i].type == '*' && i > 0
                    && tokens[i-1].type != TK_DECIMAL && tokens[i-1].type != HEX && tokens[i-1].type != RESGISTER
                    && tokens[i+1].type == HEX
                    )
		||
                (tokens[i].type == '*' && i == 0)
          )
		{
            tokens[i].type = TK_NOTYPE;
            int tmp = char_int(tokens[i+1].str);
            uintptr_t a = (uintptr_t)tmp;
            int value = *((int*)a);
            int_char(value, tokens[i+1].str);	    
            // 
            for(int j = 0 ; j < tokens_len ; j ++){
                if(tokens[j].type == TK_NOTYPE){
                    for(int k = j +1 ; k < tokens_len ; k ++){
                    tokens[k - 1] = tokens[k];
                }
                    tokens_len -- ;
                }
            }
		}
    }
  return true;
}


bool check_parentheses(int p, int q)
{
    if(tokens[p].type != '('  || tokens[q].type != ')')
        return false;
    int l = p , r = q;
    while(l < r)
    {
        if(tokens[l].type == '('){
            if(tokens[r].type == ')')
            {
                l ++ , r --;
                continue;
            }

            else
                r --;
        }
        else if(tokens[l].type == ')')
            return false;
        else l ++;
    }
    return true;
}
 

uint32_t eval(int p, int q) {
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
        if(tokens[p].type == HEX)
        {
	    long hex_value = strtol(tokens[p].str, NULL, 16);
	    // 打印十六进制数的值
	    printf("十六进制数的值是: %ld\n", hex_value);
	    return (int)hex_value; // 返回转换后的整数值
        }
        return atoi(tokens[p].str);
    }
    else if (check_parentheses(p, q) == true) {
        /* The expression is surrounded by a matched pair of parentheses.
         * If that is the case, just throw away the parentheses.
         */
        // printf("check p = %d, q = %d\n",p + 1 , q - 1);
        return eval(p + 1, q - 1);
    }
    /* else if(check_parentheses(p, q) == false){
       printf("Unique\n");
       return -1;
       }
       */
    else {
        int op = -1; // op = the position of 主运算符 in the token expression;
        bool flag = false;
        for(int i = p ; i <= q ; i ++)
        {
            if(tokens[i].type == '(')
            {
                while(tokens[i].type != ')')
                    i ++;
            }
            if(!flag && tokens[i].type == 4){
                flag = true;
                op = max(op,i);
            }
            if(!flag && tokens[i].type == 5){
                flag = true;
                op = max(op,i);
            }
            if(!flag && tokens[i].type == 3){
                flag = true;
                op = max(op,i);
            }  
            if(!flag && tokens[i].type == 1){
                flag = true;
                op = max(op,i);
            }  
            if(!flag && tokens[i].type == 2){
                flag = true;
                op = max(op,i);
            }  
            if(!flag && (tokens[i].type == '+' || tokens[i].type == '-')){
                flag = true;
                op = max(op, i);
            }
            if(!flag && (tokens[i].type == '*' || tokens[i].type == '/') ){
                op = max(op, i);
            }
            
            
        }
              printf("op position is %d\n", op);
        // if register return $register
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
                return val1 * val2;
            case '/':
                if(val2 == 0){//printf("division can't zero;\n");
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
    
    if (division_zero) {
        // 处理除以0的情况
        *success = false;
        return 0;
    }
    
    *success = true;
    return result;
}
 
