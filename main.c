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



int maxinstr = 1;
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
ushort arg1;
ushort ir;
ushort ALUresult;
ushort ALUfunc;
ushort tgt;


int main(int argc,char *argv[])
{
  int vcycle;     //virtual cycle to allow signals to settle
  init();

  // Main execution loop
  while(clk.instr <= maxinstr) {
    clearsig();
    for(clk.icycle = FETCH; clk.icycle <= MEM; clk.icycle++) {
      for(clk.phase=clk_FE; clk.phase<= clk_RE; clk.phase++){
        printf("------------------------------------------------------\n");
        printf("cycle: %d.%d, phase: %d, PC: %x, MAR: %x\n", clk.instr, clk.icycle, clk.phase, regfile[PC], sysreg[MAR]);

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
          case EXECUTE:
            //execsigs();
            break;
          case MEM:
            memsigs();
            break;
          }
          resolvemux();
          printf("\n");
        }
        printf("Stable after %d vcycles.\n", vcycle);
        printf("regfile: r1:%s, r2:%s, w:%s\n", REGFILE_STR[regsel[REGR0S]], REGFILE_STR[regsel[REGR1S]],REGFILE_STR[regsel[REGWS]]);
        printf("ALU: 0:%s(%d) 1:%s(%d) func:%s out:%d\n", BSIG_STR[bussel[OP0S]], bsig[OP0], BSIG_STR[bussel[OP1S]], bsig[OP1], ALUopc, bsig[ALUout]);
      // Latch pass - single pass!
      latch(clk.phase);
      }
    }
    clk.instr++;
  }

  // Dump lower part of RAM
  printf("----------------------------RAM-----------------------------\n");
  int i;
  for(i=0; i<11; i++){
    printf("0x%03x: %04x\n", i, ram[i]);
  }
}

void
init(void) {

  bsig[PC] = 0;

  // initialize RAM with first program
  ram[0]=0xA011;
  ram[1]=0xA01a;
  //ram[2]=0xc653;
  //ram[3]=0x4283;
  //ram[4]=0x284;
  //ram[5]=0xE451;

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
  char opc2_b;
  char opc3_b[2];
  char opc4_b[3];
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

  memcpy(ALUopc, instr_b+3, 3);

  memcpy(opc1_b, instr_b, 3);
  opc1 = bin3_to_dec(opc1_b);

  opc2_b = instr_b[3];
  opc2 = opc2_b - '0';

  memcpy(opc3_b, instr_b+4, 2);
  opc3 = bin2_to_dec(opc3_b);


  memcpy(opc4_b, instr_b+4, 3);
  opc4 = bin3_to_dec(opc4_b);

  // parse arguments - immediates
  memcpy(imm7_b, instr_b+3, 7);
  imm7= bin7_to_dec(imm7_b);

  memcpy(imm10_b, instr_b+3, 10);
  imm10 = bin10_to_dec(imm10_b);

  memcpy(imm13_b, instr_b+3, 13);
  imm13 = bin13_to_dec(imm13_b);
  //printf("imm2: %s", imm2_b);

  memcpy(imm3_b, instr_b+7, 3);
  imm3 = bin3_to_dec(imm3_b);

  // parse arguments - operands
  memcpy(arg0_b, instr_b+7, 3);
  arg0 = bin3_to_dec(arg0_b);

  memcpy(arg1_b, instr_b+10, 3);
  arg1 = bin3_to_dec(arg1_b);

  memcpy(dest_b, instr_b+13, 3);
  tgt = bin3_to_dec(dest_b);

  ir_b = instr_b[6];
  //printf("i/r: %c", ir_b);
  ir = ir_b - '0';


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
      update_bsig(IRimm, &imm3);
      break;
    case 7:
      if(opc3 == 1) {
        update_bsig(IRimm, &imm3);
      }
  }

  // Set op0 and op1 mux and bus (combinational, not latched)
  //printf("OPC1: %d", opc1);
  switch(opc1) {
  case 5:
    // ldi s10, tgt
    // defaults: op1(REG0), f(add)
    update_bussel(MDRS, IRimm);
    update_csig(MDR_LOAD, HI);
    update_bussel(OP0S, MDRout);
    break;
  }
  // case 0:
//     update_opsel(2, MDR); // 7bit immediate
//     update_opsel(3, arg1);
//     update_opsel(0, MDR);
//     update_opsel(1, REG0);
//     update_csig(MAR_SEL, HI);
//     update_csig(MAR_LOAD, HI);
//     update_csig(MDR_LOAD, HI);
//     update_csig(MDR_SEL, HI);
//     update_rfsel(dest);
//     update_csig(RF_LOAD, HI);
//     break;
//   case 1:
//   case 2:
//     update_opsel(2, MDR); // 7bit immediate
//     update_opsel(3, arg1);
//     update_opsel(0, dest);
//     update_opsel(1, REG0);
//     update_csig(MAR_SEL, HI);
//     update_csig(MAR_LOAD, HI);
//     update_csig(MDR_LOAD, HI);
//     update_csig(RAM_LOAD, HI);
//     break;
//   case 3:
//     break;
//   case 4:
//     update_opsel(0, MDR);
//     update_opsel(1, REG0);
//     update_rfsel(dest);
//     update_csig(RF_LOAD, HI);
//     break;
//   case 5:
//     update_opsel(0, MDR);
//     update_opsel(1, REG0);
//     update_csig(PC_LOAD, HI);
//     // op_sel: MDR, RF, MAR, FLAGS
//     break;
//   case 6:
//     if(ir == 1) {
//       update_opsel(0, MDR);
//     } else {
//       //printf("IR IS ZERO!!");
//       update_opsel(0, arg0);
//     }
//     update_opsel(1, arg1);
//     update_rfsel(dest);
//     update_csig(MDR_SEL, HI);
//     update_csig(RF_LOAD, HI);
//     break;
//   }
//if (opc1 == 7 && opc2 == 0) {
  //   switch(opc3) {
  //     case 1:
  //       // skip.c
  //       update_opsel(0, arg0);
  //       update_opsel(1, arg1);
  //       strcpy(ALUfunc, "101"); // subtract
  //       skipcond = 1;
  //       break;
  //   }
  // }
//
//
  // REGWS always points to tgt?
  update_regsel(REGWS, tgt);

  if(opc1 < 6) {
    strcpy(ALUopc, "000");
  }
  ALUfunc = bin3_to_dec(ALUopc);
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

void
memsigs() {
  // Set the "destination muxes"; destination is always a reg (?)
  update_regsel(REGWS, tgt);
}

void resolvemux(void) {
  // Connect the muxed busses
  update_bsig(REGR0, &regfile[regsel[REGR0S]]);
  update_bsig(REGR1, &regfile[regsel[REGR1S]]);
  update_bsig(OP0, &bsig[bussel[OP0S]]);
  update_bsig(OP1, &bsig[bussel[OP1S]]);
  update_bsig(MDRin, &bsig[bussel[MDRS]]);
  update_bsig(MDRout, &sysreg[MDR]);  // programming crutch: MDRout == MDR
}

void
latch(enum phase clk_phase) {

  switch(clk.icycle) {
  case FETCH:
    if(clk_phase == clk_FE) {
      if(csig[MAR_LOAD]==HI) {
        sysreg[MAR] = bsig[ALUout];
        printf("MAR <- %x\n", sysreg[MAR]);
      }
    }

    if(clk_phase == clk_RE) {
      sysreg[IR] = readram(sysreg[MAR]);
      printf("IR <- %x\n", readram(sysreg[MAR]));
      regfile[PC]++; // PC acts as counter
    }
    break;
  case DECODE:
    if(clk_phase == clk_FE) {
      if(csig[MDR_LOAD]==HI) {
        sysreg[MDR] = bsig[MDRin];
        printf("MDR <- %x\n", sysreg[MDR]);
      }
    }
    break;
  case MEM:
    if(clk_phase == clk_RE) {
      writeregfile();
    }
    break;
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

  switch(ALUfunc) {
  case 0:
    result = bsig[OP0] + bsig[OP1];
    break;
  }
  update_bsig(ALUout, &result);
}
//
// ushort
// addradder(ushort x, ushort y) {
//   return x + y;
// }
//
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
    printf("%s(%s) ", BUSSEL_STR[signame], BSIG_STR[value]);
    //if (signame == AIN)
      //printf("v: %x", *value);
  }
  bussel[signame] = value;
}

void update_regsel(enum regsel signame, ushort value) {
  //printf("signame: %d", signame);
  if (regsel[signame] != value) {
    sigupd = 1;
    printf("%s(%s) ", REGSEL_STR[signame], REGFILE_STR[value]);
    //if (signame == AIN)
      //printf("v: %x", *value);
  }
  regsel[signame] = value;
}
//
// int update_opsel(int opnr, int value) {
//   //printf("signame: %d", signame);
//   int *org;
//   switch(opnr){
//     case 0:
//     org = &op0_sel;
//     break;
//     case 1:
//     org = &op1_sel;
//     break;
//     case 2:
//     org = &op2_sel;
//     break;
//     case 3:
//     org = &op3_sel;
//     break;
//   }
//   if (*org != value) {
//     updated = 1;
//     printf("op%d (%s), ", opnr, BSIG_STRING[value]);
//     //if (signame == AIN)
//       //printf("v: %x", *value);
//   }
//   *org = value;
// }
//
// int update_rfsel(int value) {
//   if (rf_sel != value) {
//     updated = 1;
//     printf("rf_sel (%d), ", value);
//   }
//   rf_sel = value;
// }
//
//
// ushort readrom(ushort addr) {
//   //printf("Reading ROM at address: %x\n", addr);
//   return rom[addr];
// }
//
ushort readram(ushort addr) {
  //printf("Reading RAM at address: %x\n", addr);
  return ram[addr];
}
//
// void writeram() {
//   ram[bsig[MAR]] = bsig[MDR];
//   printf("RAM[%x] <- %x\n", bsig[MAR], bsig[MDR]);
// }
//
//
void writeregfile(void) {
  regfile[regsel[REGWS]] = bsig[ALUout];
  printf("%s <- %x\n", REGFILE_STR[regsel[REGWS]], bsig[ALUout]);
}
//
void
clearsig(){
  memset(csig, 0, sizeof(csig));
  memset(bsig, 0, sizeof(bsig));
  memset(bussel, 0, sizeof(bussel));
  memset(regsel, 0, sizeof(regsel));
  ALUfunc = 0;
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

ushort readrfile(int port, int reg) {

}
