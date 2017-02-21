#ifndef UNITS_H_   /* Include guard */
#define UNITS_H_

#include "types.h"

enum signalstate { ZZ, RE, HI, FE, LO };


#define PHASE \
      X(clk_FE) \
      X(clk_RE)

#define ICYCLE \
      X(FETCH) \
      X(DECODE) \
      X(READ) \
      X(EXECUTE)

#define CSIGS \
       X(MAR_LOAD  ) \
       X(IR_LOAD   ) \
       X(MDR_LOAD ) \
       X(REG_LOAD  ) \
       X(RAM_LOAD  ) \
       X(SKIP   )

#define REGFILE \
      X(REG0  ) \
      X(REGA   ) \
      X(REGB   ) \
      X(REGC ) \
      X(REGD  ) \
      X(REGE  ) \
      X(SP  ) \
      X(PC  ) \
      X(FLAGS)

#define SYSREG \
      X(MAR) \
      X(MDR) \
      X(IR)

#define REGSEL \
      X(REGR0S  ) \
      X(REGR1S   ) \
      X(REGWS   )

// Bussel can only select from busses never from registers
// thus, their value indexes the BSIG array
#define BUSSEL \
      X(OP0S) \
      X(OP1S) \
      X(MDRS)

#define BSIGS \
      X(REGR0  ) \
      X(REGR1  ) \
      X(OP0   ) \
      X(OP1   ) \
      X(IRimm   ) \
      X(MDRin   ) \
      X(MARin   ) \
      X(ALUout   ) \
         X(RAM) \
      X(MDRout   )  // programming crutch always equals SYSREG[MDR]

enum flags { ZR, NG};
static const char *FLAGS_STRING[] = { "ZR", "NG"};






// code to generate enums and string arrays for CSIG and BSIG
// from: http://stackoverflow.com/questions/147267/easy-way-to-use-variables-of-enum-types-as-string-in-c/29561365#29561365
#define MACROSTR(k) #k

enum regfile {
#define X(Enum)       Enum,
    REGFILE
#undef X
};

static char *REGFILE_STR[] = {
#define X(String) MACROSTR(String),
    REGFILE
#undef X
};

enum bsig {
#define X(Enum)       Enum,
    BSIGS
#undef X
};

static char *BSIG_STR[] = {
#define X(String) MACROSTR(String),
    BSIGS
#undef X
};

enum phase {
#define X(Enum)       Enum,
    PHASE
#undef X
};

static char *PHASE_STR[] = {
#define X(String) MACROSTR(String),
    PHASE
#undef X
};

enum icycle {
#define X(Enum)       Enum,
  ICYCLE
#undef X
};

static char *ICYCLE_STR[] = {
#define X(String) MACROSTR(String),
  ICYCLE
#undef X
};

enum csig {
#define X(Enum)       Enum,
    CSIGS
#undef X
};

static char *CSIG_STR[] = {
#define X(String) MACROSTR(String),
    CSIGS
#undef X
};

enum bussel {
#define X(Enum)       Enum,
    BUSSEL
#undef X
};

static char *BUSSEL_STR[] = {
#define X(String) MACROSTR(String),
    BUSSEL
#undef X
};

enum regsel {
#define X(Enum)       Enum,
    REGSEL
#undef X
};

static char *REGSEL_STR[] = {
#define X(String) MACROSTR(String),
    REGSEL
#undef X
};

enum sysreg {
#define X(Enum)       Enum,
    SYSREG
#undef X
};

static char *SYSREG_STR[] = {
#define X(String) MACROSTR(String),
    SYSREG
#undef X
};

#endif
