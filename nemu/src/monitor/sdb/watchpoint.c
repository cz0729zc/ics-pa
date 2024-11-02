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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
    bool flag; // use / unuse
    char expr[100];
    int new_value;
    int old_value;
} WP;
//设置静态可以使变量在整个程序
static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp(){
    for(WP* p = free_ ; p -> next != NULL ; p = p -> next){
        if( p -> flag == false){
            p -> flag = true;
            if(head == NULL){
                head = p;
            }
            return p;
        }
    }
    printf("No unuse point.\n");
    assert(0);
    return NULL;
}

void free_wp(WP *wp){
    if(head -> NO == wp -> NO){
        head -> flag = false;
        head = NULL;
        printf("Delete watchpoint  success.\n");
        return ;
    }
    for(WP* p = head ; p -> next != NULL ; p = p -> next){
        if(p -> next -> NO  == wp -> NO)
        {
            p -> next = p -> next -> next;
            p -> next -> flag = false; // 没有被使用
            printf("free succes.\n");
            return ;
        }
    }
}

void sdb_watchpoint_display(){
    bool flag = true;
    for(int i = 0 ; i < NR_WP ; i ++){
        if(wp_pool[i].flag){
            printf("Watchpoint.No: %d, expr = \"%s\", old_value = %d, new_value = %d\n",
                    wp_pool[i].NO, wp_pool[i].expr,wp_pool[i].old_value, wp_pool[i].new_value);
                flag = false;
        }
    }
    if(flag) printf("No watchpoint now.\n");
}

void delete_watchpoint(int no){
    for(int i = 0 ; i < NR_WP ; i ++)
        if(wp_pool[i].NO == no){
            free_wp(&wp_pool[i]);
            return ;
        }
}

void create_watchpoint(char* args){
    WP* p =  new_wp();
    strcpy(p -> expr, args);
    bool success = false;
    int tmp = expr(p -> expr,&success);
   if(success) p -> old_value = tmp;
   else printf("创建watchpoint的时候expr求值出现问题\n");
    printf("Create watchpoint No.%d success.\n", p -> NO);
}

bool check_watchpoints() {  
    WP* current_wp = head;  // 新建一个监视点结构体，并将其设为监视点池中的第一个监视点  

    bool any_changed = false;  // 用于标记是否有监视点的值发生变化  

    while (current_wp != NULL) {  // 循环条件设置为新建的监视点不为head的下一个监视点  
        bool success = false; 
        printf("urrent_wp->expr: %s",current_wp->expr);
        word_t current_value = expr(current_wp->expr, &success);  // 调用expr()函数计算表达式的值  
        
        if (success) {  
            if (current_value != current_wp->old_value) {  // 如果表达式的值发生变化  
                printf("触发监视点\n");
                printf("Watchpoint triggered: No.%d, expr=\"%s\", old_value=0x%08x, new_value=0x%08x\n",  
                        current_wp->NO, current_wp->expr, current_wp->old_value, current_value);  
                current_wp->old_value = current_value;  // 更新旧值  
                any_changed = true;  // 标记有变化  
            }  
        } else {  
            printf("Error evaluating expression for watchpoint No.%d\n", current_wp->NO);  
        }  

        current_wp = current_wp->next;  // 将监视点设置为下一个监视点  
    }  
    return any_changed;  // 返回是否有监视点的值发生变化  
}
