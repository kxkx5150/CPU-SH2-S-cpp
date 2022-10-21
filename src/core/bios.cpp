

#include "memory.h"
#include "cs0.h"
#include "debug.h"
#include "sh2core.h"
#include "bios.h"
#include "smpc.h"
#include "yabause.h"
#include "error.h"

static u8 sh2masklist[0x20] = {
0xF0, 0xE0, 0xD0, 0xC0, 0xB0, 0xA0, 0x90, 0x80,
0x80, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70,
0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70,
0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70
};

static u32 scumasklist[0x20] = {
0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFC, 0xFFFFFFF8,
0xFFFFFFF0, 0xFFFFFFE0, 0xFFFFFFC0, 0xFFFFFF80,
0xFFFFFF80, 0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00,
0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00,
0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00,
0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00,
0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00,
0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00, 0xFFFFFE00
};

u32 interruptlist[2][0x80];

extern u32 backup_file_addr;
extern u32 backup_file_size;

static void FASTCALL BiosBUPRead(SH2_struct * sh);

void BiosInit(SH2_struct *context)
{
   int i;

   SH2MappedMemoryWriteLong(context, 0x06000600, 0x002B0009);
   SH2MappedMemoryWriteLong(context, 0x06000604, 0xE0F0600C);
   SH2MappedMemoryWriteLong(context, 0x06000608, 0x400E8BFE);
   SH2MappedMemoryWriteLong(context, 0x0600060C, 0x00090009);
   SH2MappedMemoryWriteLong(context, 0x06000610, 0x000B0009);

   for (i = 0; i < 0x200; i+=4)
   {
      SH2MappedMemoryWriteLong(context, 0x06000000+i, 0x06000600);
      SH2MappedMemoryWriteLong(context, 0x06000400+i, 0x06000600);
      interruptlist[0][i >> 2] = 0x06000600;
      interruptlist[1][i >> 2] = 0x06000600;
   }

   SH2MappedMemoryWriteLong(context, 0x06000010, 0x06000604);
   SH2MappedMemoryWriteLong(context, 0x06000018, 0x06000604);
   SH2MappedMemoryWriteLong(context, 0x06000024, 0x06000604);
   SH2MappedMemoryWriteLong(context, 0x06000028, 0x06000604);
   interruptlist[0][4] = 0x06000604;
   interruptlist[0][6] = 0x06000604;
   interruptlist[0][9] = 0x06000604;
   interruptlist[0][10] = 0x06000604;

   SH2MappedMemoryWriteLong(context, 0x06000410, 0x06000604);
   SH2MappedMemoryWriteLong(context, 0x06000418, 0x06000604);
   SH2MappedMemoryWriteLong(context, 0x06000424, 0x06000604);
   SH2MappedMemoryWriteLong(context, 0x06000428, 0x06000604);
   interruptlist[1][4] = 0x06000604;
   interruptlist[1][6] = 0x06000604;
   interruptlist[1][9] = 0x06000604;
   interruptlist[1][10] = 0x06000604;

   for (i = 0; i < 0x38; i+=4)
   {
      SH2MappedMemoryWriteLong(context, 0x06000100+i, 0x00000400+i);
      interruptlist[0][0x40+(i >> 2)] = 0x00000400+i;
   }

   for (i = 0; i < 0x40; i+=4)
   {
      SH2MappedMemoryWriteLong(context, 0x06000140+i, 0x00000440+i);
      interruptlist[0][0x50+(i >> 2)] = 0x00000440+i;
   }

   for (i = 0; i < 0x100; i+=4)
      SH2MappedMemoryWriteLong(context, 0x06000A00+i, 0x06000610);

   SH2MappedMemoryWriteLong(context, 0x06000210, 0x00000210);
   SH2MappedMemoryWriteLong(context, 0x0600026C, 0x0000026C);
   SH2MappedMemoryWriteLong(context, 0x06000274, 0x00000274);
   SH2MappedMemoryWriteLong(context, 0x06000280, 0x00000280);
   SH2MappedMemoryWriteLong(context, 0x0600029C, 0x0000029C);
   SH2MappedMemoryWriteLong(context, 0x060002DC, 0x000002DC);
   SH2MappedMemoryWriteLong(context, 0x06000300, 0x00000300);
   SH2MappedMemoryWriteLong(context, 0x06000304, 0x00000304);
   SH2MappedMemoryWriteLong(context, 0x06000310, 0x00000310);
   SH2MappedMemoryWriteLong(context, 0x06000314, 0x00000314);
   SH2MappedMemoryWriteLong(context, 0x06000320, 0x00000320);
   SH2MappedMemoryWriteLong(context, 0x06000324, 0x00000000);
   SH2MappedMemoryWriteLong(context, 0x06000330, 0x00000330);
   SH2MappedMemoryWriteLong(context, 0x06000334, 0x00000334);
   SH2MappedMemoryWriteLong(context, 0x06000340, 0x00000340);
   SH2MappedMemoryWriteLong(context, 0x06000344, 0x00000344);
   SH2MappedMemoryWriteLong(context, 0x06000348, 0xFFFFFFFF);
   SH2MappedMemoryWriteLong(context, 0x06000354, 0x00000000);
   SH2MappedMemoryWriteLong(context, 0x06000358, 0x00000358);
}


static void FASTCALL BiosSetScuInterrupt(SH2_struct * sh)
{
   SH2GetRegisters(sh, &sh->regs);


   if (sh->regs.R[5] == 0)
   {
      SH2MappedMemoryWriteLong(sh, 0x06000900+(sh->regs.R[4] << 2), 0x06000610);
   }
   else
   {
      SH2MappedMemoryWriteLong(sh, 0x06000900+(sh->regs.R[4] << 2), sh->regs.R[5]);
   }

   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosGetScuInterrupt(SH2_struct * sh)
{
   SH2GetRegisters(sh, &sh->regs);


   sh->regs.R[0] = SH2MappedMemoryReadLong(sh, 0x06000900+(sh->regs.R[4] << 2));

   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosSetSh2Interrupt(SH2_struct * sh)
{
   SH2GetRegisters(sh, &sh->regs);


   if (sh->regs.R[5] == 0)
   {
      SH2MappedMemoryWriteLong(sh, sh->regs.VBR+(sh->regs.R[4] << 2), interruptlist[sh->isslave][sh->regs.R[4]]);
   }
   else
   {
      SH2MappedMemoryWriteLong(sh, sh->regs.VBR+(sh->regs.R[4] << 2), sh->regs.R[5]);
   }

   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosGetSh2Interrupt(SH2_struct * sh)
{
   SH2GetRegisters(sh, &sh->regs);


   sh->regs.R[0] = SH2MappedMemoryReadLong(sh, sh->regs.VBR+(sh->regs.R[4] << 2));

   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosSetScuInterruptMask(SH2_struct * sh)
{
   SH2GetRegisters(sh, &sh->regs);


   if (!sh->isslave)
   {
      SH2MappedMemoryWriteLong(sh, 0x06000348, sh->regs.R[4]);
      SH2MappedMemoryWriteLong(sh, 0x25FE00A0, sh->regs.R[4]);
	  SH2MappedMemoryWriteLong(sh, 0x25FE00A4, sh->regs.R[4]);
   }

   if (!(sh->regs.R[4] & 0x8000))
      SH2MappedMemoryWriteLong(sh, 0x25FE00A8, 1);

   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosChangeScuInterruptMask(SH2_struct * sh)
{
   u32 newmask;

   SH2GetRegisters(sh, &sh->regs);


   newmask = (SH2MappedMemoryReadLong(sh, 0x06000348) & sh->regs.R[4]) | sh->regs.R[5];
   if (!sh->isslave)
   {
      SH2MappedMemoryWriteLong(sh, 0x06000348, newmask);
      SH2MappedMemoryWriteLong(sh, 0x25FE00A0, newmask);
      SH2MappedMemoryWriteLong(sh, 0x25FE00A4, (u32)(s16)sh->regs.R[4]);
   }

   if (!(sh->regs.R[4] & 0x8000))
      SH2MappedMemoryWriteLong(sh, 0x25FE00A8, 1);

   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosCDINIT2(SH2_struct * sh)
{
   SH2GetRegisters(sh, &sh->regs);
   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosCDINIT1(SH2_struct * sh)
{
   SH2GetRegisters(sh, &sh->regs);
   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosGetSemaphore(SH2_struct * sh)
{
   u8 temp;

   SH2GetRegisters(sh, &sh->regs);


   if ((temp = MappedMemoryReadByte(sh, 0x06000B00 + sh->regs.R[4])) == 0)
      sh->regs.R[0] = 1;
   else
      sh->regs.R[0] = 0;

   temp |= 0x80;
   MappedMemoryWriteByte(sh, 0x06000B00 + sh->regs.R[4], temp);

   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosClearSemaphore(SH2_struct * sh)
{
   SH2GetRegisters(sh, &sh->regs);


   MappedMemoryWriteByte(sh, 0x06000B00 + sh->regs.R[4], 0);

   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosChangeSystemClock(SH2_struct * sh)
{
   int i, j;
   u32 mask;
   SH2GetRegisters(sh, &sh->regs);


   SH2MappedMemoryWriteLong(sh, 0x06000324, sh->regs.R[4]);

   SH2MappedMemoryWriteLong(sh, 0x25FE00A8, 0);
   SH2MappedMemoryWriteLong(sh, 0x25FE00B8, 0);

   SH2MappedMemoryWriteByte(sh, 0xFFFFFE91, 0x80);
   SH2MappedMemoryWriteWord(sh, 0xFFFFFE80, 0xA51D);
   SH2MappedMemoryWriteWord(sh, 0xFFFFFEE0, 0x8000);

   if (sh->regs.R[4] == 0)
      SmpcCKCHG320();
   else
      SmpcCKCHG352();

   for (j = 0; j < 3; j++)
   {
      for (i = 0; i < 7; i++)
         SH2MappedMemoryWriteLong(sh, 0x25FE0000+(j*0xC)+(i*4), 0);
   }

   MappedMemoryWriteLong(sh, 0x25FE0060, 0);
   MappedMemoryWriteLong(sh, 0x25FE0080, 0);
   MappedMemoryWriteLong(sh, 0x25FE00B0, 0x1FF01FF0);
   MappedMemoryWriteLong(sh, 0x25FE00B4, 0x1FF01FF0);
   MappedMemoryWriteLong(sh, 0x25FE00B8, 0x1F);
   MappedMemoryWriteLong(sh, 0x25FE00A8, 0x1);
   MappedMemoryWriteLong(sh, 0x25FE0090, 0x3FF);
   MappedMemoryWriteLong(sh, 0x25FE0094, 0x1FF);
   MappedMemoryWriteLong(sh, 0x25FE0098, 0);

   mask = SH2MappedMemoryReadLong(sh, 0x06000348);
   SH2MappedMemoryWriteLong(sh, 0x25FE00A0, mask);

   if (!(mask & 0x8000))
      SH2MappedMemoryWriteLong(sh, 0x25FE00A8, 1);

   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosChangeScuInterruptPriority(SH2_struct * sh)
{
   int i;

   SH2GetRegisters(sh, &sh->regs);


   for (i = 0; i < 0x20; i++)
   {
      scumasklist[i] = SH2MappedMemoryReadLong(sh, sh->regs.R[4]+(i << 2));
      sh2masklist[i] = (scumasklist[i] >> 16);
      if (scumasklist[i] & 0x8000)
         scumasklist[i] |= 0xFFFF0000;
      else
         scumasklist[i] &= 0x0000FFFF;
   }

   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosExecuteCDPlayer(SH2_struct * sh)
{
   SH2GetRegisters(sh, &sh->regs);


   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosPowerOnMemoryClear(SH2_struct * sh)
{
   SH2GetRegisters(sh, &sh->regs);


   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosCheckMPEGCard(SH2_struct * sh)
{
   SH2GetRegisters(sh, &sh->regs);


   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static u32 GetDeviceStats(u32 device, u32 *size, u32 *addr, u32 *blocksize)
{
   switch(device)
   {
      case 0:
          *addr = backup_file_addr;
          *size = backup_file_size;
          *blocksize = 0x40;
        return 0;
      case 1:
         if ((CartridgeArea->cartid & 0xF0) == 0x20)
         {
            *addr = 0x04000000;
            *size = 0x40000 << (CartridgeArea->cartid & 0x0F);
            if (CartridgeArea->cartid == 0x24)
               *blocksize = 0x400;
            else
               *blocksize = 0x200;

            return 0;
         }
         else
            return 1;
      default:
         *addr = 0;
         *size = 0;
         *blocksize = 0;
         return 1;
   }

   return 1;
}


static int CheckHeader(UNUSED u32 device)
{
   return 0;
}


static int CalcSaveSize(SH2_struct *context, u32 tableaddr, int blocksize)
{
   int numblocks=0;


   for(;;)
   {
       u16 block;
       if (((tableaddr - 1) & ((blocksize << 1) - 1)) == 0) {
         tableaddr += 8;
       }
       block = (MappedMemoryReadByte(context, tableaddr) << 8) | MappedMemoryReadByte(context, tableaddr + 2);
       if (block == 0)
         break;
       tableaddr += 4;
       numblocks++;
   }

   return numblocks;
}


static u32 GetFreeSpace(SH2_struct *context, UNUSED u32 device, u32 size, u32 addr, u32 blocksize)
{
   u32 i;
   u32 usedblocks=0;

   for (i = ((2 * blocksize) << 1); i < (size << 1); i += (blocksize << 1))
   {
      if (((s8)MappedMemoryReadByte(context, addr + i + 1)) < 0)
      {
         usedblocks += (CalcSaveSize(context, addr+i+0x45, blocksize) + 1);
      }
   }

   return ((size / blocksize) - 2 - usedblocks);
}


static u32 FindSave(SH2_struct *context, UNUSED u32 device, u32 stringaddr, u32 blockoffset, u32 size, u32 addr, u32 blocksize)
{
   u32 i;

   for (i = ((blockoffset * blocksize) << 1); i < (size << 1); i += (blocksize << 1))
   {
      if (((s8)MappedMemoryReadByte(context, addr + i + 1)) < 0)
      {
         int i3;

         for (i3 = 0; i3 < 11; i3++)
         {
            u8 data = MappedMemoryReadByte(context, stringaddr+i3);

            if (MappedMemoryReadByte(context, addr+i+0x9+(i3*2)) != data)
            {
               if (data == 0)
                  return ((i / blocksize) >> 1);
               else
                  i3 = 12;
            }
            else
            {
               if (i3 == 10 || data == 0)
                  return ((i / blocksize) >> 1);
            }
         }
      }
   }

   return 0;
}



static u32 FindSave2(SH2_struct *context, UNUSED u32 device, const char *string, u32 blockoffset, u32 size, u32 addr, u32 blocksize)
{
   u32 i;

   for (i = ((blockoffset * blocksize) << 1); i < (size << 1); i += (blocksize << 1))
   {
      if (((s8)MappedMemoryReadByte(context, addr + i + 1)) < 0)
      {
         int i3;

         for (i3 = 0; i3 < 11; i3++)
         {
            if (MappedMemoryReadByte(context, addr+i+0x9+(i3*2)) != string[i3])
            {
               if (string[i3] == 0)
                  return ((i / blocksize) >> 1);
               else
                  i3 = 12;
            }
            else
            {
               if (i3 == 10 || string[i3] == 0)
                  return ((i / blocksize) >> 1);
            }
         }
      }
   }

   return 0;
}


static void DeleteSave(SH2_struct *context, u32 addr, u32 blockoffset, u32 blocksize)
{
    MappedMemoryWriteByte(context, addr + (blockoffset * blocksize * 2) + 0x1, 0x00);
}


static u16 *GetFreeBlocks(SH2_struct *context, u32 addr, u32 blocksize, u32 numblocks, u32 size)
{
   u8 *blocktbl;
   u16 *freetbl;
   u32 tableaddr;
   u32 i;
   u32 blockcount=0;

   if ((blocktbl = (u8 *)malloc(sizeof(u8) * (size / blocksize))) == NULL)
      return NULL;

   memset(blocktbl, 0, (size / blocksize));

   for (i = ((2 * blocksize) << 1); i < (size << 1); i += (blocksize << 1))
   {
      if (((s8)MappedMemoryReadByte(context, addr + i + 1)) < 0)
      {
         tableaddr = addr+i+0x45;
         blocktbl[i / (blocksize << 1)] = 1;

         for(;;)
         {
            u16 block;
            if (((tableaddr-1) & ((blocksize << 1) - 1)) == 0)
               tableaddr += 8;
            block = (MappedMemoryReadByte(context, tableaddr) << 8) | MappedMemoryReadByte(context, tableaddr + 2);
            if (block == 0)
               break;
            tableaddr += 4;
            blocktbl[block] = 1;
         }
      }
   }

   if ((freetbl = (u16 *)malloc(sizeof(u16) * numblocks)) == NULL)
   {
      free(blocktbl);
      return NULL;
   }

   for (i = 2; i < (size / blocksize); i++)
   {
      if (blocktbl[i] == 0)
      {
         freetbl[blockcount] = (u16)i;
         blockcount++;

         if (blockcount >= numblocks)
            break;
      }
   }

   free(blocktbl);

   return freetbl;
}


static u16 *ReadBlockTable(SH2_struct *context, u32 addr, u32 *tableaddr, int block, int blocksize, int *numblocks, int *blocksread)
{
   u16 *blocktbl = NULL;
   int i=0;
   int allocsize = 32;
   int index = 0;

   tableaddr[0] = addr + (block * blocksize * 2) + 0x45;
   blocksread[0]=0;

   if ((blocktbl = (u16 *)malloc(sizeof(u16) * allocsize)) == NULL)
      return NULL;

   for(;;)
   {
       u16 block;
       block = (MappedMemoryReadByte(context, tableaddr[0]) << 8) | MappedMemoryReadByte(context, tableaddr[0] + 2);
       if (block == 0) {
         break;
       }
       tableaddr[0] += 4;
       if (((tableaddr[0]-1) & ((blocksize << 1) - 1)) == 0)
       {
          tableaddr[0] = addr + (blocktbl[blocksread[0]] * blocksize * 2) + 9;
          blocksread[0]++;
       }
       blocktbl[index] = block;
       index++;
       if (index >= allocsize ) {
         allocsize *= 2;
         blocktbl = (u16 *)realloc(blocktbl, sizeof(u16) * allocsize);
       }
   }

   tableaddr[0] += 4;
   if (((tableaddr[0] - 1) & ((blocksize << 1) - 1)) == 0)
   {
     tableaddr[0] = addr + (blocktbl[blocksread[0]] * blocksize * 2) + 9;
     blocksread[0]++;
   }

   return blocktbl;
}


void FASTCALL BiosBUPInit(SH2_struct * sh)
{
   SH2GetRegisters(sh, &sh->regs);

   LOG("BiosBUPInit. arg1 = %08X, arg2 = %08X, arg3 = %08X\n", sh->regs.R[4], sh->regs.R[5], sh->regs.R[6]);

   SH2MappedMemoryWriteLong(sh, 0x06000354, sh->regs.R[5]);
   SH2MappedMemoryWriteLong(sh, sh->regs.R[5]+0x00, 0x00000380);
   SH2MappedMemoryWriteLong(sh, sh->regs.R[5]+0x04, 0x00000384);
   SH2MappedMemoryWriteLong(sh, sh->regs.R[5]+0x08, 0x00000388);
   SH2MappedMemoryWriteLong(sh, sh->regs.R[5]+0x0C, 0x0000038C);
   SH2MappedMemoryWriteLong(sh, sh->regs.R[5]+0x10, 0x00000390);
   SH2MappedMemoryWriteLong(sh, sh->regs.R[5]+0x14, 0x00000394);
   SH2MappedMemoryWriteLong(sh, sh->regs.R[5]+0x18, 0x00000398);
   SH2MappedMemoryWriteLong(sh, sh->regs.R[5]+0x1C, 0x0000039C);
   SH2MappedMemoryWriteLong(sh, sh->regs.R[5]+0x20, 0x000003A0);
   SH2MappedMemoryWriteLong(sh, sh->regs.R[5]+0x24, 0x000003A4);
   SH2MappedMemoryWriteLong(sh, sh->regs.R[5]+0x28, 0x000003A8);
   SH2MappedMemoryWriteLong(sh, sh->regs.R[5]+0x2C, 0x000003AC);


   MappedMemoryWriteWord(sh, sh->regs.R[6], 1);
   MappedMemoryWriteWord(sh, sh->regs.R[6]+0x2, 1);

   if ((CartridgeArea->cartid & 0xF0) == 0x20)
   {
      MappedMemoryWriteWord(sh, sh->regs.R[6]+0x4, 2);
      MappedMemoryWriteWord(sh, sh->regs.R[6]+0x6, 1);
   }
   else
   {
      MappedMemoryWriteWord(sh, sh->regs.R[6]+0x4, 0);
      MappedMemoryWriteWord(sh, sh->regs.R[6]+0x6, 0);
   }

   MappedMemoryWriteWord(sh, sh->regs.R[6]+0x08, 0);
   MappedMemoryWriteWord(sh, sh->regs.R[6]+0x0A, 0);


   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosBUPSelectPartition(SH2_struct * sh)
{
   u32 size;
   u32 addr;
   u32 blocksize;
   u32 ret;
   u32 freeblocks = 0;
   u32 needsize;
   int aftersize;

   SH2GetRegisters(sh, &sh->regs);

   LOG("BiosBUPSelectPartition. PR = %08X device = %d \n", sh->regs.PR, sh->regs.R[4] );

   ret = GetDeviceStats(sh->regs.R[4], &size, &addr, &blocksize);
   if (ret != 0) {
     sh->regs.R[0] = ret;
     sh->regs.PC = sh->regs.PR;
     SH2SetRegisters(sh, &sh->regs);
     return;
   }

   sh->regs.R[0] = 0;
   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosBUPFormat(SH2_struct * sh)
{
   SH2GetRegisters(sh, &sh->regs);


   BupFormat(sh->regs.R[4]);

   sh->regs.R[0] = 0;
   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosBUPStatus(SH2_struct * sh)
{
   u32 size;
   u32 addr;
   u32 blocksize;
   u32 ret;
   u32 freeblocks=0;
   u32 needsize;
   int aftersize;

   SH2GetRegisters(sh, &sh->regs);

   LOG("BiosBUPStatus. arg1 = %d, arg2 = %d, arg3 = %08X, PR = %08X\n", sh->regs.R[4], sh->regs.R[5], sh->regs.R[6], sh->regs.PR);

   ret = GetDeviceStats(sh->regs.R[4], &size, &addr, &blocksize);

   if (ret == 1 || CheckHeader(sh->regs.R[4]) != 0)
   {
      sh->regs.R[0] = ret;
      sh->regs.PC = sh->regs.PR;
      SH2SetRegisters(sh, &sh->regs);
      return;
   }

   freeblocks = GetFreeSpace(sh, sh->regs.R[4], size, addr, blocksize);

   needsize = sh->regs.R[5];
   aftersize = (((blocksize - 6) * freeblocks) - 30) - needsize;
   if (aftersize < 0) aftersize = 0;

   SH2MappedMemoryWriteLong(sh, sh->regs.R[6], size);
   SH2MappedMemoryWriteLong(sh, sh->regs.R[6]+0x4, size / blocksize);
   SH2MappedMemoryWriteLong(sh, sh->regs.R[6]+0x8, blocksize);
   SH2MappedMemoryWriteLong(sh, sh->regs.R[6]+0xC, ((blocksize - 6) * freeblocks) - 30);
   SH2MappedMemoryWriteLong(sh, sh->regs.R[6]+0x10, freeblocks);
   SH2MappedMemoryWriteLong(sh, sh->regs.R[6]+0x14, aftersize / blocksize);


   sh->regs.R[0] = ret;
   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosBUPWrite(SH2_struct * sh)
{
   u32 size;
   u32 addr;
   u32 blocksize;
   u32 block;
   u32 ret;
   u32 savesize;
   u16 *blocktbl;
   u32 workaddr;
   u32 blockswritten=0;
   u32 datasize;
   u32 i;
   u8 val;

   SH2GetRegisters(sh, &sh->regs);

   LOG("BiosBUPWrite. arg1 = %d, arg2 = %08X, arg3 = %08X, arg4 = %d, PR = %08X\n", sh->regs.R[4], sh->regs.R[5], sh->regs.R[6], sh->regs.R[7], sh->regs.PR);

   ret = GetDeviceStats(sh->regs.R[4], &size, &addr, &blocksize);
   if (ret == 1)
   {
      sh->regs.R[0] = ret;
      sh->regs.PC = sh->regs.PR;
      SH2SetRegisters(sh, &sh->regs);
      return;
   }

   if ((block = FindSave(sh, sh->regs.R[4], sh->regs.R[5], 2, size, addr, blocksize)) != 0)
   {

      if (sh->regs.R[7] != 0)
      {
         sh->regs.R[0] = 6;
         sh->regs.PC = sh->regs.PR;
         SH2SetRegisters(sh, &sh->regs);
         return;
      }

      DeleteSave(sh, addr, block, blocksize);
   }

   datasize = SH2MappedMemoryReadLong(sh, sh->regs.R[5]+0x1C);
   savesize = 1 + ((datasize + 0x1D) / (blocksize - 6));

   if (savesize > GetFreeSpace(sh, sh->regs.R[4], size, addr, blocksize))
   {
      sh->regs.R[0] = 4;
      sh->regs.PC = sh->regs.PR;
      SH2SetRegisters(sh, &sh->regs);
      return;
   }

   if ((blocktbl = GetFreeBlocks(sh, addr, blocksize, savesize, size)) == NULL)
   {
      sh->regs.R[0] = 8;
      sh->regs.PC = sh->regs.PR;
      SH2SetRegisters(sh, &sh->regs);
      return;
   }

   workaddr = addr + (blocktbl[0] * blocksize * 2);

   MappedMemoryWriteByte(sh, workaddr+0x1, 0x80);

   for (i = workaddr+0x9; i < ((workaddr+0x9) + (11 * 2)); i+=2)
   {
      MappedMemoryWriteByte(sh, i, MappedMemoryReadByte(sh, sh->regs.R[5]));
      sh->regs.R[5]++;
   }

   sh->regs.R[5]++;

   for (i = workaddr+0x21; i < ((workaddr+0x21) + (10 * 2)); i+=2)
   {
      MappedMemoryWriteByte(sh, i, MappedMemoryReadByte(sh, sh->regs.R[5]));
      sh->regs.R[5]++;
   }

   MappedMemoryWriteByte(sh, workaddr+0x1F, MappedMemoryReadByte(sh, sh->regs.R[5]));
   sh->regs.R[5]++;

   sh->regs.R[5]++;

   for (i = workaddr+0x35; i < ((workaddr+0x35) + (4 * 2)); i+=2)
   {
      MappedMemoryWriteByte(sh, i, MappedMemoryReadByte(sh, sh->regs.R[5]));
      sh->regs.R[5]++;
   }

   for (i = workaddr+0x3D; i < ((workaddr+0x3D) + (4 * 2)); i+=2)
   {
      MappedMemoryWriteByte(sh, i, MappedMemoryReadByte(sh, sh->regs.R[5]));
      sh->regs.R[5]++;
   }

   workaddr += 0x45;
   for (i = 1; i < savesize; i++)
   {
      MappedMemoryWriteByte(sh, workaddr, (u8)(blocktbl[i] >> 8));
      MappedMemoryWriteByte(sh, workaddr + 2, (u8)blocktbl[i]);
      LOG("write block %08X,%d,%04X", workaddr, i, blocktbl[i]);
      workaddr += 4;
      if (((workaddr-1) & ((blocksize << 1) - 1)) == 0) {
         blockswritten++;
         workaddr = addr + (blocktbl[blockswritten] * blocksize * 2) + 9;
      }
   }

   MappedMemoryWriteByte(sh, workaddr, 0);
   workaddr+=2;
   if (((workaddr - 1) & ((blocksize << 1) - 1)) == 0) {
     blockswritten++;
     workaddr = addr + (blocktbl[blockswritten] * blocksize * 2) + 9;
     LOG("BiosBUPWrite %d, block move %08X, %d/%d", __LINE__, workaddr, blockswritten, savesize);
   }

   MappedMemoryWriteByte(sh, workaddr, 0);
   workaddr+=2;
   if (((workaddr - 1) & ((blocksize << 1) - 1)) == 0) {
     blockswritten++;
     workaddr = addr + (blocktbl[blockswritten] * blocksize * 2) + 9;
     LOG("BiosBUPWrite %d, block move %08X, %d/%d", __LINE__, workaddr, blockswritten, savesize);
   }

   LOG("BiosBUPWrite from %08X size %08X", sh->regs.R[6], datasize);


   while (datasize > 0)
   {
      if (((workaddr-1) & ((blocksize << 1) - 1)) == 0)
      {
         blockswritten++;
         workaddr = addr + (blocktbl[blockswritten] * blocksize * 2) + 9;
         LOG("BiosBUPWrite %d, block move %08X, %d/%d", __LINE__, workaddr, blockswritten, savesize);
      }
      val = MappedMemoryReadByte(sh, sh->regs.R[6]);
      MappedMemoryWriteByte(sh, workaddr, val);

      datasize--;
      sh->regs.R[6]++;
      workaddr+=2;
   }
   free(blocktbl);

   YabFlushBackups();

   sh->regs.R[0] = 0;
   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}




static void FASTCALL BiosBUPDelete(SH2_struct * sh)
{
   u32 size;
   u32 addr;
   u32 blocksize;
   u32 block;
   u32 ret;

   SH2GetRegisters(sh, &sh->regs);

   LOG("BiosBUPDelete. PR = %08X\n", sh->regs.PR);

   ret = GetDeviceStats(sh->regs.R[4], &size, &addr, &blocksize);
   if (ret == 1)
   {
      sh->regs.R[0] = ret;
      sh->regs.PC = sh->regs.PR;
      SH2SetRegisters(sh, &sh->regs);
      return;
   }

   if ((block = FindSave(sh, sh->regs.R[4], sh->regs.R[5], 2, size, addr, blocksize)) == 0)
   {

      sh->regs.R[0] = 5;
      sh->regs.PC = sh->regs.PR;
      SH2SetRegisters(sh, &sh->regs);
      return;
   }

   DeleteSave(sh, addr, block, blocksize);

   sh->regs.R[0] = 0;
   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosBUPDirectory(SH2_struct * sh)
{
   u32 size;
   u32 addr;
   u32 blocksize;
   u32 ret;
   u32 i;
   char filename[12];
   u32 blockoffset=2;

   SH2GetRegisters(sh, &sh->regs);

   for (i = 0; i < 12; i++)
      filename[i] = MappedMemoryReadByte(sh, sh->regs.R[5]+i);

   LOG("BiosBUPDirectory. arg1 = %d, arg2 = %s, arg3 = %08X, arg4 = %08X, PR = %08X\n", sh->regs.R[4], filename, sh->regs.R[6], sh->regs.R[7], sh->regs.PR);

   ret = GetDeviceStats(sh->regs.R[4], &size, &addr, &blocksize);

   if (ret == 1)
   {
     sh->regs.R[0] = 0;
      sh->regs.PC = sh->regs.PR;
      SH2SetRegisters(sh, &sh->regs);
      return;
   }

   for (i = 0; i < 256; i++)
   {
      u32 block = FindSave(sh, sh->regs.R[4], sh->regs.R[5], blockoffset, size, addr, blocksize);

      if (block == 0)
         break;

      blockoffset = block + 1;
      block = addr + (blocksize * block * 2);
   }

   if (sh->regs.R[6] < i)
   {
      sh->regs.R[0] = -(s32)i;
      sh->regs.PC = sh->regs.PR;
      SH2SetRegisters(sh, &sh->regs);
      return;
   }

   blockoffset = 2;

   for (i = 0; i < sh->regs.R[6]; i++)
   {
      u32 i4;
      u32 datasize=0;
      u32 block = FindSave(sh, sh->regs.R[4], sh->regs.R[5], blockoffset, size, addr, blocksize);

      if (block == 0)
         break;

      blockoffset = block+1;

      block = addr + (blocksize * block * 2);

      for (i4 = block+0x9; i4 < ((block+0x9) + (11 * 2)); i4+=2)
      {
         MappedMemoryWriteByte(sh, sh->regs.R[7], MappedMemoryReadByte(sh, i4));
         sh->regs.R[7]++;
      }
      MappedMemoryWriteByte(sh, sh->regs.R[7], 0);
      sh->regs.R[7]++;

      for (i4 = block+0x21; i4 < ((block+0x21) + (10 * 2)); i4+=2)
      {
         MappedMemoryWriteByte(sh, sh->regs.R[7], MappedMemoryReadByte(sh, i4));
         sh->regs.R[7]++;
      }

      MappedMemoryWriteByte(sh, sh->regs.R[7], MappedMemoryReadByte(sh, block+0x1F));
      sh->regs.R[7]++;

      MappedMemoryWriteByte(sh, sh->regs.R[7], 0);
      sh->regs.R[7]++;

      for (i4 = block+0x35; i4 < ((block+0x35) + (4 * 2)); i4+=2)
      {
         MappedMemoryWriteByte(sh, sh->regs.R[7], MappedMemoryReadByte(sh, i4));
         sh->regs.R[7]++;
      }

      for (i4 = block+0x3D; i4 < ((block+0x3D) + (4 * 2)); i4+=2)
      {
         u8 data;
         datasize <<= 8;
         data = MappedMemoryReadByte(sh, i4);
         MappedMemoryWriteByte(sh, sh->regs.R[7], data);
         datasize |= data;
         sh->regs.R[7]++;
      }

      MappedMemoryWriteWord(sh, sh->regs.R[7], (u16)(((datasize + 0x1D) / (blocksize - 6)) + 1));
      sh->regs.R[7] += 4;
   }

   sh->regs.R[0] = i;
   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosBUPVerify(SH2_struct * sh)
{
   u32 size;
   u32 addr;
   u32 blocksize;
   u32 block;
   u32 ret;
   u32 tableaddr;
   u32 datasize;
   u16 *blocktbl;
   int numblocks;
   int blocksread;

   SH2GetRegisters(sh, &sh->regs);

   LOG("BiosBUPVerify. PR = %08X\n", sh->regs.PR);

   ret = GetDeviceStats(sh->regs.R[4], &size, &addr, &blocksize);

   if (ret == 1)
   {
      sh->regs.R[0] = ret;
      sh->regs.PC = sh->regs.PR;
      SH2SetRegisters(sh, &sh->regs);
      return;
   }

   if ((block = FindSave(sh, sh->regs.R[4], sh->regs.R[5], 2, size, addr, blocksize)) == 0)
   {
      sh->regs.R[0] = 5;
      sh->regs.PC = sh->regs.PR;
      SH2SetRegisters(sh, &sh->regs);
      return;
   }

   tableaddr = addr + (block * blocksize * 2) + 0x3D;
   datasize = (MappedMemoryReadByte(sh, tableaddr) << 24) | (MappedMemoryReadByte(sh, tableaddr + 2) << 16) |
              (MappedMemoryReadByte(sh, tableaddr+4) << 8) | MappedMemoryReadByte(sh, tableaddr + 6);

   if ((blocktbl = ReadBlockTable(sh, addr, &tableaddr, block, blocksize, &numblocks, &blocksread)) == NULL)
   {
      sh->regs.R[0] = 8;
      sh->regs.PC = sh->regs.PR;
      SH2SetRegisters(sh, &sh->regs);
      return;
   }

   while (datasize > 0)
   {
      if (((tableaddr-1) & ((blocksize << 1) - 1)) == 0)
      {
         tableaddr = addr + (blocktbl[blocksread] * blocksize * 2) + 9;
         blocksread++;
      }
      if (MappedMemoryReadByte(sh, sh->regs.R[6]) != MappedMemoryReadByte(sh, tableaddr))
      {
         LOG("BiosBUPVerify. failed at %08X  want = %02X get = %08X\n", tableaddr, MappedMemoryReadByte(sh, sh->regs.R[6]), MappedMemoryReadByte(sh, tableaddr));
         free(blocktbl);
         sh->regs.R[0] = 7;
         sh->regs.PC = sh->regs.PR;
         SH2SetRegisters(sh, &sh->regs);
         return;
      }

      datasize--;
      sh->regs.R[6]++;
      tableaddr+=2;
   }

   free(blocktbl);

   sh->regs.R[0] = 0;
   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void ConvertMonthAndDay(SH2_struct *sh, u32 data, u32 monthaddr, u32 dayaddr, int type)
{
   int i;
   u16 monthtbl[11] = { 31, 31+28, 31+28+31, 31+28+31+30, 31+28+31+30+31,
                        31+28+31+30+31+30, 31+28+31+30+31+30+31,
                        31+28+31+30+31+30+31+31, 31+28+31+30+31+30+31+31+30,
                        31+28+31+30+31+30+31+31+30+31,
                        31+28+31+30+31+30+31+31+30+31+30 };

   if (data < monthtbl[0])
   {
      MappedMemoryWriteByte(sh, monthaddr, 1);

      MappedMemoryWriteByte(sh, dayaddr, (u8)(data + 1));
      return;
   }

   for (i = 1; i < 11; i++)
   {
      if (data <= monthtbl[i])
         break;
   }

   if (type == 1)
   {
      MappedMemoryWriteByte(sh, monthaddr, (u8)(i + 1));

      if ((i + 1) == 2)
         MappedMemoryWriteByte(sh, dayaddr, (u8)(data - monthtbl[(i - 1)] + 1));
      else
         MappedMemoryWriteByte(sh, dayaddr, (u8)(data - monthtbl[(i - 1)]));
   }
   else
   {
      MappedMemoryWriteByte(sh, monthaddr, (u8)(i + 1));

      MappedMemoryWriteByte(sh, dayaddr, (u8)(data - monthtbl[(i - 1)] + 1));
   }
}

static void ConvertMonthAndDayMem(u32 data, u8* monthaddr, u8 * dayaddr, int type)
{
   int i;
   u16 monthtbl[11] = { 31, 31+28, 31+28+31, 31+28+31+30, 31+28+31+30+31,
                        31+28+31+30+31+30, 31+28+31+30+31+30+31,
                        31+28+31+30+31+30+31+31, 31+28+31+30+31+30+31+31+30,
                        31+28+31+30+31+30+31+31+30+31,
                        31+28+31+30+31+30+31+31+30+31+30 };

   if (data < monthtbl[0])
   {
      *monthaddr = 1;

      *dayaddr = (u8)(data + 1);
      return;
   }

   for (i = 1; i < 11; i++)
   {
      if (data <= monthtbl[i])
         break;
   }

   if (type == 1)
   {
      *monthaddr = (u8)(i + 1);

      if ((i + 1) == 2)
         *dayaddr = (u8)(data - monthtbl[(i - 1)] + 1);
      else
         *dayaddr = (u8)(data - monthtbl[(i - 1)]);
   }
   else
   {
      *monthaddr = (u8)(i + 1);

      *dayaddr = (u8)(data - monthtbl[(i - 1)] + 1);
   }
}



static void FASTCALL BiosBUPGetDate(SH2_struct * sh)
{
   u32 date;
   u32 div;
   u32 yearoffset;
   u32 yearremainder;

   SH2GetRegisters(sh, &sh->regs);

   LOG("BiosBUPGetDate. PR = %08X\n", sh->regs.PR);

   date = sh->regs.R[4];

   MappedMemoryWriteByte(sh, sh->regs.R[5]+3, (u8)((date % 0x5A0) / 0x3C));

   MappedMemoryWriteByte(sh, sh->regs.R[5]+4, (u8)(date % 0x3C));

   div = date / 0x5A0;

   if (div > 0xAB71)
      MappedMemoryWriteByte(sh, sh->regs.R[5]+5, (u8)((div + 1) % 7));
   else
      MappedMemoryWriteByte(sh, sh->regs.R[5]+5, (u8)((div + 2) % 7));

   yearremainder = div % 0x5B5;

   if (yearremainder > 0x16E)
   {
      yearoffset = (yearremainder - 1) / 0x16D;
      ConvertMonthAndDay(sh, (yearremainder - 1) % 0x16D, sh->regs.R[5]+1, sh->regs.R[5]+2, 0);
   }
   else
   {
      yearoffset = 0;
      ConvertMonthAndDay(sh, 0, sh->regs.R[5]+1, sh->regs.R[5]+2, 1);
   }

   MappedMemoryWriteByte(sh, sh->regs.R[5], (u8)(((div / 0x5B5) * 4) + yearoffset));

   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosBUPSetDate(SH2_struct * sh)
{
   u32 date;
   u8 data;
   u32 remainder;
   u16 monthtbl[11] = { 31, 31+28, 31+28+31, 31+28+31+30, 31+28+31+30+31,
                        31+28+31+30+31+30, 31+28+31+30+31+30+31,
                        31+28+31+30+31+30+31+31, 31+28+31+30+31+30+31+31+30,
                        31+28+31+30+31+30+31+31+30+31,
                        31+28+31+30+31+30+31+31+30+31+30 };

   SH2GetRegisters(sh, &sh->regs);

   LOG("BiosBUPSetDate. PR = %08X\n", sh->regs.PR);

   data = MappedMemoryReadByte(sh, sh->regs.R[4]);
   date = (data / 4) * 0x5B5;
   remainder = data % 4;
   if (remainder)
      date += (remainder * 0x16D) + 1;

   data = MappedMemoryReadByte(sh, sh->regs.R[4]+1);
   if (data != 1 && data < 13)
   {
      date += monthtbl[data - 2];
      if (date > 2 && remainder == 0)
         date++;
   }

   date += MappedMemoryReadByte(sh, sh->regs.R[4]+2) - 1;
   date *= 0x5A0;

   date += (MappedMemoryReadByte(sh, sh->regs.R[4]+3) * 0x3C);

   date += MappedMemoryReadByte(sh, sh->regs.R[4]+4);

   sh->regs.R[0] = date;
   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosHandleScuInterrupt(SH2_struct * sh, int vector)
{
   SH2GetRegisters(sh, &sh->regs);

   sh->regs.R[15] -= 4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[15], sh->regs.R[0]);
   sh->regs.R[15] -= 4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[15], sh->regs.R[1]);
   sh->regs.R[15] -= 4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[15], sh->regs.R[2]);
   sh->regs.R[15] -= 4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[15], sh->regs.R[3]);
   sh->regs.R[15] -= 4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[15], SH2MappedMemoryReadLong(sh, 0x06000348));
   sh->regs.R[15] -= 4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[15], sh->regs.R[4]);
   sh->regs.R[15] -= 4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[15], sh->regs.R[5]);
   sh->regs.R[15] -= 4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[15], sh->regs.R[6]);
   sh->regs.R[15] -= 4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[15], sh->regs.R[7]);
   sh->regs.R[15] -= 4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[15], sh->regs.PR);
   sh->regs.R[15] -= 4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[15], sh->regs.GBR);

   sh->regs.SR.all = (u32)sh2masklist[vector - 0x40];

   SH2MappedMemoryWriteLong(sh, 0x06000348, SH2MappedMemoryReadLong(sh, 0x06000348) | scumasklist[vector - 0x40]);
   SH2MappedMemoryWriteLong(sh, 0x25FE00A0, SH2MappedMemoryReadLong(sh, 0x06000348) | scumasklist[vector - 0x40]);

   sh->regs.PR = 0x00000480;

   sh->regs.PC = SH2MappedMemoryReadLong(sh, 0x06000900+(vector << 2));

   SH2SetRegisters(sh, &sh->regs);
}


static void FASTCALL BiosHandleScuInterruptReturn(SH2_struct * sh)
{
   u32 oldmask;

   SH2GetRegisters(sh, &sh->regs);

   sh->regs.GBR = SH2MappedMemoryReadLong(sh, sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.PR = SH2MappedMemoryReadLong(sh, sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.R[7] = SH2MappedMemoryReadLong(sh, sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.R[6] = SH2MappedMemoryReadLong(sh, sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.R[5] = SH2MappedMemoryReadLong(sh, sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.R[4] = SH2MappedMemoryReadLong(sh, sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.SR.all = 0xF0;
   oldmask = SH2MappedMemoryReadLong(sh, sh->regs.R[15]);
   SH2MappedMemoryWriteLong(sh, 0x06000348, oldmask);
   SH2MappedMemoryWriteLong(sh, 0x25FE00A0, oldmask);
   sh->regs.R[15] += 4;
   sh->regs.R[3] = SH2MappedMemoryReadLong(sh, sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.R[2] = SH2MappedMemoryReadLong(sh, sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.R[1] = SH2MappedMemoryReadLong(sh, sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.R[0] = SH2MappedMemoryReadLong(sh, sh->regs.R[15]);
   sh->regs.R[15] += 4;

   sh->regs.PC = SH2MappedMemoryReadLong(sh, sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.SR.all = SH2MappedMemoryReadLong(sh, sh->regs.R[15]) & 0x000003F3;
   sh->regs.R[15] += 4;

   SH2SetRegisters(sh, &sh->regs);
}


int FASTCALL BiosHandleFunc(SH2_struct * sh)
{
   int addr = (sh->regs.PC & 0xFFFFF);
   SH2GetRegisters(sh, &sh->regs);
   switch((addr - 0x200) >> 2)
   {
      case 0x04:
         BiosPowerOnMemoryClear(sh);
         break;
      case 0x1B:
         BiosExecuteCDPlayer(sh);
         break;
      case 0x1D:
         BiosCheckMPEGCard(sh);
         break;
      case 0x20:
         BiosChangeScuInterruptPriority(sh);
         break;
      case 0x27:
         BiosCDINIT2(sh);
         break;
      case 0x37:
         BiosCDINIT1(sh);
         break;
      case 0x40:
         BiosSetScuInterrupt(sh);
         break;
      case 0x41:
         BiosGetScuInterrupt(sh);
         break;
      case 0x44:
         BiosSetSh2Interrupt(sh);
         break;
      case 0x45:
         BiosGetSh2Interrupt(sh);
         break;
      case 0x48:
         BiosChangeSystemClock(sh);
         break;
      case 0x4C:
         BiosGetSemaphore(sh);
         break;
      case 0x4D:
         BiosClearSemaphore(sh);
         break;
      case 0x50:
         BiosSetScuInterruptMask(sh);
         break;
      case 0x51:
         BiosChangeScuInterruptMask(sh);
         break;
      case 0x56:
         BiosBUPInit(sh);
         break;
      case 0x60:
         break;
      case 0x61:
         BiosBUPSelectPartition(sh);
         break;
      case 0x62:
         BiosBUPFormat(sh);
         break;
      case 0x63:
         BiosBUPStatus(sh);
         break;
      case 0x64:
         BiosBUPWrite(sh);
         break;
      case 0x65:
         BiosBUPRead(sh);
         break;
      case 0x66:
         BiosBUPDelete(sh);
         break;
      case 0x67:
         BiosBUPDirectory(sh);
         break;
      case 0x68:
         BiosBUPVerify(sh);
         break;
      case 0x69:
         BiosBUPGetDate(sh);
         break;
      case 0x6A:
         BiosBUPSetDate(sh);
         break;
      case 0x6B:
         break;
      case 0x80:
      case 0x81:
      case 0x82:
      case 0x83:
      case 0x84:
      case 0x85:
      case 0x86:
      case 0x87:
      case 0x88:
      case 0x89:
      case 0x8A:
      case 0x8B:
      case 0x8C:
      case 0x8D:
      case 0x90:
      case 0x91:
      case 0x92:
      case 0x93:
      case 0x94:
      case 0x95:
      case 0x96:
      case 0x97:
      case 0x98:
      case 0x99:
      case 0x9A:
      case 0x9B:
      case 0x9C:
      case 0x9D:
      case 0x9E:
      case 0x9F:
         BiosHandleScuInterrupt(sh, (addr - 0x300) >> 2);
         break;
      case 0xA0:
         BiosHandleScuInterruptReturn(sh);
         break;
      default:
         return 0;
   }

   return 1;
}


deviceinfo_struct *BupGetDeviceList(int *numdevices)
{
   deviceinfo_struct *device;
   int devicecount=1;

   if ((CartridgeArea->cartid & 0xF0) == 0x20)
      devicecount++;

   if ((device = (deviceinfo_struct *)malloc(devicecount * sizeof(deviceinfo_struct))) == NULL)
   {
      *numdevices = 0;
      return NULL;
   }

   *numdevices = devicecount;

   device[0].id = 0;
   sprintf(device[0].name, "Internal Backup RAM");

   if ((CartridgeArea->cartid & 0xF0) == 0x20)
   {
      device[1].id = 1;
      sprintf(device[1].name, "%d Mbit Backup RAM Cartridge", 1 << ((CartridgeArea->cartid & 0xF)+1));
   }


   return device;
}


int BupGetStats(SH2_struct *sh, u32 device, u32 *freespace, u32 *maxspace)
{
   u32 ret;
   u32 size;
   u32 addr;
   u32 blocksize;

   ret = GetDeviceStats(device, &size, &addr, &blocksize);

   if (ret == 1 || CheckHeader(device) != 0)
      return 0;

   *maxspace = size / blocksize;
   *freespace = GetFreeSpace(sh, device, size, addr, blocksize);

   return 1;
}


saveinfo_struct *BupGetSaveList(SH2_struct *sh, u32 device, int *numsaves)
{
   u32 ret;
   u32 size;
   u32 addr;
   u32 blocksize;
   saveinfo_struct *save;
   int savecount=0;
   u32 i, j;
   u32 workaddr;
   u32 date;
   u32 div;
   u32 yearremainder;
   u32 yearoffset;

   ret = GetDeviceStats(device, &size, &addr, &blocksize);

   if (ret == 1 || CheckHeader(device) != 0)
   {
      *numsaves = 0;
      return NULL;
   }

   for (i = ((2 * blocksize) << 1); i < (size << 1); i += (blocksize << 1))
   {
      if (((s8)MappedMemoryReadByte(sh, addr + i + 1)) < 0)
         savecount++;
   }

   if ((save = (saveinfo_struct *)malloc(savecount * sizeof(saveinfo_struct))) == NULL)
   {
      *numsaves = 0;
      return NULL;
   }

   *numsaves = savecount;

   savecount = 0;

   for (i = ((2 * blocksize) << 1); i < (size << 1); i += (blocksize << 1))
   {
      if (((s8)MappedMemoryReadByte(sh, addr + i + 1)) < 0)
      {
         workaddr = addr + i;

         for (j = 0; j < 11; j++)
            save[savecount].filename[j] = MappedMemoryReadByte(sh, workaddr+0x9+(j * 2));
         save[savecount].filename[11] = '\0';

         for (j = 0; j < 10; j++)
            save[savecount].comment[j] = MappedMemoryReadByte(sh, workaddr+0x21+(j * 2));
         save[savecount].comment[10] = '\0';

         save[savecount].language = MappedMemoryReadByte(sh, workaddr+0x1F);

         date = (MappedMemoryReadByte(sh, workaddr+0x35) << 24) |
                    (MappedMemoryReadByte(sh, workaddr+0x37) << 16) |
                    (MappedMemoryReadByte(sh, workaddr+0x39) << 8) |
                    MappedMemoryReadByte(sh, workaddr+0x3B);

        save[savecount].date = date;
        save[savecount].hour = (u8)((date % 0x5A0) / 0x3C);
        save[savecount].minute = (u8)(date % 0x3C);

        div = date / 0x5A0;
        if (div > 0xAB71)
          save[savecount].week = (u8)((div + 1) % 7);
        else
          save[savecount].week = (u8)((div + 2) % 7);

        yearremainder = div % 0x5B5;
        if (yearremainder > 0x16E)
        {
           yearoffset = (yearremainder - 1) / 0x16D;
           ConvertMonthAndDayMem((yearremainder - 1) % 0x16D, &save[savecount].month, &save[savecount].day, 0);
        }
        else
        {
           yearoffset = 0;
           ConvertMonthAndDayMem((yearremainder - 1), &save[savecount].month, &save[savecount].day, 1);
        }
        save[savecount].year = (u8)(((div / 0x5B5) * 4) + yearoffset);

         save[savecount].datasize = (MappedMemoryReadByte(sh, workaddr+0x3D) << 24) |
                                    (MappedMemoryReadByte(sh, workaddr+0x3F) << 16) |
                                    (MappedMemoryReadByte(sh, workaddr+0x41) << 8) |
                                    MappedMemoryReadByte(sh, workaddr+0x43);

         save[savecount].blocksize = CalcSaveSize(sh, workaddr+0x45, blocksize) + 1;
         savecount++;
      }
   }

   return save;
}


int BupDeleteSave(SH2_struct *sh, u32 device, const char *savename)
{
   u32 ret;
   u32 size;
   u32 addr;
   u32 blocksize;
   u32 block;

   ret = GetDeviceStats(device, &size, &addr, &blocksize);

   if (ret == 1 || CheckHeader(device) != 0)
      return -1;

   if ((block = FindSave2(sh, device, savename, 2, size, addr, blocksize)) != 0)
   {
      DeleteSave(sh, addr, block, blocksize);
      return 0;
   }

   return -2;
}

extern FILE * pbackup;

void BupFormat(u32 device)
{
   switch (device)
   {
      case 0:
         FormatBackupRam(BupRam, backup_file_size);
         break;
      case 1:
         if ((CartridgeArea->cartid & 0xF0) == 0x20)
         {
            switch (CartridgeArea->cartid & 0xF)
            {
               case 1:
                  FormatBackupRam(CartridgeArea->bupram, CART_BUP4MBIT_SIZE);
                  break;
               case 2:
                  FormatBackupRam(CartridgeArea->bupram, CART_BUP8MBIT_SIZE);
                  break;
               case 3:
                  FormatBackupRam(CartridgeArea->bupram, CART_BUP16MBIT_SIZE);
                  break;
               case 4:
                  FormatBackupRam(CartridgeArea->bupram, CART_BUP32MBIT_SIZE);
                  break;
               default: break;
            }
         }
         break;
      case 2:
         LOG("Formatting FDD not supported\n");
      default: break;
   }
}


int BupCopySave(UNUSED u32 srcdevice, UNUSED u32 dstdevice, UNUSED const char *savename)
{
   return 0;
}


int BupImportSave(UNUSED u32 device, const char *filename)
{
   FILE *fp;
   long filesize;
   u8 *buffer;
   size_t num_read = 0;

   if (!filename)
      return -1;

   if ((fp = fopen(filename, "rb")) == NULL)
      return -1;

   fseek(fp, 0, SEEK_END);
   filesize = ftell(fp);

   if (filesize <= 0)
   {
      YabSetError(YAB_ERR_FILEREAD, filename);
      fclose(fp);
      return -1;
   }

   fseek(fp, 0, SEEK_SET);

   if ((buffer = (u8 *)malloc(filesize)) == NULL)
   {
      fclose(fp);
      return -2;
   }

   num_read = fread((void *)buffer, 1, filesize, fp);
   fclose(fp);


   free(buffer);
   return 0;
}


int BupExportSave(UNUSED u32 device, UNUSED const char *savename, UNUSED const char *filename)
{
   return 0;
}

int BiosBUPExport(SH2_struct *sh, u32 device, const char *savename, char ** buf, int * bufsize )
{
  u32 ret;
  u32 size;
  u32 addr;
  u32 blocksize;
  u32 block;
  u32 tableaddr;
  u16 *blocktbl;
  int numblocks;
  int blocksread;
  u32 datasize;
  char fname[11];
  int i;

  ret = GetDeviceStats(device, &size, &addr, &blocksize);

  if (ret == 1 || CheckHeader(device) != 0)
     return -1;

  if ((block = FindSave2(sh, device, savename, 2, size, addr, blocksize)) == 0) {
     LOG("%s is not found on this device(%d)", savename, device);
     return -1;
  }

  tableaddr = addr + (block * blocksize * 2) + 0x3D;
  datasize = (MappedMemoryReadByte(sh, tableaddr) << 24) | (MappedMemoryReadByte(sh, tableaddr + 2) << 16) |
             (MappedMemoryReadByte(sh, tableaddr+4) << 8) | MappedMemoryReadByte(sh, tableaddr + 6);

  LOG("tableaddr=%08X, datasize = %d", tableaddr, datasize );

  if ((blocktbl = ReadBlockTable(sh, addr, &tableaddr, block, blocksize, &numblocks, &blocksread)) == NULL)
  {
    LOG("ReadBlockTable failed", tableaddr, datasize );
    return -1;
  }

  LOG("BiosBUPExport from %08X size %08X", tableaddr, datasize);
  *buf = (char*)malloc(datasize);
  if( (*buf) ==NULL ){
    LOG("Failed to allocate *buf");
    return -1;
  }
  *bufsize = datasize;

  i=0;
  while (datasize > 0)
  {
    (*buf)[i] = MappedMemoryReadByte(sh, tableaddr);
     datasize--;
     i++;
     tableaddr+=2;

     if (((tableaddr-1) & ((blocksize << 1) - 1)) == 0)
     {
        tableaddr = addr + (blocktbl[blocksread] * blocksize * 2) + 9;
        blocksread++;
     }
  }
  free(blocktbl);
  LOG("BiosBUPExport success!");
   return 0;
}

int BiosBUPImport(SH2_struct *sh, u32 device, saveinfo_struct * saveinfo, const char * buf, int bufsize )
{
   u32 size;
   u32 addr;
   u32 blocksize;
   u32 block;
   u32 ret;
   u32 savesize;
   u16 *blocktbl;
   u32 workaddr;
   u32 blockswritten=0;
   u32 datasize;
   u32 i;
   u32 rindex;


   ret = GetDeviceStats(device, &size, &addr, &blocksize);
   if (ret == 1)
   {
      return -1;
   }

   if ((block = FindSave2(sh, device, saveinfo->filename, 2, size, addr, blocksize)) != 0)
   {
      DeleteSave(sh, addr, block, blocksize);
   }

   datasize = bufsize;
   savesize = (datasize + 0x1D) / (blocksize - 6);
   if ((datasize + 0x1D) % (blocksize - 6))
      savesize++;

   if (savesize > GetFreeSpace(sh, device, size, addr, blocksize))
   {
      return 4;
   }

   if ((blocktbl = GetFreeBlocks(sh, addr, blocksize, savesize, size)) == NULL)
   {
      return 8;
   }

   workaddr = addr + (blocktbl[0] * blocksize * 2);

   MappedMemoryWriteByte(sh, workaddr+0x1, 0x80);

   rindex = 0;
   for (i = workaddr+0x9; i < ((workaddr+0x9) + (11 * 2)); i+=2)
   {
      MappedMemoryWriteByte(sh, i, saveinfo->filename[rindex]);
      rindex++;
   }

   rindex = 0;
   for (i = workaddr+0x21; i < ((workaddr+0x21) + (10 * 2)); i+=2)
   {
      MappedMemoryWriteByte(sh, i, saveinfo->comment[rindex]);
      rindex++;
   }

   MappedMemoryWriteByte(sh, workaddr+0x1F, saveinfo->language);

  MappedMemoryWriteByte(sh, workaddr+0x35, (saveinfo->date>>24)&0xFF);
  MappedMemoryWriteByte(sh, workaddr+0x37, (saveinfo->date>>16)&0xFF);
  MappedMemoryWriteByte(sh, workaddr+0x39, (saveinfo->date>>8)&0xFF);
  MappedMemoryWriteByte(sh, workaddr+0x3B, saveinfo->date&0xFF);


  MappedMemoryWriteByte(sh, workaddr+0x3D, (saveinfo->datasize>>24)&0xFF);
  MappedMemoryWriteByte(sh, workaddr+0x3F, (saveinfo->datasize>>16)&0xFF);
  MappedMemoryWriteByte(sh, workaddr+0x41, (saveinfo->datasize>>8)&0xFF);
  MappedMemoryWriteByte(sh, workaddr+0x43, saveinfo->datasize&0xFF);

   workaddr += 0x45;

   for (i = 1; i < savesize; i++)
   {
      MappedMemoryWriteByte(sh, workaddr, (u8)(blocktbl[i] >> 8));
      workaddr+=2;
      MappedMemoryWriteByte(sh, workaddr, (u8)blocktbl[i]);
      workaddr+=2;

      if (((workaddr-1) & ((blocksize << 1) - 1)) == 0)
      {
         blockswritten++;
         workaddr = addr + (blocktbl[blockswritten] * blocksize * 2) + 9;
      }
   }

   MappedMemoryWriteByte(sh, workaddr, 0);
   workaddr+=2;
   if (((workaddr - 1) & ((blocksize << 1) - 1)) == 0)
   {
     blockswritten++;
     workaddr = addr + (blocktbl[blockswritten] * blocksize * 2) + 9;
   }

   MappedMemoryWriteByte(sh, workaddr, 0);
   workaddr+=2;
   if (((workaddr - 1) & ((blocksize << 1) - 1)) == 0)
   {
     blockswritten++;
     workaddr = addr + (blocktbl[blockswritten] * blocksize * 2) + 9;
   }

   LOG("BiosBUPWrite from %08X size %08X", workaddr, datasize);

   rindex=0;
   while (datasize > 0)
   {
      MappedMemoryWriteByte(sh, workaddr, buf[rindex]);


      datasize--;
      rindex++;
      workaddr+=2;

      if (((workaddr-1) & ((blocksize << 1) - 1)) == 0)
      {
         blockswritten++;
         workaddr = addr + (blocktbl[blockswritten] * blocksize * 2) + 9;
      }
   }
   free(blocktbl);
   return 0;
}



static void FASTCALL BiosBUPRead(SH2_struct * sh)
{
  u32 size;
  u32 addr;
  u32 blocksize;
  u32 block;
  u32 ret;
  u32 tableaddr;
  u16 *blocktbl;
  int numblocks;
  int blocksread;
  u32 datasize;
  char fname[11];
  int i;
  u8 val;

  SH2GetRegisters(sh, &sh->regs);

  for (i = 0; i < 10; i++) {
    fname[i] = MappedMemoryReadByte(sh, sh->regs.R[5]+i);
  }
  fname[10] = 0;

   LOG("BiosBUPRead rtn=%08X device=%d, %s, \n", sh->regs.PR, sh->regs.R[4], fname);

   ret = GetDeviceStats(sh->regs.R[4], &size, &addr, &blocksize);

   if (ret == 1)
   {
      sh->regs.R[0] = ret;
      sh->regs.PC = sh->regs.PR;
      SH2SetRegisters(sh, &sh->regs);
      return;
   }

   if ((block = FindSave(sh, sh->regs.R[4], sh->regs.R[5], 2, size, addr, blocksize)) == 0)
   {
      LOG("BiosBUPRead not found");
      sh->regs.R[0] = 5;
      sh->regs.PC = sh->regs.PR;
      SH2SetRegisters(sh, &sh->regs);
      return;
   }

   tableaddr = addr + (block * blocksize * 2) + 0x3D;
   datasize = (MappedMemoryReadByte(sh, tableaddr) << 24) | (MappedMemoryReadByte(sh, tableaddr + 2) << 16) |
              (MappedMemoryReadByte(sh, tableaddr+4) << 8) | MappedMemoryReadByte(sh, tableaddr + 6);

   if ((blocktbl = ReadBlockTable(sh, addr, &tableaddr, block, blocksize, &numblocks, &blocksread)) == NULL)
   {
      sh->regs.R[0] = 8;
      sh->regs.PC = sh->regs.PR;
      SH2SetRegisters(sh, &sh->regs);
      return;
   }

   LOG("BiosBUPRead from %08X size %08X to %08X", tableaddr, datasize, sh->regs.R[6] );

   while (datasize > 0)
   {
      if (((tableaddr-1) & ((blocksize << 1) - 1)) == 0)
      {
         tableaddr = addr + (blocktbl[blocksread] * blocksize * 2) + 9;
         blocksread++;
      }
      val = MappedMemoryReadByte(sh, tableaddr);
      MappedMemoryWriteByte(sh, sh->regs.R[6], val);
      datasize--;
      sh->regs.R[6]++;
      tableaddr+=2;

   }
   free(blocktbl);

   sh->regs.R[0] = 0;
   sh->regs.PC = sh->regs.PR;
   SH2SetRegisters(sh, &sh->regs);
}



int BiosBUPStatusMem(SH2_struct *sh, int device, devicestatus_struct * status )
{
   u32 size;
   u32 addr;
   u32 blocksize;
   u32 ret;
   u32 freeblocks=0;
   u32 needsize;
   int aftersize;


   ret = GetDeviceStats(device, &size, &addr, &blocksize);

   if (ret == 1 )
   {
      return -1;
   }

   freeblocks = GetFreeSpace(sh, device, size, addr, blocksize);

   needsize = 0;
   aftersize = (((blocksize - 6) * freeblocks) - 30) - needsize;
   if (aftersize < 0) aftersize = 0;

   status->totalsize = size;
   status->totalblock = size / blocksize;
   status->blocksize = blocksize;
   status->freesize = ((blocksize - 6) * freeblocks) - 30;
   status->freeblock = freeblocks;
   status->datanum = aftersize / blocksize;
   return 0;
}

