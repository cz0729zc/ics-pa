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

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
  TYPE_I, TYPE_U, TYPE_S,
  TYPE_N, TYPE_J, TYPE_B,
  TYPE_R// none
};

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
//新增
#define immJ() do { \
    *imm = (SEXT(BITS(i, 31, 31), 1) << 20) | /* imm[20] */ \
           (BITS(i, 19, 12) << 12) |          /* imm[19:12] */ \
           (BITS(i, 20, 20) << 11) |          /* imm[11] */ \
           (BITS(i, 30, 21) << 1);            /* imm[10:1] */ \
           *imm = SEXT(*imm, 21);             /* 符号扩展 21 位立即数 */ \
} while(0)

#define immB() do { \
    *imm = (SEXT(BITS(i, 31, 31), 1) << 12) | /* imm[12] */ \
           (BITS(i, 7, 7) << 11) |		        /* imm[11] */ \
           (BITS(i, 30, 25) << 5) |           /* imm[10:5] */ \
           (BITS(i, 11,8) << 1);              /* imm[4:1] */ \
           *imm = SEXT(*imm, 13);             /* 符号扩展 13 位立即数 */ \
} while(0)  


static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst.val;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);
  switch (type) {
    //case TYPE_I: src1R();          immI();printf("rs1: %u, rd: %u, imm: %d\n",rs1,*rd,*imm); break;
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_J:                   immJ(); break;
    //case TYPE_B: src1R(); src2R(); immB(); printf("rs1: %u,rs2: %u, rd: %u, imm: %d\n",*src1,*src2,*rd,*imm); break;
    case TYPE_B: src1R(); src2R(); immB(); break;
    //case TYPE_R: src1R(); src2R(); printf("rs1: %3d,  rs2: %3d, rd: %3u\n",*src1,*src2,*rd);       ; break;
    case TYPE_R: src1R(); src2R();       ; break;
  }
}

static int decode_exec(Decode *s) {
  int rd = 0;
  word_t src1 = 0, src2 = 0, imm = 0;
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}

  INSTPAT_START();
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, R(rd) = imm);
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw     , I, R(rd) = Mr(src1 + imm, 4));
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, Mw(src1 + imm, 4, src2));

  //新增指令
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh     , S, Mw(src1 + imm, 2, src2));
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 +imm , 1 ,src2 & 0xff));
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1+imm);
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi   , I, R(rd) = src1 & imm);//printf("rs1: 0x%0x,  imm: 0x%0x, R(rd): %3d\n",src1,imm,R(rd));
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh     , I, R(rd) = SEXT(Mr(src1 + imm, 2),16));//符号位扩展，共读取两个字节八个比特位
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu    , I, R(rd) = Mr(src1 + imm, 2));
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("0000000 ????? ????? 001 ????? 00100 11", slli   , I, R(rd) = src1 << (imm & 0x1F));
  INSTPAT("0000000 ????? ????? 101 ????? 00100 11", srli   , I, R(rd) = src1 >> (imm & 0x1F));
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll    , I, R(rd) = src1 << (src2 & 0x1F));
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl    , I, R(rd) = src1 >> (src2 & 0x1F));
  INSTPAT("0100000 ????? ????? 101 ????? 00100 11", srai   , I, R(rd) = (sword_t)src1 >> (imm & 0x1F));
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal    , J, R(rd) = s->snpc; s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr   , I, R(rd) = s->snpc; s->dnpc = (src1 + imm) & ~1);//确保整数倍
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul    , R, R(rd) = src1*src2);
  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div    , R, R(rd) = (sword_t)src1/(sword_t)src2);
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem    , R, R(rd) = (sword_t)src1%(sword_t)src2);
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub    , R, R(rd) = src1-src2);
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add    , R, R(rd) = src1+src2);
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or     , R, R(rd) = src1|src2);
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor    , R, R(rd) = src1^src2);
  INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh   , R, printf("rs1: %ld,  rs2: %d, R(rd): %ld\n",SEXT(src1,32),src2,SEXT(src1,32)*SEXT(src2,32) >> 32);R(rd) = SEXT(src1,32)*SEXT(src2,32) >> 32);//乘法指令取高32位
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul    , R, R(rd) = src1*src2);//乘法指令取低32位
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu   , R, R(rd) = src1%src2);
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu   , R, R(rd) = src1/src2);
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori   , I, R(rd) = src1|imm);
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu  , I, \
    if ((sword_t)src1<(sword_t)imm) \
      R(rd) = 1; \
    else \
      R(rd) = 0;);
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu   , R, \
    if (src1<imm) \
      R(rd) = 1; \
    else \
      R(rd) = 0;);
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt    , R, \
    if ((sword_t)src1 < (sword_t)src2) \
      R(rd) = 1; \
    else \
      R(rd) = 0;);  


  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne    , B, \
    if ((sword_t)src1 != (sword_t)src2) { \
        s->dnpc = (s->pc + imm) & ~1; \
    }); 
  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq    , B, \
  if ((sword_t)src1 == (sword_t)src2) { \
      s->dnpc = (s->pc + imm) & ~1; \
  }); 
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge    , B, \
  if ((sword_t)src1 >= (sword_t)src2) { \
      s->dnpc = (s->pc + imm) & ~1; \
  };); 
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt    , B, \
  if ((sword_t)src1 < (sword_t)src2) { \
      s->dnpc = (s->pc + imm) & ~1; \
  };); //printf("rs1: %3d,  rs2: %3d, \n",src1,src2);


  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}
