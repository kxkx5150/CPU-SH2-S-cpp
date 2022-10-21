
#ifndef CS1_H
#define CS1_H

#include "cs0.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

u8 FASTCALL   Cs1ReadByte(SH2_struct *context, u8 *, u32);
u16 FASTCALL  Cs1ReadWord(SH2_struct *context, u8 *, u32);
u32 FASTCALL  Cs1ReadLong(SH2_struct *context, u8 *, u32);
void FASTCALL Cs1WriteByte(SH2_struct *context, u8 *, u32, u8);
void FASTCALL Cs1WriteWord(SH2_struct *context, u8 *, u32, u16);
void FASTCALL Cs1WriteLong(SH2_struct *context, u8 *, u32, u32);

#ifdef __cplusplus
}
#endif

#endif
