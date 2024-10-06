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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/paddr.h>


static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static int cmd_p(char* args);

static int cmd_w(char* args);

static int cmd_d(char* args);

void sdb_watchpoint_display();

void delete_watchpoint(int no);

void create_watchpoint(char* args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "运行 n行", cmd_si },
  { "info", "r打印寄存器值w打印监视点信息", cmd_info },
  { "x", "扫描内存 一次4行 x 10 0x80000000", cmd_x },
  { "p", "正则表达式求值" , cmd_p },
  { "w", "设置监视点" , cmd_w },
  { "d", "删除监视点" , cmd_d },
  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args) {
  int step = 1;  // Default step is 1
  if (args != NULL) {
    step = atoi(args);
  }

  for (int i = 0; i < step; i++) {
    cpu_exec(1);  // Execute one instruction
  }

  return 0;
}

static int cmd_info(char *args) {
    if(args == NULL)
        printf("No args.\n");
    else if(strcmp(args, "r") == 0)
        isa_reg_display();
    else if(strcmp(args, "w") == 0)
        sdb_watchpoint_display();
    return 0;
}

static int cmd_x(char *args){
	//printf("args = %s\n", args);
    char* n = strtok(args," ");  
    //printf("n = %s\n", n);
    
    char* baseaddr = strtok(NULL," ");
    //printf("baseaddr = %s\n", baseaddr);
    
    int len = 0;
    paddr_t addr = 0;
    sscanf(n, "%d", &len);
    printf("len = %d\n", len);
    
    sscanf(baseaddr,"%x", &addr);
    printf("addr = 0x%x\n", addr);
    for(int i = 0 ; i < len ; i ++)
    {
        printf("0x%x\n",paddr_read(addr,4));//addr len
        addr = addr + 4;
    }
    return 0;
}

static int cmd_p(char* args){
    if(args == NULL){
        printf("No args\n");
        return 0;
    }
    //  printf("args = %s\n", args);
    bool flag = false;
    printf("%d\n",expr(args, &flag));
    return 0;
}

static int cmd_d (char *args){
    if(args == NULL)
        printf("No args.\n");
    else{
        delete_watchpoint(atoi(args));
    }
    return 0;
}

static int cmd_w(char* args){
    create_watchpoint(args);
    return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }
  
  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

    if (strcmp(cmd, "si") == 0) {
      cmd_si(args);
      continue;
   }
   
    if (strcmp(cmd, "info") == 0) {
      cmd_info(args);
      continue;
    }
    
    if (strcmp(cmd, "p") == 0) {
      cmd_p(args);
      continue;
    }
    
    if (strcmp(cmd, "w") == 0) {
      cmd_w(args);
      continue;
    }
    
    if (strcmp(cmd, "d") == 0) {
      cmd_d(args);
      continue;
    }
#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
