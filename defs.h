#include "types.h"
#include "arch.h"
char *decimal_to_binary16(int n);
int getbit(char *bitstring, int bitnr);
int bin2dec(char *bin);
int sbin2dec(char *bin);
int bin2_to_dec(char *bin);
int bin3_to_dec(char *bin);
int bin7_to_dec(char *bin);
int bin10_to_dec(char *bin);
int bin13_to_dec(char *bin);
int getbit6(char *bitstring, int bitnr);
int getbit16(char *bitstring, int bitnr);
void init(void);
ushort readram(ushort addr);
void update_csig(enum csig signame, enum signalstate state);
void update_bsig(int signame, ushort *value);
void update_bussel(enum bussel, ushort value);
void update_regsel(enum regsel signame, ushort value);
void dosignals(void);
void latch(enum phase);
void writeregfile(void);
void ALU();
void clearsig(void);
void fetchsigs(void);
void decodesigs(void);
void readsigs(void);
void execsigs(void);
void resolvemux(void);
