

#include <stdlib.h>
#include "cs1.h"
#include "cs0.h"

u8 FASTCALL Cs1ReadByte(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    addr &= 0xFFFFFF;
    return CartridgeArea->Cs1ReadByte(context, memory, addr);
}

u16 FASTCALL Cs1ReadWord(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    addr &= 0xFFFFFF;
    return CartridgeArea->Cs1ReadWord(context, memory, addr);
}

u32 FASTCALL Cs1ReadLong(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    addr &= 0xFFFFFF;
    return CartridgeArea->Cs1ReadLong(context, memory, addr);
}

void FASTCALL Cs1WriteByte(SH2_struct *context, UNUSED u8 *memory, u32 addr, u8 val)
{
    addr &= 0xFFFFFF;

    if (addr == 0xFFFFFF)
        return;

    CartridgeArea->Cs1WriteByte(context, memory, addr, val);
}

void FASTCALL Cs1WriteWord(SH2_struct *context, UNUSED u8 *memory, u32 addr, u16 val)
{
    addr &= 0xFFFFFF;

    if (addr == 0xFFFFFE)
        return;

    CartridgeArea->Cs1WriteWord(context, memory, addr, val);
}

void FASTCALL Cs1WriteLong(SH2_struct *context, UNUSED u8 *memory, u32 addr, u32 val)
{
    addr &= 0xFFFFFF;

    if (addr == 0xFFFFFC)
        return;

    CartridgeArea->Cs1WriteLong(context, memory, addr, val);
}
