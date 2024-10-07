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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

// 全局变量用于跟踪缓冲区索引  
static int index_buf = 0;  

// 生成随机数字并添加到缓冲区  
static void gen_num() {  
    int num = rand() % 100; // 生成一个0到99之间的随机数字  
    char num_str[12]; // 存储数字字符串的缓冲区  
    sprintf(num_str, "%d", num); // 将数字转换为字符串  
    for (int i = 0; num_str[i] != '\0'; i++) {  
        buf[index_buf++] = num_str[i]; // 添加到 buf  
    }  
}  

// 向缓冲区中添加一个字符  
static void gen(char c) {  
    if (index_buf < sizeof(buf) - 1) {  
        buf[index_buf++] = c;  
        buf[index_buf] = '\0'; // 字符串结束符  
    } else {  
        // 处理缓冲区溢出  
        buf[0] = '\0';  
        index_buf = 0;  
    }  
}  

// 生成随机运算符并添加到缓冲区  
static void gen_rand_op() {  
    char ops[] = {'+', '-', '*', '/'}; // 可用的运算符  
    int op_index = rand() % 4; // 随机选择一个运算符  
    gen(ops[op_index]); // 将运算符添加到缓冲区  
}  

// 从0到n-1随机选择一个数字  
static int choose(int n) {  
    return rand() % n; // 生成随机索引  
}  

static void gen_rand_expr() {
  //  buf[0] = '\0' ;
  if(index_buf > 655){
  //    printf("overSize\n");
  index_buf = 0;}
  
  switch(choose(3)){
    case 0: 
      gen_num();
 
      break;
    case 1:
      gen('(');
      gen_rand_expr();
      gen(')');
      break;
    default:
      gen_rand_expr();
      gen_rand_op();
      gen_rand_expr();
      break;

    }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    index_buf = 0;
    gen_rand_expr();
    buf[index_buf] = '\0';
    long size = 0;
    sprintf(code_buf, code_format, buf);
 
    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);
    
    FILE *fp_err = fopen("/tmp/.err_message","w");
    assert(fp_err != NULL);
 
    int ret = system("gcc /tmp/.code.c -o /tmp/.expr 2>/tmp/.err_message");
    
    fseek(fp_err, 0, SEEK_END);
 
    // 获取文件大小
    size = ftell(fp_err);
    fclose(fp_err);
    if (ret != 0 ||size != 0){index_buf = 0; i--; continue;}
    
    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);
 
    uint32_t result;
    ret = fscanf(fp, "%u", &result);
    pclose(fp);
 
    printf("%u %s\n", result, buf);
    index_buf = 0;
  }
  return 0;
}

