

#include <stdlib.h>
#include "cs0.h"
#include "error.h"
#include "decrypt.h"

cartridge_struct *CartridgeArea;

static u8 decryptOn = 0;

#define LOGSTV

static u8 FASTCALL DummyCs0ReadByte(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr)
{
    return 0xFF;
}

static u16 FASTCALL DummyCs0ReadWord(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr)
{
    return 0xFFFF;
}

static u32 FASTCALL DummyCs0ReadLong(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr)
{
    return 0xFFFFFFFF;
}

static void FASTCALL DummyCs0WriteByte(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr, UNUSED u8 val)
{
}

static void FASTCALL DummyCs0WriteWord(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr, UNUSED u16 val)
{
}

static void FASTCALL DummyCs0WriteLong(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr, UNUSED u32 val)
{
}

static u8 FASTCALL DummyCs1ReadByte(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr)
{
    if (addr == 0xFFFFFF)
        return CartridgeArea->cartid;
    return 0xFF;
}

static u16 FASTCALL DummyCs1ReadWord(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr)
{
    if (addr == 0xFFFFFE)
        return (0xFF00 | CartridgeArea->cartid);
    return 0xFFFF;
}

static u32 FASTCALL DummyCs1ReadLong(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr)
{
    if (addr == 0xFFFFFC)
        return (0xFF00FF00 | (CartridgeArea->cartid << 16) | CartridgeArea->cartid);
    return 0xFFFFFFFF;
}

static void FASTCALL DummyCs1WriteByte(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr, UNUSED u8 val)
{
}

static void FASTCALL DummyCs1WriteWord(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr, UNUSED u16 val)
{
}

static void FASTCALL DummyCs1WriteLong(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr, UNUSED u32 val)
{
}

static u8 FASTCALL DummyCs2ReadByte(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr)
{
    return 0xFF;
}

static u16 FASTCALL DummyCs2ReadWord(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr)
{
    return 0xFFFF;
}

static u32 FASTCALL DummyCs2ReadLong(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr)
{
    return 0xFFFFFFFF;
}

static void FASTCALL DummyCs2WriteByte(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr, UNUSED u8 val)
{
}

static void FASTCALL DummyCs2WriteWord(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr, UNUSED u16 val)
{
}

static void FASTCALL DummyCs2WriteLong(SH2_struct *context, UNUSED u8 *memory, UNUSED u32 addr, UNUSED u32 val)
{
}

typedef enum
{
    FL_READ,
    FL_SDP,
    FL_CMD,
    FL_ID,
    FL_IDSDP,
    FL_IDCMD,
    FL_WRITEBUF,
    FL_WRITEARRAY
} flashstate;

u8 flreg0 = 0;
u8 flreg1 = 0;

u8 vendorid = 0x1F;
u8 deviceid = 0xD5;

flashstate flstate0;
flashstate flstate1;

u8 flbuf0[128];
u8 flbuf1[128];

static u8 FASTCALL FlashCs0ReadByte(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    flashstate *state;
    u8         *reg;

    if (addr & 1) {
        state = &flstate1;
        reg   = &flreg1;
    } else {
        state = &flstate0;
        reg   = &flreg0;
    }

    switch (*state) {
        case FL_ID:
        case FL_IDSDP:
        case FL_IDCMD:
            if (addr & 2)
                return deviceid;
            else
                return vendorid;
        case FL_WRITEARRAY:
            *reg ^= 0x02;
        case FL_WRITEBUF:
            return *reg;
        case FL_SDP:
        case FL_CMD:
            *state = FL_READ;
        case FL_READ:
        default:
            return T2ReadByte(memory, addr);
    }
}

static u16 FASTCALL FlashCs0ReadWord(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    return ((u16)(FlashCs0ReadByte(NULL, memory, addr) << 8) | (u16)(FlashCs0ReadByte(NULL, memory, addr + 1)));
}

static u32 FASTCALL FlashCs0ReadLong(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    return ((u32)FlashCs0ReadWord(NULL, memory, addr) << 16) | (u32)FlashCs0ReadWord(NULL, memory, addr + 2);
}

static void FASTCALL FlashCs0WriteByte(SH2_struct *context, UNUSED u8 *memory, u32 addr, u8 val)
{
    flashstate *state;
    u8         *reg;
    u8         *buf;

    if (addr & 1) {
        state = &flstate1;
        reg   = &flreg1;
        buf   = flbuf1;
    } else {
        state = &flstate0;
        reg   = &flreg0;
        buf   = flbuf0;
    }

    switch (*state) {
        case FL_READ:
            if (((addr & 0xfffe) == 0xaaaa) && (val == 0xaa))
                *state = FL_SDP;
            return;
        case FL_WRITEBUF:
            buf[(addr >> 1) & 0x7f] = val;
            if (((addr >> 1) & 0x7f) == 0x7f) {
                int i;
                int j = addr & 0x1;
                addr &= 0xffffff00;
                for (i = 0; i <= 127; i++) {
                    T2WriteByte(memory, (addr + i * 2 + j), buf[i]);
                }
                *state = FL_READ;
            }
            return;
        case FL_SDP:
            if (((addr & 0xfffe) == 0x5554) && (val == 0x55))
                *state = FL_CMD;
            else
                *state = FL_READ;
            return;
        case FL_ID:
            if (((addr & 0xfffe) == 0xaaaa) && (val == 0xaa))
                *state = FL_IDSDP;
            else
                *state = FL_ID;
            return;
        case FL_IDSDP:
            if (((addr & 0xfffe) == 0x5554) && (val == 0x55))
                *state = FL_READ;
            else
                *state = FL_ID;
            return;
        case FL_IDCMD:
            if (((addr & 0xfffe) == 0xaaaa) && (val == 0xf0))
                *state = FL_READ;
            else
                *state = FL_ID;
            return;
        case FL_CMD:
            if ((addr & 0xfffe) != 0xaaaa) {
                *state = FL_READ;
                return;
            }

            switch (val) {
                case 0xa0:
                    *state = FL_WRITEBUF;
                    return;
                case 0x90:
                    *state = FL_ID;
                    return;
                default:
                    *state = FL_READ;
                    return;
            }
        default:
            break;
    }
}

static void FASTCALL FlashCs0WriteWord(SH2_struct *context, UNUSED u8 *memory, u32 addr, u16 val)
{
    FlashCs0WriteByte(context, memory, addr, (u8)(val >> 8));
    FlashCs0WriteByte(context, memory, addr + 1, (u8)(val & 0xff));
}

static void FASTCALL FlashCs0WriteLong(SH2_struct *context, UNUSED u8 *memory, u32 addr, u32 val)
{
    FlashCs0WriteWord(context, memory, addr, (u16)(val >> 16));
    FlashCs0WriteWord(context, memory, addr + 2, (u16)(val & 0xffff));
}

static u8 FASTCALL AR4MCs0ReadByte(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    addr &= 0x1FFFFFF;

    switch (addr >> 20) {
        case 0x00: {
            if ((addr & 0x80000) == 0)
                return FlashCs0ReadByte(NULL, memory, addr);
            break;
        }
        case 0x01: {
            break;
        }
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            return T1ReadByte(CartridgeArea->dram, addr & 0x3FFFFF);
        default:
            break;
    }

    return 0xFF;
}

static u16 FASTCALL AR4MCs0ReadWord(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    addr &= 0x1FFFFFF;

    switch (addr >> 20) {
        case 0x00: {
            if ((addr & 0x80000) == 0)
                return FlashCs0ReadWord(NULL, memory, addr);
            break;
        }
        case 0x01: {
            break;
        }
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            return T1ReadWord(CartridgeArea->dram, addr & 0x3FFFFF);
        case 0x12:
        case 0x1E:
            if (0x80000)
                return 0xFFFD;
            break;
        case 0x13:
        case 0x16:
        case 0x17:
        case 0x1A:
        case 0x1B:
        case 0x1F:
            return 0xFFFD;
        default:
            break;
    }

    return 0xFFFF;
}

static u32 FASTCALL AR4MCs0ReadLong(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    addr &= 0x1FFFFFF;

    switch (addr >> 20) {
        case 0x00: {
            if ((addr & 0x80000) == 0)
                return FlashCs0ReadLong(NULL, memory, addr);
            break;
        }
        case 0x01: {
            break;
        }
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            return T1ReadLong(CartridgeArea->dram, addr & 0x3FFFFF);
        case 0x12:
        case 0x1E:
            if (0x80000)
                return 0xFFFDFFFD;
            break;
        case 0x13:
        case 0x16:
        case 0x17:
        case 0x1A:
        case 0x1B:
        case 0x1F:
            return 0xFFFDFFFD;
        default:
            break;
    }

    return 0xFFFFFFFF;
}

static void FASTCALL AR4MCs0WriteByte(SH2_struct *context, UNUSED u8 *memory, u32 addr, u8 val)
{
    addr &= 0x1FFFFFF;

    switch (addr >> 20) {
        case 0x00: {
            if ((addr & 0x80000) == 0)
                FlashCs0WriteByte(context, memory, addr, val);
            break;
        }
        case 0x01: {
            break;
        }
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            T1WriteByte(CartridgeArea->dram, addr & 0x3FFFFF, val);
            break;
        default:
            break;
    }
}

static void FASTCALL AR4MCs0WriteWord(SH2_struct *context, UNUSED u8 *memory, u32 addr, u16 val)
{
    addr &= 0x1FFFFFF;

    switch (addr >> 20) {
        case 0x00: {
            if ((addr & 0x80000) == 0)
                FlashCs0WriteWord(context, memory, addr, val);
            break;
        }
        case 0x01: {
            break;
        }
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            T1WriteWord(CartridgeArea->dram, addr & 0x3FFFFF, val);
            break;
        default:
            break;
    }
}

static void FASTCALL AR4MCs0WriteLong(SH2_struct *context, UNUSED u8 *memory, u32 addr, u32 val)
{
    addr &= 0x1FFFFFF;

    switch (addr >> 20) {
        case 0x00: {
            if ((addr & 0x80000) == 0)
                FlashCs0WriteLong(context, memory, addr, val);
            break;
        }
        case 0x01: {
            break;
        }
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            T1WriteLong(CartridgeArea->dram, addr & 0x3FFFFF, val);
            break;
        default:
            break;
    }
}

static u8 FASTCALL DRAM8MBITCs0ReadByte(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    addr &= 0x1FFFFFF;

    switch (addr >> 20) {
        case 0x04:
            return T1ReadByte(CartridgeArea->dram, addr & 0x7FFFF);
        case 0x06:
            return T1ReadByte(CartridgeArea->dram, 0x80000 | (addr & 0x7FFFF));
        default:
            break;
    }

    return 0xFF;
}

static u16 FASTCALL DRAM8MBITCs0ReadWord(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    addr &= 0x1FFFFFF;

    switch (addr >> 20) {
        case 0x04:
            return T1ReadWord(CartridgeArea->dram, addr & 0x7FFFF);
        case 0x06:
            return T1ReadWord(CartridgeArea->dram, 0x80000 | (addr & 0x7FFFF));
        default:
            break;
    }

    return 0xFFFF;
}

static u32 FASTCALL DRAM8MBITCs0ReadLong(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    addr &= 0x1FFFFFF;

    switch (addr >> 20) {
        case 0x04:
            return T1ReadLong(CartridgeArea->dram, addr & 0x7FFFF);
        case 0x06:
            return T1ReadLong(CartridgeArea->dram, 0x80000 | (addr & 0x7FFFF));
        default:
            break;
    }

    return 0xFFFFFFFF;
}

static void FASTCALL DRAM8MBITCs0WriteByte(SH2_struct *context, UNUSED u8 *memory, u32 addr, u8 val)
{
    addr &= 0x1FFFFFF;

    switch (addr >> 20) {
        case 0x04:
            T1WriteByte(CartridgeArea->dram, addr & 0x7FFFF, val);
            break;
        case 0x06:
            T1WriteByte(CartridgeArea->dram, 0x80000 | (addr & 0x7FFFF), val);
            break;
        default:
            break;
    }
}

static void FASTCALL DRAM8MBITCs0WriteWord(SH2_struct *context, UNUSED u8 *memory, u32 addr, u16 val)
{
    addr &= 0x1FFFFFF;

    switch (addr >> 20) {
        case 0x04:
            T1WriteWord(CartridgeArea->dram, addr & 0x7FFFF, val);
            break;
        case 0x06:
            T1WriteWord(CartridgeArea->dram, 0x80000 | (addr & 0x7FFFF), val);
            break;
        default:
            break;
    }
}

static void FASTCALL DRAM8MBITCs0WriteLong(SH2_struct *context, UNUSED u8 *memory, u32 addr, u32 val)
{
    addr &= 0x1FFFFFF;

    switch (addr >> 20) {
        case 0x04:
            T1WriteLong(CartridgeArea->dram, addr & 0x7FFFF, val);
            break;
        case 0x06:
            T1WriteLong(CartridgeArea->dram, 0x80000 | (addr & 0x7FFFF), val);
            break;
        default:
            break;
    }
}

static u8 FASTCALL DRAM32MBITCs0ReadByte(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    addr &= 0x1FFFFFF;

    switch (addr >> 20) {
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            return T1ReadByte(CartridgeArea->dram, addr & 0x3FFFFF);
        default:
            break;
    }

    return 0xFF;
}

static u16 FASTCALL DRAM32MBITCs0ReadWord(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    addr &= 0x1FFFFFF;

    switch (addr >> 20) {
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            return T1ReadWord(CartridgeArea->dram, addr & 0x3FFFFF);
        default:
            break;
    }

    return 0xFFFF;
}

static u32 FASTCALL DRAM32MBITCs0ReadLong(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    addr &= 0x1FFFFFF;

    switch (addr >> 20) {
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            return T1ReadLong(CartridgeArea->dram, addr & 0x3FFFFF);
        default:
            break;
    }

    return 0xFFFFFFFF;
}

static void FASTCALL DRAM32MBITCs0WriteByte(SH2_struct *context, UNUSED u8 *memory, u32 addr, u8 val)
{
    addr &= 0x1FFFFFF;

    switch (addr >> 20) {
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            T1WriteByte(CartridgeArea->dram, addr & 0x3FFFFF, val);
            break;
        default:
            break;
    }
}

static void FASTCALL DRAM32MBITCs0WriteWord(SH2_struct *context, UNUSED u8 *memory, u32 addr, u16 val)
{
    addr &= 0x1FFFFFF;

    switch (addr >> 20) {
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            T1WriteWord(CartridgeArea->dram, addr & 0x3FFFFF, val);
            break;
        default:
            break;
    }
}

static void FASTCALL DRAM32MBITCs0WriteLong(SH2_struct *context, UNUSED u8 *memory, u32 addr, u32 val)
{
    addr &= 0x1FFFFFF;

    switch (addr >> 20) {
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            T1WriteLong(CartridgeArea->dram, addr & 0x3FFFFF, val);
            break;
        default:
            break;
    }
}

static u8 FASTCALL BUP4MBITCs1ReadByte(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    addr = addr & ((CART_BUP4MBIT_SIZE << 1) - 1);
    if (addr & 0x1) {
        return T1ReadByte(CartridgeArea->bupram, addr >> 1);
    } else
        return 0xFF;
}

static u16 FASTCALL BUP4MBITCs1ReadWord(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    printf("bup\t: BUP4MBIT read word - %08X\n", addr);
    return 0;
}

static u32 FASTCALL BUP4MBITCs1ReadLong(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    printf("bup\t: BUP4MBIT read long - %08X\n", addr);
    return 0;
}

static void FASTCALL BUP4MBITCs1WriteByte(SH2_struct *context, UNUSED u8 *memory, u32 addr, u8 val)
{
    addr = addr & ((CART_BUP4MBIT_SIZE << 1) - 1);
    if (addr & 0x1) {
        T1WriteByte(CartridgeArea->bupram, addr >> 1, val);
    }
}

static void FASTCALL BUP4MBITCs1WriteWord(SH2_struct *context, UNUSED u8 *memory, u32 addr, u16 val)
{
    printf("bup\t: BUP4MBIT write word - %08X\n", addr);
}

static void FASTCALL BUP4MBITCs1WriteLong(SH2_struct *context, UNUSED u8 *memory, u32 addr, u32 val)
{
    printf("bup\t: BUP4MBIT write long - %08X\n", addr);
}

static u8 FASTCALL BUP8MBITCs1ReadByte(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    addr = addr & ((CART_BUP8MBIT_SIZE << 1) - 1);
    if (addr & 0x1) {
        return T1ReadByte(CartridgeArea->bupram, addr >> 1);
    } else
        return 0xFF;
}

static u16 FASTCALL BUP8MBITCs1ReadWord(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    printf("bup\t: BUP8MBIT read word - %08X\n", addr);
    return 0;
}

static u32 FASTCALL BUP8MBITCs1ReadLong(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    printf("bup\t: BUP8MBIT read long - %08X\n", addr);
    return 0;
}

static void FASTCALL BUP8MBITCs1WriteByte(SH2_struct *context, UNUSED u8 *memory, u32 addr, u8 val)
{
    addr = addr & ((CART_BUP8MBIT_SIZE << 1) - 1);
    if (addr & 0x1) {
        T1WriteByte(CartridgeArea->bupram, addr >> 1, val);
    }
}

static void FASTCALL BUP8MBITCs1WriteWord(SH2_struct *context, UNUSED u8 *memory, u32 addr, u16 val)
{
    printf("bup\t: BUP8MBIT write word - %08X\n", addr);
}

static void FASTCALL BUP8MBITCs1WriteLong(SH2_struct *context, UNUSED u8 *memory, u32 addr, u32 val)
{
    printf("bup\t: BUP8MBIT write long - %08X\n", addr);
}

static u8 FASTCALL BUP16MBITCs1ReadByte(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    addr = addr & ((CART_BUP16MBIT_SIZE << 1) - 1);
    if (addr & 0x1) {
        return T1ReadByte(CartridgeArea->bupram, addr >> 1);
    } else
        return 0xFF;
}

static u16 FASTCALL BUP16MBITCs1ReadWord(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    printf("bup\t: BUP16MBIT read word - %08X\n", addr);
    return 0;
}

static u32 FASTCALL BUP16MBITCs1ReadLong(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    printf("bup\t: BUP16MBIT read long - %08X\n", addr);
    return 0;
}

static void FASTCALL BUP16MBITCs1WriteByte(SH2_struct *context, UNUSED u8 *memory, u32 addr, u8 val)
{
    addr = addr & ((CART_BUP16MBIT_SIZE << 1) - 1);
    if (addr & 0x1) {
        T1WriteByte(CartridgeArea->bupram, addr >> 1, val);
    }
}

static void FASTCALL BUP16MBITCs1WriteWord(SH2_struct *context, UNUSED u8 *memory, u32 addr, u16 val)
{
    printf("bup\t: BUP16MBIT write word - %08X\n", addr);
}

static void FASTCALL BUP16MBITCs1WriteLong(SH2_struct *context, UNUSED u8 *memory, u32 addr, u32 val)
{
    printf("bup\t: BUP16MBIT write long - %08X\n", addr);
}

static u8 FASTCALL BUP32MBITCs1ReadByte(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    addr = addr & ((CART_BUP32MBIT_SIZE << 1) - 1);
    if (addr & 0x1) {
        return T1ReadByte(CartridgeArea->bupram, addr >> 1);
    } else
        return 0xFF;
}

static u16 FASTCALL BUP32MBITCs1ReadWord(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    printf("bup\t: BUP32MBIT read word - %08X\n", addr);
    return 0;
}

static u32 FASTCALL BUP32MBITCs1ReadLong(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    printf("bup\t: BUP32MBIT read long - %08X\n", addr);
    return 0;
}

static void FASTCALL BUP32MBITCs1WriteByte(SH2_struct *context, UNUSED u8 *memory, u32 addr, u8 val)
{
    addr = addr & ((CART_BUP32MBIT_SIZE << 1) - 1);
    if (addr & 0x1) {
        T1WriteByte(CartridgeArea->bupram, addr >> 1, val);
    }
}

static void FASTCALL BUP32MBITCs1WriteWord(SH2_struct *context, UNUSED u8 *memory, u32 addr, u16 val)
{
    printf("bup\t: BUP32MBIT write word - %08X\n", addr);
}

static void FASTCALL BUP32MBITCs1WriteLong(SH2_struct *context, UNUSED u8 *memory, u32 addr, u32 val)
{
    printf("bup\t: BUP32MBIT write long - %08X\n", addr);
}

static u8 FASTCALL BUP128MBITCs1ReadByte(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    u32 ret = T1ReadByte(CartridgeArea->dram, addr & 0xFFFFFF);
    return ret;
}

static u16 FASTCALL BUP128MBITCs1ReadWord(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    u32 ret = T1ReadWord(CartridgeArea->dram, addr & 0xFFFFFF);
    return ret;
}

static u32 FASTCALL BUP128MBITCs1ReadLong(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    u32 ret = T1ReadLong(CartridgeArea->dram, addr & 0xFFFFFF);
    return ret;
}

static void FASTCALL BUP128MBITCs1WriteByte(SH2_struct *context, UNUSED u8 *memory, u32 addr, u8 val)
{
    addr &= 0xFFFFFF;
    T1WriteByte(CartridgeArea->dram, addr, val);
}

static void FASTCALL BUP128MBITCs1WriteWord(SH2_struct *context, UNUSED u8 *memory, u32 addr, u16 val)
{
    addr &= 0xFFFFFF;
    T1WriteWord(CartridgeArea->dram, addr, val);
}

static void FASTCALL BUP128MBITCs1WriteLong(SH2_struct *context, UNUSED u8 *memory, u32 addr, u32 val)
{
    addr &= 0xFFFFFF;
    T1WriteLong(CartridgeArea->dram, addr, val);
}

static u8 FASTCALL ROM16MBITCs0ReadByte(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    return T1ReadByte(CartridgeArea->rom, addr & 0x1FFFFF);
}

static u16 FASTCALL ROM16MBITCs0ReadWord(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    return T1ReadWord(CartridgeArea->rom, addr & 0x1FFFFF);
}

static u32 FASTCALL ROM16MBITCs0ReadLong(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    return T1ReadLong(CartridgeArea->rom, addr & 0x1FFFFF);
}

static void FASTCALL ROM16MBITCs0WriteByte(SH2_struct *context, UNUSED u8 *memory, u32 addr, u8 val)
{
    T1WriteByte(CartridgeArea->rom, addr & 0x1FFFFF, val);
}

static void FASTCALL ROM16MBITCs0WriteWord(SH2_struct *context, UNUSED u8 *memory, u32 addr, u16 val)
{
    T1WriteWord(CartridgeArea->rom, addr & 0x1FFFFF, val);
}

static void FASTCALL ROM16MBITCs0WriteLong(SH2_struct *context, UNUSED u8 *memory, u32 addr, u32 val)
{
    T1WriteLong(CartridgeArea->rom, addr & 0x1FFFFF, val);
}

static u8 FASTCALL ROMSTVCs0ReadByte(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    return T1ReadByte(CartridgeArea->rom, addr & 0x1FFFFFF);
}

static u16 FASTCALL ROMSTVCs0ReadWord(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    u16 ret = T1ReadWord(CartridgeArea->rom, addr & 0x1FFFFFF);
    return ret;
}

static u32 FASTCALL ROMSTVCs0ReadLong(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    u32 ret = T1ReadLong(CartridgeArea->rom, addr & 0x1FFFFFF);
    return ret;
}

static void FASTCALL ROMSTVCs0WriteByte(SH2_struct *context, UNUSED u8 *memory, u32 addr, u8 val)
{
    T1WriteByte(CartridgeArea->rom, addr & 0x1FFFFFF, val);
}

static void FASTCALL ROMSTVCs0WriteWord(SH2_struct *context, UNUSED u8 *memory, u32 addr, u16 val)
{
    T1WriteWord(CartridgeArea->rom, addr & 0x1FFFFFF, val);
}

static void FASTCALL ROMSTVCs0WriteLong(SH2_struct *context, UNUSED u8 *memory, u32 addr, u32 val)
{
    T1WriteLong(CartridgeArea->rom, addr & 0x1FFFFFF, val);
}

static u8 FASTCALL ROMSTVCs1ReadByte(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    return T1ReadByte(&CartridgeArea->rom[0x2000000], addr & 0xFFFFFF);
}

static u16 FASTCALL ROMSTVCs1ReadWord(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    return T1ReadWord(&CartridgeArea->rom[0x2000000], addr & 0xFFFFFF);
}

static u32 FASTCALL ROMSTVCs1ReadLong(SH2_struct *context, UNUSED u8 *memory, u32 addr)
{
    LOGSTV("%s %x\n", __FUNCTION__, addr);
    u8 decryptCmd = addr & 0xF;
    if (decryptOn & 0x1) {
        if (decryptCmd == 0xc) {
            u16 res  = cryptoDecrypt();
            u16 res2 = cryptoDecrypt();
            res      = ((res & 0xff00) >> 8) | ((res & 0x00ff) << 8);
            res2     = ((res2 & 0xff00) >> 8) | ((res2 & 0x00ff) << 8);
            return res2 | (res << 16);
        }
    }
    return T1ReadLong(&CartridgeArea->rom[0x2000000], addr & 0xFFFFFF);
}

static void FASTCALL ROMSTVCs1WriteByte(SH2_struct *context, UNUSED u8 *memory, u32 addr, u8 val)
{
    LOGSTV("%s %x=%x\n", __FUNCTION__, addr, val);
    u8 decryptCmd = addr & 0xF;
    if (decryptCmd == 0x1) {
        decryptOn = val & 0x1;
        return;
    }
    T1WriteByte(&CartridgeArea->rom[0x2000000], addr & 0xFFFFFF, val);
}

static void FASTCALL ROMSTVCs1WriteWord(SH2_struct *context, UNUSED u8 *memory, u32 addr, u16 val)
{
    LOGSTV("%s %x=%x\n", __FUNCTION__, addr, val);
    u8 decryptCmd = addr & 0xF;
    if (decryptCmd == 0x1) {
        decryptOn = val & 0x1;
    } else if (decryptCmd == 0x8) {
        cyptoSetLowAddr(val);
    } else if (decryptCmd == 0xa) {
        cyptoSetHighAddr(val);
    } else if (decryptCmd == 0xc) {
        cyptoSetSubkey(val);
    } else
        T1WriteWord(&CartridgeArea->rom[0x2000000], addr & 0xFFFFFF, val);
}

static void FASTCALL ROMSTVCs1WriteLong(SH2_struct *context, UNUSED u8 *memory, u32 addr, u32 val)
{
    T1WriteLong(&CartridgeArea->rom[0x2000000], addr & 0xFFFFFF, val);
}

int CartInit(const char *filename, int type)
{
    if ((CartridgeArea = (cartridge_struct *)calloc(1, sizeof(cartridge_struct))) == NULL)
        return -1;

    CartridgeArea->carttype = type;
    CartridgeArea->filename = filename;

    CartridgeArea->Cs0ReadByte  = &DummyCs0ReadByte;
    CartridgeArea->Cs0ReadWord  = &DummyCs0ReadWord;
    CartridgeArea->Cs0ReadLong  = &DummyCs0ReadLong;
    CartridgeArea->Cs0WriteByte = &DummyCs0WriteByte;
    CartridgeArea->Cs0WriteWord = &DummyCs0WriteWord;
    CartridgeArea->Cs0WriteLong = &DummyCs0WriteLong;

    CartridgeArea->Cs1ReadByte  = &DummyCs1ReadByte;
    CartridgeArea->Cs1ReadWord  = &DummyCs1ReadWord;
    CartridgeArea->Cs1ReadLong  = &DummyCs1ReadLong;
    CartridgeArea->Cs1WriteByte = &DummyCs1WriteByte;
    CartridgeArea->Cs1WriteWord = &DummyCs1WriteWord;
    CartridgeArea->Cs1WriteLong = &DummyCs1WriteLong;

    CartridgeArea->Cs2ReadByte  = &DummyCs2ReadByte;
    CartridgeArea->Cs2ReadWord  = &DummyCs2ReadWord;
    CartridgeArea->Cs2ReadLong  = &DummyCs2ReadLong;
    CartridgeArea->Cs2WriteByte = &DummyCs2WriteByte;
    CartridgeArea->Cs2WriteWord = &DummyCs2WriteWord;
    CartridgeArea->Cs2WriteLong = &DummyCs2WriteLong;

    decryptOn = 0;

    switch (type) {
        case CART_PAR: {
            if ((CartridgeArea->rom = T2MemoryInit(0x40000)) == NULL)
                return -1;

            if ((CartridgeArea->dram = T1MemoryInit(0x400000)) == NULL)
                return -1;

            CartridgeArea->cartid = 0x5C;

            if (T123Load(CartridgeArea->rom, 0x40000, 2, filename) != 0)
                return -1;

            vendorid = 0x1F;
            deviceid = 0xD5;
            flstate0 = FL_READ;
            flstate1 = FL_READ;

            CartridgeArea->Cs0ReadByte  = &AR4MCs0ReadByte;
            CartridgeArea->Cs0ReadWord  = &AR4MCs0ReadWord;
            CartridgeArea->Cs0ReadLong  = &AR4MCs0ReadLong;
            CartridgeArea->Cs0WriteByte = &AR4MCs0WriteByte;
            CartridgeArea->Cs0WriteWord = &AR4MCs0WriteWord;
            CartridgeArea->Cs0WriteLong = &AR4MCs0WriteLong;
            break;
        }
        case CART_BACKUPRAM4MBIT: {
            if ((CartridgeArea->bupram = T1MemoryInit(CART_BUP4MBIT_SIZE)) == NULL)
                return -1;

            CartridgeArea->cartid = 0x21;

            if (T123Load(CartridgeArea->bupram, CART_BUP4MBIT_SIZE, 1, filename) != 0)
                FormatBackupRam(CartridgeArea->bupram, CART_BUP4MBIT_SIZE);

            CartridgeArea->Cs1ReadByte  = &BUP4MBITCs1ReadByte;
            CartridgeArea->Cs1ReadWord  = &BUP4MBITCs1ReadWord;
            CartridgeArea->Cs1ReadLong  = &BUP4MBITCs1ReadLong;
            CartridgeArea->Cs1WriteByte = &BUP4MBITCs1WriteByte;
            CartridgeArea->Cs1WriteWord = &BUP4MBITCs1WriteWord;
            CartridgeArea->Cs1WriteLong = &BUP4MBITCs1WriteLong;
            break;
        }
        case CART_BACKUPRAM8MBIT: {
            if ((CartridgeArea->bupram = T1MemoryInit(CART_BUP8MBIT_SIZE)) == NULL)
                return -1;

            CartridgeArea->cartid = 0x22;

            if (T123Load(CartridgeArea->bupram, CART_BUP8MBIT_SIZE, 1, filename) != 0)
                FormatBackupRam(CartridgeArea->bupram, CART_BUP8MBIT_SIZE);

            CartridgeArea->Cs1ReadByte  = &BUP8MBITCs1ReadByte;
            CartridgeArea->Cs1ReadWord  = &BUP8MBITCs1ReadWord;
            CartridgeArea->Cs1ReadLong  = &BUP8MBITCs1ReadLong;
            CartridgeArea->Cs1WriteByte = &BUP8MBITCs1WriteByte;
            CartridgeArea->Cs1WriteWord = &BUP8MBITCs1WriteWord;
            CartridgeArea->Cs1WriteLong = &BUP8MBITCs1WriteLong;
            break;
        }
        case CART_BACKUPRAM16MBIT: {
            if ((CartridgeArea->bupram = T1MemoryInit(CART_BUP16MBIT_SIZE)) == NULL)
                return -1;

            CartridgeArea->cartid = 0x23;

            if (T123Load(CartridgeArea->bupram, CART_BUP16MBIT_SIZE, 1, filename) != 0)
                FormatBackupRam(CartridgeArea->bupram, CART_BUP16MBIT_SIZE);

            CartridgeArea->Cs1ReadByte  = &BUP16MBITCs1ReadByte;
            CartridgeArea->Cs1ReadWord  = &BUP16MBITCs1ReadWord;
            CartridgeArea->Cs1ReadLong  = &BUP16MBITCs1ReadLong;
            CartridgeArea->Cs1WriteByte = &BUP16MBITCs1WriteByte;
            CartridgeArea->Cs1WriteWord = &BUP16MBITCs1WriteWord;
            CartridgeArea->Cs1WriteLong = &BUP16MBITCs1WriteLong;
            break;
        }
        case CART_BACKUPRAM32MBIT: {
            if ((CartridgeArea->bupram = T1MemoryInit(CART_BUP32MBIT_SIZE)) == NULL)
                return -1;

            CartridgeArea->cartid = 0x24;
            if (T123Load(CartridgeArea->bupram, CART_BUP32MBIT_SIZE, 1, filename) != 0)
                FormatBackupRam(CartridgeArea->bupram, CART_BUP32MBIT_SIZE);
            CartridgeArea->Cs1ReadByte  = &BUP32MBITCs1ReadByte;
            CartridgeArea->Cs1ReadWord  = &BUP32MBITCs1ReadWord;
            CartridgeArea->Cs1ReadLong  = &BUP32MBITCs1ReadLong;
            CartridgeArea->Cs1WriteByte = &BUP32MBITCs1WriteByte;
            CartridgeArea->Cs1WriteWord = &BUP32MBITCs1WriteWord;
            CartridgeArea->Cs1WriteLong = &BUP32MBITCs1WriteLong;
            break;
        }
        case CART_DRAM128MBIT: {
            if ((CartridgeArea->dram = T1MemoryInit(CART_DRAM128MBIT_SIZE)) == NULL)
                return -1;

            CartridgeArea->cartid       = 0xFF;
            CartridgeArea->Cs1ReadByte  = &BUP128MBITCs1ReadByte;
            CartridgeArea->Cs1ReadWord  = &BUP128MBITCs1ReadWord;
            CartridgeArea->Cs1ReadLong  = &BUP128MBITCs1ReadLong;
            CartridgeArea->Cs1WriteByte = &BUP128MBITCs1WriteByte;
            CartridgeArea->Cs1WriteWord = &BUP128MBITCs1WriteWord;
            CartridgeArea->Cs1WriteLong = &BUP128MBITCs1WriteLong;
            break;
        }
        case CART_DRAM8MBIT: {
            if ((CartridgeArea->dram = T1MemoryInit(0x100000)) == NULL)
                return -1;

            CartridgeArea->cartid = 0x5A;

            CartridgeArea->Cs0ReadByte  = &DRAM8MBITCs0ReadByte;
            CartridgeArea->Cs0ReadWord  = &DRAM8MBITCs0ReadWord;
            CartridgeArea->Cs0ReadLong  = &DRAM8MBITCs0ReadLong;
            CartridgeArea->Cs0WriteByte = &DRAM8MBITCs0WriteByte;
            CartridgeArea->Cs0WriteWord = &DRAM8MBITCs0WriteWord;
            CartridgeArea->Cs0WriteLong = &DRAM8MBITCs0WriteLong;
            break;
        }
        case CART_DRAM32MBIT: {
            if ((CartridgeArea->dram = T1MemoryInit(0x400000)) == NULL)
                return -1;

            CartridgeArea->cartid = 0x5C;

            CartridgeArea->Cs0ReadByte  = &DRAM32MBITCs0ReadByte;
            CartridgeArea->Cs0ReadWord  = &DRAM32MBITCs0ReadWord;
            CartridgeArea->Cs0ReadLong  = &DRAM32MBITCs0ReadLong;
            CartridgeArea->Cs0WriteByte = &DRAM32MBITCs0WriteByte;
            CartridgeArea->Cs0WriteWord = &DRAM32MBITCs0WriteWord;
            CartridgeArea->Cs0WriteLong = &DRAM32MBITCs0WriteLong;
            break;
        }
        case CART_ROM16MBIT: {
            if ((CartridgeArea->rom = T1MemoryInit(0x200000)) == NULL)
                return -1;

            CartridgeArea->cartid = 0xFF;

            if (T123Load(CartridgeArea->rom, 0x200000, 1, filename) != 0)
                return -1;

            CartridgeArea->Cs0ReadByte  = &ROM16MBITCs0ReadByte;
            CartridgeArea->Cs0ReadWord  = &ROM16MBITCs0ReadWord;
            CartridgeArea->Cs0ReadLong  = &ROM16MBITCs0ReadLong;
            CartridgeArea->Cs0WriteByte = &ROM16MBITCs0WriteByte;
            CartridgeArea->Cs0WriteWord = &ROM16MBITCs0WriteWord;
            CartridgeArea->Cs0WriteLong = &ROM16MBITCs0WriteLong;
            break;
        }
        case CART_ROMSTV: {
            if ((CartridgeArea->rom = T1MemoryInit(0x3000000)) == NULL)
                return -1;
            CartridgeArea->cartid       = 0xFF;
            CartridgeArea->Cs0ReadByte  = &ROMSTVCs0ReadByte;
            CartridgeArea->Cs0ReadWord  = &ROMSTVCs0ReadWord;
            CartridgeArea->Cs0ReadLong  = &ROMSTVCs0ReadLong;
            CartridgeArea->Cs0WriteByte = &ROMSTVCs0WriteByte;
            CartridgeArea->Cs0WriteWord = &ROMSTVCs0WriteWord;
            CartridgeArea->Cs0WriteLong = &ROMSTVCs0WriteLong;

            CartridgeArea->Cs1ReadByte  = &ROMSTVCs1ReadByte;
            CartridgeArea->Cs1ReadWord  = &ROMSTVCs1ReadWord;
            CartridgeArea->Cs1ReadLong  = &ROMSTVCs1ReadLong;
            CartridgeArea->Cs1WriteByte = &ROMSTVCs1WriteByte;
            CartridgeArea->Cs1WriteWord = &ROMSTVCs1WriteWord;
            CartridgeArea->Cs1WriteLong = &ROMSTVCs1WriteLong;
            break;
        }
        case CART_USBDEV: {
            if ((CartridgeArea->rom = T2MemoryInit(0x40000)) == NULL)
                return -1;

            if ((CartridgeArea->dram = T1MemoryInit(0x400000)) == NULL)
                return -1;

            CartridgeArea->cartid = 0;

            if (T123Load(CartridgeArea->rom, 0x40000, 2, filename) != 0)
                return -1;

            vendorid = 0xBF;
            deviceid = 0xB5;

            flstate0 = FL_READ;
            flstate1 = FL_READ;

            CartridgeArea->Cs0ReadByte  = &AR4MCs0ReadByte;
            CartridgeArea->Cs0ReadWord  = &AR4MCs0ReadWord;
            CartridgeArea->Cs0ReadLong  = &AR4MCs0ReadLong;
            CartridgeArea->Cs0WriteByte = &AR4MCs0WriteByte;
            CartridgeArea->Cs0WriteWord = &AR4MCs0WriteWord;
            CartridgeArea->Cs0WriteLong = &AR4MCs0WriteLong;
            break;
        }

        default: {
            CartridgeArea->cartid = 0xFF;
            break;
        }
    }

    return 0;
}

void CartFlush(void)
{
    if (CartridgeArea) {
        if (CartridgeArea->carttype == CART_PAR) {
            if (CartridgeArea->rom) {
                if (T123Save(CartridgeArea->rom, 0x40000, 2, CartridgeArea->filename) != 0)
                    YabSetError(YAB_ERR_FILEWRITE, (void *)CartridgeArea->filename);
            }
        }

        if (CartridgeArea->bupram) {
            u32 size = 0;

            switch (CartridgeArea->carttype) {
                case CART_BACKUPRAM4MBIT: {
                    size = CART_BUP4MBIT_SIZE;
                    break;
                }
                case CART_BACKUPRAM8MBIT: {
                    size = CART_BUP8MBIT_SIZE;
                    break;
                }
                case CART_BACKUPRAM16MBIT: {
                    size = CART_BUP16MBIT_SIZE;
                    break;
                }
                case CART_BACKUPRAM32MBIT: {
                    size = CART_BUP32MBIT_SIZE;
                    break;
                }
            }

            if (size != 0) {
                if (T123Save(CartridgeArea->bupram, size, 1, CartridgeArea->filename) != 0)
                    YabSetError(YAB_ERR_FILEWRITE, (void *)CartridgeArea->filename);
            }
        }
    }
}

void CartDeInit(void)
{
    if (CartridgeArea) {
        if (CartridgeArea->carttype == CART_PAR) {
            if (CartridgeArea->rom) {
                if (T123Save(CartridgeArea->rom, 0x40000, 2, CartridgeArea->filename) != 0)
                    YabSetError(YAB_ERR_FILEWRITE, (void *)CartridgeArea->filename);
                T2MemoryDeInit(CartridgeArea->rom);
            }
        } else {
            if (CartridgeArea->rom)
                T1MemoryDeInit(CartridgeArea->rom);
        }

        if (CartridgeArea->bupram) {
            u32 size = 0;

            switch (CartridgeArea->carttype) {
                case CART_BACKUPRAM4MBIT: {
                    size = CART_BUP4MBIT_SIZE;
                    break;
                }
                case CART_BACKUPRAM8MBIT: {
                    size = CART_BUP8MBIT_SIZE;
                    break;
                }
                case CART_BACKUPRAM16MBIT: {
                    size = CART_BUP16MBIT_SIZE;
                    break;
                }
                case CART_BACKUPRAM32MBIT: {
                    size = CART_BUP32MBIT_SIZE;
                    break;
                }
            }

            if (size != 0) {
                if (T123Save(CartridgeArea->bupram, size, 1, CartridgeArea->filename) != 0)
                    YabSetError(YAB_ERR_FILEWRITE, (void *)CartridgeArea->filename);

                T1MemoryDeInit(CartridgeArea->bupram);
            }
        }

        if (CartridgeArea->dram)
            T1MemoryDeInit(CartridgeArea->dram);

        free(CartridgeArea);
    }
    CartridgeArea = NULL;
}

int CartSaveState(void **stream)
{
    int offset;

    offset = MemStateWriteHeader(stream, "CART", 1);

    MemStateWrite((void *)&CartridgeArea->carttype, 4, 1, stream);

    switch (CartridgeArea->carttype) {
        case CART_DRAM8MBIT: {
            MemStateWrite((void *)CartridgeArea->dram, 1, 0x100000, stream);
            break;
        }
        case CART_DRAM32MBIT: {
            MemStateWrite((void *)CartridgeArea->dram, 1, 0x400000, stream);
            break;
        }
    }
    return MemStateFinishHeader(stream, offset);
}

int CartLoadState(const void *stream, UNUSED int version, int size)
{
    int newtype;

    MemStateRead((void *)&newtype, 4, 1, stream);

    if (newtype == CART_DRAM8MBIT || newtype == CART_DRAM32MBIT) {
        if (newtype != CartridgeArea->carttype) {
            CartDeInit();
            CartInit(NULL, newtype);
        }

        switch (CartridgeArea->carttype) {
            case CART_DRAM8MBIT: {
                MemStateRead((void *)CartridgeArea->dram, 1, 0x100000, stream);
                break;
            }
            case CART_DRAM32MBIT: {
                MemStateRead((void *)CartridgeArea->dram, 1, 0x400000, stream);
                break;
            }
        }
    }
    return size;
}
