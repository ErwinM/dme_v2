//C hello world example
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "defs.h"
#include "arch.h"
#include "types.h"


struct {
  int instr;
  enum icycle icycle;
  enum phase phase;
} clk;


enum signalstate csig[16] = {0};
ushort bsig[20] = {0};
ushort ram[1024] = {0};
ushort regfile[16] = {0};
ushort sysreg[3] = {0};
ushort regsel[3] = {0};
ushort bussel[10] = {0};




int maxinstr = 10;
int sigupd;
char ALUopc[4];

ushort instr;
ushort opc1;
ushort opc2;
ushort opc3;
ushort opc4;
ushort imm7;
ushort imm10;
ushort imm13;
ushort imm3;
ushort arg0;
ushort arg0imm;
ushort arg1;
ushort ir;
ushort ALUresult;
ushort tgt;


int main(int argc,char *argv[])
{
  int vcycle;     //virtual cycle to allow signals to settle
  init();

  // Main execution loop
  while(clk.instr <= maxinstr) {
    for(clk.icycle = FETCH; clk.icycle <= EXECUTE; clk.icycle++) {
      clearsig();
      for(clk.phase=clk_FE; clk.phase<= clk_RE; clk.phase++){
        ushort temp = readram(sysreg[MAR]);
        printf("------------------------------------------------------\n");
        printf("cycle: %d.%d, phase: %d, PC: %x, MAR: %x(%x)\n", clk.instr, clk.icycle, clk.phase, regfile[PC], sysreg[MAR], temp);

        // signal generation phase
        sigupd=1;
        vcycle=0;
        while(sigupd==1) {
          if(vcycle > 15) {
            printf("ERROR: signal does not stabilize!\n");
            exit(1);
          }
          sigupd = 0;
          vcycle++;
          switch(clk.icycle){
          case FETCH:
            fetchsigs();
            ALU();
            break;
          case DECODE:
            decodesigs();
            ALU();
            break;
          case READ:
            readsigs();
            ALU();
            break;
          case EXECUTE:
            execsigs();
            ALU();
            break;
          }
          resolvemux();
          printf("\n");
        }
        printf("Stable after %d vcycles.\n", vcycle);
        printf("regfile: r0:%s, r1:%s, w:%s\n", REGFILE_STR[regsel[REGR0S]], REGFILE_STR[regsel[REGR1S]],REGFILE_STR[regsel[REGWS]]);
        printf("ALU: 0:%s(%x) 1:%s(%x) func:%s out:%x\n", BSIG_STR[bussel[OP0S]], bsig[OP0], BSIG_STR[bussel[OP1S]], bsig[OP1], ALUopc, bsig[ALUout]);
      // Latch pass - single pass!
      latch(clk.phase);
      }
    }
    clk.instr++;
  }
  dump();
}

void dump() {
  // Dump lower part of RAM and regs
  printf("----------------------------RAM-----------------------------\n");
  int i;
  for(i=0; i<11; i++){
    printf("0x%03x: %04x\n", i, ram[i]);
  }
  printf("----------------------------REGISTERS-----------------------\n");
  for(i=0; i<7; i++){
    printf("R%d: %04x\n", i, regfile[i]);
  }
}

void
init(void) {

  bsig[PC] = 0;

  // initialize RAM with first program
  //ram[0]=0xA011;
  // ram[0]=0xA322; // ldi 100, RB
//   ram[1]=0x1D94; // ldw -10(RA), RD
//   ram[2]=0xc3d5; // add 7, RB, RE
//   //ram[3]=0x8003; // br 4 (jump to 7)
//   ram[3]=0xa021; // ldi 4, RA
//   ram[4]=0xd0ac; // skip.c RB, RE, SGT
//   ram[90]=0xdead;
  //ram[3]=0x4283;
  //ram[4]=0x284;
  //ram[5]=0xE451;

  ram[0] = 0xa031;
  ram[2] = 0xa052;
  ram[4] = 0xd054;
  ram[6] = 0x8002;
  ram[8] = 0xc00b;
  ram[10] = 0x0;
  ram[12] = 0xc013;
  ram[14] = 0x0;

  printf("Init done..\n");
}

void fetchsigs()
{
  // FETCH LOGIC
  // defaults: OP1 defaults to reg0(0) and ALUopc to 0(add)
  update_regsel(REGR1S, PC);
  update_bussel(OP0S, REGR1);
  update_csig(MAR_LOAD, HI);
}

void
decodesigs() {

  char *instr_b;
  char opc1_b[3];
  char opc2_b[3];
  //char opc3_b[2];
  //char opc4_b[3];
  char imm7_b[7];
  char imm10_b[10];
  char imm13_b[13];
  char imm3_b[3];
  char arg0_b[3];
  char arg1_b[3];
  char dest_b[3];
  char ir_b;

  int skipcond = 0;

  printf("IR: %x ", sysreg[IR]);
  // parse instruction
  instr_b = decimal_to_binary16(sysreg[IR]);
  printf("(%s), ", instr_b);

  memcpy(opc1_b, instr_b, 3);
  opc1 = bin3_to_dec(opc1_b);

  memcpy(opc2_b, instr_b+3, 3);
  opc2 = bin2dec(opc2_b, 3);

  //memcpy(opc4_b, instr_b+4, 3);
  //opc4 = bin3_to_dec(opc4_b);

  // parse arguments - immediates
  memcpy(imm7_b, instr_b+3, 7);
  imm7= sbin2dec(imm7_b, 7);

  memcpy(imm10_b, instr_b+3, 10);
  imm10 = sbin2dec(imm10_b, 10);

  memcpy(imm13_b, instr_b+3, 13);
  imm13 = sbin2dec(imm13_b, 13);
  //printf("imm2: %s", imm2_b);

  memcpy(imm3_b, instr_b+7, 3);
  imm3 = bin3_to_dec(imm3_b);

  // parse arguments - operands
  memcpy(arg0_b, instr_b+7, 3);
  arg0 = bin3_to_dec(arg0_b);

  // this imm has a bespoke encoding
  arg0imm = immtable[arg0];

  memcpy(arg1_b, instr_b+10, 3);
  arg1 = bin3_to_dec(arg1_b);
  printf(">>%s<<", arg1_b);

  memcpy(dest_b, instr_b+13, 3);
  tgt = bin3_to_dec(dest_b);

  ir_b = instr_b[6];
  //printf("i/r: %c", ir_b);
  ir = ir_b - '0';


  // byte encoding
  // if(instr_b[15]=='0'){
//     // aligned (even number)
//
//
//   }

  // IRimm MUX - which bits from IR should feed IRimm
  switch(opc1) {
    case 0:
    case 1:
    case 2:
    case 3:
      update_bsig(IRimm, &imm7);
      break;
    case 4:
      update_bsig(IRimm, &imm13);
      break;
    case 5:
      update_bsig(IRimm, &imm10);
      break;
    case 6:
      if(ir) {
        update_bsig(IRimm, &arg0imm);
        //printf("IRimm: %d, IRmmtable: %d", arg0, arg0imm);
      } else {
        update_bsig(IRimm, &imm3);
      }
      break;
    case 7:
      if(opc3 == 1) {
        update_bsig(IRimm, &imm3);
      }
  }

  // Set op0 and op1 mux and bus (combinational, not latched)
  //printf("OPC1: %d", opc1);
  switch(opc1) {
  case 0:
    // ldw sw7(base), tgt
    update_bussel(MDRS, IRimm);
    update_csig(MDR_LOAD, HI);
    break;
  case 4:
    update_bussel(MDRS, IRimm);
    update_csig(MDR_LOAD, HI);
    break;
  case 5:
    // ldi s10, tgt
    // defaults: op1(REG0), f(add)
    update_bussel(MDRS, IRimm);
    update_csig(MDR_LOAD, HI);
    update_bussel(OP0S, MDRout);
    break;
  case 6:
    switch(opc2) {
    case 0 ... 5:
      if(ir==1) {
        // arg1 is an imm
        update_bussel(MDRS, IRimm);
        update_csig(MDR_LOAD, HI);
      }
      break;
    }
    break;
  }

  // REGWS always points to tgt?
  update_regsel(REGWS, tgt);

}
  // // Condition testing
 //  // 000 EQ, 001 NEQ
 //  if (skipcond == 1) {
 //    switch(dest) {
 //      case 0:
 //        if(flags[ZR] == HI)
 //          update_csig(SKIP, HI);
 //        break;
 //      case 1:
 //        if(flags[ZR] != HI)
 //          update_csig(SKIP, HI);
 //        break;
 //
 //    }
 // }

void readsigs(void) {
  // Set op0 and op1 mux and bus (combinational, not latched)
  //printf("OPC1: %d", opc1);
  switch(opc1) {
  case 0:
    update_regsel(REGR0S, arg1);
    update_bussel(OP0S, MDRout);
    update_bussel(OP1S, REGR0);
    update_csig(MAR_LOAD, HI);
    // latched on RE:
    update_bussel(MDRS, RAM);
    update_csig(MDR_LOAD, HI);
    break;
  }
}


void
execsigs() {
  // Set the "destination muxes"; destination is always a reg (?)
  // one exception? if more then integrate into switch statement below
  if(opc1==4){
    update_regsel(REGWS, PC);
  } else {
  update_regsel(REGWS, tgt);
  }
  //printf("opc1: %d ", opc1);
  switch(opc1) {
  case 0:
    update_bussel(OP1S, MDRout);
    update_csig(REG_LOAD, HI);
    break;
  case 4:
    update_bussel(OP0S, MDRout);
    update_bussel(OP1S, REGR0);
    update_regsel(REGR0S, PC);
    update_csig(REG_LOAD, HI);
    break;
  case 5:
    // ldi s10, tgt
    // defaults: op1(REG0), f(add)
    update_bussel(OP0S, MDRout);
    update_csig(REG_LOAD, HI);
    break;
  case 6:
  //printf("opc2: %d", opc2);
    switch(opc2) {
    case 0 ... 3:
      if(ir==1) {
        // arg1 is an imm
        update_bussel(OP0S, MDRout);
      } else {
        update_bussel(OP0S, REGR0);
        update_regsel(REGR0S, arg0);
      }
      update_bussel(OP1S, REGR1);
      update_regsel(REGR1S, arg1);
      update_csig(REG_LOAD, HI);
      break;
    case 4:
      if(ir==1) {
        // arg1 is an imm
        update_bussel(OP0S, MDRout);
      } else {
        update_bussel(OP0S, REGR0);
        update_regsel(REGR0S, arg0);
      }
      update_bussel(OP1S, REGR1);
      update_regsel(REGR1S, arg1);
      update_bussel(ALUS, 1);
      update_bussel(COND, tgt);
      chkskip();
      //if(bsig[ALUout]==0) {
      //  update_csig(SKIP, HI);
      //}
      break;

    case 5:
      if(ir==1) {
        // arg1 is an imm
        update_bussel(OP0S, MDRout);
      } else {
        update_bussel(OP0S, REGR0);
        update_regsel(REGR0S, arg0);
      }
      update_bussel(OP1S, REGR1);
      update_regsel(REGR1S, arg1);
      update_csig(REG_LOAD, HI);
      if(bsig[ALUout]==0) {
        update_csig(SKIP, HI);
      }
      break;
    }
    break;
  }
}

void resolvemux(void) {
  // Connect the muxed busses
  update_bsig(REGR0, &regfile[regsel[REGR0S]]);
  update_bsig(REGR1, &regfile[regsel[REGR1S]]);
  update_bsig(OP0, &bsig[bussel[OP0S]]);
  update_bsig(OP1, &bsig[bussel[OP1S]]);
  update_bsig(MDRin, &bsig[bussel[MDRS]]);
  update_bsig(MDRout, &sysreg[MDR]);  // programming crutch: MDRout == MDR

  ushort temp = readram(sysreg[MAR]);
  update_bsig(RAM, &temp);
}

void
latch(enum phase clk_phase) {


  // TO DO GET RID OF THIS TOO
  if(clk.icycle == FETCH && clk_phase == clk_RE) {
    sysreg[IR] = readram(sysreg[MAR]);
    printf("IR <- %x\n", readram(sysreg[MAR]));
    regfile[PC]+=2; // PC acts as counter
  }

  // FALLING EDGE LATCHES
  if(clk_phase == clk_FE) {
    if(csig[MAR_LOAD]==HI) {
      sysreg[MAR] = bsig[ALUout];
      printf("MAR <- %x\n", sysreg[MAR]);
    }
    if(csig[SKIP]==HI) {
      regfile[PC] +=2;
      printf("PC++");
    }
  }

  // RISING EDGE LATCHES
  if(clk_phase == clk_RE) {
    if(csig[MDR_LOAD]==HI) {
      sysreg[MDR] = bsig[MDRin];
      printf("MDR <- %x\n", sysreg[MDR]);
    }
    if(csig[REG_LOAD]==HI) {
      writeregfile();
    }
  }
}
  // FETCH



  // DECODE
  // if(clk.icycle == DECODE && clk_phase == clk_RE) {
  //   bsig[MDR] = bsig[MDRin];                      // in decode mdr can always point to imm?
  //   printf("MDR <- %x\n", bsig[MDRin]);
  // }

  // if(clk.icycle == DECODE && clk_phase == clk_RE) {
//     if(csig[MAR_LOAD] == HI) {
//       bsig[MAR] = bsig[MARin];
//       printf("MAR <- %x\n", bsig[MAR]);
//     }
//   }


  // EXECUTE
  //
  // if(clk.icycle == EXECUTE && clk_phase == clk_FE) {
//     if(1) { // csig[MDR_LOAD] == HI
//       bsig[MDR] = bsig[MDRin];
//       printf("MDR <- %x\n", bsig[MDR]);
//     }
//     if(csig[PC_LOAD] == HI) {
//       bsig[PC] = bsig[ALUout];
//       printf("PC <- %x\n", bsig[ALUout]);
//     }
//     if(csig[SKIP] == HI) {
//       bsig[PC]++;
//       printf("PC <- skip\n");
//     }
//   }
// }
//
  // MEM
  // write/read to regfile or RAM
 //  if(clk.icycle == MEM && clk_phase == clk_RE)
//     writeregfile();
// }
//
//
//
//
//
//
//
void
ALU(void) {
  ushort result;

  switch(bussel[ALUS]) {
  case 0:
    result = bsig[OP0] + bsig[OP1];
    break;
  case 1:
    result = bsig[OP0] - bsig[OP1];
    break;
  }
  update_bsig(ALUout, &result);
}

void chkskip(void){
// condS  cond  0  neg  pos
// SLTEQ    3   1  1    0
// EQ       0   1  0    0
// SGTEQ    5   1  0    1
// NEQ      1   0  1    1
// SLT      2   0  1    0
// SGT      4   0  0    1
  // if its 0
  if(bsig[ALUout] == 0 && bussel[COND] == 3) { update_csig(SKIP, HI); }
  if(bsig[ALUout] == 0 && bussel[COND] == 0) { update_csig(SKIP, HI); }
  if(bsig[ALUout] == 0 && bussel[COND] == 5) { update_csig(SKIP, HI); }
  // if its neg - in HDL need to check most significant bit[0]
  if(bsig[ALUout] < 0 && bussel[COND] == 3) { update_csig(SKIP, HI); }
  if(bsig[ALUout] < 0 && bussel[COND] == 1) { update_csig(SKIP, HI);}
  if(bsig[ALUout] < 0 && bussel[COND] == 2) { update_csig(SKIP, HI);}
  // if its pos - in HDL need to check most significant bit[0]
  if(bsig[ALUout] > 0 && bussel[COND] == 5) { update_csig(SKIP, HI); }
  if(bsig[ALUout] > 0 && bussel[COND] == 1) { update_csig(SKIP, HI); }
  if(bsig[ALUout] > 0 && bussel[COND] == 4) { update_csig(SKIP, HI);}
}


void update_csig(enum csig signame, enum signalstate state) {
  if (csig[signame] != state) {
    sigupd = 1;
    printf("%s ", CSIG_STR[signame]);
  }
  csig[signame] = state;
}

void update_bsig(int signame, ushort *value) {
  //printf("signame: %d", signame);
  if (bsig[signame] != *value) {
    sigupd = 1;
    printf("%s ", BSIG_STR[signame]);
    //if (signame == AIN)
      //printf("v: %x", *value);
  }
  bsig[signame] = *value;
}

void update_bussel(enum bussel signame, ushort value) {
  //printf("signame: %d", signame);
  if (bussel[signame] != value) {
    sigupd = 1;
    switch(signame){
    case 0 ... 2:
      printf("%s(%s) ", BUSSEL_STR[signame], BSIG_STR[value]);
      break;
    case 3:
      printf("%s(%s) ", BUSSEL_STR[signame], ALUFUNC_STR[value]);
      break;
    case 4:
      printf("%s(%s) ", BUSSEL_STR[signame], COND_STR[value]);
      break;
    //if (signame == AIN)
      //printf("v: %x", *value);
    }
  }
  bussel[signame] = value;
}

void update_regsel(enum regsel signame, ushort value) {
  //printf("signame: %d", signame);
  if (regsel[signame] != value) {
    sigupd = 1;
    printf("%s(%s) ", REGSEL_STR[signame], REGFILE_STR[value]);
  }
  regsel[signame] = value;
}

ushort readram(ushort addr) {
  int b1, b2, woord;

  woord = ram[addr];
  // b2 = ram[addr+1];
 //  woord = (b1 << 8 ) | b2;
  //printf("b1: %x, b2: %x, readram: %x", b1, b2,woord);
  return woord;
}
//
// void writeram() {
//   ram[bsig[MAR]] = bsig[MDR];
//   printf("RAM[%x] <- %x\n", bsig[MAR], bsig[MDR]);
// }
//
//
void writeregfile(void) {
  if(regsel[REGWS] == REG0) {
    printf("\nwriteregfile: error trying to write to REG0!!\n");
    dump();
    exit(1);
  }
  regfile[regsel[REGWS]] = bsig[ALUout];
  printf("%s <- %x\n", REGFILE_STR[regsel[REGWS]], bsig[ALUout]);
}
//
void
clearsig(){
  memset(csig, 0, sizeof(csig));
  memset(bussel, 0, sizeof(bussel));
  memset(regsel, 0, sizeof(regsel));
}
//
// void setconsole(enum clkstate phase, int vflag) {
//   if (vflag)
//     return;
//
//   if (phase != clk_RE ) {
//     fflush(stdout);
//     nullOut = fopen("/dev/null", "w");
//     dup2(fileno(nullOut), 1);
//   } else {
//     fflush(stdout);
//     fclose(nullOut);
//     // Restore stdout
//     dup2(stdoutBackupFd, 1);
//     //close(stdoutBackupFd);
//   }
// }
