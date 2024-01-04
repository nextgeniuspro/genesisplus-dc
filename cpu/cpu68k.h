#ifndef CPU68K_H
#define CPU68K_H

//#define CPU68K_USE_MUSASHI
#define CPU68K_USE_C68K

#define CPU68K_AUTOVECTOR_BASE_EX   24

#define CPU68K_INT_ACK_AUTOVECTOR   -1

typedef u32     FASTCALL CPU68K_READ(const u32 adr);
typedef void    FASTCALL CPU68K_WRITE(const u32 adr, u32 data);

typedef u32     FASTCALL CPU68K_INT_CALLBACK(u32 level);
typedef void    FASTCALL CPU68K_RESET_CALLBACK(void);

void M68K_GetContext(void);
void M68K_SetContext(void);

u32 M68K_GetCurrentCore(void);
void M68K_SetCurrentCore(u32 core);

void M68K_Init(void);
s32  M68K_Reset(void);

s32  M68K_Exec(s32 cycles);

void M68K_SetIRQ(u32 level);

u32  M68K_GetOdo(void);
void M68K_EndExec(void);

void M68K_SetFetch(u32 low_adr, u32 high_adr, u32 fetch_adr);

u32  M68K_GetDReg(u32 num);
u32  M68K_GetAReg(u32 num);
u32  M68K_GetSP(void);
u32  M68K_GetPC(void);
u32  M68K_GetSR(void);

void M68K_SetAReg(u32 num, u32 val);
void M68K_SetDReg(u32 num, u32 val);
void M68K_SetSP(u32 val);
void M68K_SetPC(u32 val);
void M68K_SetSR(u32 val);

u8  *M68K_Disassemble(u32 *PC);

#endif	/* CPU68K_H */
